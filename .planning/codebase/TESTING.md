# Testing Patterns

**Analysis Date:** 2026-05-12

## Test Framework

**Runner:**
- Catch2 3 via `Catch2::Catch2WithMain`
- Config: `tests/CMakeLists.txt`

**Assertion Library:**
- Catch2 assertion macros from `#include <catch2/catch_test_macros.hpp>` as used in `tests/unit/result_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, and `tests/unit/validation_policy_tests.cpp`

**Run Commands:**
```bash
cmake --build --preset windows-msvc-debug-static      # Build library and tests (`README.md`)
ctest --preset windows-msvc-debug-static --output-on-failure  # Run default Windows test suite (`README.md`)
cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures  # Refresh committed fixtures (`tests/fixtures/README.md`)
```

## Test File Organization

**Location:**
- Primary unit and policy tests live in `tests/unit/*.cpp`.
- Build/package smoke coverage lives in `tests/package-consumer/` and `tests/export-surface/`.
- Committed fixture inputs, generated archives, manifests, and generators live in `tests/fixtures/generated/` and are documented in `tests/fixtures/README.md`.

**Naming:**
- Use `<area>_tests.cpp` for Catch2 sources, such as `tests/unit/archive_reader_tests.cpp`, `tests/unit/bulk_extraction_tests.cpp`, and `tests/unit/thread_safety_docs_policy_tests.cpp`.
- Use `generate_*_fixtures.cpp` for helper executables that synthesize committed fixture archives, such as `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` and `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.

**Structure:**
```text
tests/
├── CMakeLists.txt
├── unit/                       # Catch2 tests and policy/static-boundary checks
├── fixtures/
│   ├── README.md               # Fixture policy and label taxonomy
│   └── generated/
│       ├── archives/           # Committed legal archives + manifests
│       ├── source/             # Synthetic source payloads
│       ├── generate_*.cpp      # Fixture generator executables
│       └── validate_fixture_manifests.py
├── package-consumer/           # Installed-package smoke test project
└── export-surface/             # Shared-library export verification CMake scripts
```

## Test Structure

**Suite Organization:**
```cpp
namespace {

std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path, std::ios::binary};
  return nlohmann::json::parse(stream);
}

class collecting_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

 private:
  std::vector<std::byte> bytes_;
};

} // namespace

TEST_CASE("ba2_dx10_malformed manifest cases fail with stable errors",
          "[unit][fixture][malformed][ba2_dx10_malformed]") {
  // ...
}
```
- This pattern is taken directly from `tests/unit/ba2_dx10_malformed_tests.cpp` and mirrors many other suites under `tests/unit/`.

**Patterns:**
- Define tiny file-local helpers and test doubles in an anonymous namespace at the top of each file (`tests/unit/archive_reader_tests.cpp`, `tests/unit/bulk_extraction_tests.cpp`, `tests/unit/local_game_fixture_tests.cpp`).
- Use descriptive sentence-style `TEST_CASE` names plus tag taxonomies, for example `"[unit][fixture][malformed][ba2_dx10_malformed]"` in `tests/unit/ba2_dx10_malformed_tests.cpp`.
- Use `INFO(...)` to annotate loops and matrix-style cases before assertions, as in `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/validation_policy_tests.cpp`, and `tests/unit/compatibility_matrix_tests.cpp`.
- Mix `REQUIRE` for prerequisites and `CHECK` for multiple postconditions in the same scenario, as in `tests/unit/tes4_bsa_writer_tests.cpp` and `tests/unit/bulk_extraction_tests.cpp`.
- Add compile-time contract checks with `static_assert` alongside runtime tests for public API shape, as in `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/writer_ownership_tests.cpp`, and `tests/unit/archive_reader_tests.cpp`.

## Mocking

**Framework:** None.

**Patterns:**
```cpp
class recording_sink_factory final : public libbsa::bulk_extract_sink_factory {
 public:
  libbsa::result<std::unique_ptr<libbsa::payload_sink>> create(std::string_view path,
                                                               const libbsa::entry_metadata&) override {
    // capture calls, optionally fail, and return a concrete sink
  }
};

class partial_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    return bytes.empty() ? 0U : bytes.size() - 1U;
  }
};
```
- This concrete-test-double style is used in `tests/unit/bulk_extraction_tests.cpp`.
- Additional examples include `collecting_sink` in `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, and `tests/unit/ba2_dx10_malformed_tests.cpp`, plus `fnv1a32_sink` in `tests/unit/local_game_fixture_tests.cpp`.

**What to Mock:**
- Mock callback-style extension points by implementing small concrete subclasses of `libbsa::payload_sink` and `libbsa::bulk_extract_sink_factory`.
- Simulate partial writes, sink-factory failures, or byte capture with purpose-built fakes in the test file instead of a general mocking library.

**What NOT to Mock:**
- Do not mock archive parsing contracts when committed generated fixtures exist; use real archives from `tests/fixtures/generated/archives/`.
- Do not mock public packaging/install behavior; use the real package-consumer smoke project in `tests/package-consumer/` and the export-surface checks in `tests/export-surface/`.

## Fixtures and Factories

**Test Data:**
```cpp
const auto manifest = read_json_file(generated_archive_path("ba2_dx10_malformed_manifest.json"));

for (const auto& test_case : manifest.at("cases")) {
  auto opened = libbsa::archive_reader::open(archive.string());
  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == expected_error);
}
```
- Manifest-driven validation is the default fixture pattern in `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `tests/unit/validation_api_tests.cpp`.

**Location:**
- Committed legal archive fixtures: `tests/fixtures/generated/archives/`
- Synthetic source payloads: `tests/fixtures/generated/source/`
- Generator executables: `tests/fixtures/generated/generate_*.cpp`
- Manifest schema validator: `tests/fixtures/generated/validate_fixture_manifests.py`
- Fixture policy and provenance rules: `tests/fixtures/README.md`

## Coverage

**Requirements:** None enforced.
- No line-coverage target, coverage threshold, or coverage report command is defined in `CMakeLists.txt` or `tests/CMakeLists.txt`.
- Quality is enforced through breadth of behavior tests plus static-boundary, documentation, and packaging policy suites in files such as `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, and `tests/unit/benchmark_policy_tests.cpp`.

**View Coverage:**
```bash
Not detected
```

## Test Types

**Unit Tests:**
- Core API and helper behavior tests cover `result`, parsers, readers, codecs, path normalization, payload streaming, validation, and writer internals in `tests/unit/result_tests.cpp`, `tests/unit/parser_primitives_tests.cpp`, `tests/unit/payload_stream_tests.cpp`, `tests/unit/deflate_codec_tests.cpp`, and related files.

**Integration Tests:**
- Round-trip and real-file workflow tests create archives with public writers and reopen them through `archive_reader`, such as `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.
- Installed-package smoke tests compile and run an external consumer project via `tests/package-consumer/CMakeLists.txt` plus the `package_consumer_smoke` and `package_consumer_runtime_dll_copy` tests registered in `tests/CMakeLists.txt`.
- Shared-library export validation runs through `tests/export-surface/check-dll-exports.cmake` when `WIN32 AND BUILD_SHARED_LIBS` in `tests/CMakeLists.txt`.

**E2E Tests:**
- No separate GUI/service E2E framework is used.
- The closest end-to-end coverage is package installation, archive round-trip, and fixture-generation validation through CTest entries in `tests/CMakeLists.txt`.

## Common Patterns

**Async Testing:**
```cpp
libbsa::bulk_extract_options options;
options.worker_count = worker_count;

auto extracted = reader.extract_entries(requests, sink_factory, options);
REQUIRE(extracted.has_value());
```
- The codebase is synchronous at the API level; concurrency is tested by varying `worker_count` and inspecting deterministic outputs, as in `tests/unit/bulk_extraction_tests.cpp` and writer execution tests such as `tests/unit/writer_execution_options_tests.cpp`.

**Error Testing:**
```cpp
auto result = libbsa::archive_reader::open("missing/example.bsa");

REQUIRE_FALSE(result.has_value());
REQUIRE(result.error().code == libbsa::error_code::io_error);
```
- Prefer checking stable `error.code` values, with message substring checks only when the text itself is part of the contract. See `tests/unit/archive_reader_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, and `tests/unit/writer_publish_tests.cpp`.

## Labels and Discovery

- `tests/CMakeLists.txt` uses `catch_discover_tests(libbsa_tests ADD_TAGS_AS_LABELS DISCOVERY_MODE PRE_TEST ...)`, so Catch2 tags become CTest labels automatically.
- The label taxonomy is documented in `tests/fixtures/README.md` and enforced by `tests/unit/validation_policy_tests.cpp`:
  - `unit`
  - `fixture`
  - `roundtrip`
  - `compat`
  - `malformed`
  - `slow`
  - `requires-game-fixture`
- Optional local-corpus tests in `tests/unit/local_game_fixture_tests.cpp` use `SKIP(...)` unless `LIBBSA_GAME_FIXTURES` or `LIBBSA_BSARCHPRO_EXPECTED` is configured.

## Validation Practices

- Keep `TES5Edit/` out of the test workspace; `tests/fixtures/README.md`, `README.md`, and CI in `.github/workflows/ci.yml` all treat it as read-only.
- Validate fixture metadata separately from parser behavior with `validate_fixture_manifests` in `tests/CMakeLists.txt` and `tests/fixtures/generated/validate_fixture_manifests.py`.
- Treat docs and packaging as testable contracts: `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, `tests/unit/benchmark_policy_tests.cpp`, and `tests/unit/validation_policy_tests.cpp` read source/docs/config files directly and assert required tokens.
- Keep benchmark tooling opt-in and outside default CTest timing gates, enforced by `tests/unit/benchmark_policy_tests.cpp` and documented in `benchmarks/README.md`.

---

*Testing analysis: 2026-05-12*
