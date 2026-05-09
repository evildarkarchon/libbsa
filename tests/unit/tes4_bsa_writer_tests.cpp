#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

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
    CHECK(metadata.value().default_compression == libbsa::entry_compression::none);
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
