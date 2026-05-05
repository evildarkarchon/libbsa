# Phase 1: Build, Error, and Test Foundation - Research

**Researched:** 2026-05-05  
**Domain:** C++20 library build scaffold, vcpkg dependency plumbing, structured result/error API, and Catch2/CTest foundation  
**Confidence:** HIGH for build/test/dependency guidance; MEDIUM for local `result<T>` implementation details until compiled against the chosen compiler matrix [VERIFIED: `.planning/research/STACK.md`, Context7 `/kitware/cmake`, Microsoft Learn vcpkg docs, Catch2 docs]

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

## Implementation Decisions

### Build Shape
- **D-01:** Provide static/shared build options in Phase 1 rather than a single fixed library target. The planner should expose CMake options that let consumers choose static or shared output while keeping the foundation minimal.
- **D-02:** Add basic install support for the library and public headers, but do not require full generated CMake package config/version files unless the planner finds it trivial and low-risk.
- **D-03:** Add a primary Windows vcpkg CMake preset because Windows is the primary target. Linux/macOS presets can wait until portability validation becomes active.
- **D-04:** Root CMake should find all Phase 1 declared dependencies: `libdeflate`, `lz4`, `DirectXTex`, and `Catch2`. Runtime dependency implementation can remain private/minimal, but dependency resolution must be proven now.

### Result API
- **D-05:** Implement a local `libbsa::result<T>` class as the C++20-compatible result surface. Do not expose C++23 `std::expected` in public headers.
- **D-06:** Support `libbsa::result<void>` for operations that can fail but do not return a value.
- **D-07:** Phase 1 errors should carry an error category/code plus a short message. Rich parser context such as offsets, archive paths, nested causes, or source locations can wait until parser phases require it.
- **D-08:** Initial tests should lock result/error construction, `has_value`-style state checks, value access, error access, and representative error propagation/access behavior. Monadic helpers such as `and_then`/`transform` are not required in Phase 1.

### Test Layout
- **D-09:** Use one initial Catch2 test executable for foundation tests rather than many domain-specific executables before domains exist.
- **D-10:** Register Catch2 tests with CTest through Catch2's CMake discovery helper when available.
- **D-11:** Use `unit` and `smoke` labels in Phase 1. Reserve `fixture`, `roundtrip`, `compat`, and `slow` labels for later phases when those test types exist.
- **D-12:** Add a compile-only public-header isolation test that includes the foundational public header from a consumer translation unit without private dependency headers.

### Reference Boundary and Header Layout
- **D-13:** Document the `TES5Edit/` read-only boundary in `README.md` and add a CMake comment near explicit source lists so implementers see the rule where build sources are maintained.
- **D-14:** Use explicit CMake source lists. Do not use recursive source globbing that could accidentally pull in files from `TES5Edit/`.
- **D-15:** Do not create an empty compatibility-notes file in Phase 1. Add compatibility notes when a later phase actually traces non-obvious reference behavior.
- **D-16:** Keep public headers under `include/libbsa/` and private implementation headers under `src/`. Avoid `include/libbsa/detail` for Phase 1 internals unless a later public inline/template need forces it.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may still choose exact file names and CMake option names as long as the decisions above and SPEC acceptance criteria are satisfied.

### Deferred Ideas (OUT OF SCOPE)

## Deferred Ideas

None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FND-01 | Consumer can build libbsa as a reusable C++20 library with CMake and vcpkg-managed dependencies. [CITED: `.planning/REQUIREMENTS.md`] | Use CMake 3.24+, `target_compile_features(libbsa PUBLIC cxx_std_20)`, vcpkg manifest mode, and a Windows vcpkg preset. [VERIFIED: `.planning/research/STACK.md`; CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration] |
| FND-02 | Consumer can run a focused test suite through CTest for parser, codec, fixture, and round-trip behavior. [CITED: `.planning/REQUIREMENTS.md`] | Phase 1 should create the CTest/Catch2 foundation only; future parser/codec/fixture/round-trip targets can attach labels later. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| FND-03 | Consumer can receive structured errors for invalid magic, unsupported versions, truncated records, impossible offsets, decompression failures, and malformed archives. [CITED: `.planning/REQUIREMENTS.md`] | Define foundational error categories/codes now and test representative unsupported-format, malformed-archive, I/O, and decompression failures. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| FND-04 | Consumer can use archive operations without global mutable state or application-specific UI/tooling dependencies. [CITED: `.planning/REQUIREMENTS.md`] | Keep public headers libbsa-owned and dependency/UI-free; Phase 1 should not introduce global registries or singleton configuration. [CITED: `AGENTS.md`; `.planning/PROJECT.md`] |
| FND-05 | Maintainer can trace non-obvious compatibility behavior to BSArchPro/TES5Edit reference code without modifying the `TES5Edit/` submodule. [CITED: `.planning/REQUIREMENTS.md`] | Document the boundary in README and CMake source-list comments; do not create compatibility notes until reference behavior is actually traced. [CITED: `AGENTS.md`; `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`] |
| VAL-01 | Maintainer can run unit tests for hash algorithms, compression round trips, header parsing, and record serialization. [CITED: `.planning/REQUIREMENTS.md`] | Phase 1 supplies the test executable, CTest registration, and labels so later phases can add those unit suites. [CITED: `.planning/ROADMAP.md`] |
</phase_requirements>

## Summary

Phase 1 should establish a conventional, target-based CMake C++20 library skeleton with vcpkg manifest dependency resolution, one Catch2 foundation test executable, CTest registration, and minimal install rules for the library plus public headers. [VERIFIED: Context7 `/kitware/cmake`; CITED: https://cmake.org/cmake/help/latest/module/CTest.html; CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration] The repository currently has no root `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json`, `include/`, `src/`, or `tests/` scaffold, so planning should begin with a Wave 0 build/test skeleton rather than refactoring existing implementation. [VERIFIED: glob search 2026-05-05]

The result/error API should be local and C++20-compatible because `std::expected` is a C++23 facility, including its `void` specialization and observers such as `has_value()`, `value()`, and `error()`. [CITED: https://en.cppreference.com/w/cpp/utility/expected] The smallest useful Phase 1 surface is `libbsa::error_category`/`error_code` or equivalent enum, `libbsa::error` with category/code/message, `libbsa::result<T>`, and `libbsa::result<void>`. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`] Do not add archive readers, binary I/O, compression adapters, DDS analysis, path/hash services, or real archive fixtures in this phase. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]

**Primary recommendation:** Plan one build-system task, one public error/result API task, one Catch2/CTest task, and one documentation/boundary task, with verification gates for configure/build/test, public-header isolation, vcpkg dependency discovery, and zero `TES5Edit/` inclusion. [VERIFIED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`; CITED: `AGENTS.md`]

## Project Constraints (from AGENTS.md)

- `TES5Edit/` is read-only reference material and must not be edited, formatted, staged, compiled, linked, or used as vendored source. [CITED: `AGENTS.md`]
- Implementation belongs outside `TES5Edit/` and must expose clean, portable C++ interfaces rather than Delphi/Pascal structure. [CITED: `AGENTS.md`]
- Use C++ as the implementation language; the phase spec narrows this to C++20. [CITED: `AGENTS.md`; `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]
- Required dependencies are `libdeflate`, official `lz4`, `DirectXTex`, and vcpkg; do not introduce other external dependencies speculatively. [CITED: `AGENTS.md`]
- Public APIs and substantially rewritten methods need Doxygen-compliant C++ doc comments; comments explaining non-obvious compatibility, ownership, error-handling, and boundary decisions should be added. [CITED: `AGENTS.md`]
- Never delete accurate comments as cleanup; mention any comment removal or rewrite in the final execution reply. [CITED: `AGENTS.md`]
- Add focused tests as surfaces are implemented; do not use `TES5Edit/` as a mutable test fixture. [CITED: `AGENTS.md`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| CMake/vcpkg build configuration | Build system | Repository docs | Build configuration owns target creation, dependency discovery, presets, and install rules. [VERIFIED: Context7 `/kitware/cmake`; CITED: Microsoft Learn vcpkg CMake integration] |
| Public result/error API | Library public API | Private implementation | Consumers need libbsa-owned error/result types without private dependency leakage. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| Result/error tests | Test layer | Build system | Catch2 test code owns behavior assertions; CMake/CTest owns registration and labels. [CITED: Catch2 CMake integration docs; CMake CTest docs] |
| Public-header isolation | Test layer | Public API | A consumer-style compile target should prove public headers do not include libdeflate/LZ4/DirectXTex/platform/TES5Edit types. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| TES5Edit boundary visibility | Repository docs | Build system | README and explicit source-list comments make the read-only reference rule visible where maintainers work. [CITED: `AGENTS.md`; `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`] |

## Standard Stack

### Core

| Library / Tool | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| C++ | C++20 | Public API and implementation language | Matches locked project constraints while avoiding C++23-only public API exposure. [CITED: `.planning/research/STACK.md`; https://en.cppreference.com/w/cpp/utility/expected] |
| CMake | Minimum 3.24; local environment has 4.3.2 | Build, target definition, install rules, CTest integration | CMake supports target-based C++ feature requirements, file sets, install rules, `BUILD_SHARED_LIBS`, and CTest. [VERIFIED: local `cmake --version`; VERIFIED: Context7 `/kitware/cmake`; CITED: CMake docs] |
| vcpkg | Local CLI 2026-04-08; use manifest mode with baseline | Dependency acquisition | vcpkg manifest mode detects `vcpkg.json` through the CMake toolchain and can automatically acquire dependencies during configure. [VERIFIED: local `vcpkg --version`; CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration] |
| Catch2 | vcpkg `3.14.0#0`, last updated 2026-04-06 | Unit/smoke tests | Catch2 provides CMake integration and `catch_discover_tests` via `Catch.cmake`. [VERIFIED: vcpkg package page; CITED: Catch2 CMake docs] |
| CTest | Bundled with CMake; local 4.3.2 | Test orchestration | Including CTest creates the `BUILD_TESTING` option and enables CTest-based test execution. [VERIFIED: local `ctest --version`; CITED: https://cmake.org/cmake/help/latest/module/CTest.html] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| libdeflate | vcpkg `1.25#0`, last updated 2025-11-03 | Required future deflate compression/decompression dependency | Find and link privately in Phase 1 to prove dependency resolution; implement codecs in Phase 3. [VERIFIED: vcpkg package page; CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`] |
| lz4 | vcpkg `1.10.0#0`, last updated 2024-07-25 | Required future LZ4 frame/raw block dependency | Find and link privately in Phase 1 to prove dependency resolution; implement codecs in Phase 3. [VERIFIED: vcpkg package page] |
| DirectXTex | vcpkg `2026-03-31#0`, last updated 2026-04-01 | Required future DDS metadata/texture-analysis dependency | Find and link privately in Phase 1 to prove dependency resolution; implement wrappers in BA2 DDS phases. [VERIFIED: vcpkg package page; CITED: `.planning/research/STACK.md`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| CMake + vcpkg | Meson, Bazel, raw Visual Studio solutions | Contradicts locked vcpkg/CMake direction and adds planning risk for no Phase 1 value. [CITED: `.planning/research/STACK.md`] |
| Catch2 | GoogleTest | Catch2 is already selected in stack research and discussion; switching would violate the locked dependency list. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| Local `result<T>` | `std::expected` | `std::expected` is C++23, so exposing it would violate C++20 public API constraints. [CITED: https://en.cppreference.com/w/cpp/utility/expected] |
| Explicit source lists | Recursive `file(GLOB_RECURSE ...)` | Recursive globbing risks accidental `TES5Edit/` inclusion and contradicts D-14. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`] |

**Installation:**
```powershell
# Use vcpkg manifest mode through CMake; do not vendor dependencies.
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Debug
ctest --test-dir build --output-on-failure
```
[CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration]

**Version verification:** Package versions above were verified from current vcpkg package pages on 2026-05-05, and local tool versions were probed from PATH. [VERIFIED: vcpkg package pages; VERIFIED: local environment audit]

## Architecture Patterns

### System Architecture Diagram

```text
Developer / Consumer
        |
        v
 CMake configure --preset windows-vcpkg
        |
        +--> vcpkg toolchain detects vcpkg.json and installs deps
        |       |
        |       +--> libdeflate / lz4 / DirectXTex / Catch2 resolved privately
        |
        v
 libbsa target (C++20, explicit sources, public include/libbsa headers)
        |
        +--> install public headers + library artifact
        |
        +--> foundation_tests target links libbsa + Catch2
                  |
                  v
             CTest / catch_discover_tests
                  |
                  +--> unit: result/error behavior
                  +--> smoke: linkability + public-header isolation
```
[VERIFIED: Context7 `/kitware/cmake`; CITED: Catch2 CMake integration docs; CITED: CMake CTest docs]

### Recommended Project Structure

```text
CMakeLists.txt               # Root target graph, explicit source lists, CTest, install rules
CMakePresets.json            # Primary Windows vcpkg configure preset
vcpkg.json                   # Manifest dependencies and builtin baseline
README.md                    # Foundation usage and TES5Edit read-only boundary
include/libbsa/              # Public C++20 headers only
  result.hpp                 # result<T>, result<void>, error/category/code surface
src/                         # Private implementation files/headers only
  libbsa.cpp                 # Minimal linkable library source
tests/                       # One initial Catch2 foundation executable
  foundation_tests.cpp       # result/error unit tests and link smoke tests
  public_header_smoke.cpp    # Consumer-style include/link target if kept separate
```
[CITED: `.planning/research/STACK.md`; `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]

### Pattern 1: Target-Based CMake with Explicit Sources

**What:** Define `libbsa` with explicit source/header lists, C++20 compile features, and private dependency links. [VERIFIED: Context7 `/kitware/cmake`; CITED: https://cmake.org/cmake/help/latest/command/target_sources.html]  
**When to use:** Use for all library and test targets in Phase 1. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]  
**Example:**
```cmake
# Source: CMake docs: target_sources, BUILD_SHARED_LIBS, install(TARGETS)
option(BUILD_SHARED_LIBS "Build libbsa as a shared library" OFF)

add_library(libbsa)
add_library(libbsa::libbsa ALIAS libbsa)

target_compile_features(libbsa PUBLIC cxx_std_20)
target_sources(libbsa
  PRIVATE
    src/libbsa.cpp
  PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS include
    FILES
      include/libbsa/result.hpp)
```

### Pattern 2: vcpkg Manifest + Preset Before `project()`

**What:** Put the vcpkg toolchain in `CMakePresets.json` or the configure command so CMake sees it before `project()`. [CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration]  
**When to use:** Use for the primary Windows vcpkg preset. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]  
**Example:**
```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "windows-msvc-vcpkg",
      "generator": "Visual Studio 17 2022",
      "binaryDir": "${sourceDir}/build/windows-msvc-vcpkg",
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "BUILD_TESTING": "ON"
      }
    }
  ]
}
```

### Pattern 3: Catch2 Discovery via CTest

**What:** Include CTest, link one foundation test executable with Catch2, and call `catch_discover_tests`. [CITED: Catch2 CMake integration docs; CMake CTest docs]  
**When to use:** Use for D-09 and D-10. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]  
**Example:**
```cmake
# Source: Catch2 CMake integration docs
include(CTest)
if(BUILD_TESTING)
  find_package(Catch2 CONFIG REQUIRED)
  add_executable(libbsa_foundation_tests tests/foundation_tests.cpp)
  target_link_libraries(libbsa_foundation_tests PRIVATE libbsa::libbsa Catch2::Catch2WithMain)
  include(Catch)
  catch_discover_tests(libbsa_foundation_tests PROPERTIES LABELS "unit;smoke")
endif()
```

### Pattern 4: Minimal Expected-Like Result API

**What:** Implement a local API shaped around `has_value()`, `value()`, `error()`, and `result<void>` without monadic helpers. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`; https://en.cppreference.com/w/cpp/utility/expected]  
**When to use:** Use for all Phase 1 fallible API tests. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]  
**Example:**
```cpp
// Source: Phase 1 CONTEXT.md D-05 through D-08; cppreference std::expected observer shape.
namespace libbsa {
enum class error_code {
    unsupported_format,
    malformed_archive,
    io_failure,
    decompression_failure,
};

struct error {
    error_code code;
    std::string message;
};

template <class T>
class result;

template <>
class result<void>;
} // namespace libbsa
```

### Anti-Patterns to Avoid

- **Recursive source globbing:** It can accidentally pull in `TES5Edit/` and violates D-14; use explicit source lists. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]
- **Public dependency leakage:** Public headers must not include libdeflate, LZ4, DirectXTex, Windows, Delphi, or TES5Edit headers. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]
- **C++23 public surface:** Do not expose `std::expected`; it is C++23. [CITED: https://en.cppreference.com/w/cpp/utility/expected]
- **Archive behavior creep:** Binary readers, archive detection, parsing, compression adapters, DDS analysis, and fixtures are out of scope. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]
- **Global mutable configuration:** Project constraints reject global singleton configuration. [CITED: `.planning/REQUIREMENTS.md`; `AGENTS.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Build orchestration | Custom scripts or Visual Studio-only projects | CMake targets and presets | CMake/vcpkg is the locked reusable library path. [CITED: `.planning/research/STACK.md`] |
| Dependency acquisition | Vendored copies of libdeflate/lz4/DirectXTex/Catch2 | vcpkg manifest mode | vcpkg manifest mode isolates dependencies per project and supports baselines/versioning. [CITED: https://learn.microsoft.com/vcpkg/concepts/manifest-mode; https://learn.microsoft.com/vcpkg/users/versioning] |
| Test discovery | Custom parser for Catch2 output | `catch_discover_tests` | Catch2 provides CMake scripts for automatic CTest registration. [CITED: Catch2 CMake integration docs] |
| Expected-like standard type | Backporting or exposing `std::expected` | Local `libbsa::result<T>` | `std::expected` is C++23; local type satisfies C++20. [CITED: https://en.cppreference.com/w/cpp/utility/expected] |
| Compression/DDS stubs | Fake codec or DDS wrappers in public API | Private link-only dependency proof | Implementation of codec/texture behavior is explicitly deferred. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |

**Key insight:** Phase 1 is a foundation and contract phase; custom tooling or speculative implementation will create compatibility debt before any archive semantics exist. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`; `.planning/research/SUMMARY.md`]

## Common Pitfalls

### Pitfall 1: vcpkg Toolchain Set Too Late
**What goes wrong:** `find_package()` cannot locate manifest dependencies. [CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration]  
**Why it happens:** vcpkg's toolchain is evaluated during the first `project()` call, so related variables must be set before `project()`. [CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration]  
**How to avoid:** Put the toolchain path in `CMakePresets.json` or the configure command. [CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration]  
**Warning signs:** Configure succeeds without manifest install or `find_package()` fails for required packages. [CITED: Microsoft Learn vcpkg troubleshooting]

### Pitfall 2: Shared/Static Option Added After Targets
**What goes wrong:** `BUILD_SHARED_LIBS` does not consistently control `add_library()` defaults. [CITED: https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html]  
**Why it happens:** CMake says projects should create the `BUILD_SHARED_LIBS` option before any `add_library()` calls. [CITED: https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html]  
**How to avoid:** Declare `option(BUILD_SHARED_LIBS ...)` at the top of the root CMake file before adding targets. [CITED: https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html]  
**Warning signs:** Reconfigures produce different static/shared behavior. [CITED: https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html]

### Pitfall 3: Header Isolation Test Links But Does Not Isolate
**What goes wrong:** The library builds, but consumers inherit private dependency headers. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]  
**Why it happens:** Public headers include private implementation headers or external library headers. [CITED: `.planning/research/STACK.md`]  
**How to avoid:** Create a consumer-style translation unit that includes only `include/libbsa/result.hpp` and fails if dependency/platform/TES5Edit types leak. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]  
**Warning signs:** Public headers include `<libdeflate.h>`, `<lz4frame.h>`, DirectXTex headers, Windows SDK headers, or Pascal/TES5Edit names. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]

### Pitfall 4: Overbuilding the Error API
**What goes wrong:** Parser-only context, source locations, nested causes, or monadic helpers delay foundation work. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]  
**Why it happens:** `std::expected`-like designs can grow quickly. [CITED: https://en.cppreference.com/w/cpp/utility/expected]  
**How to avoid:** Implement category/code/message, `result<T>`, `result<void>`, state checks, value/error access, and representative propagation only. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]  
**Warning signs:** Plans mention offsets, archive paths, nested causes, or `and_then`/`transform` in Phase 1. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]

## Code Examples

Verified patterns from official or project sources:

### vcpkg Manifest Shape
```json
{
  "name": "libbsa",
  "version-semver": "0.1.0",
  "dependencies": [
    { "name": "libdeflate", "features": ["compression", "decompression"] },
    "lz4",
    "directxtex",
    "catch2"
  ],
  "builtin-baseline": "<commit from vcpkg x-update-baseline --add-initial-baseline>"
}
```
[CITED: https://learn.microsoft.com/vcpkg/users/versioning; VERIFIED: `.planning/research/STACK.md`]

### Minimal Result Test Shape
```cpp
// Source: Phase 1 SPEC acceptance criteria and CONTEXT D-08.
TEST_CASE("result stores successful values", "[unit]") {
    libbsa::result<int> value = libbsa::success(42);
    REQUIRE(value.has_value());
    CHECK(value.value() == 42);
}

TEST_CASE("result stores categorized errors", "[unit]") {
    auto failure = libbsa::failure<int>({
        libbsa::error_code::unsupported_format,
        "unsupported archive format"
    });
    REQUIRE_FALSE(failure.has_value());
    CHECK(failure.error().code == libbsa::error_code::unsupported_format);
}
```
[CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`; `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]

### Public Header Isolation Smoke Target
```cmake
# Source: Phase 1 CONTEXT.md D-12.
add_executable(libbsa_public_header_smoke tests/public_header_smoke.cpp)
target_link_libraries(libbsa_public_header_smoke PRIVATE libbsa::libbsa)
add_test(NAME libbsa.public_header_smoke COMMAND libbsa_public_header_smoke)
set_tests_properties(libbsa.public_header_smoke PROPERTIES LABELS smoke)
```
[CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`; CMake CTest docs]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Classic/global vcpkg installs | Manifest mode with per-project install tree and baseline | Current Microsoft Learn guidance recommends manifest mode for most users. [CITED: https://learn.microsoft.com/vcpkg/concepts/manifest-mode] | Use committed `vcpkg.json` and baseline rather than relying on developer-global packages. |
| Parse Catch2 tests at configure time | `catch_discover_tests` runs the executable and registers tests with CTest | Catch2 docs mark `ParseAndAddCatchTests` deprecated in favor of `catch_discover_tests`. [CITED: Catch2 CMake integration docs] | Use `include(Catch)` and `catch_discover_tests`. |
| Public `std::expected` for result APIs | Local `result<T>` in C++20 projects | `std::expected` is standardized in C++23. [CITED: https://en.cppreference.com/w/cpp/utility/expected] | Do not expose `std::expected` until a deliberate C++23 migration. |
| Recursive source discovery | Explicit target source lists and CMake file sets | Phase discussion locked explicit source lists. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`] | Reduces risk of building `TES5Edit/` by accident. |

**Deprecated/outdated:**
- `ParseAndAddCatchTests.cmake`: deprecated and superseded by `catch_discover_tests`. [CITED: Catch2 CMake integration docs]
- `VCPKG_PREFER_SYSTEM_LIBS`: deprecated in vcpkg CMake integration docs; do not use it in presets. [CITED: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration]

## Assumptions Log

> List all claims tagged `[ASSUMED]` in this research. The planner and discuss-phase use this section to identify decisions that need user confirmation before execution.

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|

**If this table is empty:** All claims in this research were verified or cited — no user confirmation needed.

## Open Questions

1. **Should Phase 1 generate full CMake package config/version files?**
   - What we know: D-02 requires basic install support but does not require full generated package config/version files unless trivial and low-risk. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]
   - What's unclear: Whether the executor should spend time on exported config files in Phase 1. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`]
   - Recommendation: Plan basic `install(TARGETS ... FILE_SET ...)` first and make package config generation optional stretch only if it does not endanger core acceptance. [VERIFIED: Context7 `/kitware/cmake`]

2. **Which Windows generator should the preset use on this machine?**
   - What we know: `cmake`, `ctest`, `vcpkg`, and `git` are on PATH; `ninja` and `cl` are not on PATH in the current shell. [VERIFIED: local environment audit]
   - What's unclear: Visual Studio may still be installed and usable through the Visual Studio CMake generator even though `cl` is not in this non-developer shell PATH. [VERIFIED: local environment audit]
   - Recommendation: Use `Visual Studio 17 2022` in the committed Windows preset and add a documented fallback command; executor should verify configure locally. [CITED: Microsoft Learn vcpkg CMake integration]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build/install/test integration | ✓ | 4.3.2 | Minimum project should require 3.24+. [VERIFIED: local environment audit; `.planning/research/STACK.md`] |
| CTest | Test execution | ✓ | 4.3.2 | Bundled with CMake. [VERIFIED: local environment audit] |
| vcpkg | Manifest dependency acquisition | ✓ | 2026-04-08 CLI | Use `%VCPKG_ROOT%` / `$env:VCPKG_ROOT` in preset. [VERIFIED: local environment audit] |
| VCPKG_ROOT | CMake preset toolchain path | ✓ | `C:\vcpkg` | Use CMakeUserPresets or command-line `CMAKE_TOOLCHAIN_FILE` if absent. [VERIFIED: local environment audit; CITED: Microsoft Learn vcpkg CMake integration] |
| git | vcpkg baseline/source control | ✓ | 2.54.0.windows.1 | Required for baseline updates. [VERIFIED: local environment audit] |
| Ninja | Optional generator | ✗ | — | Use Visual Studio generator. [VERIFIED: local environment audit] |
| MSVC `cl` in current PATH | Compiler for command-line Ninja/NMake builds | ✗ | — | Use Visual Studio generator or run from Developer PowerShell. [VERIFIED: local environment audit] |

**Missing dependencies with no fallback:**
- None confirmed. [VERIFIED: local environment audit]

**Missing dependencies with fallback:**
- Ninja is missing; use the Visual Studio generator. [VERIFIED: local environment audit]
- `cl` is missing from this shell PATH; use the Visual Studio generator or a Developer PowerShell. [VERIFIED: local environment audit]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 `3.14.0#0` through vcpkg + CTest 4.3.2 locally. [VERIFIED: vcpkg package page; local environment audit] |
| Config file | None yet — Wave 0 creates root `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json`, and `tests/`. [VERIFIED: glob search] |
| Quick run command | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit` [CITED: CMake CTest docs] |
| Full suite command | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure` [CITED: CMake CTest docs] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FND-01 | Configure/build C++20 `libbsa` target with vcpkg dependencies | smoke | `cmake --preset windows-msvc-vcpkg && cmake --build --preset windows-msvc-vcpkg` | ❌ Wave 0 [VERIFIED: glob search] |
| FND-02 | CTest can run the foundation suite and accept future labels | smoke | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure` | ❌ Wave 0 [VERIFIED: glob search] |
| FND-03 | Result/error API covers success, error, propagation, and representative categories | unit | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit` | ❌ Wave 0 [VERIFIED: glob search] |
| FND-04 | Public API has no global mutable state or UI/tooling dependency exposure | compile/smoke | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L smoke` | ❌ Wave 0 [VERIFIED: glob search] |
| FND-05 | TES5Edit stays read-only and outside build source lists | smoke/manual review | `git status --short TES5Edit && ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L smoke` | ❌ Wave 0 [VERIFIED: glob search] |
| VAL-01 | Unit-test framework foundation exists for later hash/compression/header/record tests | smoke | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure` | ❌ Wave 0 [VERIFIED: glob search] |

### Sampling Rate
- **Per task commit:** `cmake --build --preset windows-msvc-vcpkg && ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit` [CITED: CMake docs]
- **Per wave merge:** Full configure/build/test from a clean build directory. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]
- **Phase gate:** `ctest --test-dir <build-dir> --output-on-failure` green and public-header isolation target passing. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`]

### Wave 0 Gaps
- [ ] `CMakeLists.txt` — root project, library target, explicit sources, dependency discovery, CTest, install basics. [VERIFIED: glob search]
- [ ] `vcpkg.json` — declares `libdeflate`, `lz4`, `directxtex`, and `catch2` with baseline. [VERIFIED: glob search]
- [ ] `CMakePresets.json` — primary Windows vcpkg preset. [VERIFIED: glob search]
- [ ] `include/libbsa/result.hpp` — public foundation API. [VERIFIED: glob search]
- [ ] `src/libbsa.cpp` — minimal linkable library source. [VERIFIED: glob search]
- [ ] `tests/foundation_tests.cpp` — result/error and linkability tests. [VERIFIED: glob search]
- [ ] `tests/public_header_smoke.cpp` — consumer include isolation. [VERIFIED: glob search]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No authentication surface in this phase. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| V3 Session Management | no | No sessions or network state in this phase. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| V4 Access Control | no | No user/resource authorization surface in this phase. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| V5 Input Validation | yes | Validate public API state access and reject malformed/unsupported categories through structured errors; deeper binary input validation starts in parser phases. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| V6 Cryptography | no | No cryptography in this phase. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |

### Known Threat Patterns for C++ Build/Test Foundation

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Accidental compilation of reference submodule code | Tampering | Explicit source lists; no recursive globs; README and CMake comments documenting read-only boundary. [CITED: `AGENTS.md`; `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md`] |
| Public header leaks platform/dependency implementation types | Information Disclosure / Maintainability Risk | Consumer compile-only smoke test and public/private include separation. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| Unstructured failure paths in future parsers | Tampering / Denial of Service | Foundational categorized errors and tests for representative malformed/archive failure classes. [CITED: `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`] |
| Dependency supply drift | Tampering | vcpkg manifest mode with committed baseline. [CITED: https://learn.microsoft.com/vcpkg/users/versioning] |

## Sources

### Primary (HIGH confidence)
- Context7 `/kitware/cmake` - target-based library, install, `BUILD_SHARED_LIBS`, and public header file-set patterns. [VERIFIED: Context7]
- CMake official docs - CTest module, `BUILD_SHARED_LIBS`, and `target_sources(FILE_SET)` behavior: https://cmake.org/cmake/help/latest/module/CTest.html, https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html, https://cmake.org/cmake/help/latest/command/target_sources.html. [CITED: CMake docs]
- Microsoft Learn vcpkg docs - CMake toolchain integration, manifest mode, baselines, `version>=`, and overrides: https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration, https://learn.microsoft.com/vcpkg/concepts/manifest-mode, https://learn.microsoft.com/vcpkg/users/versioning. [CITED: Microsoft Learn]
- Catch2 CMake integration docs - `catch_discover_tests`, `Catch.cmake`, and deprecated `ParseAndAddCatchTests`: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md. [CITED: Catch2 docs]
- vcpkg package pages - `libdeflate` `1.25#0`, `lz4` `1.10.0#0`, `directxtex` `2026-03-31#0`, and `catch2` `3.14.0#0`. [VERIFIED: vcpkg package pages]
- Phase/project artifacts - `01-CONTEXT.md`, `01-SPEC.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/research/STACK.md`, `.planning/research/SUMMARY.md`, and `AGENTS.md`. [VERIFIED: local file reads]

### Secondary (MEDIUM confidence)
- cppreference `std::expected` page - C++23 status and observer vocabulary used as design reference for a local C++20-compatible type: https://en.cppreference.com/w/cpp/utility/expected. [CITED: cppreference]

### Tertiary (LOW confidence)
- None. [VERIFIED: sources reviewed]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - locked by project context and verified against vcpkg/Microsoft/CMake/Catch2 sources. [VERIFIED: local docs and official docs]
- Architecture: HIGH - Phase 1 has a narrow build/API/test/doc scope with no existing implementation to preserve. [VERIFIED: glob search; CITED: `01-SPEC.md`]
- Pitfalls: HIGH - primary pitfalls are documented by official CMake/vcpkg/Catch2 behavior and locked project constraints. [CITED: official docs; `01-CONTEXT.md`]
- Environment: MEDIUM - CMake/vcpkg/CTest are available, but compiler availability needs executor verification because `cl` is not on the current shell PATH. [VERIFIED: local environment audit]

**Research date:** 2026-05-05  
**Valid until:** 2026-06-04 for build/test stack; re-check vcpkg package versions before implementation if delayed more than 30 days. [VERIFIED: vcpkg package pages]
