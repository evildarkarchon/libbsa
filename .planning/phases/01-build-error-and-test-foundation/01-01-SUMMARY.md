---
phase: 01-build-error-and-test-foundation
plan: 01
subsystem: infra
tags: [cmake, vcpkg, cpp20, libdeflate, lz4, directxtex, catch2]

# Dependency graph
requires: []
provides:
  - CMake-configurable C++20 libbsa library target with explicit sources
  - vcpkg manifest and Windows MSVC vcpkg preset
  - Public result/error declaration header and minimal linkable source
  - Foundation build documentation and TES5Edit read-only boundary
affects: [build-system, public-api, documentation, phase-1-foundation]

# Tech tracking
tech-stack:
  added: [CMake, vcpkg, libdeflate, lz4, DirectXTex, Catch2]
  patterns: [explicit CMake source lists, public header file set, private dependency linkage, TES5Edit boundary comments]

key-files:
  created: [CMakeLists.txt, CMakePresets.json, vcpkg.json, include/libbsa/result.hpp, src/libbsa.cpp]
  modified: [README.md]

key-decisions:
  - "Kept the committed primary preset on Visual Studio 17 2022 as planned even though this machine only exposes Visual Studio 18 2026."
  - "Used imported-target fallback selection in CMake so vcpkg dependency target naming differences remain private to the build system."

patterns-established:
  - "CMake source lists are explicit and keep TES5Edit/ outside compiled sources."
  - "Public headers live under include/libbsa/ and implementation sources live under src/."

requirements-completed: [FND-01, FND-05]

# Metrics
duration: 3min
completed: 2026-05-05
---

# Phase 01 Plan 01: Build Scaffold Summary

**CMake/vcpkg C++20 libbsa scaffold with explicit source lists, private dependency discovery, and visible TES5Edit read-only safeguards.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-05T11:28:12Z
- **Completed:** 2026-05-05T11:31:06Z
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments

- Added vcpkg manifest-mode dependencies for libdeflate, lz4, DirectXTex, and Catch2 with a committed builtin baseline.
- Added a primary `windows-msvc-vcpkg` preset and a C++20 `libbsa` CMake target with installable public headers.
- Added a minimal public result/error declaration header and linkable source outside `TES5Edit/`.
- Documented Windows build commands and the `TES5Edit/` read-only reference boundary in `README.md`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Declare vcpkg manifest and Windows preset** - `f627363` (chore)
2. **Task 2: Create explicit CMake library target and public/private skeleton** - `1dff50a` (feat)
3. **Task 3: Document build usage and TES5Edit source boundary** - `644095d` (docs)

**Plan metadata:** pending final metadata commit

## Files Created/Modified

- `vcpkg.json` - Declares package metadata, dependency manifest entries, features, and builtin baseline.
- `CMakePresets.json` - Defines the `windows-msvc-vcpkg` configure/build/test presets.
- `CMakeLists.txt` - Defines dependency discovery, explicit `libbsa` sources, public header file set, private dependency links, and install rules.
- `include/libbsa/result.hpp` - Declares the foundational libbsa error code, error payload, and result API names.
- `src/libbsa.cpp` - Provides the minimal implementation translation unit for a linkable library target.
- `README.md` - Documents the project purpose, build commands, public/private layout, and TES5Edit boundary.

## Decisions Made

- Preserved the planned Visual Studio 17 2022 preset rather than changing project behavior for this machine's Visual Studio 18 2026 environment.
- Added a small CMake imported-target resolver to keep dependency target-name compatibility private to build configuration.

## Deviations from Plan

None - plan artifacts and committed files match the planned scope. Verification used an additional local fallback configure/build because the planned preset generator was unavailable in this environment.

## Issues Encountered

- `cmake --preset windows-msvc-vcpkg` could not complete because CMake could not find a Visual Studio 17 2022 instance on this machine. Visual Studio 18 2026 is available, so a local fallback configure/build with `-G "Visual Studio 18 2026"` succeeded.

## Verification

| Command | Outcome |
|---------|---------|
| `cmake --list-presets` | Passed; listed `windows-msvc-vcpkg`. |
| `cmake --preset windows-msvc-vcpkg` | Blocked by environment: Visual Studio 17 2022 generator unavailable. |
| `cmake -S . -B build/windows-msvc-vcpkg-vs18 -G "Visual Studio 18 2026" -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DBUILD_TESTING=ON` | Passed; vcpkg resolved required dependencies and CMake generated successfully. |
| `cmake --build build/windows-msvc-vcpkg-vs18 --config Debug` | Passed; built `libbsa.lib`. |
| README boundary PowerShell check | Passed. |
| Local CMake/source acceptance checks | Passed; no source globbing token, expected source/header patterns present, and `src/libbsa.cpp` has no `TES5Edit` token. |
| `git status --short TES5Edit` | Passed; no TES5Edit changes. |

## User Setup Required

Install Visual Studio 17 2022 or update the local configure command to an installed generator before using the committed `windows-msvc-vcpkg` preset on this machine. No external services are required.

## Known Stubs

None found in plan-created or plan-modified files.

## Next Phase Readiness

- The repository now has a buildable library scaffold for later result API and test foundation work.
- Later plans can add concrete `result<T>` behavior and Catch2/CTest targets against the committed CMake/vcpkg plumbing.

---
*Phase: 01-build-error-and-test-foundation*
*Completed: 2026-05-05*

## Self-Check: PASSED

All expected files and task commits were found.
