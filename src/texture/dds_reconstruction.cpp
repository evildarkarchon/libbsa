#include "dds_reconstruction.hpp"

#include <algorithm>
#include <cstdint>

namespace libbsa::detail {
namespace {

constexpr std::uint32_t dds_magic = 0x20534444U;
constexpr std::uint32_t dds_header_size = 124U;
constexpr std::uint32_t dds_pixel_format_size = 32U;
constexpr std::uint32_t ddsd_caps = 0x00000001U;
constexpr std::uint32_t ddsd_height = 0x00000002U;
constexpr std::uint32_t ddsd_width = 0x00000004U;
constexpr std::uint32_t ddsd_pixel_format = 0x00001000U;
constexpr std::uint32_t ddsd_mipmap_count = 0x00020000U;
constexpr std::uint32_t ddsd_linear_size = 0x00080000U;
constexpr std::uint32_t ddpf_fourcc = 0x00000004U;
constexpr std::uint32_t fourcc_dx10 = 0x30315844U;
constexpr std::uint32_t ddscaps_complex = 0x00000008U;
constexpr std::uint32_t ddscaps_texture = 0x00001000U;
constexpr std::uint32_t ddscaps_mipmap = 0x00400000U;
constexpr std::uint32_t ddscaps2_cubemap_all_faces = 0x0000fe00U;
constexpr std::uint32_t dds_dimension_texture2d = 3U;
constexpr std::uint32_t dds_resource_misc_texturecube = 0x00000004U;
constexpr std::uint32_t dxgi_format_r8g8b8a8_unorm = 28U;
constexpr std::uint32_t dxgi_format_bc1_unorm = 71U;
constexpr std::uint32_t dxgi_format_bc3_unorm = 77U;
constexpr std::uint32_t dxgi_format_bc5_unorm = 83U;
constexpr std::uint32_t dxgi_format_bc7_unorm = 98U;

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

std::uint32_t top_level_linear_size(dxgi_format format, std::uint32_t width, std::uint32_t height) noexcept
{
    if (format.value == dxgi_format_r8g8b8a8_unorm) {
        return width * 4U;
    }

    const auto blocks_wide = std::max(1U, (width + 3U) / 4U);
    const auto blocks_high = std::max(1U, (height + 3U) / 4U);
    const auto block_bytes = format.value == dxgi_format_bc1_unorm ? 8U : 16U;
    return blocks_wide * blocks_high * block_bytes;
}

bool supported_reconstruction_format(dxgi_format format) noexcept
{
    // Phase 10 supports the DDS formats libbsa can reconstruct and validate without transcoding;
    // broader corpus-driven format expansion belongs to Phase 11 compatibility validation.
    return format.value == dxgi_format_r8g8b8a8_unorm || format.value == dxgi_format_bc1_unorm || format.value == dxgi_format_bc3_unorm ||
           format.value == dxgi_format_bc5_unorm || format.value == dxgi_format_bc7_unorm;
}

} // namespace

result<std::vector<std::byte>> reconstruct_dds(const texture_metadata& metadata, std::span<const std::byte> image_payload)
{
    if (metadata.width == 0 || metadata.height == 0 || metadata.mip_count == 0 || metadata.array_size == 0 || image_payload.empty()) {
        return failure<std::vector<std::byte>>({error_code::malformed_archive, "invalid DDS reconstruction metadata"});
    }
    if (!supported_reconstruction_format(metadata.format)) {
        return failure<std::vector<std::byte>>({error_code::unsupported_format, "unsupported DDS reconstruction format"});
    }

    std::vector<std::byte> bytes;
    bytes.reserve(4U + dds_header_size + 20U + image_payload.size());
    append_u32(bytes, dds_magic);

    append_u32(bytes, dds_header_size);
    append_u32(bytes, ddsd_caps | ddsd_height | ddsd_width | ddsd_pixel_format | ddsd_linear_size |
                          (metadata.mip_count > 1 ? ddsd_mipmap_count : 0U));
    append_u32(bytes, metadata.height);
    append_u32(bytes, metadata.width);
    append_u32(bytes, top_level_linear_size(metadata.format, metadata.width, metadata.height) * metadata.array_size);
    append_u32(bytes, 0);
    append_u32(bytes, metadata.mip_count);
    for (int i = 0; i < 11; ++i) {
        append_u32(bytes, 0);
    }

    append_u32(bytes, dds_pixel_format_size);
    append_u32(bytes, ddpf_fourcc);
    append_u32(bytes, fourcc_dx10);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    append_u32(bytes, 0);

    // Mipmapped, array, and cubemap DDS files need the complex caps bit for broad
    // loader compatibility; the DX10 extension carries the exact array/cubemap shape.
    append_u32(bytes, ddscaps_texture | (metadata.mip_count > 1 ? (ddscaps_complex | ddscaps_mipmap) : 0U) |
                          (metadata.array_size > 1 || metadata.is_cubemap ? ddscaps_complex : 0U));
    append_u32(bytes, metadata.is_cubemap ? ddscaps2_cubemap_all_faces : 0U);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    append_u32(bytes, 0);

    append_u32(bytes, metadata.format.value);
    append_u32(bytes, dds_dimension_texture2d);
    append_u32(bytes, metadata.is_cubemap ? dds_resource_misc_texturecube : 0U);
    // DDS DX10 stores cubemap arrays as cube count; DirectXTex reports them back
    // as face count, so a single cube is written as 1 and validates as 6 faces.
    append_u32(bytes, metadata.is_cubemap ? std::max(1U, metadata.array_size / 6U) : metadata.array_size);
    append_u32(bytes, 0);

    bytes.insert(bytes.end(), image_payload.begin(), image_payload.end());
    return success(std::move(bytes));
}

} // namespace libbsa::detail
