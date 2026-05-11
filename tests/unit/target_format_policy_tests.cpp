#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
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

bool guide_has_warning_entry(const std::string& guide, const std::string& code) {
  return guide.find("### `" + code + "`") != std::string::npos ||
         guide.find("`" + code + "`") != std::string::npos;
}

} // namespace

TEST_CASE("target_format_policy examples match compile-checked package consumer functions",
          "[unit][target_format_policy]") {
  const auto root = source_root();
  const auto docs = read_text_file(root / "docs/integration-examples.md");
  const auto consumer = read_text_file(root / "tests/package-consumer/main.cpp");

  constexpr std::array<std::string_view, 8> examples{
    "example_open_list_extract",
    "example_bulk_extract",
    "example_create_tes3_bsa",
    "example_create_tes4_bsa",
    "example_create_ba2_gnrl",
    "example_create_ba2_dx10",
    "example_handle_result_errors",
    "example_validate_archive",
  };

  for (const auto example : examples) {
    INFO("Missing integration example: " << example);
    REQUIRE(consumer.find(std::string{example}) != std::string::npos);
    REQUIRE(docs.find("## `" + std::string{example} + "`") != std::string::npos);
  }

  REQUIRE(consumer.find("extract_entries") != std::string::npos);
  REQUIRE(consumer.find("write_execution_options") != std::string::npos);
  REQUIRE(consumer.find("validate_archive") != std::string::npos);
  REQUIRE(consumer.find("error().code") != std::string::npos);
}

TEST_CASE("target_format_policy guide covers every supported target format and compression route",
          "[unit][target_format_policy]") {
  const auto guide = read_text_file(source_root() / "docs/target-format-guide.md");

  constexpr std::array<std::string_view, 13> required_headings{
    "## TES3 BSA",
    "## TES4-family BSA v103",
    "## TES4-family BSA v104",
    "## Skyrim SE/AE BSA v105",
    "## Fallout 4 BA2 GNRL",
    "## Fallout 4 BA2 DX10",
    "## Starfield BA2 v2 GNRL",
    "## Starfield BA2 v3 GNRL",
    "## Starfield BA2 v3 DX10",
    "## deflate",
    "## LZ4 frame",
    "## raw LZ4 block",
    "## writer target policies",
  };

  for (const auto heading : required_headings) {
    INFO("Missing target-format guide heading: " << heading);
    REQUIRE(guide.find(std::string{heading}) != std::string::npos);
  }

  REQUIRE(guide.find("DX10 DDS data is not resized, transcoded, mip-generated, repaired, or otherwise transformed") !=
          std::string::npos);
  REQUIRE(guide.find("BC6, SRGB, or SNORM") != std::string::npos);
  REQUIRE(guide.find("CompressionMethod == 3") != std::string::npos);
  REQUIRE(guide.find("tes4_bsa_target::skyrim_se") != std::string::npos);
  REQUIRE(guide.find("ba2_gnrl_target::starfield_v3") != std::string::npos);
  REQUIRE(guide.find("ba2_dx10_target::starfield_v3") != std::string::npos);
}

TEST_CASE("target_format_policy guide documents public compatibility_warning_code values",
          "[unit][target_format_policy][validation_policy]") {
  const auto guide = read_text_file(source_root() / "docs/target-format-guide.md");
  const auto warning_codes = compatibility_warning_codes_from_public_header();
  REQUIRE_FALSE(warning_codes.empty());

  REQUIRE(guide.find("docs/compatibility-evidence.md") != std::string::npos);
  REQUIRE(guide.find("## compatibility warnings") != std::string::npos);

  for (const auto& code : warning_codes) {
    INFO("Missing target-format warning entry: " << code);
    REQUIRE(guide_has_warning_entry(guide, code));
  }
}

TEST_CASE("target_format_policy guide preserves legal evidence and reference boundaries",
          "[unit][target_format_policy][fixture]") {
  const auto guide = read_text_file(source_root() / "docs/target-format-guide.md");
  const auto fixture_policy = read_text_file(source_root() / "tests/fixtures/README.md");

  REQUIRE(guide.find("legal synthetic data") != std::string::npos);
  REQUIRE(guide.find("TES5Edit/ is a read-only reference") != std::string::npos);
  REQUIRE(guide.find("fixture workspace") != std::string::npos);
  REQUIRE(fixture_policy.find("benchmarks/README.md") != std::string::npos);
  REQUIRE(fixture_policy.find("legal synthetic data") != std::string::npos);
}
