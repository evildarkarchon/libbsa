#include "texture/directxtex_analyzer.hpp"

// The analyzer includes Windows-backed texture headers privately; NOMINMAX keeps those headers
// from rewriting standard-library min/max calls in this translation unit.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <DirectXTex.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace libbsa::texture
{
  namespace
  {

    result<std::uint32_t> checked_u32(std::size_t value, const char *field_name)
    {
      if (value > (std::numeric_limits<std::uint32_t>::max)())
      {
        return error{error_code::format_error, std::string{field_name} + " exceeds public metadata limits"};
      }
      return static_cast<std::uint32_t>(value);
    }

    bool is_supported_writer_source_format(DXGI_FORMAT format) noexcept
    {
      switch (static_cast<std::uint32_t>(format))
      {
      case 28U: // R8G8B8A8_UNORM
      case 71U: // BC1_UNORM
      case 72U: // BC1_UNORM_SRGB
      case 77U: // BC3_UNORM
      case 80U: // BC4_UNORM
      case 83U: // BC5_UNORM
      case 84U: // BC5_SNORM
      case 95U: // BC6H_UF16
      case 96U: // BC6H_SF16
      case 98U: // BC7_UNORM
      case 99U: // BC7_UNORM_SRGB
      case 29U: // R8G8B8A8_UNORM_SRGB
      case 87U: // B8G8R8A8_UNORM
      case 61U: // R8_UNORM
      case 31U: // R8G8B8A8_SNORM
        return true;
      default:
        return false;
      }
    }

    /// Rejects DDS shapes the BA2 DX10 writer cannot round-trip without changing resource dimension.
    result<void> validate_writer_source_shape(const DirectX::TexMetadata &metadata)
    {
      if (metadata.dimension != DirectX::TEX_DIMENSION_TEXTURE2D)
      {
        return error{error_code::format_error, "DDS source shape is unsupported by the BA2 DX10 writer"};
      }
      if (metadata.depth > 1U)
      {
        return error{error_code::format_error, "DDS source depth is unsupported by the BA2 DX10 writer"};
      }
      if (metadata.IsCubemap() && (metadata.arraySize % 6U) != 0U)
      {
        return error{error_code::format_error, "DDS cubemap source does not contain complete face groups"};
      }
      return {};
    }

    result<texture_metadata> translate_source_metadata(const DirectX::TexMetadata &metadata)
    {
      auto width = checked_u32(metadata.width, "DDS width");
      if (!width)
      {
        return width.error();
      }
      auto height = checked_u32(metadata.height, "DDS height");
      if (!height)
      {
        return height.error();
      }
      auto mip_count = checked_u32(metadata.mipLevels, "DDS mip count");
      if (!mip_count)
      {
        return mip_count.error();
      }
      const auto logical_array_size = metadata.IsCubemap() ? metadata.arraySize / 6U : metadata.arraySize;
      auto array_size = checked_u32(logical_array_size, "DDS array size");
      if (!array_size)
      {
        return array_size.error();
      }

      texture_metadata translated{};
      translated.width = width.value();
      translated.height = height.value();
      translated.mip_count = mip_count.value();
      translated.dxgi_format = static_cast<std::uint32_t>(metadata.format);
      translated.array_size = array_size.value();
      translated.is_cubemap = metadata.IsCubemap();
      translated.unknown_tex = 0U;
      translated.cube_maps_raw = 0U;
      return translated;
    }

  } // namespace

  result<texture_metadata> analyze_dds_metadata(std::span<const std::byte> dds_bytes)
  {
    DirectX::TexMetadata metadata{};
    const HRESULT hr = DirectX::GetMetadataFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, metadata);
    if (hr < 0)
    {
      return error{error_code::format_error, "DDS metadata could not be loaded by the texture analyzer"};
    }

    auto width = checked_u32(metadata.width, "DDS width");
    if (!width)
    {
      return width.error();
    }
    auto height = checked_u32(metadata.height, "DDS height");
    if (!height)
    {
      return height.error();
    }
    auto mip_count = checked_u32(metadata.mipLevels, "DDS mip count");
    if (!mip_count)
    {
      return mip_count.error();
    }
    auto array_size = checked_u32(metadata.arraySize, "DDS array size");
    if (!array_size)
    {
      return array_size.error();
    }

    texture_metadata translated{};
    translated.width = width.value();
    translated.height = height.value();
    translated.mip_count = mip_count.value();
    translated.dxgi_format = static_cast<std::uint32_t>(metadata.format);
    translated.array_size = array_size.value();
    translated.is_cubemap = metadata.IsCubemap();
    translated.unknown_tex = 0U;
    translated.cube_maps_raw = 0U;
    return translated;
  }

  result<dds_source_analysis> analyze_dds_source(std::span<const std::byte> dds_bytes)
  {
    DirectX::TexMetadata metadata{};
    DirectX::ScratchImage image{};
    const HRESULT hr = DirectX::LoadFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
    if (hr < 0)
    {
      return error{error_code::format_error, "DDS source could not be loaded by the texture analyzer"};
    }
    auto shape = validate_writer_source_shape(metadata);
    if (!shape)
    {
      return shape.error();
    }
    if (!is_supported_writer_source_format(metadata.format))
    {
      return error{error_code::format_error, "DDS source format is unsupported by the BA2 DX10 writer"};
    }

    auto translated = translate_source_metadata(metadata);
    if (!translated)
    {
      return translated.error();
    }

    dds_source_analysis analysis{};
    analysis.metadata = translated.value();
    analysis.dds_bytes.assign(dds_bytes.begin(), dds_bytes.end());

    const DirectX::Image *images = image.GetImages();
    const auto image_count = image.GetImageCount();
    if (images == nullptr || image_count == 0U)
    {
      return error{error_code::format_error, "DDS source has no image payloads"};
    }

    const std::uint32_t faces_per_array = metadata.IsCubemap() ? 6U : 1U;
    for (std::size_t index = 0; index < image_count; ++index)
    {
      const auto &source = images[index];
      if (source.pixels == nullptr || source.slicePitch == 0U)
      {
        return error{error_code::format_error, "DDS source image payload is empty"};
      }
      const auto array_face = static_cast<std::uint32_t>(index / metadata.mipLevels);
      const auto mip = static_cast<std::uint32_t>(index % metadata.mipLevels);
      const auto array_index = metadata.IsCubemap() ? array_face / faces_per_array : array_face;
      const auto face_index = metadata.IsCubemap() ? array_face % faces_per_array : 0U;
      std::vector<std::byte> copied;
      copied.reserve(source.slicePitch);
      const auto *begin = reinterpret_cast<const std::byte *>(source.pixels);
      copied.insert(copied.end(), begin, begin + source.slicePitch);
      // D-03 requires writer-owned snapshots: callers may delete or mutate the source DDS after add.
      // Copying here avoids storing DirectX-owned pointers or borrowing caller file bytes across phases.
      analysis.image_payload_bytes.insert(analysis.image_payload_bytes.end(), copied.begin(), copied.end());
      analysis.subresources.push_back(dds_source_subresource{array_index, face_index, mip, std::move(copied)});
    }

    return analysis;
  }

} // namespace libbsa::texture
