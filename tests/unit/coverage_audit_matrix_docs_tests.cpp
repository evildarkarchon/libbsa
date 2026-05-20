#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <initializer_list>
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

} // namespace

TEST_CASE("coverage_audit_matrix lists every current archive family", "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(matrix,
                     {"TES3 BSA",
                      "TES4-family BSA",
                      "BA2 GNRL",
                      "BA2 DX10"});
}

TEST_CASE("coverage_audit_matrix preserves every required support axis", "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(matrix,
                     {"Reader/open/list metadata",
                      "Extraction",
                      "Writer",
                      "Round-trip/reopen",
                      "Malformed handling",
                      "Validation API behavior",
                      "Compatibility warnings",
                      "Public/package-consumer API proof",
                      "Docs/support-claim proof"});
}

TEST_CASE("coverage_audit_matrix preserves status vocabulary and ranked gaps", "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(matrix,
                     {"Status vocabulary",
                      "**Proven**",
                      "**Partial**",
                      "**Missing**",
                      "**Deferred**",
                      "**N/A**",
                      "Ranked gap list"});

  INFO("Matrix must include ranked COV-GAP entries or an explicit no-high-risk-gap statement.");
  REQUIRE((matrix.find("COV-GAP-") != std::string::npos ||
           matrix.find("No high-risk executable-proof gap was found") != std::string::npos));
}

TEST_CASE("coverage_audit_matrix separates advisory local evidence from default proof",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(matrix,
                     {"Default evidence sources",
                      "Optional local corpora are advisory evidence only",
                      "Advisory local evidence",
                      "LIBBSA_GAME_FIXTURES",
                      "LIBBSA_BSARCHPRO_EXPECTED",
                      "Absent local/copyrighted inputs do **not** block default green status",
                      "Present local-only checks also do **not** replace default proof"});
}

TEST_CASE("coverage_audit_matrix names compatibility matrix as malformed submatrix only",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(matrix,
                     {"tests/fixtures/generated/compatibility_matrix.json",
                      "is a malformed-hardening submatrix",
                      "It is **not** the full support matrix",
                      "does not replace the reader, extraction, writer, round-trip, package-consumer, docs-policy, or optional local-corpus evidence"});
}

TEST_CASE("coverage_audit_matrix stays free of internal planning identifiers",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_no_tokens(matrix,
                    {".gsd/",
                     "M001",
                     "S01"});
}

TEST_CASE("coverage_audit_matrix is discoverable from compatibility evidence catalog",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto compatibility_evidence = read_text_file(source_root() / "docs" / "compatibility-evidence.md");

  require_all_tokens(compatibility_evidence, {"coverage-audit-matrix.md"});
}
