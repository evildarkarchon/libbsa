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
                      "installed package-consumer runtime smoke",
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
                      "`tests/package-consumer/main.cpp`",
                      "`package_consumer_smoke` CTest gate",
                      "installed `libbsa::libbsa` target",
                      "creates, opens, validates, and extracts writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives",
                      "ctest --preset windows-msvc-debug-static -R package_consumer_smoke --output-on-failure",
                      "`tests/fixtures/generated/validate_fixture_manifests.py`",
                      "`LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` are optional advisory inputs",
                      "unset variables must not block the default suite",
                      "Default acceptance must continue to pass from committed generated fixtures, writer-output archives, and policy tests alone"});
}

TEST_CASE("coverage_audit_matrix proves installed package-consumer runtime for every family",
          "[unit][coverage_audit_matrix][docs_policy][package_consumer]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  const auto tes3 = require_markdown_section(matrix, "### TES3 BSA");
  require_all_tokens(tes3,
                     {"| Public/package-consumer API proof | Proven |",
                      "`package_consumer_smoke` runtime creates a writer-produced TES3 BSA",
                      "opens it through `archive_reader`",
                      "validates it through `validate_archive`",
                      "extracts it through sink, `extract_bytes`, and bulk extraction paths"});

  const auto tes4 = require_markdown_section(matrix, "### TES4-family BSA");
  require_all_tokens(tes4,
                     {"| Public/package-consumer API proof | Proven |",
                      "`package_consumer_smoke` runtime creates a writer-produced TES4-family BSA",
                      "opens it through `archive_reader`",
                      "validates it through `validate_archive`",
                      "extracts it through sink, `extract_bytes`, and bulk extraction paths"});

  const auto ba2_gnrl = require_markdown_section(matrix, "### BA2 GNRL");
  require_all_tokens(ba2_gnrl,
                     {"| Public/package-consumer API proof | Proven |",
                      "`package_consumer_smoke` runtime creates a writer-produced BA2 GNRL archive",
                      "opens it through `archive_reader`",
                      "validates it through `validate_archive`",
                      "extracts it through sink, `extract_bytes`, and bulk extraction paths"});

  const auto ba2_dx10 = require_markdown_section(matrix, "### BA2 DX10");
  require_all_tokens(ba2_dx10,
                     {"| Public/package-consumer API proof | Proven |",
                      "`package_consumer_smoke` runtime generates a tiny legal BC1 DXT10 DDS input inline",
                      "creates a writer-produced BA2 DX10 archive",
                      "opens it through `archive_reader`",
                      "validates it through `validate_archive`",
                      "extracts the reconstructed DDS through sink, `extract_bytes`, and bulk extraction paths"});
}

TEST_CASE("coverage_audit_matrix preserves direct validation success evidence",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  const auto tes4 = require_markdown_section(matrix, "### TES4-family BSA");
  require_all_tokens(tes4,
                     {"| Validation API behavior | Proven |",
                      "`tests/unit/validation_api_tests.cpp`",
                      "`tes4_v103.bsa`",
                      "`tes4_v104.bsa`",
                      "`tes4_v105.bsa`",
                      "extractability enabled",
                      "writer-produced TES4 archives",
                      "malformed BSA diagnostics"});

  const auto ba2_gnrl = require_markdown_section(matrix, "### BA2 GNRL");
  require_all_tokens(ba2_gnrl,
                     {"| Validation API behavior | Proven |",
                      "`tests/unit/validation_api_tests.cpp`",
                      "`ba2_gnrl_fo4.ba2`",
                      "`ba2_gnrl_sfv2.ba2`",
                      "`ba2_gnrl_sfv3.ba2`",
                      "writer-produced Starfield BA2 v3",
                      "method 0 deflate",
                      "method 3 raw LZ4 block",
                      "malformed matrix rows cover BA2 GNRL failures"});

  const auto ba2_dx10 = require_markdown_section(matrix, "### BA2 DX10");
  require_all_tokens(ba2_dx10,
                     {"| Validation API behavior | Proven |",
                      "`tests/unit/validation_api_tests.cpp`",
                      "`ba2_dx10_fo4.ba2`",
                      "`ba2_dx10_sfv3.ba2`",
                      "writer-produced Starfield BA2 v3",
                      "method 0 deflate",
                      "method 3 raw LZ4 block texture routes",
                      "malformed matrix rows cover BA2 DX10 failures"});
}

TEST_CASE("coverage_audit_matrix keeps COV-GAP-001 and COV-GAP-003 closed while preserving deferred gaps",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

  require_all_tokens(matrix,
                     {"`COV-GAP-001` validation-success gap is no longer open",
                      "`COV-GAP-003` package-consumer runtime gap is no longer open",
                      "package_consumer_smoke",
                      "installed `libbsa::libbsa` target",
                      "creates, opens, validates, and extracts writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives",
                      "`COV-GAP-002` | Low / Deferred",
                      "`COV-GAP-004` | Low / Deferred"});
  require_no_tokens(matrix,
                    {"| 1 | `COV-GAP-001`",
                     "| 1 | `COV-GAP-003`",
                     "| 2 | `COV-GAP-003`",
                     "| 3 | `COV-GAP-003`",
                     "| `COV-GAP-001` |",
                     "see `COV-GAP-001`",
                     "not currently enumerated",
                     "Direct validation success rows for Starfield v2/v3 generated fixtures are not enumerated",
                     "Direct validation success rows for Starfield v3 DX10 generated fixtures and both Starfield compression methods are not enumerated",
                     "installed-package runtime archive creation/opening for every family is intentionally not proven by default",
                     "actual archive behavior is covered by always-on unit fixture tests",
                     "S05 integrated confidence/package-release polish"});
}

TEST_CASE("compatibility_evidence names direct validation matrix and Starfield compression routes",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto compatibility_evidence = read_text_file(source_root() / "docs" / "compatibility-evidence.md");

  require_all_tokens(compatibility_evidence,
                     {"direct validation success matrix includes",
                      "`tes4_v103.bsa`",
                      "`tes4_v104.bsa`",
                      "`tes4_v105.bsa`",
                      "`ba2_gnrl_fo4.ba2`",
                      "`ba2_gnrl_sfv2.ba2`",
                      "`ba2_gnrl_sfv3.ba2`",
                      "`ba2_dx10_fo4.ba2`",
                      "`ba2_dx10_sfv3.ba2`",
                      "writer-produced Starfield BA2 v3 method 0 deflate and method 3 raw LZ4 block routes",
                      "both BA2 GNRL and BA2 DX10"});
}

TEST_CASE("public_api_reality_check routes closed validation and package-consumer proof without API redesign",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto public_api_reality_check = read_text_file(source_root() / "docs" / "public-api-reality-check.md");

  require_all_tokens(public_api_reality_check,
                     {"`COV-GAP-001` is closed by direct validation proof",
                      "Direct validation success rows now exist",
                      "The validation API shape required no helper/API redesign",
                      "method 0 deflate",
                      "method 3 raw LZ4 block",
                      "Closed. Keep validation API and docs-policy tests as the guardrail",
                      "S05 closes `COV-GAP-003`",
                      "package_consumer_smoke",
                      "installed `libbsa::libbsa` target",
                      "create, open, validate, and extract writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives",
                      "without changing the public API shape"});
  require_no_tokens(public_api_reality_check,
                    {"direct success validation rows for all variant-specific routes are not fully enumerated",
                     "if direct proof is required",
                     "Coverage matrix notes direct variant-specific validation success rows are incomplete",
                     "installed-package runtime archive creation/opening for every family is intentionally not proven by default",
                     "Package-consumer runtime proof is representative, not every-family archive runtime proof",
                     "leave installed-package every-family runtime proof to the `COV-GAP-003` route"});
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

TEST_CASE("public coverage docs stay free of internal planning identifiers",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  SECTION("coverage audit matrix")
  {
    const auto matrix = read_text_file(source_root() / "docs" / "coverage-audit-matrix.md");

    require_no_tokens(matrix,
                      {".gsd/",
                       "M001",
                       "S01"});
  }

  SECTION("compatibility evidence catalog")
  {
    const auto compatibility_evidence = read_text_file(source_root() / "docs" / "compatibility-evidence.md");

    require_no_tokens(compatibility_evidence,
                      {".gsd/",
                       "M001",
                       "S01"});
  }

  SECTION("public API reality check")
  {
    const auto public_api_reality_check = read_text_file(source_root() / "docs" / "public-api-reality-check.md");

    require_no_tokens(public_api_reality_check,
                      {".gsd/",
                       "M001",
                       "S01"});
  }
}

TEST_CASE("coverage_audit_matrix is discoverable from compatibility evidence catalog",
          "[unit][coverage_audit_matrix][docs_policy]")
{
  const auto compatibility_evidence = read_text_file(source_root() / "docs" / "compatibility-evidence.md");

  require_all_tokens(compatibility_evidence, {"coverage-audit-matrix.md"});
}
