---
phase: 08-ba2-gnrl-write-new-support
plan: 07
subsystem: archive-writer
tags: [cpp20, ba2, gnrl, writer, filesystem-safety, catch2]

# Dependency graph
requires:
  - phase: 08-ba2-gnrl-write-new-support
    provides: BA2 GNRL writer API, serialization, compression routing, and deduplication from plans 08-01 through 08-06
provides:
  - Filesystem-safe BA2 GNRL publish path with unique temporary directories and overwrite backup rollback
  - Public writer regression tests for temp collision preservation, unsafe overwrite targets, and no-overwrite preservation
affects: [phase-08-verification, ba2-gnrl-writer, filesystem-publish]

# Tech tracking
tech-stack:
  added: []
  patterns: [std::filesystem error_code overloads, unique sibling temp directories, backup-and-rollback publish]

key-files:
  created: [.planning/phases/08-ba2-gnrl-write-new-support/08-07-SUMMARY.md]
  modified: [src/formats/ba2/ba2_gnrl_writer.cpp, tests/unit/ba2_gnrl_writer_tests.cpp]

key-decisions:
  - "BA2 GNRL writer publish now reserves a unique sibling temporary directory instead of using and deleting deterministic `<output>.tmp` files."
  - "Overwrite mode rejects non-regular existing destinations and uses backup+rollback before publishing replacement bytes."

patterns-established:
  - "Caller-controlled filesystem probes in public writer finalization use std::error_code overloads and return libbsa::error_code::io_error."
  - "Publish cleanup is best-effort while primary write/publish failures remain the returned error."

requirements-completed: [WBA2-01, WBA2-02, WBA2-03, WBA2-04, WBA2-05]

# Metrics
duration: 9min
completed: 2026-05-09
---

# Phase 08 Plan 07: BA2 GNRL Publish Safety Summary

**Filesystem-safe BA2 GNRL finalization using unique temp directories, regular-file overwrite checks, and backup rollback**

## Performance

- **Duration:** 9 min
- **Started:** 2026-05-09T08:32:30Z
- **Completed:** 2026-05-09T08:41:34Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Added public writer regressions that first failed against the deterministic `.tmp` deletion and directory-overwrite behavior.
- Replaced delete-then-rename publishing with a unique temporary publish directory and backup/rollback replacement sequence.
- Converted caller-controlled publish probes to non-throwing `std::error_code` filesystem APIs and verified related BA2/DX10/public-boundary regressions.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add publish-safety regression tests** - `ebaf82a` (test)
2. **Task 2: Replace deterministic delete-then-rename publish with unique temp and rollback** - `fa71099` (fix)

**Plan metadata:** pending final docs commit

_Note: TDD RED was verified before implementation: new publish-safety tests failed for temp collision deletion and unsafe directory overwrite, then passed after the fix._

## Files Created/Modified
- `tests/unit/ba2_gnrl_writer_tests.cpp` - Added temp-name collision preservation, unsafe overwrite-directory rejection, and existing-file preservation assertions.
- `src/formats/ba2/ba2_gnrl_writer.cpp` - Added non-throwing existence checks, unique publish directory reservation, best-effort cleanup, backup path reservation, and rollback-aware publish logic.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-07-SUMMARY.md` - Execution summary and verification record.

## Decisions Made
- BA2 GNRL writer publish now reserves a unique sibling temporary directory instead of using and deleting deterministic `<output>.tmp` files.
- Overwrite mode rejects non-regular existing destinations and uses backup+rollback before publishing replacement bytes.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- TDD RED behaved as expected: `safe-temp-collision.ba2.tmp` was deleted by old code and directory overwrite unexpectedly succeeded. Both failures were resolved by the planned publish refactor.

## TDD Gate Compliance

- RED gate commit: `ebaf82a` (`test(08-07): add BA2 publish safety regressions`)
- GREEN gate commit: `fa71099` (`fix(08-07): make BA2 GNRL publish filesystem-safe`)

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — passed
- `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` — passed, 16/16 tests
- `ctest --preset windows-msvc-debug-static -R "ba2_gnrl_reader|ba2_dx10|tes4_bsa_writer|public_include_boundary" --output-on-failure` — passed, 15/15 tests
- `git -C TES5Edit status --short` — passed, no output

## Known Stubs

None.

## Threat Flags

None - publish filesystem trust boundaries were already listed in the plan threat model and mitigated here.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 8 verification gap is closed for public BA2 GNRL writer publish/finalization safety.
- TES5Edit remains read-only and clean.

## Self-Check: PASSED

- Found `src/formats/ba2/ba2_gnrl_writer.cpp`
- Found `tests/unit/ba2_gnrl_writer_tests.cpp`
- Found `.planning/phases/08-ba2-gnrl-write-new-support/08-07-SUMMARY.md`
- Found commit `ebaf82a`
- Found commit `fa71099`

---
*Phase: 08-ba2-gnrl-write-new-support*
*Completed: 2026-05-09*
