#include "formats/ba2/ba2_gnrl_parser.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_metadata_materialization.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/binary_io.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

struct gnrl_record {
    std::uint32_t name_hash;
    std::array<std::byte, 4> extension;
    std::uint32_t directory_hash;
    std::uint32_t unknown;
    std::uint64_t offset;
    std::uint32_t packed_size;
    std::uint32_t size;
};

using detail::add_fits_u64;
using detail::multiply_fits;

/// Decodes the fixed-width GNRL records from the already-bounded record table.
result<std::vector<gnrl_record>> read_records(detail::binary_reader& reader,
                                              std::uint32_t file_count) {
    std::vector<gnrl_record> records;
    auto reserved = detail::reserve_metadata_vector(records, file_count, "BA2 GNRL records");
    if (!reserved) {
        return reserved.error();
    }
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto name_hash = reader.read_u32_le();
        auto extension_bytes = reader.read_bytes(4U);
        const auto directory_hash = reader.read_u32_le();
        const auto unknown = reader.read_u32_le();
        const auto offset = reader.read_u64_le();
        const auto packed_size = reader.read_u32_le();
        const auto size = reader.read_u32_le();
        const auto sentinel = reader.read_u32_le();
        if (!name_hash || !extension_bytes || !directory_hash || !unknown || !offset ||
            !packed_size || !size || !sentinel) {
            return error{error_code::format_error, "BA2 GNRL record table is truncated"};
        }
        if (sentinel.value() != ba2_record_sentinel) {
            return error{error_code::format_error, "BA2 GNRL record BAADF00D sentinel is invalid"};
        }
        std::array<std::byte, 4> extension{};
        std::copy(extension_bytes.value().begin(), extension_bytes.value().end(),
                  extension.begin());
        records.push_back(gnrl_record{name_hash.value(), extension, directory_hash.value(),
                                      unknown.value(), offset.value(), packed_size.value(),
                                      size.value()});
    }
    return records;
}

/// GNRL wording for BA2 Metadata Materialization; every string is verbatim.
constexpr ba2_metadata_wording gnrl_metadata_wording{
    "BA2 GNRL filename table entries",
    "BA2 GNRL filename table is truncated before UInt16 length",
    "BA2 GNRL filename table is truncated before name bytes",
    "BA2 GNRL filename table contains an empty name",
    "BA2 GNRL filename length",
    "BA2 GNRL filename bytes",
    "BA2 GNRL entry payload span is outside the archive",
    "BA2 GNRL entry payload span intersects header or record table",
    "BA2 GNRL filename table intersects payload data",
    "BA2 GNRL entry payload spans partially overlap",
    "BA2 GNRL contains duplicate canonical archive paths",
    "BA2 GNRL canonical path set",
};

/// Validates record/name relationships and builds deterministic public entries.
///
/// Record naming and payload span admission run through `materialization`,
/// which may throw `std::bad_alloc`; the guard below translates it into GNRL
/// entry-metadata wording.
result<std::vector<entry_metadata>> materialize_entries(
    ba2_metadata_materialization& materialization, std::span<const gnrl_record> records,
    const ba2_profile& profile) {
    try {
        std::vector<entry_metadata> entries;
        auto reserved_entries =
            detail::reserve_metadata_vector(entries, records.size(), "BA2 GNRL entry metadata");
        if (!reserved_entries) {
            return reserved_entries.error();
        }

        for (std::size_t index = 0; index < records.size(); ++index) {
            // The first call also reserves the seen-canonical-path set, so it
            // must come after the entry-vector reservation above.
            const ba2_stored_record_identity stored_identity{
                records[index].name_hash, records[index].directory_hash, records[index].extension};
            auto named = materialization.name_record(index, stored_identity);
            if (!named) {
                return named.error();
            }

            const auto stored_size =
                records[index].packed_size != 0U ? records[index].packed_size : records[index].size;
            // The call stays here, interleaved with the record's other checks
            // (after its duplicate-path check, before the next record), because
            // which diagnostic an archive with several defects reports is
            // observable behavior.
            auto admitted = materialization.admit_payload_span(records[index].offset, stored_size);
            if (!admitted) {
                return admitted.error();
            }

            auto entry = entry_metadata{std::move(named.value().canonical_path),
                                        std::move(named.value().display_path),
                                        records[index].size,
                                        stored_size,
                                        records[index].offset,
                                        records[index].name_hash,
                                        profile.stored_compression(records[index].packed_size),
                                        records[index].unknown,
                                        false,
                                        0U};
            // Assigned rather than appended positionally: the aggregate already
            // carries ten fields, and a future field inserted mid-struct would
            // silently shift a trailing positional initializer onto it.
            entry.record_identity_mismatch = named.value().record_identity_mismatch;
            entries.push_back(std::move(entry));
        }

        std::sort(entries.begin(), entries.end(),
                  [](const entry_metadata& lhs, const entry_metadata& rhs) {
                      return lhs.path < rhs.path;
                  });
        return entries;
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("BA2 GNRL entry metadata");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("BA2 GNRL entry metadata");
    }
}

}  // namespace

result<opened_ba2_archive> materialize_ba2_gnrl_archive(const ba2_archive_source& source,
                                                        const ba2_archive_header& header) {
    if (!header.profile().is_gnrl()) {
        return error{error_code::unsupported, "BA2 Archive Header is not GNRL"};
    }

    std::size_t records_size = 0U;
    if (!multiply_fits(header.file_count(), ba2_gnrl_record_size, records_size)) {
        return error{error_code::format_error, "BA2 GNRL record table is too large"};
    }
    std::uint64_t records_end = 0U;
    if (!add_fits_u64(static_cast<std::uint64_t>(header.profile().header_size()),
                      static_cast<std::uint64_t>(records_size), records_end) ||
        header.filename_table_offset() < records_end) {
        return error{error_code::format_error,
                     "BA2 GNRL FileTableOffset is outside the metadata span"};
    }

    auto record_bytes =
        source.read_exact(header.profile().header_size(), records_size, "BA2 GNRL record table");
    if (!record_bytes) {
        return record_bytes.error();
    }
    detail::binary_reader record_reader{record_bytes.value()};
    auto records = read_records(record_reader, header.file_count());
    if (!records) {
        return records.error();
    }

    // The writer-required BA2 layout can place payload bytes before the final
    // filename table, so the stable source reads exactly file_count
    // length-prefixed names from FileTableOffset instead of deriving a table
    // size from the first payload offset. This preserves bounded open behavior
    // for both sparse payloads and end tables.
    auto materialization =
        ba2_metadata_materialization::create(source, header, records_end, gnrl_metadata_wording);
    if (!materialization) {
        return materialization.error();
    }

    auto entries = materialize_entries(materialization.value(), records.value(), header.profile());
    if (!entries) {
        return entries.error();
    }

    return opened_ba2_archive{header.materialize_metadata(), std::move(entries).value(),
                              ba2_subtype::gnrl};
}

}  // namespace libbsa::formats::ba2
