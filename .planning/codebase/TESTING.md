# Testing Patterns

**Analysis Date:** 2026-05-11

## Test Framework

**Runner:**
- Catch2 3 is the C++ test framework. `tests/CMakeLists.txt` uses `find_package(Catch2 CONFIG REQUIRED)` and links `Catch2::Catch2WithMain` into the `libbsa_tests` executable.
- CTest is the orchestration runner. `CMakeLists.txt` includes `CTest`; `tests/CMakeLists.txt` registers Catch2 cases through `catch_discover_tests(libbsa_tests ADD_TAGS_AS_LABELS DISCOVERY_MODE PRE_TEST DL_PATHS $<TARGET_FILE_DIR:libbsa>)`.
- Config: `tests/CMakeLists.txt`, root `CMakeLists.txt`, and `CMakePresets.json`.

**Assertion Library:**
- Catch2 macros from `<catch2/catch_test_macros.hpp>` are used throughout `tests/unit/*.cpp`.
- Use `REQUIRE` for preconditions or when later assertions depend on success, `CHECK` for independent observations after setup succeeds, `REQUIRE_FALSE` and `CHECK_FALSE` for negative cases, `REQUIRE_THROWS_AS` for programmer-misuse exception checks, `INFO` for loop context, `FAIL` for unreachable conversion branches, and `SKIP` for opt-in local fixture gates. Examples: `tests/unit/result_tests.cpp`, `tests/unit/validation_api_tests.cpp`, `tests/unit/local_game_fixture_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`.

**Run Commands:**
```bash
cmake --preset windows-msvc-debug-static              # Configure static Windows/MSVC build with tests
cmake --build --preset windows-msvc-debug-static      # Build libbsa and libbsa_tests
ctest --preset windows-msvc-debug-static --output-on-failure  # Run all discovered CTest cases
cmake --preset windows-msvc-debug-shared              # Configure shared Windows/MSVC build with tests
cmake --build --preset windows-msvc-debug-shared      # Build shared variant
ctest --preset windows-msvc-debug-shared --output-on-failure  # Run shared variant tests
```

**Watch Mode:**
- Not detected. `CMakePresets.json` defines static and shared configure/build/test presets, but no watch command.

**Coverage:**
- Not detected. No `coverage`, `gcov`, `llvm-cov`, `OpenCppCoverage`, `lcov`, or Codecov setup appears in `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/ci.yml`, or `tests/`.

## Test File Organization

**Location:**
- Unit and policy tests are centralized under `tests/unit/*.cpp`. There are 36 unit test files, all compiled into the single `libbsa_tests` target in `tests/CMakeLists.txt`.
- Fixture generator source lives under `tests/fixtures/generated/*.cpp`: `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.
- Committed legal generated archives, manifests, and DDS sources live under `tests/fixtures/generated/archives/` and `tests/fixtures/generated/source/`.
- Package-consumer integration smoke tests live under `tests/package-consumer/`: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`, `tests/package-consumer/verify-runtime-dll-copy.cmake`.
- Local game-derived archives belong only under ignored `tests/fixtures/local/` or outside the repo via `LIBBSA_GAME_FIXTURES`; this policy is documented in `tests/fixtures/README.md` and enforced by `tests/unit/validation_policy_tests.cpp`.

**Naming:**
- Use `<surface>_tests.cpp` for test files: `tests/unit/binary_io_tests.cpp`, `tests/unit/compression_router_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`.
- Use `TEST_CASE` names that state the behavior, not the implementation detail alone: `"archive_reader open reports I/O errors for missing host files"` in `tests/unit/archive_reader_tests.cpp`, `"BA2 GNRL disk payload streaming rejects source size changes"` in `tests/unit/ba2_gnrl_writer_tests.cpp`.
- Use Catch2 tags to classify scope and behavior: `[unit]`, `[fixture]`, `[roundtrip]`, `[compat]`, `[malformed]`, `[slow]`, `[requires-game-fixture]`, `[public-api]`. The label taxonomy is documented in `tests/fixtures/README.md` and checked by `tests/unit/validation_policy_tests.cpp`.

**Structure:**
```text
tests/
├── CMakeLists.txt                         # Catch2 target, fixture generator targets, CTest tests
├── unit/                                  # Catch2 unit, fixture, policy, roundtrip, malformed tests
├── fixtures/
│   ├── README.md                          # Fixture provenance, label, and TES5Edit boundary policy
│   ├── generated/
│   │   ├── generate_*_fixtures.cpp        # Legal synthetic fixture generators
│   │   ├── archives/                      # Committed generated .bsa/.ba2/.json contracts
│   │   └── source/                        # Committed generated DDS source inputs
│   └── local/                             # Ignored local-only game-derived fixtures
└── package-consumer/                      # Installed package and runtime DLL smoke tests
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

TEST_CASE("surface behavior description", "[unit][fixture][surface_tag]") {
  auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());
  REQUIRE(opened.has_value());

  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  CHECK(metadata.value().type == libbsa::archive_type::bsa);
}
```
- This pattern is used in `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `tests/unit/ba2_dx10_extraction_tests.cpp`.

**Patterns:**
- Put reusable helpers, test fakes, and conversion routines in an anonymous namespace at the top of each test file: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/local_game_fixture_tests.cpp`, `tests/unit/validation_api_tests.cpp`.
- Use `static_assert` and C++20 `requires` expressions for public compile-time API contracts: `tests/unit/archive_reader_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.
- Use manifest-driven loops for generated fixtures and malformed cases, with `INFO` identifying the current archive or case: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/compatibility_matrix_tests.cpp`.
- Use `SECTION` for closely related branches inside one behavior case: `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/dds_layout_tests.cpp`.
- Prefer deterministic temp output directories under `std::filesystem::temp_directory_path()` for writer tests: `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/validation_api_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`.
- Use policy tests that inspect repository files when a convention is itself a requirement: `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, `tests/unit/validation_policy_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.

## Mocking

**Framework:** No mocking framework is used.

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
- Use concrete test doubles for public callback interfaces instead of mocks. `collecting_sink` appears in `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, and `tests/unit/tes4_bsa_writer_tests.cpp`.
- Use deliberately failing or partial fakes to prove error handling: `partial_sink` and `recording_sink` in `tests/unit/tes3_bsa_reader_tests.cpp`, `fnv1a32_sink` in `tests/unit/local_game_fixture_tests.cpp`, bulk extraction factories in `tests/unit/bulk_extraction_tests.cpp`.

**What to Mock:**
- Mock only caller-owned callback surfaces and local test I/O seams, such as `payload_sink` and `bulk_extract_sink_factory` from `include/libbsa/archive.hpp`.
- Use in-memory byte vectors and temporary files for source/archive payloads: `tests/unit/payload_stream_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`.

**What NOT to Mock:**
- Do not mock parser, writer, compression, or texture analysis paths when a fixture or generated archive can exercise the real behavior. Use `tests/fixtures/generated/archives/` and writer roundtrip tests instead, as in `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, and `tests/unit/ba2_writer_execution_tests.cpp`.
- Do not use `TES5Edit/` as a mutable fixture workspace or committed fixture source. This boundary is documented in `tests/fixtures/README.md` and enforced through policy tests in `tests/unit/validation_policy_tests.cpp`.

## Fixtures and Factories

**Test Data:**
```cpp
std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}
```
- This manifest-backed pattern appears in `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, and `tests/unit/validation_api_tests.cpp`.
- Generated fixture targets are declared in `tests/CMakeLists.txt`: `generate_tes4_bsa_fixtures`, `generate_tes3_bsa_fixtures`, `generate_tes3_bsa_writer_fixtures`, `generate_ba2_gnrl_fixtures`, and `generate_ba2_dx10_fixtures`.
- Fixture manifests encode expected public metadata, payload bytes or hashes, malformed-case error codes, and compatibility evidence. See `tests/fixtures/README.md`, `tests/fixtures/generated/archives/tes3_success_manifest.json`, and `tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json`.
- `LIBBSA_SOURCE_DIR` is injected by `tests/CMakeLists.txt` so tests can locate committed fixtures without depending on process working directory.
- Local BSArchPro-derived comparison data is opt-in through `LIBBSA_GAME_FIXTURES` or `LIBBSA_BSARCHPRO_EXPECTED` in `tests/unit/local_game_fixture_tests.cpp`.

**Location:**
- Legal committed archive fixtures: `tests/fixtures/generated/archives/`.
- Legal committed DDS source fixtures: `tests/fixtures/generated/source/`.
- Fixture generators: `tests/fixtures/generated/*.cpp`.
- Local ignored game archives: `tests/fixtures/local/`, protected by `.gitignore`.
- Fixture policy and provenance: `tests/fixtures/README.md`.

## Coverage

**Requirements:** None enforced. Coverage tooling and coverage thresholds are not configured in `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json`, or `.github/workflows/ci.yml`.

**View Coverage:**
```bash
# Not detected: no repo-native coverage command is configured.
```

## Test Types

**Unit Tests:**
- Use Catch2 in `tests/unit/*.cpp` for public API, internal helpers, parser dispatch, compression codecs, archive path normalization, binary I/O, validation, thread-safety docs, and policy checks.
- Examples: `tests/unit/result_tests.cpp`, `tests/unit/archive_path_tests.cpp`, `tests/unit/binary_io_tests.cpp`, `tests/unit/compression_router_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.

**Integration Tests:**
- Use generated fixture archives and manifests to exercise real parser, extraction, writer, compression, DDS, validation, and roundtrip behavior: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/validation_api_tests.cpp`.
- Use package-consumer CMake tests to verify install/export and runtime DLL copy behavior: `tests/package-consumer/smoke.cmake`, `tests/package-consumer/verify-runtime-dll-copy.cmake`, registered from `tests/CMakeLists.txt`.
- Use Python only for manifest validation through `tests/fixtures/generated/validate_fixture_manifests.py`, registered as the `validate_fixture_manifests` CTest test in `tests/CMakeLists.txt`.

**E2E Tests:**
- Not used as a separate browser/UI-style category. The closest end-to-end coverage is archive writer to reader roundtrip and package-consumer smoke coverage in `tests/unit/*writer*_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`, and `tests/package-consumer/smoke.cmake`.

**Policy Tests:**
- Repository policy is tested as code where it affects future implementation: `tests/unit/docs_policy_tests.cpp` for Doxygen input boundaries, `tests/unit/thread_safety_docs_policy_tests.cpp` for thread-safety documentation, `tests/unit/validation_policy_tests.cpp` for Windows-only presets and fixture policy, `tests/unit/public_include_boundary_tests.cpp` for public header isolation.

## Common Patterns

**Async Testing:**
```cpp
libbsa::write_execution_options execution;
execution.worker_count = 4U;

auto written = writer.write_to(output.string(), execution);
REQUIRE(written.has_value());
```
- There is no async/await test pattern. Concurrency-related behavior is tested through explicit `worker_count` options and deterministic output comparison in `tests/unit/bsa_writer_execution_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`, `tests/unit/writer_execution_options_tests.cpp`, and `tests/unit/bulk_extraction_tests.cpp`.

**Error Testing:**
```cpp
auto opened = libbsa::archive_reader::open(generated_archive_path(archive).string());

REQUIRE_FALSE(opened.has_value());
REQUIRE(opened.error().code == expected);
```
- Compare stable `error_code` values, not full diagnostic messages, for ordinary parser and writer failures. This is the dominant pattern in `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `tests/unit/ba2_dx10_malformed_tests.cpp`.
- Use `REQUIRE_THROWS_AS` only for programmer misuse of `result`, as in `tests/unit/result_tests.cpp`.
- Use `SKIP` for environment-dependent local tests rather than failing default CI: `tests/unit/local_game_fixture_tests.cpp`, `tests/unit/validation_api_tests.cpp` for sparse-file support.

**Regression Testing:**
- Add a generated fixture or manifest row for archive-format regressions when possible, then assert the public metadata, payload bytes, or stable error code. Examples include TES3 raw-offset regression in `tests/fixtures/generated/archives/tes3_raw_offset_absolute_regression.bsa`, malformed manifests under `tests/fixtures/generated/archives/`, and compatibility matrix validation in `tests/unit/compatibility_matrix_tests.cpp`.
- For wrong-output writer bugs, compare reopened metadata and extracted bytes, or compare serialized bytes between serial and parallel outputs. Examples: `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`.

**CI Testing:**
```bash
cmake --preset ${{ matrix.preset }}
cmake --build --preset ${{ matrix.preset }}
ctest --preset ${{ matrix.preset }} --output-on-failure
```
- CI runs Windows MSVC static and shared presets from `.github/workflows/ci.yml` and `CMakePresets.json`.
- CI provisions CMake `4.3.2` explicitly and verifies `cmake --version` in `.github/workflows/ci.yml`.
- CI verifies `TES5Edit/` stays read-only by checking `git status --short TES5Edit` in `.github/workflows/ci.yml`.

---

*Testing analysis: 2026-05-11*
