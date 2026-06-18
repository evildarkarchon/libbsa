#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>

#include <cstdint>
#include <optional>
#include <type_traits>

TEST_CASE("ba2_dx10_metadata exposes dependency-light texture metadata fields",
          "[unit][fixture][ba2_dx10_metadata]") {
    libbsa::texture_chunk_metadata chunk{};
    chunk.payload_offset = 128U;
    chunk.stored_size = 64U;
    chunk.raw_size = 256U;
    chunk.start_mip = 0U;
    chunk.end_mip = 3U;
    chunk.compression = libbsa::entry_compression::deflate;

    STATIC_REQUIRE(std::is_same_v<decltype(chunk.payload_offset), std::uint64_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(chunk.stored_size), std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(chunk.raw_size), std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(chunk.start_mip), std::uint16_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(chunk.end_mip), std::uint16_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(chunk.compression), libbsa::entry_compression>);

    libbsa::texture_metadata texture{};
    texture.width = 4U;
    texture.height = 4U;
    texture.mip_count = 3U;
    texture.dxgi_format = 71U;
    texture.array_size = 1U;
    texture.is_cubemap = false;
    texture.unknown_tex = 0U;
    texture.cube_maps_raw = 0U;
    texture.chunks.push_back(chunk);

    STATIC_REQUIRE(std::is_same_v<decltype(texture.width), std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(texture.height), std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(texture.mip_count), std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(texture.dxgi_format), std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(texture.array_size), std::uint32_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(texture.is_cubemap), bool>);
    STATIC_REQUIRE(std::is_same_v<decltype(texture.unknown_tex), std::uint8_t>);
    STATIC_REQUIRE(std::is_same_v<decltype(texture.cube_maps_raw), std::uint16_t>);
    STATIC_REQUIRE(
        std::is_same_v<decltype(texture.chunks), std::vector<libbsa::texture_chunk_metadata>>);

    REQUIRE(texture.chunks.size() == 1U);
    REQUIRE(texture.chunks.front().compression == libbsa::entry_compression::deflate);
}

TEST_CASE("ba2_dx10_metadata keeps non-texture entries opt-in",
          "[unit][fixture][ba2_dx10_metadata]") {
    libbsa::entry_metadata entry{"textures/example.dds",
                                 "Textures/Example.dds",
                                 148U,
                                 64U,
                                 512U,
                                 0x0102030405060708ULL,
                                 libbsa::entry_compression::none,
                                 0U,
                                 false,
                                 0U,
                                 std::nullopt};

    STATIC_REQUIRE(
        std::is_same_v<decltype(entry.texture), std::optional<libbsa::texture_metadata>>);
    REQUIRE_FALSE(entry.texture.has_value());
}
