---
phase: 08-ba2-gnrl-write-new-support
plan: 02
subsystem: parser
tags: [cpp20, ba2, gnrl, reader, tdd, ctest]

requires:
  - phase: 05-ba2-gnrl-read-extract
    provides: BA2 GNRL reader, lookup, and extraction semantics
  - phase: 08-ba2-gnrl-write-new-support
    provides: public BA2 GNRL writer contract from Plan 08-01
provides:
  - BA2 GNRL reader support for payload-before-filename-table archives
  - Regression coverage for end-of-archive filename tables required by writer output
affects: [08-ba2-gnrl-write-new-support, ba2-gnrl-reader, ba2-gnrl-writer]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN parser regression
    - Bounded host-file filename parsing from FileTableOffset
    - Reader-backed BA2 writer prerequisite validation

key-files:
  created:
    - .planning/phases/08-ba2-gnrl-write-new-support/08-02-SUMMARY.md
  modified:
    - tests/unit/ba2_gnrl_reader_tests.cpp
    - src/formats/ba2/ba2_gnrl_parser.cpp

key-decisions:
  - "BA2 GNRL host-file open now parses exactly file_count length-prefixed names from FileTableOffset instead of deriving filename-table size from the first payload offset."
  - "Filename-table safety validation now rejects actual table/payload span intersections, allowing valid table-before-payload and payload-before-table layouts."

patterns-established:
  - "End-table regression: synthetic BA2 bytes exercise public archive_reader reopen, metadata, lookup, and extraction behavior."
  - "Bounded host-file parse: open/list reads header, records, and exact filename bytes without allocating intervening payload bytes."

requirements-completed: [WBA2-03]

duration: 3 min
completed: 2026-05-09
---

# Phase 08 Plan 02: End Filename-Table Reader Support Summary

**BA2 GNRL reader now accepts writer-required payload-before-filename-table archives while preserving bounded open/list parsing.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T07:58:19Z
- **Completed:** 2026-05-09T08:01:33Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added a RED Catch2 regression for a synthetic BA2 GNRL archive whose `FileTableOffset` points to a final length-prefixed filename table after raw payload bytes.
- Updated host-file BA2 GNRL parsing to read exactly `file_count` names from `FileTableOffset`, avoiding the previous first-payload-offset table-size assumption.
- Preserved BA2 path semantics and bounded parsing by validating name-table and payload spans without reading payload bytes as metadata.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add end filename-table reader regression** - `5edf804` (test)
2. **Task 2 GREEN: Parse exactly file_count names from FileTableOffset** - `afdb8d3` (feat)

## Files Created/Modified

- `tests/unit/ba2_gnrl_reader_tests.cpp` - Adds `[ba2_gnrl_end_table]` regression with `0xBAADF00D`, final `FileTableOffset`, mixed-case lookup, public metadata, and extraction assertions.
- `src/formats/ba2/ba2_gnrl_parser.cpp` - Adds exact host-file filename parsing from `FileTableOffset` and table/payload intersection validation.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-02-SUMMARY.md` - Documents plan execution, verification, and state handoff.

## Decisions Made

- BA2 GNRL host-file parsing should not infer filename-table length from the first payload offset; it should parse the declared number of length-prefixed names and stop after consumed bytes.
- Payload/name-table safety is an overlap check, not an ordering rule. This keeps existing table-before-payload fixtures valid while enabling Phase 8 writer output with names at the end.

## TDD Gate Compliance

- **RED:** `5edf804` added the failing `[ba2_gnrl_end_table]` regression. Verification failed before parser changes at `REQUIRE(opened.has_value())`.
- **GREEN:** `afdb8d3` implemented bounded end-table parsing. The regression and existing BA2 GNRL tests pass.
- **REFACTOR:** No separate refactor commit was needed.

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R ba2_gnrl_end_table --output-on-failure` — RED failed as expected before parser changes.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "ba2_gnrl_end_table|ba2_gnrl_reader" --output-on-failure` — passed after parser changes.
- `ctest --preset windows-msvc-debug-static -R ba2_gnrl --output-on-failure` — 11/11 BA2 GNRL tests passed.
- `git -C TES5Edit status --short` — no output; read-only submodule remained clean.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Next Phase Readiness

Plan 08-03 can build BA2 GNRL writer-owned state knowing future writer output can be reopened once it serializes payloads before the final filename table.

## Self-Check: PASSED

- FOUND: `src/formats/ba2/ba2_gnrl_parser.cpp`
- FOUND: `tests/unit/ba2_gnrl_reader_tests.cpp`
- FOUND: `5edf804`
- FOUND: `afdb8d3`

---
*Phase: 08-ba2-gnrl-write-new-support*
*Completed: 2026-05-09*
