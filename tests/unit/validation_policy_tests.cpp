#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

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

struct verification_lane_contract {
    std::string_view preset_name;
    std::string_view role_name;
    bool package_proof_lane;
};

// Keep the supported lane matrix in one place so future role or lane changes
// must update the shared contract before the per-surface assertions can pass
// again.
constexpr std::array<verification_lane_contract, 5> verification_matrix_contract() {
    return {{
        {"windows-msvc-debug-static", "debug quick path", false},
        {"windows-msvc-debug-shared", "debug inner-loop lane", false},
        {"windows-msvc-release-static", "release package-proof lane", true},
        {"windows-msvc-release-shared", "release package-proof lane", true},
        {"windows-msvc-asan-static", "MSVC AddressSanitizer hardening lane", false},
    }};
}

constexpr std::array<std::string_view, 2> release_package_proof_tests() {
    return {{"package_consumer_smoke", "package_consumer_runtime_dll_copy"}};
}

constexpr std::array<std::string_view, 2> supported_matrix_exclusions() {
    return {{"Windows-only", "requires-game-fixture"}};
}

/// Parse a small YAML block by indentation so workflow topology tests can stay
/// dependency-light.
std::optional<std::string> yaml_block(std::string_view text, std::string_view header,
                                      std::size_t indent_spaces) {
    const auto header_token = std::string(indent_spaces, ' ') + std::string{header} + ":";
    std::istringstream lines{std::string{text}};
    std::string line;
    std::ostringstream block;
    bool in_block = false;

    while (std::getline(lines, line)) {
        const auto trimmed_line = trim_copy(line);
        if (!in_block) {
            if (trimmed_line == std::string{header} + ":") {
                in_block = true;
                block << line << '\n';
            }
            continue;
        }

        if (line.starts_with(header_token)) {
            block << line << '\n';
            continue;
        }

        if (line.size() > indent_spaces && line.starts_with(std::string(indent_spaces, ' ')) &&
            line[indent_spaces] != ' ' && line[indent_spaces] != '-' &&
            trimmed_line.ends_with(':')) {
            break;
        }

        block << line << '\n';
    }

    if (!in_block) {
        return std::nullopt;
    }

    return block.str();
}

std::size_t count_occurrences(std::string_view text, std::string_view needle) {
    std::size_t count = 0;
    std::size_t offset = 0;
    while ((offset = text.find(needle, offset)) != std::string_view::npos) {
        ++count;
        offset += needle.size();
    }
    return count;
}

void require_preset_family(std::string_view presets, const verification_lane_contract& lane) {
    INFO("Checking preset family for " << lane.preset_name << " as the " << lane.role_name);
    REQUIRE(presets.find(lane.preset_name) != std::string_view::npos);
}

void require_lane_names(std::string_view text) {
    for (const auto& lane : verification_matrix_contract()) {
        INFO("Missing supported lane token: " << lane.preset_name);
        REQUIRE(text.find(lane.preset_name) != std::string_view::npos);
    }
}

void require_release_package_proof_comment(std::string_view tests_cmake,
                                           const verification_lane_contract& lane) {
    INFO("Release ownership must be spelled out for " << lane.preset_name);
    REQUIRE(tests_cmake.find(lane.preset_name) != std::string_view::npos);
}

void require_all_tokens(std::string_view text, std::initializer_list<std::string_view> tokens) {
    for (const auto token : tokens) {
        INFO("Missing token: " << token);
        REQUIRE(text.find(token) != std::string_view::npos);
    }
}

}  // namespace

TEST_CASE(
    "validation_policy CI and presets preserve static shared and "
    "TES5Edit build boundaries",
    "[unit][public-api][static_boundary]") {
    const auto root = source_root();
    const auto presets = read_text_file(root / "CMakePresets.json");
    const auto workflow = read_text_file(root / ".github/workflows/ci.yml");

    REQUIRE(presets.find("windows-msvc-debug-static") != std::string::npos);
    REQUIRE(presets.find("windows-msvc-debug-shared") != std::string::npos);
    REQUIRE(presets.find("\"BUILD_SHARED_LIBS\": \"OFF\"") != std::string::npos);
    REQUIRE(presets.find("\"BUILD_SHARED_LIBS\": \"ON\"") != std::string::npos);

    REQUIRE(workflow.find("windows-msvc-debug-static") != std::string::npos);
    REQUIRE(workflow.find("windows-msvc-debug-shared") != std::string::npos);
    REQUIRE(workflow.find("LIBBSA_CMAKE_VERSION: 4.3.2") != std::string::npos);
    REQUIRE(workflow.find("cmake-$cmakeVersion-windows-x86_64.zip") != std::string::npos);
    REQUIRE(workflow.find("https://github.com/Kitware/CMake/releases/download") !=
            std::string::npos);
    REQUIRE(workflow.find("cmake --version") != std::string::npos);
    REQUIRE(workflow.find("cmake --preset ${{ matrix.preset }}") != std::string::npos);
    REQUIRE(workflow.find("cmake --build --preset ${{ matrix.preset }}") != std::string::npos);
    REQUIRE(workflow.find("ctest --preset ${{ matrix.preset }} --output-on-failure") !=
            std::string::npos);
    REQUIRE(workflow.find("git status --short TES5Edit") != std::string::npos);
    REQUIRE(workflow.find("TES5Edit submodule changed during CI") != std::string::npos);
}

TEST_CASE(
    "validation_policy configured build profiles are Windows-only and "
    "documented",
    "[unit][validation_policy][static_boundary][doc_structure]") {
    const auto root = source_root();
    const auto presets = read_text_file(root / "CMakePresets.json");
    const auto workflow = read_text_file(root / ".github/workflows/ci.yml");
    const auto fixture_policy = read_text_file(root / "tests/fixtures/README.md");
    const auto readme = read_text_file(root / "README.md");

    REQUIRE(presets.find("windows-msvc-debug-static") != std::string::npos);
    REQUIRE(presets.find("windows-msvc-debug-shared") != std::string::npos);
    REQUIRE(presets.find("windows-msvc-release-static") != std::string::npos);
    REQUIRE(presets.find("windows-msvc-release-shared") != std::string::npos);
    REQUIRE(presets.find("windows-msvc-asan-static") != std::string::npos);
    REQUIRE(presets.find("linux-clang-asan-ubsan") == std::string::npos);
    REQUIRE(presets.find("-fsanitize=address,undefined") == std::string::npos);

    REQUIRE(workflow.find("windows-msvc-debug-static") != std::string::npos);
    REQUIRE(workflow.find("windows-msvc-debug-shared") != std::string::npos);
    REQUIRE(workflow.find("windows-msvc-release-static") != std::string::npos);
    REQUIRE(workflow.find("windows-msvc-release-shared") != std::string::npos);
    REQUIRE(workflow.find("windows-msvc-asan-static") != std::string::npos);
    REQUIRE(workflow.find("git status --short TES5Edit") != std::string::npos);
    REQUIRE(workflow.find("windows-latest") != std::string::npos);

    REQUIRE(fixture_policy.find("Windows-only") != std::string::npos);
    REQUIRE(fixture_policy.find("linux-clang-asan-ubsan") == std::string::npos);
    REQUIRE(readme.find("Windows-only") != std::string::npos);
}

TEST_CASE("validation_policy verification matrix contract keeps README truthful",
          "[unit][validation_policy][doc_structure]") {
    const auto readme = read_text_file(source_root() / "README.md");

    require_lane_names(readme);
    require_all_tokens(
        readme, {"quick day-to-day path", "Debug inner-loop lanes", "Release package-proof lanes",
                 "MSVC AddressSanitizer hardening lane", "package_consumer_smoke", "Windows-only",
                 "requires-game-fixture", "skipped by default"});
}

TEST_CASE(
    "validation_policy verification matrix contract keeps fixture policy "
    "truthful",
    "[unit][validation_policy][doc_structure]") {
    const auto fixture_policy = read_text_file(source_root() / "tests/fixtures/README.md");

    require_lane_names(fixture_policy);
    require_all_tokens(
        fixture_policy,
        {"Debug inner-loop lanes", "Release package-proof lanes",
         "MSVC AddressSanitizer hardening lane", "package_consumer_smoke", "Windows-only",
         "requires-game-fixture", "skipped by default", "WSL", "extra sanitizer families"});
}

// The verification matrix was previously also asserted against the agent
// instruction file (CLAUDE.md, later AGENTS.md). That coupled the test suite to
// files that configure coding agents rather than to the library or its user-
// facing documentation, and it broke whenever that guidance was reorganized.
// The lane names and lane descriptions it checked are already covered against
// README.md and tests/fixtures/README.md above, so the case was removed rather
// than repointed.

TEST_CASE(
    "validation_policy verification matrix contract requires supported "
    "preset triads",
    "[unit][validation_policy][static_boundary]") {
    const auto presets = read_text_file(source_root() / "CMakePresets.json");

    for (const auto& lane : verification_matrix_contract()) {
        require_preset_family(presets, lane);
    }
}

TEST_CASE(
    "validation_policy verification matrix contract requires checked-in "
    "MSVC ASan wiring",
    "[unit][validation_policy][static_boundary]") {
    const auto root_cmake = read_text_file(source_root() / "CMakeLists.txt");

    require_all_tokens(root_cmake, {"LIBBSA_ENABLE_MSVC_ASAN", "MSVC", "/fsanitize=address"});
}

TEST_CASE(
    "validation_policy verification matrix contract keeps release "
    "package proof inside CTest",
    "[unit][validation_policy][static_boundary]") {
    const auto tests_cmake = read_text_file(source_root() / "tests/CMakeLists.txt");

    for (const auto& lane : verification_matrix_contract()) {
        if (!lane.package_proof_lane) {
            continue;
        }

        require_release_package_proof_comment(tests_cmake, lane);
    }

    for (const auto test_name : release_package_proof_tests()) {
        INFO("Checking release package-proof test registration for " << test_name);
        REQUIRE(tests_cmake.find(std::string{test_name}) != std::string::npos);
    }

    for (const auto exclusion : supported_matrix_exclusions()) {
        INFO("Checking contract exclusion token for " << exclusion);
        REQUIRE(tests_cmake.find(std::string{exclusion}) != std::string::npos);
    }
}

TEST_CASE(
    "validation_policy verification matrix contract keeps the main "
    "workflow matrix truthful",
    "[unit][validation_policy][static_boundary]") {
    const auto workflow = read_text_file(source_root() / ".github/workflows/ci.yml");

    require_all_tokens(
        workflow,
        {"name: Windows MSVC ${{ matrix.role }} (${{ matrix.preset }})", "fail-fast: false",
         "include:", "role: Debug quick path", "role: Debug inner-loop lane",
         "role: Release package-proof lane", "preset: windows-msvc-debug-static",
         "preset: windows-msvc-debug-shared", "preset: windows-msvc-release-static",
         "preset: windows-msvc-release-shared", "cmake --preset ${{ matrix.preset }}",
         "cmake --build --preset ${{ matrix.preset }}",
         "ctest --preset ${{ matrix.preset }} --output-on-failure"});

    REQUIRE(count_occurrences(workflow, "preset: windows-msvc-") == 4);
    REQUIRE(workflow.find("preset: windows-msvc-asan-static") == std::string::npos);
}

TEST_CASE(
    "validation_policy verification matrix contract keeps ASan as a "
    "separate hardening job",
    "[unit][validation_policy][static_boundary]") {
    const auto workflow = read_text_file(source_root() / ".github/workflows/ci.yml");
    const auto windows_job = yaml_block(workflow, "windows-msvc", 2);
    const auto asan_job = yaml_block(workflow, "windows-msvc-asan-static", 2);
    REQUIRE(windows_job.has_value());
    REQUIRE(asan_job.has_value());

    require_all_tokens(*asan_job, {"name: Windows MSVC AddressSanitizer hardening lane "
                                   "(windows-msvc-asan-static)",
                                   "cmake --preset windows-msvc-asan-static",
                                   "cmake --build --preset windows-msvc-asan-static",
                                   "ctest --preset windows-msvc-asan-static --output-on-failure",
                                   "git status --short TES5Edit"});

    REQUIRE(asan_job->find("matrix:") == std::string::npos);
    REQUIRE(asan_job->find("${{ matrix.preset }}") == std::string::npos);
    REQUIRE(asan_job->find("needs:") == std::string::npos);
    REQUIRE(asan_job->find("windows-msvc-release-static") == std::string::npos);
    REQUIRE(asan_job->find("windows-msvc-release-shared") == std::string::npos);

    REQUIRE(windows_job->find("windows-msvc-asan-static") == std::string::npos);
}
