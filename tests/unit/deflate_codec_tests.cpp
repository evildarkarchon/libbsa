#include <catch2/catch_test_macros.hpp>

#include <detail/deflate_codec.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> deflate_vector() {
  constexpr std::string_view text = "libbsa deflate vector";
  std::vector<std::byte> bytes;
  for (int repeat = 0; repeat < 4; ++repeat) {
    std::transform(text.begin(), text.end(), std::back_inserter(bytes), [](char value) { return static_cast<std::byte>(value); });
  }
  return bytes;
}

} // namespace

TEST_CASE("deflate_codec round trips exact-size payloads", "[unit][compression][deflate]") {
  const auto original = deflate_vector();

  auto compressed = libbsa::detail::compress_deflate(original);
  REQUIRE(compressed);
  auto decompressed = libbsa::detail::decompress_deflate_exact(compressed.value(), original.size());

  REQUIRE(decompressed);
  REQUIRE(decompressed.value() == original);
}

TEST_CASE("deflate_codec rejects truncated and exact-size mismatches", "[unit][malformed][compression][deflate]") {
  const auto original = deflate_vector();
  auto compressed = libbsa::detail::compress_deflate(original);
  REQUIRE(compressed);

  auto truncated_bytes = compressed.value();
  truncated_bytes.resize(truncated_bytes.size() / 2);
  auto truncated = libbsa::detail::decompress_deflate_exact(truncated_bytes, original.size());
  REQUIRE_FALSE(truncated);
  REQUIRE(truncated.error().code == libbsa::error_code::format_error);

  auto too_large = libbsa::detail::decompress_deflate_exact(compressed.value(), original.size() + 1);
  REQUIRE_FALSE(too_large);
  REQUIRE(too_large.error().code == libbsa::error_code::format_error);

  auto too_small = libbsa::detail::decompress_deflate_exact(compressed.value(), original.size() - 1);
  REQUIRE_FALSE(too_small);
  REQUIRE(too_small.error().code == libbsa::error_code::format_error);
}
