#include "texture/dds_layout.hpp"

#include <detail/binary_io.hpp>

#include <algorithm>
#include <limits>
#include <string>

namespace libbsa::texture {
namespace {

constexpr std::uint32_t dds_magic = 0x20534444U;
constexpr std::uint32_t dds_header_size = 124U;
constexpr std::uint32_t dds_flags = 0x0002100FU;
constexpr std::uint32_t dds_pitch_or_linear_size = 0U;
constexpr std::uint32_t dds_depth = 0U;
constexpr std::uint32_t dds_pixel_format_size = 32U;
constexpr std::uint32_t dds_pixel_format_fourcc_flag = 0x00000004U;
constexpr std::uint32_t dds_fourcc_dx10 = 0x30315844U;
constexpr std::uint32_t dds_caps_texture = 0x00001000U;
constexpr std::uint32_t dds_caps_complex = 0x00000008U;
constexpr std::uint32_t dds_caps_mipmap = 0x00400000U;
constexpr std::uint32_t dds_caps2_cubemap = 0x00000200U;
constexpr std::uint32_t dds_caps2_cubemap_all_faces = 0x0000FC00U;
constexpr std::uint32_t dds_dxt10_resourceDimension_texture2d = 3U;
constexpr std::uint32_t dds_dxt10_misc_texturecube = 0x4U;
constexpr std::uint32_t dds_dxt10_misc_flags2_default = 0U;
constexpr std::size_t dds_reserved1_count = 11U;

libbsa::error format_error(std::string message) {
  return {libbsa::error_code::format_error, std::move(message)};
}

bool validate_layout_shape(const dds_texture_layout& layout) noexcept {
  return layout.width != 0U && layout.height != 0U && layout.mip_count != 0U && layout.array_size != 0U;
}

bool checked_add(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
  if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
    return false;
  }
  total = lhs + rhs;
  return true;
}

bool checked_mul(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
  if (rhs != 0U && lhs > std::numeric_limits<std::uint64_t>::max() / rhs) {
    return false;
  }
  total = lhs * rhs;
  return true;
}

std::uint32_t mip_dimension(std::uint32_t dimension, std::uint32_t mip) noexcept {
  const auto shifted = mip >= 31U ? 0U : dimension >> mip;
  return std::max(1U, shifted);
}

result<std::uint64_t> rgba8_mip_size(std::uint32_t width, std::uint32_t height) {
  std::uint64_t pixels = 0;
  if (!checked_mul(width, height, pixels)) {
    return format_error("DDS layout dimensions overflow pixel count");
  }
  std::uint64_t bytes = 0;
  if (!checked_mul(pixels, 4U, bytes)) {
    return format_error("DDS layout RGBA8 mip size overflows");
  }
  return bytes;
}

result<std::uint64_t> bc1_mip_size(std::uint32_t width, std::uint32_t height) {
  const std::uint64_t blocks_wide = std::max<std::uint32_t>(1U, (width + 3U) / 4U);
  const std::uint64_t blocks_high = std::max<std::uint32_t>(1U, (height + 3U) / 4U);
  std::uint64_t blocks = 0;
  if (!checked_mul(blocks_wide, blocks_high, blocks)) {
    return format_error("DDS layout BC1 block count overflows");
  }
  std::uint64_t bytes = 0;
  if (!checked_mul(blocks, 8U, bytes)) {
    return format_error("DDS layout BC1 mip size overflows");
  }
  return bytes;
}

result<std::uint64_t> mip_size_for_format(const dds_texture_layout& layout, std::uint32_t mip) {
  const auto width = mip_dimension(layout.width, mip);
  const auto height = mip_dimension(layout.height, mip);
  switch (layout.dxgi_format) {
  case 28U: // DXGI_FORMAT_R8G8B8A8_UNORM; emitted by generated Phase 6 fixtures.
    return rgba8_mip_size(width, height);
  case 71U: // DXGI_FORMAT_BC1_UNORM; used by layout tests to lock block-compressed sizing.
  case 72U:
    return bc1_mip_size(width, height);
  default:
    return format_error("DDS layout has unsupported DXGI fixture format");
  }
}

result<std::uint64_t> mip_range_size(const dds_texture_layout& layout, std::uint32_t start_mip,
                                     std::uint32_t end_mip) {
  std::uint64_t total = 0;
  for (std::uint32_t mip = start_mip; mip <= end_mip; ++mip) {
    auto size = mip_size_for_format(layout, mip);
    if (!size) {
      return size.error();
    }
    if (!checked_add(total, size.value(), total)) {
      return format_error("DDS layout mip range size overflows");
    }
  }
  return total;
}

result<void> write_u32(detail::binary_writer& writer, std::uint32_t value) {
  auto written = writer.write_u32_le(value);
  if (!written) {
    return written.error();
  }
  return {};
}

result<void> write_zeroes(detail::binary_writer& writer, std::size_t count) {
  for (std::size_t index = 0; index < count; ++index) {
    auto written = writer.write_u32_le(0U);
    if (!written) {
      return written.error();
    }
  }
  return {};
}

} // namespace

result<std::vector<std::byte>> build_dds_dxt10_header(const dds_texture_layout& layout) {
  if (!validate_layout_shape(layout)) {
    return format_error("DDS layout has zero dimensions, mip count, or array size");
  }
  auto first_mip_size = mip_size_for_format(layout, 0U);
  if (!first_mip_size) {
    return first_mip_size.error();
  }

  detail::binary_writer writer;
  const std::uint32_t caps = dds_caps_texture | (layout.mip_count > 1U ? dds_caps_complex | dds_caps_mipmap : 0U) |
                             (layout.is_cubemap ? dds_caps_complex : 0U);
  const std::uint32_t caps2 = layout.is_cubemap ? (dds_caps2_cubemap | dds_caps2_cubemap_all_faces) : 0U;

  // D-09 always emits the DXT10 extension, even for formats that could be represented by legacy
  // DDS pixel formats. This keeps extraction deterministic and avoids per-format header branches.
  auto ok = write_u32(writer, dds_magic);
  if (!ok) {
    return ok.error();
  }
  for (const auto value : {dds_header_size, dds_flags, layout.height, layout.width, dds_pitch_or_linear_size,
                           dds_depth, layout.mip_count}) {
    ok = write_u32(writer, value);
    if (!ok) {
      return ok.error();
    }
  }
  ok = write_zeroes(writer, dds_reserved1_count);
  if (!ok) {
    return ok.error();
  }
  for (const auto value : {dds_pixel_format_size, dds_pixel_format_fourcc_flag, dds_fourcc_dx10, 0U, 0U, 0U, 0U, 0U,
                           caps, caps2, 0U, 0U, 0U}) {
    ok = write_u32(writer, value);
    if (!ok) {
      return ok.error();
    }
  }

  // D-18 defaults unclear DXT10 fields conservatively: texture2D resource dimension, cube misc flag
  // only for cubemaps, and alpha metadata left at zero.
  const std::uint32_t resourceDimension = dds_dxt10_resourceDimension_texture2d;
  for (const auto value : {layout.dxgi_format, resourceDimension,
                           layout.is_cubemap ? dds_dxt10_misc_texturecube : 0U, layout.array_size,
                           dds_dxt10_misc_flags2_default}) {
    ok = write_u32(writer, value);
    if (!ok) {
      return ok.error();
    }
  }

  const auto bytes = writer.bytes();
  return std::vector<std::byte>{bytes.begin(), bytes.end()};
}

result<std::vector<logical_texture_segment>> validate_and_order_chunks(const dds_texture_layout& layout,
                                                                       std::span<const texture_chunk_metadata> chunks) {
  if (!validate_layout_shape(layout)) {
    return format_error("DDS layout has zero dimensions, mip count, or array size");
  }
  if (chunks.empty()) {
    return format_error("DDS layout has no texture chunks");
  }
  auto format_probe = mip_size_for_format(layout, 0U);
  if (!format_probe) {
    return format_probe.error();
  }

  // D-12: normal extraction must not ask DirectXTex whether these bytes are acceptable; this
  // metadata-only pass fails closed before payload reads and leaves DirectXTex to tests/analyzers.
  // D-15: BA2 texture chunks are accepted only in computed DDS order: array slices ascend first,
  // then reference-backed cubemap face order +X, -X, +Y, -Y, +Z, -Z, then mip ranges ascend.
  const std::uint32_t faces_per_array = layout.is_cubemap ? 6U : 1U;
  std::uint32_t array_index = 0;
  std::uint32_t face_index = 0;
  std::uint32_t expected_start_mip = 0;
  std::vector<logical_texture_segment> segments;
  segments.reserve(chunks.size());

  for (std::size_t source_chunk_index = 0; source_chunk_index < chunks.size(); ++source_chunk_index) {
    if (array_index >= layout.array_size) {
      return format_error("DDS layout has more chunks than expected array/face coverage");
    }

    const auto& chunk = chunks[source_chunk_index];
    if (chunk.start_mip > chunk.end_mip || chunk.end_mip >= layout.mip_count) {
      return format_error("DDS layout chunk has impossible mip range");
    }
    // D-15/DDS-07 permits repeated mip ranges across different array slices or cubemap faces, but
    // within one face/slice a repeated or earlier range is a duplicate/overlap instead of a new face.
    if (chunk.start_mip != expected_start_mip) {
      return format_error("DDS layout chunk sequence has a mip gap, duplicate, or contradicts BA2 order");
    }

    auto expected_size = mip_range_size(layout, chunk.start_mip, chunk.end_mip);
    if (!expected_size) {
      return expected_size.error();
    }
    if (expected_size.value() != chunk.raw_size) {
      return format_error("DDS layout chunk raw byte total does not match mip range");
    }

    segments.push_back(logical_texture_segment{array_index, face_index, chunk.start_mip, chunk.end_mip,
                                               source_chunk_index});
    expected_start_mip = static_cast<std::uint32_t>(chunk.end_mip) + 1U;

    if (expected_start_mip == layout.mip_count) {
      expected_start_mip = 0;
      ++face_index;
      if (face_index == faces_per_array) {
        face_index = 0;
        ++array_index;
      }
    }
  }

  if (array_index != layout.array_size || face_index != 0U || expected_start_mip != 0U) {
    return format_error("DDS layout chunk coverage is incomplete");
  }

  return segments;
}

} // namespace libbsa::texture
