#pragma once

#include <libbsa/ba2.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::detail {

/// Publicly independent metadata reported after private DDS validation.
struct dds_validation_metadata {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t mip_count{};
    std::uint32_t array_size{};
    dxgi_format format{};
    bool is_cubemap{};
};

/// Validates DDS bytes through the private texture backend and returns libbsa-owned metadata.
///
/// This boundary intentionally keeps texture-library types out of public headers and callers.
[[nodiscard]] result<dds_validation_metadata> validate_dds(std::span<const std::byte> dds_bytes);

} // namespace libbsa::detail
