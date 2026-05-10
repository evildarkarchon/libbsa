---
phase: 07-tes4-family-bsa-write-new-support
plan: 03
subsystem: writer
tags: [cpp20, bsa, tes4, writer, roundtrip, tdd]

requires:
  - phase: 07-tes4-family-bsa-write-new-support
    provides: public TES4-family writer API and validation-owned writer state
provides:
  - Raw v103/v104/v105 TES4-family BSA table and payload serialization
  - Reader-backed round-trip tests for disk, memory, zero-byte, and copied-memory entries
  - Hash-sorted multi-folder folder/file tables with derived archive and file flags
affects: [phase-07-compression, phase-07-embedded-names, phase-07-deduplication]

tech-stack:
  added: []
  patterns: [detail::binary_writer serialization, detail::hash_tes4 table ordering, temporary sibling publish]

key-files:
  created: []
  modified:
    - src/formats/bsa/tes4_bsa_writer.cpp
    - tests/unit/tes4_bsa_writer_tests.cpp

key-decisions:
  - "Raw TES4-family writer output is validated exclusively through archive_reader reopen/list/find/contains/extract APIs."
  - "archive_metadata::default_compression remains target codec metadata; all-raw policy is proven through archive flags and per-entry raw compression metadata."

patterns-established:
  - "Writer folder records use TES5Edit-compatible offsets that include the later file-name table length."
  - "Writer output is published through a temporary sibling path before replacing the destination."

requirements-completed: [WBSA-01, WBSA-02, WBSA-03, WBSA-05, WBSA-06, WBSA-10]

duration: 35min
completed: 2026-05-09
---

# Phase 07 Plan 03: Raw TES4-Family BSA Serialization Summary

**Raw v103/v104/v105 TES4-family BSA serialization with reader-backed round trips for disk, memory, zero-byte, and copied-memory payloads.**

## Performance

- **Duration:** 35 min
- **Started:** 2026-05-09T05:49:00Z
- **Completed:** 2026-05-09T06:24:17Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added RED writer-output tests that create all-raw archives for Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105.
- Implemented raw BSA header, folder record, folder block, file-name table, and payload serialization with hash sorting and checked size/offset arithmetic.
- Verified produced archives by reopening with `archive_reader`, listing entries, running `contains`/`find`, extracting bytes, streaming into a sink, and confirming copied memory survives caller mutation.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add raw profile round-trip, sorting, and flag tests** - `3bcefe9` (test)
2. **Task 2 GREEN: Serialize hash-sorted raw TES4-family BSA archives** - `f857f1f` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/tes4_bsa_writer_tests.cpp` - Adds target-profile raw round-trip coverage and copied-memory extraction proof.
- `src/formats/bsa/tes4_bsa_writer.cpp` - Serializes raw TES4-family BSA bytes, derives flags, hash-sorts tables, and publishes via temporary sibling files.
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-03-SUMMARY.md` - Captures execution results and verification evidence.

## Decisions Made

- `archive_metadata::default_compression` continues to report the target-family codec capability established by the reader, while all-raw writer policy is asserted through absent archive compression flags and per-entry `entry_compression::none`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected RED metadata expectation for target default compression**
- **Found during:** Task 2 (GREEN verification)
- **Issue:** The RED test asserted `archive_metadata::default_compression == none` for all-raw archives, but the existing reader contract reports target-family default codec (`deflate` for v103/v104, `lz4_frame` for v105) independent of the archive compression flag.
- **Fix:** Updated the test to assert the established target default metadata and separately assert the all-raw archive flag and per-entry raw compression behavior.
- **Files modified:** `tests/unit/tes4_bsa_writer_tests.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure`
- **Committed in:** `f857f1f`

---

**Total deviations:** 1 auto-fixed (1 Rule 1 bug)
**Impact on plan:** The correction preserves the plan's all-raw behavior proof without changing the existing reader metadata contract.

## Issues Encountered

- The exact plan verification command `ctest --preset windows-msvc-debug-static -R "tes4_bsa_reader|public_include_boundary" --output-on-failure` matched only public boundary test names in the current CTest registry. Public boundary tests passed, and the TES4 writer round-trip suite passed separately.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None beyond the planned writer model→binary archive and generated archive→archive_reader boundaries covered by T-07-03-01 through T-07-03-04.

## TDD Gate Compliance

- RED gate: `3bcefe9 test(07-03): add failing raw TES4 BSA round-trip tests`
- GREEN gate: `f857f1f feat(07-03): serialize raw TES4-family BSA archives`
- REFACTOR gate: not needed

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — PASS
- `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` — PASS (7/7)
- `ctest --preset windows-msvc-debug-static -R "tes4_bsa_reader|public_include_boundary" --output-on-failure` — PASS for matched public boundary tests (2/2)
- `git -C TES5Edit status --short` — PASS (empty)

## Next Phase Readiness

Ready for Plan 07-04 to layer target-routed deflate/LZ4-frame compression defaults and per-entry overrides on top of the raw table serializer.

## Self-Check: PASSED

- Found summary file: `.planning/phases/07-tes4-family-bsa-write-new-support/07-03-SUMMARY.md`
- Found task commits: `3bcefe9`, `f857f1f`
- Key modified files exist: `src/formats/bsa/tes4_bsa_writer.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`

---
*Phase: 07-tes4-family-bsa-write-new-support*
*Completed: 2026-05-09*
