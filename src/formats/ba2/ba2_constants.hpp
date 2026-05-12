#pragma once

#include <cstddef>
#include <cstdint>

namespace libbsa::formats::ba2 {

inline constexpr std::uint32_t ba2_btdx_magic = 0x5844'5442U;
inline constexpr std::uint32_t ba2_gnrl_magic = 0x4C52'4E47U;
inline constexpr std::uint32_t ba2_dx10_magic = 0x3031'5844U;

inline constexpr std::uint32_t ba2_fallout4_version = 1U;
inline constexpr std::uint32_t ba2_starfield_v2_version = 2U;
inline constexpr std::uint32_t ba2_starfield_v3_version = 3U;

inline constexpr std::size_t ba2_common_header_size = 24U;
inline constexpr std::size_t ba2_starfield_v2_header_size = 32U;
inline constexpr std::size_t ba2_starfield_v3_header_size = 36U;
inline constexpr std::size_t ba2_gnrl_record_size = 36U;
inline constexpr std::size_t ba2_dx10_record_size = 24U;

// TES5Edit's BA2 readers and writers require the BAADF00D sentinel after each GNRL record and DX10 chunk.
inline constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;

// BA2 GNRL and DX10 records use PackedSize == 0 to mean the payload bytes are stored raw.
inline constexpr std::uint32_t ba2_packed_size_raw = 0U;

// Starfield BA2 v3 routes CompressionMethod 0 through deflate and method 3 through raw LZ4 blocks.
inline constexpr std::uint32_t ba2_starfield_compression_deflate = 0U;
inline constexpr std::uint32_t ba2_starfield_compression_lz4_block = 3U;

// TES5Edit writes and reads a fixed 24-byte DX10 chunk header; other widths make record parsing drift.
inline constexpr std::uint16_t ba2_dx10_chunk_header_size = 24U;

// BA2 DX10 stores cubemap state as raw marker values rather than a boolean field.
inline constexpr std::uint16_t ba2_dx10_non_cubemap_raw = 2048U;
inline constexpr std::uint16_t ba2_dx10_cubemap_raw = 2049U;

// Reference-derived writer-owned texture byte; not public until stronger compatibility evidence requires it.
inline constexpr std::uint8_t ba2_dx10_unknown_tex_default = 0U;

} // namespace libbsa::formats::ba2
