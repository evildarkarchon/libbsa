#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
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
