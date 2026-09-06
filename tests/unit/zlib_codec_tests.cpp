#include <catch2/catch_test_macros.hpp>

#include <detail/byte_vector.hpp>
#include <detail/zlib_codec.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> zlib_vector() {
    constexpr std::string_view text = "libbsa zlib vector";
    std::vector<std::byte> bytes;
    for (int repeat = 0; repeat < 4; ++repeat) {
        std::transform(text.begin(), text.end(), std::back_inserter(bytes),
                       [](char value) { return static_cast<std::byte>(value); });
    }
    return bytes;
}

std::size_t impossible_byte_vector_size() {
    const auto max_size = std::vector<std::byte>{}.max_size();
    if (max_size == std::numeric_limits<std::size_t>::max()) {
        SKIP("byte vector max_size cannot be overflowed on this standard library");
    }
    return max_size + 1U;
}

/// Reproduces the `Fallout - Misc.bsa` entry the reference tolerates: a
/// complete DEFLATE stream under a valid zlib header, with the four-byte
/// Adler-32 trailer chopped off entirely.
std::vector<std::byte> without_adler32_trailer(std::vector<std::byte> stream) {
    REQUIRE(stream.size() > 4U);
    stream.resize(stream.size() - 4U);
    return stream;
}

}  // namespace

TEST_CASE("zlib_codec emits RFC1950-framed streams", "[unit][compression][zlib]") {
    const auto original = zlib_vector();

    auto compressed = libbsa::detail::compress_zlib(original);
    REQUIRE(compressed);
    REQUIRE(compressed.value().size() >= 6U);

    // Bethesda archives store zlib-wrapped payloads, so the writer must emit a
    // valid RFC1950 header: CM == 8 and the two header bytes a multiple of 31.
    const auto cmf = std::to_integer<std::uint32_t>(compressed.value()[0]);
    const auto flg = std::to_integer<std::uint32_t>(compressed.value()[1]);
    CHECK((cmf & 0x0FU) == 8U);
    CHECK((((cmf << 8U) | flg) % 31U) == 0U);
}

TEST_CASE("zlib_codec round trips exact-size payloads", "[unit][compression][zlib]") {
    const auto original = zlib_vector();

    auto compressed = libbsa::detail::compress_zlib(original);
    REQUIRE(compressed);
    auto decompressed = libbsa::detail::decompress_zlib_exact(compressed.value(), original.size());

    REQUIRE(decompressed);
    REQUIRE(decompressed.value() == original);
}

TEST_CASE("zlib_codec rejects raw deflate payloads", "[unit][malformed][compression][zlib]") {
    const auto original = zlib_vector();
    auto compressed = libbsa::detail::compress_zlib(original);
    REQUIRE(compressed);

    // Strip the RFC1950 header to leave a bare RFC1951 stream. libbsa decoded
    // BA2 payloads this way before issue #42; the zlib route must not accept it.
    std::vector<std::byte> raw_deflate(compressed.value().begin() + 2, compressed.value().end());
    auto decompressed = libbsa::detail::decompress_zlib_exact(raw_deflate, original.size());

    REQUIRE_FALSE(decompressed);
    REQUIRE(decompressed.error().code == libbsa::error_code::format_error);
}

TEST_CASE("zlib_codec rejects truncated and exact-size mismatches",
          "[unit][malformed][compression][zlib]") {
    const auto original = zlib_vector();
    auto compressed = libbsa::detail::compress_zlib(original);
    REQUIRE(compressed);

    auto truncated_bytes = compressed.value();
    truncated_bytes.resize(truncated_bytes.size() / 2);
    auto truncated = libbsa::detail::decompress_zlib_exact(truncated_bytes, original.size());
    REQUIRE_FALSE(truncated);
    REQUIRE(truncated.error().code == libbsa::error_code::format_error);

    auto too_large = libbsa::detail::decompress_zlib_exact(compressed.value(), original.size() + 1);
    REQUIRE_FALSE(too_large);
    REQUIRE(too_large.error().code == libbsa::error_code::format_error);

    auto too_small = libbsa::detail::decompress_zlib_exact(compressed.value(), original.size() - 1);
    REQUIRE_FALSE(too_small);
    REQUIRE(too_small.error().code == libbsa::error_code::format_error);
}

TEST_CASE("zlib_codec tolerates a missing Adler-32 trailer at the exact size",
          "[unit][compression][zlib][compatibility]") {
    const auto original = zlib_vector();
    auto compressed = libbsa::detail::compress_zlib(original);
    REQUIRE(compressed);

    auto trimmed = without_adler32_trailer(compressed.value());
    auto decompressed = libbsa::detail::decompress_zlib_exact(trimmed, original.size());

    REQUIRE(decompressed);
    REQUIRE(decompressed.value() == original);

    // The tolerance is only for the trailer, not for the size contract.
    auto wrong_size = libbsa::detail::decompress_zlib_exact(trimmed, original.size() - 1);
    REQUIRE_FALSE(wrong_size);
    REQUIRE(wrong_size.error().code == libbsa::error_code::format_error);
}

TEST_CASE("zlib_codec rejects a present but corrupt Adler-32 trailer",
          "[unit][malformed][compression][zlib]") {
    const auto original = zlib_vector();
    auto compressed = libbsa::detail::compress_zlib(original);
    REQUIRE(compressed);

    // A full-length trailer that fails verification is corruption, not the
    // vanilla short-stream shape, so the missing-trailer tolerance must not
    // cover it.
    auto corrupted = compressed.value();
    corrupted.back() =
        static_cast<std::byte>(std::to_integer<std::uint32_t>(corrupted.back()) ^ 0xFFU);
    auto decompressed = libbsa::detail::decompress_zlib_exact(corrupted, original.size());

    REQUIRE_FALSE(decompressed);
    REQUIRE(decompressed.error().code == libbsa::error_code::format_error);
}

TEST_CASE("zlib_codec decodes an empty payload to zero bytes",
          "[unit][compression][zlib][compatibility]") {
    // Vanilla Fallout - Misc.bsa stores a zero-length file as a compressed
    // entry whose payload is zero bytes long, with no zlib header at all.
    auto decompressed = libbsa::detail::decompress_zlib_exact({}, 0U);

    REQUIRE(decompressed);
    CHECK(decompressed.value().empty());
}

TEST_CASE("zlib_codec rejects an empty payload when bytes are expected",
          "[unit][malformed][compression][zlib]") {
    auto decompressed = libbsa::detail::decompress_zlib_exact({}, 1U);

    REQUIRE_FALSE(decompressed);
    REQUIRE(decompressed.error().code == libbsa::error_code::format_error);
}

TEST_CASE("zlib_codec translates impossible output allocations",
          "[unit][malformed][compression][zlib][allocation]") {
    auto allocated =
        libbsa::detail::make_byte_vector(impossible_byte_vector_size(), "test zlib allocation");
    REQUIRE_FALSE(allocated);
    REQUIRE(allocated.error().code == libbsa::error_code::format_error);

    auto decompressed = libbsa::detail::decompress_zlib_exact({}, impossible_byte_vector_size());
    REQUIRE_FALSE(decompressed);
    REQUIRE(decompressed.error().code == libbsa::error_code::format_error);
}
