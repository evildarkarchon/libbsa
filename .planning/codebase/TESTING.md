# Testing Patterns

**Analysis Date:** 2026-05-11

## Test Framework

**Runner:**
- Catch2 v3 via vcpkg and CTest discovery.
- Config: `tests/CMakeLists.txt` defines `libbsa_tests`, links `Catch2::Catch2WithMain`, and calls `catch_discover_tests(libbsa_tests ADD_TAGS_AS_LABELS DISCOVERY_MODE PRE_TEST DL_PATHS $<TARGET_FILE_DIR:libbsa>)`.

**Assertion Library:**
- Catch2 macros from `<catch2/catch_test_macros.hpp>`: `REQUIRE`, `REQUIRE_FALSE`, `CHECK`, `INFO`, `FAIL`, `SKIP`, `REQUIRE_THROWS_AS`. Examples: `tests/unit/result_tests.cpp`, `tests/unit/archive_path_tests.cpp`, `tests/unit/local_game_fixture_tests.cpp`.
- Static API contracts use C++ `static_assert` and `requires` expressions in `tests/unit/public_include_boundary_tests.cpp`.

**Run Commands:**
```bash
cmake --preset windows-msvc-debug-static                         # Configure static Debug build with tests
cmake --build --preset windows-msvc-debug-static                 # Build library, tests, fixture tools, benchmarks
ctest --preset windows-msvc-debug-static --output-on-failure     # Run all default tests
ctest --preset windows-msvc-debug-static -L unit --output-on-failure  # Run unit-labeled tests
ctest --preset windows-msvc-debug-static -L "fixture|malformed" --output-on-failure  # Run fixture/malformed slices
cmake --build --preset windows-msvc-debug-static --target generate_tes4_bsa_fixtures  # Regenerate TES4 fixtures
cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures  # Regenerate TES3 fixtures
cmake --build --preset windows-msvc-debug-static --target generate_ba2_gnrl_fixtures  # Regenerate BA2 GNRL fixtures
cmake --build --preset windows-msvc-debug-static --target generate_ba2_dx10_fixtures  # Regenerate BA2 DX10 fixtures
cmake --build --preset windows-msvc-debug-static --target libbsa_docs                 # Generate Doxygen docs when installed
cmake --build --preset windows-msvc-debug-static --target libbsa_benchmark_report     # Generate correctness-checked benchmark report
```

## Test File Organization

**Location:**
- Unit, fixture, policy, and integration-style tests are co-located under `tests/unit/` and compiled into one executable target in `tests/CMakeLists.txt`.
- Package consumption tests live under `tests/package-consumer/` and run as CTest script tests: `tests/package-consumer/smoke.cmake`, `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`.
- Export surface tests live under `tests/export-surface/` and run only for shared Windows builds: `tests/export-surface/check-dll-exports.cmake`.
- Fixture generators and manifest validators live under `tests/fixtures/generated/`: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/validate_fixture_manifests.py`.

**Naming:**
- Test files use `<area>_tests.cpp`: `tests/unit/binary_io_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/validation_api_tests.cpp`.
- Catch2 test names are descriptive sentences prefixed by the feature area when useful: `tes3_bsa_metadata opens generated Morrowind archives` in `tests/unit/tes3_bsa_reader_tests.cpp`, `writer_publish refuses an existing destination before writing when overwrite is disabled` in `tests/unit/writer_publish_tests.cpp`.
- Catch2 tags are bracketed and become CTest labels through `ADD_TAGS_AS_LABELS`: `[unit][fixture][tes3_bsa_metadata]`, `[unit][writer_publish][publish]`, `[requires-game-fixture][unit][compat]`.

**Structure:**
```
tests/
├── CMakeLists.txt                         # Catch2 target, fixture generator targets, CTest labels
├── unit/                                  # Main test executable sources
├── fixtures/
│   ├── README.md                          # Fixture provenance, labels, local fixture policy
│   ├── generated/
│   │   ├── archives/                      # Committed legal generated .bsa/.ba2 and JSON manifests
│   │   ├── source/                        # Committed synthetic DDS source inputs
│   │   └── validate_fixture_manifests.py  # Manifest schema/evidence validation
│   └── local/                             # Ignored local game-derived data placeholder
├── package-consumer/                      # Installed package smoke tests
└── export-surface/                        # DLL export policy tests
```

## Test Structure

**Suite Organization:**
```typescript
// C++ Catch2 pattern used in `tests/unit/archive_path_tests.cpp` and `tests/unit/tes3_bsa_reader_tests.cpp`.
namespace {
std::filesystem::path generated_archive_path(std::string_view filename) {
  return generated_archive_dir() / std::string{filename};
}
} // namespace

TEST_CASE("tes3_bsa_metadata opens generated Morrowind archives", "[unit][fixture][tes3_bsa_metadata]") {
  const auto manifest = read_json_file(generated_archive_path("tes3_success_manifest.json"));

  auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  REQUIRE(metadata.value().variant == libbsa::archive_variant::tes3);
  REQUIRE(metadata.value().file_count == manifest.at("file_count").get<std::uint32_t>());
}
```

**Patterns:**
- Put file-local helpers and test doubles in an anonymous namespace before `TEST_CASE`: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/writer_publish_tests.cpp`, `tests/unit/local_game_fixture_tests.cpp`.
- Use `REQUIRE` for setup and invariants that make later assertions unsafe, and `CHECK` for multiple independent comparisons after setup: `tests/unit/local_game_fixture_tests.cpp`, `tests/unit/compatibility_matrix_tests.cpp`.
- Assert stable error codes instead of exact messages for parser and API behavior: `tests/unit/archive_path_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/result_tests.cpp`.
- Use `INFO` to annotate looped checks over JSON manifests and policy rows: `tests/unit/docs_policy_tests.cpp`, `tests/unit/compatibility_matrix_tests.cpp`.
- Keep policy/static-boundary tests as first-class regression tests by reading source/config files: `tests/unit/docs_policy_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/benchmark_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`.

## Mocking

**Framework:** Hand-written fakes/stubs; no mocking library is configured.

**Patterns:**
```typescript
// C++ sink fake pattern from `tests/unit/tes3_bsa_reader_tests.cpp`.
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
- Mock caller-owned interfaces such as `payload_sink` and sink factories with simple test-local classes: `collecting_sink`, `partial_sink`, and `recording_sink` in `tests/unit/tes3_bsa_reader_tests.cpp`; `fnv1a32_sink` in `tests/unit/local_game_fixture_tests.cpp`.
- Use callbacks/lambdas to fake writer publish behavior at the helper boundary: `tests/unit/writer_publish_tests.cpp` passes lambdas into `libbsa::detail::publish_writer_output`.

**What NOT to Mock:**
- Do not mock parser behavior, compression codecs, generated fixture archives, or public writer/readers when a committed fixture or writer-output archive can exercise the real path. Fixture-backed tests in `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `tests/unit/ba2_dx10_extraction_tests.cpp` use real generated archives.
- Do not use `TES5Edit/` as a mutable fixture workspace or compiled test dependency. This boundary is documented in `tests/fixtures/README.md` and guarded in `.github/workflows/ci.yml`.

## Fixtures and Factories

**Test Data:**
```typescript
// C++ manifest-backed fixture pattern from `tests/unit/tes3_bsa_reader_tests.cpp`.
const auto manifest = read_json_file(generated_archive_path("tes3_success_manifest.json"));
auto opened = libbsa::archive_reader::open(generated_archive_path("tes3_success.bsa").string());
REQUIRE(opened.has_value());

for (const auto& expected : manifest.at("entries")) {
  auto found = opened.value().find(expected.at("path").get<std::string>());
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  REQUIRE(found.value()->raw_size == expected.at("raw_size").get<std::uint64_t>());
}
```

**Location:**
- Committed legal archive fixtures and manifests: `tests/fixtures/generated/archives/`.
- Synthetic DDS source fixtures: `tests/fixtures/generated/source/`.
- Fixture policy and manifest schemas: `tests/fixtures/README.md`.
- Generated archive manifest validator: `tests/fixtures/generated/validate_fixture_manifests.py`.
- Local game-derived fixtures: `tests/fixtures/local/` or `LIBBSA_GAME_FIXTURES`, skipped unless configured by `tests/unit/local_game_fixture_tests.cpp`.
- Optional BSArchPro comparison manifests: `LIBBSA_BSARCHPRO_EXPECTED` or `bsarchpro_expected.json` under `LIBBSA_GAME_FIXTURES`, consumed by `tests/unit/local_game_fixture_tests.cpp`.

## Coverage

**Requirements:** Not detected. No coverage target, coverage threshold, or coverage report command is configured in `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json`, or `.github/workflows/ci.yml`.

**View Coverage:**
```bash
# Not configured. Use CTest labels and focused fixture/policy slices instead.
ctest --preset windows-msvc-debug-static --output-on-failure
```

## Test Types

**Unit Tests:**
- Small deterministic tests for public and internal units live under `tests/unit/` and carry the `[unit]` tag: `tests/unit/result_tests.cpp`, `tests/unit/archive_path_tests.cpp`, `tests/unit/binary_io_tests.cpp`, `tests/unit/compression_router_tests.cpp`.

**Integration Tests:**
- Fixture-backed reader/extractor/writer tests use committed generated archives and writer-output archives with `[fixture]`, `[roundtrip]`, `[malformed]`, and format-specific tags: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`.
- Package and installed-target integration is covered by `package_consumer_smoke` and `package_consumer_runtime_dll_copy` in `tests/CMakeLists.txt` using `tests/package-consumer/`.
- Shared-library export integration is covered by `shared_export_surface` in `tests/CMakeLists.txt` and `tests/export-surface/check-dll-exports.cmake` when `WIN32 AND BUILD_SHARED_LIBS`.
- CI runs both static and shared Windows Debug presets from `.github/workflows/ci.yml` and verifies `TES5Edit/` stays unchanged.

**E2E Tests:**
- No separate E2E framework is used. End-to-end behavior is represented by public API open/write/extract/validate flows in Catch2 tests and package-consumer CTest scripts: `tests/unit/validation_api_tests.cpp`, `tests/unit/local_game_fixture_tests.cpp`, `tests/package-consumer/smoke.cmake`.

## Common Patterns

**Async Testing:**
```typescript
// C++ concurrency/parallel behavior is tested synchronously through worker-count options.
auto result = libbsa::detail::run_indexed_work(task_count, worker_count, [&](std::size_t index) {
  // Exercise independent indexed work and return libbsa::result<void>.
  return libbsa::result<void>{};
});
REQUIRE(result.has_value());
```
- Use worker-count controls rather than async test frameworks. Relevant surfaces are `src/detail/parallel_work.cpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `tests/unit/bulk_extraction_tests.cpp`, `tests/unit/writer_execution_options_tests.cpp`, and `tests/unit/thread_safety_docs_policy_tests.cpp`.

**Error Testing:**
```typescript
// Stable error-code pattern from `tests/unit/archive_path_tests.cpp`.
auto key = libbsa::detail::normalize_archive_path("textures/../bad.dds");
REQUIRE_FALSE(key);
REQUIRE(key.error().code == libbsa::error_code::invalid_argument);
```
- For malformed fixtures, read the malformed manifest, open/extract real fixture files, and compare expected stable error categories: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/compatibility_matrix_tests.cpp`.

---

*Testing analysis: 2026-05-11*
