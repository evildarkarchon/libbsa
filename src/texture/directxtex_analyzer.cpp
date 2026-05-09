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

namespace libbsa::texture {
namespace {

result<std::uint32_t> checked_u32(std::size_t value, const char* field_name) {
  if (value > (std::numeric_limits<std::uint32_t>::max)()) {
    return error{error_code::format_error, std::string{field_name} + " exceeds public metadata limits"};
  }
  return static_cast<std::uint32_t>(value);
}

} // namespace

result<texture_metadata> analyze_dds_metadata(std::span<const std::byte> dds_bytes) {
  DirectX::TexMetadata metadata{};
  const HRESULT hr = DirectX::GetMetadataFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, metadata);
  if (hr < 0) {
    return error{error_code::format_error, "DDS metadata could not be loaded by the texture analyzer"};
  }

  auto width = checked_u32(metadata.width, "DDS width");
  if (!width) {
    return width.error();
  }
  auto height = checked_u32(metadata.height, "DDS height");
  if (!height) {
    return height.error();
  }
  auto mip_count = checked_u32(metadata.mipLevels, "DDS mip count");
  if (!mip_count) {
    return mip_count.error();
  }
  auto array_size = checked_u32(metadata.arraySize, "DDS array size");
  if (!array_size) {
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

} // namespace libbsa::texture
