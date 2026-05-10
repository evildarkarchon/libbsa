#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <array>
#include <atomic>
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
  auto path = std::filesystem::temp_directory_path() / "libbsa_bsa_writer_execution_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path output_path(std::string name) {
  static std::atomic_uint64_t counter{0};
  auto path = writer_test_dir() / std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
  std::filesystem::create_directories(path);
  return path / std::move(name);
}

std::vector<std::byte> patterned_bytes(std::size_t size, std::uint8_t seed) {
  std::vector<std::byte> bytes;
  bytes.reserve(size);
  for (std::size_t index = 0; index < size; ++index) {
    bytes.push_back(static_cast<std::byte>((seed + (index * 31U)) & 0xFFU));
  }
  return bytes;
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (const char ch : text) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
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

void require_extracted_bytes(const libbsa::archive_reader& reader,
                             std::string_view path,
                             const std::vector<std::byte>& expected) {
  auto extracted = reader.extract_bytes(path);
  REQUIRE(extracted.has_value());
  CHECK(extracted.value() == expected);
}

void require_same_reopened_payloads(const std::filesystem::path& serial_output,
                                    const std::filesystem::path& parallel_output,
                                    std::span<const std::pair<std::string_view, std::vector<std::byte>>> entries) {
  auto serial_reader = libbsa::archive_reader::open(serial_output.string());
  REQUIRE(serial_reader.has_value());
  auto parallel_reader = libbsa::archive_reader::open(parallel_output.string());
  REQUIRE(parallel_reader.has_value());

  auto serial_metadata = serial_reader.value().metadata();
  REQUIRE(serial_metadata.has_value());
  auto parallel_metadata = parallel_reader.value().metadata();
  REQUIRE(parallel_metadata.has_value());
  CHECK(parallel_metadata.value().type == serial_metadata.value().type);
  CHECK(parallel_metadata.value().variant == serial_metadata.value().variant);
  CHECK(parallel_metadata.value().version == serial_metadata.value().version);
  CHECK(parallel_metadata.value().file_count == serial_metadata.value().file_count);
  CHECK(parallel_metadata.value().archive_flags == serial_metadata.value().archive_flags);

  for (const auto& [archive_path, expected] : entries) {
    require_extracted_bytes(serial_reader.value(), archive_path, expected);
    require_extracted_bytes(parallel_reader.value(), archive_path, expected);
  }
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

void add_tes4_sources(libbsa::tes4_bsa_writer& writer,
                      const std::filesystem::path& root,
                      std::span<const std::pair<std::string_view, std::vector<std::byte>>> entries) {
  for (std::size_t index = 0; index < entries.size(); ++index) {
    const auto source = root / ("source-" + std::to_string(index) + ".bin");
    write_binary_file(source, entries[index].second);
    REQUIRE(writer.add_file(entries[index].first, source.string()).has_value());
  }
}

} // namespace

TEST_CASE("bsa_writer_execution TES3 worker_count streams large disk-backed source and reopens output",
          "[unit][bsa_writer_execution][tes3_bsa_writer]") {
  const auto source = output_path("large-tes3-source.nif");
  const auto archive = output_path("large-tes3-output.bsa");
  const auto expected = patterned_bytes((2U * 64U * 1024U) + 4097U, 0x35U);
  write_binary_file(source, expected);

  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};
  REQUIRE(writer.add_file("Meshes/Large/Probe.NIF", source.string()).has_value());

  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  auto written = writer.write_to(archive.string(), execution);
  REQUIRE(written.has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  require_extracted_bytes(opened.value(), "Meshes/Large/Probe.NIF", expected);
}

TEST_CASE("bsa_writer_execution TES4 worker_count preserves deflate and LZ4-frame output compatibility",
          "[unit][bsa_writer_execution][tes4_bsa_writer]") {
  constexpr std::array targets{libbsa::tes4_bsa_target::fallout3, libbsa::tes4_bsa_target::skyrim_se};
  const std::array entries{
      std::pair<std::string_view, std::vector<std::byte>>{"Meshes/Large/Model.nif",
                                                          patterned_bytes(92U * 1024U, 0x11U)},
      std::pair<std::string_view, std::vector<std::byte>>{"Textures/Large/Diffuse.dds",
                                                          patterned_bytes(75U * 1024U, 0x42U)},
      std::pair<std::string_view, std::vector<std::byte>>{"Scripts/Small/Quest.pex",
                                                          bytes_from_text("small script payload")},
  };

  for (const auto target : targets) {
    const auto root = output_path(target_name(target) + "-sources").parent_path();
    const auto serial_output = root / (target_name(target) + "-serial.bsa");
    const auto parallel_output = root / (target_name(target) + "-parallel.bsa");

    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_compressed;
    options.overwrite_existing = true;

    libbsa::tes4_bsa_writer serial_writer{target, options};
    add_tes4_sources(serial_writer, root, entries);
    REQUIRE(serial_writer.write_to(serial_output.string()).has_value());

    libbsa::tes4_bsa_writer parallel_writer{target, options};
    add_tes4_sources(parallel_writer, root, entries);
    libbsa::write_execution_options execution;
    execution.worker_count = 4U;
    REQUIRE(parallel_writer.write_to(parallel_output.string(), execution).has_value());

    CHECK(read_binary_file(parallel_output) == read_binary_file(serial_output));
    require_same_reopened_payloads(serial_output, parallel_output, entries);
  }
}

TEST_CASE("bsa_writer_execution missing disk source with worker_count preserves existing output",
          "[unit][bsa_writer_execution][tes4_bsa_writer][publish]") {
  const auto archive = output_path("missing-source-preserves-output.bsa");
  const auto missing_source = output_path("missing-source-input.nif");
  const auto sentinel = bytes_from_text("existing output sentinel");
  write_binary_file(archive, sentinel);
  std::error_code fs_error;
  std::filesystem::remove(missing_source, fs_error);

  libbsa::tes4_bsa_writer_options options;
  options.compression_policy = libbsa::archive_compression_policy::all_compressed;
  options.overwrite_existing = true;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
  REQUIRE(writer.add_file("Meshes/Missing/Source.nif", missing_source.string()).has_value());

  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  auto written = writer.write_to(archive.string(), execution);

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
  CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE("bsa_writer_execution overwrite failure with worker_count preserves readable previous archive",
          "[unit][bsa_writer_execution][tes4_bsa_writer][publish]") {
  const auto archive = output_path("overwrite-failure-keeps-readable-archive.bsa");
  const auto missing_source = output_path("overwrite-missing-source.nif");
  const auto original_payload = bytes_from_text("original readable archive payload");

  libbsa::tes4_bsa_writer_options original_options;
  original_options.compression_policy = libbsa::archive_compression_policy::all_raw;
  original_options.overwrite_existing = true;
  libbsa::tes4_bsa_writer original_writer{libbsa::tes4_bsa_target::fallout3, original_options};
  REQUIRE(original_writer.add_bytes("Meshes/Original/Readable.nif", original_payload).has_value());
  REQUIRE(original_writer.write_to(archive.string()).has_value());
  const auto sentinel = read_binary_file(archive);

  std::error_code fs_error;
  std::filesystem::remove(missing_source, fs_error);
  libbsa::tes4_bsa_writer_options replacement_options;
  replacement_options.compression_policy = libbsa::archive_compression_policy::all_compressed;
  replacement_options.overwrite_existing = true;
  libbsa::tes4_bsa_writer replacement_writer{libbsa::tes4_bsa_target::fallout3, replacement_options};
  REQUIRE(replacement_writer.add_file("Meshes/Missing/Replacement.nif", missing_source.string()).has_value());

  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  auto written = replacement_writer.write_to(archive.string(), execution);

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
  CHECK(read_binary_file(archive) == sentinel);
  auto reopened = libbsa::archive_reader::open(archive.string());
  REQUIRE(reopened.has_value());
  require_extracted_bytes(reopened.value(), "Meshes/Original/Readable.nif", original_payload);
}
