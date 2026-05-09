#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
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

std::vector<std::byte> sample_bytes() {
  return {std::byte{0x42}, std::byte{0x53}, std::byte{0x41}, std::byte{0x21}};
}

} // namespace

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
