#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_ba2_gnrl_writer_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path output_path(std::string name) { return writer_test_dir() / std::move(name); }

void write_binary_file(const std::filesystem::path& path, std::vector<std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.good());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
}

std::vector<std::byte> sample_bytes() {
  return {std::byte{0x42}, std::byte{0x41}, std::byte{0x32}, std::byte{0x21}};
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  REQUIRE(input.good());
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  REQUIRE_FALSE(input.bad());
  return bytes;
}

std::uint16_t read_u16_le(std::span<const std::byte> bytes, std::size_t& offset) {
  REQUIRE(offset + 2U <= bytes.size());
  const auto value = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
                                               (std::to_integer<std::uint8_t>(bytes[offset + 1U]) << 8U));
  offset += 2U;
  return value;
}

std::uint32_t read_u32_le(std::span<const std::byte> bytes, std::size_t& offset) {
  REQUIRE(offset + 4U <= bytes.size());
  std::uint32_t value = 0;
  for (std::size_t index = 0; index < 4U; ++index) {
    value |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + index])) << (index * 8U);
  }
  offset += 4U;
  return value;
}

std::uint64_t read_u64_le(std::span<const std::byte> bytes, std::size_t& offset) {
  REQUIRE(offset + 8U <= bytes.size());
  std::uint64_t value = 0;
  for (std::size_t index = 0; index < 8U; ++index) {
    value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[offset + index])) << (index * 8U);
  }
  offset += 8U;
  return value;
}

struct physical_record {
  std::uint64_t offset{};
  std::uint32_t packed_size{};
  std::uint32_t raw_size{};
};

struct physical_layout {
  std::uint32_t version{};
  std::uint64_t file_table_offset{};
  std::optional<std::uint32_t> starfield_unknown1;
  std::optional<std::uint32_t> starfield_unknown2;
  std::optional<std::uint32_t> compression_method;
  std::vector<physical_record> records;
  std::vector<std::string> names;
};

physical_layout read_physical_layout(const std::filesystem::path& archive_path) {
  const auto bytes = read_binary_file(archive_path);
  std::size_t offset = 0;
  REQUIRE(read_u32_le(bytes, offset) == 0x5844'5442U); // BTDX
  physical_layout layout;
  layout.version = read_u32_le(bytes, offset);
  REQUIRE(read_u32_le(bytes, offset) == 0x4C52'4E47U); // GNRL
  const auto file_count = read_u32_le(bytes, offset);
  layout.file_table_offset = read_u64_le(bytes, offset);
  if (layout.version >= 2U) {
    layout.starfield_unknown1 = read_u32_le(bytes, offset);
    layout.starfield_unknown2 = read_u32_le(bytes, offset);
  }
  if (layout.version >= 3U) {
    layout.compression_method = read_u32_le(bytes, offset);
  }

  layout.records.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    (void)read_u32_le(bytes, offset);
    offset += 4U;
    (void)read_u32_le(bytes, offset);
    (void)read_u32_le(bytes, offset);
    const auto payload_offset = read_u64_le(bytes, offset);
    const auto packed_size = read_u32_le(bytes, offset);
    const auto raw_size = read_u32_le(bytes, offset);
    REQUIRE(read_u32_le(bytes, offset) == 0xBAAD'F00DU);
    layout.records.push_back(physical_record{payload_offset, packed_size, raw_size});
  }

  REQUIRE(layout.file_table_offset <= bytes.size());
  offset = static_cast<std::size_t>(layout.file_table_offset);
  layout.names.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto length = read_u16_le(bytes, offset);
    REQUIRE(offset + length <= bytes.size());
    std::string name;
    name.reserve(length);
    for (std::uint16_t byte_index = 0; byte_index < length; ++byte_index) {
      name.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes[offset + byte_index])));
    }
    offset += length;
    layout.names.push_back(std::move(name));
  }
  return layout;
}

struct expected_entry {
  std::string path;
  std::vector<std::byte> bytes;
  std::uint32_t record_flags{};
};

libbsa::ba2_gnrl_writer_options overwriting_raw_options() {
  libbsa::ba2_gnrl_writer_options options;
  options.overwrite_existing = true;
  options.compression = libbsa::archive_compression_policy::all_raw;
  return options;
}

void require_raw_round_trip(const std::filesystem::path& output,
                            const std::vector<expected_entry>& expected,
                            std::uint32_t expected_version,
                            libbsa::archive_variant expected_variant,
                            std::optional<std::uint32_t> expected_unknown1,
                            std::optional<std::uint32_t> expected_unknown2,
                            std::optional<std::uint32_t> expected_compression_method) {
  auto opened = libbsa::archive_reader::open(output.string());
  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  CHECK(metadata.value().type == libbsa::archive_type::ba2);
  CHECK(metadata.value().variant == expected_variant);
  CHECK(metadata.value().version == expected_version);
  CHECK(metadata.value().file_count == expected.size());
  REQUIRE(metadata.value().ba2.has_value());
  CHECK(metadata.value().ba2->starfield_unknown1 == expected_unknown1);
  CHECK(metadata.value().ba2->starfield_unknown2 == expected_unknown2);
  CHECK(metadata.value().ba2->compression_method == expected_compression_method);

  const auto layout = read_physical_layout(output);
  CHECK(layout.version == expected_version);
  CHECK(layout.starfield_unknown1 == expected_unknown1);
  CHECK(layout.starfield_unknown2 == expected_unknown2);
  CHECK(layout.compression_method == expected_compression_method);
  REQUIRE(layout.records.size() == expected.size());
  REQUIRE(layout.names.size() == expected.size());
  for (const auto& record : layout.records) {
    CHECK(record.packed_size == 0U);
    CHECK(layout.file_table_offset >= record.offset + record.raw_size);
  }
  CHECK(std::all_of(layout.names.begin(), layout.names.end(), [](const std::string& name) {
    return name.find('\\') == std::string::npos;
  }));

  auto entries = opened.value().entries();
  REQUIRE(entries.has_value());
  REQUIRE(entries.value().size() == expected.size());
  for (const auto& expected_entry : expected) {
    auto contains = opened.value().contains(expected_entry.path);
    REQUIRE(contains.has_value());
    CHECK(contains.value());

    auto found = opened.value().find(expected_entry.path);
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->original_path == expected_entry.path);
    CHECK(found.value()->raw_size == expected_entry.bytes.size());
    CHECK(found.value()->stored_size == expected_entry.bytes.size());
    CHECK(found.value()->compression == libbsa::entry_compression::none);
    CHECK(found.value()->record_flags == expected_entry.record_flags);
    CHECK(found.value()->archive_hash != 0U);
    CHECK(found.value()->payload_offset < layout.file_table_offset);

    auto extracted = opened.value().extract_bytes(expected_entry.path);
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == expected_entry.bytes);
  }
}

struct compression_case {
  libbsa::ba2_gnrl_target target;
  std::uint32_t version;
  std::string file_name;
  std::optional<std::uint32_t> compression_method;
  libbsa::entry_compression expected_compression;
};

void require_compressed_round_trip(const std::filesystem::path& output,
                                  const std::vector<expected_entry>& expected,
                                  std::uint32_t expected_version,
                                  libbsa::archive_variant expected_variant,
                                  std::optional<std::uint32_t> expected_compression_method,
                                  libbsa::entry_compression expected_compression) {
  auto opened = libbsa::archive_reader::open(output.string());
  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  CHECK(metadata.value().type == libbsa::archive_type::ba2);
  CHECK(metadata.value().variant == expected_variant);
  CHECK(metadata.value().version == expected_version);
  CHECK(metadata.value().file_count == expected.size());
  CHECK(metadata.value().default_compression == expected_compression);
  REQUIRE(metadata.value().ba2.has_value());
  CHECK(metadata.value().ba2->compression_method == expected_compression_method);

  const auto layout = read_physical_layout(output);
  CHECK(layout.version == expected_version);
  CHECK(layout.compression_method == expected_compression_method);
  REQUIRE(layout.records.size() == expected.size());

  for (const auto& expected_entry : expected) {
    auto found = opened.value().find(expected_entry.path);
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->compression == expected_compression);
    CHECK(found.value()->raw_size == expected_entry.bytes.size());
    CHECK(found.value()->stored_size > 0U);
    CHECK(found.value()->stored_size != expected_entry.bytes.size());
    CHECK(found.value()->payload_offset < layout.file_table_offset);

    auto extracted = opened.value().extract_bytes(expected_entry.path);
    REQUIRE(extracted.has_value());
    CHECK(extracted.value() == expected_entry.bytes);
  }

  for (const auto& record : layout.records) {
    CHECK(record.packed_size > 0U);
    CHECK(record.raw_size > 0U);
    CHECK(layout.file_table_offset >= record.offset + record.packed_size);
  }
}

} // namespace

TEST_CASE("BA2 GNRL writer rejects empty disk source host paths", "[unit][ba2_gnrl_writer]") {
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

  auto added = writer.add_file("Meshes/EmptySource.nif", "");

  REQUIRE_FALSE(added.has_value());
  REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("BA2 GNRL writer reports invalid archive paths as invalid arguments", "[unit][ba2_gnrl_writer]") {
  for (const std::string invalid_path : {"/rooted/file.txt", "C:/drive/file.txt", "folder/../file.txt", ""}) {
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

    auto added = writer.add_bytes(invalid_path, sample_bytes());

    REQUIRE_FALSE(added.has_value());
    REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
  }
}

TEST_CASE("BA2 GNRL writer rejects duplicate canonical archive paths at write time", "[unit][ba2_gnrl_writer]") {
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

  REQUIRE(writer.add_bytes("Meshes/Foo.nif", sample_bytes()).has_value());
  REQUIRE(writer.add_bytes("meshes/foo.nif", std::vector<std::byte>{std::byte{0x24}}).has_value());

  auto written = writer.write_to(output_path("duplicate-canonical-path.ba2").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("BA2 GNRL writer refuses to overwrite existing output when overwrite_existing is false",
          "[unit][ba2_gnrl_writer]") {
  const auto existing = output_path("overwrite-default.ba2");
  write_binary_file(existing, {std::byte{0x01}});

  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
  REQUIRE(writer.add_bytes("Meshes/Unique.nif", sample_bytes()).has_value());

  auto written = writer.write_to(existing.string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
}

TEST_CASE("BA2 GNRL writer reports missing disk sources as I/O errors", "[unit][ba2_gnrl_writer]") {
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};
  const auto missing_source = output_path("missing-source-input.nif");
  std::filesystem::remove(missing_source);

  auto added = writer.add_file("Meshes/Missing.nif", missing_source.string());
  REQUIRE(added.has_value());

  auto written = writer.write_to(output_path("missing-source-output.ba2").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
}

TEST_CASE("BA2 GNRL writer accepts explicit Fallout 4 disk archive paths", "[unit][ba2_gnrl_writer]") {
  const auto source = output_path("disk-source.nif");
  write_binary_file(source, sample_bytes());

  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4};

  auto added = writer.add_file("Meshes/Disk.nif", source.string());

  REQUIRE(added.has_value());
}

TEST_CASE("BA2 GNRL writer raw Fallout 4 output reopens with end filename table", "[unit][ba2_gnrl_writer]") {
  const auto source = output_path("raw-fo4-disk-source.psc");
  const std::vector<std::byte> disk_bytes{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}};
  write_binary_file(source, disk_bytes);
  std::vector<std::byte> mutable_memory{std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
  const auto expected_original = mutable_memory;
  const std::vector<std::byte> empty_bytes;

  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, overwriting_raw_options()};
  REQUIRE(writer.add_bytes("Meshes/MixedCase/Alpha.nif", mutable_memory).has_value());
  REQUIRE(writer.add_file("Scripts/Quest.psc", source.string()).has_value());
  REQUIRE(writer.add_bytes("Zero/Empty.txt", empty_bytes).has_value());
  mutable_memory.assign(mutable_memory.size(), std::byte{0x00});

  const auto output = output_path("raw-fo4-round-trip.ba2");
  auto written = writer.write_to(output.string());
  REQUIRE(written.has_value());

  require_raw_round_trip(output,
                         {{"Meshes/MixedCase/Alpha.nif", expected_original, 0U},
                          {"Scripts/Quest.psc", disk_bytes, 0U},
                          {"Zero/Empty.txt", empty_bytes, 0U}},
                         1U,
                         libbsa::archive_variant::fallout4,
                         std::nullopt,
                         std::nullopt,
                         std::nullopt);
}

TEST_CASE("BA2 GNRL writer raw Starfield v2 and v3 defaults reopen through public metadata",
          "[unit][ba2_gnrl_writer]") {
  for (const auto [target, version, file_name, compression_method] :
       {std::tuple{libbsa::ba2_gnrl_target::starfield_v2, 2U, "raw-sfv2-default.ba2", std::optional<std::uint32_t>{}},
        std::tuple{libbsa::ba2_gnrl_target::starfield_v3,
                   3U,
                   "raw-sfv3-default.ba2",
                   std::optional<std::uint32_t>{3U}}}) {
    libbsa::ba2_gnrl_writer writer{target, overwriting_raw_options()};
    const std::vector<std::byte> bytes{std::byte{0x53}, std::byte{0x46}, static_cast<std::byte>(version)};
    REQUIRE(writer.add_bytes("Meshes/MixedCase/Alpha.nif", bytes).has_value());

    const auto output = output_path(file_name);
    auto written = writer.write_to(output.string());
    REQUIRE(written.has_value());

    require_raw_round_trip(output,
                           {{"Meshes/MixedCase/Alpha.nif", bytes, 0U}},
                           version,
                           libbsa::archive_variant::starfield,
                           1U,
                           0U,
                           compression_method);
  }
}

TEST_CASE("BA2 GNRL writer preserves Starfield unknown overrides and BA2 record flags",
          "[unit][ba2_gnrl_writer]") {
  for (const auto [target, version, file_name, compression_method] :
       {std::tuple{libbsa::ba2_gnrl_target::starfield_v2, 2U, "raw-sfv2-overrides.ba2", std::optional<std::uint32_t>{}},
        std::tuple{libbsa::ba2_gnrl_target::starfield_v3,
                   3U,
                   "raw-sfv3-overrides.ba2",
                   std::optional<std::uint32_t>{3U}}}) {
    auto options = overwriting_raw_options();
    options.starfield_unknown1 = 7U;
    options.starfield_unknown2 = 9U;
    libbsa::ba2_gnrl_writer writer{target, options};

    libbsa::ba2_gnrl_entry_options entry_options;
    entry_options.compression = libbsa::entry_compression_policy::raw;
    entry_options.record_flags = 0x40U;
    const std::vector<std::byte> bytes{std::byte{0x01}, std::byte{0x02}};
    REQUIRE(writer.add_bytes("Scripts/Quest.psc", bytes, entry_options).has_value());

    const auto output = output_path(file_name);
    auto written = writer.write_to(output.string());
    REQUIRE(written.has_value());

    require_raw_round_trip(output,
                           {{"Scripts/Quest.psc", bytes, 0x40U}},
                           version,
                           libbsa::archive_variant::starfield,
                           7U,
                           9U,
                           compression_method);
  }
}

TEST_CASE("BA2 GNRL writer all-compressed policy routes through target compression methods",
          "[unit][ba2_gnrl_writer]") {
  const std::vector<compression_case> cases{
      {libbsa::ba2_gnrl_target::fallout4, 1U, "compressed-fo4-deflate.ba2", std::nullopt,
       libbsa::entry_compression::deflate},
      {libbsa::ba2_gnrl_target::starfield_v2, 2U, "compressed-sfv2-deflate.ba2", std::nullopt,
       libbsa::entry_compression::deflate},
      {libbsa::ba2_gnrl_target::starfield_v3, 3U, "compressed-sfv3-method0-deflate.ba2", 0U,
       libbsa::entry_compression::deflate},
      {libbsa::ba2_gnrl_target::starfield_v3, 3U, "compressed-sfv3-method3-lz4-block.ba2", 3U,
       libbsa::entry_compression::lz4_block},
  };

  for (const auto& test_case : cases) {
    libbsa::ba2_gnrl_writer_options options;
    options.overwrite_existing = true;
    options.compression = libbsa::archive_compression_policy::all_compressed;
    if (test_case.compression_method.has_value()) {
      options.starfield_compression_method = *test_case.compression_method;
    }
    if (test_case.file_name == "compressed-sfv3-method0-deflate.ba2") {
      options.starfield_compression_method = 0U;
    }
    libbsa::ba2_gnrl_writer writer{test_case.target, options};
    const std::vector<std::byte> bytes{std::byte{0x4E}, std::byte{0x4F}, std::byte{0x54}, std::byte{0x44},
                                       std::byte{0x44}, std::byte{0x53}, std::byte{0x21}, std::byte{0x21},
                                       std::byte{0x21}, std::byte{0x21}, std::byte{0x21}, std::byte{0x21}};
    REQUIRE(writer.add_bytes("NoExtensionInference/Generic.bin", bytes).has_value());

    const auto output = output_path(test_case.file_name);
    auto written = writer.write_to(output.string());
    REQUIRE(written.has_value());

    require_compressed_round_trip(output,
                                  {{"NoExtensionInference/Generic.bin", bytes, 0U}},
                                  test_case.version,
                                  test_case.target == libbsa::ba2_gnrl_target::fallout4
                                      ? libbsa::archive_variant::fallout4
                                      : libbsa::archive_variant::starfield,
                                  test_case.compression_method,
                                  test_case.expected_compression);
  }
}

TEST_CASE("BA2 GNRL writer per-entry raw and compressed overrides affect only raw versus packed state",
          "[unit][ba2_gnrl_writer]") {
  libbsa::ba2_gnrl_writer_options options;
  options.overwrite_existing = true;
  options.compression = libbsa::archive_compression_policy::all_compressed;
  options.starfield_compression_method = 3U;
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::starfield_v3, options};

  const std::vector<std::byte> compressed_bytes{std::byte{0x43}, std::byte{0x43}, std::byte{0x43}, std::byte{0x43},
                                                std::byte{0x43}, std::byte{0x43}, std::byte{0x43}, std::byte{0x43}};
  const std::vector<std::byte> raw_bytes{std::byte{0x52}, std::byte{0x41}, std::byte{0x57}};
  const std::vector<std::byte> empty_bytes;
  REQUIRE(writer.add_bytes("Generic/Compressed.bin", compressed_bytes).has_value());
  REQUIRE(writer.add_bytes("Generic/RawOverride.bin", raw_bytes, libbsa::entry_compression_policy::raw).has_value());
  REQUIRE(writer.add_bytes("Generic/Empty.bin", empty_bytes).has_value());

  const auto output = output_path("compressed-sfv3-overrides.ba2");
  auto written = writer.write_to(output.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(output.string());
  REQUIRE(opened.has_value());
  auto compressed = opened.value().find("Generic/Compressed.bin");
  auto raw = opened.value().find("Generic/RawOverride.bin");
  auto empty = opened.value().find("Generic/Empty.bin");
  REQUIRE(compressed.has_value());
  REQUIRE(compressed.value().has_value());
  REQUIRE(raw.has_value());
  REQUIRE(raw.value().has_value());
  REQUIRE(empty.has_value());
  REQUIRE(empty.value().has_value());

  CHECK(compressed.value()->compression == libbsa::entry_compression::lz4_block);
  CHECK(compressed.value()->stored_size > 0U);
  CHECK(raw.value()->compression == libbsa::entry_compression::none);
  CHECK(raw.value()->stored_size == raw.value()->raw_size);
  CHECK(empty.value()->compression == libbsa::entry_compression::none);
  CHECK(empty.value()->stored_size == 0U);
  CHECK(empty.value()->raw_size == 0U);

  auto compressed_extracted = opened.value().extract_bytes("Generic/Compressed.bin");
  auto raw_extracted = opened.value().extract_bytes("Generic/RawOverride.bin");
  auto empty_extracted = opened.value().extract_bytes("Generic/Empty.bin");
  REQUIRE(compressed_extracted.has_value());
  REQUIRE(raw_extracted.has_value());
  REQUIRE(empty_extracted.has_value());
  CHECK(compressed_extracted.value() == compressed_bytes);
  CHECK(raw_extracted.value() == raw_bytes);
  CHECK(empty_extracted.value() == empty_bytes);
}
