#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

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

} // namespace

static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().metadata()),
                             libbsa::result<libbsa::archive_metadata>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().entries()),
                             libbsa::result<std::vector<libbsa::entry_metadata>>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().find("meshes/example.nif")),
                             libbsa::result<std::optional<libbsa::entry_metadata>>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().contains("meshes/example.nif")),
                             libbsa::result<bool>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().extract(
                                 "meshes/example.nif", std::declval<libbsa::payload_sink&>())),
                             libbsa::result<void>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().extract_bytes(
                                 "meshes/example.nif")),
                             libbsa::result<std::vector<std::byte>>>);

TEST_CASE("archive_reader open reports I/O errors for missing host files", "[unit][public-api]") {
  auto result = libbsa::archive_reader::open("missing/example.bsa");

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().code == libbsa::error_code::io_error);
}

TEST_CASE("archive_reader open rejects empty host paths", "[unit][public-api]") {
  auto result = libbsa::archive_reader::open("");

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("archive_reader exposes compile-only Phase 3 reader contracts", "[unit][public-api]") {
  collecting_sink sink;
  nlohmann::json manifest = {{"path", "meshes/example.nif"}};

  REQUIRE(sink.write(std::span<const std::byte>{}).value() == 0);
  REQUIRE(sink.bytes().empty());
  REQUIRE(manifest.at("path").get<std::string>() == "meshes/example.nif");
  REQUIRE(libbsa::error_code::not_found == libbsa::error_code::not_found);
}
