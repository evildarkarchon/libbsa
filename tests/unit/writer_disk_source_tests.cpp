#include <catch2/catch_test_macros.hpp>

#include <detail/writer_disk_source.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace {

libbsa::detail::writer_disk_source_context test_context() noexcept {
  return {"test failed to open source",
          "test failed to inspect source",
          "test failed while reading source",
          "test source changed",
          "test source allocation"};
}

std::filesystem::path source_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_writer_disk_source_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path source_path(std::string_view name) { return source_test_dir() / std::string{name}; }

std::vector<std::byte> bytes_from_text(std::string_view text) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (const char ch : text) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.good());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
}

} // namespace

TEST_CASE("writer_disk_source reads exact whole-file payloads", "[unit][writer-source-io]") {
  const auto path = source_path("exact.bin");
  const auto expected = bytes_from_text("exact source bytes");
  write_binary_file(path, expected);

  auto bytes = libbsa::detail::read_disk_source_exact(path.string(), static_cast<std::uint64_t>(expected.size()),
                                                      test_context());

  REQUIRE(bytes.has_value());
  CHECK(bytes.value() == expected);
}

TEST_CASE("writer_disk_source reads bounded short prefixes", "[unit][writer-source-io]") {
  const auto path = source_path("prefix.bin");
  const auto expected = bytes_from_text("abc");
  write_binary_file(path, expected);

  auto short_prefix = libbsa::detail::read_disk_source_prefix(path.string(), 8U, test_context());
  auto bounded_prefix = libbsa::detail::read_disk_source_prefix(path.string(), 2U, test_context());

  REQUIRE(short_prefix.has_value());
  CHECK(short_prefix.value() == expected);
  REQUIRE(bounded_prefix.has_value());
  CHECK(bounded_prefix.value() == std::vector<std::byte>{expected.begin(), expected.begin() + 2});
}

TEST_CASE("writer_disk_source iterates bounded chunks", "[unit][writer-source-io]") {
  const auto path = source_path("chunks.bin");
  const auto expected = bytes_from_text("chunked source bytes");
  write_binary_file(path, expected);

  std::vector<std::byte> visited;
  std::size_t callback_count = 0;
  auto iterated = libbsa::detail::for_each_disk_source_chunk(
      path.string(),
      static_cast<std::uint64_t>(expected.size()),
      test_context(),
      [&](std::span<const std::byte> chunk) -> libbsa::result<void> {
        ++callback_count;
        visited.insert(visited.end(), chunk.begin(), chunk.end());
        CHECK(chunk.size() <= 5U);
        return {};
      },
      5U);

  REQUIRE(iterated.has_value());
  CHECK(callback_count > 1U);
  CHECK(visited == expected);
}

TEST_CASE("writer_disk_source reports missing sources with caller diagnostics", "[unit][writer-source-io]") {
  const auto missing = source_path("missing.bin");
  std::filesystem::remove(missing);

  auto bytes = libbsa::detail::read_disk_source_exact(missing.string(), 1U, test_context());

  REQUIRE_FALSE(bytes.has_value());
  CHECK(bytes.error().code == libbsa::error_code::io_error);
  CHECK(bytes.error().message == "test failed to inspect source");
}

TEST_CASE("writer_disk_source rejects sources that shrink or grow", "[unit][writer-source-io]") {
  const auto path = source_path("changed.bin");
  const auto expected = bytes_from_text("stable");

  SECTION("shrunk before exact read") {
    write_binary_file(path, std::span<const std::byte>{expected.data(), expected.size() - 1U});

    auto bytes = libbsa::detail::read_disk_source_exact(path.string(), static_cast<std::uint64_t>(expected.size()),
                                                        test_context());

    REQUIRE_FALSE(bytes.has_value());
    CHECK(bytes.error().code == libbsa::error_code::io_error);
    CHECK(bytes.error().message == "test source changed");
  }

  SECTION("grown before exact read") {
    auto grown = expected;
    grown.push_back(std::byte{0x21});
    write_binary_file(path, grown);

    auto bytes = libbsa::detail::read_disk_source_exact(path.string(), static_cast<std::uint64_t>(expected.size()),
                                                        test_context());

    REQUIRE_FALSE(bytes.has_value());
    CHECK(bytes.error().code == libbsa::error_code::io_error);
    CHECK(bytes.error().message == "test source changed");
  }
}

TEST_CASE("writer_disk_source reports allocation limits through results", "[unit][writer-source-io]") {
  const auto path = source_path("allocation.bin");
  write_binary_file(path, bytes_from_text("small"));

  auto bytes = libbsa::detail::read_disk_source_prefix(path.string(), std::numeric_limits<std::size_t>::max(),
                                                       test_context());

  REQUIRE_FALSE(bytes.has_value());
  CHECK(bytes.error().code == libbsa::error_code::format_error);
}
