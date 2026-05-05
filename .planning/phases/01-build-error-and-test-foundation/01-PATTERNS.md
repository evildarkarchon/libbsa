# Phase 1: Build, Error, and Test Foundation - Pattern Map

**Mapped:** 2026-05-05
**Files analyzed:** 8
**Analogs found:** 0 / 8

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `CMakeLists.txt` | config | batch | Research guidance: `01-RESEARCH.md` Pattern 1/3 | no-codebase-analog |
| `CMakePresets.json` | config | batch | Research guidance: `01-RESEARCH.md` Pattern 2 | no-codebase-analog |
| `vcpkg.json` | config | batch | Research guidance: `01-RESEARCH.md` vcpkg manifest shape | no-codebase-analog |
| `README.md` | config/docs | request-response | Boundary guidance: `AGENTS.md` and `01-CONTEXT.md` | no-codebase-analog |
| `include/libbsa/result.hpp` | model/utility | transform | Research guidance: `01-RESEARCH.md` Pattern 4 | no-codebase-analog |
| `src/libbsa.cpp` | service/utility | request-response | Research structure guidance: `01-RESEARCH.md` | no-codebase-analog |
| `tests/foundation_tests.cpp` | test | request-response | Research guidance: `01-RESEARCH.md` minimal result tests | no-codebase-analog |
| `tests/public_header_smoke.cpp` | test | request-response | Research guidance: `01-RESEARCH.md` public header smoke target | no-codebase-analog |

## Pattern Assignments

### `CMakeLists.txt` (config, batch)

**Analog:** No existing implementation analog. Repository has no root `CMakeLists.txt`, `include/`, `src/`, or `tests/` scaffold (`01-CONTEXT.md` lines 99-105; `01-RESEARCH.md` lines 61-67).

**Use research/source guidance instead.**

**Build target pattern** (`01-RESEARCH.md` lines 180-197):
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

**CTest/Catch2 pattern** (`01-RESEARCH.md` lines 226-235):
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

**Boundary pattern:** Add an implementation-facing CMake comment near explicit source lists that `TES5Edit/` is read-only reference material and must not be compiled into libbsa (`01-CONTEXT.md` lines 61-65; `AGENTS.md` lines 16-29).

---

### `CMakePresets.json` (config, batch)

**Analog:** No existing implementation analog.

**Preset pattern** (`01-RESEARCH.md` lines 204-218):
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

**Pitfall to copy:** Set vcpkg toolchain through the preset/configure command before `project()` so `find_package()` sees manifest dependencies (`01-RESEARCH.md` lines 288-292).

---

### `vcpkg.json` (config, batch)

**Analog:** No existing implementation analog.

**Manifest pattern** (`01-RESEARCH.md` lines 317-328):
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

**Dependency boundary:** Find/link `libdeflate`, `lz4`, and `DirectXTex` privately in Phase 1 only to prove dependency resolution; implement actual codecs/texture adapters later (`01-RESEARCH.md` lines 103-108, 274-283).

---

### `README.md` (config/docs, request-response)

**Analog:** Existing `README.md` is empty; use project instructions as the source.

**TES5Edit boundary pattern** (`AGENTS.md` lines 16-29):
```markdown
`TES5Edit/` is a read-only reference submodule. Do not modify it for any reason.

This includes:

- Do not edit files under `TES5Edit/`.
- Do not format files under `TES5Edit/`.
- Do not apply generated changes under `TES5Edit/`.
- Do not update the submodule pointer.
- Do not stage or commit changes inside `TES5Edit/`.
- Do not treat the submodule as vendored source to be compiled into this project.

All implementation work belongs outside `TES5Edit/`.
```

**Build usage pattern:** Include the configure/build/test commands from `01-RESEARCH.md` lines 119-124, adjusted to the committed preset names.

---

### `include/libbsa/result.hpp` (model/utility, transform)

**Analog:** No existing implementation analog.

**Public API shape** (`01-RESEARCH.md` lines 243-263):
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

**Validation/access pattern:** Implement `has_value()`, `value()`, `error()`, success construction, failure construction, and `result<void>`; do not add `and_then`/`transform` in Phase 1 (`01-CONTEXT.md` lines 49-54; `01-RESEARCH.md` lines 306-310).

**Public header isolation:** Do not include libdeflate, LZ4, DirectXTex, Windows SDK, Delphi, or TES5Edit types (`01-SPEC.md` lines 67-73; `01-RESEARCH.md` lines 300-304).

**Documentation pattern:** Public API declarations need Doxygen-compliant comments (`AGENTS.md` lines 51-57).

---

### `src/libbsa.cpp` (service/utility, request-response)

**Analog:** No existing implementation analog.

**Core pattern:** Keep this as the minimal linkable implementation source for the `libbsa` target (`01-RESEARCH.md` lines 158-172). Do not implement archive readers, compression adapters, DDS analysis, path/hash services, or fixtures in this phase (`01-SPEC.md` lines 57-65; `01-RESEARCH.md` lines 266-272).

**Dependency boundary:** It may be used to prove private dependency linkage from CMake, but public API should stay dependency-free (`01-RESEARCH.md` lines 83-87, 103-108).

---

### `tests/foundation_tests.cpp` (test, request-response)

**Analog:** No existing implementation analog.

**Catch2 test pattern** (`01-RESEARCH.md` lines 333-349):
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

**Coverage pattern:** Lock result/error construction, `has_value` checks, value access, error access, `result<void>`, and representative propagation/access behavior (`01-CONTEXT.md` lines 49-54; `01-SPEC.md` lines 32-40).

---

### `tests/public_header_smoke.cpp` (test, request-response)

**Analog:** No existing implementation analog.

**CMake smoke target pattern** (`01-RESEARCH.md` lines 352-359):
```cmake
# Source: Phase 1 CONTEXT.md D-12.
add_executable(libbsa_public_header_smoke tests/public_header_smoke.cpp)
target_link_libraries(libbsa_public_header_smoke PRIVATE libbsa::libbsa)
add_test(NAME libbsa.public_header_smoke COMMAND libbsa_public_header_smoke)
set_tests_properties(libbsa.public_header_smoke PROPERTIES LABELS smoke)
```

**Test intent:** Consumer-style translation unit includes only the foundational public header and verifies no private dependency/platform/TES5Edit types leak (`01-CONTEXT.md` lines 55-60; `01-RESEARCH.md` lines 300-304).

## Shared Patterns

### Explicit build source lists
**Source:** `01-CONTEXT.md` lines 61-65 and `01-RESEARCH.md` lines 266-269  
**Apply to:** `CMakeLists.txt` and all future source additions
```text
Use explicit CMake source lists. Do not use recursive source globbing that could accidentally pull in files from TES5Edit/.
```

### Public/private boundary
**Source:** `01-CONTEXT.md` lines 61-65; `01-SPEC.md` lines 27-35  
**Apply to:** `include/libbsa/result.hpp`, `src/libbsa.cpp`, tests
```text
Public headers live under include/libbsa/. Private implementation files and private headers live under src/. Public headers expose libbsa-owned C++20 types only.
```

### Error/result scope control
**Source:** `01-CONTEXT.md` lines 49-54; `01-RESEARCH.md` lines 306-310  
**Apply to:** `include/libbsa/result.hpp`, `tests/foundation_tests.cpp`
```text
Implement category/code/message, result<T>, result<void>, state checks, value/error access, and representative propagation only. Do not add parser offsets, archive paths, nested causes, source locations, or monadic helpers in Phase 1.
```

### TES5Edit read-only boundary
**Source:** `AGENTS.md` lines 16-29  
**Apply to:** `README.md`, `CMakeLists.txt`, all build/test files
```text
TES5Edit/ is read-only reference material. Do not edit, format, stage, compile, link, or vendor it into libbsa.
```

## No Analog Found

The repository currently has no implementation scaffold outside planning artifacts and the read-only `TES5Edit/` submodule. Planner should use `01-RESEARCH.md`, `01-CONTEXT.md`, `01-SPEC.md`, and `AGENTS.md` as the pattern sources.

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `CMakeLists.txt` | config | batch | No root CMake project exists. |
| `CMakePresets.json` | config | batch | No CMake presets exist. |
| `vcpkg.json` | config | batch | No vcpkg manifest exists. |
| `README.md` | config/docs | request-response | Existing file is empty. |
| `include/libbsa/result.hpp` | model/utility | transform | No public headers exist. |
| `src/libbsa.cpp` | service/utility | request-response | No source directory or C++ library source exists. |
| `tests/foundation_tests.cpp` | test | request-response | No tests directory exists. |
| `tests/public_header_smoke.cpp` | test | request-response | No tests directory exists. |

## Metadata

**Analog search scope:** root implementation files (`CMakeLists.txt`, `vcpkg*.json`, `CMakePresets.json`, `include/`, `src/`, `tests/`), root `README.md`, and planning artifacts. `TES5Edit/` was treated as read-only reference and not used as a copyable implementation analog.  
**Files scanned:** 8 target files plus project/planning guidance files.  
**Pattern extraction date:** 2026-05-05
