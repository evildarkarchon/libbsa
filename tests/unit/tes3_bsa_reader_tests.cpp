#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <detail/host_file_path.hpp>

#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"

#include <detail/bethesda_hash.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
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
  FAIL("unknown TES3 malformed expected_error: " << value);
  return libbsa::error_code::format_error;
}

std::vector<std::byte> bytes_from_text(std::string_view value) {
  std::vector<std::byte> bytes;
  bytes.reserve(value.size());
  for (const char ch : value) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
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

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

std::uint32_t read_u32_le(const std::vector<std::byte>& bytes, std::size_t offset) {
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.at(offset))) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.at(offset + 1U))) << 8U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.at(offset + 2U))) << 16U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.at(offset + 3U))) << 24U);
}

void overwrite_u32_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
  for (std::uint32_t index = 0; index < 4U; ++index) {
    bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
  }
}

void overwrite_u64_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint64_t value) {
  for (std::uint32_t index = 0; index < 8U; ++index) {
    bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
  }
}

void append_u32_le(std::vector<std::byte>& bytes, std::uint32_t value) {
  for (std::uint32_t index = 0; index < 4U; ++index) {
    bytes.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
  }
}

void append_u64_le(std::vector<std::byte>& bytes, std::uint64_t value) {
  for (std::uint32_t index = 0; index < 8U; ++index) {
    bytes.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
  }
}

void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
  std::ofstream output{path, std::ios::binary};
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

struct synthetic_tes3_entry {
  std::string path;
  std::vector<std::byte> payload;
  std::uint32_t raw_offset{0U};
  std::uint64_t archive_hash{0U};
};

struct synthetic_tes3_archive {
  std::vector<std::byte> bytes;
  std::vector<synthetic_tes3_entry> hash_ordered_entries;
  std::uint32_t data_section_start{0U};
};

std::uint32_t checked_test_u32(std::size_t value, std::string_view description) {
  INFO(description);
  REQUIRE(value <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()));
  return static_cast<std::uint32_t>(value);
}

bool tes3_hash_less(const synthetic_tes3_entry& lhs, const synthetic_tes3_entry& rhs) noexcept {
  const auto lhs_key = libbsa::detail::tes3_hash_sort_key(lhs.archive_hash);
  const auto rhs_key = libbsa::detail::tes3_hash_sort_key(rhs.archive_hash);
  if (lhs_key != rhs_key) {
    return lhs_key < rhs_key;
  }
  return lhs.path < rhs.path;
}

synthetic_tes3_archive build_synthetic_tes3_archive(std::vector<synthetic_tes3_entry> entries, bool sort_by_hash = true) {
  for (auto& entry : entries) {
    entry.archive_hash = libbsa::detail::hash_tes3(entry.path);
  }
  if (sort_by_hash) {
    std::sort(entries.begin(), entries.end(), tes3_hash_less);
  }

  std::uint32_t name_table_size = 0U;
  for (const auto& entry : entries) {
    name_table_size += checked_test_u32(entry.path.size() + 1U, "TES3 synthetic name table size");
  }

  constexpr std::uint32_t tes3_magic_version = 0x0000'0100U;
  constexpr std::uint32_t fixed_header_size = 12U;
  const auto file_count = checked_test_u32(entries.size(), "TES3 synthetic file count");
  const auto records_size = checked_test_u32(entries.size() * 8U, "TES3 synthetic file records size");
  const auto name_offsets_size = checked_test_u32(entries.size() * 4U, "TES3 synthetic name offsets size");
  const auto hash_records_size = checked_test_u32(entries.size() * 8U, "TES3 synthetic hash records size");
  const auto hash_table_start = checked_test_u32(fixed_header_size + records_size + name_offsets_size + name_table_size,
                                                "TES3 synthetic hash table start");
  const auto data_section_start = checked_test_u32(hash_table_start + hash_records_size,
                                                  "TES3 synthetic data section start");

  std::size_t payload_bytes_size = 0U;
  for (const auto& entry : entries) {
    payload_bytes_size = std::max(payload_bytes_size, static_cast<std::size_t>(entry.raw_offset) + entry.payload.size());
  }
  std::vector<std::byte> payload_bytes(payload_bytes_size, std::byte{0});
  for (const auto& entry : entries) {
    std::copy(entry.payload.begin(), entry.payload.end(), payload_bytes.begin() + static_cast<std::ptrdiff_t>(entry.raw_offset));
  }

  std::vector<std::byte> bytes;
  bytes.reserve(data_section_start + payload_bytes.size());
  append_u32_le(bytes, tes3_magic_version);
  append_u32_le(bytes, hash_table_start - fixed_header_size);
  append_u32_le(bytes, file_count);
  for (const auto& entry : entries) {
    append_u32_le(bytes, checked_test_u32(entry.payload.size(), "TES3 synthetic payload size"));
    append_u32_le(bytes, entry.raw_offset);
  }

  std::uint32_t name_offset = 0U;
  for (const auto& entry : entries) {
    append_u32_le(bytes, name_offset);
    name_offset += checked_test_u32(entry.path.size() + 1U, "TES3 synthetic name offset");
  }
  for (const auto& entry : entries) {
    for (const char ch : entry.path) {
      bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    bytes.push_back(std::byte{0});
  }
  for (const auto& entry : entries) {
    append_u64_le(bytes, entry.archive_hash);
  }
  bytes.insert(bytes.end(), payload_bytes.begin(), payload_bytes.end());

  return synthetic_tes3_archive{std::move(bytes), std::move(entries), data_section_start};
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

class recording_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    write_sizes.push_back(bytes.size());
    return bytes.size();
  }

  std::vector<std::size_t> write_sizes;
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
  const auto host_path = std::filesystem::temp_directory_path() / "libbsa_tes3_raw_payload_source.bsa";
  write_binary_file(host_path, std::vector<std::byte>{payload.begin(), payload.end()});
  collecting_sink sink;

  auto resolved_host_path = libbsa::detail::resolve_host_file_path(host_path.string());
  REQUIRE(resolved_host_path.has_value());

  auto extracted = libbsa::formats::bsa::extract_tes3_bsa_payload(resolved_host_path.value(), raw_entry, sink);

  REQUIRE(extracted.has_value());
  REQUIRE(sink.bytes() == std::vector<std::byte>{payload.begin(), payload.end()});

  partial_sink partial;
  auto partial_result = libbsa::formats::bsa::extract_tes3_bsa_payload(resolved_host_path.value(), raw_entry, partial);
  REQUIRE_FALSE(partial_result.has_value());
  REQUIRE(partial_result.error().code == libbsa::error_code::io_error);

  constexpr std::size_t expected_chunk_ceiling = 64U * 1024U;
  std::vector<std::byte> large_payload(expected_chunk_ceiling + 17U, std::byte{'x'});
  const auto large_host_path = std::filesystem::temp_directory_path() / "libbsa_tes3_large_payload_source.bsa";
  write_binary_file(large_host_path, large_payload);
  const libbsa::entry_metadata large_entry{"large/path.bin", "Large/Path.bin", large_payload.size(), large_payload.size(),
                                            0U, 0U, libbsa::entry_compression::none, 0U, false, 0U};
  recording_sink recording;

  auto resolved_large_host_path = libbsa::detail::resolve_host_file_path(large_host_path.string());
  REQUIRE(resolved_large_host_path.has_value());

  auto large_result =
      libbsa::formats::bsa::extract_tes3_bsa_payload(resolved_large_host_path.value(), large_entry, recording);

  REQUIRE(large_result.has_value());
  REQUIRE(recording.write_sizes.size() > 1U);
  REQUIRE(std::all_of(recording.write_sizes.begin(), recording.write_sizes.end(), [](std::size_t size) {
    return size <= expected_chunk_ceiling;
  }));

  const libbsa::entry_metadata compressed_entry{"raw/path.txt", "Raw/Path.txt", 3U, 3U, 0U, 0U,
                                                 libbsa::entry_compression::deflate, 0U, false, 0U};
  collecting_sink compressed_sink;
  auto compressed_result =
      libbsa::formats::bsa::extract_tes3_bsa_payload(resolved_host_path.value(), compressed_entry, compressed_sink);
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

    if (test_case.at("id").get<std::string>() == "tes3_stored_hash_mismatch") {
      REQUIRE(test_case.at("structural_issue").get<std::string>() == "stored_hash_mismatch");
    }
    if (test_case.at("id").get<std::string>() == "tes3_hash_collision") {
      REQUIRE(test_case.at("structural_issue").get<std::string>() == "duplicate_stored_hash");
    }

    auto opened = libbsa::archive_reader::open(generated_archive_path(archive).string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == expected);
  }
}

TEST_CASE("tes3_bsa_malformed rejects overlapping non-empty payload spans", "[unit][tes3_bsa_malformed]") {
  auto archive = build_synthetic_tes3_archive({
      {.path = "meshes/overlap/a.nif", .payload = bytes_from_text("aaaa"), .raw_offset = 0U},
      {.path = "meshes/overlap/b.nif", .payload = bytes_from_text("bbbb"), .raw_offset = 2U},
  });
  const auto archive_path = std::filesystem::temp_directory_path() / "libbsa_tes3_overlapping_payload_spans.bsa";
  write_binary_file(archive_path, archive.bytes);

  auto opened = libbsa::archive_reader::open(archive_path.string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::format_error);
  CHECK(opened.error().message.find("payload spans overlap") != std::string::npos);
}

TEST_CASE("tes3_bsa_entries accepts adjacent spans and zero-byte boundary entries", "[unit][tes3_bsa_metadata]") {
  auto archive = build_synthetic_tes3_archive({
      {.path = "meshes/boundary/a.nif", .payload = bytes_from_text("abc"), .raw_offset = 0U},
      {.path = "meshes/boundary/empty.txt", .payload = {}, .raw_offset = 3U},
      {.path = "meshes/boundary/b.nif", .payload = bytes_from_text("de"), .raw_offset = 3U},
  });
  const auto archive_path = std::filesystem::temp_directory_path() / "libbsa_tes3_adjacent_and_empty_spans.bsa";
  write_binary_file(archive_path, archive.bytes);

  auto opened = libbsa::archive_reader::open(archive_path.string());

  REQUIRE(opened.has_value());
  auto entries = opened.value().entries();
  REQUIRE(entries.has_value());
  REQUIRE(entries.value().size() == 3U);

  auto first = opened.value().find("meshes/boundary/a.nif");
  REQUIRE(first.has_value());
  REQUIRE(first.value().has_value());
  CHECK(first.value()->payload_offset == archive.data_section_start);
  CHECK(first.value()->raw_size == 3U);

  auto empty = opened.value().find("meshes/boundary/empty.txt");
  REQUIRE(empty.has_value());
  REQUIRE(empty.value().has_value());
  CHECK(empty.value()->payload_offset == archive.data_section_start + 3U);
  CHECK(empty.value()->raw_size == 0U);

  auto second = opened.value().find("meshes/boundary/b.nif");
  REQUIRE(second.has_value());
  REQUIRE(second.value().has_value());
  CHECK(second.value()->payload_offset == archive.data_section_start + 3U);
  CHECK(second.value()->raw_size == 2U);
}

TEST_CASE("tes3_bsa_malformed reports unsorted hashes before payload overlap", "[unit][tes3_bsa_malformed]") {
  std::vector<synthetic_tes3_entry> entries{
      {.path = "meshes/precedence/a.nif", .payload = bytes_from_text("aaaa"), .raw_offset = 0U},
      {.path = "meshes/precedence/b.nif", .payload = bytes_from_text("bbbb"), .raw_offset = 2U},
      {.path = "meshes/precedence/c.nif", .payload = bytes_from_text("cccc"), .raw_offset = 8U},
  };
  for (auto& entry : entries) {
    entry.archive_hash = libbsa::detail::hash_tes3(entry.path);
  }
  std::sort(entries.begin(), entries.end(), tes3_hash_less);
  std::swap(entries[0], entries[1]);
  REQUIRE(libbsa::detail::tes3_hash_sort_key(entries[0].archive_hash) >
          libbsa::detail::tes3_hash_sort_key(entries[1].archive_hash));

  auto archive = build_synthetic_tes3_archive(std::move(entries), false);
  const auto archive_path = std::filesystem::temp_directory_path() / "libbsa_tes3_hash_precedes_overlap.bsa";
  write_binary_file(archive_path, archive.bytes);

  auto opened = libbsa::archive_reader::open(archive_path.string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::format_error);
  CHECK(opened.error().message.find("hash records are not sorted") != std::string::npos);
  CHECK(opened.error().message.find("payload spans overlap") == std::string::npos);
}

TEST_CASE("tes3_bsa_entries materializes large non-overlapping spans outside payload order",
          "[unit][tes3_bsa_metadata]") {
  constexpr std::size_t entry_count = 1024U;
  std::vector<synthetic_tes3_entry> entries;
  entries.reserve(entry_count);
  for (std::size_t index = 0; index < entry_count; ++index) {
    entries.push_back(synthetic_tes3_entry{
        .path = "meshes/large/span_" + std::to_string(index) + ".bin",
        .payload = {static_cast<std::byte>(index & 0xFFU)},
        .raw_offset = 0U,
    });
  }
  for (auto& entry : entries) {
    entry.archive_hash = libbsa::detail::hash_tes3(entry.path);
  }
  std::sort(entries.begin(), entries.end(), tes3_hash_less);
  for (std::size_t index = 0; index < entries.size(); ++index) {
    entries[index].raw_offset = checked_test_u32((entries.size() - index - 1U) * 2U,
                                                "TES3 synthetic reverse payload offset");
  }
  REQUIRE_FALSE(std::is_sorted(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.raw_offset < rhs.raw_offset;
  }));

  auto archive = build_synthetic_tes3_archive(std::move(entries), false);
  const auto archive_path = std::filesystem::temp_directory_path() / "libbsa_tes3_large_reverse_payload_order.bsa";
  write_binary_file(archive_path, archive.bytes);

  auto opened = libbsa::archive_reader::open(archive_path.string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  CHECK(metadata.value().file_count == entry_count);
  auto parsed_entries = opened.value().entries();
  REQUIRE(parsed_entries.has_value());
  CHECK(parsed_entries.value().size() == entry_count);
}

TEST_CASE("tes3_bsa_malformed returns format_error for oversized declared metadata without exceptions",
          "[unit][fixture][malformed][tes3_bsa_malformed][allocation]") {
  auto bytes = read_binary_file(generated_archive_path("tes3_success.bsa"));
  overwrite_u32_le(bytes, 4U, 0xFFFF'FFF0U);

  const auto mutated = std::filesystem::temp_directory_path() / "libbsa_tes3_oversized_metadata.bsa";
  write_binary_file(mutated, bytes);

  auto opened = libbsa::archive_reader::open(mutated.string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE("tes3_bsa_malformed rejects declared file counts above the metadata limit",
          "[unit][fixture][malformed][tes3_bsa_malformed]") {
  auto bytes = read_binary_file(generated_archive_path("tes3_success.bsa"));
  overwrite_u32_le(bytes, 8U, static_cast<std::uint32_t>(libbsa::detail::metadata_entry_count_limit + 1U));

  const auto mutated = std::filesystem::temp_directory_path() / "libbsa_tes3_excessive_file_count.bsa";
  write_binary_file(mutated, bytes);

  auto opened = libbsa::archive_reader::open(mutated.string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::format_error);

  auto validated = libbsa::validate_archive(mutated.string());
  REQUIRE(validated.has_value());
  REQUIRE_FALSE(validated.value().is_valid());
  REQUIRE(validated.value().errors.size() == 1U);
  CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
}

TEST_CASE("tes3_bsa_malformed maps invalid archive names to format_error", "[unit][fixture][malformed][tes3_bsa_malformed]") {
  auto bytes = read_binary_file(generated_archive_path("tes3_success.bsa"));
  const auto file_count = read_u32_le(bytes, 8U);
  const auto name_section_start = 12U + file_count * 8U + file_count * 4U;
  const auto first_name_offset = read_u32_le(bytes, 12U + file_count * 8U);
  const auto first_name_start = name_section_start + first_name_offset;

  std::string original;
  for (auto offset = first_name_start; offset < bytes.size() && bytes.at(offset) != std::byte{0}; ++offset) {
    original.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes.at(offset))));
  }
  REQUIRE_FALSE(original.empty());
  auto mutated_name = original;
  mutated_name.front() = '/';

  std::transform(mutated_name.begin(), mutated_name.end(), bytes.begin() + static_cast<std::ptrdiff_t>(first_name_start), [](char value) {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
  });

  const auto hash_table_start = 12U + read_u32_le(bytes, 4U);
  overwrite_u64_le(bytes, hash_table_start, libbsa::detail::hash_tes3(mutated_name));
  const auto mutated = std::filesystem::temp_directory_path() / "libbsa_tes3_invalid_archive_name.bsa";
  write_binary_file(mutated, bytes);

  auto opened = libbsa::archive_reader::open(mutated.string());

  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == libbsa::error_code::format_error);
}
