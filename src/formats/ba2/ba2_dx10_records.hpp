#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/binary_io.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace libbsa::formats::ba2 {

/// Fixed BA2 DX10 header fields read before texture records are materialized.
struct ba2_dx10_header_fields {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t subtype;
    std::uint32_t file_count;
    std::uint64_t file_table_offset;
    ba2_archive_metadata ba2;
};

/// Raw BA2 DX10 texture chunk record exactly as stored in the texture record
/// table.
struct ba2_dx10_chunk_record {
    std::uint64_t offset;
    std::uint32_t packed_size;
    std::uint32_t raw_size;
    std::uint16_t start_mip;
    std::uint16_t end_mip;
};

/// Raw BA2 DX10 texture record plus its owned chunk records.
struct ba2_dx10_record {
    std::uint32_t name_hash;
    std::array<std::byte, 4> extension;
    std::uint32_t directory_hash;
    std::uint8_t unknown_tex;
    std::uint8_t chunk_count;
    std::uint16_t chunk_header_size;
    std::uint16_t height;
    std::uint16_t width;
    std::uint8_t num_mips;
    std::uint8_t dxgi_format;
    std::uint16_t cube_maps_raw;
    std::vector<ba2_dx10_chunk_record> chunks;
};

/// Reads the fixed BA2 DX10 header without consuming texture records or
/// filename-table bytes.
[[nodiscard]] result<ba2_dx10_header_fields> read_ba2_dx10_header(detail::binary_reader& reader);

/// Reads the fixed-width BA2 DX10 texture record and chunk tables without
/// materializing public entries.
[[nodiscard]] result<std::vector<ba2_dx10_record>> read_ba2_dx10_records(
    detail::binary_reader& reader, std::uint32_t file_count, std::uint64_t file_table_offset);

}  // namespace libbsa::formats::ba2
