#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

} // namespace

TEST_CASE("CTest label taxonomy is documented and backed by selectable tests", "[unit][fixture][roundtrip][compat][malformed][slow]") {
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

TEST_CASE("requires-game-fixture label is selectable without local archives", "[unit][requires-game-fixture]") {
  const auto readme = read_text_file(source_root() / "tests/fixtures/README.md");

  REQUIRE(readme.find("Tests discovered by default") != std::string::npos);
  REQUIRE(readme.find("skipped unless local") != std::string::npos);
  REQUIRE(readme.find("LIBBSA_GAME_FIXTURES") != std::string::npos);
}

TEST_CASE("compatibility evidence catalog documents public warning codes", "[unit][compat][validation_policy]") {
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

TEST_CASE("local fixture policy keeps game archives ignored and provenance documented", "[unit][fixture]") {
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

TEST_CASE("CI and presets preserve static shared and TES5Edit build boundaries", "[unit][public-api]") {
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

TEST_CASE("configured build profiles are Windows-only and documented", "[unit][validation_policy]") {
  const auto root = source_root();
  const auto presets = read_text_file(root / "CMakePresets.json");
  const auto workflow = read_text_file(root / ".github/workflows/ci.yml");
  const auto fixture_policy = read_text_file(root / "tests/fixtures/README.md");
  const auto readme = read_text_file(root / "README.md");
  const auto agents = read_text_file(root / "AGENTS.md");
  const auto claude = read_text_file(root / "CLAUDE.md");

  REQUIRE(presets.find("windows-msvc-debug-static") != std::string::npos);
  REQUIRE(presets.find("windows-msvc-debug-shared") != std::string::npos);
  REQUIRE(presets.find("linux-clang-asan-ubsan") == std::string::npos);
  REQUIRE(presets.find("-fsanitize=address,undefined") == std::string::npos);

  REQUIRE(workflow.find("windows-msvc-debug-static") != std::string::npos);
  REQUIRE(workflow.find("windows-msvc-debug-shared") != std::string::npos);
  REQUIRE(workflow.find("git status --short TES5Edit") != std::string::npos);

  REQUIRE(fixture_policy.find("Windows-only") != std::string::npos);
  REQUIRE(fixture_policy.find("linux-clang-asan-ubsan") == std::string::npos);
  REQUIRE(readme.find("Windows-only") != std::string::npos);
  REQUIRE(agents.find("Windows-only") != std::string::npos);
  REQUIRE(claude.find("Windows-only") != std::string::npos);
}
