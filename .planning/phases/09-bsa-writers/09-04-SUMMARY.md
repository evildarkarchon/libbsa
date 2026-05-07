---
phase: 09-bsa-writers
plan: 04
subsystem: bsa-writer
tags: [cpp20, bsa, writer, disk-input, tdd, catch2]
requires:
  - phase: 09-bsa-writers
    provides: TES4-family BSA writer planning/finalization from plans 09-01 through 09-03
provides:
  - Disk-backed BSA planning tests proving memory equivalence and plan-owned payload bytes
  - BSA writer structured failure coverage for disk inputs, duplicate paths, invalid paths, layout overflow, unsupported targets, and sink failures
  - Pre-I/O disk archive-path validation so malformed duplicate disk paths are returned before host file failures
affects: [bsa-writers, writer-validation, disk-inputs]
tech-stack:
  added: []
  patterns: [TDD RED-GREEN, plan-owned disk payloads, validate archive paths before host file reads]
key-files:
  created: []
  modified: [tests/bsa_writer_tests.cpp, src/bsa_writer.cpp]
key-decisions:
  - "Disk-backed BSA inputs validate caller-provided archive virtual paths before opening host files, preserving structured archive-path errors ahead of filesystem I/O failures."
patterns-established:
  - "Disk input tests use explicit host-file plus archive-virtual-path pairs and compare generated archives through open_bsa/extract_bsa_entry."
  - "Disk planner validation rejects duplicate normalized archive paths before host file reads to avoid host/archive path confusion."
requirements-completed: [WRT-01]
duration: 3min
completed: 2026-05-07
---

# Phase 09 Plan 04: Disk Input Planning and Sink Failure Safety Summary

**Disk-backed BSA planning validated through read-back equivalence, plan-owned payload tests, and pre-I/O archive-path failure handling.**

## Performance

- **Duration:** 3min
- **Started:** 2026-05-07T06:59:44Z
- **Completed:** 2026-05-07T07:02:32Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added disk-backed BSA tests covering memory/disk read-back equivalence, planning-time disk byte ownership, missing disk inputs, malformed archive paths, duplicate paths, impossible layout arithmetic, unsupported target combinations, and unchanged sink failure propagation.
- Added a stricter RED case proving duplicate normalized disk archive paths are rejected before host files are opened.
- Implemented disk-entry archive-path validation before filesystem reads, preserving structured malformed-archive errors before I/O failures.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing disk equivalence and finalization failure tests** - `2a9cbfd` (test)
2. **Task 2 GREEN: Implement disk planning and failure propagation** - `95c401a` (feat)

**Plan metadata:** recorded in the plan metadata docs commit history

## Files Created/Modified

- `tests/bsa_writer_tests.cpp` - Added disk-backed BSA equivalence, planning-time ownership, duplicate/invalid/missing input, layout overflow, unsupported target, and sink failure tests.
- `src/bsa_writer.cpp` - Added disk archive-path validation before host file reads in `plan_bsa_write_from_disk`.

## Decisions Made

- Disk-backed inputs now validate archive virtual paths before opening host files. This preserves D-05/D-08 separation between host paths and archive paths and ensures malformed archive-input errors are not masked by missing or unreadable host files.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Added pre-I/O disk archive-path validation**
- **Found during:** Task 1 RED
- **Issue:** The initially requested disk/failure tests mostly passed because disk planning already existed, but a stricter disk duplicate-path case showed duplicate normalized archive paths could be masked by a later missing host file and returned `io_failure` instead of `malformed_archive`.
- **Fix:** Added `validate_disk_entry_archive_paths` and call it before reading disk files in `plan_bsa_write_from_disk`.
- **Files modified:** `tests/bsa_writer_tests.cpp`, `src/bsa_writer.cpp`
- **Verification:** `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_bsa_writer_tests|libbsa_writer_tests"` passed.
- **Committed in:** `95c401a`

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** The auto-fix tightened correctness at the host/archive trust boundary without adding directory traversal, policy helpers, dependencies, or public API surface.

## Issues Encountered

- The first RED run unexpectedly passed because the planned disk-backed behavior was already largely present in `src/bsa_writer.cpp`. I tightened the RED step with a disk duplicate-path validation-order test that failed for the intended missing behavior before implementing the fix.

## Known Stubs

None - stub scan found no TODO/FIXME/placeholder or hardcoded empty UI data patterns in changed files.

## User Setup Required

None - no external service configuration required.

## Verification

- `rg -n "disk-backed and memory-backed|read during planning|duplicate normalized|invalid archive paths|missing disk inputs|impossible BSA layout|unsupported BSA target|first sink failure" tests/bsa_writer_tests.cpp` found all required RED cases.
- RED focused run failed on `duplicate normalized disk BSA paths fail before host file reads` with `io_failure` before GREEN.
- GREEN focused run passed all `libbsa_bsa_writer_tests` tests.
- Plan-level focused verification passed: `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_bsa_writer_tests|libbsa_writer_tests"`.
- Directory policy check found no `recursive`, `directory_iterator`, `symlink`, or `exclude` additions in `include/libbsa/bsa_writer.hpp` or `src/bsa_writer.cpp`.
- TDD gate commits exist: RED `2a9cbfd`, GREEN `95c401a`.

## TDD Gate Compliance

- **RED:** `2a9cbfd` added failing disk/failure coverage; focused run failed before implementation.
- **GREEN:** `95c401a` implemented pre-I/O disk archive-path validation; focused tests passed.
- **REFACTOR:** Not needed.

## Threat Flags

None - this plan covered the expected host filesystem to disk planning and finalization to byte_sink trust boundaries from the plan threat model.

## Next Phase Readiness

- Plan 09-04 is complete and ready for remaining Phase 09 writer plans.
- Disk-backed TES4-family BSA inputs now have focused read-back and failure coverage.

## Self-Check: PASSED

- Found summary file and modified source/test files on disk.
- Found RED commit `2a9cbfd` and GREEN commit `95c401a` in git history.
- Focused plan verification passed after GREEN.

---
*Phase: 09-bsa-writers*
*Completed: 2026-05-07*
