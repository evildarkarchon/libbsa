#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "texture/directxtex_analyzer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
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
  REQUIRE((stream.good() || stream.eof()));
  return bytes;
}

std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_writer_tests";
  std::filesystem::create_directories(path);
  return path;
}

struct collecting_sink final : libbsa::payload_sink {
  std::vector<std::byte> bytes;

  libbsa::result<std::size_t> write(std::span<const std::byte> chunk) override {
    bytes.insert(bytes.end(), chunk.begin(), chunk.end());
    return chunk.size();
  }
};

struct writer_proof_case {
  std::string_view id;
  std::string_view format_name;
  libbsa::ba2_dx10_target target;
};

constexpr auto writer_proof_matrix = std::to_array<writer_proof_case>({
    {"bc1_unorm", "BC1_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc1_unorm_srgb", "BC1_UNORM_SRGB", libbsa::ba2_dx10_target::fallout4},
    {"bc3_unorm", "BC3_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc4_unorm", "BC4_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc5_unorm", "BC5_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc5_snorm", "BC5_SNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc6h_uf16", "BC6H_UF16", libbsa::ba2_dx10_target::starfield_v3},
    {"bc7_unorm", "BC7_UNORM", libbsa::ba2_dx10_target::starfield_v3},
    {"r8g8b8a8_unorm_srgb", "R8G8B8A8_UNORM_SRGB", libbsa::ba2_dx10_target::starfield_v3},
    {"b8g8r8a8_unorm", "B8G8R8A8_UNORM", libbsa::ba2_dx10_target::starfield_v3},
    {"r8_unorm", "R8_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"r8g8b8a8_snorm", "R8G8B8A8_SNORM", libbsa::ba2_dx10_target::starfield_v3},
});

std::filesystem::path unique_output_path(std::string_view stem) {
  static std::uint32_t counter = 0;
  auto path = writer_test_dir() / (std::string{stem} + "-" + std::to_string(++counter) + ".ba2");
  std::filesystem::remove(path);
  return path;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.is_open());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
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

const nlohmann::json& valid_source_case(const nlohmann::json& manifest, std::string_view id) {
  const auto found = std::ranges::find_if(manifest.at("valid_cases"), [id](const nlohmann::json& source_case) {
    return source_case.at("id").get<std::string>() == id;
  });
  REQUIRE(found != manifest.at("valid_cases").end());
  return *found;
}

const nlohmann::json& invalid_source_case(const nlohmann::json& manifest, std::string_view id) {
  const auto found = std::ranges::find_if(manifest.at("invalid_cases"), [id](const nlohmann::json& source_case) {
    return source_case.at("id").get<std::string>() == id;
  });
  REQUIRE(found != manifest.at("invalid_cases").end());
  return *found;
}

void add_matrix_cases(libbsa::ba2_dx10_writer& writer,
                      const nlohmann::json& manifest,
                      libbsa::ba2_dx10_target target,
                      std::vector<const nlohmann::json*>& added_cases) {
  for (const auto& matrix_case : writer_proof_matrix) {
    if (matrix_case.target != target) {
      continue;
    }
    const auto& source_case = valid_source_case(manifest, matrix_case.id);
    INFO("writer proof matrix format: " << matrix_case.format_name);
    auto added = writer.add_file(source_case.at("archive_path").get<std::string>(),
                                 (generated_source_dir() / source_case.at("file").get<std::string>()).string());
    REQUIRE(added.has_value());
    added_cases.push_back(&source_case);
  }
}

void require_metadata_matches_source(const libbsa::texture_metadata& texture, const libbsa::texture::dds_source_analysis& source) {
  CHECK(texture.dxgi_format == source.metadata.dxgi_format);
  CHECK(texture.width == source.metadata.width);
  CHECK(texture.height == source.metadata.height);
  CHECK(texture.mip_count == source.metadata.mip_count);
  CHECK(texture.array_size == source.metadata.array_size);
  CHECK(texture.is_cubemap == source.metadata.is_cubemap);
}

void require_extracted_matches_source(std::span<const std::byte> dds_bytes,
                                      const libbsa::texture::dds_source_analysis& source) {
  auto extracted = libbsa::texture::analyze_dds_source(dds_bytes);
  REQUIRE(extracted.has_value());
  require_metadata_matches_source(extracted.value().metadata, source);
  CHECK(extracted.value().image_payload_bytes == source.image_payload_bytes);
}

void require_reader_backed_entry(const libbsa::archive_reader& reader,
                                 const nlohmann::json& source_case,
                                 libbsa::entry_compression expected_compression) {
  const auto archive_path = source_case.at("archive_path").get<std::string>();
  auto found = reader.find(archive_path);
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  auto contains = reader.contains(archive_path);
  REQUIRE(contains.has_value());
  CHECK(contains.value());

  const auto source_bytes = read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>());
  auto source = libbsa::texture::analyze_dds_source(source_bytes);
  REQUIRE(source.has_value());
  REQUIRE(found.value()->texture.has_value());
  require_metadata_matches_source(found.value()->texture.value(), source.value());

  bool saw_compressed_chunk = false;
  for (const auto& chunk : found.value()->texture->chunks) {
    CHECK(chunk.stored_size != 0U);
    CHECK(chunk.compression == expected_compression);
    saw_compressed_chunk = true;
  }
  CHECK(saw_compressed_chunk);

  collecting_sink sink;
  auto extracted_to_sink = reader.extract(archive_path, sink);
  REQUIRE(extracted_to_sink.has_value());
  require_extracted_matches_source(sink.bytes, source.value());

  auto extracted_bytes = reader.extract_bytes(archive_path);
  REQUIRE(extracted_bytes.has_value());
  require_extracted_matches_source(extracted_bytes.value(), source.value());
}

void require_writer_round_trip(libbsa::ba2_dx10_target target,
                               std::uint32_t starfield_compression_method,
                               std::string_view stem,
                               libbsa::entry_compression expected_compression) {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  libbsa::ba2_dx10_writer_options options;
  options.starfield_compression_method = starfield_compression_method;
  libbsa::ba2_dx10_writer writer{target, options};
  std::vector<const nlohmann::json*> added_cases;
  add_matrix_cases(writer, manifest, target, added_cases);
  REQUIRE_FALSE(added_cases.empty());

  const auto output_path = unique_output_path(stem);
  auto written = writer.write_to(output_path.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(output_path.string());
  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  CHECK(metadata.value().type == libbsa::archive_type::ba2);
  CHECK(metadata.value().file_count == added_cases.size());
  CHECK(metadata.value().default_compression == expected_compression);

  if (target == libbsa::ba2_dx10_target::fallout4) {
    CHECK(metadata.value().variant == libbsa::archive_variant::fallout4);
    CHECK(metadata.value().version == 1U);
    REQUIRE(metadata.value().ba2.has_value());
    CHECK_FALSE(metadata.value().ba2->starfield_unknown1.has_value());
    CHECK_FALSE(metadata.value().ba2->compression_method.has_value());
  } else {
    CHECK(metadata.value().variant == libbsa::archive_variant::starfield);
    CHECK(metadata.value().version == 3U);
    REQUIRE(metadata.value().ba2.has_value());
    REQUIRE(metadata.value().ba2->starfield_unknown1.has_value());
    CHECK(metadata.value().ba2->starfield_unknown1.value() == 1U);
    REQUIRE(metadata.value().ba2->starfield_unknown2.has_value());
    CHECK(metadata.value().ba2->starfield_unknown2.value() == 0U);
    REQUIRE(metadata.value().ba2->compression_method.has_value());
    CHECK(metadata.value().ba2->compression_method.value() == starfield_compression_method);
  }

  auto entries = opened.value().entries();
  REQUIRE(entries.has_value());
  CHECK(entries.value().size() == added_cases.size());
  for (const auto* source_case : added_cases) {
    require_reader_backed_entry(opened.value(), *source_case, expected_compression);
  }

  std::filesystem::remove(output_path);
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

TEST_CASE("ba2_dx10_writer::add_file accepts a valid DDS source", "[unit][ba2_dx10_writer][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc7_unorm");

  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
  auto added = writer.add_file(source_case.at("archive_path").get<std::string>(),
                               (generated_source_dir() / source_case.at("file").get<std::string>()).string());

  REQUIRE(added.has_value());
}

TEST_CASE("ba2_dx10_writer::add_file rejects an empty DDS host path", "[unit][ba2_dx10_writer][add]") {
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

  auto added = writer.add_file("textures/empty-host.dds", "");

  REQUIRE_FALSE(added.has_value());
  CHECK(added.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("ba2_dx10_writer::add_file rejects a missing DDS host path with structured error", "[unit][ba2_dx10_writer][add]") {
  const auto missing = writer_test_dir() / "missing-source.dds";
  std::filesystem::remove(missing);
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

  auto added = writer.add_file("textures/missing.dds", missing.string());

  REQUIRE_FALSE(added.has_value());
  CHECK(added.error().code == libbsa::error_code::io_error);
}

TEST_CASE("ba2_dx10_writer::add_file rejects malformed DDS bytes", "[unit][ba2_dx10_writer][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = invalid_source_case(manifest, "malformed_truncated_dds");
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

  auto added = writer.add_file("textures/malformed.dds", (generated_source_dir() / source_case.at("file").get<std::string>()).string());

  REQUIRE_FALSE(added.has_value());
  CHECK(added.error().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_dx10_writer::add_file rejects unsupported DDS formats", "[unit][ba2_dx10_writer][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = invalid_source_case(manifest, "unsupported_r32g32b32a32_float");
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

  auto added = writer.add_file("textures/unsupported.dds", (generated_source_dir() / source_case.at("file").get<std::string>()).string());

  REQUIRE_FALSE(added.has_value());
  CHECK(added.error().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2_dx10_writer::add_file snapshots DDS bytes before later source file changes", "[unit][ba2_dx10_writer][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto original_bytes = read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>());
  const auto scratch_path = writer_test_dir() / "snapshot-source.dds";
  write_binary_file(scratch_path, original_bytes);
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

  auto added = writer.add_file(source_case.at("archive_path").get<std::string>(), scratch_path.string());

  REQUIRE(added.has_value());
  const auto malformed_bytes = read_binary_file(generated_source_dir() / "ba2_dx10_malformed_truncated.dds");
  write_binary_file(scratch_path, malformed_bytes);
  std::filesystem::remove(scratch_path);
  SUCCEED("snapshot add succeeded before the source DDS was overwritten and deleted");
}

TEST_CASE("ba2_dx10_writer::add_file accepts duplicate canonical archive paths for write-time validation",
          "[unit][ba2_dx10_writer][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto source_path = (generated_source_dir() / source_case.at("file").get<std::string>()).string();
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

  auto first = writer.add_file("Textures/Duplicate.dds", source_path);
  auto second = writer.add_file("textures/duplicate.dds", source_path);

  REQUIRE(first.has_value());
  REQUIRE(second.has_value());
}

TEST_CASE("BA2 DX10 writer reopens FO4 deflate archives through archive_reader",
          "[unit][ba2_dx10_writer][fo4][compression]") {
  // DX10 output is compressed-only by public contract; tests intentionally avoid raw per-entry overrides.
  require_writer_round_trip(libbsa::ba2_dx10_target::fallout4, 3U, "fo4-dx10-writer", libbsa::entry_compression::deflate);
}

TEST_CASE("BA2 DX10 writer reopens Starfield method 3 raw LZ4 archives through archive_reader",
          "[unit][ba2_dx10_writer][starfield][compression]") {
  libbsa::ba2_dx10_writer_options options;
  options.starfield_compression_method = 3U;
  CHECK(options.starfield_compression_method == 3U);
  require_writer_round_trip(libbsa::ba2_dx10_target::starfield_v3, options.starfield_compression_method,
                            "starfield-v3-dx10-writer", libbsa::entry_compression::lz4_block);
}

TEST_CASE("BA2 DX10 writer reopens Starfield method 0 deflate archives through archive_reader",
          "[unit][ba2_dx10_writer][starfield][compression]") {
  libbsa::ba2_dx10_writer_options options;
  options.starfield_compression_method = 0U;
  CHECK(options.starfield_compression_method == 0U);
  require_writer_round_trip(libbsa::ba2_dx10_target::starfield_v3, options.starfield_compression_method,
                            "starfield-v3-dx10-deflate-writer", libbsa::entry_compression::deflate);
}
