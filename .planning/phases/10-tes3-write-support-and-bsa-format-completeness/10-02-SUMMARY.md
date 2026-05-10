---
phase: 10-tes3-write-support-and-bsa-format-completeness
plan: 02
subsystem: bsa-writer
tags: [cpp20, tes3, bsa, writer, tdd, safe-publish]

requires:
  - phase: 10-tes3-write-support-and-bsa-format-completeness
    provides: public TES3 writer contract and private bridge from 10-01
provides:
  - TES3 writer source ownership validation and late disk-source reads
  - TES3 raw archive serialization sufficient for validation and publish tests
  - unique-temp safe publish with overwrite backup/rollback behavior
affects: [tes3-writer, bsa-format-completeness, phase-10-plan-03]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN/REFACTOR for TES3 writer finalization behavior
    - unique sibling `.libbsa-tmp-N` publish directory with overwrite backup rollback
    - stable `libbsa::error_code` assertions instead of diagnostic text

key-files:
  created:
    - tests/unit/tes3_bsa_writer_tests.cpp
  modified:
    - tests/CMakeLists.txt
    - src/formats/bsa/tes3_bsa_writer.cpp

key-decisions:
  - "TES3 writer prepares and validates every source before reserving a publish temp directory, preventing partial output on missing disk sources."
  - "TES3 writer safe publish uses unique `.libbsa-tmp-N` sibling directories and overwrite backups instead of deterministic `<output>.tmp` replacement."
  - "TES3 writer validation tests assert stable error_code values and no diagnostic strings."

patterns-established:
  - "TES3 writer finalization: validate duplicates, late-read disk sources, sort by TES3 hash sort key, then publish atomically."
  - "CTest-filterable TES3 writer test names include `tes3_bsa_writer` as well as the Catch2 tag."

requirements-completed: [WBSA-04]

duration: 4min
completed: 2026-05-10
---

# Phase 10 Plan 02: Source Ownership, Validation, and Safe Publish Summary

**TES3 writer finalization now validates source state, copies memory payloads, late-reads disk sources, serializes minimal raw TES3 archives, and publishes through overwrite-safe unique temp directories.**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-10T00:05:55Z
- **Completed:** 2026-05-10T00:10:07Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Added focused `[unit][tes3_bsa_writer]` Catch2 coverage for memory ownership, missing disk-source reporting, invalid paths, duplicate canonical paths, empty finalization, overwrite refusal/replacement, and `<output>.tmp` sibling preservation.
- Implemented TES3 writer finalization validation with stable `invalid_argument`, `format_error`, and `io_error` outcomes.
- Implemented raw TES3 archive serialization and safe output publishing using `.libbsa-tmp-N` temp directories and overwrite backup/rollback.

## TDD Cycle Notes

- **RED:** `c696721` added TES3 writer validation/publish tests and registered them in `tests/CMakeLists.txt`; the focused Catch2 run failed against the existing `unsupported` serializer stub.
- **GREEN:** `db0611d` implemented validation, source preparation, raw TES3 serialization, CTest-filterable names, and safe publish behavior; focused TES3 writer tests passed.
- **REFACTOR:** `5e22917` simplified header-write error flow and documented why all sources are prepared before reserving a publish path.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED validation, ownership, and publish tests** - `c696721` (test)
2. **Task 2: GREEN finalization validation and safe publish behavior** - `db0611d` (feat)
3. **Task 3: REFACTOR publish cleanup and comments** - `5e22917` (refactor)

**Plan metadata:** final `docs(10-02)` metadata commit (see completion output)

## Files Created/Modified

- `tests/unit/tes3_bsa_writer_tests.cpp` - New TES3 writer unit tests for source ownership, stable error codes, duplicate validation, overwrite behavior, and temp sibling preservation.
- `tests/CMakeLists.txt` - Registers the TES3 writer tests in `libbsa_tests`.
- `src/formats/bsa/tes3_bsa_writer.cpp` - Implements TES3 finalization validation, late source reads, minimal raw TES3 serialization, and safe publish.

## Decisions Made

- TES3 writer prepares every source payload before publish reservation so source failures return without leaving partial archives.
- Safe publish follows the Phase 8/9 hardened pattern: unique `.libbsa-tmp-N` directory plus backup/rollback for overwrite mode.
- Tests intentionally assert stable `error_code` values and avoid diagnostic string checks per D-17.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Configured the missing local CMake build directory**
- **Found during:** Task 1 (RED validation, ownership, and publish tests)
- **Issue:** The planned `build` directory did not contain a CMake cache, so the prescribed build command could not run.
- **Fix:** Ran `cmake --preset windows-msvc-debug-static` and used the configured `build/windows-msvc-debug-static` directory for validation.
- **Files modified:** build artifacts only (untracked/ignored)
- **Verification:** `cmake --build build/windows-msvc-debug-static --target libbsa_tests --config Debug` succeeded.
- **Committed in:** N/A (environment setup only)

**2. [Rule 3 - Blocking] Made TES3 writer tests selectable by the planned CTest regex**
- **Found during:** Task 2 (GREEN finalization validation and safe publish behavior)
- **Issue:** `ctest -R tes3_bsa_writer` matches test names, not Catch2 tags; initial test names only contained the tag, so CTest found no tests.
- **Fix:** Renamed the TES3 writer test cases to include `tes3_bsa_writer` in their names while retaining `[unit][tes3_bsa_writer]` tags.
- **Files modified:** `tests/unit/tes3_bsa_writer_tests.cpp`
- **Verification:** `ctest --test-dir build/windows-msvc-debug-static -C Debug -R "tes3_bsa_writer|public_include_boundary" --output-on-failure` ran 11 tests and passed.
- **Committed in:** `db0611d`

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both fixes were required to run the planned verification gates. No scope expansion beyond TES3 writer validation and publish behavior.

## Issues Encountered

- Initial plan command `ctest --test-dir build -R tes3_bsa_writer --output-on-failure` could not run because no `build` cache existed. The configured preset build directory was used instead.
- CTest name filtering required test names to include `tes3_bsa_writer`; this was fixed during GREEN.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## TDD Gate Compliance

- **RED commit:** `c696721` present before implementation.
- **GREEN commit:** `db0611d` present after RED.
- **REFACTOR commit:** `5e22917` present after GREEN.
- **Status:** Passed.

## Verification

- `cmake --build build/windows-msvc-debug-static --target libbsa_tests --config Debug` — passed.
- `ctest --test-dir build/windows-msvc-debug-static -C Debug -R "tes3_bsa_writer|public_include_boundary" --output-on-failure` — passed (11/11 tests).
- `git -C TES5Edit status --short` — passed (no output; submodule untouched).

## Next Phase Readiness

Plan 03 can build on a fail-closed TES3 writer with source ownership, validation, and safe publish behavior in place. Full byte-level TES3 hash/name/offset/payload proof remains owned by Plan 03.

## Self-Check: PASSED

- Found expected files: `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/CMakeLists.txt`, `src/formats/bsa/tes3_bsa_writer.cpp`, and this summary.
- Found task commits: `c696721`, `db0611d`, and `5e22917`.

---
*Phase: 10-tes3-write-support-and-bsa-format-completeness*
*Completed: 2026-05-10*
