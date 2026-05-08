#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
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

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  return nlohmann::json::parse(stream);
}

std::uint64_t hex_u64_from_manifest(const nlohmann::json& value) {
  return std::stoull(value.get<std::string>(), nullptr, 16);
}

libbsa::entry_compression entry_compression_from_manifest(std::string_view value) {
  if (value == "raw") {
    return libbsa::entry_compression::none;
  }
  if (value == "deflate") {
    return libbsa::entry_compression::deflate;
  }
  if (value == "lz4_frame") {
    return libbsa::entry_compression::lz4_frame;
  }
  return libbsa::entry_compression::lz4_block;
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

std::string archive_original_path_from_manifest(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  return value;
}

std::vector<std::byte> bytes_from_hex(std::string_view hex) {
  REQUIRE(hex.size() % 2U == 0U);
  std::vector<std::byte> bytes;
  bytes.reserve(hex.size() / 2U);
  for (std::size_t offset = 0; offset < hex.size(); offset += 2U) {
    const auto pair = std::string{hex.substr(offset, 2U)};
    bytes.push_back(static_cast<std::byte>(std::stoul(pair, nullptr, 16)));
  }
  return bytes;
}

std::vector<std::byte> read_binary_file_span(const std::filesystem::path& path, std::uint64_t offset, std::uint64_t count) {
  std::ifstream input{path, std::ios::binary};
  input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  std::vector<std::byte> bytes(static_cast<std::size_t>(count));
  input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return bytes;
}

class collecting_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

 private:
  std::vector<std::byte> bytes_;
};

class partial_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    return bytes.empty() ? 0U : bytes.size() - 1U;
  }
};

} // namespace

TEST_CASE("tes3_bsa_detector classifies Morrowind magic bytes before parser dispatch",
          "[unit][fixture][tes3_bsa_detector]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_success_manifest.json"));
  const auto prefix = read_binary_file_span(generated_archive_path("tes3_success.bsa"), 0U, 12U);

  auto detected = libbsa::formats::bsa::detect_bsa_format(prefix);

  REQUIRE(detected.has_value());
  REQUIRE(detected.value().variant == libbsa::archive_variant::tes3);
  REQUIRE(detected.value().version == manifest.at("version").get<std::uint32_t>());
  REQUIRE(detected.value().version == 0x00000100U);
  REQUIRE(detected.value().default_compression == libbsa::entry_compression::none);
}

TEST_CASE("tes3_bsa_detector leaves unrelated bytes unsupported", "[unit][tes3_bsa_detector]") {
  constexpr std::array unrelated{std::byte{'N'}, std::byte{'O'}, std::byte{'P'}, std::byte{'E'},
                                 std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};

  auto detected = libbsa::formats::bsa::detect_bsa_format(unrelated);

  REQUIRE_FALSE(detected.has_value());
  REQUIRE(detected.error().code == libbsa::error_code::unsupported);
}

TEST_CASE("tes3_bsa_metadata opens generated Morrowind archives", "[unit][fixture][tes3_bsa_metadata]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_success_manifest.json"));

  auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  REQUIRE(metadata.value().type == libbsa::archive_type::bsa);
  REQUIRE(metadata.value().variant == libbsa::archive_variant::tes3);
  REQUIRE(metadata.value().version == manifest.at("version").get<std::uint32_t>());
  REQUIRE(metadata.value().archive_flags == 0U);
  REQUIRE(metadata.value().file_count == manifest.at("file_count").get<std::uint32_t>());
  REQUIRE(metadata.value().default_compression == libbsa::entry_compression::none);
}

TEST_CASE("tes3_bsa_entries exposes manifest-backed metadata and offsets", "[unit][fixture][tes3_bsa_entries]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_success_manifest.json"));
  auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());
  REQUIRE(opened.has_value());

  auto entries = opened.value().entries();
  REQUIRE(entries.has_value());
  REQUIRE(entries.value().size() == manifest.at("entries").size());

  std::vector<std::string> sorted_paths;
  for (const auto& expected : manifest.at("entries")) {
    sorted_paths.push_back(expected.at("path").get<std::string>());
  }
  std::sort(sorted_paths.begin(), sorted_paths.end());

  for (std::size_t index = 0; index < entries.value().size(); ++index) {
    const auto& actual = entries.value().at(index);
    const auto& expected = *std::find_if(manifest.at("entries").begin(), manifest.at("entries").end(), [&](const auto& candidate) {
      return candidate.at("path").get<std::string>() == sorted_paths.at(index);
    });

    REQUIRE(actual.path == expected.at("path").get<std::string>());
    REQUIRE(actual.original_path == archive_original_path_from_manifest(expected.at("original_path").get<std::string>()));
    REQUIRE(actual.raw_size == expected.at("raw_size").get<std::uint64_t>());
    REQUIRE(actual.stored_size == expected.at("stored_size").get<std::uint64_t>());
    REQUIRE(actual.payload_offset == expected.at("payload_offset").get<std::uint64_t>());
    REQUIRE(actual.payload_offset == manifest.at("data_section_start").get<std::uint64_t>() +
                                         expected.at("raw_tes3_data_offset").get<std::uint64_t>());
    REQUIRE(actual.archive_hash == hex_u64_from_manifest(expected.at("archive_hash")));
    REQUIRE(actual.record_flags == 0U);
    REQUIRE(actual.compression == entry_compression_from_manifest(expected.at("compression").get<std::string>()));
    REQUIRE(actual.has_embedded_name == expected.at("has_embedded_name").get<bool>());
    REQUIRE(actual.embedded_name_prefix_size == expected.at("embedded_name_prefix_size").get<std::uint32_t>());
  }
}

TEST_CASE("tes3_bsa_lookup normalizes variants and stable missing-path behavior", "[unit][fixture][tes3_bsa_lookup]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_success_manifest.json"));
  auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());
  REQUIRE(opened.has_value());

  for (const auto& expected : manifest.at("entries")) {
    for (const auto& variant : expected.at("lookup_variants")) {
      auto found = opened.value().find(variant.get<std::string>());
      REQUIRE(found.has_value());
      REQUIRE(found.value().has_value());
      REQUIRE(found.value()->path == expected.at("path").get<std::string>());
      REQUIRE(found.value()->archive_hash == hex_u64_from_manifest(expected.at("archive_hash")));

      auto contains = opened.value().contains(variant.get<std::string>());
      REQUIRE(contains.has_value());
      REQUIRE(contains.value());
    }
  }

  auto missing = opened.value().find("valid/missing/path.txt");
  REQUIRE(missing.has_value());
  REQUIRE_FALSE(missing.value().has_value());

  auto missing_contains = opened.value().contains("valid/missing/path.txt");
  REQUIRE(missing_contains.has_value());
  REQUIRE_FALSE(missing_contains.value());

  for (const std::string invalid : {"", "/rooted/file.txt", "..\\escape.txt", "folder//file.txt"}) {
    auto invalid_find = opened.value().find(invalid);
    REQUIRE_FALSE(invalid_find.has_value());
    REQUIRE(invalid_find.error().code == libbsa::error_code::invalid_argument);

    auto invalid_contains = opened.value().contains(invalid);
    REQUIRE_FALSE(invalid_contains.has_value());
    REQUIRE(invalid_contains.error().code == libbsa::error_code::invalid_argument);
  }
}

TEST_CASE("tes3_bsa_lookup helper normalizes variants over parsed entries", "[unit][fixture][tes3_bsa_lookup]") {
  auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());
  REQUIRE(opened.has_value());
  auto entries = opened.value().entries();
  REQUIRE(entries.has_value());

  auto found = libbsa::formats::bsa::find_tes3_bsa_entry(entries.value(), "MESHES\\TINY\\PROBE.NIF");

  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  REQUIRE(found.value()->path == "meshes/tiny/probe.nif");
}

TEST_CASE("tes3_bsa_extract streams and returns bytes from data-section-relative offsets", "[unit][fixture][tes3_bsa_extract]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_success_manifest.json"));
  auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());
  REQUIRE(opened.has_value());

  for (const auto& expected : manifest.at("entries")) {
    const auto expected_bytes = bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>());
    collecting_sink sink;

    auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

    REQUIRE(extracted.has_value());
    REQUIRE(sink.bytes() == expected_bytes);

    auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
    REQUIRE(bytes.has_value());
    REQUIRE(bytes.value() == expected_bytes);

    const auto archive_bytes = read_binary_file_span(generated_archive_path("tes3_success.bsa"),
                                                     expected.at("payload_offset").get<std::uint64_t>(),
                                                     expected.at("stored_size").get<std::uint64_t>());
    REQUIRE(archive_bytes == expected_bytes);
  }
}

TEST_CASE("tes3_bsa_extract helper is raw-only and enforces sink writes", "[unit][tes3_bsa_extract]") {
  const std::array payload{std::byte{'r'}, std::byte{'a'}, std::byte{'w'}};
  const libbsa::entry_metadata raw_entry{"raw/path.txt", "Raw/Path.txt", 3U, 3U, 0U, 0U,
                                         libbsa::entry_compression::none, 0U, false, 0U};
  collecting_sink sink;

  auto extracted = libbsa::formats::bsa::extract_tes3_bsa_payload(payload, raw_entry, sink);

  REQUIRE(extracted.has_value());
  REQUIRE(sink.bytes() == std::vector<std::byte>{payload.begin(), payload.end()});

  partial_sink partial;
  auto partial_result = libbsa::formats::bsa::extract_tes3_bsa_payload(payload, raw_entry, partial);
  REQUIRE_FALSE(partial_result.has_value());
  REQUIRE(partial_result.error().code == libbsa::error_code::io_error);

  const libbsa::entry_metadata compressed_entry{"raw/path.txt", "Raw/Path.txt", 3U, 3U, 0U, 0U,
                                                libbsa::entry_compression::deflate, 0U, false, 0U};
  collecting_sink compressed_sink;
  auto compressed_result =
      libbsa::formats::bsa::extract_tes3_bsa_payload(payload, compressed_entry, compressed_sink);
  REQUIRE_FALSE(compressed_result.has_value());
  REQUIRE(compressed_result.error().code == libbsa::error_code::format_error);
  REQUIRE(compressed_sink.bytes().empty());
}

TEST_CASE("tes3_bsa_malformed rejects generated malformed TES3 cases with stable error codes",
          "[unit][fixture][malformed][tes3_bsa_malformed]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_malformed_manifest.json"));
  const std::vector<std::string> required_case_ids{
      "tes3_truncated_header",
      "tes3_truncated_records",
      "tes3_invalid_name_span",
      "tes3_invalid_payload_span",
      "tes3_duplicate_canonical_path",
      "tes3_inconsistent_counts_offsets",
      "tes3_stored_hash_mismatch",
      "tes3_hash_collision",
      "tes3_unsorted_hash_records",
      "tes3_raw_offset_absolute_regression",
  };

  for (const auto& required_id : required_case_ids) {
    REQUIRE(std::any_of(manifest.at("cases").begin(), manifest.at("cases").end(), [&](const auto& test_case) {
      return test_case.at("id").get<std::string>() == required_id;
    }));
  }

  for (const auto& test_case : manifest.at("cases")) {
    const auto archive = test_case.at("archive").get<std::string>();
    const auto expected = error_code_from_manifest(test_case.at("expected_error").get<std::string>());

    auto opened = libbsa::archive_reader::open(generated_archive_path(archive).string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == expected);
  }
}
