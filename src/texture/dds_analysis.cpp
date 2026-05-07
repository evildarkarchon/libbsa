#include "dds_analysis.hpp"

#include <DirectXTex.h>

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>

namespace libbsa::detail {
namespace {

constexpr std::uint32_t supported_bc1_unorm = 71U;
constexpr std::uint32_t dds_magic = 0x20534444U;
constexpr std::uint32_t dds_header_size = 124U;
constexpr std::uint32_t dds_pixel_format_size = 32U;
constexpr std::uint32_t dds_fourcc_dx10 = 0x30315844U;
constexpr std::size_t dds_header_size_offset = 4U;
constexpr std::size_t dds_pixel_format_size_offset = 76U;
constexpr std::size_t dds_fourcc_offset = 84U;
constexpr std::size_t dds_dx10_format_offset = 128U;

error dds_analysis_error(std::string_view message)
{
    return {error_code::malformed_archive, std::string{message}};
}

result<std::uint32_t> checked_metadata_u32(std::size_t value)
{
    if (value > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())) {
        return failure<std::uint32_t>(dds_analysis_error("DDS analysis failed: metadata value too large"));
    }
    return success(static_cast<std::uint32_t>(value));
}

result<std::uint16_t> checked_metadata_u16(std::size_t value)
{
    if (value > static_cast<std::size_t>((std::numeric_limits<std::uint16_t>::max)())) {
        return failure<std::uint16_t>(dds_analysis_error("DDS analysis failed: metadata value too large"));
    }
    return success(static_cast<std::uint16_t>(value));
}

void append_image_bytes(std::vector<std::byte>& bytes, const DirectX::Image& image)
{
    const auto* begin = reinterpret_cast<const std::byte*>(image.pixels);
    bytes.insert(bytes.end(), begin, begin + image.slicePitch);
}

std::uint32_t read_u32(std::span<const std::byte> bytes, std::size_t offset) noexcept
{
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1U])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2U])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3U])) << 24U);
}

result<void> reject_unsupported_dx10_format(std::span<const std::byte> dds_bytes)
{
    if (dds_bytes.size() < dds_dx10_format_offset + sizeof(std::uint32_t)) {
        return success();
    }
    if (read_u32(dds_bytes, 0U) != dds_magic || read_u32(dds_bytes, dds_header_size_offset) != dds_header_size ||
        read_u32(dds_bytes, dds_pixel_format_size_offset) != dds_pixel_format_size || read_u32(dds_bytes, dds_fourcc_offset) != dds_fourcc_dx10) {
        return success();
    }
    if (read_u32(dds_bytes, dds_dx10_format_offset) != supported_bc1_unorm) {
        return failure<void>({error_code::unsupported_format, "DDS analysis failed: unsupported DDS format"});
    }
    return success();
}

std::uint32_t mip_dimension(std::uint32_t value, std::uint32_t mip) noexcept
{
    return (std::max)(1U, value >> mip);
}

} // namespace

result<analyzed_dds_texture> analyze_dds(std::span<const std::byte> dds_bytes)
{
    const auto supported_format = reject_unsupported_dx10_format(dds_bytes);
    if (!supported_format.has_value()) {
        return failure<analyzed_dds_texture>(supported_format.error());
    }

    DirectX::TexMetadata metadata{};
    DirectX::ScratchImage image{};
    // DirectXTex remains isolated here so public BA2 writer headers expose only libbsa-owned types.
    const auto hr = DirectX::LoadFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
    if (hr < 0) {
        return failure<analyzed_dds_texture>(dds_analysis_error("DDS analysis failed"));
    }
    if (metadata.dimension != DirectX::TEX_DIMENSION_TEXTURE2D || metadata.IsVolumemap()) {
        return failure<analyzed_dds_texture>({error_code::unsupported_format, "DDS analysis failed: unsupported texture layout"});
    }

    const auto format = checked_metadata_u32(static_cast<std::size_t>(metadata.format));
    const auto width = checked_metadata_u32(metadata.width);
    const auto height = checked_metadata_u32(metadata.height);
    const auto mip_count = checked_metadata_u32(metadata.mipLevels);
    const auto array_size = checked_metadata_u32(metadata.arraySize);
    if (!format.has_value()) {
        return failure<analyzed_dds_texture>(format.error());
    }
    if (!width.has_value()) {
        return failure<analyzed_dds_texture>(width.error());
    }
    if (!height.has_value()) {
        return failure<analyzed_dds_texture>(height.error());
    }
    if (!mip_count.has_value()) {
        return failure<analyzed_dds_texture>(mip_count.error());
    }
    if (!array_size.has_value()) {
        return failure<analyzed_dds_texture>(array_size.error());
    }
    if (width.value() == 0 || height.value() == 0 || mip_count.value() == 0 || array_size.value() == 0) {
        return failure<analyzed_dds_texture>(dds_analysis_error("DDS analysis failed: empty texture metadata"));
    }
    if (format.value() != supported_bc1_unorm) {
        return failure<analyzed_dds_texture>({error_code::unsupported_format, "DDS analysis failed: unsupported DDS format"});
    }
    if (!checked_metadata_u16(metadata.width).has_value() || !checked_metadata_u16(metadata.height).has_value() ||
        !checked_metadata_u16(metadata.arraySize).has_value() || metadata.mipLevels > 0xffU || metadata.format > 0xffU) {
        return failure<analyzed_dds_texture>(dds_analysis_error("DDS analysis failed: BA2 DX10 metadata narrowing failed"));
    }

    analyzed_dds_texture analyzed{};
    analyzed.format = dxgi_format{format.value()};
    analyzed.width = width.value();
    analyzed.height = height.value();
    analyzed.mip_count = mip_count.value();
    analyzed.array_size = array_size.value();
    analyzed.is_cubemap = metadata.IsCubemap();

    for (std::uint32_t mip = 0; mip < analyzed.mip_count;) {
        const auto mip_width = mip_dimension(analyzed.width, mip);
        const auto mip_height = mip_dimension(analyzed.height, mip);
        const auto end_mip = (std::max)(mip_width, mip_height) <= 256U ? analyzed.mip_count - 1U : mip;

        analyzed_dds_chunk chunk{};
        chunk.start_mip = static_cast<std::uint16_t>(mip);
        chunk.end_mip = static_cast<std::uint16_t>(end_mip);
        for (std::uint32_t chunk_mip = mip; chunk_mip <= end_mip; ++chunk_mip) {
            for (std::uint32_t item = 0; item < analyzed.array_size; ++item) {
                const auto* image_at_mip = image.GetImage(chunk_mip, item, 0);
                if (image_at_mip == nullptr || image_at_mip->pixels == nullptr || image_at_mip->slicePitch == 0) {
                    return failure<analyzed_dds_texture>(dds_analysis_error("DDS analysis failed: mip image missing"));
                }
                append_image_bytes(chunk.payload, *image_at_mip);
            }
        }
        analyzed.chunks.push_back(std::move(chunk));
        mip = end_mip + 1U;
    }

    return success(std::move(analyzed));
}

} // namespace libbsa::detail
