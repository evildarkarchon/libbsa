#pragma once

#include <libbsa/archive.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace libbsa::test {

inline constexpr std::uint32_t VERSION_FO4_DX10_V1 = 0x01;
inline constexpr std::uint32_t VERSION_FO4_DX10_V7 = 0x07;
inline constexpr std::uint32_t VERSION_FO4_DX10_V8 = 0x08;
inline constexpr std::uint32_t VERSION_STARFIELD_DX10_V3 = 0x03;

/// Describes one generated BA2 DX10 chunk before archive bytes are materialized.
struct ba2_dds_chunk_descriptor {
    std::uint16_t start_mip{};
    std::uint16_t end_mip{};
    std::vector<std::byte> payload;
    compression_state compression{compression_state::raw};
};

/// Describes one generated BA2 DX10 texture entry.
struct ba2_dds_texture_descriptor {
    std::string path{"textures/generated/one_mip.dds"};
    std::uint32_t name_hash{0x11223344};
    std::uint32_t directory_hash{0xaabbccdd};
    std::uint16_t width{4};
    std::uint16_t height{4};
    std::uint8_t mip_count{1};
    std::uint8_t dxgi_format{71};
    std::uint16_t array_size{1};
    bool cubemap{};
    std::vector<ba2_dds_chunk_descriptor> chunks;
};

/// Owns generated BA2 DX10 archive bytes and the semantic texture descriptors used to build them.
struct ba2_dds_fixture {
    std::uint32_t version{};
    std::uint32_t compression_method{};
    std::vector<ba2_dds_texture_descriptor> textures;
    std::vector<std::byte> bytes;
};

/// Builds BA2 DX10 bytes for caller-supplied texture descriptors.
[[nodiscard]] ba2_dds_fixture make_ba2_dds_fixture(std::uint32_t version,
                                                   std::vector<ba2_dds_texture_descriptor> textures,
                                                   std::uint32_t compression_method = 0);

/// Returns a Fallout 4 DX10 v1 one-mip, single raw chunk fixture.
[[nodiscard]] ba2_dds_fixture fo4_dx10_v1_one_mip_fixture();

/// Returns a Fallout 4 DX10 v7 one-mip, single raw chunk fixture.
[[nodiscard]] ba2_dds_fixture fo4_dx10_v7_one_mip_fixture();

/// Returns a Fallout 4 DX10 v8 one-mip, single raw chunk fixture.
[[nodiscard]] ba2_dds_fixture fo4_dx10_v8_one_mip_fixture();

/// Returns a Starfield DX10 v3 fixture with method 3 raw block compression metadata.
[[nodiscard]] ba2_dds_fixture starfield_dx10_v3_lz4_block_fixture();

/// Returns a representative one-mip 2D fixture.
[[nodiscard]] ba2_dds_fixture one_mip_fixture(std::uint32_t version);

/// Returns a representative multi-mip fixture.
[[nodiscard]] ba2_dds_fixture multi_mip_fixture(std::uint32_t version);

/// Returns a representative cubemap fixture.
[[nodiscard]] ba2_dds_fixture cubemap_fixture(std::uint32_t version);

/// Returns a representative array texture fixture.
[[nodiscard]] ba2_dds_fixture array_fixture(std::uint32_t version);

/// Returns a fixture whose texture chunk is stored raw.
[[nodiscard]] ba2_dds_fixture raw_chunk_fixture(std::uint32_t version);

/// Returns a fixture whose texture chunk is deflate-compressed.
[[nodiscard]] ba2_dds_fixture deflate_chunk_fixture(std::uint32_t version);

/// Returns a Starfield fixture whose texture chunk is compressed as an LZ4 raw block.
[[nodiscard]] ba2_dds_fixture lz4_block_chunk_fixture();

/// Returns a fixture truncated inside the DX10 record table.
[[nodiscard]] ba2_dds_fixture malformed_truncated_record_fixture();

/// Returns a fixture with a chunk range outside the generated source bytes.
[[nodiscard]] ba2_dds_fixture malformed_invalid_chunk_range_fixture();

/// Returns a fixture with duplicate normalized names.
[[nodiscard]] ba2_dds_fixture malformed_duplicate_normalized_names_fixture();

/// Returns a fixture whose metadata requires an unsupported codec route.
[[nodiscard]] ba2_dds_fixture malformed_unsupported_codec_route_fixture();

/// Returns a fixture whose chunk-to-mip mapping is inconsistent.
[[nodiscard]] ba2_dds_fixture malformed_inconsistent_mip_chunk_mapping_fixture();

/// Returns a fixture whose metadata should fail DDS reconstruction.
[[nodiscard]] ba2_dds_fixture malformed_reconstruction_failure_fixture();

/// Checks only semantic archive identity bytes that are stable across all generated DX10 fixtures.
void require_ba2_dds_identity(const ba2_dds_fixture& fixture);

} // namespace libbsa::test
