#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_path(std::string_view filename) {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" /
           "archives" / filename;
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

class unused_sink_factory final : public libbsa::bulk_extract_sink_factory {
   public:
    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(
        std::string_view, const libbsa::entry_metadata&) override {
        return libbsa::error{libbsa::error_code::unsupported,
                             "moved-from reader must not create sinks"};
    }
};

template <typename T>
void require_unopened_reader_error(const libbsa::result<T>& operation) {
    REQUIRE_FALSE(operation.has_value());
    REQUIRE(operation.error().code == libbsa::error_code::unsupported);
}

}  // namespace

static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().metadata()),
                             libbsa::result<libbsa::archive_metadata>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().entries()),
                             libbsa::result<std::vector<libbsa::entry_metadata>>>);
static_assert(std::is_same_v<
              decltype(std::declval<const libbsa::archive_reader&>().find("meshes/example.nif")),
              libbsa::result<std::optional<libbsa::entry_metadata>>>);
static_assert(std::is_same_v<decltype(std::declval<const libbsa::archive_reader&>().contains(
                                 "meshes/example.nif")),
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

TEST_CASE("archive_reader moved-from state preserves unopened reader errors",
          "[unit][public-api][archive_entry_catalog][reader_extraction_dispatch]") {
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());
    REQUIRE(opened.has_value());

    auto source = std::move(opened).value();
    auto destination = std::move(source);
    REQUIRE(destination.entries().has_value());

    collecting_sink sink;
    unused_sink_factory sink_factory;
    require_unopened_reader_error(source.metadata());
    require_unopened_reader_error(source.entries());
    require_unopened_reader_error(source.find("meshes/tiny/probe.nif"));
    require_unopened_reader_error(source.contains("meshes/tiny/probe.nif"));
    require_unopened_reader_error(source.extract("meshes/tiny/probe.nif", sink));
    require_unopened_reader_error(source.extract_bytes("meshes/tiny/probe.nif"));
    require_unopened_reader_error(
        source.extract_entries(std::span<const libbsa::bulk_extract_request>{}, sink_factory,
                               libbsa::bulk_extract_options{.worker_count = 0U}));
}
