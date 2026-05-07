#pragma once

#include <libbsa/ba2.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace libbsa::detail {

/// Owns one target-rule BA2 DDS chunk derived from source DDS image data.
struct analyzed_dds_chunk {
    std::uint16_t start_mip{};
    std::uint16_t end_mip{};
    std::vector<std::byte> payload;
};

/// Owns libbsa texture metadata and chunk payloads derived from a DDS input.
struct analyzed_dds_texture {
    dxgi_format format{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t mip_count{};
    std::uint32_t array_size{};
    bool is_cubemap{};
    std::vector<analyzed_dds_chunk> chunks;
};

/// Analyzes a DDS file image and returns BA2-ready metadata plus headerless mip payload chunks.
[[nodiscard]] result<analyzed_dds_texture> analyze_dds(std::span<const std::byte> dds_bytes);

} // namespace libbsa::detail
