#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "texture/directxtex_analyzer.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_ba2_writer_execution_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path output_path(std::string name) {
  static std::atomic_uint64_t counter{0};
  auto path = writer_test_dir() / std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
  std::filesystem::create_directories(path);
  return path / std::move(name);
}

std::filesystem::path generated_source_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "source";
}

std::vector<std::byte> repeated_bytes(std::size_t size, std::uint8_t value) {
  return std::vector<std::byte>(size, static_cast<std::byte>(value));
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
  REQUIRE_FALSE(input.bad());
  return bytes;
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}

const nlohmann::json& valid_source_case(const nlohmann::json& manifest, std::string_view id) {
  const auto found = std::ranges::find_if(manifest.at("valid_cases"), [id](const nlohmann::json& source_case) {
    return source_case.at("id").get<std::string>() == id;
  });
  REQUIRE(found != manifest.at("valid_cases").end());
  return *found;
}

struct gnrl_case {
  std::string archive_path;
  std::vector<std::byte> bytes;
};

void add_gnrl_disk_sources(libbsa::ba2_gnrl_writer& writer,
                           const std::filesystem::path& root,
                           std::span<const gnrl_case> entries) {
  for (std::size_t index = 0; index < entries.size(); ++index) {
    const auto source = root / ("gnrl-source-" + std::to_string(index) + ".bin");
    write_binary_file(source, entries[index].bytes);
    REQUIRE(writer.add_file(entries[index].archive_path, source.string()).has_value());
  }
}

void require_gnrl_outputs_match(const std::filesystem::path& serial_output,
                                const std::filesystem::path& parallel_output,
                                std::span<const gnrl_case> entries,
                                libbsa::archive_variant expected_variant,
                                std::uint32_t expected_version,
                                libbsa::entry_compression expected_compression) {
  auto serial_reader = libbsa::archive_reader::open(serial_output.string());
  REQUIRE(serial_reader.has_value());
  auto parallel_reader = libbsa::archive_reader::open(parallel_output.string());
  REQUIRE(parallel_reader.has_value());

  auto serial_metadata = serial_reader.value().metadata();
  auto parallel_metadata = parallel_reader.value().metadata();
  REQUIRE(serial_metadata.has_value());
  REQUIRE(parallel_metadata.has_value());
  CHECK(parallel_metadata.value().type == libbsa::archive_type::ba2);
  CHECK(parallel_metadata.value().variant == expected_variant);
  CHECK(parallel_metadata.value().version == expected_version);
  CHECK(parallel_metadata.value().file_count == serial_metadata.value().file_count);
  CHECK(parallel_metadata.value().default_compression == expected_compression);

  for (const auto& entry : entries) {
    auto serial_found = serial_reader.value().find(entry.archive_path);
    auto parallel_found = parallel_reader.value().find(entry.archive_path);
    REQUIRE(serial_found.has_value());
    REQUIRE(parallel_found.has_value());
    REQUIRE(serial_found.value().has_value());
    REQUIRE(parallel_found.value().has_value());
    CHECK(serial_found.value()->compression == expected_compression);
    CHECK(parallel_found.value()->compression == expected_compression);
    CHECK(serial_found.value()->raw_size == entry.bytes.size());
    CHECK(parallel_found.value()->raw_size == entry.bytes.size());

    auto serial_bytes = serial_reader.value().extract_bytes(entry.archive_path);
    auto parallel_bytes = parallel_reader.value().extract_bytes(entry.archive_path);
    REQUIRE(serial_bytes.has_value());
    REQUIRE(parallel_bytes.has_value());
    CHECK(serial_bytes.value() == entry.bytes);
    CHECK(parallel_bytes.value() == entry.bytes);
  }
}

struct dx10_case {
  std::string archive_path;
  std::filesystem::path source_path;
  libbsa::texture::dds_source_analysis source;
};

dx10_case make_dx10_case(const nlohmann::json& manifest, std::string_view id) {
  const auto& source_case = valid_source_case(manifest, id);
  auto source_path = generated_source_dir() / source_case.at("file").get<std::string>();
  auto source_bytes = read_binary_file(source_path);
  auto source = libbsa::texture::analyze_dds_source(source_bytes);
  REQUIRE(source.has_value());
  return {source_case.at("archive_path").get<std::string>(), std::move(source_path), std::move(source.value())};
}

void add_dx10_sources(libbsa::ba2_dx10_writer& writer, std::span<const dx10_case> entries) {
  for (const auto& entry : entries) {
    REQUIRE(writer.add_file(entry.archive_path, entry.source_path.string()).has_value());
  }
}

void require_dx10_outputs_match(const std::filesystem::path& serial_output,
                                const std::filesystem::path& parallel_output,
                                std::span<const dx10_case> entries,
                                libbsa::archive_variant expected_variant,
                                std::uint32_t expected_version,
                                libbsa::entry_compression expected_compression) {
  auto serial_reader = libbsa::archive_reader::open(serial_output.string());
  REQUIRE(serial_reader.has_value());
  auto parallel_reader = libbsa::archive_reader::open(parallel_output.string());
  REQUIRE(parallel_reader.has_value());

  auto serial_metadata = serial_reader.value().metadata();
  auto parallel_metadata = parallel_reader.value().metadata();
  REQUIRE(serial_metadata.has_value());
  REQUIRE(parallel_metadata.has_value());
  CHECK(parallel_metadata.value().type == libbsa::archive_type::ba2);
  CHECK(parallel_metadata.value().variant == expected_variant);
  CHECK(parallel_metadata.value().version == expected_version);
  CHECK(parallel_metadata.value().file_count == serial_metadata.value().file_count);
  CHECK(parallel_metadata.value().default_compression == expected_compression);

  for (const auto& entry : entries) {
    auto serial_found = serial_reader.value().find(entry.archive_path);
    auto parallel_found = parallel_reader.value().find(entry.archive_path);
    REQUIRE(serial_found.has_value());
    REQUIRE(parallel_found.has_value());
    REQUIRE(serial_found.value().has_value());
    REQUIRE(parallel_found.value().has_value());
    REQUIRE(serial_found.value()->texture.has_value());
    REQUIRE(parallel_found.value()->texture.has_value());
    CHECK(parallel_found.value()->texture->dxgi_format == entry.source.metadata.dxgi_format);
    CHECK(parallel_found.value()->texture->mip_count == entry.source.metadata.mip_count);
    CHECK(parallel_found.value()->texture->array_size == entry.source.metadata.array_size);

    for (const auto& chunk : parallel_found.value()->texture->chunks) {
      CHECK(chunk.compression == expected_compression);
    }

    auto serial_bytes = serial_reader.value().extract_bytes(entry.archive_path);
    auto parallel_bytes = parallel_reader.value().extract_bytes(entry.archive_path);
    REQUIRE(serial_bytes.has_value());
    REQUIRE(parallel_bytes.has_value());
    auto serial_dds = libbsa::texture::analyze_dds_source(serial_bytes.value());
    auto parallel_dds = libbsa::texture::analyze_dds_source(parallel_bytes.value());
    REQUIRE(serial_dds.has_value());
    REQUIRE(parallel_dds.has_value());
    CHECK(serial_dds.value().metadata.dxgi_format == entry.source.metadata.dxgi_format);
    CHECK(parallel_dds.value().metadata.dxgi_format == entry.source.metadata.dxgi_format);
    CHECK(serial_dds.value().image_payload_bytes == entry.source.image_payload_bytes);
    CHECK(parallel_dds.value().image_payload_bytes == entry.source.image_payload_bytes);
  }
}

} // namespace

TEST_CASE("ba2_writer_execution GNRL worker_count preserves Fallout 4 deflate output",
          "[unit][ba2_writer_execution][bounded_memory_policy][ba2_gnrl_writer][worker_count]") {
  const std::vector<gnrl_case> entries{
      {"Meshes/Large/CompressedA.nif", repeated_bytes(96U * 1024U, 0x41U)},
      {"Scripts/Large/CompressedB.pex", repeated_bytes((80U * 1024U) + 17U, 0x51U)},
  };
  const auto root = output_path("gnrl-fo4-sources").parent_path();
  const auto serial_output = root / "gnrl-fo4-serial.ba2";
  const auto parallel_output = root / "gnrl-fo4-parallel.ba2";

  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::all_compressed;
  options.overwrite_existing = true;

  libbsa::ba2_gnrl_writer serial_writer{libbsa::ba2_gnrl_target::fallout4, options};
  add_gnrl_disk_sources(serial_writer, root, entries);
  libbsa::write_execution_options serial_execution;
  serial_execution.worker_count = 1U;
  REQUIRE(serial_writer.write_to(serial_output.string(), serial_execution).has_value());

  libbsa::ba2_gnrl_writer parallel_writer{libbsa::ba2_gnrl_target::fallout4, options};
  add_gnrl_disk_sources(parallel_writer, root, entries);
  libbsa::write_execution_options parallel_execution;
  parallel_execution.worker_count = 4U;
  REQUIRE(parallel_writer.write_to(parallel_output.string(), parallel_execution).has_value());

  CHECK(read_binary_file(parallel_output) == read_binary_file(serial_output));
  require_gnrl_outputs_match(serial_output,
                             parallel_output,
                             entries,
                             libbsa::archive_variant::fallout4,
                             1U,
                             libbsa::entry_compression::deflate);
}

TEST_CASE("ba2_writer_execution GNRL worker_count preserves starfield v3 raw LZ4 output",
          "[unit][ba2_writer_execution][bounded_memory_policy][ba2_gnrl_writer][worker_count][starfield]") {
  const std::vector<gnrl_case> entries{
      {"Meshes/Starfield/CompressedA.mesh", repeated_bytes(128U * 1024U, 0x6AU)},
      {"Textures/Starfield/CompressedB.bin", repeated_bytes((66U * 1024U) + 9U, 0x22U)},
  };
  const auto root = output_path("gnrl-starfield-sources").parent_path();
  const auto serial_output = root / "gnrl-starfield-serial.ba2";
  const auto parallel_output = root / "gnrl-starfield-parallel.ba2";

  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::all_compressed;
  options.overwrite_existing = true;
  options.starfield_compression_method = 3U;

  libbsa::ba2_gnrl_writer serial_writer{libbsa::ba2_gnrl_target::starfield_v3, options};
  add_gnrl_disk_sources(serial_writer, root, entries);
  REQUIRE(serial_writer.write_to(serial_output.string()).has_value());

  libbsa::ba2_gnrl_writer parallel_writer{libbsa::ba2_gnrl_target::starfield_v3, options};
  add_gnrl_disk_sources(parallel_writer, root, entries);
  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  REQUIRE(parallel_writer.write_to(parallel_output.string(), execution).has_value());

  CHECK(read_binary_file(parallel_output) == read_binary_file(serial_output));
  require_gnrl_outputs_match(serial_output,
                             parallel_output,
                             entries,
                             libbsa::archive_variant::starfield,
                             3U,
                             libbsa::entry_compression::lz4_block);
}

TEST_CASE("ba2_writer_execution DX10 worker_count preserves Fallout 4 deflate texture output",
          "[unit][ba2_writer_execution][bounded_memory_policy][ba2_dx10_writer][worker_count][DX10]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const std::vector<dx10_case> entries{
      make_dx10_case(manifest, "bc1_unorm"),
      make_dx10_case(manifest, "bc3_unorm"),
  };
  const auto root = output_path("dx10-fo4-sources").parent_path();
  const auto serial_output = root / "dx10-fo4-serial.ba2";
  const auto parallel_output = root / "dx10-fo4-parallel.ba2";

  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;

  libbsa::ba2_dx10_writer serial_writer{libbsa::ba2_dx10_target::fallout4, options};
  add_dx10_sources(serial_writer, entries);
  REQUIRE(serial_writer.write_to(serial_output.string()).has_value());

  libbsa::ba2_dx10_writer parallel_writer{libbsa::ba2_dx10_target::fallout4, options};
  add_dx10_sources(parallel_writer, entries);
  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  REQUIRE(parallel_writer.write_to(parallel_output.string(), execution).has_value());

  require_dx10_outputs_match(serial_output,
                             parallel_output,
                             entries,
                             libbsa::archive_variant::fallout4,
                             1U,
                             libbsa::entry_compression::deflate);
}

TEST_CASE("ba2_writer_execution DX10 worker_count preserves starfield v3 raw LZ4 texture output",
          "[unit][ba2_writer_execution][bounded_memory_policy][ba2_dx10_writer][worker_count][DX10][starfield]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const std::vector<dx10_case> entries{
      make_dx10_case(manifest, "bc7_unorm"),
      make_dx10_case(manifest, "r8g8b8a8_unorm"),
  };
  const auto root = output_path("dx10-starfield-sources").parent_path();
  const auto serial_output = root / "dx10-starfield-serial.ba2";
  const auto parallel_output = root / "dx10-starfield-parallel.ba2";

  libbsa::ba2_dx10_writer_options options;
  options.overwrite_existing = true;

  libbsa::ba2_dx10_writer serial_writer{libbsa::ba2_dx10_target::starfield_v3, options};
  add_dx10_sources(serial_writer, entries);
  REQUIRE(serial_writer.write_to(serial_output.string()).has_value());

  libbsa::ba2_dx10_writer parallel_writer{libbsa::ba2_dx10_target::starfield_v3, options};
  add_dx10_sources(parallel_writer, entries);
  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  REQUIRE(parallel_writer.write_to(parallel_output.string(), execution).has_value());

  require_dx10_outputs_match(serial_output,
                             parallel_output,
                             entries,
                             libbsa::archive_variant::starfield,
                             3U,
                             libbsa::entry_compression::lz4_block);
}

TEST_CASE("ba2_writer_execution missing GNRL disk source with worker_count preserves existing sentinel",
          "[unit][ba2_writer_execution][bounded_memory_policy][ba2_gnrl_writer][worker_count][publish]") {
  const auto archive = output_path("missing-source-preserves-output.ba2");
  const auto missing_source = output_path("missing-gnrl-source.nif");
  const auto sentinel = bytes_from_text("existing BA2 output sentinel");
  write_binary_file(archive, sentinel);
  std::error_code fs_error;
  std::filesystem::remove(missing_source, fs_error);

  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::all_compressed;
  options.overwrite_existing = true;
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
  REQUIRE(writer.add_file("Meshes/Missing/Source.nif", missing_source.string()).has_value());

  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  auto written = writer.write_to(archive.string(), execution);

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
  CHECK(read_binary_file(archive) == sentinel);
}

TEST_CASE("ba2_writer_execution missing GNRL disk source with worker_count leaves no partial output",
          "[unit][ba2_writer_execution][bounded_memory_policy][ba2_gnrl_writer][worker_count][publish]") {
  const auto archive = output_path("missing-source-no-partial.ba2");
  const auto missing_source = output_path("missing-gnrl-source-no-partial.nif");
  std::error_code fs_error;
  std::filesystem::remove(archive, fs_error);
  std::filesystem::remove(missing_source, fs_error);

  libbsa::ba2_gnrl_writer_options options;
  options.compression = libbsa::archive_compression_policy::all_compressed;
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
  REQUIRE(writer.add_file("Meshes/Missing/NoPartial.nif", missing_source.string()).has_value());

  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  auto written = writer.write_to(archive.string(), execution);

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
  CHECK_FALSE(std::filesystem::exists(archive));
}

TEST_CASE("ba2_writer_execution duplicate DX10 canonical paths return format_error without partial output",
          "[unit][ba2_writer_execution][bounded_memory_policy][ba2_dx10_writer][worker_count][publish][DX10]") {
  const auto manifest = read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
  const auto source = make_dx10_case(manifest, "bc1_unorm");
  const auto archive = output_path("dx10-duplicate-no-partial.ba2");
  std::error_code fs_error;
  std::filesystem::remove(archive, fs_error);

  libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
  REQUIRE(writer.add_file("Textures/Duplicate/Texture.dds", source.source_path.string()).has_value());
  REQUIRE(writer.add_file("textures/duplicate/texture.dds", source.source_path.string()).has_value());

  libbsa::write_execution_options execution;
  execution.worker_count = 4U;
  auto written = writer.write_to(archive.string(), execution);

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::format_error);
  CHECK_FALSE(std::filesystem::exists(archive));
}
