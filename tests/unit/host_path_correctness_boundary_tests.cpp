#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <texture/dds_layout.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

constexpr std::string_view non_ascii_path_token = "libbsa-Ångström-日本語";
constexpr std::wstring_view non_ascii_path_token_wide = L"libbsa-Ångström-日本語";

std::filesystem::path project_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::filesystem::path suite_source_path() {
    return project_root() / "tests" / "unit" / "host_path_correctness_boundary_tests.cpp";
}

std::filesystem::path tests_cmake_path() { return project_root() / "tests" / "CMakeLists.txt"; }

std::filesystem::path generated_archive_dir() {
    return project_root() / "tests" / "fixtures" / "generated" / "archives";
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.is_open());
    return std::string{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

bool contains_text(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.is_open());
    return nlohmann::json::parse(input);
}

std::vector<std::byte> bytes_from_hex(std::string_view hex) {
    REQUIRE(hex.size() % 2U == 0U);
    std::vector<std::byte> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t index = 0; index < hex.size(); index += 2U) {
        bytes.push_back(
            static_cast<std::byte>(std::stoul(std::string{hex.substr(index, 2U)}, nullptr, 16)));
    }
    return bytes;
}

class temporary_directory_cleanup final {
   public:
    /// Owns best-effort cleanup for the copied archive root so failed assertions
    /// do not leave non-ASCII paths behind.
    explicit temporary_directory_cleanup(std::filesystem::path root) : root_{std::move(root)} {}

    ~temporary_directory_cleanup() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

   private:
    std::filesystem::path root_;
};

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
};

class capturing_sink final : public libbsa::payload_sink {
   public:
    explicit capturing_sink(std::shared_ptr<sink_capture> capture) : capture_{std::move(capture)} {}

    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        capture_->bytes.insert(capture_->bytes.end(), bytes.begin(), bytes.end());
        return bytes.size();
    }

   private:
    std::shared_ptr<sink_capture> capture_;
};

struct capture_report {
    std::map<std::string, std::size_t> create_count_by_path;
    std::map<std::string, std::vector<std::vector<std::byte>>> sink_bytes_by_path;
};

class recording_sink_factory final : public libbsa::bulk_extract_sink_factory {
   public:
    libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(
        std::string_view path, const libbsa::entry_metadata&) override {
        auto capture = std::make_shared<sink_capture>();
        const auto key = std::string{path};
        {
            std::lock_guard lock{mutex_};
            ++create_count_by_path_[key];
            captures_[key].push_back(capture);
        }
        return std::unique_ptr<libbsa::payload_sink>{new capturing_sink{std::move(capture)}};
    }

    [[nodiscard]] capture_report report() const {
        std::lock_guard lock{mutex_};
        capture_report report;
        report.create_count_by_path = create_count_by_path_;
        for (const auto& [path, captures] : captures_) {
            auto& sink_bytes = report.sink_bytes_by_path[path];
            sink_bytes.reserve(captures.size());
            for (const auto& capture : captures) {
                sink_bytes.push_back(capture->bytes);
            }
        }
        return report;
    }

   private:
    mutable std::mutex mutex_;
    std::map<std::string, std::size_t> create_count_by_path_;
    std::map<std::string, std::vector<std::shared_ptr<sink_capture>>> captures_;
};

struct representative_archive_case {
    std::string_view archive_file;
    std::string_view manifest_file;
    libbsa::archive_type expected_type;
    libbsa::archive_variant expected_variant;
};

const auto& representative_archive_cases() {
    static const std::array cases{
        representative_archive_case{"tes3_success.bsa", "tes3_success_manifest.json",
                                    libbsa::archive_type::bsa, libbsa::archive_variant::tes3},
        representative_archive_case{"tes4_v103.bsa", "tes4_v103_manifest.json",
                                    libbsa::archive_type::bsa, libbsa::archive_variant::tes4},
        representative_archive_case{"tes4_v104.bsa", "tes4_v104_manifest.json",
                                    libbsa::archive_type::bsa, libbsa::archive_variant::tes4},
        representative_archive_case{"tes4_v105.bsa", "tes4_v105_manifest.json",
                                    libbsa::archive_type::bsa, libbsa::archive_variant::tes4},
        representative_archive_case{"ba2_gnrl_fo4.ba2", "ba2_gnrl_fo4_manifest.json",
                                    libbsa::archive_type::ba2, libbsa::archive_variant::fallout4},
        representative_archive_case{"ba2_dx10_fo4.ba2", "ba2_dx10_fo4_manifest.json",
                                    libbsa::archive_type::ba2, libbsa::archive_variant::fallout4},
        representative_archive_case{"ba2_gnrl_sfv3.ba2", "ba2_gnrl_sfv3_manifest.json",
                                    libbsa::archive_type::ba2, libbsa::archive_variant::starfield}};
    return cases;
}

std::filesystem::path unique_non_ascii_root() {
    static std::atomic_uint64_t counter{0};
    return std::filesystem::temp_directory_path() /
           std::filesystem::path{std::wstring{non_ascii_path_token_wide} + L"-host-path-" +
                                 std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed))};
}

std::filesystem::path copied_archive_path(const representative_archive_case& archive_case,
                                          const std::filesystem::path& root) {
    const auto source = generated_archive_dir() / std::string{archive_case.archive_file};
    const auto renamed = std::filesystem::path{source.stem().wstring() + L"-" +
                                               std::wstring{non_ascii_path_token_wide} +
                                               source.extension().wstring()};
    return root / renamed;
}

std::filesystem::path copy_archive_under_test(const representative_archive_case& archive_case,
                                              const std::filesystem::path& root) {
    std::filesystem::create_directories(root);
    const auto source = generated_archive_dir() / std::string{archive_case.archive_file};
    const auto copied = copied_archive_path(archive_case, root);
    std::filesystem::copy_file(source, copied, std::filesystem::copy_options::overwrite_existing);
    return copied;
}

const nlohmann::json& canonical_entry_from_manifest(const nlohmann::json& manifest) {
    const auto& entries = manifest.at("entries");
    REQUIRE_FALSE(entries.empty());
    return entries.front();
}

std::string utf8_string_from_path(const std::filesystem::path& path) {
    // The public reader API still takes UTF-8 text, so the suite must hand it an
    // explicit UTF-8 string rather than relying on locale-sensitive narrow
    // conversions from a Windows path that already contains non-ASCII segments.
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

std::vector<std::byte> expected_dds_bytes_from_manifest_entry(const nlohmann::json& entry) {
    const libbsa::texture::dds_texture_layout layout{
        .width = entry.at("width").get<std::uint32_t>(),
        .height = entry.at("height").get<std::uint32_t>(),
        .mip_count = entry.at("num_mips").get<std::uint32_t>(),
        .dxgi_format = entry.at("dxgi_format").get<std::uint32_t>(),
        .array_size = entry.at("array_size").get<std::uint32_t>(),
        .is_cubemap = entry.at("is_cubemap").get<bool>()};
    auto header = libbsa::texture::build_dds_dxt10_header(layout);
    REQUIRE(header.has_value());

    auto expected_bytes = header.value();
    const auto payload =
        bytes_from_hex(entry.at("expected_dds_payload").at("bytes_hex").get<std::string>());
    expected_bytes.insert(expected_bytes.end(), payload.begin(), payload.end());
    return expected_bytes;
}

std::vector<std::byte> expected_bytes_from_manifest_entry(const nlohmann::json& entry) {
    if (entry.contains("expected_dds_payload")) {
        return expected_dds_bytes_from_manifest_entry(entry);
    }
    return bytes_from_hex(entry.at("expected").at("bytes_hex").get<std::string>());
}

std::vector<std::string> sorted_manifest_paths(const nlohmann::json& manifest) {
    std::vector<std::string> paths;
    for (const auto& entry : manifest.at("entries")) {
        paths.push_back(entry.at("path").get<std::string>());
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

void require_canonical_extraction_matches_manifest(
    const representative_archive_case& archive_case) {
    const auto manifest_path = generated_archive_dir() / std::string{archive_case.manifest_file};
    const auto manifest = read_json_file(manifest_path);
    const auto& canonical_entry = canonical_entry_from_manifest(manifest);
    const auto root = unique_non_ascii_root();
    temporary_directory_cleanup cleanup{root};
    const auto copied_archive = copy_archive_under_test(archive_case, root);
    const auto host_path = utf8_string_from_path(copied_archive);
    const auto expected_bytes = expected_bytes_from_manifest_entry(canonical_entry);

    auto opened = libbsa::archive_reader::open(host_path);
    REQUIRE(opened.has_value());
    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == archive_case.expected_type);
    CHECK(metadata.value().variant == archive_case.expected_variant);

    libbsa::validation_options options;
    options.validate_entry_extractability = true;
    auto validated = libbsa::validate_archive(host_path, options);
    REQUIRE(validated.has_value());
    CHECK(validated.value().valid);
    CHECK(validated.value().is_valid());
    CHECK(validated.value().errors.empty());
    REQUIRE(validated.value().metadata.has_value());
    CHECK(validated.value().metadata->type == archive_case.expected_type);
    CHECK(validated.value().metadata->variant == archive_case.expected_variant);

    collecting_sink sink;
    const auto canonical_path = canonical_entry.at("path").get<std::string>();
    auto extracted = opened.value().extract(canonical_path, sink);
    REQUIRE(extracted.has_value());
    CHECK(sink.bytes() == expected_bytes);

    auto bytes = opened.value().extract_bytes(canonical_path);
    REQUIRE(bytes.has_value());
    CHECK(bytes.value() == expected_bytes);
}

}  // namespace

TEST_CASE(
    "host_path_correctness_boundary suite registration is wired into "
    "libbsa_tests",
    "[unit][host_path_correctness_boundary]") {
    const auto tests_cmake = read_text_file(tests_cmake_path());

    REQUIRE(contains_text(tests_cmake, "unit/host_path_correctness_boundary_tests.cpp"));
}

TEST_CASE(
    "host_path_correctness_boundary suite source carries the locked "
    "non-ASCII token and smoke selector",
    "[unit][host_path_correctness_boundary][host_path_correctness_"
    "boundary_smoke]") {
    REQUIRE(non_ascii_path_token == "libbsa-Ångström-日本語");
}

TEST_CASE(
    "host_path_correctness_boundary smoke setup uses the locked "
    "non-ASCII directory and filename without copying manifests",
    "[unit][fixture][host_path_correctness_boundary][host_path_"
    "correctness_boundary_smoke]") {
    const auto& archive_case = representative_archive_cases().at(1);
    const auto manifest_path = generated_archive_dir() / std::string{archive_case.manifest_file};
    const auto manifest = read_json_file(manifest_path);
    const auto& canonical_entry = canonical_entry_from_manifest(manifest);
    const auto root = unique_non_ascii_root();
    temporary_directory_cleanup cleanup{root};
    const auto expected_copied_archive = copied_archive_path(archive_case, root);
    const auto copied_archive = copy_archive_under_test(archive_case, root);

    REQUIRE(copied_archive.parent_path() == root);
    REQUIRE(copied_archive == expected_copied_archive);
    REQUIRE(std::filesystem::exists(copied_archive));
    REQUIRE(manifest_path.native().find(root.native()) == std::filesystem::path::string_type::npos);
    REQUIRE(canonical_entry.at("path").get<std::string>().empty() == false);
    REQUIRE_FALSE(
        bytes_from_hex(canonical_entry.at("expected").at("bytes_hex").get<std::string>()).empty());
}

TEST_CASE(
    "host_path_correctness_boundary representative archives open "
    "validate and extract from non-ASCII paths",
    "[unit][fixture][host_path_correctness_boundary]") {
    REQUIRE(representative_archive_cases().size() == 7U);

    for (const auto& archive_case : representative_archive_cases()) {
        INFO(archive_case.archive_file);
        require_canonical_extraction_matches_manifest(archive_case);
    }
}

TEST_CASE(
    "host_path_correctness_boundary non-ASCII host path exercises Archive Entry Catalog and "
    "extraction surfaces",
    "[unit][fixture][host_path_correctness_boundary][archive_entry_catalog]"
    "[reader_extraction_dispatch]") {
    for (const auto& archive_case : representative_archive_cases()) {
        INFO(archive_case.archive_file);
        const auto manifest =
            read_json_file(generated_archive_dir() / std::string{archive_case.manifest_file});
        const auto& canonical_entry = canonical_entry_from_manifest(manifest);
        const auto canonical_path = canonical_entry.at("path").get<std::string>();
        const auto expected_bytes = expected_bytes_from_manifest_entry(canonical_entry);
        const auto root = unique_non_ascii_root();
        temporary_directory_cleanup cleanup{root};
        const auto copied_archive = copy_archive_under_test(archive_case, root);

        auto opened = libbsa::archive_reader::open(utf8_string_from_path(copied_archive));
        REQUIRE(opened.has_value());
        const auto& reader = opened.value();

        auto entries = reader.entries();
        REQUIRE(entries.has_value());
        REQUIRE(entries.value().size() == manifest.at("entries").size());
        const auto expected_paths = sorted_manifest_paths(manifest);
        for (std::size_t index = 0; index < entries.value().size(); ++index) {
            CHECK(entries.value().at(index).path == expected_paths.at(index));
        }

        auto found = reader.find(canonical_path);
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        CHECK(found.value()->path == canonical_path);

        auto contains = reader.contains(canonical_path);
        REQUIRE(contains.has_value());
        CHECK(contains.value());

        recording_sink_factory sink_factory;
        std::vector requests{libbsa::bulk_extract_request{.path = canonical_path}};
        auto extracted_entries = reader.extract_entries(requests, sink_factory);
        REQUIRE(extracted_entries.has_value());
        REQUIRE(extracted_entries.value().size() == requests.size());
        REQUIRE(extracted_entries.value().front().succeeded());
        REQUIRE(extracted_entries.value().front().entry.has_value());
        CHECK(extracted_entries.value().front().entry->path == canonical_path);

        const auto report = sink_factory.report();
        REQUIRE(report.create_count_by_path.at(canonical_path) == 1U);
        const auto& sink_bytes = report.sink_bytes_by_path.at(canonical_path);
        REQUIRE(sink_bytes.size() == 1U);
        CHECK(sink_bytes.front() == expected_bytes);
    }
}

TEST_CASE("host_path_correctness_boundary stays public-API-only and phase-scoped",
          "[unit][host_path_correctness_boundary][host_path_correctness_boundary_"
          "smoke]") {
    const auto suite_source = read_text_file(suite_source_path());
    const auto open_call = std::string{
        "archive_reader::"
        "open(host_path)"};
    const auto validate_call = std::string{
        "validate_"
        "archive(host_path, options)"};
    const auto extract_bytes_call = std::string{
        "extract_"
        "bytes(canonical_path)"};
    const auto writer_publish_call = std::string{
        "write_"
        "to("};
    const auto writer_add_file_call = std::string{
        "add_"
        "file("};

    REQUIRE(contains_text(suite_source, open_call));
    REQUIRE(contains_text(suite_source, validate_call));
    REQUIRE(contains_text(suite_source, extract_bytes_call));

    REQUIRE_FALSE(contains_text(suite_source, writer_publish_call));
    REQUIRE_FALSE(contains_text(suite_source, writer_add_file_call));
}
