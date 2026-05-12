#include <catch2/catch_test_macros.hpp>

#include <detail/compression_router.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> router_vector() {
  constexpr std::string_view text = "libbsa explicit compression router vector";
  std::vector<std::byte> bytes;
  for (int repeat = 0; repeat < 4; ++repeat) {
    std::transform(text.begin(), text.end(), std::back_inserter(bytes), [](char value) { return static_cast<std::byte>(value); });
  }
  return bytes;
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.is_open());
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

} // namespace

TEST_CASE("compression_router dispatches explicit codec methods", "[unit][compression][compression-router]") {
  const auto original = router_vector();
  for (auto method : {libbsa::detail::compression_method::deflate, libbsa::detail::compression_method::lz4_frame,
                      libbsa::detail::compression_method::lz4_block}) {
    auto compressed = libbsa::detail::compress_payload(method, original);
    REQUIRE(compressed);
    auto decoded = libbsa::detail::decompress_payload_exact(method, compressed.value(), original.size());
    REQUIRE(decoded);
    REQUIRE(decoded.value() == original);
  }
}

TEST_CASE("compression_router none method preserves exact input bytes", "[unit][compression][compression-router]") {
  const auto original = router_vector();
  auto compressed = libbsa::detail::compress_payload(libbsa::detail::compression_method::none, original);
  REQUIRE(compressed);
  REQUIRE(compressed.value() == original);

  auto decoded = libbsa::detail::decompress_payload_exact(libbsa::detail::compression_method::none, original, original.size());
  REQUIRE(decoded);
  REQUIRE(decoded.value() == original);

  auto wrong_size = libbsa::detail::decompress_payload_exact(libbsa::detail::compression_method::none, original, original.size() + 1);
  REQUIRE_FALSE(wrong_size);
  REQUIRE(wrong_size.error().code == libbsa::error_code::format_error);
}

TEST_CASE("compression_router none method uses result-based byte-vector allocation",
          "[unit][compression][compression-router][bounded_memory_policy]") {
  const auto text = read_text_file(std::filesystem::path{LIBBSA_SOURCE_DIR} / "src" / "detail" / "compression_router.cpp");

  CHECK(text.find("make_byte_vector") != std::string::npos);
  CHECK(text.find("return {input.begin(), input.end()}") == std::string::npos);
}

TEST_CASE("compression_router rejects unsupported explicit enum values", "[unit][malformed][compression][compression-router]") {
  const auto original = router_vector();
  const auto unsupported = static_cast<libbsa::detail::compression_method>(255);

  auto compressed = libbsa::detail::compress_payload(unsupported, original);
  REQUIRE_FALSE(compressed);
  REQUIRE(compressed.error().code == libbsa::error_code::invalid_argument);

  auto decoded = libbsa::detail::decompress_payload_exact(unsupported, original, original.size());
  REQUIRE_FALSE(decoded);
  REQUIRE(decoded.error().code == libbsa::error_code::invalid_argument);
}
