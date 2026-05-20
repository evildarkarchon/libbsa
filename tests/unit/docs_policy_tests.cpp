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

TEST_CASE("docs_policy mainpage links audited API proof and consumer sharp edges",
          "[unit][docs_policy][doc_structure]")
{
  const auto mainpage = read_text_file(source_root() / "docs" / "api-mainpage.md");

  require_all_tokens(mainpage,
                     {"docs/public-api-reality-check.md",
                      "docs/coverage-audit-matrix.md",
                      "docs/compatibility-evidence.md",
                      "archive virtual paths",
                      "find",
                      "contains",
                      "extract_bytes",
                      "`worker_count` must be positive",
                      "BA2 DX10 texture writers have a one-shot writer lifecycle",
                      "result<T>::error().code",
                      "error().message",
                      "Optional corpus evidence"});
}

TEST_CASE("docs_policy integration examples document the full consumer journey",
          "[unit][docs_policy][doc_structure]")
{
  const auto examples = read_text_file(source_root() / "docs" / "integration-examples.md");

  require_all_tokens(examples,
                     {"host filesystem paths and archive virtual paths separate",
                      "find()",
                      "contains()",
                      "extract()",
                      "extract_bytes()",
                      "`worker_count == 0` is invalid",
                      "bulk_extract_entry_result",
                      "write_execution_options::worker_count",
                      "BA2 DX10 has a one-shot writer lifecycle",
                      "result<T>::error().code",
                      "validation_report::warnings",
                      "compatibility_warning"});
}

TEST_CASE("docs_policy compatibility evidence points to audit and preserves proof boundaries",
          "[unit][docs_policy][doc_structure]")
{
  const auto catalog = read_text_file(source_root() / "docs" / "compatibility-evidence.md");

  require_all_tokens(catalog,
                     {"docs/coverage-audit-matrix.md",
                      "docs/public-api-reality-check.md",
                      "Optional local game or BSArchPro-derived checks",
                      "never required for the default suite",
                      "Optional local corpus checks are advisory evidence only",
                      "absent local or copyrighted inputs do not block default green status"});
}

TEST_CASE("docs_policy public API audit is discoverable and routes proof gaps",
          "[unit][docs_policy][doc_structure][coverage_audit_matrix]")
{
  const auto root = source_root();
  const auto mainpage = read_text_file(root / "docs" / "api-mainpage.md");
  const auto examples = read_text_file(root / "docs" / "integration-examples.md");
  const auto catalog = read_text_file(root / "docs" / "compatibility-evidence.md");
  const auto audit = read_text_file(root / "docs" / "public-api-reality-check.md");
  const auto matrix = read_text_file(root / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(mainpage,
                     {"docs/public-api-reality-check.md",
                      "docs/coverage-audit-matrix.md",
                      "docs/compatibility-evidence.md",
                      "Optional corpus evidence",
                      "not required for default support claims"});

  require_all_tokens(examples,
                     {"These examples are compile-checked by `tests/package-consumer/main.cpp`",
                      "includes only `<libbsa/libbsa.hpp>`",
                      "archive_reader::open",
                      "metadata()",
                      "entries()",
                      "find()",
                      "contains()",
                      "extract()",
                      "extract_bytes()",
                      "extract_entries",
                      "validate_archive",
                      "result<T>::error().code",
                      "tes3_bsa_writer",
                      "tes4_bsa_writer",
                      "ba2_gnrl_writer",
                      "ba2_dx10_writer"});

  require_all_tokens(audit,
                     {"proof-and-routing document",
                      "Default proof means evidence that is reproducible from the repository without local game archives or BSArchPro-generated corpus output",
                      "Package-consumer proof: `tests/package-consumer/main.cpp`",
                      "Public-boundary and documentation policy proof:",
                      "archive_reader",
                      "find",
                      "contains",
                      "extract",
                      "extract_bytes",
                      "extract_entries",
                      "validate_archive",
                      "result<T>",
                      "stable `error_code`",
                      "tes3_bsa_writer",
                      "tes4_bsa_writer",
                      "ba2_gnrl_writer",
                      "ba2_dx10_writer",
                      "No broad public facade is justified",
                      "`COV-GAP-003`",
                      "S05",
                      "installed-package runtime archive creation/opening for every family is intentionally not proven by default"});

  require_all_tokens(catalog,
                     {"Optional local game or BSArchPro-derived checks",
                      "never required for the default suite",
                      "Optional local corpus checks are advisory evidence only",
                      "absent local or copyrighted inputs do not block default green status"});

  require_all_tokens(matrix,
                     {"`COV-GAP-003`",
                      "Package-consumer runtime proof for creating/opening every archive family from an installed package",
                      "actual archive behavior is covered by always-on unit fixture tests",
                      "S05 integrated confidence/package-release polish"});
}

TEST_CASE("docs_policy public proof docs keep optional evidence and fixture boundaries explicit",
          "[unit][docs_policy][doc_structure][static_boundary]")
{
  const auto root = source_root();
  constexpr auto proof_documents = std::array{
      "docs/public-api-reality-check.md",
      "docs/api-mainpage.md",
      "docs/integration-examples.md",
      "docs/compatibility-evidence.md",
      "docs/coverage-audit-matrix.md",
  };

  for (const auto relative_path : proof_documents)
  {
    const auto text = read_text_file(root / relative_path);
    INFO("Public proof document: " << relative_path);
    require_no_tokens(text,
                      {".gsd/",
                       ".planning/",
                       ".audits/",
                       "TES5Edit fixture workspace",
                       "TES5Edit/ fixture",
                       "mutable TES5Edit",
                       "write into `TES5Edit/`",
                       "local copyrighted fixture directory"});
  }

  const auto audit = read_text_file(root / "docs" / "public-api-reality-check.md");
  require_all_tokens(audit,
                     {"Optional local game corpora and BSArchPro-derived manifests remain advisory evidence only",
                      "`TES5Edit/` remains a read-only reference boundary"});

  const auto catalog = read_text_file(root / "docs" / "compatibility-evidence.md");
  require_all_tokens(catalog,
                     {"Optional local corpus checks are advisory evidence only",
                      "absent local or copyrighted inputs do not block default green status",
                      "Treat `TES5Edit/` as read-only reference material, not a fixture workspace or output directory."});
}

TEST_CASE("docs_policy public documentation surfaces hide planning identifiers",
          "[unit][docs_policy][doc_structure]")
{
  const auto root = source_root();
  constexpr auto public_documentation_files = std::array{
      "include/libbsa/archive.hpp",
      "include/libbsa/writer.hpp",
      "docs/api-mainpage.md",
      "docs/integration-examples.md",
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
