---
phase: 01-foundation-api-boundary-and-test-harness
plan: 01
subsystem: build
tags: [cmake, vcpkg, cxx20, packaging]
requires: []
provides:
  - C++20 libbsa CMake target
  - Static and shared CMake presets
  - Install/export CMake package metadata
affects: [phase-01, build-system, public-api]
tech-stack:
  added: [CMake, vcpkg]
  patterns: [target-file-sets, install-export-package, explicit-static-shared-presets]
key-files:
  created: [CMakeLists.txt, cmake/libbsaConfig.cmake.in, CMakePresets.json, vcpkg-configuration.json, include/libbsa/version.hpp, src/libbsa.cpp]
  modified: []
key-decisions:
  - "Use CMake target file sets for public headers so install/export metadata owns the include boundary."
  - "Keep vcpkg dependencies directly in the manifest and commit the existing manifest with the preset baseline configuration."
patterns-established:
  - "CMake package exports use libbsa:: namespaced targets."
  - "Static and shared builds are selected through named presets rather than ad-hoc flags."
requirements-completed: [FND-01, FND-02, DOC-04]
duration: 8 min
completed: 2026-05-08
---

# Phase 01 Plan 01: Build and Package Foundation Summary

**C++20 libbsa CMake package skeleton with static/shared presets and vcpkg baseline configuration**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-08T00:24:00Z
- **Completed:** 2026-05-08T00:32:00Z
- **Tasks:** 2 completed
- **Files modified:** 7

## Accomplishments

- Created a root CMake project with a real `libbsa` target, C++20 compile features, warning configuration, public header file set, and install/export package metadata.
- Added version header/source anchors that build without archive parsing or dependency leakage.
- Added explicit static/shared Windows presets plus vcpkg baseline configuration for reproducible dependency restore.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create root CMake project and install/export package skeleton** - `025e9dc` (feat)
2. **Task 2: Add explicit static/shared presets and vcpkg configuration** - `af69c4f` (chore)

## Files Created/Modified

- `CMakeLists.txt` - Root CMake project, library target, public header file set, install/export package rules, and optional test entry point.
- `cmake/libbsaConfig.cmake.in` - Installed package config template.
- `CMakePresets.json` - Static and shared Windows/MSVC Debug configure/build/test presets.
- `vcpkg-configuration.json` - vcpkg builtin registry baseline configuration.
- `vcpkg.json` - Existing vcpkg manifest committed to make the foundation reproducible.
- `include/libbsa/version.hpp` - Public version constants with Doxygen comments.
- `src/libbsa.cpp` - Minimal source anchor for the library target.

## Decisions Made

- Used CMake file sets to keep installed public headers explicit and package-owned.
- Kept vcpkg dependencies directly listed in `vcpkg.json`; no manifest features were introduced for tests.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Committed existing vcpkg manifest with preset baseline work**
- **Found during:** Task 2 (Add explicit static/shared presets and vcpkg configuration)
- **Issue:** `vcpkg.json` already existed but was untracked, leaving dependency manifest state outside the commit history and causing a dirty tree after the task.
- **Fix:** Staged and committed the unchanged manifest together with `vcpkg-configuration.json` so vcpkg manifest mode is reproducible.
- **Files modified:** `vcpkg.json`
- **Verification:** `rg` confirmed `libdeflate`, `lz4`, `directxtex`, and `catch2` remain present.
- **Committed in:** `af69c4f`

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** No scope creep; this preserves the planned vcpkg dependency boundary.

## Issues Encountered

- Local `ninja` is not on PATH, so preset build execution may require installing Ninja or using the CI runner environment. The direct CMake configure/build verification succeeded with the platform default Visual Studio generator.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The build target and preset foundation are ready for Plan 02 public API headers and Plan 03 Catch2/CTest wiring.

## Self-Check: PASSED

- Found `CMakeLists.txt`.
- Found `include/libbsa/version.hpp`.
- Found commits `025e9dc` and `af69c4f`.
- Verified no `TES5Edit` references in `CMakeLists.txt`, `cmake`, `include`, or `src`.

---
*Phase: 01-foundation-api-boundary-and-test-harness*
*Completed: 2026-05-08*
