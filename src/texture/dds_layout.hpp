#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace libbsa::texture {

/// Texture dimensions and DDS DXT10 metadata needed to reconstruct a header.
///
/// These values are libbsa-owned so DDS extraction can build deterministic bytes without exposing
/// DirectXTex or platform graphics types across the internal texture boundary.
struct dds_texture_layout {
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t mip_count;
  std::uint32_t dxgi_format;
  std::uint32_t array_size;
  bool is_cubemap;
};

/// Identity of a validated texture payload segment in computed DDS output order.
///
/// `source_chunk_index` maps the logical array/face/mip segment back to the parsed archive chunk so
/// extraction can write chunks in validated DDS order instead of blindly streaming archive order.
struct logical_texture_segment {
  std::uint32_t array_index;
  std::uint32_t face_index;
  std::uint32_t start_mip;
  std::uint32_t end_mip;
  std::size_t source_chunk_index;
};

/// Builds `DDS ` + `DDS_HEADER` + `DDS_HEADER_DXT10` bytes for a BA2 texture layout.
///
/// Returns `format_error` for impossible dimensions or unsupported fixture-backed DXGI formats.
[[nodiscard]] result<std::vector<std::byte>> build_dds_dxt10_header(const dds_texture_layout& layout);

/// Validates BA2 texture chunks and returns their logical DDS segment identities.
///
/// The accepted order is array slice ascending, cubemap face order `+X, -X, +Y, -Y, +Z, -Z`, then
/// ascending mip ranges within each face/slice. Gaps, overlaps, duplicates, unsupported formats,
/// and impossible raw byte totals return `format_error` before extraction reads payload bytes.
[[nodiscard]] result<std::vector<logical_texture_segment>> validate_and_order_chunks(
    const dds_texture_layout& layout, std::span<const texture_chunk_metadata> chunks);

} // namespace libbsa::texture
