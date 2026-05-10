---
phase: 10-tes3-write-support-and-bsa-format-completeness
plan: 06
subsystem: archive-writing
tags: [cpp20, tes3, bsa, writer, path-validation, atomic-publish]

requires:
  - phase: 10-tes3-write-support-and-bsa-format-completeness
    provides: TES3 writer public API, serializer, safe publish scaffolding, and verification gap report
provides:
  - Shared archive path embedded-NUL rejection
  - TES3 writer embedded-NUL regression coverage through add_bytes/add_file
  - Verified no-replace publish helper coverage for overwrite_existing=false finalization
affects: [tes3-writer, archive-path-normalization, publish-semantics, WBSA-04]

tech-stack:
  added: []
  patterns: [TDD regression for verifier gaps, shared path validation before format-specific serialization]

key-files:
  created:
    - .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-06-SUMMARY.md
  modified:
    - src/detail/archive_path.cpp
    - tests/unit/archive_path_tests.cpp

key-decisions:
  - "Shared archive path normalization rejects embedded NUL bytes before separator normalization so all callers get the same invalid_argument behavior."
  - "TES3 non-overwrite publish remains routed through publish_file_without_replace after a final destination existence check."

patterns-established:
  - "Archive-internal path validators reject bytes that are incompatible with downstream on-disk string encodings."
  - "No-replace publication is validated at the helper boundary and used by writers after final output existence checks."

requirements-completed: [WBSA-04]

duration: 12min
completed: 2026-05-10
---

# Phase 10 Plan 06: TES3 Writer Gap Closure Summary

**Shared embedded-NUL path rejection plus verified TES3 no-replace publish semantics close Phase 10 WBSA-04 verification gaps.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-05-10T02:14:00Z
- **Completed:** 2026-05-10T02:25:52Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added a RED/GREEN regression for embedded-NUL archive paths in the shared archive path validator.
- Updated `normalize_archive_path` to return `invalid_argument` before NUL bytes can flow into TES3 NUL-terminated name serialization.
- Re-ran the TES3 no-replace publish tests already present from the verification-gap remediation work, confirming final non-overwrite publish uses `publish_file_without_replace` and preserves existing destinations.

## Task Commits

Each task was committed or verified atomically:

1. **Task 1: Reject embedded-NUL archive paths at the shared validator and TES3 writer boundary**
   - `45bce90` (test) — added failing shared archive path embedded-NUL regression.
   - `5ab8c28` (fix) — rejected embedded NUL bytes in `normalize_archive_path`.
2. **Task 2: Enforce final no-replace publish semantics for overwrite_existing=false**
   - Previously present remediation commits verified in this plan: `a2f034e` / `dce8878` implemented final non-overwrite publish race protection and the no-replace helper.
   - Current focused verification passed with the existing task artifacts.

**Plan metadata:** pending final docs commit.

_Note: Task 1 followed the TDD test → fix sequence. Task 2 artifacts were already present from prior Phase 10 review remediation and were re-verified rather than duplicated._

## Files Created/Modified

- `src/detail/archive_path.cpp` - Rejects embedded NUL bytes during shared archive path normalization.
- `tests/unit/archive_path_tests.cpp` - Adds an owning-string embedded-NUL regression test for shared archive paths.

## Decisions Made

- Shared path normalization is the authoritative NUL rejection point because TES3, TES4, BA2, and public lookup paths all depend on normalized archive keys.
- Existing TES3 no-replace publish implementation remains valid: non-overwrite output performs a final existence check and publishes through `detail::publish_file_without_replace`, while overwrite-enabled output uses `detail::replace_file_atomically`.

## Deviations from Plan

### Auto-fixed Issues

None - no unplanned fixes were required during this executor run.

### Pre-existing Gap Remediation

Task 2 code and tests were already present before this executor run, introduced by prior Phase 10 CR/WR remediation commits. The task was not reimplemented to avoid duplicating working code; it was verified against the plan acceptance criteria instead.

## Verification

- RED check: `cmake --build "build/windows-msvc-debug-static" --target libbsa_tests --config Debug && ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path" --output-on-failure` failed on `archive_path rejects embedded-NUL virtual paths` before implementation.
- Task 1 GREEN/focused check: `cmake --build "build/windows-msvc-debug-static" --target libbsa_tests --config Debug && ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path|tes3_bsa_writer" --output-on-failure` passed 21/21 tests.
- Task 2 focused check: `cmake --build "build/windows-msvc-debug-static" --target libbsa_tests --config Debug && ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "tes3_bsa_writer" --output-on-failure` passed 18/18 tests.
- Phase focused check: `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path|tes3_bsa_writer|tes3_bsa_reader|TES4 BSA writer|public_include_boundary" --output-on-failure` passed 43/43 tests.
- Full suite: `ctest --test-dir "build/windows-msvc-debug-static" -C Debug --output-on-failure` passed 174/174 tests, with the expected local game fixture skip.
- TES5Edit cleanliness: `git -C "TES5Edit" status --short` produced no output.

## TDD Gate Compliance

- RED commit present: `45bce90`.
- GREEN commit present after RED: `5ab8c28`.
- No refactor commit was needed.

## Known Stubs

None.

## Issues Encountered

- Task 2 acceptance artifacts were already present before execution. I verified them in place rather than creating redundant code changes.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 10 verification gaps are closed: embedded-NUL archive paths now fail shared validation, and non-overwrite publish semantics are covered by helper-level and writer-level tests.
- WBSA-04 can be re-verified with the full suite and TES5Edit cleanliness check.

## Self-Check: PASSED

- Found modified implementation file: `src/detail/archive_path.cpp`.
- Found modified regression test file: `tests/unit/archive_path_tests.cpp`.
- Found RED commit: `45bce90`.
- Found GREEN commit: `5ab8c28`.

---
*Phase: 10-tes3-write-support-and-bsa-format-completeness*
*Completed: 2026-05-10*
