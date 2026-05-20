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

  std::string_view require_markdown_section(std::string_view text, std::string_view heading)
  {
    const auto start = text.find(heading);
    INFO("Missing section heading: " << heading);
    REQUIRE(start != std::string_view::npos);

    auto end = text.size();
    const auto after_heading = start + heading.size();
    const auto next_sibling = text.find("\n### ", after_heading);
    if (next_sibling != std::string_view::npos && next_sibling < end)
    {
      end = next_sibling;
    }

    const auto next_parent = text.find("\n## ", after_heading);
    if (next_parent != std::string_view::npos && next_parent < end)
    {
      end = next_parent;
    }

    return text.substr(start, end - start);
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

TEST_CASE("coverage_audit_matrix preserves default round-trip evidence for every family",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  const auto tes3 = require_markdown_section(matrix, "### TES3 BSA");
  require_all_tokens(tes3,
                     {"| Round-trip/reopen | Proven |",
                      "`tests/unit/tes3_bsa_writer_tests.cpp`",
                      "`tests/unit/validation_api_tests.cpp`",
                      "Writer-output archives reopen through `archive_reader` lookup/extraction",
                      "validation API accepts writer-produced TES3 archives with extractability enabled",
                      "`tests/fixtures/generated/archives/tes3_success_manifest.json`",
                      "`tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json`"});

  const auto tes4 = require_markdown_section(matrix, "### TES4-family BSA");
  require_all_tokens(tes4,
                     {"| Round-trip/reopen | Proven |",
                      "`tests/unit/tes4_bsa_writer_tests.cpp`",
                      "`tests/unit/archive_reader_dispatch_tests.cpp`",
                      "`tests/unit/validation_api_tests.cpp`",
                      "Writer raw output reopens for every target profile",
                      "target-default compression round-trips inherited entries",
                      "representative writer-produced TES4 archives validate with extractability enabled",
                      "`tests/fixtures/generated/archives/tes4_v103_manifest.json`",
                      "`tests/fixtures/generated/archives/tes4_v104_manifest.json`",
                      "`tests/fixtures/generated/archives/tes4_v105_manifest.json`"});

  const auto ba2_gnrl = require_markdown_section(matrix, "### BA2 GNRL");
  require_all_tokens(ba2_gnrl,
                     {"| Round-trip/reopen | Proven |",
                      "`tests/unit/ba2_gnrl_writer_tests.cpp`",
                      "`tests/unit/archive_reader_dispatch_tests.cpp`",
                      "`tests/unit/validation_api_tests.cpp`",
                      "Writer-output BA2 GNRL archives reopen through public metadata and extraction",
                      "representative Fallout 4 writer output validates with extractability enabled",
                      "`tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json`",
                      "`tests/fixtures/generated/archives/ba2_gnrl_sfv2_manifest.json`",
                      "`tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json`"});

  const auto ba2_dx10 = require_markdown_section(matrix, "### BA2 DX10");
  require_all_tokens(ba2_dx10,
                     {"| Round-trip/reopen | Proven |",
                      "`tests/unit/ba2_dx10_writer_tests.cpp`",
                      "`tests/unit/archive_reader_dispatch_tests.cpp`",
                      "`tests/unit/validation_api_tests.cpp`",
                      "Writer-output archives reopen through `archive_reader`",
                      "Starfield routes preserve texture payload bytes through extraction",
                      "representative Fallout 4 writer output validates with extractability enabled",
                      "`tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json`",
                      "`tests/fixtures/generated/archives/ba2_dx10_sfv3_manifest.json`",
                      "`tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json`"});
}

TEST_CASE("coverage_audit_matrix preserves default fixture and manifest evidence policy",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(matrix,
                     {"Default evidence sources",
                      "committed legal/generated fixtures",
                      "writer-output archives",
                      "always-on Catch2/CTest coverage",
                      "Fixture policy and provenance",
                      "generated fixture manifests under `tests/fixtures/generated/archives/`",
                      "Malformed-hardening index",
                      "`tests/fixtures/generated/compatibility_matrix.json`",
                      "Optional local corpora are advisory evidence only"});
}

TEST_CASE("compatibility_evidence exposes default fixture round-trip proof sweep",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto compatibility_evidence = read_text_file(source_root() / "docs" / "compatibility-evidence.md");

  require_all_tokens(compatibility_evidence,
                     {"## Default fixture and round-trip proof sweep",
                      "committed legal generated fixtures",
                      "writer-output archives produced by the public writer APIs",
                      "Catch2/CTest cases",
                      "manifest validation only",
                      "It does not require local game archives",
                      "BSArchPro-derived comparison output",
                      "TES3 BSA",
                      "TES4-family BSA",
                      "BA2 GNRL",
                      "BA2 DX10",
                      "`tests/unit/archive_reader_dispatch_tests.cpp`",
                      "`tests/unit/tes3_bsa_writer_tests.cpp`",
                      "`tests/unit/tes4_bsa_writer_tests.cpp`",
                      "`tests/unit/ba2_gnrl_writer_tests.cpp`",
                      "`tests/unit/ba2_dx10_writer_tests.cpp`",
                      "`tests/unit/validation_api_tests.cpp`",
                      "`tests/fixtures/generated/validate_fixture_manifests.py`",
                      "`LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` are optional advisory inputs",
                      "unset variables must not block the default suite",
                      "Default acceptance must continue to pass from committed generated fixtures, writer-output archives, and policy tests alone"});
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
