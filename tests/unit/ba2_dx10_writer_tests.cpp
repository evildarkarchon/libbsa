#include <catch2/catch_test_macros.hpp>

#include <detail/bethesda_hash.hpp>

#include <libbsa/libbsa.hpp>

#include "texture/directxtex_analyzer.hpp"

#include <algorithm>
#include <array>
#include <cctype>
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

std::string utf8_string_from_path(const std::filesystem::path& path) {
  // Public writer APIs take UTF-8 host text, so tests must avoid Windows ACP-dependent narrow conversions.
  const auto utf8 = path.u8string();
  return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
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

struct ba2_dx10_record_metadata {
  std::string filename_table_path;
  std::uint32_t name_hash{};
  std::array<char, 4> extension{};
  std::uint32_t directory_hash{};
};

constexpr std::string_view snapshot_directory_prefix = "libbsa-dx10-snapshot-";

constexpr auto writer_proof_matrix = std::to_array<writer_proof_case>({
    {"bc1_unorm", "BC1_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc1_unorm_srgb", "BC1_UNORM_SRGB", libbsa::ba2_dx10_target::starfield_v3},
    {"bc3_unorm", "BC3_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc4_unorm", "BC4_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc5_unorm", "BC5_UNORM", libbsa::ba2_dx10_target::fallout4},
    {"bc5_snorm", "BC5_SNORM", libbsa::ba2_dx10_target::starfield_v3},
    {"bc6h_uf16", "BC6H_UF16", libbsa::ba2_dx10_target::starfield_v3},
    {"bc6h_sf16", "BC6H_SF16", libbsa::ba2_dx10_target::starfield_v3},
    {"bc7_unorm", "BC7_UNORM", libbsa::ba2_dx10_target::starfield_v3},
    {"bc7_unorm_srgb", "BC7_UNORM_SRGB", libbsa::ba2_dx10_target::starfield_v3},
    {"r8g8b8a8_unorm", "R8G8B8A8_UNORM", libbsa::ba2_dx10_target::starfield_v3},
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

std::filesystem::path unique_non_ascii_output_path(std::string_view stem) {
  static std::uint32_t counter = 0;
  auto root = writer_test_dir() / std::filesystem::path{L"libbsa-Angstrom-日本語"} / "outputs";
  std::filesystem::create_directories(root);
  auto path = root / (std::string{stem} + "-" + std::to_string(++counter) + ".ba2");
  std::filesystem::remove(path);
  return path;
}

/// Lists BA2 DX10 snapshot temp directories without querying metadata for unrelated temp entries.
std::set<std::filesystem::path> snapshot_directories() {
  std::set<std::filesystem::path> paths;
  for (const auto& entry : std::filesystem::directory_iterator{std::filesystem::temp_directory_path()}) {
    const auto name = entry.path().filename().string();
    if (name.rfind(snapshot_directory_prefix, 0U) != 0U) {
      continue;
    }

    std::error_code fs_error;
    if (entry.is_directory(fs_error)) {
      paths.insert(entry.path());
    }
  }
  return paths;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.is_open());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
}

std::uint16_t read_u16_le(std::span<const std::byte> bytes, std::size_t offset) {
  REQUIRE(offset + 2U <= bytes.size());
  return static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset])) |
         static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset + 1U]) << 8U);
}

std::uint32_t read_u32_le(std::span<const std::byte> bytes, std::size_t offset) {
  REQUIRE(offset + 4U <= bytes.size());
  std::uint32_t value = 0;
  for (std::uint32_t index = 0; index < 4U; ++index) {
    value |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + index])) << (index * 8U);
  }
  return value;
}

std::uint64_t read_u64_le(std::span<const std::byte> bytes, std::size_t offset) {
  REQUIRE(offset + 8U <= bytes.size());
  std::uint64_t value = 0;
  for (std::uint32_t index = 0; index < 8U; ++index) {
    value |= static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[offset + index])) << (index * 8U);
  }
  return value;
}

std::array<char, 4> read_ascii4(std::span<const std::byte> bytes, std::size_t offset) {
  REQUIRE(offset + 4U <= bytes.size());
  return {static_cast<char>(bytes[offset]), static_cast<char>(bytes[offset + 1U]), static_cast<char>(bytes[offset + 2U]),
          static_cast<char>(bytes[offset + 3U])};
}

std::string canonicalize_archive_path(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::pair<std::string_view, std::string_view> split_directory_file(std::string_view archive_path) noexcept {
  const auto slash = archive_path.find_last_of('/');
  if (slash == std::string_view::npos) {
    return {{}, archive_path};
  }
  return {archive_path.substr(0U, slash), archive_path.substr(slash + 1U)};
}

std::pair<std::string_view, std::string_view> split_stem_extension(std::string_view file_name) {
  const auto dot = file_name.find_last_of('.');
  REQUIRE(dot != std::string_view::npos);
  REQUIRE(dot != 0U);
  REQUIRE(dot + 1U != file_name.size());
  return {file_name.substr(0U, dot), file_name.substr(dot + 1U)};
}

std::array<char, 4> expected_extension_fourcc(std::string_view extension) {
  REQUIRE(extension.size() <= 4U);
  std::array<char, 4> value{};
  std::copy(extension.begin(), extension.end(), value.begin());
  return value;
}

ba2_dx10_record_metadata expected_record_metadata_for(std::string_view archive_path) {
  const auto canonical = canonicalize_archive_path(std::string{archive_path});
  const auto [directory, file_name] = split_directory_file(canonical);
  const auto [stem, extension] = split_stem_extension(file_name);
  return {std::string{archive_path}, libbsa::detail::hash_fo4(stem), expected_extension_fourcc(extension),
          libbsa::detail::hash_fo4(directory)};
}

std::vector<ba2_dx10_record_metadata> read_ba2_dx10_record_metadata(const std::filesystem::path& archive_path) {
  const auto bytes = read_binary_file(archive_path);
  REQUIRE(read_u32_le(bytes, 0U) == 0x5844'5442U);
  const auto version = read_u32_le(bytes, 4U);
  REQUIRE(read_u32_le(bytes, 8U) == 0x3031'5844U);
  const auto file_count = read_u32_le(bytes, 12U);
  const auto file_table_offset = read_u64_le(bytes, 16U);

  std::vector<ba2_dx10_record_metadata> records;
  records.reserve(file_count);
  std::size_t record_cursor = version >= 3U ? 36U : 24U;
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto chunk_count = static_cast<unsigned char>(bytes[record_cursor + 13U]);
    records.push_back({{}, read_u32_le(bytes, record_cursor), read_ascii4(bytes, record_cursor + 4U),
                       read_u32_le(bytes, record_cursor + 8U)});
    record_cursor += 24U + static_cast<std::size_t>(chunk_count) * 24U;
  }

  std::size_t name_cursor = static_cast<std::size_t>(file_table_offset);
  for (auto& record : records) {
    const auto name_size = read_u16_le(bytes, name_cursor);
    name_cursor += 2U;
    REQUIRE(name_cursor + name_size <= bytes.size());
    record.filename_table_path.assign(reinterpret_cast<const char*>(bytes.data() + name_cursor), name_size);
    name_cursor += name_size;
  }
  return records;
}

void require_writer_record_metadata_matches_reference(const std::filesystem::path& archive_path,
                                                      std::span<const std::string_view> added_archive_paths) {
  const auto records = read_ba2_dx10_record_metadata(archive_path);
  REQUIRE(records.size() == added_archive_paths.size());
  for (const auto archive_path_text : added_archive_paths) {
    const auto expected = expected_record_metadata_for(archive_path_text);
    const auto found = std::ranges::find_if(records, [&](const ba2_dx10_record_metadata& record) {
      return record.filename_table_path == expected.filename_table_path;
    });
    REQUIRE(found != records.end());
    CHECK(found->name_hash == expected.name_hash);
    CHECK(found->directory_hash == expected.directory_hash);
    CHECK(found->extension == expected.extension);
  }
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
  std::vector<std::string_view> added_archive_paths;
  added_archive_paths.reserve(added_cases.size());
  for (const auto* source_case : added_cases) {
    added_archive_paths.push_back(source_case->at("archive_path").get_ref<const std::string&>());
    require_reader_backed_entry(opened.value(), *source_case, expected_compression);
  }
  require_writer_record_metadata_matches_reference(output_path, added_archive_paths);

  std::filesystem::remove(output_path);
}

void require_structural_writer_round_trip() {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::starfield_v3};
  std::vector<const nlohmann::json*> added_cases;
  for (const auto id :
       {"multi_mip_bc7_unorm", "array_bc5_unorm_2slice", "cubemap_bc1_unorm_6face", "cubemap_array_bc1_unorm_12face"}) {
    const auto& source_case = valid_source_case(manifest, id);
    auto added = writer.add_file(source_case.at("archive_path").get<std::string>(),
                                 (generated_source_dir() / source_case.at("file").get<std::string>()).string());
    REQUIRE(added.has_value());
    added_cases.push_back(&source_case);
  }

  const auto output_path = unique_output_path("starfield-v3-dx10-structural-writer");
  auto written = writer.write_to(output_path.string());
  REQUIRE(written.has_value());
  auto opened = libbsa::archive_reader::open(output_path.string());
  REQUIRE(opened.has_value());

  for (const auto* source_case : added_cases) {
    require_reader_backed_entry(opened.value(), *source_case, libbsa::entry_compression::lz4_block);
  }

  const auto cubemap = opened.value().find("textures/structural/cubemap_bc1_unorm_6face.dds");
  REQUIRE(cubemap.has_value());
  REQUIRE(cubemap.value().has_value());
  REQUIRE(cubemap.value()->texture.has_value());
  CHECK(cubemap.value()->texture->is_cubemap);

  const auto array = opened.value().find("textures/structural/array_bc5_unorm_2slice.dds");
  REQUIRE(array.has_value());
  REQUIRE(array.value().has_value());
  REQUIRE(array.value()->texture.has_value());
  CHECK(array.value()->texture->array_size == 2U);

  const auto cubemap_array = opened.value().find("textures/structural/cubemap_array_bc1_unorm_12face.dds");
  REQUIRE(cubemap_array.has_value());
  REQUIRE(cubemap_array.value().has_value());
  REQUIRE(cubemap_array.value()->texture.has_value());
  CHECK(cubemap_array.value()->texture->is_cubemap);
  CHECK(cubemap_array.value()->texture->array_size == 2U);

  const auto multi = opened.value().find("textures/structural/multi_mip_bc7_unorm.dds");
  REQUIRE(multi.has_value());
  REQUIRE(multi.value().has_value());
  REQUIRE(multi.value()->texture.has_value());
  CHECK(multi.value()->texture->mip_count == 5U);

  std::filesystem::remove(output_path);
}

std::vector<libbsa::texture_chunk_metadata> texture_chunks_for(const libbsa::archive_reader& reader,
                                                               std::string_view archive_path) {
  auto found = reader.find(archive_path);
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  REQUIRE(found.value()->texture.has_value());
  return found.value()->texture->chunks;
}

void add_duplicate_dds_pair(libbsa::ba2_dx10_writer& writer, const nlohmann::json& source_case) {
  const auto source_path = (generated_source_dir() / source_case.at("file").get<std::string>()).string();
  REQUIRE(writer.add_file("textures/dedupe/a.dds", source_path).has_value());
  REQUIRE(writer.add_file("textures/dedupe/b.dds", source_path).has_value());
}

} // namespace

TEST_CASE("BA2 DX10 writer DDS source manifest covers locked formats", "[unit][fixture][ba2_dx10_writer][dds]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  constexpr auto locked_formats =
      std::to_array<std::uint32_t>({28U, 71U, 72U, 77U, 80U, 83U, 84U, 95U, 96U, 98U, 99U, 29U, 87U, 61U, 31U});

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

  const auto& cubemap_array = structural_case(manifest, "cubemap_array_bc1_unorm_12face");
  CHECK(cubemap_array.at("file").get<std::string>() == "ba2_dx10_cubemap_array_bc1_unorm_12face.dds");
  CHECK(cubemap_array.at("format_id").get<std::uint32_t>() == 71U);
  CHECK(cubemap_array.at("format_name").get<std::string>() == "BC1_UNORM");
  CHECK(cubemap_array.at("width").get<std::uint32_t>() == 32U);
  CHECK(cubemap_array.at("height").get<std::uint32_t>() == 32U);
  CHECK(cubemap_array.at("mip_count").get<std::uint32_t>() == 1U);
  CHECK(cubemap_array.at("array_size").get<std::uint32_t>() == 2U);
  CHECK(cubemap_array.at("is_cubemap").get<bool>());
  CHECK(cubemap_array.at("archive_path").get<std::string>() ==
        "textures/structural/cubemap_array_bc1_unorm_12face.dds");
  CHECK(cubemap_array.at("structural_case").get<std::string>() == "cubemap_array");
  require_analyzes_valid_source_case(cubemap_array);
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

TEST_CASE("BA2 DX10 writer rejects valid non-2D DDS sources", "[unit][ba2_dx10_writer][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");

  for (const auto id : {"unsupported_r8_unorm_1d", "unsupported_r8g8b8a8_unorm_3d"}) {
    const auto& source_case = invalid_source_case(manifest, id);
    const auto source_path = generated_source_dir() / source_case.at("file").get<std::string>();
    const auto bytes = read_binary_file(source_path);
    INFO("non-2D DDS source: " << id);

    auto metadata = libbsa::texture::analyze_dds_metadata(bytes);
    REQUIRE(metadata.has_value());

    auto analysis = libbsa::texture::analyze_dds_source(bytes);
    REQUIRE_FALSE(analysis.has_value());
    CHECK(analysis.error().code == libbsa::error_code::format_error);

    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
    auto added = writer.add_file("textures/non-2d.dds", source_path.string());
    REQUIRE_FALSE(added.has_value());
    CHECK(added.error().code == libbsa::error_code::format_error);
  }
}

TEST_CASE("BA2 DX10 writer rejects Starfield-only DDS formats for Fallout 4 archives", "[unit][ba2_dx10_writer][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");

  for (const auto id : {"bc1_unorm_srgb", "bc5_snorm", "bc6h_uf16", "bc7_unorm_srgb", "r8g8b8a8_unorm_srgb",
                        "r8g8b8a8_snorm"}) {
    const auto& source_case = valid_source_case(manifest, id);
    const auto source_path = generated_source_dir() / source_case.at("file").get<std::string>();
    INFO("Starfield-only format: " << id);

    const auto bytes = read_binary_file(source_path);
    auto analysis = libbsa::texture::analyze_dds_source(bytes);
    REQUIRE(analysis.has_value());

    libbsa::ba2_dx10_writer fallout4_writer{libbsa::ba2_dx10_target::fallout4};
    auto fallout4_added = fallout4_writer.add_file(source_case.at("archive_path").get<std::string>(), source_path.string());
    REQUIRE_FALSE(fallout4_added.has_value());
    CHECK(fallout4_added.error().code == libbsa::error_code::format_error);

    libbsa::ba2_dx10_writer starfield_writer{libbsa::ba2_dx10_target::starfield_v3};
    auto starfield_added = starfield_writer.add_file(source_case.at("archive_path").get<std::string>(), source_path.string());
    REQUIRE(starfield_added.has_value());
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

TEST_CASE("BA2 DX10 writer resolves non-ASCII UTF-8 output host paths", "[unit][ba2_dx10_writer]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto archive_path = source_case.at("archive_path").get<std::string>();
  const auto source_path = generated_source_dir() / source_case.at("file").get<std::string>();
  const auto output_path = unique_non_ascii_output_path("dx10-output");

  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
  REQUIRE(writer.add_file(archive_path, source_path.string()).has_value());

  REQUIRE(writer.write_to(utf8_string_from_path(output_path)).has_value());
  auto opened = libbsa::archive_reader::open(utf8_string_from_path(output_path));
  REQUIRE(opened.has_value());
  require_reader_backed_entry(opened.value(), source_case, libbsa::entry_compression::deflate);
}

TEST_CASE("BA2 DX10 writer records use canonical stem hash and extension metadata",
          "[unit][ba2_dx10_writer][metadata]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  constexpr std::string_view mixed_case_archive_path = "Textures/HashCase/MixedName.DDS";
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
  REQUIRE(writer.add_file(mixed_case_archive_path, (generated_source_dir() / source_case.at("file").get<std::string>()).string())
              .has_value());

  const auto output_path = unique_output_path("dx10-record-metadata");
  REQUIRE(writer.write_to(output_path.string()).has_value());

  constexpr auto added_paths = std::to_array<std::string_view>({mixed_case_archive_path});
  require_writer_record_metadata_matches_reference(output_path, added_paths);
  auto opened = libbsa::archive_reader::open(output_path.string());
  REQUIRE(opened.has_value());
  auto found = opened.value().find("textures/hashcase/mixedname.dds");
  REQUIRE(found.has_value());
  CHECK(found.value().has_value());
  std::filesystem::remove(output_path);
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

TEST_CASE("ba2_dx10_writer::add_file snapshots DDS bytes before later source file changes",
          "[unit][ba2_dx10_writer][bounded_memory_policy][add]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto original_bytes = read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>());
  auto original_source = libbsa::texture::analyze_dds_source(original_bytes);
  REQUIRE(original_source.has_value());
  const auto scratch_path = writer_test_dir() / "snapshot-source.dds";
  write_binary_file(scratch_path, original_bytes);
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

  auto added = writer.add_file(source_case.at("archive_path").get<std::string>(), scratch_path.string());

  REQUIRE(added.has_value());
  const auto malformed_bytes = read_binary_file(generated_source_dir() / "ba2_dx10_malformed_truncated.dds");
  write_binary_file(scratch_path, malformed_bytes);
  std::filesystem::remove(scratch_path);

  const auto output_path = unique_output_path("dx10-snapshot-source-mutated");
  auto written = writer.write_to(output_path.string());
  REQUIRE(written.has_value());
  auto opened = libbsa::archive_reader::open(output_path.string());
  REQUIRE(opened.has_value());
  auto extracted = opened.value().extract_bytes(source_case.at("archive_path").get<std::string>());
  REQUIRE(extracted.has_value());
  require_extracted_matches_source(extracted.value(), original_source.value());
  std::filesystem::remove(output_path);
}

TEST_CASE("BA2 DX10 writer state removes snapshot temp directory on teardown",
          "[unit][ba2_dx10_writer][bounded_memory_policy][cleanup]") {
  const auto before = snapshot_directories();
  std::set<std::filesystem::path> staged_snapshot_dirs;

  {
    const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
    const auto& source_case = valid_source_case(manifest, "bc1_unorm");
    libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};

    REQUIRE(writer.add_file(source_case.at("archive_path").get<std::string>(),
                            (generated_source_dir() / source_case.at("file").get<std::string>()).string())
                .has_value());

    const auto during = snapshot_directories();
    for (const auto& path : during) {
      if (!before.contains(path)) {
        staged_snapshot_dirs.insert(path);
      }
    }

    REQUIRE_FALSE(staged_snapshot_dirs.empty());
    for (const auto& path : staged_snapshot_dirs) {
      INFO("staged BA2 DX10 snapshot directory: " << path.string());
      CHECK(std::filesystem::exists(path));
    }
  }

  for (const auto& path : staged_snapshot_dirs) {
    INFO("teardown-owned BA2 DX10 snapshot directory: " << path.string());
    CHECK_FALSE(std::filesystem::exists(path));
    std::error_code fs_error;
    // If this check fails, still remove the test-created snapshot so later runs start cleanly.
    std::filesystem::remove_all(path, fs_error);
  }
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

TEST_CASE("ba2_dx10_writer reopens fo4 deflate compression archives through archive_reader",
          "[unit][ba2_dx10_writer][fo4][compression]") {
  // DX10 output is compressed-only by public contract; tests intentionally avoid raw per-entry overrides.
  require_writer_round_trip(libbsa::ba2_dx10_target::fallout4, 3U, "fo4-dx10-writer", libbsa::entry_compression::deflate);
}

TEST_CASE("ba2_dx10_writer reopens starfield method 3 raw LZ4 compression archives through archive_reader",
          "[unit][ba2_dx10_writer][starfield][compression]") {
  libbsa::ba2_dx10_writer_options options;
  options.starfield_compression_method = 3U;
  CHECK(options.starfield_compression_method == 3U);
  require_writer_round_trip(libbsa::ba2_dx10_target::starfield_v3, options.starfield_compression_method,
                            "starfield-v3-dx10-writer", libbsa::entry_compression::lz4_block);
}

TEST_CASE("ba2_dx10_writer reopens starfield method 0 deflate compression archives through archive_reader",
          "[unit][ba2_dx10_writer][starfield][compression]") {
  libbsa::ba2_dx10_writer_options options;
  options.starfield_compression_method = 0U;
  CHECK(options.starfield_compression_method == 0U);
  require_writer_round_trip(libbsa::ba2_dx10_target::starfield_v3, options.starfield_compression_method,
                            "starfield-v3-dx10-deflate-writer", libbsa::entry_compression::deflate);
}

TEST_CASE("ba2_dx10_writer preserves multi mip array and cubemap image payload bytes through extraction",
          "[unit][ba2_dx10_writer][starfield][structural]") {
  require_structural_writer_round_trip();
}

TEST_CASE("BA2 DX10 writer keeps duplicate DDS chunk offsets distinct when deduplicate_payloads = false",
          "[unit][ba2_dx10_writer][dedupe]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  libbsa::ba2_dx10_writer_options options;
  options.deduplicate_payloads = false;
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
  add_duplicate_dds_pair(writer, source_case);

  const auto output_path = unique_output_path("dx10-dedupe-disabled-distinct-offsets");
  REQUIRE(writer.write_to(output_path.string()).has_value());

  auto opened = libbsa::archive_reader::open(output_path.string());
  REQUIRE(opened.has_value());
  const auto first_chunks = texture_chunks_for(opened.value(), "textures/dedupe/a.dds");
  const auto second_chunks = texture_chunks_for(opened.value(), "textures/dedupe/b.dds");
  REQUIRE(first_chunks.size() == second_chunks.size());
  for (std::size_t index = 0; index < first_chunks.size(); ++index) {
    CHECK(first_chunks[index].payload_offset != second_chunks[index].payload_offset);
    CHECK(first_chunks[index].compression == libbsa::entry_compression::deflate);
    CHECK(second_chunks[index].compression == libbsa::entry_compression::deflate);
  }
  auto extracted_first = opened.value().extract_bytes("textures/dedupe/a.dds");
  REQUIRE(extracted_first.has_value());
  auto extracted_duplicate = opened.value().extract_bytes("textures/dedupe/b.dds");
  REQUIRE(extracted_duplicate.has_value());
  const auto source = libbsa::texture::analyze_dds_source(read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>()));
  REQUIRE(source.has_value());
  require_extracted_matches_source(extracted_first.value(), source.value());
  require_extracted_matches_source(extracted_duplicate.value(), source.value());
}

TEST_CASE("BA2 DX10 writer shares duplicate DDS chunk offsets when deduplicate_payloads = true",
          "[unit][ba2_dx10_writer][dedupe]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  libbsa::ba2_dx10_writer_options options;
  options.deduplicate_payloads = true;
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
  add_duplicate_dds_pair(writer, source_case);

  const auto output_path = unique_output_path("dx10-dedupe-enabled-shared-offsets");
  REQUIRE(writer.write_to(output_path.string()).has_value());

  auto opened = libbsa::archive_reader::open(output_path.string());
  REQUIRE(opened.has_value());
  const auto first_chunks = texture_chunks_for(opened.value(), "textures/dedupe/a.dds");
  const auto second_chunks = texture_chunks_for(opened.value(), "textures/dedupe/b.dds");
  REQUIRE(first_chunks.size() == second_chunks.size());
  for (std::size_t index = 0; index < first_chunks.size(); ++index) {
    CHECK(first_chunks[index].payload_offset == second_chunks[index].payload_offset);
    CHECK(first_chunks[index].raw_size == second_chunks[index].raw_size);
    CHECK(first_chunks[index].stored_size == second_chunks[index].stored_size);
    CHECK(first_chunks[index].compression == second_chunks[index].compression);
  }
  auto extracted_first = opened.value().extract_bytes("textures/dedupe/a.dds");
  REQUIRE(extracted_first.has_value());
  auto extracted_duplicate = opened.value().extract_bytes("textures/dedupe/b.dds");
  REQUIRE(extracted_duplicate.has_value());
  const auto source = libbsa::texture::analyze_dds_source(read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>()));
  REQUIRE(source.has_value());
  require_extracted_matches_source(extracted_first.value(), source.value());
  require_extracted_matches_source(extracted_duplicate.value(), source.value());
}

TEST_CASE("BA2 DX10 writer refuses to overwrite existing output by default and preserves bytes",
          "[unit][ba2_dx10_writer][publish][overwrite]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto output = writer_test_dir() / "dx10-overwrite-default.ba2";
  const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
  write_binary_file(output, sentinel);
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
  REQUIRE(writer.add_file(source_case.at("archive_path").get<std::string>(),
                          (generated_source_dir() / source_case.at("file").get<std::string>()).string())
              .has_value());

  auto written = writer.write_to(output.string());

  REQUIRE_FALSE(written.has_value());
  CHECK(written.error().code == libbsa::error_code::io_error);
  CHECK(written.error().message.find("BA2 DX10 writer") != std::string::npos);
  CHECK(read_binary_file(output) == sentinel);
}

TEST_CASE("BA2 DX10 writer preserves caller-owned temp-name sibling files during unique temp publish",
          "[unit][ba2_dx10_writer][publish][temp]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto output = unique_output_path("dx10-safe-temp-collision");
  const auto collision = output.string() + ".tmp";
  const std::vector<std::byte> sentinel{std::byte{0x54}, std::byte{0x4D}, std::byte{0x50}};
  write_binary_file(collision, sentinel);
  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
  REQUIRE(writer.add_file(source_case.at("archive_path").get<std::string>(),
                          (generated_source_dir() / source_case.at("file").get<std::string>()).string())
              .has_value());

  auto written = writer.write_to(output.string());

  REQUIRE(written.has_value());
  REQUIRE(std::filesystem::exists(collision));
  CHECK(read_binary_file(collision) == sentinel);
}

TEST_CASE("BA2 DX10 writer rejects non-regular overwrite targets without replacing them",
          "[unit][ba2_dx10_writer][publish][overwrite]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto directory = writer_test_dir() / "dx10-non-regular-overwrite.ba2";
  std::error_code fs_error;
  std::filesystem::remove_all(directory, fs_error);
  REQUIRE(std::filesystem::create_directory(directory));
  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
  REQUIRE(writer.add_file(source_case.at("archive_path").get<std::string>(),
                          (generated_source_dir() / source_case.at("file").get<std::string>()).string())
              .has_value());

  auto written = writer.write_to(directory.string());

  REQUIRE_FALSE(written.has_value());
  CHECK(written.error().code == libbsa::error_code::io_error);
  CHECK(std::filesystem::is_directory(directory));
}

TEST_CASE("BA2 DX10 writer overwrites existing archives when overwrite_existing is true",
          "[unit][ba2_dx10_writer][publish][overwrite]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto& source_case = valid_source_case(manifest, "bc1_unorm");
  const auto output = unique_output_path("dx10-overwrite-existing");
  const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
  write_binary_file(output, sentinel);
  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;
  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
  REQUIRE(writer.add_file(source_case.at("archive_path").get<std::string>(),
                          (generated_source_dir() / source_case.at("file").get<std::string>()).string())
              .has_value());

  auto written = writer.write_to(output.string());

  REQUIRE(written.has_value());
  CHECK(read_binary_file(output) != sentinel);
  auto opened = libbsa::archive_reader::open(output.string());
  REQUIRE(opened.has_value());
  auto extracted = opened.value().extract_bytes(source_case.at("archive_path").get<std::string>());
  REQUIRE(extracted.has_value());
  const auto source = libbsa::texture::analyze_dds_source(read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>()));
  REQUIRE(source.has_value());
  require_extracted_matches_source(extracted.value(), source.value());
}
