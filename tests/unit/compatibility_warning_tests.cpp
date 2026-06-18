#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

std::filesystem::path warning_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_compatibility_warning_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::filesystem::path unique_output_path(std::string_view stem, std::string_view extension) {
    static std::atomic_uint64_t counter{0};
    auto path =
        warning_test_dir() /
        (std::string{stem} + "-" + std::to_string(counter.fetch_add(1, std::memory_order_relaxed)) +
         std::string{extension});
    std::filesystem::remove(path);
    return path;
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::vector<std::byte> repeated_text_bytes(std::string_view text, std::size_t repetitions) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size() * repetitions);
    for (std::size_t index = 0; index < repetitions; ++index) {
        const auto chunk = bytes_from_text(text);
        bytes.insert(bytes.end(), chunk.begin(), chunk.end());
    }
    return bytes;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::string trim_copy(std::string value) {
    const auto first = std::find_if(value.begin(), value.end(),
                                    [](unsigned char ch) { return !std::isspace(ch); });
    const auto last = std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
                          return !std::isspace(ch);
                      }).base();

    if (first >= last) {
        return {};
    }
    return std::string{first, last};
}

std::vector<std::string> compatibility_warning_codes_from_public_header() {
    const auto header = read_text_file(source_root() / "include/libbsa/validation.hpp");
    const auto enum_name = std::string{"enum class compatibility_warning_code"};
    const auto enum_start = header.find(enum_name);
    REQUIRE(enum_start != std::string::npos);

    const auto body_start = header.find('{', enum_start);
    REQUIRE(body_start != std::string::npos);
    const auto body_end = header.find("};", body_start);
    REQUIRE(body_end != std::string::npos);

    std::vector<std::string> codes;
    std::istringstream lines{header.substr(body_start + 1, body_end - body_start - 1)};
    std::string line;
    while (std::getline(lines, line)) {
        if (const auto comment = line.find("//"); comment != std::string::npos) {
            line.erase(comment);
        }
        if (const auto comma = line.find(','); comma != std::string::npos) {
            line.erase(comma);
        }

        auto code = trim_copy(line);
        if (!code.empty()) {
            codes.push_back(std::move(code));
        }
    }
    return codes;
}

std::string warning_code_name(libbsa::compatibility_warning_code code) {
    switch (code) {
        case libbsa::compatibility_warning_code::compressed_sound_payload:
            return "compressed_sound_payload";
        case libbsa::compatibility_warning_code::bsa_embedded_name_compatibility_risk:
            return "bsa_embedded_name_compatibility_risk";
        case libbsa::compatibility_warning_code::target_family_mismatch:
            return "target_family_mismatch";
    }

    FAIL("unknown compatibility_warning_code");
    return {};
}

const libbsa::compatibility_warning& require_warning(
    const libbsa::validation_report& report, libbsa::compatibility_warning_code code,
    libbsa::compatibility_warning_severity severity, bool has_archive_path) {
    for (const auto& warning : report.warnings) {
        if (warning.code == code) {
            CHECK(warning.severity == severity);
            CHECK(warning.archive_path.has_value() == has_archive_path);
            return warning;
        }
    }
    FAIL("missing expected compatibility warning");
}

libbsa::validation_report require_validated_report(const std::filesystem::path& archive,
                                                   libbsa::validation_options options = {}) {
    auto validated = libbsa::validate_archive(archive.string(), options);
    REQUIRE(validated.has_value());
    CHECK(validated.value().is_valid());
    CHECK(validated.value().errors.empty());
    return validated.value();
}

std::filesystem::path write_raw_ba2_archive() {
    const auto output = unique_output_path("target-family-mismatch", ".ba2");
    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    options.overwrite_existing = true;
    libbsa::ba2_gnrl_writer writer{libbsa::ba2_gnrl_target::fallout4, options};
    REQUIRE(
        writer
            .add_bytes("Meshes/Mismatch/Probe.nif", bytes_from_text("ba2 target mismatch payload"))
            .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());
    return output;
}

std::filesystem::path write_embedded_name_bsa_archive() {
    const auto output = unique_output_path("embedded-name-risk", ".bsa");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.embed_file_names = true;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer
                .add_bytes("Meshes/Embedded/Model.nif",
                           bytes_from_text("embedded name compatibility payload"))
                .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());
    return output;
}

std::filesystem::path write_compressed_sound_bsa_archive() {
    const auto output = unique_output_path("compressed-sound-risk", ".bsa");
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_compressed;
    options.overwrite_existing = true;
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::fallout3, options};
    REQUIRE(writer.add_bytes("sound/fx/alert.wav", repeated_text_bytes("alert sound payload ", 16U))
                .has_value());
    REQUIRE(writer.write_to(output.string()).has_value());
    return output;
}

}  // namespace

TEST_CASE("compatibility_warning reports BA2 target family mismatch",
          "[unit][compat][compatibility_warning]") {
    libbsa::validation_options options;
    options.expected_type = libbsa::archive_type::bsa;

    const auto report = require_validated_report(write_raw_ba2_archive(), options);

    require_warning(report, libbsa::compatibility_warning_code::target_family_mismatch,
                    libbsa::compatibility_warning_severity::risky, false);
}

TEST_CASE("compatibility_warning reports BSA embedded name compatibility risk",
          "[unit][compat][compatibility_warning]") {
    const auto report = require_validated_report(write_embedded_name_bsa_archive());

    require_warning(report,
                    libbsa::compatibility_warning_code::bsa_embedded_name_compatibility_risk,
                    libbsa::compatibility_warning_severity::risky, true);
}

TEST_CASE("compatibility_warning reports compressed sound payloads",
          "[unit][compat][compatibility_warning]") {
    const auto report = require_validated_report(write_compressed_sound_bsa_archive());

    require_warning(report, libbsa::compatibility_warning_code::compressed_sound_payload,
                    libbsa::compatibility_warning_severity::advisory, true);
}

TEST_CASE("compatibility_warning behavior covers every public warning code",
          "[unit][compat][compatibility_warning][validation_policy]") {
    libbsa::validation_options mismatch_options;
    mismatch_options.expected_type = libbsa::archive_type::bsa;

    const std::array reports{
        require_validated_report(write_raw_ba2_archive(), mismatch_options),
        require_validated_report(write_embedded_name_bsa_archive()),
        require_validated_report(write_compressed_sound_bsa_archive()),
    };

    std::vector<std::string> observed_codes;
    for (const auto& report : reports) {
        for (const auto& warning : report.warnings) {
            observed_codes.push_back(warning_code_name(warning.code));
        }
    }

    const auto public_codes = compatibility_warning_codes_from_public_header();
    REQUIRE_FALSE(public_codes.empty());
    for (const auto& public_code : public_codes) {
        INFO("missing behavior-backed warning code: " << public_code);
        REQUIRE(std::find(observed_codes.begin(), observed_codes.end(), public_code) !=
                observed_codes.end());
    }
}
