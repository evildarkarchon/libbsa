#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <nlohmann/json.hpp>

#include <detail/bethesda_hash.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_tes3_bsa_writer_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path output_path(std::string name) { return writer_test_dir() / std::move(name); }

void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.good());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  REQUIRE(input.good());

  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  REQUIRE(input.good());
  return nlohmann::json::parse(input);
}

std::vector<std::byte> sample_bytes() {
  return {std::byte{0x42}, std::byte{0x53}, std::byte{0x41}, std::byte{0x21}};
}

std::uint32_t read_u32_le_at(const std::vector<std::byte>& bytes, std::size_t offset) {
  REQUIRE(offset + 4U <= bytes.size());
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1U])) << 8U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2U])) << 16U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3U])) << 24U);
}

std::uint64_t read_u64_le_at(const std::vector<std::byte>& bytes, std::size_t offset) {
  REQUIRE(offset + 8U <= bytes.size());
  std::uint64_t value = 0;
  for (std::uint32_t index = 0; index < 8U; ++index) {
    value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + index])) << (index * 8U);
  }
  return value;
}

std::string read_null_terminated_name_at(const std::vector<std::byte>& bytes, std::size_t offset, std::size_t limit) {
  REQUIRE(offset < limit);
  REQUIRE(limit <= bytes.size());
  std::string value;
  for (std::size_t index = offset; index < limit; ++index) {
    if (bytes[index] == std::byte{0}) {
      return value;
    }
    value.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes[index])));
  }
  FAIL("TES3 writer name is not null terminated");
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (const char ch : text) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
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

void require_extracted_bytes(const libbsa::archive_reader& reader,
                             std::string_view path,
                             const std::vector<std::byte>& expected) {
  auto extracted = reader.extract_bytes(path);
  REQUIRE(extracted.has_value());
  CHECK(extracted.value() == expected);
}

void require_extracts_bytes(const libbsa::archive_reader& reader,
                            std::string_view path,
                            const std::vector<std::byte>& expected) {
  require_extracted_bytes(reader, path, expected);

  collecting_sink sink;
  auto streamed = reader.extract(path, sink);
  REQUIRE(streamed.has_value());
  CHECK(sink.bytes() == expected);
}

libbsa::entry_metadata require_finds_entry(const libbsa::archive_reader& reader, std::string_view path) {
  auto found = reader.find(path);
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  return *found.value();
}

void require_contains_lookup_variants(const libbsa::archive_reader& reader,
                                      std::string_view expected_canonical_path,
                                      std::span<const std::string_view> lookup_variants) {
  for (const auto lookup : lookup_variants) {
    auto contained = reader.contains(lookup);
    REQUIRE(contained.has_value());
    CHECK(contained.value());

    auto found = reader.find(lookup);
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->path == expected_canonical_path);
  }
}

struct expected_tes3_layout_entry {
  std::string serialized_name;
  std::vector<std::byte> payload;
  std::uint64_t archive_hash{0};
  std::uint32_t raw_tes3_data_offset{0};
};

std::vector<expected_tes3_layout_entry> expected_hash_sorted_layout() {
  std::vector<expected_tes3_layout_entry> entries{
      {.serialized_name = "Meshes/Mixed/Probe.NIF", .payload = bytes_from_text("nif-data")},
      {.serialized_name = "textures/Memory/Probe.dds", .payload = bytes_from_text("dds-data")},
      {.serialized_name = "Readme.txt", .payload = {}},
  };
  for (auto& entry : entries) {
    entry.archive_hash = libbsa::detail::hash_tes3(entry.serialized_name);
  }
  std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
    return libbsa::detail::tes3_hash_sort_key(lhs.archive_hash) <
           libbsa::detail::tes3_hash_sort_key(rhs.archive_hash);
  });
  std::uint32_t raw_offset = 0;
  for (auto& entry : entries) {
    entry.raw_tes3_data_offset = raw_offset;
    raw_offset += static_cast<std::uint32_t>(entry.payload.size());
  }
  return entries;
}

std::uint32_t data_section_start_from_tes3_bytes(const std::vector<std::byte>& bytes) {
  const auto file_count = read_u32_le_at(bytes, 8U);
  const auto hash_table_start = 12U + read_u32_le_at(bytes, 4U);
  return hash_table_start + (file_count * 8U);
}

std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

} // namespace

TEST_CASE("tes3_bsa_writer committed fixture manifest records canonical writer evidence",
          "[unit][fixture][tes3_bsa_writer]") {
  const auto manifest = read_json_file(generated_archive_dir() / "tes3_writer_canonical_manifest.json");

  REQUIRE(manifest.at("manifest_kind").get<std::string>() == "tes3_writer_canonical");
}

TEST_CASE("tes3_bsa_writer emits byte-accurate raw TES3 tables in hash order", "[unit][tes3_bsa_writer]") {
  const auto archive = output_path("byte-accurate-layout.bsa");
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};

  REQUIRE(writer.add_bytes("Meshes/Mixed/Probe.NIF", bytes_from_text("nif-data")).has_value());
  REQUIRE(writer.add_bytes("textures\\Memory\\Probe.dds", bytes_from_text("dds-data")).has_value());
  REQUIRE(writer.add_bytes("Readme.txt", std::span<const std::byte>{}).has_value());

  REQUIRE(writer.write_to(archive.string()).has_value());
  const auto bytes = read_binary_file(archive);
  const auto expected_entries = expected_hash_sorted_layout();

  constexpr std::uint32_t tes3_magic_version = 0x00000100U;
  constexpr std::uint32_t fixed_header_size = 12U;
  constexpr std::uint32_t file_record_size = 8U;
  constexpr std::uint32_t name_offset_size = 4U;
  constexpr std::uint32_t hash_record_size = 8U;

  std::uint32_t name_table_size = 0;
  for (const auto& entry : expected_entries) {
    name_table_size += static_cast<std::uint32_t>(entry.serialized_name.size() + 1U);
  }
  const auto file_count = static_cast<std::uint32_t>(expected_entries.size());
  const std::uint32_t hash_table_start = fixed_header_size + (file_count * file_record_size) +
                                         (file_count * name_offset_size) + name_table_size;
  const std::uint32_t data_section_start = hash_table_start + (file_count * hash_record_size);

  CHECK(read_u32_le_at(bytes, 0U) == tes3_magic_version);
  CHECK(read_u32_le_at(bytes, 4U) == hash_table_start - 12U);
  CHECK(read_u32_le_at(bytes, 8U) == file_count);
  REQUIRE(bytes.size() == data_section_start + 16U);

  const std::size_t file_records_start = fixed_header_size;
  const std::size_t name_offsets_start = file_records_start + (file_count * file_record_size);
  const std::size_t name_table_start = name_offsets_start + (file_count * name_offset_size);
  std::uint32_t expected_name_offset = 0;
  for (std::size_t index = 0; index < expected_entries.size(); ++index) {
    const auto& entry = expected_entries[index];
    const auto file_record_offset = file_records_start + (index * file_record_size);
    CHECK(read_u32_le_at(bytes, file_record_offset) == entry.payload.size());
    CHECK(read_u32_le_at(bytes, file_record_offset + 4U) == entry.raw_tes3_data_offset);

    CHECK(read_u32_le_at(bytes, name_offsets_start + (index * name_offset_size)) == expected_name_offset);
    CHECK(read_null_terminated_name_at(bytes, name_table_start + expected_name_offset, hash_table_start) ==
          entry.serialized_name);
    expected_name_offset += static_cast<std::uint32_t>(entry.serialized_name.size() + 1U);

    CHECK(read_u64_le_at(bytes, hash_table_start + (index * hash_record_size)) == entry.archive_hash);
    const auto payload_start = data_section_start + entry.raw_tes3_data_offset;
    REQUIRE(payload_start + entry.payload.size() <= bytes.size());
    CHECK(std::vector<std::byte>{bytes.begin() + payload_start, bytes.begin() + payload_start + entry.payload.size()} ==
          entry.payload);
  }
}

TEST_CASE("tes3_bsa_writer output reopens through reader lookup and extraction APIs", "[unit][tes3_bsa_writer]") {
  const auto root = writer_test_dir() / "reader-backed-round-trip";
  std::filesystem::create_directories(root);
  const auto disk_source = root / "disk-probe.nif";
  const auto archive = output_path("reader-backed-round-trip.bsa");

  const auto disk_bytes = bytes_from_text("disk payload from host file");
  auto memory_bytes = bytes_from_text("copied memory payload");
  const auto copied_memory_bytes = memory_bytes;
  const std::vector<std::byte> zero_bytes;
  write_binary_file(disk_source, disk_bytes);

  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};
  REQUIRE(writer.add_file("Meshes/Disk/Probe.NIF", disk_source.string()).has_value());
  REQUIRE(writer.add_bytes("textures\\Memory\\Probe.dds", memory_bytes).has_value());
  REQUIRE(writer.add_bytes("Readme.txt", std::span<const std::byte>{}).has_value());
  memory_bytes.assign({std::byte{0x00}, std::byte{0x01}, std::byte{0x02}});

  REQUIRE(writer.write_to(archive.string()).has_value());
  const auto archive_bytes = read_binary_file(archive);
  const auto data_section_start = data_section_start_from_tes3_bytes(archive_bytes);

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  CHECK(metadata.value().type == libbsa::archive_type::bsa);
  CHECK(metadata.value().variant == libbsa::archive_variant::tes3);
  CHECK(metadata.value().version == 0x00000100U);
  CHECK(metadata.value().file_count == 3U);
  CHECK(metadata.value().default_compression == libbsa::entry_compression::none);

  auto entries = opened.value().entries();
  REQUIRE(entries.has_value());
  REQUIRE(entries.value().size() == 3U);
  for (const auto& entry : entries.value()) {
    CHECK(entry.compression == libbsa::entry_compression::none);
  }

  const std::array<std::string_view, 3U> disk_lookups{"Meshes/Disk/Probe.NIF", "meshes/disk/probe.nif",
                                                       "MESHES\\DISK\\PROBE.NIF"};
  require_contains_lookup_variants(opened.value(), "meshes/disk/probe.nif", disk_lookups);
  const std::array<std::string_view, 3U> memory_lookups{"textures/Memory/Probe.dds", "textures/memory/probe.dds",
                                                         "TEXTURES\\MEMORY\\PROBE.DDS"};
  require_contains_lookup_variants(opened.value(), "textures/memory/probe.dds", memory_lookups);
  const std::array<std::string_view, 2U> root_lookups{"Readme.txt", "readme.txt"};
  require_contains_lookup_variants(opened.value(), "readme.txt", root_lookups);

  auto missing = opened.value().find("valid/missing/path.txt");
  REQUIRE(missing.has_value());
  CHECK_FALSE(missing.value().has_value());
  auto missing_contains = opened.value().contains("valid/missing/path.txt");
  REQUIRE(missing_contains.has_value());
  CHECK_FALSE(missing_contains.value());
  auto invalid_find = opened.value().find("../invalid.txt");
  REQUIRE_FALSE(invalid_find.has_value());
  CHECK(invalid_find.error().code == libbsa::error_code::invalid_argument);
  auto invalid_contains = opened.value().contains("../invalid.txt");
  REQUIRE_FALSE(invalid_contains.has_value());
  CHECK(invalid_contains.error().code == libbsa::error_code::invalid_argument);

  require_extracts_bytes(opened.value(), "Meshes/Disk/Probe.NIF", disk_bytes);
  require_extracts_bytes(opened.value(), "textures/Memory/Probe.dds", copied_memory_bytes);
  require_extracts_bytes(opened.value(), "Readme.txt", zero_bytes);

  const auto disk_entry = require_finds_entry(opened.value(), "Meshes/Disk/Probe.NIF");
  const auto memory_entry = require_finds_entry(opened.value(), "textures/Memory/Probe.dds");
  const auto root_entry = require_finds_entry(opened.value(), "Readme.txt");
  CHECK(disk_entry.original_path == "Meshes/Disk/Probe.NIF");
  CHECK(memory_entry.original_path == "textures/Memory/Probe.dds");
  CHECK(root_entry.original_path == "Readme.txt");

  const auto file_count = read_u32_le_at(archive_bytes, 8U);
  const auto hash_table_start = 12U + read_u32_le_at(archive_bytes, 4U);
  const auto file_records_start = 12U;
  const auto name_offsets_start = file_records_start + (file_count * 8U);
  const auto name_table_start = name_offsets_start + (file_count * 4U);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto raw_record_offset = read_u32_le_at(archive_bytes, file_records_start + (index * 8U) + 4U);
    const auto name_offset = read_u32_le_at(archive_bytes, name_offsets_start + (index * 4U));
    const auto name = read_null_terminated_name_at(archive_bytes, name_table_start + name_offset, hash_table_start);
    const auto entry = require_finds_entry(opened.value(), name);
    CHECK(entry.payload_offset == static_cast<std::uint64_t>(data_section_start) + raw_record_offset);
  }
}

TEST_CASE("tes3_bsa_writer copies memory entries into writer-owned state", "[unit][tes3_bsa_writer]") {
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};
  auto bytes = sample_bytes();
  const auto copied = bytes;

  REQUIRE(writer.add_bytes("Meshes/Copy.NIF", bytes).has_value());
  bytes.assign({std::byte{0x00}, std::byte{0x01}});
  bytes.clear();

  const auto archive = output_path("copied-memory.bsa");
  REQUIRE(writer.write_to(archive.string()).has_value());
  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  require_extracted_bytes(opened.value(), "meshes/copy.nif", copied);
}

TEST_CASE("tes3_bsa_writer reports missing disk sources from write_to", "[unit][tes3_bsa_writer]") {
  libbsa::tes3_bsa_writer writer;
  const auto missing_source = output_path("missing-source-input.nif");
  std::error_code fs_error;
  std::filesystem::remove(missing_source, fs_error);

  auto added = writer.add_file("Meshes/Disk.NIF", missing_source.string());
  REQUIRE(added.has_value());

  auto written = writer.write_to(output_path("missing-source-output.bsa").string());
  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
}

TEST_CASE("tes3_bsa_writer rejects invalid archive paths", "[unit][tes3_bsa_writer]") {
  for (const std::string invalid_path : {"/rooted/file.txt", "C:/drive/file.txt", "folder/../file.txt", ""}) {
    libbsa::tes3_bsa_writer writer;

    auto added_memory = writer.add_bytes(invalid_path, sample_bytes());
    REQUIRE_FALSE(added_memory.has_value());
    REQUIRE(added_memory.error().code == libbsa::error_code::invalid_argument);

    auto added_file = writer.add_file(invalid_path, "source.bin");
    REQUIRE_FALSE(added_file.has_value());
    REQUIRE(added_file.error().code == libbsa::error_code::invalid_argument);
  }
}

TEST_CASE("tes3_bsa_writer rejects duplicate canonical archive paths at write time", "[unit][tes3_bsa_writer]") {
  libbsa::tes3_bsa_writer writer;
  REQUIRE(writer.add_bytes("Meshes/Duplicate.NIF", sample_bytes()).has_value());
  REQUIRE(writer.add_bytes("meshes/duplicate.nif", bytes_from_text("duplicate")).has_value());

  auto written = writer.write_to(output_path("duplicate-canonical-path.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("tes3_bsa_writer rejects empty archives", "[unit][tes3_bsa_writer]") {
  libbsa::tes3_bsa_writer writer;

  auto written = writer.write_to(output_path("empty-archive.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("tes3_bsa_writer refuses overwrite by default and preserves existing bytes", "[unit][tes3_bsa_writer]") {
  const auto archive = output_path("overwrite-disabled.bsa");
  const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
  write_binary_file(archive, sentinel);
  libbsa::tes3_bsa_writer writer;
  REQUIRE(writer.add_bytes("Meshes/Unique.NIF", sample_bytes()).has_value());

  auto written = writer.write_to(archive.string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
  CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE("tes3_bsa_writer replaces existing output only when overwrite is enabled", "[unit][tes3_bsa_writer]") {
  const auto archive = output_path("overwrite-enabled.bsa");
  const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
  write_binary_file(archive, sentinel);
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};
  REQUIRE(writer.add_bytes("Meshes/Replaced.NIF", sample_bytes()).has_value());

  auto written = writer.write_to(archive.string());

  REQUIRE(written.has_value());
  CHECK(read_binary_file(archive) != sentinel);
}

TEST_CASE("tes3_bsa_writer preserves caller-owned temp-name sibling files", "[unit][tes3_bsa_writer]") {
  const auto archive = output_path("safe-temp-collision.bsa");
  const auto collision = archive.string() + ".tmp";
  const std::vector<std::byte> sentinel{std::byte{0x54}, std::byte{0x4D}, std::byte{0x50}};
  write_binary_file(collision, sentinel);
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};
  REQUIRE(writer.add_bytes("Meshes/SafeTemp.NIF", sample_bytes()).has_value());

  auto written = writer.write_to(archive.string());

  REQUIRE(written.has_value());
  REQUIRE(std::filesystem::exists(collision));
  CHECK(read_binary_file(collision) == sentinel);
}
