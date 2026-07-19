#pragma once

#include <libbsa/result.hpp>

#include <detail/binary_io.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace libbsa::formats::ba2 {

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

/// Reads the fixed-width BA2 DX10 texture record and chunk tables without
/// materializing public entries.
///
/// `reader` must contain exactly the header-delimited record-table bytes; any
/// trailing padding is rejected so FileTableOffset remains authoritative.
[[nodiscard]] result<std::vector<ba2_dx10_record>> read_ba2_dx10_records(
    detail::binary_reader& reader, std::uint32_t file_count);

}  // namespace libbsa::formats::ba2
