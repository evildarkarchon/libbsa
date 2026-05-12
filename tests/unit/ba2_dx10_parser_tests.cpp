#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <detail/bethesda_hash.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
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
  std::ifstream stream{path, std::ios::binary};
  std::string text{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
  std::string escaped;
  escaped.reserve(text.size());
  for (const char ch : text) {
    // Wave 1 fixture manifests preserve the four-byte BA2 extension field literally, including NUL padding.
    // Escape it at test-read time so the manifest remains parser-free while nlohmann-json can validate content.
    if (ch == '\0') {
      escaped += "\\u0000";
    } else {
      escaped.push_back(ch);
    }
  }
  return nlohmann::json::parse(escaped);
}

libbsa::entry_compression expected_default_compression(const nlohmann::json& manifest) {
  const auto version = manifest.at("version").get<std::uint32_t>();
  if (version == 3U && manifest.at("compression_method").get<std::uint32_t>() == 3U) {
    return libbsa::entry_compression::lz4_block;
  }
  return libbsa::entry_compression::deflate;
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

std::uint64_t hex_u64_from_manifest(const nlohmann::json& value) {
  return std::stoull(value.get<std::string>(), nullptr, 16);
}

std::string archive_original_path_from_manifest(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  return value;
}

/// Reads a complete binary fixture into memory so tests can corrupt selected record fields.
std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

/// Writes a mutated binary fixture to a temporary host path.
void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

/// Overwrites a little-endian UInt32 field inside a mutable binary fixture.
void overwrite_u32_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
  for (std::uint32_t index = 0; index < 4U; ++index) {
    bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
  }
}

/// Reads a little-endian UInt32 field from a binary fixture.
std::uint32_t read_u32_le(const std::vector<std::byte>& bytes, std::size_t offset) {
  std::uint32_t value = 0;
  for (std::uint32_t index = 0; index < 4U; ++index) {
    value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.at(offset + index))) << (index * 8U);
  }
  return value;
}

const nlohmann::json& manifest_entry_for_path(const nlohmann::json& manifest, std::string_view path) {
  const auto found = std::find_if(manifest.at("entries").begin(), manifest.at("entries").end(), [&](const auto& entry) {
    return entry.at("path").get<std::string>() == path;
  });
  REQUIRE(found != manifest.at("entries").end());
  return *found;
}

void write_u8(std::ofstream& out, std::uint8_t value) {
  const auto byte = static_cast<char>(value);
  out.write(&byte, 1);
}

void write_u16(std::ofstream& out, std::uint16_t value) {
  write_u8(out, static_cast<std::uint8_t>(value & 0xFFU));
  write_u8(out, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void write_u32(std::ofstream& out, std::uint32_t value) {
  for (std::uint32_t index = 0; index < 4U; ++index) {
    write_u8(out, static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
  }
}

void write_u64(std::ofstream& out, std::uint64_t value) {
  for (std::uint32_t index = 0; index < 8U; ++index) {
    write_u8(out, static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
  }
}

void write_ascii4(std::ofstream& out, std::string_view value) {
  REQUIRE(value.size() == 4U);
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void write_ascii4(std::ofstream& out, const std::array<char, 4U>& value) {
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void write_sparse_dx10_archive(const std::filesystem::path& path) {
  constexpr std::uint64_t sparse_payload_offset = 4ULL * 1024ULL * 1024ULL * 1024ULL;
  constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
  constexpr std::uint16_t chunk_header_size = 24U;
  const std::string original_path = "Textures/Generated/Sparse.dds";
  const auto name_hash = libbsa::detail::hash_fo4("sparse");
  const auto directory_hash = libbsa::detail::hash_fo4("textures/generated");
  const auto file_table_offset = std::uint64_t{72U};

  std::filesystem::create_directories(path.parent_path());
  std::ofstream out{path, std::ios::binary | std::ios::trunc};
  REQUIRE(out);

  write_ascii4(out, "BTDX");
  write_u32(out, 1U);
  write_ascii4(out, "DX10");
  write_u32(out, 1U);
  write_u64(out, file_table_offset);

  write_u32(out, name_hash);
  write_ascii4(out, std::array<char, 4U>{'d', 'd', 's', '\0'});
  write_u32(out, directory_hash);
  write_u8(out, 0U);
  write_u8(out, 1U);
  write_u16(out, chunk_header_size);
  write_u16(out, 1U);
  write_u16(out, 1U);
  write_u8(out, 1U);
  write_u8(out, 28U);
  write_u16(out, 0U);
  write_u64(out, sparse_payload_offset);
  write_u32(out, 0U);
  write_u32(out, 4U);
  write_u16(out, 0U);
  write_u16(out, 0U);
  write_u32(out, ba2_record_sentinel);

  write_u16(out, static_cast<std::uint16_t>(original_path.size()));
  out.write(original_path.data(), static_cast<std::streamsize>(original_path.size()));
  out.seekp(static_cast<std::streamoff>(sparse_payload_offset), std::ios::beg);
  const std::array<char, 4U> payload{static_cast<char>(0x10), static_cast<char>(0x20), static_cast<char>(0x30), static_cast<char>(0x40)};
  out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  REQUIRE(out);
}

void require_common_dx10_metadata(const nlohmann::json& manifest,
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

void require_texture_metadata(const libbsa::entry_metadata& actual, const nlohmann::json& expected) {
  REQUIRE(actual.texture.has_value());
  const auto& texture = *actual.texture;
  const auto& public_texture = expected.at("expected_public_texture_metadata");
  REQUIRE(texture.width == public_texture.at("width").get<std::uint32_t>());
  REQUIRE(texture.height == public_texture.at("height").get<std::uint32_t>());
  REQUIRE(texture.mip_count == public_texture.at("mip_count").get<std::uint32_t>());
  REQUIRE(texture.dxgi_format == public_texture.at("dxgi_format").get<std::uint32_t>());
  REQUIRE(texture.array_size == public_texture.at("array_size").get<std::uint32_t>());
  REQUIRE(texture.is_cubemap == public_texture.at("is_cubemap").get<bool>());
  REQUIRE(texture.unknown_tex == expected.at("unknown_tex").get<std::uint8_t>());
  REQUIRE(texture.cube_maps_raw == expected.at("cube_maps_raw").get<std::uint16_t>());
  REQUIRE(texture.chunks.size() == expected.at("chunks").size());

  std::uint64_t raw_payload_size = 0;
  std::uint64_t stored_payload_size = 0;
  for (std::size_t index = 0; index < texture.chunks.size(); ++index) {
    const auto& actual_chunk = texture.chunks.at(index);
    const auto& expected_chunk = expected.at("chunks").at(index);
    REQUIRE(actual_chunk.payload_offset == expected_chunk.at("offset").get<std::uint64_t>());
    REQUIRE(actual_chunk.raw_size == expected_chunk.at("raw_size").get<std::uint32_t>());
    const auto packed_size = expected_chunk.at("packed_size").get<std::uint32_t>();
    REQUIRE(actual_chunk.stored_size == (packed_size == 0U ? actual_chunk.raw_size : packed_size));
    REQUIRE(actual_chunk.start_mip == expected_chunk.at("start_mip").get<std::uint16_t>());
    REQUIRE(actual_chunk.end_mip == expected_chunk.at("end_mip").get<std::uint16_t>());
    REQUIRE(actual_chunk.compression == entry_compression_from_manifest(expected_chunk.at("compression_route").get<std::string>()));
    raw_payload_size += actual_chunk.raw_size;
    stored_payload_size += actual_chunk.stored_size;
  }

  REQUIRE(actual.raw_size == 148U + raw_payload_size);
  REQUIRE(actual.stored_size == stored_payload_size);
}

} // namespace

TEST_CASE("ba2_dx10_detector opens generated FO4 texture archive", "[unit][fixture][ba2_dx10_detector]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_dx10_fo4_manifest.json"));

  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_dx10_fo4.ba2").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  require_common_dx10_metadata(manifest, metadata.value(), libbsa::archive_variant::fallout4);
  REQUIRE_FALSE(metadata.value().ba2->starfield_unknown1.has_value());
  REQUIRE_FALSE(metadata.value().ba2->starfield_unknown2.has_value());
  REQUIRE_FALSE(metadata.value().ba2->compression_method.has_value());
}

TEST_CASE("ba2_dx10_detector opens generated Starfield v3 texture archive", "[unit][fixture][ba2_dx10_detector]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_dx10_sfv3_manifest.json"));

  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_dx10_sfv3.ba2").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  require_common_dx10_metadata(manifest, metadata.value(), libbsa::archive_variant::starfield);
  REQUIRE(metadata.value().ba2->compression_method == manifest.at("compression_method").get<std::uint32_t>());
  REQUIRE(metadata.value().default_compression == libbsa::entry_compression::lz4_block);
}

TEST_CASE("ba2_dx10_metadata exposes texture chunks from manifest records", "[unit][fixture][ba2_dx10_metadata]") {
  bool saw_cubemap = false;
  bool saw_array = false;
  bool saw_deflate = false;
  bool saw_lz4_block = false;

  for (const auto fixture : {"ba2_dx10_fo4", "ba2_dx10_sfv3"}) {
    const auto manifest = read_json_file(generated_archive_path(std::string{fixture} + "_manifest.json"));
    auto opened = libbsa::archive_reader::open(generated_archive_path(std::string{fixture} + ".ba2").string());
    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == manifest.at("entries").size());

    for (const auto& entry : entries.value()) {
      const auto& expected = manifest_entry_for_path(manifest, entry.path);
      REQUIRE(entry.original_path == archive_original_path_from_manifest(expected.at("original_path").get<std::string>()));
      REQUIRE(entry.payload_offset == expected.at("chunks").front().at("offset").get<std::uint64_t>());
      REQUIRE(entry.archive_hash == hex_u64_from_manifest(expected.at("name_hash")));
      REQUIRE(entry.record_flags == expected.at("unknown_tex").get<std::uint32_t>());
      REQUIRE_FALSE(entry.has_embedded_name);
      REQUIRE(entry.embedded_name_prefix_size == 0U);
      require_texture_metadata(entry, expected);

      saw_cubemap = saw_cubemap || entry.texture->is_cubemap;
      saw_array = saw_array || entry.texture->array_size > 1U;
      for (const auto& chunk : entry.texture->chunks) {
        saw_deflate = saw_deflate || chunk.compression == libbsa::entry_compression::deflate;
        saw_lz4_block = saw_lz4_block || chunk.compression == libbsa::entry_compression::lz4_block;
      }
    }
  }

  REQUIRE(saw_cubemap);
  REQUIRE(saw_array);
  REQUIRE(saw_deflate);
  REQUIRE(saw_lz4_block);
}

TEST_CASE("ba2_dx10_lookup preserves canonical lowercase paths and original spelling", "[unit][fixture][ba2_dx10_detector]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_dx10_fo4_manifest.json"));
  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_dx10_fo4.ba2").string());
  REQUIRE(opened.has_value());

  for (const auto& expected : manifest.at("entries")) {
    const auto canonical = expected.at("path").get<std::string>();
    auto found = opened.value().find(expected.at("original_path").get<std::string>());
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    REQUIRE(found.value()->path == canonical);
    REQUIRE(found.value()->original_path == archive_original_path_from_manifest(expected.at("original_path").get<std::string>()));
    REQUIRE(found.value()->texture.has_value());

    auto contains = opened.value().contains(canonical);
    REQUIRE(contains.has_value());
    REQUIRE(contains.value());
  }

  auto missing = opened.value().find("textures/generated/missing.dds");
  REQUIRE(missing.has_value());
  REQUIRE_FALSE(missing.value().has_value());
}

TEST_CASE("ba2_dx10_detector opens sparse archive without reading the payload gap",
          "[unit][fixture][bounded_memory_policy][ba2_dx10_detector][sparse]") {
  const auto sparse_path = std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_sparse_gap.ba2";
  write_sparse_dx10_archive(sparse_path);

  const auto start = std::chrono::steady_clock::now();
  auto opened = libbsa::archive_reader::open(sparse_path.string());
  const auto open_duration = std::chrono::steady_clock::now() - start;

  REQUIRE(opened.has_value());
  REQUIRE(open_duration < std::chrono::seconds{4});
  auto entries = opened.value().entries();
  REQUIRE(entries.has_value());
  REQUIRE(entries.value().size() == 1U);
  auto found = opened.value().find("Textures/Generated/Sparse.dds");
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  REQUIRE(found.value()->path == "textures/generated/sparse.dds");
  auto contains = opened.value().contains("textures/generated/sparse.dds");
  REQUIRE(contains.has_value());
  REQUIRE(contains.value());
}

TEST_CASE("ba2_dx10_detector returns format_error for oversized declared filename table offsets",
          "[unit][fixture][malformed][ba2_dx10_detector][allocation]") {
  const auto temp_path = std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_oversized_filename_offset.ba2";
  std::error_code remove_error;
  std::filesystem::remove(temp_path, remove_error);

  {
    std::ofstream out{temp_path, std::ios::binary | std::ios::trunc};
    REQUIRE(out);
    write_ascii4(out, "BTDX");
    write_u32(out, 1U);
    write_ascii4(out, "DX10");
    write_u32(out, 1U);
    write_u64(out, std::numeric_limits<std::uint64_t>::max());
    REQUIRE(out);
  }

  auto opened = libbsa::archive_reader::open(temp_path.string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::format_error);

  std::filesystem::remove(temp_path, remove_error);
}

TEST_CASE("ba2_dx10 aggregate overflow fixtures are unreachable under record field bounds",
          "[unit][fixture][ba2_dx10_detector][allocation]") {
  constexpr std::uint64_t max_dx10_chunks_per_texture = std::numeric_limits<std::uint8_t>::max();
  constexpr std::uint64_t max_dx10_chunk_size = std::numeric_limits<std::uint32_t>::max();
  constexpr std::uint64_t reconstructed_dds_header_size = 148U;

  // BA2 DX10 encodes chunk count as UInt8 and each chunk size as UInt32, so an archive fixture cannot
  // reach the defensive UInt64 aggregate-overflow branch without first violating the record schema.
  constexpr std::uint64_t max_payload_aggregate = max_dx10_chunks_per_texture * max_dx10_chunk_size;

  REQUIRE(max_payload_aggregate <= std::numeric_limits<std::uint64_t>::max() - reconstructed_dds_header_size);
  REQUIRE(max_payload_aggregate + reconstructed_dds_header_size == 1'095'216'660'373ULL);
}

TEST_CASE("ba2_dx10_detector rejects record hash mismatches",
          "[unit][fixture][malformed][ba2_dx10_detector][ba2_dx10_hash_lookup]") {
  constexpr std::size_t first_record_name_hash_offset = 24U;
  constexpr std::size_t first_record_extension_offset = 28U;
  constexpr std::size_t first_record_directory_hash_offset = 32U;

  SECTION("NameHash") {
    auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
    overwrite_u32_le(bytes, first_record_name_hash_offset,
                     read_u32_le(bytes, first_record_name_hash_offset) ^ 0x1000U);

    const auto mutated = std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_name_hash_mismatch.ba2";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(mutated.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);

    std::error_code ignored;
    std::filesystem::remove(mutated, ignored);
  }

  SECTION("record extension") {
    auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
    const std::array<std::byte, 4U> mismatched_extension{std::byte{'n'}, std::byte{'i'}, std::byte{'f'}, std::byte{0}};
    std::copy(mismatched_extension.begin(), mismatched_extension.end(), bytes.begin() + first_record_extension_offset);

    const auto mutated = std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_extension_mismatch.ba2";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(mutated.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);

    std::error_code ignored;
    std::filesystem::remove(mutated, ignored);
  }

  SECTION("DirectoryHash") {
    auto bytes = read_binary_file(generated_archive_path("ba2_dx10_fo4.ba2"));
    overwrite_u32_le(bytes, first_record_directory_hash_offset,
                     read_u32_le(bytes, first_record_directory_hash_offset) ^ 0x1000U);

    const auto mutated = std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_directory_hash_mismatch.ba2";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(mutated.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);

    std::error_code ignored;
    std::filesystem::remove(mutated, ignored);
  }
}

TEST_CASE("ba2_dx10_layout exposes validated order and rejects contradictory format-defined order",
          "[unit][fixture][ba2_dx10_layout]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_dx10_fo4_manifest.json"));
  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_dx10_fo4.ba2").string());
  REQUIRE(opened.has_value());

  auto cube = opened.value().find("textures/generated/fo4cube.dds");
  REQUIRE(cube.has_value());
  REQUIRE(cube.value().has_value());
  REQUIRE(cube.value()->texture.has_value());
  const auto& expected_cube = manifest_entry_for_path(manifest, cube.value()->path);
  for (std::size_t chunk_index = 0; chunk_index < cube.value()->texture->chunks.size(); ++chunk_index) {
    const auto& logical_texture_segment = expected_cube.at("chunks").at(chunk_index).at("logical_texture_segment");
    REQUIRE(logical_texture_segment.at("array_index").get<std::uint32_t>() == 0U);
    REQUIRE(logical_texture_segment.at("face_index").get<std::uint32_t>() == chunk_index);
    REQUIRE(logical_texture_segment.at("start_mip").get<std::uint32_t>() == 0U);
    REQUIRE(logical_texture_segment.at("end_mip").get<std::uint32_t>() == 0U);
    REQUIRE(logical_texture_segment.at("source_chunk_index").get<std::uint32_t>() == chunk_index);
  }

  const auto malformed = read_json_file(generated_archive_path("ba2_dx10_malformed_manifest.json"));
  auto duplicate_case = std::find_if(malformed.at("cases").begin(), malformed.at("cases").end(), [](const auto& candidate) {
    return candidate.at("id").get<std::string>() == "ba2_dx10_duplicate_mip_face";
  });
  REQUIRE(duplicate_case != malformed.at("cases").end());
  auto rejected = libbsa::archive_reader::open(generated_archive_path(duplicate_case->at("archive").get<std::string>()).string());
  REQUIRE_FALSE(rejected.has_value());
  REQUIRE(rejected.error().code == libbsa::error_code::format_error);
}
