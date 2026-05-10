# Phase 01 Research: Foundation, API Boundary, and Test Harness

**Phase:** 01 — Foundation, API Boundary, and Test Harness  
**Researched:** 2026-05-07  
**Confidence:** HIGH  
**Mode:** forced via `--research`

## Research Goal

Answer: what must be known to plan Phase 1 well?

Phase 1 must turn the repository into a buildable C++20 library foundation without implementing real BSA/BA2 parsing. The critical planning constraints are the CMake/vcpkg package boundary, public header cleanliness, a local C++20 result/error surface, Catch2/CTest labels, fixture policy, CI validation, and the read-only `TES5Edit/` boundary.

## Source Inputs

- `.planning/phases/01-foundation-api-boundary-and-test-harness/01-SPEC.md` — locked Phase 1 requirements and acceptance criteria.
- `.planning/phases/01-foundation-api-boundary-and-test-harness/01-CONTEXT.md` — implementation decisions D-01 through D-35.
- `.planning/ROADMAP.md` — Phase 1 goal and requirement IDs.
- `.planning/REQUIREMENTS.md` — FND-01 through FND-07 and DOC-04.
- `.planning/research/STACK.md` — project stack and dependency policies.
- `.planning/research/ARCHITECTURE.md` — public/internal boundary guidance.
- `.planning/research/PITFALLS.md` — pitfalls relevant to public dependency leakage, result/error collapse, weak fixtures, and `TES5Edit/` mutation.
- Context7 `/kitware/cmake` — package export and header file-set examples.
- Context7 `/catchorg/catch2` — `catch_discover_tests(... ADD_TAGS_AS_LABELS)` integration.

## Implementation Guidance

### Build and Package Foundation

- Use CMake minimum `3.24` and `project(libbsa VERSION 0.1.0 LANGUAGES CXX)`.
- Create a real `libbsa` library target with `target_compile_features(libbsa PUBLIC cxx_std_20)`.
- Keep public include directories limited to `include/` through generator expressions:
  - `$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>`
  - `$<INSTALL_INTERFACE:include>`
- Add install/export support in Phase 1 per D-09 and D-24:
  - `install(TARGETS libbsa EXPORT libbsaTargets FILE_SET HEADERS)`
  - `install(EXPORT libbsaTargets NAMESPACE libbsa:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libbsa)`
  - `configure_package_config_file(...)`
  - `write_basic_package_version_file(...)`
- Provide explicit static and shared presets per D-20/D-21:
  - `windows-msvc-debug-static`
  - `windows-msvc-debug-shared`
- Enable high warnings, but do not make warnings errors per D-22.

### Public API Boundary

- Public headers should be conventional headers only; C++20 modules are deferred.
- Public namespace is exactly `libbsa`; private helpers use `libbsa::detail` only when needed per D-08.
- Provide:
  - `include/libbsa/result.hpp`
  - `include/libbsa/archive.hpp`
  - `include/libbsa/libbsa.hpp`
  - `include/libbsa/version.hpp`
- Public headers must not include `libdeflate.h`, `lz4.h`, DirectXTex headers, Windows SDK headers, or any `TES5Edit/` file.
- Do not expose `std::expected`; the public API is C++20-compatible.

### Result/Error Shape

Use a local `libbsa::result<T>` and `libbsa::result<void>` with:

- `bool has_value() const noexcept`
- `explicit operator bool() const noexcept`
- `T& value() &` and const/ref-qualified variants where practical
- `const libbsa::error& error() const noexcept`

Use:

- `enum class libbsa::error_code { unsupported, invalid_argument, io_error, format_error }` at minimum.
- `struct libbsa::error { error_code code; std::string message; }`.

Per D-12, tests should assert stable error codes, not exact diagnostic message text. Per D-15, `value()` may throw for programmer misuse when the result contains an error.

### Minimal Facade Stub

Provide `libbsa::archive_reader` with an explicit factory/open call, not a fallible constructor:

```cpp
static libbsa::result<archive_reader> open(std::string_view host_path);
```

The Phase 1 implementation must return a structured `error_code::unsupported` result for non-empty paths because real archive detection/parsing is out of scope. Empty path input may return `error_code::invalid_argument`. Do not invent archive metadata placeholders.

### Test Harness

- Integrate Catch2 through vcpkg and CMake.
- Use `include(CTest)`, `include(Catch)`, and `catch_discover_tests(libbsa_tests ADD_TAGS_AS_LABELS DISCOVERY_MODE PRE_TEST)`.
- Required labels/tags to establish in docs and initial tests: `unit`, `fixture`, `roundtrip`, `compat`, `malformed`, `slow`, and `requires-game-fixture`.
- Initial automated tests should cover:
  - result success/failure behavior
  - stable error codes
  - `archive_reader::open(...)` returns `unsupported` for non-empty paths
  - public dependency-boundary include smoke test
  - local game fixture test is discovered and skipped by default with setup hints

### Fixture Policy

- Committed legal fixtures live under:
  - `tests/fixtures/generated/source`
  - `tests/fixtures/generated/archives`
- Local-only game fixtures live under ignored `tests/fixtures/local` or via `LIBBSA_GAME_FIXTURES`.
- Fixture docs must prohibit using `TES5Edit/` as fixture workspace or source of committed fixture files.
- Each committed generated fixture must document generator/source recipe, legal provenance, and behavior it proves.

### CI Validation

- CI must validate Windows/MSVC static and shared configure/build/test flows per D-23.
- CI should not mutate, format, compile, or stage `TES5Edit/`.
- CI commands should use CMake presets:
  - `cmake --preset windows-msvc-debug-static`
  - `cmake --build --preset windows-msvc-debug-static`
  - `ctest --preset windows-msvc-debug-static --output-on-failure`
  - same for `windows-msvc-debug-shared`
- Add an installed-package consumer smoke test that runs from CTest or CI to prove `find_package(libbsa CONFIG REQUIRED)` and `target_link_libraries(... libbsa::libbsa)` work against installed artifacts.

## Dependency Decision

Boost.Outcome was explicitly allowed for investigation by D-17, but the locked public API boundary requires libbsa-owned result/error types. For Phase 1 planning, do **not** add Boost.Outcome. A local result implementation is smaller, avoids dependency-policy exception paperwork, and satisfies D-11 through D-18. Revisit only if implementation reveals concrete correctness issues that cannot be handled locally.

## Validation Architecture

Phase 1 validation should be fast and fully automated.

- **Framework:** Catch2 v3 via vcpkg + CTest.
- **Quick command:** `ctest --preset windows-msvc-debug-static -L unit --output-on-failure`.
- **Full command:** `ctest --preset windows-msvc-debug-static --output-on-failure` and `ctest --preset windows-msvc-debug-shared --output-on-failure`.
- **Boundary checks:** add grep-based checks to prove public headers do not include `std::expected`, `libdeflate`, `lz4`, `DirectXTex`, Windows SDK, or `TES5Edit` references.
- **CI checks:** static and shared preset configure/build/test jobs.

## Architectural Responsibility Map

| Responsibility | Tier | Phase 1 Files |
|----------------|------|---------------|
| CMake/vcpkg project and package export | Build system | `CMakeLists.txt`, `cmake/libbsaConfig.cmake.in`, `CMakePresets.json`, `vcpkg-configuration.json` |
| Public API/result/facade shell | Public API | `include/libbsa/result.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/libbsa.hpp`, `include/libbsa/version.hpp` |
| Stub implementation | Thin implementation | `src/archive.cpp`, `src/libbsa.cpp` |
| Test harness | Tests | `tests/CMakeLists.txt`, `tests/unit/*.cpp`, `tests/package-consumer/*` |
| Fixture policy | Tests/docs | `tests/fixtures/README.md`, `tests/fixtures/generated/**`, `.gitignore` |
| CI | Automation | `.github/workflows/ci.yml` |

## Common Pitfalls to Avoid in Plans

- Do not add writer APIs in Phase 1; D-02 reserves writer shells for writer phases.
- Do not design stream abstractions in Phase 1; D-03 says the open stub accepts host path text via a string-view-style API.
- Do not implement real archive detection, binary I/O helpers, path normalization, hashing, compression adapters, or DDS behavior.
- Do not add C++20 modules.
- Do not use or mutate `TES5Edit/` as fixtures, source, or build input.
- Do not make warnings errors in Phase 1.
- Do not compare diagnostic message strings exactly in tests; assert error codes.

## Research Complete

Phase 1 is ready to plan with no blocking unknowns.
