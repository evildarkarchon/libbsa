#include <catch2/catch_test_macros.hpp>

#include <libbsa/compression.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace {

std::vector<std::byte> lz4_sample()
{
    return {std::byte{0x10}, std::byte{0x20}, std::byte{0x20}, std::byte{0x30}};
}

void require_decompression_failure(const libbsa::result<std::vector<std::byte>>& result)
{
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::decompression_failure);
}

} // namespace

TEST_CASE("LZ4 frame payloads round-trip and carry frame magic", "[unit][codec]")
{
    const auto original = lz4_sample();

    const auto packed = libbsa::compress_payload(libbsa::compression_algorithm::lz4_frame,
                                                 std::span<const std::byte>{original});
    REQUIRE(packed.has_value());
    REQUIRE(packed.value().size() >= 4);
    CHECK(packed.value()[0] == std::byte{0x04});
    CHECK(packed.value()[1] == std::byte{0x22});
    CHECK(packed.value()[2] == std::byte{0x4d});
    CHECK(packed.value()[3] == std::byte{0x18});

    const auto unpacked = libbsa::decompress_payload(libbsa::compression_algorithm::lz4_frame,
                                                     std::span<const std::byte>{packed.value()},
                                                     original.size());
    REQUIRE(unpacked.has_value());
    CHECK(unpacked.value() == original);
}

TEST_CASE("raw LZ4 block payloads round-trip through the block route", "[unit][codec]")
{
    const auto original = lz4_sample();

    const auto packed = libbsa::compress_payload(libbsa::compression_algorithm::lz4_block,
                                                 std::span<const std::byte>{original});
    REQUIRE(packed.has_value());

    const auto unpacked = libbsa::decompress_payload(libbsa::compression_algorithm::lz4_block,
                                                     std::span<const std::byte>{packed.value()},
                                                     original.size());
    REQUIRE(unpacked.has_value());
    CHECK(unpacked.value() == original);
}

TEST_CASE("LZ4 frame and raw block routes reject cross-fed bytes", "[unit][codec]")
{
    const auto original = lz4_sample();
    const auto frame = libbsa::compress_payload(libbsa::compression_algorithm::lz4_frame,
                                                std::span<const std::byte>{original});
    const auto block = libbsa::compress_payload(libbsa::compression_algorithm::lz4_block,
                                                std::span<const std::byte>{original});
    REQUIRE(frame.has_value());
    REQUIRE(block.has_value());

    require_decompression_failure(libbsa::decompress_payload(libbsa::compression_algorithm::lz4_frame,
                                                             std::span<const std::byte>{block.value()},
                                                             original.size()));
    require_decompression_failure(libbsa::decompress_payload(libbsa::compression_algorithm::lz4_block,
                                                             std::span<const std::byte>{frame.value()},
                                                             original.size()));
}

TEST_CASE("raw LZ4 blocks reject exact-size mismatches", "[unit][codec]")
{
    const auto original = lz4_sample();
    const auto packed = libbsa::compress_payload(libbsa::compression_algorithm::lz4_block,
                                                 std::span<const std::byte>{original});
    REQUIRE(packed.has_value());

    const auto unpacked = libbsa::decompress_payload(libbsa::compression_algorithm::lz4_block,
                                                     std::span<const std::byte>{packed.value()},
                                                     5);

    REQUIRE_FALSE(unpacked.has_value());
    CHECK(unpacked.error().code == libbsa::error_code::decompression_failure);
    CHECK(unpacked.error().message == "lz4 block size mismatch");
}
