#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

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
