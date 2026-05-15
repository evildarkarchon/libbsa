#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

namespace
{

  std::filesystem::path source_root()
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR};
  }

  std::string read_text_file(const std::filesystem::path &path)
  {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
  }

  void require_all_tokens(std::string_view text, std::initializer_list<std::string_view> tokens)
  {
    for (const auto token : tokens)
    {
      INFO("Missing token: " << token);
      REQUIRE(text.find(token) != std::string_view::npos);
    }
  }

  void require_no_tokens(std::string_view text, std::initializer_list<std::string_view> tokens)
  {
    for (const auto token : tokens)
    {
      INFO("Forbidden token: " << token);
      REQUIRE(text.find(token) == std::string_view::npos);
    }
  }

  void require_no_planning_identifier_patterns(std::string_view relative_path, const std::string &text)
  {
    struct ForbiddenPattern
    {
      std::string_view family;
      std::regex pattern;
    };

    const auto patterns = std::array{
        ForbiddenPattern{"phase label", std::regex{R"(\bphase\s+\d+\b)", std::regex_constants::icase}},
        ForbiddenPattern{"milestone label", std::regex{R"(\bmilestone\s+\d+\b)", std::regex_constants::icase}},
        ForbiddenPattern{"decision ID", std::regex{R"(\bD-\d+\b)", std::regex_constants::icase}},
    };

    for (const auto &forbidden : patterns)
    {
      INFO("Public documentation file: " << relative_path);
      INFO("Forbidden planning token family: " << forbidden.family);
      REQUIRE_FALSE(std::regex_search(text, forbidden.pattern));
    }
  }

} // namespace

TEST_CASE("docs_policy static boundary keeps public API documentation optional", "[unit][docs_policy][static_boundary]")
{
  const auto cmake = read_text_file(source_root() / "CMakeLists.txt");

  require_all_tokens(cmake,
                     {"find_package(Doxygen QUIET)",
                      "if(DOXYGEN_FOUND)",
                      "configure_file(",
                      "docs/Doxyfile.in",
                      "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile",
                      "add_custom_target(libbsa_docs",
                      "${DOXYGEN_EXECUTABLE}",
                      "Doxygen not found; libbsa_docs target not available"});
}

TEST_CASE("docs_policy Doxyfile boundary documents public headers and excludes private inputs",
          "[unit][docs_policy][static_boundary]")
{
  const auto doxyfile = read_text_file(source_root() / "docs" / "Doxyfile.in");

  require_all_tokens(doxyfile,
                     {"PROJECT_NAME = libbsa",
                      "INPUT = @CMAKE_CURRENT_SOURCE_DIR@/include/libbsa",
                      "@CMAKE_CURRENT_SOURCE_DIR@/docs/api-mainpage.md",
                      "@CMAKE_CURRENT_SOURCE_DIR@/docs/thread-safety.md",
                      "RECURSIVE = YES",
                      "EXCLUDE = @CMAKE_CURRENT_SOURCE_DIR@/src",
                      "@CMAKE_CURRENT_SOURCE_DIR@/TES5Edit",
                      "@CMAKE_CURRENT_SOURCE_DIR@/tests",
                      "@CMAKE_CURRENT_BINARY_DIR@",
                      "GENERATE_HTML = YES",
                      "GENERATE_LATEX = NO",
                      "WARN_IF_UNDOCUMENTED = YES",
                      "WARN_AS_ERROR = NO",
                      "EXTRACT_PRIVATE = NO"});

  require_no_tokens(doxyfile,
                    {"INPUT = @CMAKE_CURRENT_SOURCE_DIR@/src",
                     "INPUT = @CMAKE_CURRENT_SOURCE_DIR@/TES5Edit",
                     "WARN_AS_ERROR = YES"});
}

TEST_CASE("docs_policy API mainpage preserves public operation structure", "[unit][docs_policy][doc_structure]")
{
  const auto mainpage = read_text_file(source_root() / "docs" / "api-mainpage.md");

  require_all_tokens(mainpage,
                     {"@mainpage",
                      "archive_reader",
                      "payload_sink",
                      "extract_entries",
                      "bulk_extract_options",
                      "bulk_extract_sink_factory",
                      "bulk_extract_entry_result",
                      "tes3_bsa_writer",
                      "tes4_bsa_writer",
                      "ba2_gnrl_writer",
                      "ba2_dx10_writer",
                      "write_execution_options",
                      "validate_archive",
                      "validation_report",
                      "@ref thread_safety",
                      "target-format guide"});
}

TEST_CASE("docs_policy public documentation surfaces hide planning identifiers",
          "[unit][docs_policy][doc_structure]")
{
  const auto root = source_root();
  constexpr auto public_documentation_files = std::array{
      "include/libbsa/archive.hpp",
      "include/libbsa/writer.hpp",
      "docs/thread-safety.md",
      "docs/compatibility-evidence.md",
      "tests/fixtures/README.md",
  };

  for (const auto relative_path : public_documentation_files)
  {
    const auto text = read_text_file(root / relative_path);
    require_no_planning_identifier_patterns(relative_path, text);
  }
}
