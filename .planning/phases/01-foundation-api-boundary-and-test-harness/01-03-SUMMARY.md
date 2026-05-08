---
phase: 01-foundation-api-boundary-and-test-harness
plan: 03
subsystem: testing
tags: [catch2, ctest, labels, fixtures]
requires:
  - phase: 01-02
    provides: Public API tests and initial test target
provides:
  - Catch2 tests discoverable through CTest labels
  - Local game fixture skip-by-default smoke test
affects: [phase-01, testing, future-fixtures]
tech-stack:
  added: [Catch2, CTest]
  patterns: [catch-tags-as-ctest-labels, skipped-local-fixture-tests]
key-files:
  created: [tests/unit/local_game_fixture_tests.cpp]
  modified: [CMakePresets.json, tests/CMakeLists.txt]
key-decisions:
  - "CTest label discovery uses Catch2 tags through ADD_TAGS_AS_LABELS."
  - "Local game fixture tests are discovered but skipped when LIBBSA_GAME_FIXTURES is unset."
patterns-established:
  - "Use `[unit]`, `[public-api]`, and `[requires-game-fixture]` Catch2 tags to drive CTest selection."
requirements-completed: [FND-06, FND-03, FND-05]
duration: 10 min
completed: 2026-05-08
---

# Phase 01 Plan 03: Catch2 and CTest Harness Summary

**Catch2 tests mapped to CTest labels with skipped local-game-fixture discovery behavior**

## Performance

- **Duration:** 10 min
- **Started:** 2026-05-08T00:55:00Z
- **Completed:** 2026-05-08T01:05:00Z
- **Tasks:** 2 completed
- **Files modified:** 3

## Accomplishments

- Verified Catch2/CTest integration with `ADD_TAGS_AS_LABELS` and the static preset configure path.
- Added a `requires-game-fixture` local fixture test that is discovered by CTest but skipped by default with setup hints.
- Verified `ctest -L unit` selects all unit tests and reports the local game fixture test as skipped, not failed.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add Catch2 test target and CTest discovery labels** - `f926dca` (fix)
2. **Task 2: Add local game fixture discovery skip test** - `bb43973` (test)

## Files Created/Modified

- `CMakePresets.json` - Adjusted presets to use the available local CMake generator and Debug configuration.
- `tests/CMakeLists.txt` - Includes all current unit test files in `libbsa_tests` and maps Catch2 tags to CTest labels.
- `tests/unit/local_game_fixture_tests.cpp` - Skipped-by-default local game fixture discovery test.

## Decisions Made

- Kept local fixture tests discoverable by default so CI and maintainers can see the optional validation path without requiring game data.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Made presets independent of missing local Ninja executable**
- **Found during:** Task 1 (Add Catch2 test target and CTest discovery labels)
- **Issue:** `cmake --preset windows-msvc-debug-static` failed locally because `Ninja` was not on PATH.
- **Fix:** Removed the hardcoded generator from presets and added Debug build/test configuration fields so CMake can use the installed Visual Studio generator locally while preserving static/shared preset behavior.
- **Files modified:** `CMakePresets.json`
- **Verification:** `cmake --preset windows-msvc-debug-static` completed successfully.
- **Committed in:** `f926dca`

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** Preset names and static/shared behavior remain intact; local verification is now executable without adding a speculative tool dependency.

## Issues Encountered

- `ctest -L unit` reports the local game fixture test as skipped when `LIBBSA_GAME_FIXTURES` is unset; this is intended behavior.

## User Setup Required

None - local game fixtures are optional and skipped by default.

## Next Phase Readiness

The labeled test harness is ready for Plan 05 package-consumer smoke validation and CI execution.

## Self-Check: PASSED

- Found `tests/unit/local_game_fixture_tests.cpp`.
- Found commits `f926dca` and `bb43973`.
- Verified `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` passes with one skipped local fixture test.
- Verified `ctest --preset windows-msvc-debug-static -N` lists seven tests.

---
*Phase: 01-foundation-api-boundary-and-test-harness*
*Completed: 2026-05-08*
