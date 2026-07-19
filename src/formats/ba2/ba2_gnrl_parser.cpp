#include "formats/ba2/ba2_gnrl_parser.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/binary_io.hpp>
#include <detail/host_file.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

struct header_fields {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t subtype;
    std::uint32_t file_count;
    std::uint64_t file_table_offset;
    ba2_archive_metadata ba2;
};

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

using detail::add_fits;
using detail::add_fits_u64;
using detail::archive_string_from_bytes;
using detail::multiply_fits;
using detail::read_file_bytes_at;
using detail::span_fits;
using detail::span_fits_u64;
using detail::spans_overlap_u64;

result<header_fields> read_header(detail::binary_reader& reader) {
    const auto magic = reader.read_u32_le();
    const auto version = reader.read_u32_le();
    const auto subtype = reader.read_u32_le();
    const auto file_count = reader.read_u32_le();
    const auto file_table_offset = reader.read_u64_le();
    if (!magic || !version || !subtype || !file_count || !file_table_offset) {
        return error{error_code::format_error,
                     "BA2 GNRL fixed header is truncated before FileTableOffset"};
    }

    ba2_archive_metadata ba2{};
    if (version.value() >= ba2_starfield_v2_version) {
        const auto unknown1 = reader.read_u32_le();
        const auto unknown2 = reader.read_u32_le();
        if (!unknown1 || !unknown2) {
            return error{error_code::format_error,
                         "BA2 GNRL Starfield v2 header fields are truncated"};
        }
        ba2.starfield_unknown1 = unknown1.value();
        ba2.starfield_unknown2 = unknown2.value();
    }
    if (version.value() >= ba2_starfield_v3_version) {
        const auto compression_method = reader.read_u32_le();
        if (!compression_method) {
            return error{error_code::format_error,
                         "BA2 GNRL Starfield v3 CompressionMethod is truncated"};
        }
        ba2.compression_method = compression_method.value();
    }

    return header_fields{magic.value(),      version.value(),           subtype.value(),
                         file_count.value(), file_table_offset.value(), ba2};
}

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

result<std::vector<std::string>> read_names(std::span<const std::byte> name_table,
                                            std::uint32_t file_count, std::size_t& consumed) {
    detail::binary_reader reader{name_table};
    std::vector<std::string> names;
    auto reserved =
        detail::reserve_metadata_vector(names, file_count, "BA2 GNRL filename table entries");
    if (!reserved) {
        return reserved.error();
    }
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto length = reader.read_u16_le();
        if (!length) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table is truncated before UInt16 length"};
        }
        const auto bytes = reader.read_bytes(length.value());
        if (!bytes) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table is truncated before name bytes"};
        }
        if (bytes.value().empty()) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table contains an empty name"};
        }
        auto name = archive_string_from_bytes(bytes.value(), "BA2 GNRL filename bytes");
        if (!name) {
            return name.error();
        }
        names.push_back(std::move(name.value()));
    }
    consumed = reader.position();
    return names;
}

result<std::vector<std::string>> read_names_from_file(std::ifstream& input,
                                                      std::uint64_t file_table_offset,
                                                      std::uint32_t file_count,
                                                      std::uint64_t archive_size,
                                                      std::size_t& consumed) {
    if (file_table_offset > archive_size) {
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
        if (!span_fits_u64(cursor, 2U, archive_size)) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table is truncated before UInt16 length"};
        }
        auto length_bytes = read_file_bytes_at(input, cursor, 2U, "BA2 GNRL filename length");
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
        if (!span_fits_u64(cursor, length.value(), archive_size)) {
            return error{error_code::format_error,
                         "BA2 GNRL filename table is truncated before name bytes"};
        }
        auto name_bytes =
            read_file_bytes_at(input, cursor, length.value(), "BA2 GNRL filename bytes");
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

    const auto consumed_u64 = cursor - file_table_offset;
    if (consumed_u64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return error{error_code::format_error,
                     "BA2 GNRL filename table size exceeds platform limits"};
    }
    consumed = static_cast<std::size_t>(consumed_u64);
    return names;
}

entry_compression compression_for(const gnrl_record& record,
                                  detected_ba2_format detected) noexcept {
    if (record.packed_size == 0U) {
        return entry_compression::none;
    }
    return detected.profile.default_compression();
}

result<std::vector<entry_metadata>> materialize_entries(
    std::uint64_t archive_size, std::uint64_t records_end, std::uint64_t name_table_offset,
    std::uint64_t name_table_end, std::span<const gnrl_record> records,
    std::span<const std::string> names, detected_ba2_format detected) {
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
                compression_for(records[index], detected), records[index].unknown, false, 0U});
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

result<ba2_gnrl_archive> parse_ba2_gnrl_archive_impl(std::span<const std::byte> metadata_bytes,
                                                     std::size_t archive_size,
                                                     detected_ba2_format detected) {
    if (!detected.profile.is_gnrl()) {
        return error{error_code::unsupported, "detected BA2 format is not GNRL"};
    }

    detail::binary_reader reader{metadata_bytes};
    auto header = read_header(reader);
    if (!header) {
        return header.error();
    }
    if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_gnrl_magic) {
        return error{error_code::format_error, "BA2 GNRL header magic or subtype is invalid"};
    }
    if (header.value().version != detected.profile.version() ||
        header.value().file_count != detected.file_count) {
        return error{error_code::format_error,
                     "BA2 GNRL detected header does not match parsed header"};
    }
    auto count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "BA2 GNRL file count");
    if (!count_limit) {
        return count_limit.error();
    }

    std::size_t records_size = 0;
    if (!multiply_fits(header.value().file_count, ba2_gnrl_record_size, records_size)) {
        return error{error_code::format_error, "BA2 GNRL record table is too large"};
    }
    std::size_t records_end = 0;
    if (!add_fits(detected.profile.header_size(), records_size, records_end) ||
        header.value().file_table_offset < records_end ||
        header.value().file_table_offset > archive_size) {
        return error{error_code::format_error,
                     "BA2 GNRL FileTableOffset is outside the metadata span"};
    }

    auto records = read_records(reader, header.value().file_count);
    if (!records) {
        return records.error();
    }

    const auto file_table_offset = static_cast<std::size_t>(header.value().file_table_offset);
    if (!span_fits(file_table_offset, metadata_bytes.size() - file_table_offset,
                   metadata_bytes.size())) {
        return error{error_code::format_error, "BA2 GNRL FileTableOffset is outside archive bytes"};
    }
    std::size_t name_table_consumed = 0;
    auto names = read_names(metadata_bytes.subspan(file_table_offset), header.value().file_count,
                            name_table_consumed);
    if (!names) {
        return names.error();
    }
    std::size_t name_table_end = 0;
    if (!add_fits(file_table_offset, name_table_consumed, name_table_end)) {
        return error{error_code::format_error, "BA2 GNRL filename table is too large"};
    }

    auto entries = materialize_entries(static_cast<std::uint64_t>(archive_size),
                                       static_cast<std::uint64_t>(records_end), file_table_offset,
                                       name_table_end, records.value(), names.value(), detected);
    if (!entries) {
        return entries.error();
    }

    return ba2_gnrl_archive{
        archive_metadata{archive_type::ba2, detected.profile.variant(), header.value().version, 0U,
                         header.value().file_count, detected.profile.default_compression(),
                         detected.stored_metadata},
        std::move(entries.value())};
}

}  // namespace

result<ba2_gnrl_archive> parse_ba2_gnrl_archive(std::span<const std::byte> bytes,
                                                detected_ba2_format detected) {
    return parse_ba2_gnrl_archive_impl(bytes, bytes.size(), detected);
}

result<ba2_gnrl_archive> parse_ba2_gnrl_archive_file(const detail::host_file_path& host_path,
                                                     std::uint64_t archive_size,
                                                     detected_ba2_format detected) {
    if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return error{error_code::format_error, "BA2 GNRL archive exceeds platform limits"};
    }

    const detail::host_file_context host_context{
        "failed to open archive host path", "failed to determine archive host path size",
        "failed while reading archive host path", "archive host path changed while reading",
        "BA2 GNRL metadata table"};
    auto input = detail::open_host_file(host_path, host_context);
    if (!input) {
        return input.error();
    }
    auto fixed_header = read_file_bytes_at(input.value(), 0U, detected.profile.header_size(),
                                           "BA2 GNRL fixed header");
    if (!fixed_header) {
        return fixed_header.error();
    }
    detail::binary_reader header_reader{fixed_header.value()};
    auto header = read_header(header_reader);
    if (!header) {
        return header.error();
    }
    if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_gnrl_magic) {
        return error{error_code::format_error, "BA2 GNRL header magic or subtype is invalid"};
    }
    if (header.value().version != detected.profile.version() ||
        header.value().file_count != detected.file_count) {
        return error{error_code::format_error,
                     "BA2 GNRL detected header does not match parsed header"};
    }
    auto count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "BA2 GNRL file count");
    if (!count_limit) {
        return count_limit.error();
    }

    std::size_t records_size = 0;
    if (!multiply_fits(header.value().file_count, ba2_gnrl_record_size, records_size)) {
        return error{error_code::format_error, "BA2 GNRL record table is too large"};
    }
    std::size_t records_end = 0;
    if (!add_fits(detected.profile.header_size(), records_size, records_end) ||
        header.value().file_table_offset < records_end ||
        header.value().file_table_offset > archive_size) {
        return error{error_code::format_error,
                     "BA2 GNRL FileTableOffset is outside the metadata span"};
    }

    const auto metadata_size = records_end;
    auto metadata_bytes =
        read_file_bytes_at(input.value(), 0U, metadata_size, "BA2 GNRL header and record table");
    if (!metadata_bytes) {
        return metadata_bytes.error();
    }

    detail::binary_reader metadata_reader{metadata_bytes.value()};
    auto parsed_header = read_header(metadata_reader);
    if (!parsed_header) {
        return parsed_header.error();
    }
    auto records = read_records(metadata_reader, header.value().file_count);
    if (!records) {
        return records.error();
    }

    std::size_t name_table_consumed = 0;
    // The writer-required BA2 layout can place payload bytes before the final
    // filename table, so host-file open parses exactly file_count length-prefixed
    // names from FileTableOffset instead of deriving a table size from the first
    // payload offset. This preserves bounded open behavior for both sparse
    // payloads and end tables.
    auto names = read_names_from_file(input.value(), header.value().file_table_offset,
                                      header.value().file_count, archive_size, name_table_consumed);
    if (!names) {
        return names.error();
    }
    std::uint64_t name_table_end = 0;
    if (!add_fits_u64(header.value().file_table_offset,
                      static_cast<std::uint64_t>(name_table_consumed), name_table_end)) {
        return error{error_code::format_error, "BA2 GNRL filename table is too large"};
    }
    auto entries = materialize_entries(archive_size, static_cast<std::uint64_t>(records_end),
                                       header.value().file_table_offset, name_table_end,
                                       records.value(), names.value(), detected);
    if (!entries) {
        return entries.error();
    }

    return ba2_gnrl_archive{
        archive_metadata{archive_type::ba2, detected.profile.variant(), header.value().version, 0U,
                         header.value().file_count, detected.profile.default_compression(),
                         detected.stored_metadata},
        std::move(entries.value())};
}

}  // namespace libbsa::formats::ba2
