#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

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

void require_extracted_bytes(const libbsa::archive_reader& reader,
                             std::string_view path,
                             const std::vector<std::byte>& expected) {
  auto extracted = reader.extract_bytes(path);
  REQUIRE(extracted.has_value());
  CHECK(extracted.value() == expected);
}

} // namespace

TEST_CASE("TES3 BSA writer copies memory entries into writer-owned state", "[unit][tes3_bsa_writer]") {
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

TEST_CASE("TES3 BSA writer reports missing disk sources from write_to", "[unit][tes3_bsa_writer]") {
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

TEST_CASE("TES3 BSA writer rejects invalid archive paths", "[unit][tes3_bsa_writer]") {
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

TEST_CASE("TES3 BSA writer rejects duplicate canonical archive paths at write time", "[unit][tes3_bsa_writer]") {
  libbsa::tes3_bsa_writer writer;
  REQUIRE(writer.add_bytes("Meshes/Duplicate.NIF", sample_bytes()).has_value());
  REQUIRE(writer.add_bytes("meshes/duplicate.nif", bytes_from_text("duplicate")).has_value());

  auto written = writer.write_to(output_path("duplicate-canonical-path.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("TES3 BSA writer rejects empty archives", "[unit][tes3_bsa_writer]") {
  libbsa::tes3_bsa_writer writer;

  auto written = writer.write_to(output_path("empty-archive.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("TES3 BSA writer refuses overwrite by default and preserves existing bytes", "[unit][tes3_bsa_writer]") {
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

TEST_CASE("TES3 BSA writer replaces existing output only when overwrite is enabled", "[unit][tes3_bsa_writer]") {
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

TEST_CASE("TES3 BSA writer preserves caller-owned temp-name sibling files", "[unit][tes3_bsa_writer]") {
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
