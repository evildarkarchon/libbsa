#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" /
           "archives";
}

std::filesystem::path generated_archive_path(std::string_view filename) {
    return generated_archive_dir() / std::string{filename};
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());
    return nlohmann::json::parse(stream);
}

std::vector<std::byte> bytes_from_hex(std::string_view hex) {
    REQUIRE(hex.size() % 2U == 0U);
    std::vector<std::byte> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t offset = 0; offset < hex.size(); offset += 2U) {
        const auto pair = std::string{hex.substr(offset, 2U)};
        bytes.push_back(static_cast<std::byte>(std::stoul(pair, nullptr, 16)));
    }
    return bytes;
}

struct archive_format_fixture {
    std::string name;
    std::string archive_filename;
    std::string manifest_filename;
    std::string expected_extract_path;
    std::string expected_missing_path;
    std::string invalid_archive_path;
};

std::vector<archive_format_fixture> archive_format_fixtures() {
    return {{"tes3_bsa", "tes3_success.bsa", "tes3_success_manifest.json", "meshes/tiny/probe.nif",
             "valid/missing/path.txt", "folder//file.txt"},
            {"tes4_bsa", "tes4_v103.bsa", "tes4_v103_manifest.json", "meshes/tiny/rawmesh.nif",
             "valid/missing/path.txt", "folder//file.txt"},
            {"ba2_gnrl_fo4", "ba2_gnrl_fo4.ba2", "ba2_gnrl_fo4_manifest.json",
             "meshes/mixedcase/probe.nif", "valid/missing/path.txt", "folder//file.txt"},
            {"ba2_gnrl_starfield_v2", "ba2_gnrl_sfv2.ba2", "ba2_gnrl_sfv2_manifest.json",
             "data/scripts/rawscript.pex", "valid/missing/path.txt", "folder//file.txt"},
            {"ba2_gnrl_starfield_v3", "ba2_gnrl_sfv3.ba2", "ba2_gnrl_sfv3_manifest.json",
             "geometries/packed/block.mesh", "valid/missing/path.txt", "folder//file.txt"},
            {"ba2_dx10_fo4", "ba2_dx10_fo4.ba2", "ba2_dx10_fo4_manifest.json",
             "textures/generated/fo4raw.dds", "valid/missing/path.txt", "folder//file.txt"},
            {"ba2_dx10_starfield_v3", "ba2_dx10_sfv3.ba2", "ba2_dx10_sfv3_manifest.json",
             "textures/generated/sfrawlz4.dds", "valid/missing/path.txt", "folder//file.txt"}};
}

const nlohmann::json& find_manifest_entry(const nlohmann::json& manifest, std::string_view path) {
    const auto found = std::find_if(
        manifest.at("entries").begin(), manifest.at("entries").end(),
        [&](const auto& entry) { return entry.at("path").template get<std::string>() == path; });
    REQUIRE(found != manifest.at("entries").end());
    return *found;
}

std::vector<std::byte> expected_payload_bytes(const nlohmann::json& manifest_entry) {
    if (manifest_entry.contains("expected") &&
        manifest_entry.at("expected").contains("bytes_hex")) {
        return bytes_from_hex(manifest_entry.at("expected").at("bytes_hex").get<std::string>());
    }
    return bytes_from_hex(
        manifest_entry.at("expected_dds_payload").at("bytes_hex").get<std::string>());
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

struct sink_capture {
    std::vector<std::byte> bytes;
    bool destroyed = false;
};

class capturing_sink final : public libbsa::payload_sink {
   public:
    explicit capturing_sink(std::shared_ptr<sink_capture> capture, bool fail_writes = false)
        : capture_(std::move(capture)), fail_writes_(fail_writes) {}

    /// Records ownership release so the factory can verify `finish` observes a destroyed sink.
    ~capturing_sink() override { capture_->destroyed = true; }

    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        if (fail_writes_) {
            return libbsa::error{libbsa::error_code::io_error, "sink rejected payload bytes"};
        }
        capture_->bytes.insert(capture_->bytes.end(), bytes.begin(), bytes.end());
        return bytes.size();
    }

   private:
    std::shared_ptr<sink_capture> capture_;
    bool fail_writes_;
};

struct path_capture_report {
    std::size_t create_count = 0U;
    std::size_t finish_count = 0U;
    std::vector<bool> finish_success;
    std::vector<bool> destroyed_before_finish;
    std::vector<std::vector<std::byte>> sink_bytes;
};

struct capture_report {
    std::map<std::string, path_capture_report> paths;
};

struct path_capture_state {
    path_capture_report report;
    std::vector<std::shared_ptr<sink_capture>> captures;
};

class recording_sink_factory final : public libbsa::bulk_extract_sink_factory {
   public:
    explicit recording_sink_factory(bool fail_writes = false) : fail_writes_(fail_writes) {}

    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(
        std::string_view path, const libbsa::entry_metadata&) override {
        auto capture = std::make_shared<sink_capture>();
        const auto key = std::string{path};
        {
            std::lock_guard lock{mutex_};
            auto& state = path_states_[key];
            ++state.report.create_count;
            state.captures.push_back(capture);
        }
        return std::unique_ptr<libbsa::payload_sink>{
            new capturing_sink{std::move(capture), fail_writes_}};
    }

    /// Records the finish result and whether the matching sink was destroyed first.
    libbsa::result<void> finish(std::string_view path, bool succeeded) override {
        const auto key = std::string{path};
        std::lock_guard lock{mutex_};
        auto& state = path_states_.at(key);
        const auto capture = state.captures.back();
        ++state.report.finish_count;
        state.report.finish_success.push_back(succeeded);
        state.report.destroyed_before_finish.push_back(capture->destroyed);
        return {};
    }

    /// Snapshots per-path sink lifecycle observations and captured payload bytes.
    [[nodiscard]] capture_report report() const {
        std::lock_guard lock{mutex_};
        capture_report report;
        for (const auto& [path, state] : path_states_) {
            auto path_report = state.report;
            path_report.sink_bytes.reserve(state.captures.size());
            for (const auto& capture : state.captures) {
                path_report.sink_bytes.push_back(capture->bytes);
            }
            report.paths.emplace(path, std::move(path_report));
        }
        return report;
    }

   private:
    bool fail_writes_;
    mutable std::mutex mutex_;
    std::map<std::string, path_capture_state> path_states_;
};

std::vector<std::string> sorted_manifest_paths(const nlohmann::json& manifest) {
    std::vector<std::string> paths;
    for (const auto& entry : manifest.at("entries")) {
        paths.push_back(entry.at("path").get<std::string>());
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

std::vector<std::string> lookup_variants_for(const nlohmann::json& manifest_entry) {
    if (!manifest_entry.contains("lookup_variants")) {
        return {manifest_entry.at("path").get<std::string>()};
    }

    std::vector<std::string> variants;
    for (const auto& variant : manifest_entry.at("lookup_variants")) {
        variants.push_back(variant.get<std::string>());
    }
    return variants;
}

void require_result_entry_matches_expected(const libbsa::bulk_extract_entry_result& result,
                                           const nlohmann::json& expected) {
    REQUIRE(result.entry.has_value());
    REQUIRE(result.entry->path == expected.at("path").get<std::string>());
    if (expected.contains("raw_size")) {
        REQUIRE(result.entry->raw_size == expected.at("raw_size").get<std::uint64_t>());
    }
    if (expected.contains("stored_size")) {
        REQUIRE(result.entry->stored_size == expected.at("stored_size").get<std::uint64_t>());
    }
}

}  // namespace

TEST_CASE("archive reader catalog and extraction dispatch preserve behavior across formats",
          "[unit][fixture][archive_entry_catalog][reader_extraction_dispatch]") {
    for (const auto& fixture : archive_format_fixtures()) {
        INFO("fixture: " << fixture.name);
        const auto manifest = read_json_file(generated_archive_path(fixture.manifest_filename));
        const auto& expected_entry = find_manifest_entry(manifest, fixture.expected_extract_path);
        const auto expected_bytes = expected_payload_bytes(expected_entry);

        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
        REQUIRE(opened.has_value());
        const auto& reader = opened.value();

        auto entries = reader.entries();
        REQUIRE(entries.has_value());
        REQUIRE(entries.value().size() == manifest.at("entries").size());

        const auto expected_paths = sorted_manifest_paths(manifest);
        for (std::size_t index = 0; index < entries.value().size(); ++index) {
            CHECK(entries.value().at(index).path == expected_paths.at(index));
        }

        const auto first_path = entries.value().front().path;
        const auto first_original_path = entries.value().front().original_path;
        const auto first_raw_size = entries.value().front().raw_size;
        entries.value().front().path.clear();
        entries.value().front().original_path.clear();
        ++entries.value().front().raw_size;

        auto repeated_entries = reader.entries();
        REQUIRE(repeated_entries.has_value());
        REQUIRE(repeated_entries.value().front().path == first_path);
        REQUIRE(repeated_entries.value().front().original_path == first_original_path);
        REQUIRE(repeated_entries.value().front().raw_size == first_raw_size);

        for (const auto& variant : lookup_variants_for(expected_entry)) {
            auto found = reader.find(variant);
            REQUIRE(found.has_value());
            REQUIRE(found.value().has_value());
            REQUIRE(found.value()->path == expected_entry.at("path").get<std::string>());

            auto contains = reader.contains(variant);
            REQUIRE(contains.has_value());
            REQUIRE(contains.value());
        }

        auto missing_find = reader.find(fixture.expected_missing_path);
        REQUIRE(missing_find.has_value());
        REQUIRE_FALSE(missing_find.value().has_value());

        auto missing_contains = reader.contains(fixture.expected_missing_path);
        REQUIRE(missing_contains.has_value());
        REQUIRE_FALSE(missing_contains.value());

        auto invalid_find = reader.find(fixture.invalid_archive_path);
        REQUIRE_FALSE(invalid_find.has_value());
        REQUIRE(invalid_find.error().code == libbsa::error_code::invalid_argument);

        auto invalid_contains = reader.contains(fixture.invalid_archive_path);
        REQUIRE_FALSE(invalid_contains.has_value());
        REQUIRE(invalid_contains.error().code == libbsa::error_code::invalid_argument);

        collecting_sink sink;
        auto extracted = reader.extract(fixture.expected_extract_path, sink);
        REQUIRE(extracted.has_value());

        auto extracted_bytes = reader.extract_bytes(fixture.expected_extract_path);
        REQUIRE(extracted_bytes.has_value());

        if (expected_entry.contains("expected_dds_payload")) {
            REQUIRE(sink.bytes().size() > expected_bytes.size());
            REQUIRE(extracted_bytes.value() == sink.bytes());
            REQUIRE(std::vector<std::byte>{
                        sink.bytes().end() - static_cast<std::ptrdiff_t>(expected_bytes.size()),
                        sink.bytes().end()} == expected_bytes);
        } else {
            REQUIRE(sink.bytes() == expected_bytes);
            REQUIRE(extracted_bytes.value() == expected_bytes);
        }

        collecting_sink missing_sink;
        auto missing_extract = reader.extract(fixture.expected_missing_path, missing_sink);
        REQUIRE_FALSE(missing_extract.has_value());
        REQUIRE(missing_extract.error().code == libbsa::error_code::not_found);

        auto missing_extract_bytes = reader.extract_bytes(fixture.expected_missing_path);
        REQUIRE_FALSE(missing_extract_bytes.has_value());
        REQUIRE(missing_extract_bytes.error().code == libbsa::error_code::not_found);

        collecting_sink invalid_sink;
        auto invalid_extract = reader.extract(fixture.invalid_archive_path, invalid_sink);
        REQUIRE_FALSE(invalid_extract.has_value());
        REQUIRE(invalid_extract.error().code == libbsa::error_code::invalid_argument);

        auto invalid_extract_bytes = reader.extract_bytes(fixture.invalid_archive_path);
        REQUIRE_FALSE(invalid_extract_bytes.has_value());
        REQUIRE(invalid_extract_bytes.error().code == libbsa::error_code::invalid_argument);

        recording_sink_factory sink_factory;
        std::vector requests{libbsa::bulk_extract_request{.path = fixture.expected_extract_path},
                             libbsa::bulk_extract_request{.path = fixture.expected_extract_path},
                             libbsa::bulk_extract_request{.path = fixture.expected_missing_path},
                             libbsa::bulk_extract_request{.path = fixture.invalid_archive_path}};

        auto extracted_entries = reader.extract_entries(requests, sink_factory);
        REQUIRE(extracted_entries.has_value());
        REQUIRE(extracted_entries.value().size() == requests.size());

        const auto report = sink_factory.report();
        const auto& path_report = report.paths.at(fixture.expected_extract_path);
        REQUIRE(path_report.create_count == 1U);
        REQUIRE(path_report.finish_count == 1U);
        REQUIRE(path_report.finish_success == std::vector<bool>{true});
        REQUIRE(path_report.destroyed_before_finish == std::vector<bool>{true});
        REQUIRE(report.paths.contains(fixture.expected_missing_path) == false);
        REQUIRE(report.paths.contains(fixture.invalid_archive_path) == false);

        for (const auto result_index : {0U, 1U}) {
            const auto& result = extracted_entries.value().at(result_index);
            REQUIRE(result.succeeded());
            require_result_entry_matches_expected(result, expected_entry);
        }

        REQUIRE(extracted_entries.value().at(2).failure.has_value());
        REQUIRE(extracted_entries.value().at(2).failure->code == libbsa::error_code::not_found);
        REQUIRE_FALSE(extracted_entries.value().at(2).entry.has_value());

        REQUIRE(extracted_entries.value().at(3).failure.has_value());
        REQUIRE(extracted_entries.value().at(3).failure->code ==
                libbsa::error_code::invalid_argument);
        REQUIRE_FALSE(extracted_entries.value().at(3).entry.has_value());

        const auto& sink_bytes = path_report.sink_bytes;
        REQUIRE(sink_bytes.size() == 1U);
        if (expected_entry.contains("expected_dds_payload")) {
            REQUIRE(sink_bytes.front().size() > expected_bytes.size());
            REQUIRE(std::vector<std::byte>{sink_bytes.front().end() -
                                               static_cast<std::ptrdiff_t>(expected_bytes.size()),
                                           sink_bytes.front().end()} == expected_bytes);
        } else {
            REQUIRE(sink_bytes.front() == expected_bytes);
        }

        recording_sink_factory failing_sink_factory{true};
        const std::array failing_requests{
            libbsa::bulk_extract_request{.path = fixture.expected_extract_path}};
        auto failed_entries = reader.extract_entries(failing_requests, failing_sink_factory);
        REQUIRE(failed_entries.has_value());
        REQUIRE(failed_entries.value().size() == 1U);
        REQUIRE(failed_entries.value().front().failure.has_value());
        REQUIRE(failed_entries.value().front().failure->code == libbsa::error_code::io_error);

        const auto failure_report = failing_sink_factory.report();
        const auto& failed_path_report = failure_report.paths.at(fixture.expected_extract_path);
        REQUIRE(failed_path_report.create_count == 1U);
        REQUIRE(failed_path_report.finish_count == 1U);
        REQUIRE(failed_path_report.finish_success == std::vector<bool>{false});
        REQUIRE(failed_path_report.destroyed_before_finish == std::vector<bool>{true});
    }
}
