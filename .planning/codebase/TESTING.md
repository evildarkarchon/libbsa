---
last_mapped: 2026-05-15
last_mapped_commit: ead68b6856b37c3ea3acbf007180951c3cc22698
---

# Testing Patterns

**Analysis Date:** 2026-05-15

## Test Framework

**Runner:**
- Catch2 v3 via vcpkg package `catch2` in `vcpkg.json`.
- Main unit executable: `libbsa_tests` defined in `tests/CMakeLists.txt`.
- Discovery: `catch_discover_tests(libbsa_tests ADD_TAGS_AS_LABELS DISCOVERY_MODE PRE_TEST DL_PATHS $<TARGET_FILE_DIR:libbsa>)` in `tests/CMakeLists.txt`.
- CTest integration is enabled by root `CMakeLists.txt` through `include(CTest)` and `add_subdirectory(tests)` when `BUILD_TESTING` and `LIBBSA_BUILD_TESTS` are on.

**Assertion Library:**
- Catch2 macros from `#include <catch2/catch_test_macros.hpp>`.
- JSON fixture assertions use `nlohmann_json` through `#include <nlohmann/json.hpp>` in tests such as `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/validation_api_tests.cpp`, and `tests/unit/compatibility_matrix_tests.cpp`.

**Run Commands:**
```bash
cmake --preset windows-msvc-debug-static              # Configure static Debug with tests
cmake --build --preset windows-msvc-debug-static      # Build library, tests, and fixture tools
ctest --preset windows-msvc-debug-static --output-on-failure

cmake --preset windows-msvc-debug-shared
cmake --build --preset windows-msvc-debug-shared
ctest --preset windows-msvc-debug-shared --output-on-failure

cmake --preset windows-msvc-release-static
cmake --build --preset windows-msvc-release-static
ctest --preset windows-msvc-release-static --output-on-failure

cmake --preset windows-msvc-release-shared
cmake --build --preset windows-msvc-release-shared
ctest --preset windows-msvc-release-shared --output-on-failure

cmake --preset windows-msvc-asan-static
cmake --build --preset windows-msvc-asan-static
ctest --preset windows-msvc-asan-static --output-on-failure
```

**CI:**
- `.github/workflows/ci.yml` runs Windows MSVC Debug static/shared, Release static/shared, and AddressSanitizer static presets.
- CI verifies `TES5Edit/` remains unchanged with `git status --short TES5Edit` after each job.

## Test File Organization

**Location:**
- Unit tests are centralized under `tests/unit/` and compiled into a single `libbsa_tests` target in `tests/CMakeLists.txt`.
- Fixture generator tools live in `tests/fixtures/generated/` and are built as separate executables: `generate_tes4_bsa_fixtures_tool`, `generate_tes3_bsa_fixtures_tool`, `generate_tes3_bsa_writer_fixtures_tool`, `generate_ba2_gnrl_fixtures_tool`, and `generate_ba2_dx10_fixtures_tool`.
- Generated archive fixtures and manifests live under `tests/fixtures/generated/archives/` and `tests/fixtures/generated/source/`.
- Package consumer smoke tests live under `tests/package-consumer/`.
- Shared export surface CTest script lives under `tests/export-surface/check-dll-exports.cmake`.

**Naming:**
- Test files use `<feature>_tests.cpp`: `tests/unit/result_tests.cpp`, `tests/unit/archive_reader_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/writer_publish_tests.cpp`.
- Test case names are descriptive sentences and usually start with the feature under test: `archive_path rejects obvious invalid virtual paths`, `validation_api reports result-level setup errors`, `writer_publish refuses an existing destination before writing when overwrite is disabled`.
- Catch2 tags are bracketed and align with CTest labels through `ADD_TAGS_AS_LABELS`: `[unit]`, `[fixture]`, `[malformed]`, `[public-api]`, `[export_surface]`, `[writer_publish]`, `[requires-game-fixture]`.

**Structure:**
```text
tests/
├── CMakeLists.txt                         # Catch2, CTest, fixture tools, package/export tests
├── unit/                                  # Catch2 unit, fixture, seam, and policy tests
│   ├── result_tests.cpp
│   ├── archive_path_tests.cpp
│   ├── ba2_dx10_malformed_tests.cpp
│   └── ...
├── fixtures/
│   ├── README.md
│   └── generated/
│       ├── generate_*.cpp                 # Legal synthetic fixture generators
│       ├── validate_fixture_manifests.py  # Python manifest/schema validation
│       ├── archives/                      # Generated .bsa/.ba2 fixtures and manifests
│       └── source/                        # Generated source DDS files and manifests
├── package-consumer/                      # Installed-package smoke project and scripts
└── export-surface/                        # Windows DLL export inspection script
```

## Test Structure

**Suite Organization:**
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

namespace
{
  std::filesystem::path generated_archive_dir()
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
  }

  nlohmann::json read_json_file(const std::filesystem::path &path)
  {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());
    return nlohmann::json::parse(stream);
  }
}

TEST_CASE("feature describes expected behavior", "[unit][fixture][feature]")
{
  auto opened = libbsa::archive_reader::open(generated_archive_path("fixture.ba2").string());

  REQUIRE(opened.has_value());
  auto metadata = opened.value().metadata();
  REQUIRE(metadata.has_value());
  CHECK(metadata.value().type == libbsa::archive_type::ba2);
}
```

**Patterns:**
- Put test-only helpers in an anonymous namespace at the top of each file.
- Use `REQUIRE` for preconditions and values needed by later lines; use `CHECK` for independent assertions where collecting multiple failures is useful.
- Use `INFO` inside loops over manifests or token lists so failures include row/case context, as in `tests/unit/ba2_dx10_malformed_tests.cpp` and `tests/unit/compatibility_matrix_tests.cpp`.
- Use `FAIL` in conversion helpers for unknown manifest values, then return a fallback only to satisfy control flow. Examples: `error_code_from_manifest` in `tests/unit/ba2_gnrl_reader_tests.cpp` and `archive_variant_from_string` in `tests/unit/local_game_fixture_tests.cpp`.
- Prefer testing stable `error_code` values rather than exact diagnostic strings. Containment checks are used only when message context is behavior, such as writer prefix tests in `tests/unit/writer_publish_tests.cpp`.

## CTest and CMake Integration

**Primary target:**
- `tests/CMakeLists.txt` defines `add_executable(libbsa_tests ...)` and links `Catch2::Catch2WithMain` plus `nlohmann_json::nlohmann_json`.
- `target_compile_definitions(libbsa_tests PRIVATE LIBBSA_SOURCE_DIR="${PROJECT_SOURCE_DIR}")` gives tests stable source-relative fixture access.
- `libbsa_link_internal_test_support(libbsa_tests)` links against `libbsa::libbsa` for static builds and embeds library sources directly for shared builds so tests can cover internal symbols without exporting them.

**Test discovery:**
- Catch2 tests are discovered at CTest time (`DISCOVERY_MODE PRE_TEST`) to support runtime DLL copying and ASan lanes.
- Catch2 tags become CTest labels via `ADD_TAGS_AS_LABELS`, so tags should remain meaningful and stable.

**Additional CTest tests:**
- `package_consumer_smoke` in `tests/CMakeLists.txt` runs `tests/package-consumer/smoke.cmake`, installs libbsa into a temporary prefix, configures an external consumer with `find_package(libbsa CONFIG REQUIRED)`, builds it, copies runtime DLLs, and runs the consumer test.
- `package_consumer_runtime_dll_copy` runs `tests/package-consumer/verify-runtime-dll-copy.cmake`.
- `shared_export_surface` runs only when `WIN32 AND BUILD_SHARED_LIBS`; it invokes `tests/export-surface/check-dll-exports.cmake` to verify expected public symbols and absence of private implementation symbols.
- `validate_fixture_manifests` runs `tests/fixtures/generated/validate_fixture_manifests.py` and is labeled `unit;fixture;malformed;compatibility_matrix`.

**Fixture generation targets:**
```cmake
add_custom_target(generate_tes4_bsa_fixtures ...)
add_custom_target(generate_tes3_bsa_fixtures ...)
add_custom_target(generate_tes3_bsa_writer_fixtures ...)
add_custom_target(generate_ba2_gnrl_fixtures ...)
add_custom_target(generate_ba2_dx10_fixtures ...)
```
- These targets are defined in `tests/CMakeLists.txt` and write committed synthetic fixtures under `tests/fixtures/generated/archives/` and `tests/fixtures/generated/source/`.
- Regenerate fixture outputs deliberately when compatibility evidence changes; do not mutate `TES5Edit/` to create fixtures.

## Mocking and Stubs

**Framework:**
- No mocking framework is used.

**Patterns:**
```cpp
class collecting_sink final : public libbsa::payload_sink
{
public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override
  {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] const std::vector<std::byte> &bytes() const noexcept { return bytes_; }

private:
  std::vector<std::byte> bytes_;
};
```
- Use small local fake implementations for public callback interfaces. `collecting_sink` and `partial_sink` in `tests/unit/ba2_gnrl_reader_tests.cpp` test extraction success and short-write failure behavior.
- Use `discard_payload_sink` internally in `src/validation.cpp` for production validation that proves extraction without retaining bytes.
- Use RAII cleanup guards for temporary file attributes or cleanup, such as `temp_file_cleanup` in `tests/unit/validation_api_tests.cpp` and `read_only_file_guard` in `tests/unit/writer_publish_tests.cpp`.
- Use `WARN` for environment-dependent skips, such as inability to create symlinks in `tests/unit/writer_publish_tests.cpp`.

**What to Mock:**
- Mock or fake caller-owned sinks and sink factories when testing extraction delivery, partial writes, duplicate request coalescing, or concurrency contracts.
- Use synthetic files and generated fixtures for archive inputs, host file race behavior, overwrite behavior, and malformed binary structures.

**What NOT to Mock:**
- Do not mock compression libraries, DirectXTex analysis, parser primitives, or archive readers when the test is meant to prove compatibility behavior.
- Do not add production shims solely for tests. Tests should use current public APIs or explicit internal seams included via `src/` include paths.
- Do not use `TES5Edit/` as a mutable test fixture location.

## Fixtures and Factories

**Generated fixtures:**
- Archive fixtures are generated by C++ tools in `tests/fixtures/generated/` and referenced by manifests under `tests/fixtures/generated/archives/`.
- Manifest-backed tests load JSON with `nlohmann::json` and iterate cases. Examples: `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/validation_api_tests.cpp`, and `tests/unit/compatibility_matrix_tests.cpp`.

**Manifest validation:**
- `tests/fixtures/generated/validate_fixture_manifests.py` validates required manifest keys, malformed case IDs, compatibility matrix shape, expected error categories, referenced archives, and evidence references.
- Keep manifest fields stable when tests depend on them: `archive`, `expected_error`, `phase`, `target_path`, `case_id`, and `requirements` are common test inputs.

**Test data helpers:**
```cpp
std::vector<std::byte> bytes_from_text(std::string_view text)
{
  std::vector<std::byte> bytes;
  bytes.reserve(text.size());
  for (const char ch : text)
  {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}
```
- Local helpers convert text, hex, and little-endian integers into fixture buffers. Examples appear in `tests/unit/validation_api_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `tests/unit/writer_publish_tests.cpp`.
- Use `std::filesystem::temp_directory_path()` plus unique counters for temporary output paths in tests, and remove stale paths before use.

**Local game fixtures:**
- Optional real-game/BSArchPro-derived fixtures are opt-in only through environment variables in `tests/unit/local_game_fixture_tests.cpp`: `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED`.
- Tests tagged `[requires-game-fixture]` call `SKIP` when local fixture variables are absent. They must not be required for normal CI.

## Coverage

**Requirements:**
- No numeric coverage threshold or coverage tooling is configured.
- Coverage is behavior-driven through unit, fixture, malformed, compatibility matrix, public boundary, package consumer, export surface, and ASan hardening lanes.

**View Coverage:**
```bash
# No coverage command is configured in CMakePresets.json or tests/CMakeLists.txt.
# Use CTest labels to scope behavior checks instead:
ctest --preset windows-msvc-debug-static --output-on-failure -L unit
ctest --preset windows-msvc-debug-static --output-on-failure -L malformed
ctest --preset windows-msvc-debug-static --output-on-failure -L public-api
ctest --preset windows-msvc-debug-static --output-on-failure -L package_consumer
```

## Test Types

**Unit Tests:**
- Scope: public API behavior, private parser/preparer seams, binary helpers, path normalization, compression routing, writer publication, validation, and public boundary policy.
- Location: `tests/unit/*.cpp`.
- Examples: `tests/unit/result_tests.cpp`, `tests/unit/archive_path_tests.cpp`, `tests/unit/parser_primitives_tests.cpp`, `tests/unit/compression_router_tests.cpp`, `tests/unit/writer_publish_tests.cpp`.

**Fixture Tests:**
- Scope: known generated archive bytes, manifests, expected metadata, extraction bytes, malformed cases, and compatibility matrix rows.
- Location: `tests/unit/*reader_tests.cpp`, `tests/unit/*malformed_tests.cpp`, and manifests under `tests/fixtures/generated/`.
- Pattern: open archive from `tests/fixtures/generated/archives/`, compare stable metadata and error codes against manifest rows, and assert all required case IDs are present.

**Policy/Static Boundary Tests:**
- Scope: public include surface, export annotations, dependency hiding, documentation structure, thread-safety docs, benchmark policy, bounded memory policy, target format policy, and TES5Edit independence.
- Examples: `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, `tests/unit/bounded_memory_policy_tests.cpp`, `tests/unit/benchmark_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`.
- These tests commonly read source files as text and assert required/forbidden tokens. Update them when intentional public boundary or documentation policy changes occur.

**Integration Tests:**
- Package integration: `tests/package-consumer/smoke.cmake` verifies installed CMake package consumption through `find_package(libbsa CONFIG REQUIRED)` in `tests/package-consumer/CMakeLists.txt`.
- DLL export integration: `tests/export-surface/check-dll-exports.cmake` inspects the shared-library export table with `dumpbin.exe`.
- CI integration: `.github/workflows/ci.yml` runs all presets and enforces the TES5Edit read-only boundary.

**E2E Tests:**
- No browser or application E2E tests exist.
- Archive-level end-to-end behavior is covered through writer-output tests, generated fixture tests, and package-consumer smoke tests.

**Benchmarks:**
- `benchmarks/libbsa_benchmarks.cpp` builds as `libbsa_benchmarks` when `LIBBSA_BUILD_BENCHMARKS` is on.
- `libbsa_benchmark_report` in root `CMakeLists.txt` writes JSON and Markdown reports under the build tree.
- Benchmark policy is tested by `tests/unit/benchmark_policy_tests.cpp`.

## Common Patterns

**Result success/failure testing:**
```cpp
auto key = libbsa::detail::normalize_archive_path("Meshes\\Foo/BAR.NIF");
REQUIRE(key);
REQUIRE(key.value().value == "meshes/foo/bar.nif");

auto bad = libbsa::detail::normalize_archive_path("textures/../bad.dds");
REQUIRE_FALSE(bad);
REQUIRE(bad.error().code == libbsa::error_code::invalid_argument);
```
- Use this for `libbsa::result<T>` APIs. See `tests/unit/archive_path_tests.cpp` and `tests/unit/result_tests.cpp`.

**Manifest-driven malformed testing:**
```cpp
for (const auto &test_case : manifest.at("cases"))
{
  const auto id = test_case.at("id").get<std::string>();
  INFO("malformed case: " << id);
  auto opened = libbsa::archive_reader::open(generated_archive_path(test_case.at("archive").get<std::string>()).string());
  REQUIRE_FALSE(opened.has_value());
  REQUIRE(opened.error().code == error_code_from_manifest(test_case.at("expected_error").get<std::string>()));
}
```
- Use `INFO` with the manifest case ID for diagnosability.
- Assert stable `error_code`, not message text.
- After loops, assert required case IDs were observed, as in `tests/unit/ba2_dx10_malformed_tests.cpp`.

**Async/concurrency testing:**
- Concurrency behavior is tested through public contract and implementation tests rather than a dedicated async test framework.
- `tests/unit/validation_api_tests.cpp` and `tests/unit/writer_publish_tests.cpp` use `static std::atomic_uint64_t counter` to create unique temp paths safely across test execution.
- `tests/package-consumer/main.cpp` demonstrates caller-owned synchronization in `byte_vector_sink_factory` with `std::mutex` for bulk extraction callbacks.
- `tests/unit/thread_safety_docs_policy_tests.cpp` verifies public documentation states thread-safety rules.

**Error testing:**
```cpp
auto published = libbsa::detail::publish_writer_output(
    archive, false, "TES4 BSA writer", [&](const std::filesystem::path &) -> libbsa::result<void>
    {
      callback_called = true;
      return {};
    });

REQUIRE_FALSE(published.has_value());
CHECK(published.error().code == libbsa::error_code::io_error);
CHECK_FALSE(callback_called);
```
- Assert side effects that must not happen after failure, such as callback suppression or preservation of existing files.
- For filesystem safety tests, read files back to prove sentinel bytes remain unchanged, as in `tests/unit/writer_publish_tests.cpp`.

**Conditional Windows behavior:**
```cpp
#if defined(_WIN32)
  // Windows-specific assertion path.
#else
  SUCCEED("behavior is covered by Windows tests");
#endif
```
- Use this pattern for Windows API or reparse-point behavior. The project is Windows-only, so non-Windows branches are minimal test-harness accommodations rather than portability requirements.

**Static public contract testing:**
```cpp
static_assert(requires(libbsa::tes3_bsa_writer &writer, std::span<const std::byte> bytes) {
  { writer.add_bytes("Meshes/Memory.nif", bytes) } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.bsa") } -> std::same_as<libbsa::result<void>>;
});
```
- Public API contract tests in `tests/unit/public_include_boundary_tests.cpp` should be updated when intentional public API changes are made.

## Adding New Tests

**For new public API:**
- Add compile-time contract coverage in `tests/unit/public_include_boundary_tests.cpp`.
- Add export annotation/static boundary coverage in `tests/unit/export_surface_policy_tests.cpp` if the API is emitted from the shared library.
- Add runtime behavior tests in a feature-specific `tests/unit/<feature>_tests.cpp` and register the file in `tests/CMakeLists.txt` under `libbsa_tests`.

**For new parser behavior:**
- Add seam/unit tests for the parser or reader under `tests/unit/`.
- Add generated fixture bytes and a manifest row under `tests/fixtures/generated/` when behavior depends on binary compatibility evidence.
- Update `tests/fixtures/generated/validate_fixture_manifests.py` if manifest schema changes.

**For new malformed hardening:**
- Add a manifest-backed case with `expected_error` and `phase` when possible.
- Add or update compatibility matrix evidence in `tests/fixtures/generated/compatibility_matrix.json` and ensure `tests/unit/compatibility_matrix_tests.cpp` still resolves references.
- Prefer `format_error` for malformed supported archive bytes and `unsupported` for unsupported profile/route evidence.

**For new writer behavior:**
- Test staging/preparation/layout/serialization seams separately when possible, following `tests/unit/writer_stage_tests.cpp` and format-specific writer tests.
- Test final public writer output through `archive_reader` round-trips and generated source files.
- Test output publication safety through `tests/unit/writer_publish_tests.cpp` if overwrite, temporary file, or host path behavior changes.

---

*Testing analysis: 2026-05-15*
