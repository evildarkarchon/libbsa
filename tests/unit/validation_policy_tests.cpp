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

std::filesystem::path source_root() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR};
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

bool command_succeeds(const std::string& command) {
  return std::system(command.c_str()) == 0;
}

std::string quoted_path(const std::filesystem::path& path) {
  return '"' + path.string() + '"';
}

std::string trim_copy(std::string value) {
  const auto first = std::find_if(value.begin(), value.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  });
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

struct verification_lane_contract {
  std::string_view preset_name;
  std::string_view role_name;
  bool package_proof_lane;
};

// Keep the supported lane matrix in one place so future role or lane changes must update
// the shared contract before the per-surface assertions can pass again.
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

/// Parse a small YAML block by indentation so workflow topology tests can stay dependency-light.
std::optional<std::string> yaml_block(std::string_view text,
                                      std::string_view header,
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

    if (line.size() > indent_spaces &&
        line.starts_with(std::string(indent_spaces, ' ')) &&
        line[indent_spaces] != ' ' &&
        line[indent_spaces] != '-' &&
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

std::vector<std::string> yaml_scalar_values(std::string_view block, std::string_view key) {
  std::vector<std::string> values;
  std::istringstream lines{std::string{block}};
  std::string line;
  const auto prefix = std::string{key} + ":";
  while (std::getline(lines, line)) {
    const auto trimmed = trim_copy(line);
    if (!trimmed.starts_with(prefix)) {
      continue;
    }

    auto value = trim_copy(trimmed.substr(prefix.size()));
    if (!value.empty() && value.find("${{") == std::string::npos) {
      values.push_back(std::move(value));
    }
  }
  return values;
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

void require_exact_values(const std::vector<std::string>& actual,
                          std::initializer_list<std::string_view> expected) {
  INFO("Actual values count: " << actual.size());
  REQUIRE(actual.size() == expected.size());

  std::vector<std::string> normalized_actual = actual;
  std::sort(normalized_actual.begin(), normalized_actual.end());

  std::vector<std::string> normalized_expected;
  normalized_expected.reserve(expected.size());
  for (const auto value : expected) {
    normalized_expected.emplace_back(value);
  }
  std::sort(normalized_expected.begin(), normalized_expected.end());

  REQUIRE(normalized_actual == normalized_expected);
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

} // namespace

TEST_CASE("validation_policy CTest label taxonomy is documented and backed by selectable tests",
          "[unit][fixture][roundtrip][compat][malformed][slow][doc_structure]") {
  const auto readme = read_text_file(source_root() / "tests/fixtures/README.md");

  constexpr std::array<std::string_view, 7> required_labels{
    "unit",
    "fixture",
    "roundtrip",
    "compat",
    "malformed",
    "slow",
    "requires-game-fixture",
  };

  for (const auto label : required_labels) {
    INFO("Missing documented label: " << label);
    REQUIRE(readme.find(std::string{label}) != std::string::npos);
  }
}

TEST_CASE("validation_policy requires-game-fixture label is selectable without local archives",
          "[unit][requires-game-fixture][doc_structure]") {
  const auto readme = read_text_file(source_root() / "tests/fixtures/README.md");

  REQUIRE(readme.find("Tests discovered by default") != std::string::npos);
  REQUIRE(readme.find("skipped unless local") != std::string::npos);
  REQUIRE(readme.find("LIBBSA_GAME_FIXTURES") != std::string::npos);
}

TEST_CASE("validation_policy compatibility evidence catalog documents public warning-code structure",
          "[unit][compat][validation_policy][doc_structure]") {
  const auto catalog = read_text_file(source_root() / "docs/compatibility-evidence.md");
  const auto warning_codes = compatibility_warning_codes_from_public_header();
  REQUIRE_FALSE(warning_codes.empty());

  for (const auto code : warning_codes) {
    const auto heading = "### `" + code + "`";
    const auto entry_start = catalog.find(heading);
    INFO("Missing compatibility evidence entry: " << code);
    REQUIRE(entry_start != std::string::npos);

    const auto next_entry = catalog.find("\n### `", entry_start + heading.size());
    const auto entry = catalog.substr(entry_start, next_entry - entry_start);
    REQUIRE(entry.find("Rule:") != std::string::npos);
    REQUIRE(entry.find("Evidence:") != std::string::npos);
  }

  REQUIRE(catalog.find("generated") != std::string::npos);
  REQUIRE(catalog.find("writer-output") != std::string::npos);
}

TEST_CASE("validation_policy local fixture boundary keeps game archives ignored and provenance documented",
          "[unit][fixture][static_boundary][doc_structure]") {
  const auto root = source_root();
  const auto readme = read_text_file(root / "tests/fixtures/README.md");
  const auto gitignore = read_text_file(root / ".gitignore");

  REQUIRE(readme.find("tests/fixtures/generated/source") != std::string::npos);
  REQUIRE(readme.find("tests/fixtures/generated/archives") != std::string::npos);
  REQUIRE(readme.find("The generator or source recipe") != std::string::npos);
  REQUIRE(readme.find("The legal provenance") != std::string::npos);
  REQUIRE(readme.find("The behavior it proves") != std::string::npos);
  REQUIRE(readme.find("TES5Edit/ must not be used as a fixture workspace") != std::string::npos);
  REQUIRE(readme.find("LIBBSA_BSARCHPRO_EXPECTED") != std::string::npos);
  REQUIRE(readme.find("bsarchpro_expected.json") != std::string::npos);
  REQUIRE(readme.find("BSArchPro-derived expected fixture comparisons are opt-in") != std::string::npos);

  REQUIRE(gitignore.find("/tests/fixtures/local/*") != std::string::npos);
  REQUIRE(gitignore.find("!/tests/fixtures/local/.gitkeep") != std::string::npos);

  const auto git_base = "git -C " + quoted_path(root) + " check-ignore ";
#ifdef _WIN32
  const auto silence = " >NUL 2>NUL";
#else
  const auto silence = " >/dev/null 2>/dev/null";
#endif

  REQUIRE(command_succeeds(git_base + "tests/fixtures/local/example.bsa" + silence));
  REQUIRE_FALSE(command_succeeds(git_base + "tests/fixtures/local/.gitkeep" + silence));
}

TEST_CASE("validation_policy CI and presets preserve static shared and TES5Edit build boundaries",
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
  REQUIRE(workflow.find("https://github.com/Kitware/CMake/releases/download") != std::string::npos);
  REQUIRE(workflow.find("cmake --version") != std::string::npos);
  REQUIRE(workflow.find("cmake --preset ${{ matrix.preset }}") != std::string::npos);
  REQUIRE(workflow.find("cmake --build --preset ${{ matrix.preset }}") != std::string::npos);
  REQUIRE(workflow.find("ctest --preset ${{ matrix.preset }} --output-on-failure") != std::string::npos);
  REQUIRE(workflow.find("git status --short TES5Edit") != std::string::npos);
  REQUIRE(workflow.find("TES5Edit submodule changed during CI") != std::string::npos);
}

TEST_CASE("validation_policy configured build profiles are Windows-only and documented",
          "[unit][validation_policy][static_boundary][doc_structure]") {
  const auto root = source_root();
  const auto presets = read_text_file(root / "CMakePresets.json");
  const auto workflow = read_text_file(root / ".github/workflows/ci.yml");
  const auto fixture_policy = read_text_file(root / "tests/fixtures/README.md");
  const auto readme = read_text_file(root / "README.md");
  const auto agents = read_text_file(root / "AGENTS.md");
  const auto claude = read_text_file(root / "CLAUDE.md");

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
  REQUIRE(agents.find("Windows-only") != std::string::npos);
  REQUIRE(claude.find("Windows-only") != std::string::npos);
}

TEST_CASE("validation_policy verification matrix contract keeps README truthful",
          "[unit][validation_policy][doc_structure]") {
  const auto readme = read_text_file(source_root() / "README.md");

  require_lane_names(readme);
  require_all_tokens(readme,
                     {"quick day-to-day path",
                      "Debug inner-loop lanes",
                      "Release package-proof lanes",
                      "MSVC AddressSanitizer hardening lane",
                      "package_consumer_smoke",
                      "Windows-only",
                      "requires-game-fixture",
                      "skipped by default"});
}

TEST_CASE("validation_policy verification matrix contract keeps fixture policy truthful",
          "[unit][validation_policy][doc_structure]") {
  const auto fixture_policy = read_text_file(source_root() / "tests/fixtures/README.md");

  require_lane_names(fixture_policy);
  require_all_tokens(fixture_policy,
                     {"Debug inner-loop lanes",
                      "Release package-proof lanes",
                      "MSVC AddressSanitizer hardening lane",
                      "package_consumer_smoke",
                      "Windows-only",
                      "requires-game-fixture",
                      "skipped by default",
                      "WSL",
                      "extra sanitizer families"});
}

TEST_CASE("validation_policy verification matrix contract keeps planning summaries truthful",
          "[unit][validation_policy][doc_structure]") {
  const auto root = source_root();
  const auto project = read_text_file(root / ".planning/PROJECT.md");
  const auto roadmap = read_text_file(root / ".planning/ROADMAP.md");
  const auto state = read_text_file(root / ".planning/STATE.md");

  require_all_tokens(project,
                     {"v1.0 shipped on 2026-05-10",
                      "Phase 14 is the truthful verification-matrix hardening slice",
                      "Release package-proof lanes",
                      "MSVC AddressSanitizer hardening lane",
                      "without rewriting v1.0 history"});
  REQUIRE(project.find("cmake --preset") == std::string::npos);

  require_all_tokens(roadmap,
                     {"Phase 14: Verification Lane Truthfulness",
                      "supported Windows debug, Release package-proof, and MSVC AddressSanitizer verification lanes",
                      "14-03-PLAN.md"});
  REQUIRE(roadmap.find("cmake --preset") == std::string::npos);

  require_all_tokens(state,
                     {"Phase 14 is closing the truthful supported matrix loop across docs and planning",
                      "MSVC AddressSanitizer"});
  REQUIRE(state.find("cmake --preset") == std::string::npos);
}

TEST_CASE("validation_policy verification matrix contract requires supported preset triads",
          "[unit][validation_policy][static_boundary]") {
  const auto presets = read_text_file(source_root() / "CMakePresets.json");

  for (const auto& lane : verification_matrix_contract()) {
    require_preset_family(presets, lane);
  }
}

TEST_CASE("validation_policy verification matrix contract requires checked-in MSVC ASan wiring",
          "[unit][validation_policy][static_boundary]") {
  const auto root_cmake = read_text_file(source_root() / "CMakeLists.txt");

  require_all_tokens(root_cmake,
                     {"LIBBSA_ENABLE_MSVC_ASAN", "MSVC", "/fsanitize=address"});
}

TEST_CASE("validation_policy verification matrix contract keeps release package proof inside CTest",
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

TEST_CASE("validation_policy verification matrix contract keeps the main workflow matrix truthful",
          "[unit][validation_policy][static_boundary]") {
  const auto workflow = read_text_file(source_root() / ".github/workflows/ci.yml");

  require_all_tokens(workflow,
                      {"name: Windows MSVC ${{ matrix.role }} (${{ matrix.preset }})",
                       "fail-fast: false",
                       "include:",
                       "role: Debug quick path",
                       "role: Debug inner-loop lane",
                       "role: Release package-proof lane",
                       "preset: windows-msvc-debug-static",
                       "preset: windows-msvc-debug-shared",
                       "preset: windows-msvc-release-static",
                       "preset: windows-msvc-release-shared",
                       "cmake --preset ${{ matrix.preset }}",
                       "cmake --build --preset ${{ matrix.preset }}",
                       "ctest --preset ${{ matrix.preset }} --output-on-failure"});

  REQUIRE(count_occurrences(workflow, "preset: windows-msvc-") == 4);
  REQUIRE(workflow.find("preset: windows-msvc-asan-static") == std::string::npos);
}

TEST_CASE("validation_policy verification matrix contract keeps ASan as a separate hardening job",
          "[unit][validation_policy][static_boundary]") {
  const auto workflow = read_text_file(source_root() / ".github/workflows/ci.yml");
  const auto windows_job = yaml_block(workflow, "windows-msvc", 2);
  const auto asan_job = yaml_block(workflow, "windows-msvc-asan-static", 2);
  REQUIRE(windows_job.has_value());
  REQUIRE(asan_job.has_value());

  require_all_tokens(*asan_job,
                     {"name: Windows MSVC AddressSanitizer hardening lane (windows-msvc-asan-static)",
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
