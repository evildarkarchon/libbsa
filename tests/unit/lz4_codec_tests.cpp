#include <catch2/catch_test_macros.hpp>

#include <detail/byte_vector.hpp>
#include <detail/lz4_block_codec.hpp>
#include <detail/lz4_frame_codec.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> lz4_vector() {
  constexpr std::string_view text = "libbsa lz4 frame and raw block vector";
  std::vector<std::byte> bytes;
  for (int repeat = 0; repeat < 8; ++repeat) {
    std::transform(text.begin(), text.end(), std::back_inserter(bytes), [](char value) { return static_cast<std::byte>(value); });
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

} // namespace

TEST_CASE("lz4_codec frame and raw block round trip independently", "[unit][compression][lz4]") {
  const auto original = lz4_vector();

  auto frame = libbsa::detail::compress_lz4_frame(original);
  REQUIRE(frame);
  auto frame_decoded = libbsa::detail::decompress_lz4_frame_exact(frame.value(), original.size());
  REQUIRE(frame_decoded);
  REQUIRE(frame_decoded.value() == original);

  auto raw_block = libbsa::detail::compress_lz4_block(original);
  REQUIRE(raw_block);
  auto block_decoded = libbsa::detail::decompress_lz4_block_exact(raw_block.value(), original.size());
  REQUIRE(block_decoded);
  REQUIRE(block_decoded.value() == original);
}

TEST_CASE("lz4_codec rejects malformed frame and raw block payloads", "[unit][malformed][compression][lz4]") {
  const auto original = lz4_vector();
  auto frame = libbsa::detail::compress_lz4_frame(original).value();
  auto raw_block = libbsa::detail::compress_lz4_block(original).value();

  frame.resize(frame.size() / 2);
  raw_block.resize(raw_block.size() / 2);

  auto bad_frame = libbsa::detail::decompress_lz4_frame_exact(frame, original.size());
  REQUIRE_FALSE(bad_frame);
  REQUIRE(bad_frame.error().code == libbsa::error_code::format_error);

  auto bad_block = libbsa::detail::decompress_lz4_block_exact(raw_block, original.size());
  REQUIRE_FALSE(bad_block);
  REQUIRE(bad_block.error().code == libbsa::error_code::format_error);
}

TEST_CASE("lz4_codec rejects cross-format and exact-size mismatches", "[unit][malformed][compression][lz4]") {
  const auto original = lz4_vector();
  auto frame = libbsa::detail::compress_lz4_frame(original).value();
  auto raw_block = libbsa::detail::compress_lz4_block(original).value();

  auto raw_as_frame = libbsa::detail::decompress_lz4_frame_exact(raw_block, original.size());
  REQUIRE_FALSE(raw_as_frame);
  REQUIRE(raw_as_frame.error().code == libbsa::error_code::format_error);

  auto frame_as_raw = libbsa::detail::decompress_lz4_block_exact(frame, original.size());
  REQUIRE_FALSE(frame_as_raw);
  REQUIRE(frame_as_raw.error().code == libbsa::error_code::format_error);

  auto frame_wrong_size = libbsa::detail::decompress_lz4_frame_exact(frame, original.size() + 1);
  REQUIRE_FALSE(frame_wrong_size);
  REQUIRE(frame_wrong_size.error().code == libbsa::error_code::format_error);

  auto block_wrong_size = libbsa::detail::decompress_lz4_block_exact(raw_block, original.size() - 1);
  REQUIRE_FALSE(block_wrong_size);
  REQUIRE(block_wrong_size.error().code == libbsa::error_code::format_error);
}

TEST_CASE("byte_vector helpers translate impossible byte-buffer growth", "[unit][allocation][byte_vector]") {
  const auto impossible_size = impossible_byte_vector_size();

  auto allocated = libbsa::detail::make_byte_vector(impossible_size, "test byte allocation");
  REQUIRE_FALSE(allocated);
  REQUIRE(allocated.error().code == libbsa::error_code::format_error);

  std::vector<std::byte> reserved_bytes;
  auto reserved = libbsa::detail::reserve_byte_vector(reserved_bytes, impossible_size, "test byte reserve");
  REQUIRE_FALSE(reserved);
  REQUIRE(reserved.error().code == libbsa::error_code::format_error);

  const std::byte source{};
  auto appended = libbsa::detail::append_byte_vector(reserved_bytes, std::span<const std::byte>{&source, impossible_size},
                                                     "test byte append");
  REQUIRE_FALSE(appended);
  REQUIRE(appended.error().code == libbsa::error_code::format_error);
}

TEST_CASE("lz4_codec translates impossible output allocations", "[unit][malformed][compression][lz4][allocation]") {
  const auto impossible_size = impossible_byte_vector_size();

  auto frame = libbsa::detail::decompress_lz4_frame_exact({}, impossible_size);
  REQUIRE_FALSE(frame);
  REQUIRE(frame.error().code == libbsa::error_code::format_error);

  auto block = libbsa::detail::decompress_lz4_block_exact({}, impossible_size);
  REQUIRE_FALSE(block);
  REQUIRE(block.error().code == libbsa::error_code::format_error);
}
