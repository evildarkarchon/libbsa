#include "formats/ba2/ba2_dx10_records.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <utility>

namespace libbsa::formats::ba2 {

result<ba2_dx10_header_fields> read_ba2_dx10_header(detail::binary_reader& reader) {
    const auto magic = reader.read_u32_le();
    const auto version = reader.read_u32_le();
    const auto subtype = reader.read_u32_le();
    const auto file_count = reader.read_u32_le();
    const auto file_table_offset = reader.read_u64_le();
    if (!magic || !version || !subtype || !file_count || !file_table_offset) {
        return error{error_code::format_error,
                     "BA2 DX10 fixed header is truncated before FileTableOffset"};
    }

    ba2_archive_metadata ba2{};
    if (version.value() >= ba2_starfield_v2_version) {
        const auto unknown1 = reader.read_u32_le();
        const auto unknown2 = reader.read_u32_le();
        if (!unknown1 || !unknown2) {
            return error{error_code::format_error,
                         "BA2 DX10 Starfield v2 header fields are truncated"};
        }
        ba2.starfield_unknown1 = unknown1.value();
        ba2.starfield_unknown2 = unknown2.value();
    }
    if (version.value() >= ba2_starfield_v3_version) {
        const auto compression_method = reader.read_u32_le();
        if (!compression_method) {
            return error{error_code::format_error,
                         "BA2 DX10 Starfield v3 CompressionMethod is truncated"};
        }
        ba2.compression_method = compression_method.value();
    }

    return ba2_dx10_header_fields{magic.value(),      version.value(),           subtype.value(),
                                  file_count.value(), file_table_offset.value(), ba2};
}

result<std::vector<ba2_dx10_record>> read_ba2_dx10_records(detail::binary_reader& reader,
                                                           std::uint32_t file_count,
                                                           std::uint64_t file_table_offset) {
    std::vector<ba2_dx10_record> records;
    auto reserved = detail::reserve_metadata_vector(records, file_count, "BA2 DX10 records");
    if (!reserved) {
        return reserved.error();
    }
    std::uint64_t aggregate_chunk_count = 0;
    for (std::uint32_t index = 0; index < file_count; ++index) {
        const auto name_hash = reader.read_u32_le();
        const auto extension_bytes = reader.read_bytes(4U);
        const auto directory_hash = reader.read_u32_le();
        const auto unknown_tex = reader.read_u8();
        const auto chunk_count = reader.read_u8();
        const auto chunk_header_size = reader.read_u16_le();
        const auto height = reader.read_u16_le();
        const auto width = reader.read_u16_le();
        const auto num_mips = reader.read_u8();
        const auto dxgi_format = reader.read_u8();
        const auto cube_maps_raw = reader.read_u16_le();
        if (!name_hash || !extension_bytes || !directory_hash || !unknown_tex || !chunk_count ||
            !chunk_header_size || !height || !width || !num_mips || !dxgi_format ||
            !cube_maps_raw) {
            return error{error_code::format_error, "BA2 DX10 record table is truncated"};
        }
        if (chunk_count.value() == 0U) {
            return error{error_code::format_error, "BA2 DX10 record has no texture chunks"};
        }
        // TES5Edit writes and reads a fixed 24-byte DX10 chunk header. Phase 6
        // rejects other widths so parser position cannot drift into payload or
        // filename bytes.
        if (chunk_header_size.value() != ba2_dx10_chunk_header_size) {
            return error{error_code::format_error, "BA2 DX10 chunk_header_size is unsupported"};
        }
        std::uint64_t next_aggregate_chunk_count = 0;
        if (!detail::add_fits_u64(aggregate_chunk_count, chunk_count.value(),
                                  next_aggregate_chunk_count) ||
            next_aggregate_chunk_count > detail::metadata_dx10_chunk_count_limit) {
            return error{error_code::format_error,
                         "BA2 DX10 aggregate texture chunk count exceeds libbsa "
                         "metadata limit"};
        }
        aggregate_chunk_count = next_aggregate_chunk_count;

        std::array<std::byte, 4> extension{};
        std::copy(extension_bytes.value().begin(), extension_bytes.value().end(),
                  extension.begin());
        ba2_dx10_record record{name_hash.value(),      extension,
                               directory_hash.value(), unknown_tex.value(),
                               chunk_count.value(),    chunk_header_size.value(),
                               height.value(),         width.value(),
                               num_mips.value(),       dxgi_format.value(),
                               cube_maps_raw.value(),  {}};
        auto reserved_chunks = detail::reserve_metadata_vector(record.chunks, chunk_count.value(),
                                                               "BA2 DX10 chunk records");
        if (!reserved_chunks) {
            return reserved_chunks.error();
        }
        for (std::uint8_t chunk_index = 0; chunk_index < chunk_count.value(); ++chunk_index) {
            const auto offset = reader.read_u64_le();
            const auto packed_size = reader.read_u32_le();
            const auto raw_size = reader.read_u32_le();
            const auto start_mip = reader.read_u16_le();
            const auto end_mip = reader.read_u16_le();
            const auto sentinel = reader.read_u32_le();
            if (!offset || !packed_size || !raw_size || !start_mip || !end_mip || !sentinel) {
                return error{error_code::format_error, "BA2 DX10 chunk table is truncated"};
            }
            if (sentinel.value() != ba2_record_sentinel) {
                return error{error_code::format_error,
                             "BA2 DX10 chunk BAADF00D sentinel is invalid"};
            }
            record.chunks.push_back(ba2_dx10_chunk_record{offset.value(), packed_size.value(),
                                                          raw_size.value(), start_mip.value(),
                                                          end_mip.value()});
        }
        records.push_back(std::move(record));
    }
    if (reader.position() != file_table_offset) {
        return error{error_code::format_error,
                     "BA2 DX10 FileTableOffset does not match texture record table size"};
    }
    return records;
}

}  // namespace libbsa::formats::ba2
