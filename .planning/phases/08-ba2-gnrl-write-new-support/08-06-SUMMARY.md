---
phase: 08-ba2-gnrl-write-new-support
plan: 06
subsystem: archive-writer
tags: [cpp20, ba2, gnrl, writer, deduplication, ctest]

requires:
  - phase: 08-ba2-gnrl-write-new-support
    provides: Public BA2 GNRL writer contract, raw serialization, end filename-table support, and compression routing
provides:
  - Reader-backed regression coverage for opt-in BA2 GNRL final-stored-byte deduplication
  - Verification that compressed duplicate stored payloads share offsets while raw/compressed source twins remain distinct
  - Final Phase 8 regression evidence across writer, BA2 reader/DX10, TES4 writer, public boundary, full CTest, and TES5Edit cleanliness
affects: [ba2-gnrl-writer, archive-reader-validation, phase-09-ba2-dx10-writer]

tech-stack:
  added: []
  patterns:
    - BA2 GNRL dedupe tests assert public archive_reader metadata and extraction behavior, not writer internals
    - Final stored payload vectors after compression routing remain the dedupe key

key-files:
  created:
    - .planning/phases/08-ba2-gnrl-write-new-support/08-06-SUMMARY.md
  modified:
    - src/formats/ba2/ba2_gnrl_writer.cpp
    - tests/unit/ba2_gnrl_writer_tests.cpp

key-decisions:
  - "BA2 GNRL writer deduplication remains opt-in through ba2_gnrl_writer_options::deduplicate_payloads and is disabled by default."
  - "Dedupe eligibility is based on byte-identical final stored payload vectors after raw/compressed routing, not source bytes alone."

patterns-established:
  - "Reader-backed dedupe proof: reopened entry metadata payload offsets and extraction bytes are the acceptance oracle."
  - "Verification-only closeout tasks record command evidence in the summary rather than adding production changes."

requirements-completed: [WBA2-01, WBA2-02, WBA2-03, WBA2-04, WBA2-05]

duration: 2min
completed: 2026-05-09
---

# Phase 08 Plan 06: BA2 GNRL Final Stored-Byte Deduplication Summary

**Opt-in BA2 GNRL writer deduplication is now covered by reader-backed offset and extraction tests, with full Phase 8 regression gates green.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-09T08:22:11Z
- **Completed:** 2026-05-09T08:24:34Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Added BA2 GNRL writer tests proving duplicate source bytes keep distinct offsets by default and share offsets only when `deduplicate_payloads = true`.
- Added compressed duplicate coverage proving the dedupe key is the final stored payload while a raw entry with the same source bytes keeps a distinct offset.
- Ran final Phase 8 closeout gates: targeted BA2 writer tests, BA2 reader/DX10/TES4 writer/public boundary regressions, full CTest, and clean `TES5Edit/` status.

## Task Commits

Each implementation task was committed atomically:

1. **Task 1 RED: Add BA2 final-stored-byte dedupe tests** - `80cbae0` (test)
2. **Task 2 GREEN: Implement BA2 final-stored-byte deduplication** - `f553c5c` (feat)
3. **Task 3: Run final Phase 8 regression gates** - verification-only; evidence recorded below and in this metadata commit

_Note: This was a TDD plan, but the RED gate passed unexpectedly because the BA2 GNRL dedupe implementation was already present from earlier Phase 8 writer serialization work._

## Files Created/Modified

- `tests/unit/ba2_gnrl_writer_tests.cpp` - Adds public-reader-backed dedupe tests for disabled, enabled, compressed, and raw/compressed source-twin cases.
- `src/formats/ba2/ba2_gnrl_writer.cpp` - Documents that the dedupe key is the exact final stored payload byte span after compression routing.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-06-SUMMARY.md` - Records plan outcome, verification evidence, TDD gate status, and Phase 8 closeout state.

## Decisions Made

- Kept BA2 GNRL dedupe disabled by default and activated only through `ba2_gnrl_writer_options::deduplicate_payloads`.
- Preserved final-stored-byte semantics: duplicate compressed entries may share offsets only after compression output bytes match, while raw/compressed source twins remain distinct.
- Treated Task 3 as a verification-only task with evidence in this summary rather than adding artificial production changes.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Used public `payload_offset` metadata instead of nonexistent `.offset` field**
- **Found during:** Task 1 (RED dedupe tests)
- **Issue:** The plan wording referred to reopened `entry_metadata.offset`, but the public C++20 metadata field is `payload_offset`; initial tests failed to compile when using `.offset`.
- **Fix:** Updated assertions to compare `payload_offset` values through `archive_reader::find` results.
- **Files modified:** `tests/unit/ba2_gnrl_writer_tests.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` passed.
- **Committed in:** `80cbae0`

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** The fix aligned tests with the existing public API without changing scope or behavior.

## Known Stubs

None.

## Threat Flags

None.

## Issues Encountered

- RED gate passed unexpectedly: after adding the planned failing tests, targeted BA2 writer tests passed because opt-in stored-payload dedupe already existed in `src/formats/ba2/ba2_gnrl_writer.cpp` from earlier Phase 8 writer serialization work (`803004e`). This is documented under TDD Gate Compliance.

## Verification

- Build before targeted tests: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — passed.
- Task 1 targeted BA2 writer run: `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` — passed, 14/14 tests. This was an unexpected GREEN for the RED gate because implementation already existed.
- Task 2 targeted BA2 writer run: `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` — passed, 14/14 tests.
- Final targeted writer gate: `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` — passed, 14/14 tests.
- Final regression gate: `ctest --preset windows-msvc-debug-static -R "ba2_gnrl_reader|ba2_dx10|tes4_bsa_writer|public_include_boundary" --output-on-failure` — passed, 15/15 matching tests.
- Final full suite: `ctest --preset windows-msvc-debug-static --output-on-failure` — passed, 124/124 tests with `local game fixtures are opt-in` skipped as expected.
- TES5Edit cleanliness: `git -C TES5Edit status --short` — passed with no output.
- Final combined exit codes: `targeted=0 regression=0 full=0 tes5edit=0`.

## TDD Gate Compliance

- RED commit present before Task 2: `80cbae0 test(08-06): add BA2 GNRL dedupe regression tests`.
- RED behavior violation: the new tests did **not** fail before Task 2 because dedupe behavior was already implemented by earlier Phase 8 commits.
- GREEN commit present after RED: `f553c5c feat(08-06): document BA2 stored-byte dedupe key`.
- REFACTOR commit: not needed; no separate cleanup changes were made after GREEN.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 8 BA2 GNRL writer support is complete and ready for Phase 9 BA2 DX10 write-new planning. The writer now has public API coverage, target-profile serialization, end filename tables, raw and compressed payload routing, Starfield metadata, opt-in dedupe coverage, full regression evidence, and a clean `TES5Edit/` boundary.

## Self-Check: PASSED

- Found modified implementation file: `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Found modified test file: `tests/unit/ba2_gnrl_writer_tests.cpp`.
- Found summary file: `.planning/phases/08-ba2-gnrl-write-new-support/08-06-SUMMARY.md`.
- Found task commits: `80cbae0`, `f553c5c`.

---
*Phase: 08-ba2-gnrl-write-new-support*
*Completed: 2026-05-09*
