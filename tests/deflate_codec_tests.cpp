#include <catch2/catch_test_macros.hpp>

#include <libbsa/compression.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace {

std::vector<std::byte> sample_bytes()
{
    return {std::byte{0x41}, std::byte{0x41}, std::byte{0x41}, std::byte{0x42}, std::byte{0x43}};
}

} // namespace

TEST_CASE("deflate payloads round-trip through the public dispatcher", "[unit][codec]")
{
    const auto original = sample_bytes();

    const auto packed = libbsa::compress_payload(libbsa::compression_algorithm::deflate,
                                                 std::span<const std::byte>{original});
    REQUIRE(packed.has_value());

    const auto unpacked = libbsa::decompress_payload(libbsa::compression_algorithm::deflate,
                                                     std::span<const std::byte>{packed.value()},
                                                     original.size());
    REQUIRE(unpacked.has_value());
    CHECK(unpacked.value() == original);
}

TEST_CASE("deflate decompression rejects exact-size mismatches", "[unit][codec]")
{
    const auto original = sample_bytes();
    const auto packed = libbsa::compress_payload(libbsa::compression_algorithm::deflate,
                                                 std::span<const std::byte>{original});
    REQUIRE(packed.has_value());

    const auto unpacked = libbsa::decompress_payload(libbsa::compression_algorithm::deflate,
                                                     std::span<const std::byte>{packed.value()},
                                                     6);

    REQUIRE_FALSE(unpacked.has_value());
    CHECK(unpacked.error().code == libbsa::error_code::decompression_failure);
    CHECK(unpacked.error().message == "deflate size mismatch");
}

TEST_CASE("deflate decompression rejects malformed payloads", "[unit][codec]")
{
    const std::vector malformed{std::byte{0xde}, std::byte{0xad}, std::byte{0xbe}, std::byte{0xef}};

    const auto unpacked = libbsa::decompress_payload(libbsa::compression_algorithm::deflate,
                                                     std::span<const std::byte>{malformed},
                                                     5);

    REQUIRE_FALSE(unpacked.has_value());
    CHECK(unpacked.error().code == libbsa::error_code::decompression_failure);
    CHECK(unpacked.error().message == "deflate decompression failed");
}

TEST_CASE("deflate dispatcher handles empty payloads", "[unit][codec]")
{
    const std::vector<std::byte> empty;

    const auto packed = libbsa::compress_payload(libbsa::compression_algorithm::deflate,
                                                 std::span<const std::byte>{empty});
    REQUIRE(packed.has_value());

    const auto unpacked = libbsa::decompress_payload(libbsa::compression_algorithm::deflate,
                                                     std::span<const std::byte>{packed.value()},
                                                     0);
    REQUIRE(unpacked.has_value());
    CHECK(unpacked.value().empty());
}
