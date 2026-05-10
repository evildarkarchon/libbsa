#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

std::filesystem::path bulk_extraction_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_bulk_extraction_tests";
  std::filesystem::create_directories(path);
  return path;
}

std::filesystem::path output_path(std::string_view name) {
  static std::atomic_uint64_t counter{0};
  auto directory = bulk_extraction_test_dir() / std::to_string(counter.fetch_add(1U, std::memory_order_relaxed));
  std::filesystem::create_directories(directory);
  return directory / std::string{name};
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (const char ch : text) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

std::vector<std::byte> patterned_bytes(std::size_t count, std::uint8_t seed) {
  std::vector<std::byte> bytes;
  bytes.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    bytes.push_back(static_cast<std::byte>(static_cast<std::uint8_t>(seed + index)));
  }
  return bytes;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.good());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.good());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

libbsa::archive_reader create_bulk_test_reader() {
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};

  REQUIRE(writer.add_bytes("Meshes/Alpha.NIF", bytes_from_text("alpha mesh bytes")).has_value());
  REQUIRE(writer.add_bytes("Textures/Beta.DDS", bytes_from_text("beta texture bytes")).has_value());
  REQUIRE(writer.add_bytes("Docs/Gamma.txt", bytes_from_text("gamma document bytes")).has_value());

  const auto archive = output_path("bulk-source.bsa");
  REQUIRE(writer.write_to(archive.string()).has_value());

  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  return std::move(opened).value();
}

struct sink_capture {
  std::vector<std::byte> bytes;
};

class capturing_sink final : public libbsa::payload_sink {
 public:
  explicit capturing_sink(std::shared_ptr<sink_capture> capture) : capture_(std::move(capture)) {}

  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    capture_->bytes.insert(capture_->bytes.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

 private:
  std::shared_ptr<sink_capture> capture_;
};

class capturing_sink_factory final : public libbsa::bulk_extract_sink_factory {
 public:
  libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(std::string_view path,
                                                               const libbsa::entry_metadata&) override {
    auto capture = std::make_shared<sink_capture>();
    {
      std::lock_guard lock{mutex_};
      captures_.emplace(std::string{path}, capture);
    }
    return std::unique_ptr<libbsa::payload_sink>{new capturing_sink{std::move(capture)}};
  }

  [[nodiscard]] std::map<std::string, std::vector<std::byte>> bytes_by_path() const {
    std::lock_guard lock{mutex_};
    std::map<std::string, std::vector<std::byte>> bytes;
    for (const auto& [path, capture] : captures_) {
      bytes.emplace(path, capture->bytes);
    }
    return bytes;
  }

 private:
  mutable std::mutex mutex_;
  std::map<std::string, std::shared_ptr<sink_capture>> captures_;
};

class partial_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    return bytes.empty() ? 0U : bytes.size() - 1U;
  }
};

class partial_sink_factory final : public libbsa::bulk_extract_sink_factory {
 public:
  libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(std::string_view,
                                                               const libbsa::entry_metadata&) override {
    return std::unique_ptr<libbsa::payload_sink>{new partial_sink{}};
  }
};

struct counting_sink_stats {
  std::uint64_t total_bytes{0};
  std::size_t write_count{0};
  std::size_t max_chunk_size{0};
};

class counting_sink final : public libbsa::payload_sink {
 public:
  explicit counting_sink(std::shared_ptr<counting_sink_stats> stats) : stats_(std::move(stats)) {}

  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    stats_->total_bytes += bytes.size();
    ++stats_->write_count;
    stats_->max_chunk_size = std::max(stats_->max_chunk_size, bytes.size());
    return bytes.size();
  }

 private:
  std::shared_ptr<counting_sink_stats> stats_;
};

class counting_sink_factory final : public libbsa::bulk_extract_sink_factory {
 public:
  libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(std::string_view,
                                                               const libbsa::entry_metadata&) override {
    return std::unique_ptr<libbsa::payload_sink>{new counting_sink{stats_}};
  }

  [[nodiscard]] const counting_sink_stats& stats() const noexcept { return *stats_; }

 private:
  std::shared_ptr<counting_sink_stats> stats_{std::make_shared<counting_sink_stats>()};
};

struct extraction_run {
  std::vector<libbsa::bulk_extract_entry_result> results;
  std::map<std::string, std::vector<std::byte>> bytes_by_path;
};

extraction_run extract_with_workers(const libbsa::archive_reader& reader,
                                    std::span<const libbsa::bulk_extract_request> requests,
                                    std::uint32_t worker_count) {
  capturing_sink_factory sink_factory;
  libbsa::bulk_extract_options options;
  options.worker_count = worker_count;

  auto extracted = reader.extract_entries(requests, sink_factory, options);
  REQUIRE(extracted.has_value());
  return extraction_run{std::move(extracted).value(), sink_factory.bytes_by_path()};
}

std::vector<libbsa::bulk_extract_request> standard_requests() {
  return {libbsa::bulk_extract_request{.path = "Docs/Gamma.txt"},
          libbsa::bulk_extract_request{.path = "Meshes/Alpha.NIF"},
          libbsa::bulk_extract_request{.path = "Textures/Beta.DDS"}};
}

libbsa::archive_reader create_tes3_raw_reader(std::span<const std::byte> payload) {
  libbsa::tes3_bsa_writer_options options;
  options.overwrite_existing = true;
  libbsa::tes3_bsa_writer writer{options};
  REQUIRE(writer.add_bytes("Meshes/LargeRaw.NIF", payload).has_value());

  const auto archive = output_path("large-tes3-raw.bsa");
  REQUIRE(writer.write_to(archive.string()).has_value());
  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  return std::move(opened).value();
}

libbsa::archive_reader create_tes4_raw_reader(std::span<const std::byte> payload) {
  libbsa::tes4_bsa_writer_options options;
  options.overwrite_existing = true;
  options.compression_policy = libbsa::archive_compression_policy::all_raw;
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
  REQUIRE(writer.add_bytes("Meshes/LargeRaw.NIF", payload).has_value());

  const auto archive = output_path("large-tes4-raw.bsa");
  REQUIRE(writer.write_to(archive.string()).has_value());
  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  return std::move(opened).value();
}

libbsa::archive_reader create_ba2_gnrl_raw_reader(std::span<const std::byte> payload) {
  libbsa::ba2_gnrl_writer_options options;
  options.overwrite_existing = true;
  options.compression = libbsa::archive_compression_policy::all_raw;
  libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
  REQUIRE(writer.add_bytes("Meshes/LargeRaw.NIF", payload).has_value());

  const auto archive = output_path("large-ba2-gnrl-raw.ba2");
  REQUIRE(writer.write_to(archive.string()).has_value());
  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  return std::move(opened).value();
}

struct byte_buffer {
  std::vector<std::byte> bytes;

  void u8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }

  void u16(std::uint16_t value) {
    for (std::uint32_t index = 0; index < 2U; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }

  void u32(std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4U; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }

  void u64(std::uint64_t value) {
    for (std::uint32_t index = 0; index < 8U; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }

  void ascii4(std::array<char, 4U> value) {
    for (const char ch : value) {
      u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
    }
  }

  void string_u16(std::string_view value) {
    REQUIRE(value.size() <= std::numeric_limits<std::uint16_t>::max());
    u16(static_cast<std::uint16_t>(value.size()));
    for (const char ch : value) {
      u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
    }
  }

  void raw(std::span<const std::byte> values) { bytes.insert(bytes.end(), values.begin(), values.end()); }
};

std::vector<std::byte> build_raw_ba2_dx10_fixture(std::string_view archive_path, std::span<const std::byte> payload) {
  constexpr std::uint32_t file_count = 1U;
  constexpr std::uint32_t version = 1U;
  constexpr std::uint16_t chunk_header_size = 24U;
  constexpr std::uint32_t sentinel = 0xBAAD'F00DU;
  constexpr std::uint8_t dxgi_format_r8g8b8a8_unorm = 28U;
  constexpr std::uint16_t width = 256U;
  constexpr std::uint16_t height = 256U;
  constexpr std::uint64_t file_table_offset = 24ULL + 24ULL + chunk_header_size;
  const std::uint64_t payload_offset = file_table_offset + 2ULL + archive_path.size();
  REQUIRE(payload.size() == static_cast<std::size_t>(width) * height * 4U);

  byte_buffer writer;
  writer.ascii4({'B', 'T', 'D', 'X'});
  writer.u32(version);
  writer.ascii4({'D', 'X', '1', '0'});
  writer.u32(file_count);
  writer.u64(file_table_offset);
  writer.u32(0U);
  writer.ascii4({'d', 'd', 's', '\0'});
  writer.u32(0U);
  writer.u8(0U);
  writer.u8(1U);
  writer.u16(chunk_header_size);
  writer.u16(height);
  writer.u16(width);
  writer.u8(1U);
  writer.u8(dxgi_format_r8g8b8a8_unorm);
  writer.u16(0U);
  writer.u64(payload_offset);
  writer.u32(0U);
  writer.u32(static_cast<std::uint32_t>(payload.size()));
  writer.u16(0U);
  writer.u16(0U);
  writer.u32(sentinel);
  writer.string_u16(archive_path);
  writer.raw(payload);
  return writer.bytes;
}

libbsa::archive_reader create_ba2_dx10_raw_reader(std::span<const std::byte> payload) {
  const auto archive = output_path("large-ba2-dx10-raw.ba2");
  write_binary_file(archive, build_raw_ba2_dx10_fixture("textures/bulk/raw_large.dds", payload));
  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE(opened.has_value());
  return std::move(opened).value();
}

void require_bulk_extracts_in_bounded_chunks(const libbsa::archive_reader& reader,
                                             std::string_view path,
                                             std::uint64_t expected_total_bytes) {
  const std::vector requests{libbsa::bulk_extract_request{.path = std::string{path}}};
  counting_sink_factory factory;

  auto extracted = reader.extract_entries(requests, factory);

  REQUIRE(extracted.has_value());
  REQUIRE(extracted.value().size() == 1U);
  REQUIRE(extracted.value()[0].succeeded());
  CHECK(factory.stats().total_bytes == expected_total_bytes);
  CHECK(factory.stats().write_count > 1U);
  CHECK(factory.stats().max_chunk_size <= 64U * 1024U);
}

} // namespace

static_assert(std::is_same_v<decltype(libbsa::bulk_extract_options{}.worker_count), std::uint32_t>);
static_assert(std::is_same_v<decltype(libbsa::bulk_extract_request{}.path), std::string>);
static_assert(std::is_abstract_v<libbsa::bulk_extract_sink_factory>);
static_assert(std::is_same_v<decltype(std::declval<libbsa::bulk_extract_sink_factory&>().create(
                                 "meshes/example.nif", std::declval<const libbsa::entry_metadata&>())),
                             libbsa::result<std::unique_ptr<libbsa::payload_sink>>>);
static_assert(std::is_same_v<decltype(libbsa::bulk_extract_entry_result{}.path), std::string>);
static_assert(std::is_same_v<decltype(libbsa::bulk_extract_entry_result{}.entry),
                             std::optional<libbsa::entry_metadata>>);
static_assert(std::is_same_v<decltype(libbsa::bulk_extract_entry_result{}.failure), std::optional<libbsa::error>>);
static_assert(std::is_same_v<decltype(libbsa::bulk_extract_entry_result{}.succeeded()), bool>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().extract_entries(
                                 std::declval<std::span<const libbsa::bulk_extract_request>>(),
                                 std::declval<libbsa::bulk_extract_sink_factory&>())),
                             libbsa::result<std::vector<libbsa::bulk_extract_entry_result>>>);

TEST_CASE("bulk_extraction options default to serial extraction", "[unit][bulk_extraction]") {
  CHECK(libbsa::bulk_extract_options{}.worker_count == 1U);
}

TEST_CASE("bulk_extraction rejects zero workers as an outer argument error", "[unit][bulk_extraction]") {
  const auto reader = create_bulk_test_reader();
  const auto requests = standard_requests();
  capturing_sink_factory sink_factory;
  libbsa::bulk_extract_options options;
  options.worker_count = 0U;

  auto extracted = reader.extract_entries(requests, sink_factory, options);

  REQUIRE_FALSE(extracted.has_value());
  CHECK(extracted.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("bulk_extraction preserves request order and bytes in serial and parallel mode", "[unit][bulk_extraction]") {
  const auto reader = create_bulk_test_reader();
  const auto requests = standard_requests();

  auto serial = extract_with_workers(reader, requests, 1U);
  auto parallel = extract_with_workers(reader, requests, 4U);

  REQUIRE(serial.results.size() == requests.size());
  REQUIRE(parallel.results.size() == requests.size());
  for (std::size_t index = 0; index < requests.size(); ++index) {
    CAPTURE(index);
    CHECK(serial.results[index].path == requests[index].path);
    CHECK(parallel.results[index].path == requests[index].path);
    CHECK(serial.results[index].succeeded());
    CHECK(parallel.results[index].succeeded());
    REQUIRE(serial.results[index].entry.has_value());
    REQUIRE(parallel.results[index].entry.has_value());
    CHECK(serial.results[index].entry->path == parallel.results[index].entry->path);
  }

  CHECK(serial.bytes_by_path == parallel.bytes_by_path);
  CHECK(serial.bytes_by_path.at("Docs/Gamma.txt") == bytes_from_text("gamma document bytes"));
  CHECK(serial.bytes_by_path.at("Meshes/Alpha.NIF") == bytes_from_text("alpha mesh bytes"));
  CHECK(serial.bytes_by_path.at("Textures/Beta.DDS") == bytes_from_text("beta texture bytes"));
}

TEST_CASE("bulk_extraction records per-entry failures without aborting siblings", "[unit][bulk_extraction]") {
  const auto reader = create_bulk_test_reader();
  const std::vector requests{libbsa::bulk_extract_request{.path = "Meshes/Alpha.NIF"},
                             libbsa::bulk_extract_request{.path = "Missing/Entry.bin"},
                             libbsa::bulk_extract_request{.path = "Textures/Beta.DDS"}};
  capturing_sink_factory sink_factory;

  auto extracted = reader.extract_entries(requests, sink_factory, libbsa::bulk_extract_options{.worker_count = 4U});

  REQUIRE(extracted.has_value());
  REQUIRE(extracted.value().size() == requests.size());
  CHECK(extracted.value()[0].succeeded());
  CHECK(extracted.value()[0].path == "Meshes/Alpha.NIF");
  CHECK_FALSE(extracted.value()[1].succeeded());
  CHECK(extracted.value()[1].path == "Missing/Entry.bin");
  CHECK_FALSE(extracted.value()[1].entry.has_value());
  REQUIRE(extracted.value()[1].failure.has_value());
  CHECK(extracted.value()[1].failure->code == libbsa::error_code::not_found);
  CHECK(extracted.value()[2].succeeded());

  const auto bytes = sink_factory.bytes_by_path();
  CHECK(bytes.at("Meshes/Alpha.NIF") == bytes_from_text("alpha mesh bytes"));
  CHECK(bytes.at("Textures/Beta.DDS") == bytes_from_text("beta texture bytes"));
  CHECK(bytes.find("Missing/Entry.bin") == bytes.end());
}

TEST_CASE("bulk_extraction records partial sink writes as per-entry I/O errors", "[unit][bulk_extraction]") {
  const auto reader = create_bulk_test_reader();
  const std::vector requests{libbsa::bulk_extract_request{.path = "Meshes/Alpha.NIF"}};
  partial_sink_factory sink_factory;

  auto extracted = reader.extract_entries(requests, sink_factory);

  REQUIRE(extracted.has_value());
  REQUIRE(extracted.value().size() == 1U);
  CHECK_FALSE(extracted.value()[0].succeeded());
  CHECK(extracted.value()[0].path == "Meshes/Alpha.NIF");
  REQUIRE(extracted.value()[0].entry.has_value());
  REQUIRE(extracted.value()[0].failure.has_value());
  CHECK(extracted.value()[0].failure->code == libbsa::error_code::io_error);
}

TEST_CASE("bulk_extraction streams large raw archive families through bounded chunks", "[unit][bulk_extraction][payload_stream]") {
  constexpr std::size_t raw_chunk_limit = 64U * 1024U;
  const auto large_payload = patterned_bytes((raw_chunk_limit * 2U) + 17U, 0x31U);

  require_bulk_extracts_in_bounded_chunks(create_tes3_raw_reader(large_payload),
                                          "Meshes/LargeRaw.NIF",
                                          large_payload.size());
  require_bulk_extracts_in_bounded_chunks(create_tes4_raw_reader(large_payload),
                                          "Meshes/LargeRaw.NIF",
                                          large_payload.size());
  require_bulk_extracts_in_bounded_chunks(create_ba2_gnrl_raw_reader(large_payload),
                                          "Meshes/LargeRaw.NIF",
                                          large_payload.size());

  const auto dx10_payload = patterned_bytes(256U * 256U * 4U, 0x53U);
  constexpr std::uint64_t dds_dxt10_header_size = 148U;
  require_bulk_extracts_in_bounded_chunks(create_ba2_dx10_raw_reader(dx10_payload),
                                          "textures/bulk/raw_large.dds",
                                          dds_dxt10_header_size + dx10_payload.size());
}

TEST_CASE("bulk_extraction source policy documents raw chunking and compressed allocation boundaries",
          "[unit][bulk_extraction][payload_stream]") {
  const auto root = std::filesystem::path{LIBBSA_SOURCE_DIR};
  const std::array reader_files{
      root / "src" / "formats" / "bsa" / "tes3_bsa_reader.cpp",
      root / "src" / "formats" / "bsa" / "tes4_bsa_reader.cpp",
      root / "src" / "formats" / "ba2" / "ba2_gnrl_reader.cpp",
      root / "src" / "formats" / "ba2" / "ba2_dx10_reader.cpp",
  };

  for (const auto& reader_file : reader_files) {
    const auto contents = read_text_file(reader_file);
    INFO("reader file: " << reader_file.string());
    CHECK(contents.find("extraction_chunk_size = 64U * 1024U") != std::string::npos);
    CHECK(contents.find("archive_size") == std::string::npos);
  }

  const auto tes4_reader = read_text_file(root / "src" / "formats" / "bsa" / "tes4_bsa_reader.cpp");
  const auto ba2_gnrl_reader = read_text_file(root / "src" / "formats" / "ba2" / "ba2_gnrl_reader.cpp");
  const auto ba2_dx10_reader = read_text_file(root / "src" / "formats" / "ba2" / "ba2_dx10_reader.cpp");
  CHECK(tes4_reader.find("decompress_payload_exact") != std::string::npos);
  CHECK(ba2_gnrl_reader.find("decompress_payload_exact") != std::string::npos);
  CHECK(ba2_dx10_reader.find("decompress_payload_exact") != std::string::npos);

  const std::string compressed_boundary =
      "D-14 permits compressed extraction to allocate per-entry or per-texture-chunk codec buffers, but not whole-archive buffers.";
  CHECK(compressed_boundary.find("per-entry") != std::string::npos);
  CHECK(compressed_boundary.find("per-texture-chunk") != std::string::npos);
}
