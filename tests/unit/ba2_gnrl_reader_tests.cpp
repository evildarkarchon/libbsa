#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "formats/ba2/ba2_gnrl_reader.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <array>
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

libbsa::entry_compression expected_default_compression(const nlohmann::json& manifest) {
  const auto version = manifest.at("version").get<std::uint32_t>();
  if (version == 3U && manifest.at("compression_method").get<std::uint32_t>() == 3U) {
    return libbsa::entry_compression::lz4_block;
  }
  return libbsa::entry_compression::deflate;
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

struct ba2_success_fixture {
  std::string archive;
  std::string manifest;
};

std::vector<ba2_success_fixture> ba2_success_fixtures() {
  return {
      {"ba2_gnrl_fo4.ba2", "ba2_gnrl_fo4_manifest.json"},
      {"ba2_gnrl_sfv2.ba2", "ba2_gnrl_sfv2_manifest.json"},
      {"ba2_gnrl_sfv3.ba2", "ba2_gnrl_sfv3_manifest.json"},
  };
}

void require_common_ba2_metadata(const nlohmann::json& manifest,
                                 const libbsa::archive_metadata& metadata,
                                 libbsa::archive_variant expected_variant) {
  REQUIRE(metadata.type == libbsa::archive_type::ba2);
  REQUIRE(metadata.variant == expected_variant);
  REQUIRE(metadata.version == manifest.at("version").get<std::uint32_t>());
  REQUIRE(metadata.archive_flags == 0U);
  REQUIRE(metadata.file_count == manifest.at("file_count").get<std::uint32_t>());
  REQUIRE(metadata.default_compression == expected_default_compression(manifest));
  REQUIRE(metadata.ba2.has_value());
}

} // namespace

TEST_CASE("ba2_gnrl_detector opens Fallout 4 GNRL metadata without Starfield fields",
          "[unit][fixture][ba2_gnrl_detector]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_fo4_manifest.json"));

  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_fo4.ba2").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  require_common_ba2_metadata(manifest, metadata.value(), libbsa::archive_variant::fallout4);
  REQUIRE_FALSE(metadata.value().ba2->starfield_unknown1.has_value());
  REQUIRE_FALSE(metadata.value().ba2->starfield_unknown2.has_value());
  REQUIRE_FALSE(metadata.value().ba2->compression_method.has_value());
}

TEST_CASE("ba2_gnrl_detector opens Starfield v2 GNRL metadata with version-gated unknowns",
          "[unit][fixture][ba2_gnrl_detector]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_sfv2_manifest.json"));

  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_sfv2.ba2").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  require_common_ba2_metadata(manifest, metadata.value(), libbsa::archive_variant::starfield);
  REQUIRE(metadata.value().ba2->starfield_unknown1 == manifest.at("starfield_unknown1").get<std::uint32_t>());
  REQUIRE(metadata.value().ba2->starfield_unknown2 == manifest.at("starfield_unknown2").get<std::uint32_t>());
  REQUIRE_FALSE(metadata.value().ba2->compression_method.has_value());
}

TEST_CASE("ba2_gnrl_detector opens Starfield v3 GNRL metadata with compression method",
          "[unit][fixture][ba2_gnrl_detector]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_sfv3_manifest.json"));

  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_sfv3.ba2").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  require_common_ba2_metadata(manifest, metadata.value(), libbsa::archive_variant::starfield);
  REQUIRE(metadata.value().ba2->starfield_unknown1 == manifest.at("starfield_unknown1").get<std::uint32_t>());
  REQUIRE(metadata.value().ba2->starfield_unknown2 == manifest.at("starfield_unknown2").get<std::uint32_t>());
  REQUIRE(metadata.value().ba2->compression_method == manifest.at("compression_method").get<std::uint32_t>());
}

TEST_CASE("ba2_gnrl_detector rejects Phase 5 unsupported BA2 profiles with stable errors",
          "[unit][fixture][ba2_gnrl_detector]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_malformed_manifest.json"));

  for (const auto& test_case : manifest.at("cases")) {
    const auto id = test_case.at("id").get<std::string>();
    if (id != "ba2_dx10_unsupported" && id != "ba2_unsupported_v3_compression_method") {
      continue;
    }

    auto opened = libbsa::archive_reader::open(generated_archive_path(test_case.at("archive").get<std::string>()).string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == error_code_from_manifest(test_case.at("expected_error").get<std::string>()));
  }
}

TEST_CASE("ba2_gnrl_metadata lists manifest-backed records from filename tables",
          "[unit][fixture][ba2_gnrl_metadata]") {
  bool saw_raw = false;
  bool saw_deflate = false;
  bool saw_lz4_block = false;

  for (const auto& fixture : ba2_success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
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

      const auto compression = expected.at("compression").get<std::string>();
      saw_raw = saw_raw || compression == "raw";
      saw_deflate = saw_deflate || compression == "deflate";
      saw_lz4_block = saw_lz4_block || compression == "lz4_block";

      REQUIRE(actual.path == expected.at("path").get<std::string>());
      REQUIRE(actual.original_path == archive_original_path_from_manifest(expected.at("original_path").get<std::string>()));
      REQUIRE(actual.raw_size == expected.at("raw_size").get<std::uint64_t>());
      REQUIRE(actual.stored_size == expected.at("stored_size").get<std::uint64_t>());
      REQUIRE(actual.payload_offset == expected.at("payload_offset").get<std::uint64_t>());
      REQUIRE(actual.archive_hash == hex_u64_from_manifest(expected.at("archive_hash")));
      REQUIRE(actual.record_flags == expected.at("record_flags").get<std::uint32_t>());
      REQUIRE(actual.compression == entry_compression_from_manifest(compression));
      REQUIRE_FALSE(actual.has_embedded_name);
      REQUIRE(actual.embedded_name_prefix_size == 0U);
    }
  }

  REQUIRE(saw_raw);
  REQUIRE(saw_deflate);
  REQUIRE(saw_lz4_block);
}

TEST_CASE("ba2_gnrl_lookup normalizes variants and reports stable missing-path behavior",
          "[unit][fixture][ba2_gnrl_lookup]") {
  for (const auto& fixture : ba2_success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
      for (const auto& variant : expected.at("lookup_variants")) {
        auto found = opened.value().find(variant.get<std::string>());
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        REQUIRE(found.value()->path == expected.at("path").get<std::string>());
        REQUIRE(found.value()->archive_hash == hex_u64_from_manifest(expected.at("archive_hash")));
        REQUIRE(found.value()->payload_offset == expected.at("payload_offset").get<std::uint64_t>());
        REQUIRE(found.value()->record_flags == expected.at("record_flags").get<std::uint32_t>());

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
}

TEST_CASE("ba2_gnrl_extract streams manifest bytes and extract_bytes matches", "[unit][fixture][ba2_gnrl_extract]") {
  bool saw_raw = false;
  bool saw_zero_byte_raw = false;
  bool saw_deflate = false;
  bool saw_lz4_block = false;

  for (const auto& fixture : ba2_success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
      const auto compression = expected.at("compression").get<std::string>();
      saw_raw = saw_raw || compression == "raw";
      saw_zero_byte_raw = saw_zero_byte_raw || (compression == "raw" && expected.at("raw_size").get<std::uint64_t>() == 0U);
      saw_deflate = saw_deflate || compression == "deflate";
      saw_lz4_block = saw_lz4_block || compression == "lz4_block";

      const auto expected_bytes = bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>());
      collecting_sink sink;

      auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

      REQUIRE(extracted.has_value());
      REQUIRE(sink.bytes() == expected_bytes);

      auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
      REQUIRE(bytes.has_value());
      REQUIRE(bytes.value() == expected_bytes);
    }
  }

  REQUIRE(saw_raw);
  REQUIRE(saw_zero_byte_raw);
  REQUIRE(saw_deflate);
  REQUIRE(saw_lz4_block);
}

TEST_CASE("ba2_gnrl_extract helper routes by metadata and detects partial_sink writes",
          "[unit][fixture][ba2_gnrl_extract]") {
  auto fo4 = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_fo4.ba2").string());
  REQUIRE(fo4.has_value());
  auto raw_entry = fo4.value().find("meshes/mixedcase/probe.nif");
  REQUIRE(raw_entry.has_value());
  REQUIRE(raw_entry.value().has_value());
  REQUIRE(raw_entry.value()->compression == libbsa::entry_compression::none);
  partial_sink partial;

  auto partial_result = libbsa::formats::ba2::extract_ba2_gnrl_payload(
      generated_archive_path("ba2_gnrl_fo4.ba2").string(), *raw_entry.value(), partial);

  REQUIRE_FALSE(partial_result.has_value());
  REQUIRE(partial_result.error().code == libbsa::error_code::io_error);

  const auto sfv3_manifest = read_json_file(generated_archive_path("ba2_gnrl_sfv3_manifest.json"));
  auto sfv3 = libbsa::archive_reader::open(generated_archive_path("ba2_gnrl_sfv3.ba2").string());
  REQUIRE(sfv3.has_value());
  auto lz4_entry = sfv3.value().find("geometries/packed/block.mesh");
  REQUIRE(lz4_entry.has_value());
  REQUIRE(lz4_entry.value().has_value());
  REQUIRE(lz4_entry.value()->compression == libbsa::entry_compression::lz4_block);
  collecting_sink lz4_sink;

  auto lz4_result = libbsa::formats::ba2::extract_ba2_gnrl_payload(
      generated_archive_path("ba2_gnrl_sfv3.ba2").string(), *lz4_entry.value(), lz4_sink);

  REQUIRE(lz4_result.has_value());
  const auto& expected_lz4 = *std::find_if(sfv3_manifest.at("entries").begin(), sfv3_manifest.at("entries").end(), [](const auto& entry) {
    return entry.at("compression").get<std::string>() == "lz4_block";
  });
  REQUIRE(lz4_sink.bytes() == bytes_from_hex(expected_lz4.at("expected").at("bytes_hex").get<std::string>()));
}

TEST_CASE("ba2_gnrl_malformed manifest cases fail with stable error codes", "[unit][fixture][ba2_gnrl_malformed]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_malformed_manifest.json"));
  constexpr auto required_cases = std::to_array<std::string_view>({"ba2_dx10_unsupported",
                                                                  "ba2_unsupported_v3_compression_method",
                                                                  "ba2_duplicate_canonical_path",
                                                                  "ba2_corrupt_compressed_payload",
                                                                  "ba2_exact_size_mismatch"});
  std::vector<std::string> observed_cases;

  for (const auto& test_case : manifest.at("cases")) {
    const auto id = test_case.at("id").get<std::string>();
    observed_cases.push_back(id);
    const auto archive = generated_archive_path(test_case.at("archive").get<std::string>()).string();
    const auto expected_error = error_code_from_manifest(test_case.at("expected_error").get<std::string>());
    const auto phase = test_case.at("phase").get<std::string>();

    if (phase == "open") {
      auto opened = libbsa::archive_reader::open(archive);

      REQUIRE_FALSE(opened.has_value());
      REQUIRE(opened.error().code == expected_error);
      continue;
    }

    REQUIRE(phase == "extraction");
    auto opened = libbsa::archive_reader::open(archive);
    REQUIRE(opened.has_value());
    collecting_sink sink;

    auto extracted = opened.value().extract(test_case.at("target_path").get<std::string>(), sink);

    REQUIRE_FALSE(extracted.has_value());
    REQUIRE(extracted.error().code == expected_error);
  }

  for (const auto required : required_cases) {
    INFO("required malformed BA2 case: " << required);
    REQUIRE(std::find(observed_cases.begin(), observed_cases.end(), required) != observed_cases.end());
  }
}
