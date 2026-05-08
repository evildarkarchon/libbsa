#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

std::filesystem::path generated_archive_path(std::string_view filename) {
  return generated_archive_dir() / std::string{filename};
}

libbsa::entry_compression expected_default_compression(std::uint32_t version) {
  return version == 105U ? libbsa::entry_compression::lz4_frame : libbsa::entry_compression::deflate;
}

libbsa::archive_variant expected_variant(std::uint32_t version) {
  (void)version;
  return libbsa::archive_variant::tes4;
}

struct success_fixture {
  std::string archive_filename;
  std::uint32_t version;
  std::uint32_t flags;
  std::uint32_t file_count;
};

std::vector<success_fixture> success_fixtures() {
  return {
      {.archive_filename = "tes4_v103.bsa", .version = 103U, .flags = 3U, .file_count = 2U},
      {.archive_filename = "tes4_v104.bsa", .version = 104U, .flags = 259U, .file_count = 2U},
      {.archive_filename = "tes4_v105.bsa", .version = 105U, .flags = 259U, .file_count = 2U},
  };
}

libbsa::error_code error_code_from_manifest(std::string_view value) {
  if (value == "format_error") {
    return libbsa::error_code::format_error;
  }
  if (value == "unsupported") {
    return libbsa::error_code::unsupported;
  }
  return libbsa::error_code::invalid_argument;
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
  std::ofstream output{path, std::ios::binary};
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void overwrite_u32_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
  for (std::uint32_t index = 0; index < 4U; ++index) {
    bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
  }
}

} // namespace

TEST_CASE("tes4_bsa_detection opens byte-driven TES4-family BSA variants", "[unit][fixture][tes4_bsa_detection]") {
  for (const auto& fixture : success_fixtures()) {
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());

    REQUIRE(opened.has_value());
  }
}

TEST_CASE("tes4_bsa_detection rejects non-BSA bytes regardless of host extension", "[unit][fixture][tes4_bsa_detection]") {
  auto opened = libbsa::archive_reader::open(generated_archive_path("malformed_non_bsa_bytes.bsa").string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::unsupported);
}

TEST_CASE("tes4_bsa_metadata exposes archive-level open state", "[unit][fixture][tes4_bsa_metadata]") {
  for (const auto& fixture : success_fixtures()) {
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
    REQUIRE(opened.has_value());

    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    REQUIRE(metadata.value().type == libbsa::archive_type::bsa);
    REQUIRE(metadata.value().variant == expected_variant(fixture.version));
    REQUIRE(metadata.value().version == fixture.version);
    REQUIRE(metadata.value().archive_flags == fixture.flags);
    REQUIRE(metadata.value().file_count == fixture.file_count);
    REQUIRE(metadata.value().default_compression == expected_default_compression(fixture.version));
  }
}

TEST_CASE("unsupported_future_bsa reports unsupported for recognized future BSA versions",
          "[unit][fixture][unsupported_future_bsa]") {
  auto opened = libbsa::archive_reader::open(generated_archive_path("malformed_unsupported_version.bsa").string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::unsupported);
}

TEST_CASE("tes4_bsa_malformed_open rejects malformed open-phase fixtures with stable error codes",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
  std::ifstream manifest_stream{generated_archive_path("malformed_manifest.json")};
  const auto manifest = nlohmann::json::parse(manifest_stream);

  for (const auto& test_case : manifest.at("cases")) {
    if (test_case.at("phase").get<std::string>() != "open") {
      continue;
    }
    if (test_case.at("id").get<std::string>() == "duplicate_canonical_path") {
      continue;
    }
    const auto archive = test_case.at("archive").get<std::string>();
    const auto expected = error_code_from_manifest(test_case.at("expected_error").get<std::string>());
    auto opened = libbsa::archive_reader::open(generated_archive_path(archive).string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == expected);
  }
}

TEST_CASE("tes4_bsa_malformed_open rejects count-derived table spans before allocation",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
  auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
  overwrite_u32_le(bytes, 24U, 0xFFFF'FFFFU);

  const auto mutated = std::filesystem::temp_directory_path() / "libbsa_oversized_folder_names.bsa";
  write_binary_file(mutated, bytes);

  auto opened = libbsa::archive_reader::open(mutated.string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::format_error);
}
