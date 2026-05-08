---
phase: 01-foundation-api-boundary-and-test-harness
plan: 05
subsystem: ci-testing
tags: [github-actions, cmake, ctest, package-consumer, msvc]
requires:
  - phase: 01-03
    provides: Catch2 and CTest harness with preset-based validation
  - phase: 01-04
    provides: Read-only TES5Edit and fixture policy boundaries
provides:
  - Installed-package consumer smoke test using find_package and libbsa::libbsa
  - Windows/MSVC static and shared CI workflow
  - Static/shared local validation gate for Phase 1 foundation
affects: [phase-01, phase-02, ci, package-consumption]
tech-stack:
  added: [GitHub Actions package-consumer CTest smoke test]
  patterns: [installed-package-smoke-test, windows-static-shared-matrix, shared-runtime-dll-path-handling]
key-files:
  created: [.github/workflows/ci.yml, tests/package-consumer/CMakeLists.txt, tests/package-consumer/main.cpp, tests/package-consumer/smoke.cmake]
  modified: [CMakeLists.txt, tests/CMakeLists.txt]
key-decisions:
  - "Installed-package validation uses a separate consumer CMake project configured with CMAKE_PREFIX_PATH instead of source-tree include paths."
  - "Windows shared-library verification exports symbols automatically for the current source-anchor DLL and supplies runtime DLL paths during tests."
  - "CI validates both windows-msvc-debug-static and windows-msvc-debug-shared presets and fails if TES5Edit has any git status output."
patterns-established:
  - "Package consumers should link only against the exported libbsa::libbsa target."
  - "Shared Windows tests must provide DLL discovery paths rather than relying on ambient PATH or source-tree artifacts."
requirements-completed: [DOC-04, FND-01, FND-02, FND-03, FND-06]
duration: 35 min
completed: 2026-05-08
---

# Phase 01 Plan 05: CI and Package Smoke Validation Summary

**Windows/MSVC static and shared CI plus installed-package consumer validation through exported CMake targets**

## Performance

- **Duration:** 35 min
- **Started:** 2026-05-08T01:05:00Z
- **Completed:** 2026-05-08T01:40:00Z
- **Tasks:** 3 completed
- **Files modified:** 6

## Accomplishments

- Added a separate package-consumer CMake project that includes `<libbsa/libbsa.hpp>`, calls the public `archive_reader` facade, and links through `libbsa::libbsa` from an installed package.
- Wired `package_consumer_smoke` into CTest so local verification installs libbsa to a build-local prefix, configures the consumer with `CMAKE_PREFIX_PATH`, builds it, and runs it.
- Added GitHub Actions CI for Windows/MSVC static and shared presets with configure/build/test steps and a `TES5Edit/` read-only guard.
- Verified static and shared local preset builds, unit-label tests, package-consumer smoke tests, public-header dependency grep gates, and the `TES5Edit/` status guard.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add installed-package consumer smoke project** - `8755200` (test)
2. **Task 2: Add Windows/MSVC static and shared CI workflow** - `d4b86eb` (ci)
3. **Task 3: Run final static/shared foundation verification** - `54d2ceb` (fix)

## Files Created/Modified

- `.github/workflows/ci.yml` - Windows/MSVC static/shared CI matrix with configure/build/test steps and `TES5Edit/` guard.
- `tests/package-consumer/CMakeLists.txt` - Separate installed-package consumer project using `find_package(libbsa CONFIG REQUIRED)` and `libbsa::libbsa`.
- `tests/package-consumer/main.cpp` - Public-header/link smoke executable that expects Phase 1 archive opens to return `error_code::unsupported`.
- `tests/package-consumer/smoke.cmake` - CTest script that installs libbsa, configures/builds the consumer, and runs its tests.
- `tests/CMakeLists.txt` - Registers `package_consumer_smoke` and provides shared DLL paths for Catch2 test discovery.
- `CMakeLists.txt` - Enables automatic Windows symbol export so the shared build produces a usable DLL/import library during Phase 1.

## Decisions Made

- Used installed-package consumption as the package boundary proof instead of source-tree includes, because the plan required exported target identity and package config validation.
- Kept CI focused on configure/build/test and `TES5Edit/` status checks only; no formatting, staging, or commands that mutate the read-only reference submodule were added.
- Enabled `WINDOWS_EXPORT_ALL_SYMBOLS` for the current library target to make the Phase 1 source-anchor DLL linkable without designing a final export macro policy prematurely.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed shared Windows runtime discovery for local verification**
- **Found during:** Task 3 (Run final static/shared foundation verification)
- **Issue:** `ctest --preset windows-msvc-debug-shared --output-on-failure` failed while listing Catch2 tests because the shared test executable could not locate `libbsa.dll`.
- **Fix:** Enabled Windows symbol export for the shared library target, passed `$<TARGET_FILE_DIR:libbsa>` to `catch_discover_tests(DL_PATHS ...)`, and copied package-consumer runtime DLLs beside the consumer executable after build.
- **Files modified:** `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/package-consumer/CMakeLists.txt`
- **Verification:** `cmake --preset windows-msvc-debug-shared && cmake --build --preset windows-msvc-debug-shared && ctest --preset windows-msvc-debug-shared --output-on-failure` passes.
- **Committed in:** `54d2ceb`

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** The fix was required to complete the planned shared-library verification and did not expand the public API or add dependencies.

## Issues Encountered

- Shared preset CTest initially failed with Windows loader exit code `0xc0000135` during Catch2 discovery; resolved by adding explicit runtime DLL path handling.
- CMake emitted a vcpkg warning about `builtin-baseline` being present while `vcpkg-configuration.json` overrides the default registry; this is pre-existing manifest behavior and did not block configure/build/test.

## Known Stubs

None - no placeholder or unwired data stubs were found in files created or modified by this plan.

## User Setup Required

None - local validation succeeded with the available `VCPKG_ROOT` and installed Visual Studio generator.

## Next Phase Readiness

Phase 1 is ready for Phase 2 planning/execution: the CMake package boundary, public API stub, test harness, fixture policy, CI workflow, and installed-package smoke validation are all in place.

## Self-Check: PASSED

- Found `.github/workflows/ci.yml`, package-consumer files, `tests/CMakeLists.txt`, `CMakeLists.txt`, and this summary file.
- Found task commits `8755200`, `d4b86eb`, and `54d2ceb` in git history.
- Verified static preset configure/build/test, shared preset configure/build/test, unit-label tests, package-consumer smoke test, public-header grep gates, and `TES5Edit/` status guard.

---
*Phase: 01-foundation-api-boundary-and-test-harness*
*Completed: 2026-05-08*
