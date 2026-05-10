#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <detail/bethesda_hash.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_tes4_bsa_writer_tests";
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

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  REQUIRE(input.good());

  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
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
  for (std::size_t index = 0; index < 8U; ++index) {
    value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + index])) << (index * 8U);
  }
  return value;
}

std::vector<std::uint64_t> sorted_hashes(std::vector<std::string_view> names) {
  std::vector<std::uint64_t> hashes;
  hashes.reserve(names.size());
  for (const auto name : names) {
    hashes.push_back(libbsa::detail::hash_tes4(name));
  }
  std::sort(hashes.begin(), hashes.end());
  return hashes;
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

std::vector<std::byte> sample_bytes() {
  return {std::byte{0x42}, std::byte{0x53}, std::byte{0x41}, std::byte{0x21}};
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (const char ch : text) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

std::uint32_t expected_version(libbsa::tes4_bsa_target target) {
  switch (target) {
  case libbsa::tes4_bsa_target::oblivion:
    return 103;
  case libbsa::tes4_bsa_target::fallout3:
    return 104;
  case libbsa::tes4_bsa_target::skyrim_se:
    return 105;
  }
  return 0;
}

libbsa::entry_compression expected_target_default_compression(libbsa::tes4_bsa_target target) {
  switch (target) {
  case libbsa::tes4_bsa_target::oblivion:
  case libbsa::tes4_bsa_target::fallout3:
    return libbsa::entry_compression::deflate;
  case libbsa::tes4_bsa_target::skyrim_se:
    return libbsa::entry_compression::lz4_frame;
  }
  return libbsa::entry_compression::none;
}

libbsa::entry_compression compressed_entry_method(libbsa::tes4_bsa_target target) {
  switch (target) {
  case libbsa::tes4_bsa_target::oblivion:
  case libbsa::tes4_bsa_target::fallout3:
    return libbsa::entry_compression::deflate;
  case libbsa::tes4_bsa_target::skyrim_se:
    return libbsa::entry_compression::lz4_frame;
  }
  return libbsa::entry_compression::none;
}

std::string target_name(libbsa::tes4_bsa_target target) {
  switch (target) {
  case libbsa::tes4_bsa_target::oblivion:
    return "oblivion";
  case libbsa::tes4_bsa_target::fallout3:
    return "fallout3";
  case libbsa::tes4_bsa_target::skyrim_se:
    return "skyrim-se";
  }
  return "unknown";
}

libbsa::entry_metadata require_entry(const libbsa::archive_reader& reader, std::string_view path) {
  auto found = reader.find(path);
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  return *found.value();
}

void require_extracted_bytes(const libbsa::archive_reader& reader,
                             std::string_view path,
                             const std::vector<std::byte>& expected) {
  auto extracted = reader.extract_bytes(path);
  REQUIRE(extracted.has_value());
  CHECK(extracted.value() == expected);
}

} // namespace

TEST_CASE("TES4 BSA writer raw output reopens for every target profile", "[unit][tes4_bsa_writer]") {
  constexpr std::uint32_t include_directory_names = 0x0001U;
  constexpr std::uint32_t include_file_names = 0x0002U;
  constexpr std::uint32_t archive_compress_by_default = 0x0004U;
  constexpr std::uint32_t archive_embed_names = 0x0100U;
  const std::array targets{libbsa::tes4_bsa_target::oblivion, libbsa::tes4_bsa_target::fallout3,
                           libbsa::tes4_bsa_target::skyrim_se};

  for (const auto target : targets) {
    const auto root = writer_test_dir() / target_name(target);
    std::filesystem::create_directories(root / "sources");
    const auto output = root / "raw-round-trip.bsa";
    std::filesystem::remove(output);

    const std::vector<std::pair<std::string, std::vector<std::byte>>> expected_entries{
        {"Meshes/Upper/Model.NIF", bytes_from_text("NIF model payload")},
        {"textures/lower/diffuse.dds", bytes_from_text("DDS texture payload")},
        {"Scripts/Quest/Stage.PEX", bytes_from_text("PEX script payload")},
        {"Interface/Menu.XML", bytes_from_text("XML menu payload")},
        {"Empty/Zero.txt", {}}};

    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    libbsa::tes4_bsa_writer writer{target, options};

    const auto model_source = root / "sources" / "model.nif";
    const auto diffuse_source = root / "sources" / "diffuse.dds";
    write_binary_file(model_source, expected_entries[0].second);
    write_binary_file(diffuse_source, expected_entries[1].second);

    REQUIRE(writer.add_file(expected_entries[0].first, model_source.string()).has_value());
    REQUIRE(writer.add_file(expected_entries[1].first, diffuse_source.string()).has_value());
    REQUIRE(writer.add_bytes(expected_entries[2].first, expected_entries[2].second).has_value());
    REQUIRE(writer.add_bytes(expected_entries[3].first, expected_entries[3].second).has_value());
    REQUIRE(writer.add_bytes(expected_entries[4].first, expected_entries[4].second).has_value());

    auto copied = bytes_from_text("copied before caller mutation");
    const auto original_copy = copied;
    REQUIRE(writer.add_bytes("Meshes/Copy.nif", copied).has_value());
    copied.assign({std::byte{0x00}, std::byte{0x01}});
    copied.clear();

    auto written = writer.write_to(output.string());
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());

    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::bsa);
    CHECK(metadata.value().variant == libbsa::archive_variant::tes4);
    CHECK(metadata.value().version == expected_version(target));
    CHECK(metadata.value().file_count == expected_entries.size() + 1U);
    CHECK(metadata.value().default_compression == expected_target_default_compression(target));
    CHECK((metadata.value().archive_flags & include_directory_names) != 0U);
    CHECK((metadata.value().archive_flags & include_file_names) != 0U);
    CHECK((metadata.value().archive_flags & archive_compress_by_default) == 0U);
    CHECK((metadata.value().archive_flags & archive_embed_names) == 0U);

    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == expected_entries.size() + 1U);

    for (const auto& [path, expected] : expected_entries) {
      auto contained = opened.value().contains(path);
      REQUIRE(contained.has_value());
      CHECK(contained.value());

      auto found = opened.value().find(path);
      REQUIRE(found.has_value());
      REQUIRE(found.value().has_value());
      CHECK(found.value()->original_path == path);
      CHECK(found.value()->compression == libbsa::entry_compression::none);

      auto upper_lookup = opened.value().find("MESHES/UPPER/MODEL.NIF");
      if (path == "Meshes/Upper/Model.NIF") {
        REQUIRE(upper_lookup.has_value());
        REQUIRE(upper_lookup.value().has_value());
      }

      auto extracted = opened.value().extract_bytes(path);
      REQUIRE(extracted.has_value());
      CHECK(extracted.value() == expected);

      collecting_sink sink;
      auto streamed = opened.value().extract(path, sink);
      REQUIRE(streamed.has_value());
      CHECK(sink.bytes() == expected);
    }

    auto copied_extracted = opened.value().extract_bytes("Meshes/Copy.nif");
    REQUIRE(copied_extracted.has_value());
    CHECK(copied_extracted.value() == original_copy);
  }
}

TEST_CASE("TES4 BSA writer serializes derived file flags and hash-sorted tables", "[unit][tes4_bsa_writer]") {
  constexpr std::uint32_t expected_v103_file_flags = 0x0001U | 0x0002U | 0x0008U | 0x0010U | 0x0100U;
  constexpr std::size_t header_size = 36U;
  constexpr std::size_t legacy_folder_record_size = 16U;

  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::oblivion, options};

  REQUIRE(writer.add_bytes("Scripts/Beta/Quest.pex", bytes_from_text("script")).has_value());
  REQUIRE(writer.add_bytes("Meshes/Alpha/Model.nif", bytes_from_text("model")).has_value());
  REQUIRE(writer.add_bytes("Docs/Misc/Readme.txt", bytes_from_text("misc")).has_value());
  REQUIRE(writer.add_bytes("Textures/Zeta/Diffuse.dds", bytes_from_text("texture")).has_value());
  REQUIRE(writer.add_bytes("Meshes/Alpha/Anim.kf", bytes_from_text("animation")).has_value());
  REQUIRE(writer.add_bytes("Interface/Gamma/Menu.xml", bytes_from_text("menu")).has_value());

  const auto archive = output_path("byte-level-flags-and-sort-order.bsa");
  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());

  const auto bytes = read_binary_file(archive);
  REQUIRE(bytes.size() >= header_size);
  CHECK(read_u32_le_at(bytes, 32U) == expected_v103_file_flags);

  const auto folder_count = read_u32_le_at(bytes, 16U);
  REQUIRE(folder_count == 5U);

  std::vector<std::uint64_t> actual_folder_hashes;
  actual_folder_hashes.reserve(folder_count);
  std::size_t cursor = header_size;
  for (std::uint32_t index = 0; index < folder_count; ++index) {
    actual_folder_hashes.push_back(read_u64_le_at(bytes, cursor));
    cursor += legacy_folder_record_size;
  }
  CHECK(actual_folder_hashes ==
        sorted_hashes({"Docs\\Misc", "Interface\\Gamma", "Meshes\\Alpha", "Scripts\\Beta", "Textures\\Zeta"}));

  for (std::uint32_t folder_index = 0; folder_index < folder_count; ++folder_index) {
    REQUIRE(cursor < bytes.size());
    const auto encoded_name_size = std::to_integer<unsigned char>(bytes[cursor]);
    REQUIRE(encoded_name_size > 0U);
    ++cursor;
    REQUIRE(cursor + encoded_name_size <= bytes.size());
    std::string folder_name;
    folder_name.reserve(encoded_name_size - 1U);
    for (std::size_t index = 0; index + 1U < encoded_name_size; ++index) {
      folder_name.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes[cursor + index])));
    }
    cursor += encoded_name_size;

    const auto file_count_for_folder = read_u32_le_at(bytes, header_size + folder_index * legacy_folder_record_size + 8U);
    std::vector<std::uint64_t> actual_file_hashes;
    actual_file_hashes.reserve(file_count_for_folder);
    for (std::uint32_t file_index = 0; file_index < file_count_for_folder; ++file_index) {
      actual_file_hashes.push_back(read_u64_le_at(bytes, cursor));
      cursor += 16U;
    }

    CHECK(folder_name.find('/') == std::string::npos);

    if (folder_name == "Meshes\\Alpha") {
      CHECK(actual_file_hashes == sorted_hashes({"Model.nif", "Anim.kf"}));
    } else {
      CHECK(std::is_sorted(actual_file_hashes.begin(), actual_file_hashes.end()));
    }
  }
}

TEST_CASE("TES4 BSA writer copies memory entries into writer-owned state", "[unit][tes4_bsa_writer]") {
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::oblivion};
  auto bytes = sample_bytes();

  auto added = writer.add_bytes("Meshes/Copy.nif", bytes);
  REQUIRE(added.has_value());

  bytes.clear();
  bytes.shrink_to_fit();

  auto duplicate = writer.add_bytes("meshes/copy.nif", std::vector<std::byte>{std::byte{0x00}});
  REQUIRE(duplicate.has_value());

  auto written = writer.write_to(output_path("copied-memory-duplicate.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("TES4 BSA writer rejects duplicate canonical archive paths at write time", "[unit][tes4_bsa_writer]") {
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3};

  REQUIRE(writer.add_bytes("Meshes/Foo.nif", sample_bytes()).has_value());
  REQUIRE(writer.add_bytes("meshes/foo.nif", std::vector<std::byte>{std::byte{0x24}}).has_value());

  auto written = writer.write_to(output_path("duplicate-canonical-path.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("TES4 BSA writer reports invalid archive paths as invalid arguments", "[unit][tes4_bsa_writer]") {
  for (const std::string invalid_path : {"/rooted/file.txt", "C:/drive/file.txt", "folder/../file.txt", ""}) {
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::oblivion};

    auto added = writer.add_bytes(invalid_path, sample_bytes());

    REQUIRE_FALSE(added.has_value());
    REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
  }
}

TEST_CASE("TES4 BSA writer refuses to overwrite existing output when overwrite_existing is false",
          "[unit][tes4_bsa_writer]") {
  auto existing = output_path("overwrite-default.bsa");
  write_binary_file(existing, {std::byte{0x01}});

  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::oblivion};
  REQUIRE(writer.add_bytes("Meshes/Unique.nif", sample_bytes()).has_value());

  auto written = writer.write_to(existing.string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
}

TEST_CASE("TES4 BSA writer reports missing disk sources as I/O errors", "[unit][tes4_bsa_writer]") {
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::skyrim_se};
  const auto missing_source = output_path("missing-source-input.dds");
  std::filesystem::remove(missing_source);

  auto added = writer.add_file("Textures/Disk.dds", missing_source.string());
  REQUIRE(added.has_value());

  auto written = writer.write_to(output_path("missing-source-output.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
}

TEST_CASE("TES4 BSA writer requires explicit archive paths for disk entries", "[unit][tes4_bsa_writer]") {
  const auto source = output_path("disk-source.dds");
  write_binary_file(source, sample_bytes());

  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::skyrim_se};

  auto added = writer.add_file("Textures/Disk.dds", source.string());

  REQUIRE(added.has_value());
}

TEST_CASE("TES4 BSA writer compresses inherited entries and preserves raw overrides under all-compressed policy",
          "[unit][tes4_bsa_writer]") {
  const std::array targets{libbsa::tes4_bsa_target::oblivion, libbsa::tes4_bsa_target::fallout3,
                           libbsa::tes4_bsa_target::skyrim_se};

  const auto inherited_bytes = bytes_from_text("compress me according to the target profile");
  const auto raw_override_bytes = bytes_from_text("keep this entry raw even though the archive default is compressed");
  const std::vector<std::byte> zero_byte_payload;

  for (const auto target : targets) {
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_compressed;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{target, options};

    REQUIRE(writer.add_bytes("Meshes/Compressed.nif", inherited_bytes).has_value());
    REQUIRE(writer.add_bytes("Meshes/RawOverride.nif", raw_override_bytes,
                             libbsa::entry_compression_policy::raw)
                .has_value());
    REQUIRE(writer.add_bytes("Meshes/ZeroByte.nif", zero_byte_payload).has_value());

    const auto archive = output_path(target_name(target) + "-all-compressed-overrides.bsa");
    auto written = writer.write_to(archive.string());
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());

    const auto& compressed = require_entry(opened.value(), "Meshes/Compressed.nif");
    CHECK(compressed.compression == compressed_entry_method(target));
    require_extracted_bytes(opened.value(), "Meshes/Compressed.nif", inherited_bytes);

    const auto& raw_override = require_entry(opened.value(), "Meshes/RawOverride.nif");
    CHECK(raw_override.compression == libbsa::entry_compression::none);
    require_extracted_bytes(opened.value(), "Meshes/RawOverride.nif", raw_override_bytes);

    const auto& zero_byte = require_entry(opened.value(), "Meshes/ZeroByte.nif");
    CHECK(zero_byte.compression == libbsa::entry_compression::none);
    require_extracted_bytes(opened.value(), "Meshes/ZeroByte.nif", zero_byte_payload);
  }
}

TEST_CASE("TES4 BSA writer records compressed override metadata while archive default remains raw",
          "[unit][tes4_bsa_writer]") {
  constexpr std::uint32_t archive_compress_by_default = 0x0004U;
  const std::array targets{libbsa::tes4_bsa_target::oblivion, libbsa::tes4_bsa_target::fallout3,
                           libbsa::tes4_bsa_target::skyrim_se};

  const auto raw_bytes = bytes_from_text("raw archive default entry");
  const auto compressed_override_bytes = bytes_from_text("compressed per-entry override from all-raw archive");

  for (const auto target : targets) {
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{target, options};

    REQUIRE(writer.add_bytes("Textures/RawDefault.dds", raw_bytes).has_value());
    REQUIRE(writer.add_bytes("Textures/CompressedOverride.dds", compressed_override_bytes,
                             libbsa::entry_compression_policy::compressed)
                .has_value());

    const auto archive = output_path(target_name(target) + "-all-raw-compressed-override.bsa");
    auto written = writer.write_to(archive.string());
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());

    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK((metadata.value().archive_flags & archive_compress_by_default) == 0U);

    const auto& raw_default = require_entry(opened.value(), "Textures/RawDefault.dds");
    CHECK(raw_default.compression == libbsa::entry_compression::none);
    require_extracted_bytes(opened.value(), "Textures/RawDefault.dds", raw_bytes);

    const auto& compressed_override = require_entry(opened.value(), "Textures/CompressedOverride.dds");
    CHECK(compressed_override.compression == compressed_entry_method(target));
    require_extracted_bytes(opened.value(), "Textures/CompressedOverride.dds", compressed_override_bytes);
  }
}

TEST_CASE("TES4 BSA writer target-default compression round-trips inherited entries", "[unit][tes4_bsa_writer]") {
  constexpr std::uint32_t archive_compress_by_default = 0x0004U;
  const std::array targets{libbsa::tes4_bsa_target::oblivion, libbsa::tes4_bsa_target::fallout3,
                           libbsa::tes4_bsa_target::skyrim_se};
  const auto inherited_bytes = bytes_from_text("target default inherited bytes must survive extraction");

  for (const auto target : targets) {
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::target_default;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{target, options};

    REQUIRE(writer.add_bytes("Meshes/TargetDefault.nif", inherited_bytes).has_value());

    const auto archive = output_path(target_name(target) + "-target-default-compression.bsa");
    auto written = writer.write_to(archive.string());
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());

    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    const auto& entry = require_entry(opened.value(), "Meshes/TargetDefault.nif");

    if (target == libbsa::tes4_bsa_target::oblivion) {
      CHECK((metadata.value().archive_flags & archive_compress_by_default) == 0U);
      CHECK(entry.compression == libbsa::entry_compression::none);
    } else {
      CHECK((metadata.value().archive_flags & archive_compress_by_default) != 0U);
      CHECK(entry.compression == compressed_entry_method(target));
    }
    require_extracted_bytes(opened.value(), "Meshes/TargetDefault.nif", inherited_bytes);
  }
}

TEST_CASE("TES4 BSA writer overwrites existing archives when overwrite_existing is true", "[unit][tes4_bsa_writer]") {
  const auto archive = output_path("overwrite-explicit-success.bsa");
  write_binary_file(archive, bytes_from_text("pre-existing non-archive bytes"));

  const auto source_bytes = bytes_from_text("replacement archive payload");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};

  REQUIRE(writer.add_bytes("Meshes/Overwrite/Replacement.nif", source_bytes).has_value());

  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  require_extracted_bytes(opened.value(), "Meshes/Overwrite/Replacement.nif", source_bytes);
}

TEST_CASE("TES4 BSA writer leaves embedded names absent by default and when explicitly disabled",
          "[unit][tes4_bsa_writer]") {
  const std::array targets{libbsa::tes4_bsa_target::fallout3, libbsa::tes4_bsa_target::skyrim_se};
  const auto source_bytes = bytes_from_text("payload without an embedded name prefix");

  for (const auto target : targets) {
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    options.embed_file_names = false;
    libbsa::tes4_bsa_writer writer{target, options};

    REQUIRE(writer.add_bytes("Meshes/Embedded/Model.nif", source_bytes).has_value());

    const auto archive = output_path(target_name(target) + "-embedded-disabled.bsa");
    auto written = writer.write_to(archive.string());
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());

    const auto& entry = require_entry(opened.value(), "Meshes/Embedded/Model.nif");
    CHECK_FALSE(entry.has_embedded_name);
    CHECK(entry.embedded_name_prefix_size == 0U);
    require_extracted_bytes(opened.value(), "Meshes/Embedded/Model.nif", source_bytes);
  }
}

TEST_CASE("TES4 BSA writer emits opt-in embedded name prefixes for v104 and v105 targets",
          "[unit][tes4_bsa_writer]") {
  const std::array cases{std::pair{libbsa::tes4_bsa_target::fallout3, std::string{"Meshes/Embedded/Model.nif"}},
                         std::pair{libbsa::tes4_bsa_target::skyrim_se,
                                   std::string{"Scripts/Embedded/Quest.pex"}}};
  const auto source_bytes = bytes_from_text("consumer visible bytes must not include the embedded name");

  for (const auto& [target, archive_path] : cases) {
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    options.embed_file_names = true;
    libbsa::tes4_bsa_writer writer{target, options};

    REQUIRE(writer.add_bytes(archive_path, source_bytes).has_value());

    const auto archive = output_path(target_name(target) + "-embedded-enabled.bsa");
    auto written = writer.write_to(archive.string());
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(archive.string());
    REQUIRE(opened.has_value());

    const auto& entry = require_entry(opened.value(), archive_path);
    auto expected_embedded_name = archive_path;
    std::replace(expected_embedded_name.begin(), expected_embedded_name.end(), '/', '\\');
    const auto expected_prefix_size = expected_embedded_name.length() + 1U;
    CHECK(entry.has_embedded_name);
    CHECK(entry.embedded_name_prefix_size == expected_prefix_size);

    const auto archive_bytes = read_binary_file(archive);
    REQUIRE(static_cast<std::size_t>(entry.payload_offset) + expected_prefix_size <= archive_bytes.size());
    CHECK(std::to_integer<unsigned char>(archive_bytes[entry.payload_offset]) == expected_embedded_name.length());
    for (std::size_t index = 0; index < expected_embedded_name.length(); ++index) {
      CHECK(static_cast<char>(std::to_integer<unsigned char>(archive_bytes[entry.payload_offset + 1U + index])) ==
            expected_embedded_name[index]);
    }

    require_extracted_bytes(opened.value(), archive_path, source_bytes);
  }
}

TEST_CASE("TES4 BSA writer ignores embedded name option for v103 target compatibility",
          "[unit][tes4_bsa_writer]") {
  const auto source_bytes = bytes_from_text("oblivion payload remains prefix-free");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.overwrite_existing = true;
  options.embed_file_names = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::oblivion, options};

  REQUIRE(writer.add_bytes("Meshes/Embedded/Model.nif", source_bytes).has_value());

  const auto archive = output_path("oblivion-embedded-compatibility.bsa");
  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());

  const auto& entry = require_entry(opened.value(), "Meshes/Embedded/Model.nif");
  CHECK_FALSE(entry.has_embedded_name);
  CHECK(entry.embedded_name_prefix_size == 0U);
  require_extracted_bytes(opened.value(), "Meshes/Embedded/Model.nif", source_bytes);
}

TEST_CASE("TES4 BSA writer leaves duplicate stored bytes at distinct offsets by default",
          "[unit][tes4_bsa_writer]") {
  const auto source_bytes = bytes_from_text("identical source bytes stored twice without dedupe");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.deduplicate_payloads = false;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};

  REQUIRE(writer.add_bytes("Meshes/Dedupe/First.nif", source_bytes).has_value());
  REQUIRE(writer.add_bytes("Meshes/Dedupe/Second.nif", source_bytes).has_value());

  const auto archive = output_path("dedupe-disabled-distinct-offsets.bsa");
  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());

  const auto& first = require_entry(opened.value(), "Meshes/Dedupe/First.nif");
  const auto& second = require_entry(opened.value(), "Meshes/Dedupe/Second.nif");
  CHECK(first.payload_offset != second.payload_offset);
  CHECK(first.stored_size == second.stored_size);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/First.nif", source_bytes);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/Second.nif", source_bytes);
}

TEST_CASE("TES4 BSA writer shares offsets for opt-in identical final stored bytes",
          "[unit][tes4_bsa_writer]") {
  const auto source_bytes = bytes_from_text("identical source bytes stored twice with opt-in dedupe");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.deduplicate_payloads = true;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};

  REQUIRE(writer.add_bytes("Meshes/Dedupe/First.nif", source_bytes).has_value());
  REQUIRE(writer.add_bytes("Meshes/Dedupe/Second.nif", source_bytes).has_value());

  const auto archive = output_path("dedupe-enabled-shared-offsets.bsa");
  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());

  const auto& first = require_entry(opened.value(), "Meshes/Dedupe/First.nif");
  const auto& second = require_entry(opened.value(), "Meshes/Dedupe/Second.nif");
  CHECK(first.payload_offset == second.payload_offset);
  CHECK(first.stored_size == second.stored_size);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/First.nif", source_bytes);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/Second.nif", source_bytes);
}

TEST_CASE("TES4 BSA writer does not dedupe matching source bytes with different compression encodings",
          "[unit][tes4_bsa_writer]") {
  const auto source_bytes = bytes_from_text("same source but raw and compressed stored bytes differ");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.deduplicate_payloads = true;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};

  REQUIRE(writer.add_bytes("Meshes/Dedupe/Raw.nif", source_bytes, libbsa::entry_compression_policy::raw).has_value());
  REQUIRE(writer.add_bytes("Meshes/Dedupe/Compressed.nif", source_bytes,
                           libbsa::entry_compression_policy::compressed)
              .has_value());

  const auto archive = output_path("dedupe-compression-mismatch.bsa");
  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());

  const auto& raw = require_entry(opened.value(), "Meshes/Dedupe/Raw.nif");
  const auto& compressed = require_entry(opened.value(), "Meshes/Dedupe/Compressed.nif");
  CHECK(raw.payload_offset != compressed.payload_offset);
  CHECK(raw.compression == libbsa::entry_compression::none);
  CHECK(compressed.compression == libbsa::entry_compression::deflate);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/Raw.nif", source_bytes);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/Compressed.nif", source_bytes);
}

TEST_CASE("TES4 BSA writer does not dedupe matching source bytes with different embedded-name prefixes",
          "[unit][tes4_bsa_writer]") {
  const auto source_bytes = bytes_from_text("same source but embedded prefixes include file names");
  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  options.embed_file_names = true;
  options.deduplicate_payloads = true;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};

  REQUIRE(writer.add_bytes("Meshes/Dedupe/First.nif", source_bytes).has_value());
  REQUIRE(writer.add_bytes("Meshes/Dedupe/Second.nif", source_bytes).has_value());

  const auto archive = output_path("dedupe-embedded-name-mismatch.bsa");
  auto written = writer.write_to(archive.string());
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());

  const auto& first = require_entry(opened.value(), "Meshes/Dedupe/First.nif");
  const auto& second = require_entry(opened.value(), "Meshes/Dedupe/Second.nif");
  CHECK(first.payload_offset != second.payload_offset);
  CHECK(first.has_embedded_name);
  CHECK(second.has_embedded_name);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/First.nif", source_bytes);
  require_extracted_bytes(opened.value(), "Meshes/Dedupe/Second.nif", source_bytes);
}
