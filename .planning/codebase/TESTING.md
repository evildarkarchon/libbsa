# Testing Patterns

**Analysis Date:** 2026-05-10

## Test Framework

**Runner:**
- Catch2 3 through vcpkg is the C++ test runner. It is declared in `vcpkg.json`, discovered by `find_package(Catch2 CONFIG REQUIRED)` in `tests/CMakeLists.txt`, and linked as `Catch2::Catch2WithMain`.
- CTest is the orchestration layer. `tests/CMakeLists.txt` uses `catch_discover_tests(libbsa_tests ADD_TAGS_AS_LABELS DISCOVERY_MODE PRE_TEST DL_PATHS $<TARGET_FILE_DIR:libbsa>)`, so Catch2 tags become selectable CTest labels.
- The default test executable is `libbsa_tests`, built from 36 files under `tests/unit/` with 243 `TEST_CASE` definitions.
- Python 3 is used for fixture-manifest validation through the CTest case `validate_fixture_manifests`, which runs `tests/fixtures/generated/validate_fixture_manifests.py`.
- Installed-package integration is tested by CTest case `package_consumer_smoke`, which runs `tests/package-consumer/smoke.cmake` against `tests/package-consumer/main.cpp`.

**Assertion Library:**
- Use Catch2 macros from `<catch2/catch_test_macros.hpp>`.
- Use `REQUIRE`/`REQUIRE_FALSE` for preconditions and fatal setup assertions, `CHECK`/`CHECK_FALSE` for non-fatal grouped assertions, `REQUIRE_THROWS_AS` for programmer-error exception contracts, `INFO` for loop/matrix context, and `SKIP` for opt-in or platform-dependent cases.

**Run Commands:**
```bash
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static --output-on-failure

cmake --preset windows-msvc-debug-shared
cmake --build --preset windows-msvc-debug-shared
ctest --preset windows-msvc-debug-shared --output-on-failure

cmake --build --preset windows-msvc-debug-static --target libbsa_benchmark_report
```

## Test File Organization

**Location:**
- Unit, policy, fixture, writer, reader, and validation tests live in `tests/unit/*.cpp`.
- Generated legal fixtures live under `tests/fixtures/generated/archives/` and `tests/fixtures/generated/source/`.
- Fixture generator programs live under `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, and `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.
- Local game-derived fixtures are ignored under `tests/fixtures/local/` except `tests/fixtures/local/.gitkeep`; opt-in tests read `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` in `tests/unit/local_game_fixture_tests.cpp`.
- Package-consumer smoke tests live in `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`, and `tests/package-consumer/smoke.cmake`.

**Naming:**
- Test files use `<feature>_tests.cpp`, such as `tests/unit/archive_path_tests.cpp`, `tests/unit/binary_io_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/thread_safety_docs_policy_tests.cpp`.
- Test names are behavior statements, for example `result exposes stable error codes` in `tests/unit/result_tests.cpp`, `tes4_bsa_malformed_open rejects count-derived table spans before allocation` in `tests/unit/tes4_bsa_reader_tests.cpp`, and `BA2 DX10 writer refuses to overwrite existing output by default and preserves bytes` in `tests/unit/ba2_dx10_writer_tests.cpp`.
- Tags are bracketed Catch2 labels such as `[unit]`, `[fixture]`, `[roundtrip]`, `[compat]`, `[malformed]`, `[compression]`, `[public-api]`, and `[requires-game-fixture]`. The label taxonomy is documented in `tests/fixtures/README.md` and checked in `tests/unit/validation_policy_tests.cpp`.

**Structure:**
```text
tests/
├── CMakeLists.txt                         # Builds libbsa_tests, fixture generators, CTest cases
├── unit/                                  # Catch2 source files
├── fixtures/
│   ├── README.md                          # Fixture, local corpus, Windows-only, and label policy
│   ├── generated/
│   │   ├── archives/                      # Committed legal BSA/BA2 fixtures and manifests
│   │   ├── source/                        # Committed generated DDS/source fixtures
│   │   └── validate_fixture_manifests.py  # Manifest schema/policy validator
│   └── local/.gitkeep                     # Ignored local-only game fixture location
└── package-consumer/                      # Installed package smoke project
```

## Test Structure

**Suite Organization:**
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_path(std::string_view filename) {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives" /
         std::string{filename};
}

} // namespace

TEST_CASE("tes4_bsa_detection opens byte-driven TES4-family BSA variants",
          "[unit][fixture][tes4_bsa_detection]") {
  auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v103.bsa").string());

  REQUIRE(opened.has_value());
}
```

**Patterns:**
- Put file-local helpers, fake sinks, JSON readers, temp-directory helpers, and byte conversion helpers in an anonymous namespace, as in `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, and `tests/unit/validation_api_tests.cpp`.
- Use `static_assert` to lock public compile-time contracts in `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/archive_reader_tests.cpp`, `tests/unit/bulk_extraction_tests.cpp`, and `tests/unit/result_tests.cpp`.
- Use manifest-driven loops for generated fixture expectations. `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, and `tests/unit/validation_api_tests.cpp` parse JSON manifests and compare stable metadata, paths, compression routes, and extracted bytes.
- Use `SECTION` for tightly related variants within one behavior, as in `tests/unit/dds_layout_tests.cpp`, `tests/unit/writer_execution_options_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/tes4_bsa_reader_tests.cpp`.
- Use source-text policy tests for public boundary, docs, fixtures, benchmark, target-format guide, and CI contracts. Examples are `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/benchmark_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`, and `tests/unit/validation_policy_tests.cpp`.

## Mocking

**Framework:** Not used.

**Patterns:**
```cpp
class collecting_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

 private:
  std::vector<std::byte> bytes_;
};
```

**What to Mock:**
- Use local fake implementations for libbsa abstract callback points: `payload_sink` in `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, and `tests/unit/ba2_dx10_extraction_tests.cpp`; `bulk_extract_sink_factory` in `tests/unit/bulk_extraction_tests.cpp`; and hash-only streaming sinks in `tests/unit/local_game_fixture_tests.cpp`.
- Use temp files and source mutation to simulate I/O races and publish failures in writer tests such as `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.

**What NOT to Mock:**
- Do not mock archive parser/writer behavior for compatibility proofs. Reopen writer output with `libbsa::archive_reader::open`, compare metadata, and extract payloads as done in `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.
- Do not use `TES5Edit/` as a mutable fixture source or output location. This is documented in `tests/fixtures/README.md`, enforced in `.github/workflows/ci.yml`, and covered by policy tests in `tests/unit/validation_policy_tests.cpp`.
- Do not replace codec routes with mocks for compression behavior. Use real `libdeflate`/`lz4` wrappers through `tests/unit/deflate_codec_tests.cpp`, `tests/unit/lz4_codec_tests.cpp`, and `tests/unit/compression_router_tests.cpp`.

## Fixtures and Factories

**Test Data:**
```cpp
std::vector<std::byte> bytes_from_hex(std::string_view hex) {
  REQUIRE(hex.size() % 2U == 0U);
  std::vector<std::byte> bytes;
  bytes.reserve(hex.size() / 2U);
  for (std::size_t offset = 0; offset < hex.size(); offset += 2U) {
    const auto pair = std::string{hex.substr(offset, 2U)};
    bytes.push_back(static_cast<std::byte>(std::stoul(pair, nullptr, 16)));
  }
  return bytes;
}
```

**Location:**
- Committed fixture archives and manifests are under `tests/fixtures/generated/archives/`. They are generated legal data with provenance fields and are validated by `tests/fixtures/generated/validate_fixture_manifests.py`.
- DDS source fixtures are under `tests/fixtures/generated/source/` and are used by `tests/unit/ba2_dx10_writer_tests.cpp` and `tests/unit/ba2_writer_execution_tests.cpp`.
- The fixture policy in `tests/fixtures/README.md` is authoritative for generated data, local game fixtures, compatibility evidence, Windows-only testing policy, and `TES5Edit/` restrictions.
- Regeneration targets are declared in `tests/CMakeLists.txt`: `generate_tes4_bsa_fixtures`, `generate_tes3_bsa_fixtures`, `generate_tes3_bsa_writer_fixtures`, `generate_ba2_gnrl_fixtures`, and `generate_ba2_dx10_fixtures`.

## Coverage

**Requirements:** No line or branch coverage threshold is configured. There is no LCOV, gcov, llvm-cov, Codecov, or `--coverage` target in `CMakeLists.txt`, `CMakePresets.json`, or `.github/workflows/ci.yml`.

**View Coverage:**
```bash
# Not configured.
```

Coverage is behavioral and policy-driven:
- CTest labels select meaningful risk areas via Catch2 tags in `tests/unit/*.cpp`.
- `tests/unit/compatibility_matrix_tests.cpp` and `tests/fixtures/generated/validate_fixture_manifests.py` check that malformed coverage spans required families and categories.
- `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, and `tests/unit/target_format_policy_tests.cpp` keep public API/docs contracts tied to source text.
- `.github/workflows/ci.yml` runs both `windows-msvc-debug-static` and `windows-msvc-debug-shared` presets, then checks `git status --short TES5Edit`.

## Test Types

**Unit Tests:**
- Use for small deterministic helpers and API behavior: `tests/unit/result_tests.cpp`, `tests/unit/archive_path_tests.cpp`, `tests/unit/binary_io_tests.cpp`, `tests/unit/deflate_codec_tests.cpp`, `tests/unit/lz4_codec_tests.cpp`, and `tests/unit/dds_layout_tests.cpp`.

**Integration Tests:**
- Use fixture-backed parser/extraction tests for real generated archive bytes: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, and `tests/unit/ba2_dx10_extraction_tests.cpp`.
- Use writer round-trip tests that write archives, reopen through the public reader, and verify metadata/extraction: `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.
- Use installed package smoke coverage through `tests/package-consumer/smoke.cmake` and `tests/package-consumer/main.cpp`.

**E2E Tests:**
- No UI/browser E2E tests are used. The closest end-to-end checks are the package-consumer CTest case in `tests/package-consumer/` and writer/read/extract round trips in `tests/unit/*writer_tests.cpp`.

**Policy Tests:**
- Use source-text tests when repository contracts are part of the deliverable: public boundary in `tests/unit/public_include_boundary_tests.cpp`, docs in `tests/unit/docs_policy_tests.cpp`, thread-safety docs in `tests/unit/thread_safety_docs_policy_tests.cpp`, benchmark policy in `tests/unit/benchmark_policy_tests.cpp`, fixture policy in `tests/unit/validation_policy_tests.cpp`, and target-format guide coverage in `tests/unit/target_format_policy_tests.cpp`.

**Opt-in Compatibility Tests:**
- Use `[requires-game-fixture]` and `SKIP` by default for local game or BSArchPro-derived evidence in `tests/unit/local_game_fixture_tests.cpp`. These tests require `LIBBSA_GAME_FIXTURES` or `LIBBSA_BSARCHPRO_EXPECTED` and must not gate default acceptance.

## Common Patterns

**Async Testing:**
```cpp
libbsa::write_execution_options execution;
execution.worker_count = 2U;

auto written = writer.write_to(archive.string(), execution);
REQUIRE(written.has_value());
```

No async framework is used. Concurrency coverage is synchronous and worker-count based in `tests/unit/writer_execution_options_tests.cpp`, `tests/unit/bsa_writer_execution_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`, and `tests/unit/bulk_extraction_tests.cpp`.

**Error Testing:**
```cpp
auto opened = libbsa::archive_reader::open("missing/example.bsa");

REQUIRE_FALSE(opened.has_value());
REQUIRE(opened.error().code == libbsa::error_code::io_error);
```

Use stable `error_code` assertions for result failures. Use malformed fixture manifests for matrix-driven parser/validation failures in `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/validation_api_tests.cpp`, and `tests/fixtures/generated/compatibility_matrix.json`.

---

*Testing analysis: 2026-05-10*
