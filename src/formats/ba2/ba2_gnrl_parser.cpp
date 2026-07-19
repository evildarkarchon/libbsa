#include "formats/ba2/ba2_gnrl_parser.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/binary_io.hpp>
#include <detail/parser_primitives.hpp>

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

struct stored_payload_span {
    std::uint64_t offset;
    std::uint64_t size;
};

using detail::add_fits_u64;
using detail::archive_string_from_bytes;
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

/// Reads exactly `file_count` length-prefixed names through the stable source.
result<std::vector<std::string>> read_names(const ba2_archive_source& source,
                                            std::uint64_t file_table_offset,
                                            std::uint32_t file_count, std::uint64_t& table_end) {
    if (file_table_offset > source.size()) {
        return error{error_code::format_error, "BA2 GNRL FileTableOffset is outside archive bytes"};
    }

    std::vector<std::string> names;
    auto reserved =
        detail::reserve_metadata_vector(names, file_count, "BA2 GNRL filename table entries");
    if (!reserved) {
        return reserved.error();
    }
    std::uint64_t cursor = file_table_offset;
    for (std::uint32_t index = 0; index < file_count; ++index) {
        if (!span_fits_u64(cursor, 2U, source.size())) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table is truncated before UInt16 length"};
        }
        auto length_bytes = source.read_exact(cursor, 2U, "BA2 GNRL filename length");
        if (!length_bytes) {
            return length_bytes.error();
        }
        detail::binary_reader length_reader{length_bytes.value()};
        const auto length = length_reader.read_u16_le();
        if (!length) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table is truncated before UInt16 length"};
        }
        cursor += 2U;
        if (!span_fits_u64(cursor, length.value(), source.size())) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table is truncated before name bytes"};
        }
        auto name_bytes = source.read_exact(cursor, length.value(), "BA2 GNRL filename bytes");
        if (!name_bytes) {
            return name_bytes.error();
        }
        if (name_bytes.value().empty()) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table contains an empty name"};
        }
        auto name = archive_string_from_bytes(name_bytes.value(), "BA2 GNRL filename bytes");
        if (!name) {
            return name.error();
        }
        names.push_back(std::move(name.value()));
        cursor += length.value();
    }
    table_end = cursor;
    return names;
}

/// Interprets a stored GNRL record through the authoritative BA2 Profile.
entry_compression compression_for(const gnrl_record& record, const ba2_profile& profile) noexcept {
    if (record.packed_size == 0U) {
        return entry_compression::none;
    }
    return profile.default_compression();
}

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
        std::vector<stored_payload_span> accepted_payload_spans;
        auto reserved_payload_spans = detail::reserve_metadata_vector(
            accepted_payload_spans, records.size(), "BA2 GNRL stored payload spans");
        if (!reserved_payload_spans) {
            return reserved_payload_spans.error();
        }

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

            const ba2_stored_record_identity stored_identity{
                records[index].name_hash, records[index].directory_hash, records[index].extension};
            auto validated_identity =
                validate_ba2_record_identity(ba2_subtype::gnrl, stored_identity, identity.value());
            if (!validated_identity) {
                return validated_identity.error();
            }

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
                for (const auto& prior : accepted_payload_spans) {
                    const auto exact_duplicate =
                        prior.offset == records[index].offset && prior.size == stored_size;
                    if (!exact_duplicate && spans_overlap_u64(prior.offset, prior.size,
                                                              records[index].offset, stored_size)) {
                        return error{error_code::format_error,
                                     "BA2 GNRL entry payload spans partially overlap"};
                    }
                }
                // Writer dedupe can intentionally publish exact duplicate stored spans;
                // partial sharing would make two entries read ambiguous bytes from each
                // other's payload ranges.
                accepted_payload_spans.push_back(
                    stored_payload_span{records[index].offset, stored_size});
            }

            entries.push_back(entry_metadata{
                std::move(identity.value().canonical_path),
                std::move(identity.value().display_path), records[index].size, stored_size,
                records[index].offset, records[index].name_hash,
                compression_for(records[index], profile), records[index].unknown, false, 0U});
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
        header.filename_table_offset() < records_end ||
        header.filename_table_offset() > source.size()) {
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

    std::uint64_t name_table_end = 0U;
    // The writer-required BA2 layout can place payload bytes before the final
    // filename table, so the stable source reads exactly file_count
    // length-prefixed names from FileTableOffset instead of deriving a table
    // size from the first payload offset. This preserves bounded open behavior
    // for both sparse payloads and end tables.
    auto names =
        read_names(source, header.filename_table_offset(), header.file_count(), name_table_end);
    if (!names) {
        return names.error();
    }

    auto entries =
        materialize_entries(source.size(), records_end, header.filename_table_offset(),
                            name_table_end, records.value(), names.value(), header.profile());
    if (!entries) {
        return entries.error();
    }

    return opened_ba2_archive{header.materialize_metadata(), std::move(entries).value(),
                              ba2_subtype::gnrl};
}

}  // namespace libbsa::formats::ba2
