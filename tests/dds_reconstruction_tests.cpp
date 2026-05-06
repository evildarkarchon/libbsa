#include <catch2/catch_test_macros.hpp>

#include <libbsa/ba2.hpp>

#include "texture/dds_reconstruction.hpp"
#include "texture/dds_validation.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

constexpr std::uint32_t DXGI_FORMAT_BC1_UNORM = 71;

std::vector<std::byte> payload(std::size_t size, std::byte seed)
{
    std::vector<std::byte> bytes(size);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::byte>((std::to_integer<unsigned char>(seed) + i) & 0xffU);
    }
    return bytes;
}

libbsa::texture_metadata texture(std::uint32_t width,
                                 std::uint32_t height,
                                 std::uint32_t mip_count,
                                 std::uint32_t array_size = 1,
                                 bool cubemap = false)
{
    libbsa::texture_metadata metadata{};
    metadata.path = "textures/generated/reconstruct.dds";
    metadata.width = width;
    metadata.height = height;
    metadata.mip_count = mip_count;
    metadata.array_size = array_size;
    metadata.is_cubemap = cubemap;
    metadata.format = libbsa::dxgi_format{DXGI_FORMAT_BC1_UNORM};
    return metadata;
}

void assert_validated_metadata(const std::vector<std::byte>& bytes,
                               std::uint32_t width,
                               std::uint32_t height,
                               std::uint32_t mip_count,
                               std::uint32_t array_size,
                               bool cubemap)
{
    REQUIRE(bytes.size() > 128);
    CHECK(bytes[0] == std::byte{'D'});
    CHECK(bytes[1] == std::byte{'D'});
    CHECK(bytes[2] == std::byte{'S'});
    CHECK(bytes[3] == std::byte{' '});

    // The validation helper keeps DirectXTex LoadFromDDSMemory private to libbsa internals.
    const auto validated = libbsa::detail::validate_dds(std::span<const std::byte>{bytes});
    REQUIRE(validated.has_value());
    CHECK(validated.value().width == width);
    CHECK(validated.value().height == height);
    CHECK(validated.value().mip_count == mip_count);
    CHECK(validated.value().array_size == array_size);
    CHECK(validated.value().format.value == DXGI_FORMAT_BC1_UNORM);
    CHECK(validated.value().is_cubemap == cubemap);
}

} // namespace

TEST_CASE("reconstruct_dds creates a DirectXTex-valid one mip texture", "[unit][fixture]")
{
    const auto reconstructed = libbsa::detail::reconstruct_dds(texture(4, 4, 1), std::span<const std::byte>{payload(8, std::byte{0x10})});

    REQUIRE(reconstructed.has_value());
    assert_validated_metadata(reconstructed.value(), 4, 4, 1, 1, false);
}

TEST_CASE("reconstruct_dds creates a DirectXTex-valid multi mip texture", "[unit][fixture]")
{
    const auto reconstructed = libbsa::detail::reconstruct_dds(texture(8, 8, 2), std::span<const std::byte>{payload(40, std::byte{0x20})});

    REQUIRE(reconstructed.has_value());
    assert_validated_metadata(reconstructed.value(), 8, 8, 2, 1, false);
}

TEST_CASE("reconstruct_dds creates a DirectXTex-valid cubemap texture", "[unit][fixture]")
{
    const auto reconstructed = libbsa::detail::reconstruct_dds(texture(4, 4, 1, 6, true), std::span<const std::byte>{payload(48, std::byte{0x30})});

    REQUIRE(reconstructed.has_value());
    assert_validated_metadata(reconstructed.value(), 4, 4, 1, 6, true);
}

TEST_CASE("reconstruct_dds creates a DirectXTex-valid array texture", "[unit][fixture]")
{
    const auto reconstructed = libbsa::detail::reconstruct_dds(texture(4, 4, 1, 4), std::span<const std::byte>{payload(32, std::byte{0x40})});

    REQUIRE(reconstructed.has_value());
    assert_validated_metadata(reconstructed.value(), 4, 4, 1, 4, false);
}

TEST_CASE("reconstruct_dds rejects unsupported metadata preconditions", "[unit]")
{
    auto invalid = texture(0, 4, 1);

    const auto reconstructed = libbsa::detail::reconstruct_dds(invalid, std::span<const std::byte>{payload(8, std::byte{0x50})});

    REQUIRE_FALSE(reconstructed.has_value());
    CHECK(reconstructed.error().code == libbsa::error_code::malformed_archive);
}
