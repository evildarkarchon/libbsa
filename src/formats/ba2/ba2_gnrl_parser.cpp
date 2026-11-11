#include "formats/ba2/ba2_gnrl_parser.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_metadata_materialization.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/binary_io.hpp>
#include <detail/parser_primitives.hpp>
#include <detail/payload_span_exclusivity.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <unordered_set>
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
using detail::span_fits_u64;
using detail::spans_overlap_u64;

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
};

/// Validates record/name relationships and builds deterministic public entries.
result<std::vector<entry_metadata>> materialize_entries(
    std::uint64_t archive_size, std::uint64_t records_end, std::uint64_t name_table_offset,
    std::uint64_t name_table_end, std::span<const gnrl_record> records,
    std::span<const std::string> names, const ba2_profile& profile) {
    try {
        std::vector<entry_metadata> entries;
        auto reserved_entries =
            detail::reserve_metadata_vector(entries, records.size(), "BA2 GNRL entry metadata");
        if (!reserved_entries) {
            return reserved_entries.error();
        }
        std::unordered_set<std::string> canonical_paths;
        auto reserved_paths = detail::reserve_metadata_set(canonical_paths, records.size(),
                                                           "BA2 GNRL canonical path set");
        if (!reserved_paths) {
            return reserved_paths.error();
        }
        detail::payload_span_exclusivity accepted_payload_spans;

        for (std::size_t index = 0; index < records.size(); ++index) {
            auto identity = make_ba2_record_identity(ba2_subtype::gnrl, names[index],
                                                     ba2_record_identity_source::filename_table);
            if (!identity) {
                return identity.error();
            }
            if (!canonical_paths.insert(identity.value().canonical_path).second) {
                return error{error_code::format_error,
                             "BA2 GNRL contains duplicate canonical archive paths"};
            }

            // Stored lookup fields are recorded, not enforced: BSArchPro never
            // compares them against the filename table, and retail archives ship
            // records whose stored NameHash no reconstruction reproduces.
            // Fallout4 - Voices.ba2 has three in 116,101, and failing the archive
            // over them rejected every good entry with them (issue #43). All
            // three carry non-ASCII names, which hash_fo4 skips byte-for-byte as
            // the reference does, so the cause is an unresolved encoding
            // question in Bethesda's packer rather than proven corruption.
            const ba2_stored_record_identity stored_identity{
                records[index].name_hash, records[index].directory_hash, records[index].extension};
            const auto identity_mismatch =
                compare_ba2_record_identity(stored_identity, identity.value());

            const auto stored_size =
                records[index].packed_size != 0U ? records[index].packed_size : records[index].size;
            if (!span_fits_u64(records[index].offset, stored_size, archive_size)) {
                return error{error_code::format_error,
                             "BA2 GNRL entry payload span is outside the archive"};
            }
            // BSArchPro writes GNRL payloads after the fixed header and records;
            // spans into this prefix would later extract archive metadata bytes as if
            // they were file payload.
            if (spans_overlap_u64(records[index].offset, stored_size, 0U, records_end)) {
                return error{error_code::format_error,
                             "BA2 GNRL entry payload span intersects header or record table"};
            }
            if (spans_overlap_u64(records[index].offset, stored_size, name_table_offset,
                                  name_table_end - name_table_offset)) {
                return error{error_code::format_error,
                             "BA2 GNRL filename table intersects payload data"};
            }
            if (stored_size != 0U) {
                // Writer dedupe can intentionally publish exact duplicate stored spans;
                // partial sharing would make two entries read ambiguous bytes from each
                // other's payload ranges. The call stays here, interleaved with the
                // checks above, because which diagnostic an archive with several defects
                // reports is observable behavior.
                auto exclusive =
                    accepted_payload_spans.insert(records[index].offset, stored_size,
                                                  "BA2 GNRL entry payload spans partially overlap");
                if (!exclusive) {
                    return exclusive.error();
                }
            }

            // BA2 stores `/` in its filename table; the display spelling libbsa
            // reports is `\` on every format. Convert here rather than in
            // `resolve_ba2_record_path`, which the BA2 writers share for the
            // bytes they serialize.
            auto display_path = std::move(identity.value().stored_path);
            detail::normalize_display_separators(display_path);

            auto entry = entry_metadata{std::move(identity.value().canonical_path),
                                        std::move(display_path),
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
            entry.record_identity_mismatch = identity_mismatch.any();
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

    const auto name_table = materialization.value().filename_table_extent();
    auto entries =
        materialize_entries(source.size(), records_end, name_table.offset, name_table.end,
                            records.value(), materialization.value().names(), header.profile());
    if (!entries) {
        return entries.error();
    }

    return opened_ba2_archive{header.materialize_metadata(), std::move(entries).value(),
                              ba2_subtype::gnrl};
}

}  // namespace libbsa::formats::ba2
