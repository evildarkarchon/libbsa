#include "formats/ba2/ba2_dx10_parser.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_names.hpp"
#include "formats/ba2/ba2_dx10_records.hpp"
#include "formats/ba2/ba2_record_identity.hpp"
#include "texture/dds_layout.hpp"

#include <detail/binary_io.hpp>
#include <detail/byte_vector.hpp>
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

constexpr std::uint64_t reconstructed_dds_header_size = 148U;

struct stored_chunk_span {
    std::uint64_t offset;
    std::uint64_t size;
};

using detail::add_fits;
using detail::add_fits_u64;
using detail::read_file_bytes_at;
using detail::span_fits_u64;
using detail::spans_overlap_u64;

entry_compression compression_for(const ba2_dx10_chunk_record& chunk,
                                  detected_ba2_format detected) noexcept {
    if (chunk.packed_size == 0U) {
        return entry_compression::none;
    }
    // Starfield v3 CompressionMethod 3 is normalized by the detector into raw-LZ4
    // block metadata here; DX10 chunks still decide raw-vs-compressed from
    // PackedSize, never filename or extension spelling.
    return detected.profile.default_compression();
}

result<std::uint64_t> first_payload_offset_for(std::span<const ba2_dx10_record> records,
                                               std::uint64_t archive_size) {
    std::uint64_t first_payload_offset = archive_size;
    std::size_t expected_chunk_count = 0;
    for (const auto& record : records) {
        std::size_t next_chunk_count = 0;
        if (!add_fits(expected_chunk_count, record.chunks.size(), next_chunk_count)) {
            return error{error_code::format_error,
                         "BA2 DX10 aggregate texture chunk count exceeds platform limits"};
        }
        expected_chunk_count = next_chunk_count;
    }

    std::vector<stored_chunk_span> accepted_payload_spans;
    auto reserved_payload_spans = detail::reserve_metadata_vector(
        accepted_payload_spans, expected_chunk_count, "BA2 DX10 stored chunk spans");
    if (!reserved_payload_spans) {
        return reserved_payload_spans.error();
    }

    for (const auto& record : records) {
        for (const auto& chunk : record.chunks) {
            const auto stored_size = static_cast<std::uint64_t>(
                chunk.packed_size != 0U ? chunk.packed_size : chunk.raw_size);
            if (stored_size == 0U || chunk.raw_size == 0U) {
                return error{error_code::format_error, "BA2 DX10 chunk sizes are inconsistent"};
            }
            if (!span_fits_u64(chunk.offset, stored_size, archive_size)) {
                return error{error_code::format_error,
                             "BA2 DX10 chunk payload span is outside the archive"};
            }
            for (const auto& prior : accepted_payload_spans) {
                const auto exact_duplicate =
                    prior.offset == chunk.offset && prior.size == stored_size;
                if (!exact_duplicate &&
                    spans_overlap_u64(prior.offset, prior.size, chunk.offset, stored_size)) {
                    return error{error_code::format_error,
                                 "BA2 DX10 chunk payload spans partially overlap"};
                }
            }
            // Writer dedupe can intentionally publish exact duplicate chunks; partial
            // sharing would make texture chunk extraction ambiguous because logical
            // DDS segments would read bytes from each other's ranges.
            accepted_payload_spans.push_back(stored_chunk_span{chunk.offset, stored_size});
            first_payload_offset = std::min(first_payload_offset, chunk.offset);
        }
    }
    return first_payload_offset;
}

std::uint32_t inferred_array_size(const ba2_dx10_record& record) noexcept {
    // TES5Edit treats CubeMaps == 2049 as the cubemap signal. Preserve
    // cube_maps_raw separately so future compatibility work can revisit broader
    // Bethesda-specific flag meanings without data loss.
    std::uint32_t slices = 0;
    for (const auto& chunk : record.chunks) {
        if (chunk.start_mip == 0U) {
            ++slices;
        }
    }
    if (record.cube_maps_raw == ba2_dx10_cubemap_raw) {
        // BA2 stores one start-mip sequence per cubemap face, so logical cube-array
        // count is face groups divided by the six DDS cubemap faces rather than a
        // value encoded in CubeMaps.
        return std::max(1U, slices / 6U);
    }
    return std::max(1U, slices);
}

result<std::vector<texture_chunk_metadata>> public_chunks_for(const ba2_dx10_record& record,
                                                              detected_ba2_format detected) {
    try {
        std::vector<texture_chunk_metadata> archive_chunks;
        auto reserved_archive_chunks = detail::reserve_metadata_vector(
            archive_chunks, record.chunks.size(), "BA2 DX10 public texture chunks");
        if (!reserved_archive_chunks) {
            return reserved_archive_chunks.error();
        }
        for (const auto& chunk : record.chunks) {
            const auto compression = compression_for(chunk, detected);
            archive_chunks.push_back(texture_chunk_metadata{
                chunk.offset, chunk.packed_size == 0U ? chunk.raw_size : chunk.packed_size,
                chunk.raw_size, chunk.start_mip, chunk.end_mip, compression});
        }

        texture::dds_texture_layout layout{record.width,
                                           record.height,
                                           record.num_mips,
                                           record.dxgi_format,
                                           inferred_array_size(record),
                                           record.cube_maps_raw == ba2_dx10_cubemap_raw};
        auto ordered_segments = texture::validate_and_order_chunks(layout, archive_chunks);
        if (!ordered_segments) {
            return ordered_segments.error();
        }

        std::vector<texture_chunk_metadata> ordered_chunks;
        auto reserved_ordered_chunks = detail::reserve_metadata_vector(
            ordered_chunks, ordered_segments.value().size(), "BA2 DX10 ordered texture chunks");
        if (!reserved_ordered_chunks) {
            return reserved_ordered_chunks.error();
        }
        for (const auto& logical_texture_segment : ordered_segments.value()) {
            ordered_chunks.push_back(archive_chunks.at(logical_texture_segment.source_chunk_index));
        }
        return ordered_chunks;
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("BA2 DX10 public texture chunks");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("BA2 DX10 public texture chunks");
    }
}

result<std::vector<entry_metadata>> materialize_entries(std::size_t archive_size,
                                                        std::size_t name_table_end,
                                                        std::span<const ba2_dx10_record> records,
                                                        std::span<const std::string> names,
                                                        detected_ba2_format detected) {
    try {
        std::vector<entry_metadata> entries;
        auto reserved_entries =
            detail::reserve_metadata_vector(entries, records.size(), "BA2 DX10 entry metadata");
        if (!reserved_entries) {
            return reserved_entries.error();
        }
        std::unordered_set<std::string> canonical_paths;
        auto reserved_paths = detail::reserve_metadata_set(canonical_paths, records.size(),
                                                           "BA2 DX10 canonical path set");
        if (!reserved_paths) {
            return reserved_paths.error();
        }

        auto first_payload_offset = first_payload_offset_for(records, archive_size);
        if (!first_payload_offset) {
            return first_payload_offset.error();
        }
        if (name_table_end > first_payload_offset.value()) {
            return error{error_code::format_error, "BA2 DX10 filename table overlaps payload data"};
        }

        for (std::size_t index = 0; index < records.size(); ++index) {
            auto identity = make_ba2_record_identity(ba2_subtype::dx10, names[index],
                                                     ba2_record_identity_source::filename_table);
            if (!identity) {
                return identity.error();
            }
            if (!canonical_paths.insert(identity.value().canonical_path).second) {
                return error{error_code::format_error,
                             "BA2 DX10 contains duplicate canonical archive paths"};
            }

            const ba2_stored_record_identity stored_identity{
                records[index].name_hash, records[index].directory_hash, records[index].extension};
            auto validated_identity =
                validate_ba2_record_identity(ba2_subtype::dx10, stored_identity, identity.value());
            if (!validated_identity) {
                return validated_identity.error();
            }

            auto chunks = public_chunks_for(records[index], detected);
            if (!chunks) {
                return chunks.error();
            }

            std::uint64_t raw_payload_size = 0;
            std::uint64_t stored_payload_size = 0;
            bool has_compressed_chunk = false;
            std::uint64_t payload_offset = std::numeric_limits<std::uint64_t>::max();
            for (const auto& chunk : chunks.value()) {
                // Current BA2 DX10 record fields bound these totals below UInt64 max,
                // but keep the public metadata boundary checked so future chunk-size
                // widening cannot expose wrapped sizes.
                if (!add_fits_u64(raw_payload_size, chunk.raw_size, raw_payload_size)) {
                    return error{error_code::format_error,
                                 "BA2 DX10 raw payload aggregate size overflows"};
                }
                if (!add_fits_u64(stored_payload_size, chunk.stored_size, stored_payload_size)) {
                    return error{error_code::format_error,
                                 "BA2 DX10 stored payload aggregate size overflows"};
                }
                payload_offset = std::min(payload_offset, chunk.payload_offset);
                has_compressed_chunk =
                    has_compressed_chunk || chunk.compression != entry_compression::none;
            }

            std::uint64_t entry_raw_size = 0;
            if (!add_fits_u64(reconstructed_dds_header_size, raw_payload_size, entry_raw_size)) {
                return error{error_code::format_error, "BA2 DX10 reconstructed DDS size overflows"};
            }

            const auto array_size = inferred_array_size(records[index]);
            const auto is_cubemap = records[index].cube_maps_raw == ba2_dx10_cubemap_raw;
            texture_metadata texture{records[index].width,
                                     records[index].height,
                                     records[index].num_mips,
                                     records[index].dxgi_format,
                                     array_size,
                                     is_cubemap,
                                     records[index].unknown_tex,
                                     records[index].cube_maps_raw,
                                     std::move(chunks.value())};

            entries.push_back(entry_metadata{
                std::move(identity.value().canonical_path), std::move(identity.value().display_path),
                entry_raw_size, stored_payload_size, payload_offset, records[index].name_hash,
                has_compressed_chunk ? detected.profile.default_compression()
                                     : entry_compression::none,
                records[index].unknown_tex, false, 0U, std::move(texture)});
        }

        std::sort(entries.begin(), entries.end(),
                  [](const entry_metadata& lhs, const entry_metadata& rhs) {
                      return lhs.path < rhs.path;
                  });
        return entries;
    } catch (const std::bad_alloc&) {
        return detail::metadata_allocation_error("BA2 DX10 entry metadata");
    } catch (const std::length_error&) {
        return detail::metadata_allocation_error("BA2 DX10 entry metadata");
    }
}

result<ba2_dx10_archive> parse_ba2_dx10_archive_impl(std::span<const std::byte> metadata_bytes,
                                                     std::size_t archive_size,
                                                     detected_ba2_format detected) {
    if (!detected.profile.is_dx10()) {
        return error{error_code::unsupported, "detected BA2 format is not DX10"};
    }

    detail::binary_reader reader{metadata_bytes};
    auto header = read_ba2_dx10_header(reader);
    if (!header) {
        return header.error();
    }
    if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_dx10_magic) {
        return error{error_code::format_error, "BA2 DX10 header magic or subtype is invalid"};
    }
    if (header.value().version != detected.profile.version() ||
        header.value().file_count != detected.file_count) {
        return error{error_code::format_error,
                     "BA2 DX10 detected header does not match parsed header"};
    }
    auto count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "BA2 DX10 file count");
    if (!count_limit) {
        return count_limit.error();
    }
    if (header.value().file_table_offset > archive_size) {
        return error{error_code::format_error,
                     "BA2 DX10 FileTableOffset is outside the metadata span"};
    }

    auto records =
        read_ba2_dx10_records(reader, header.value().file_count, header.value().file_table_offset);
    if (!records) {
        return records.error();
    }

    const auto file_table_offset = static_cast<std::size_t>(header.value().file_table_offset);
    std::size_t name_table_consumed = 0;
    auto names = read_ba2_dx10_names(metadata_bytes.subspan(file_table_offset),
                                     header.value().file_count, name_table_consumed);
    if (!names) {
        return names.error();
    }
    std::size_t name_table_end = 0;
    if (!add_fits(file_table_offset, name_table_consumed, name_table_end)) {
        return error{error_code::format_error, "BA2 DX10 filename table is too large"};
    }

    auto entries =
        materialize_entries(archive_size, name_table_end, records.value(), names.value(), detected);
    if (!entries) {
        return entries.error();
    }

    return ba2_dx10_archive{archive_metadata{archive_type::ba2, detected.profile.variant(),
                                             header.value().version, 0U, header.value().file_count,
                                             detected.profile.default_compression(),
                                             detected.profile.ba2_metadata()},
                            std::move(entries.value())};
}

}  // namespace

result<ba2_dx10_archive> parse_ba2_dx10_archive(std::span<const std::byte> bytes,
                                                detected_ba2_format detected) {
    return parse_ba2_dx10_archive_impl(bytes, bytes.size(), detected);
}

result<ba2_dx10_archive> parse_ba2_dx10_archive_file(const detail::host_file_path& host_path,
                                                     std::uint64_t archive_size,
                                                     detected_ba2_format detected) {
    if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return error{error_code::format_error, "BA2 DX10 archive exceeds platform limits"};
    }

    const detail::host_file_context host_context{
        "failed to open archive host path", "failed to determine archive host path size",
        "failed while reading archive host path", "archive host path changed while reading",
        "BA2 DX10 metadata table"};
    auto input = detail::open_host_file(host_path, host_context);
    if (!input) {
        return input.error();
    }
    auto fixed_header = read_file_bytes_at(input.value(), 0U, detected.profile.header_size(),
                                           "BA2 DX10 fixed header");
    if (!fixed_header) {
        return fixed_header.error();
    }
    detail::binary_reader header_reader{fixed_header.value()};
    auto header = read_ba2_dx10_header(header_reader);
    if (!header) {
        return header.error();
    }
    if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_dx10_magic) {
        return error{error_code::format_error, "BA2 DX10 header magic or subtype is invalid"};
    }
    if (header.value().version != detected.profile.version() ||
        header.value().file_count != detected.file_count) {
        return error{error_code::format_error,
                     "BA2 DX10 detected header does not match parsed header"};
    }
    auto count_limit = detail::validate_metadata_count(
        header.value().file_count, detail::metadata_entry_count_limit, "BA2 DX10 file count");
    if (!count_limit) {
        return count_limit.error();
    }
    if (header.value().file_table_offset > archive_size ||
        header.value().file_table_offset < detected.profile.header_size()) {
        return error{error_code::format_error,
                     "BA2 DX10 FileTableOffset is outside the metadata span"};
    }

    auto metadata_bytes = read_file_bytes_at(
        input.value(), 0U, static_cast<std::size_t>(header.value().file_table_offset),
        "BA2 DX10 header and texture records");
    if (!metadata_bytes) {
        return metadata_bytes.error();
    }
    detail::binary_reader metadata_reader{metadata_bytes.value()};
    auto parsed_header = read_ba2_dx10_header(metadata_reader);
    if (!parsed_header) {
        return parsed_header.error();
    }
    auto records = read_ba2_dx10_records(metadata_reader, header.value().file_count,
                                         header.value().file_table_offset);
    if (!records) {
        return records.error();
    }

    auto first_payload_offset = first_payload_offset_for(records.value(), archive_size);
    if (!first_payload_offset) {
        return first_payload_offset.error();
    }
    if (header.value().file_table_offset > first_payload_offset.value()) {
        return error{error_code::format_error, "BA2 DX10 filename table overlaps payload data"};
    }
    // BA2 DX10 filename tables are count-delimited, so sparse padding between the
    // last encoded name and first payload must not be allocated during open/list
    // metadata parsing.
    auto name_table_bytes =
        read_ba2_dx10_names_from_file(input.value(), header.value().file_table_offset,
                                      first_payload_offset.value(), header.value().file_count);
    if (!name_table_bytes) {
        return name_table_bytes.error();
    }
    auto appended_names = detail::append_byte_vector(
        metadata_bytes.value(), name_table_bytes.value(), "BA2 DX10 metadata filename table");
    if (!appended_names) {
        return appended_names.error();
    }
    return parse_ba2_dx10_archive_impl(metadata_bytes.value(),
                                       static_cast<std::size_t>(archive_size), detected);
}

}  // namespace libbsa::formats::ba2
