#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
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

TEST_CASE("bulk_extract_options defaults to serial extraction", "[unit][bulk_extraction]") {
  CHECK(libbsa::bulk_extract_options{}.worker_count == 1U);
}

TEST_CASE("bulk extraction rejects zero workers as an outer argument error", "[unit][bulk_extraction]") {
  const auto reader = create_bulk_test_reader();
  const auto requests = standard_requests();
  capturing_sink_factory sink_factory;
  libbsa::bulk_extract_options options;
  options.worker_count = 0U;

  auto extracted = reader.extract_entries(requests, sink_factory, options);

  REQUIRE_FALSE(extracted.has_value());
  CHECK(extracted.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("bulk extraction preserves request order and bytes in serial and parallel mode", "[unit][bulk_extraction]") {
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

TEST_CASE("bulk extraction records per-entry failures without aborting siblings", "[unit][bulk_extraction]") {
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

TEST_CASE("bulk extraction records partial sink writes as per-entry I/O errors", "[unit][bulk_extraction]") {
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
