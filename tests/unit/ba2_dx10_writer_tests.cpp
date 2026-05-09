#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "texture/directxtex_analyzer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_source_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "source";
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream stream{path, std::ios::binary};
  REQUIRE(stream.is_open());
  stream.seekg(0, std::ios::end);
  const auto size = stream.tellg();
  REQUIRE(size >= 0);
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  stream.seekg(0, std::ios::beg);
  stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(stream.good() || stream.eof());
  return bytes;
}

const nlohmann::json& structural_case(const nlohmann::json& manifest, std::string_view key) {
  return manifest.at("structural_cases").at(std::string{key});
}

void require_source_case_shape(const nlohmann::json& source_case) {
  REQUIRE(source_case.contains("format_id"));
  REQUIRE(source_case.contains("format_name"));
  REQUIRE(source_case.contains("width"));
  REQUIRE(source_case.contains("height"));
  REQUIRE(source_case.contains("mip_count"));
  REQUIRE(source_case.contains("array_size"));
  REQUIRE(source_case.contains("is_cubemap"));
  REQUIRE(source_case.contains("archive_path"));
  REQUIRE(source_case.contains("structural_case"));
}

void require_analyzes_valid_source_case(const nlohmann::json& source_case) {
  require_source_case_shape(source_case);
  const auto bytes = read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>());
  auto analysis = libbsa::texture::analyze_dds_source(bytes);
  REQUIRE(analysis.has_value());
  CHECK(analysis.value().metadata.dxgi_format == source_case.at("format_id").get<std::uint32_t>());
  CHECK(analysis.value().metadata.width == source_case.at("width").get<std::uint32_t>());
  CHECK(analysis.value().metadata.height == source_case.at("height").get<std::uint32_t>());
  CHECK(analysis.value().metadata.mip_count == source_case.at("mip_count").get<std::uint32_t>());
  CHECK(analysis.value().metadata.array_size == source_case.at("array_size").get<std::uint32_t>());
  CHECK(analysis.value().metadata.is_cubemap == source_case.at("is_cubemap").get<bool>());
  CHECK_FALSE(analysis.value().dds_bytes.empty());
  CHECK_FALSE(analysis.value().image_payload_bytes.empty());
  CHECK_FALSE(analysis.value().subresources.empty());
}

} // namespace

TEST_CASE("BA2 DX10 writer DDS source manifest covers locked formats", "[unit][fixture][ba2_dx10_writer][dds]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  constexpr auto locked_formats = std::to_array<std::uint32_t>({71U, 72U, 77U, 80U, 83U, 84U, 95U, 98U, 29U, 87U, 61U, 31U});

  std::set<std::uint32_t> present_formats;
  for (const auto& source_case : manifest.at("valid_cases")) {
    require_analyzes_valid_source_case(source_case);
    present_formats.insert(source_case.at("format_id").get<std::uint32_t>());
  }

  for (const auto format : locked_formats) {
    INFO("locked DXGI format id: " << format);
    REQUIRE(present_formats.contains(format));
  }
}

TEST_CASE("BA2 DX10 writer DDS source manifest exposes dedicated structural cases", "[unit][fixture][ba2_dx10_writer][dds]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");

  const auto& multi_mip = structural_case(manifest, "multi_mip_bc7_unorm");
  CHECK(multi_mip.at("file").get<std::string>() == "ba2_dx10_multi_mip_bc7_unorm.dds");
  CHECK(multi_mip.at("format_id").get<std::uint32_t>() == 98U);
  CHECK(multi_mip.at("format_name").get<std::string>() == "BC7_UNORM");
  CHECK(multi_mip.at("width").get<std::uint32_t>() == 128U);
  CHECK(multi_mip.at("height").get<std::uint32_t>() == 128U);
  CHECK(multi_mip.at("mip_count").get<std::uint32_t>() == 5U);
  CHECK(multi_mip.at("array_size").get<std::uint32_t>() == 1U);
  CHECK_FALSE(multi_mip.at("is_cubemap").get<bool>());
  CHECK(multi_mip.at("archive_path").get<std::string>() == "textures/structural/multi_mip_bc7_unorm.dds");
  CHECK(multi_mip.at("structural_case").get<std::string>() == "multi_mip");
  require_analyzes_valid_source_case(multi_mip);

  const auto& array = structural_case(manifest, "array_bc5_unorm_2slice");
  CHECK(array.at("file").get<std::string>() == "ba2_dx10_array_bc5_unorm_2slice.dds");
  CHECK(array.at("format_id").get<std::uint32_t>() == 83U);
  CHECK(array.at("format_name").get<std::string>() == "BC5_UNORM");
  CHECK(array.at("width").get<std::uint32_t>() == 64U);
  CHECK(array.at("height").get<std::uint32_t>() == 64U);
  CHECK(array.at("mip_count").get<std::uint32_t>() == 1U);
  CHECK(array.at("array_size").get<std::uint32_t>() == 2U);
  CHECK_FALSE(array.at("is_cubemap").get<bool>());
  CHECK(array.at("archive_path").get<std::string>() == "textures/structural/array_bc5_unorm_2slice.dds");
  CHECK(array.at("structural_case").get<std::string>() == "array");
  require_analyzes_valid_source_case(array);

  const auto& cubemap = structural_case(manifest, "cubemap_bc1_unorm_6face");
  CHECK(cubemap.at("file").get<std::string>() == "ba2_dx10_cubemap_bc1_unorm_6face.dds");
  CHECK(cubemap.at("format_id").get<std::uint32_t>() == 71U);
  CHECK(cubemap.at("format_name").get<std::string>() == "BC1_UNORM");
  CHECK(cubemap.at("width").get<std::uint32_t>() == 32U);
  CHECK(cubemap.at("height").get<std::uint32_t>() == 32U);
  CHECK(cubemap.at("mip_count").get<std::uint32_t>() == 1U);
  CHECK(cubemap.at("array_size").get<std::uint32_t>() == 1U);
  CHECK(cubemap.at("is_cubemap").get<bool>());
  CHECK(cubemap.at("archive_path").get<std::string>() == "textures/structural/cubemap_bc1_unorm_6face.dds");
  CHECK(cubemap.at("structural_case").get<std::string>() == "cubemap");
  require_analyzes_valid_source_case(cubemap);
}

TEST_CASE("BA2 DX10 writer DDS source manifest rejects malformed and unsupported DDS sources", "[unit][fixture][ba2_dx10_writer][dds]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");

  for (const auto& source_case : manifest.at("invalid_cases")) {
    const auto bytes = read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>());
    auto analysis = libbsa::texture::analyze_dds_source(bytes);
    REQUIRE_FALSE(analysis.has_value());
    CHECK(analysis.error().code == libbsa::error_code::format_error);
    CHECK(source_case.at("expected_error").get<std::string>() == "format_error");
  }
}
