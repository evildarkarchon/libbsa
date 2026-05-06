#include "dds_validation.hpp"

#include <DirectXTex.h>

#include <limits>

namespace libbsa::detail {
namespace {

std::uint32_t checked_metadata_size(std::size_t value) noexcept
{
    if (value > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())) {
        return (std::numeric_limits<std::uint32_t>::max)();
    }
    return static_cast<std::uint32_t>(value);
}

} // namespace

result<dds_validation_metadata> validate_dds(std::span<const std::byte> dds_bytes)
{
    DirectX::TexMetadata metadata{};
    DirectX::ScratchImage image{};
    // DirectXTex validation stays in this private translation unit so public
    // libbsa headers never inherit Windows, DXGI, or texture-library types.
    const auto hr = DirectX::LoadFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
    if (hr < 0) {
        return failure<dds_validation_metadata>({error_code::malformed_archive, "DDS validation failed"});
    }

    dds_validation_metadata result{};
    result.width = checked_metadata_size(metadata.width);
    result.height = checked_metadata_size(metadata.height);
    result.mip_count = checked_metadata_size(metadata.mipLevels);
    result.array_size = checked_metadata_size(metadata.arraySize);
    result.format = dxgi_format{static_cast<std::uint32_t>(metadata.format)};
    result.is_cubemap = metadata.IsCubemap();
    return success(result);
}

} // namespace libbsa::detail
