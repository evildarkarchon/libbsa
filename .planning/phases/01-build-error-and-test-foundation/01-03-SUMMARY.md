---
phase: 01-build-error-and-test-foundation
plan: 03
subsystem: testing
tags: [cmake, ctest, catch2, smoke, public-headers]

requires:
  - phase: 01-build-error-and-test-foundation
    provides: C++20 libbsa target and local result/error public API from plans 01 and 02
provides:
  - Consumer-style public-header smoke target for <libbsa/result.hpp>
  - CTest label hygiene with Phase 1 unit and smoke lanes
  - README guidance for full, unit, and smoke test execution plus reserved future labels
affects: [testing, public-api, validation, documentation]

tech-stack:
  added: []
  patterns: [explicit CMake test registration, labeled CTest lanes, consumer public-header smoke test]

key-files:
  created: [tests/public_header_smoke.cpp]
  modified: [CMakeLists.txt, README.md]

key-decisions:
  - "Used the established local Visual Studio 18 2026 fallback build directory only for verification because the committed Visual Studio 17 2022 preset could not configure in this environment."

patterns-established:
  - "Public-header smoke coverage is a separate CTest target labeled smoke and links only libbsa::libbsa."
  - "Phase 1 label policy keeps unit and smoke active while reserving fixture, roundtrip, compat, and slow for later phases."

requirements-completed: [FND-02, FND-04, VAL-01]

duration: 3min
completed: 2026-05-05
---

# Phase 01 Plan 03: Public Header Smoke and CTest Label Foundation Summary

**Consumer-style public header smoke coverage with CTest unit/smoke label guidance and TES5Edit boundary gates**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-05T11:40:21Z
- **Completed:** 2026-05-05T11:42:36Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added `tests/public_header_smoke.cpp` as a consumer translation unit that includes only `<libbsa/result.hpp>` and verifies `libbsa::success()` / `result<void>::has_value()`.
- Registered `libbsa_public_header_smoke` in CMake, linked it only to `libbsa::libbsa`, and exposed it through CTest with the `smoke` label while preserving the Catch2 `unit` label.
- Updated README test guidance for full, `unit`, and `smoke` CTest runs and documented Phase 1 label/public-header boundaries.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add public-header isolation smoke target** - `febaa35` (feat)
2. **Task 2: Finalize CTest labels, foundation docs, and boundary gates** - `032c936` (docs)

**Plan metadata:** captured in the final docs commit for this summary and state update.

## Files Created/Modified

- `tests/public_header_smoke.cpp` - Consumer-style public-header isolation executable source.
- `CMakeLists.txt` - Adds the smoke executable/test and `smoke` label while preserving `unit` discovery for foundation tests.
- `README.md` - Documents full/unit/smoke CTest commands, Phase 1 label policy, public-header dependency boundaries, and deferred archive/test scopes.

## Decisions Made

- Used `build/local-vs2026-vcpkg` with the Visual Studio 18 2026 generator for verification only after the committed `windows-msvc-vcpkg` preset failed because Visual Studio 17 2022 was unavailable. The committed preset was not changed.

## Deviations from Plan

None - plan implementation was executed exactly as written. Verification used the user-approved local fallback build directory because the committed preset was blocked by environment availability.

## Issues Encountered

- `cmake --preset windows-msvc-vcpkg` restored vcpkg dependencies but failed during configure because no Visual Studio 17 2022 instance was available. Verification continued with `cmake -S . -B build/local-vs2026-vcpkg -G "Visual Studio 18 2026" -A x64 ...` as instructed.

## Verification

- `cmake --preset windows-msvc-vcpkg` - blocked by missing Visual Studio 17 2022 instance; preset left unchanged.
- `cmake -S . -B build/local-vs2026-vcpkg -G "Visual Studio 18 2026" -A x64 -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DBUILD_TESTING=ON` - passed.
- `cmake --build build/local-vs2026-vcpkg --config Debug` - passed.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` - passed, 7/7 tests.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -L unit -C Debug` - passed, 6/6 tests.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -L smoke -C Debug` - passed, 1/1 test.
- Final PowerShell gate for TES5Edit status, no CMake `GLOB`, result header dependency tokens, and README labels/reserved labels - passed.

## Known Stubs

None.

## Self-Check: PASSED

- Found `tests/public_header_smoke.cpp`, `CMakeLists.txt`, `README.md`, and this summary artifact.
- Found task commits `febaa35` and `032c936` in git history.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 1 now has full, `unit`, and `smoke` CTest lanes ready for later parser, fixture, round-trip, compatibility, and slow labels when those test types exist.
- Public-header isolation is locked for the current result/error API before later archive parsing, compression, DDS, and writer phases expand the library.

---
*Phase: 01-build-error-and-test-foundation*
*Completed: 2026-05-05*
