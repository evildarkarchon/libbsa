#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "formats/ba2/ba2_dx10_reader.hpp"
#include "texture/directxtex_analyzer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

constexpr std::size_t dds_dxt10_header_size = 148U;

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
  /// Appends every sink write so tests can compare sink extraction against `extract_bytes`.
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
  /// Simulates a caller sink that accepts only part of each header/chunk write.
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    return bytes.empty() ? 0U : bytes.size() - 1U;
  }
};

struct ba2_dx10_fixture {
  std::string archive;
  std::string manifest;
};

std::vector<ba2_dx10_fixture> success_fixtures() {
  return {{"ba2_dx10_fo4.ba2", "ba2_dx10_fo4_manifest.json"},
          {"ba2_dx10_sfv3.ba2", "ba2_dx10_sfv3_manifest.json"}};
}

const nlohmann::json& find_manifest_entry(const nlohmann::json& manifest, std::string_view path) {
  const auto found = std::find_if(manifest.at("entries").begin(), manifest.at("entries").end(), [&](const auto& entry) {
    return entry.at("path").template get<std::string>() == path;
  });
  REQUIRE(found != manifest.at("entries").end());
  return *found;
}

void require_dds_header_prefix(std::span<const std::byte> bytes) {
  REQUIRE(bytes.size() >= dds_dxt10_header_size);
  REQUIRE(bytes[0] == std::byte{'D'});
  REQUIRE(bytes[1] == std::byte{'D'});
  REQUIRE(bytes[2] == std::byte{'S'});
  REQUIRE(bytes[3] == std::byte{' '});
  REQUIRE(bytes[84] == std::byte{'D'});
  REQUIRE(bytes[85] == std::byte{'X'});
  REQUIRE(bytes[86] == std::byte{'1'});
  REQUIRE(bytes[87] == std::byte{'0'});
}

void require_payload_matches_validated_manifest_order(std::span<const std::byte> extracted, const nlohmann::json& expected) {
  REQUIRE(extracted.size() >= dds_dxt10_header_size);
  const auto expected_payload = bytes_from_hex(expected.at("expected_dds_payload").at("bytes_hex").get<std::string>());
  REQUIRE(std::vector<std::byte>{extracted.begin() + static_cast<std::ptrdiff_t>(dds_dxt10_header_size), extracted.end()} ==
          expected_payload);

  std::size_t payload_offset = dds_dxt10_header_size;
  for (const auto& chunk : expected.at("chunks")) {
    const auto decoded = bytes_from_hex(chunk.at("decoded_bytes_hex").get<std::string>());
    const auto segment = chunk.at("logical_texture_segment");

    // The manifest records the format-derived validated order: each logical_texture_segment identity
    // maps to the parser's source_chunk_index, proving extraction does not concatenate raw archive bytes.
    INFO("array_index=" << segment.at("array_index").get<std::uint32_t>() << " face_index="
                        << segment.at("face_index").get<std::uint32_t>() << " source_chunk_index="
                        << segment.at("source_chunk_index").get<std::uint32_t>());
    REQUIRE(segment.at("start_mip").get<std::uint32_t>() == chunk.at("start_mip").get<std::uint32_t>());
    REQUIRE(segment.at("end_mip").get<std::uint32_t>() == chunk.at("end_mip").get<std::uint32_t>());
    REQUIRE(segment.at("source_chunk_index").get<std::uint32_t>() < expected.at("chunks").size());
    REQUIRE(extracted.size() >= payload_offset + decoded.size());
    REQUIRE(std::equal(decoded.begin(), decoded.end(), extracted.begin() + static_cast<std::ptrdiff_t>(payload_offset)));
    payload_offset += decoded.size();
  }
  REQUIRE(payload_offset == extracted.size());
}

void require_directxtex_metadata_matches_manifest(std::span<const std::byte> dds_bytes, const nlohmann::json& expected) {
  auto metadata = libbsa::texture::analyze_dds_metadata(dds_bytes);
  REQUIRE(metadata.has_value());

  const auto& expected_metadata = expected.at("expected_directxtex_metadata");
  REQUIRE(metadata.value().width == expected_metadata.at("width").get<std::uint32_t>());
  REQUIRE(metadata.value().height == expected_metadata.at("height").get<std::uint32_t>());
  REQUIRE(metadata.value().mip_count == expected_metadata.at("mipLevels").get<std::uint32_t>());
  REQUIRE(metadata.value().dxgi_format == expected_metadata.at("format").get<std::uint32_t>());
  REQUIRE(metadata.value().array_size == expected_metadata.at("arraySize").get<std::uint32_t>());
  REQUIRE(metadata.value().is_cubemap == expected_metadata.at("isCubemap").get<bool>());
}

} // namespace

TEST_CASE("ba2_dx10_extract reconstructs DDS bytes through sink and extract_bytes", "[unit][fixture][ba2_dx10_extract]") {
  for (const auto& fixture : success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
      collecting_sink sink;

      auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

      REQUIRE(extracted.has_value());
      require_dds_header_prefix(sink.bytes());
      require_payload_matches_validated_manifest_order(sink.bytes(), expected);

      auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
      REQUIRE(bytes.has_value());
      REQUIRE(bytes.value() == sink.bytes());
    }
  }
}

TEST_CASE("ba2_dx10_compression routes raw deflate and LZ4 block chunks", "[unit][fixture][ba2_dx10_compression]") {
  bool saw_raw = false;
  bool saw_deflate = false;
  bool saw_lz4_block = false;

  for (const auto& fixture : success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
      auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
      REQUIRE(bytes.has_value());
      require_payload_matches_validated_manifest_order(bytes.value(), expected);

      for (const auto& chunk : expected.at("chunks")) {
        const auto route = chunk.at("compression_route").get<std::string>();
        saw_raw = saw_raw || route == "raw";
        saw_deflate = saw_deflate || route == "deflate";
        saw_lz4_block = saw_lz4_block || route == "lz4_block";
      }
    }
  }

  REQUIRE(saw_raw);
  REQUIRE(saw_deflate);
  REQUIRE(saw_lz4_block);
}

TEST_CASE("ba2_dx10_compressed_chunk_fallbacks preserve reconstructed fixture bytes",
          "[unit][fixture][ba2_dx10_compression][bounded_memory_policy]") {
  bool saw_deflate = false;
  bool saw_lz4_block = false;

  for (const auto& fixture : success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
      bool has_compressed_fallback = false;
      for (const auto& chunk : expected.at("chunks")) {
        const auto route = chunk.at("compression_route").get<std::string>();
        has_compressed_fallback = has_compressed_fallback || route == "deflate" || route == "lz4_block";
        saw_deflate = saw_deflate || route == "deflate";
        saw_lz4_block = saw_lz4_block || route == "lz4_block";
      }
      if (!has_compressed_fallback) {
        continue;
      }
      collecting_sink sink;

      auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

      REQUIRE(extracted.has_value());
      require_payload_matches_validated_manifest_order(sink.bytes(), expected);
    }
  }

  REQUIRE(saw_deflate);
  REQUIRE(saw_lz4_block);
}

TEST_CASE("ba2_dx10_directxtex validates reconstructed DDS metadata", "[unit][fixture][ba2_dx10_directxtex]") {
  for (const auto& fixture : success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.manifest));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
      auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
      REQUIRE(bytes.has_value());
      require_directxtex_metadata_matches_manifest(bytes.value(), expected);
    }
  }
}

TEST_CASE("ba2_dx10_extract detects partial_sink writes after header output", "[unit][fixture][ba2_dx10_extract]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_dx10_fo4_manifest.json"));
  auto opened = libbsa::archive_reader::open(generated_archive_path("ba2_dx10_fo4.ba2").string());
  REQUIRE(opened.has_value());
  const auto& expected = find_manifest_entry(manifest, "textures/generated/fo4raw.dds");
  auto found = opened.value().find(expected.at("path").get<std::string>());
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  partial_sink partial;

  auto result = libbsa::formats::ba2::extract_ba2_dx10_payload(generated_archive_path("ba2_dx10_fo4.ba2").string(),
                                                               *found.value(), partial);

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().code == libbsa::error_code::io_error);
}
