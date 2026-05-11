#include "texture/dds_layout.hpp"

#include <detail/binary_io.hpp>

#include <algorithm>
#include <array>
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

struct dxgi_format_descriptor {
  std::uint32_t format;
  std::uint32_t block_width;
  std::uint32_t block_height;
  std::uint32_t bytes_per_block;
};

constexpr std::array locked_formats{
    dxgi_format_descriptor{28U, 1U, 1U, 4U},  // DXGI_FORMAT_R8G8B8A8_UNORM.
    dxgi_format_descriptor{29U, 1U, 1U, 4U},  // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB.
    dxgi_format_descriptor{31U, 1U, 1U, 4U},  // DXGI_FORMAT_R8G8B8A8_SNORM.
    dxgi_format_descriptor{61U, 1U, 1U, 1U},  // DXGI_FORMAT_R8_UNORM.
    dxgi_format_descriptor{71U, 4U, 4U, 8U},  // DXGI_FORMAT_BC1_UNORM.
    dxgi_format_descriptor{72U, 4U, 4U, 8U},  // DXGI_FORMAT_BC1_UNORM_SRGB.
    dxgi_format_descriptor{77U, 4U, 4U, 16U}, // DXGI_FORMAT_BC3_UNORM.
    dxgi_format_descriptor{80U, 4U, 4U, 8U},  // DXGI_FORMAT_BC4_UNORM.
    dxgi_format_descriptor{83U, 4U, 4U, 16U}, // DXGI_FORMAT_BC5_UNORM.
    dxgi_format_descriptor{84U, 4U, 4U, 16U}, // DXGI_FORMAT_BC5_SNORM.
    dxgi_format_descriptor{95U, 4U, 4U, 16U}, // DXGI_FORMAT_BC6H_UF16.
    dxgi_format_descriptor{98U, 4U, 4U, 16U}, // DXGI_FORMAT_BC7_UNORM.
    dxgi_format_descriptor{99U, 4U, 4U, 16U}, // DXGI_FORMAT_BC7_UNORM_SRGB.
    dxgi_format_descriptor{87U, 1U, 1U, 4U},  // DXGI_FORMAT_B8G8R8A8_UNORM.
};

const dxgi_format_descriptor* find_descriptor(std::uint32_t format) noexcept {
  const auto found = std::find_if(locked_formats.begin(), locked_formats.end(), [format](const auto& descriptor) {
    return descriptor.format == format;
  });
  return found == locked_formats.end() ? nullptr : &*found;
}

std::uint32_t mip_dimension(std::uint32_t dimension, std::uint32_t mip) noexcept {
  const auto shifted = mip >= 31U ? 0U : dimension >> mip;
  return std::max(1U, shifted);
}

std::uint64_t rounded_block_count(std::uint32_t dimension, std::uint32_t block_dimension) noexcept {
  if (block_dimension == 1U) {
    return dimension;
  }

  // Promote before adding the block-rounding bias so hostile uint32 DDS dimensions cannot wrap.
  return std::max<std::uint64_t>(
      1U, (static_cast<std::uint64_t>(dimension) + static_cast<std::uint64_t>(block_dimension) - 1U) /
              static_cast<std::uint64_t>(block_dimension));
}

result<std::uint64_t> described_mip_size(const dxgi_format_descriptor& descriptor, std::uint32_t width,
                                         std::uint32_t height) {
  // DirectX block-compressed DDS formats still allocate at least one 4x4 block for tiny mips.
  const std::uint64_t blocks_wide = rounded_block_count(width, descriptor.block_width);
  const std::uint64_t blocks_high = rounded_block_count(height, descriptor.block_height);
  std::uint64_t blocks = 0;
  if (!checked_mul(blocks_wide, blocks_high, blocks)) {
    return format_error("DDS layout mip block count overflows");
  }
  std::uint64_t bytes = 0;
  if (!checked_mul(blocks, descriptor.bytes_per_block, bytes)) {
    return format_error("DDS layout mip size overflows");
  }
  return bytes;
}

struct planned_mip_range {
  std::uint32_t start_mip;
  std::uint32_t end_mip;
  std::uint64_t raw_size;
};

result<std::vector<planned_mip_range>> default_mip_ranges(const dds_texture_layout& layout) {
  std::vector<planned_mip_range> ranges;
  std::uint32_t mip = 0;

  // The reference-derived BA2 DX10 default emits a few large mips individually, then groups the tail.
  while (mip + 1U < layout.mip_count && ranges.size() < 3U && mip_dimension(layout.width, mip) >= 512U &&
         mip_dimension(layout.height, mip) >= 512U) {
    auto size = mip_range_size(layout, mip, mip);
    if (!size) {
      return size.error();
    }
    ranges.push_back(planned_mip_range{mip, mip, size.value()});
    ++mip;
  }

  auto tail_size = mip_range_size(layout, mip, layout.mip_count - 1U);
  if (!tail_size) {
    return tail_size.error();
  }
  ranges.push_back(planned_mip_range{mip, layout.mip_count - 1U, tail_size.value()});
  return ranges;
}

result<std::vector<planned_mip_range>> capped_mip_ranges(const dds_texture_layout& layout,
                                                         std::uint32_t max_decoded_chunk_bytes) {
  std::vector<planned_mip_range> ranges;
  std::uint32_t start_mip = 0;
  std::uint64_t current_size = 0;

  for (std::uint32_t mip = 0; mip < layout.mip_count; ++mip) {
    auto mip_size = mip_size_for_format(layout, mip);
    if (!mip_size) {
      return mip_size.error();
    }
    if (mip_size.value() > max_decoded_chunk_bytes) {
      return format_error("DDS layout max_decoded_chunk_bytes cannot fit one mip");
    }
    std::uint64_t next_size = 0;
    if (!checked_add(current_size, mip_size.value(), next_size)) {
      return format_error("DDS layout capped chunk size overflows");
    }
    if (current_size != 0U && next_size > max_decoded_chunk_bytes) {
      ranges.push_back(planned_mip_range{start_mip, mip - 1U, current_size});
      start_mip = mip;
      current_size = mip_size.value();
      continue;
    }
    current_size = next_size;
  }

  ranges.push_back(planned_mip_range{start_mip, layout.mip_count - 1U, current_size});
  return ranges;
}

void append_chunks_for_ranges(std::vector<planned_texture_chunk>& chunks,
                              const std::vector<planned_mip_range>& ranges,
                              std::uint32_t array_index,
                              std::uint32_t face_index) {
  for (const auto& range : ranges) {
    chunks.push_back(planned_texture_chunk{array_index, face_index, range.start_mip, range.end_mip, range.raw_size});
  }
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

result<std::uint64_t> mip_size_for_format(const dds_texture_layout& layout, std::uint32_t mip) {
  if (mip >= layout.mip_count) {
    return format_error("DDS layout mip index is out of range");
  }
  const auto* descriptor = find_descriptor(layout.dxgi_format);
  if (descriptor == nullptr) {
    return format_error("DDS layout has unsupported DXGI format");
  }
  return described_mip_size(*descriptor, mip_dimension(layout.width, mip), mip_dimension(layout.height, mip));
}

result<std::uint64_t> mip_range_size(const dds_texture_layout& layout, std::uint32_t start_mip,
                                     std::uint32_t end_mip) {
  if (start_mip > end_mip || end_mip >= layout.mip_count) {
    return format_error("DDS layout mip range is invalid");
  }
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

result<std::vector<planned_texture_chunk>> plan_dx10_chunks(const dds_texture_layout& layout,
                                                            std::uint32_t max_decoded_chunk_bytes) {
  if (!validate_layout_shape(layout)) {
    return format_error("DDS layout has zero dimensions, mip count, or array size");
  }
  auto ranges = max_decoded_chunk_bytes == 0U ? default_mip_ranges(layout)
                                             : capped_mip_ranges(layout, max_decoded_chunk_bytes);
  if (!ranges) {
    return ranges.error();
  }

  const std::uint32_t faces_per_array = layout.is_cubemap ? 6U : 1U;
  std::vector<planned_texture_chunk> chunks;
  chunks.reserve(static_cast<std::size_t>(layout.array_size) * faces_per_array * ranges.value().size());
  for (std::uint32_t array_index = 0; array_index < layout.array_size; ++array_index) {
    for (std::uint32_t face_index = 0; face_index < faces_per_array; ++face_index) {
      append_chunks_for_ranges(chunks, ranges.value(), array_index, face_index);
    }
  }
  return chunks;
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
