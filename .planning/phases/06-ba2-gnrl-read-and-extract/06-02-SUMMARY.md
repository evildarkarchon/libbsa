---
phase: 06-ba2-gnrl-read-and-extract
plan: 02
subsystem: ba2-parser
tags: [cpp20, ba2, gnrl, tdd, fixture-tests]

requires:
  - phase: 06-ba2-gnrl-read-and-extract
    provides: BA2 public metadata-only API and open_ba2 unsupported stub from plan 06-01
provides:
  - Generated deterministic BA2 GNRL metadata fixtures for FO4 v1/v7/v8 and Starfield v2
  - Bounded BA2 GNRL header, record, name-table, and payload-range metadata parser
  - Public entry metadata mapping for BA2 hashes, absolute offsets, size semantics, and raw/deflate state
affects: [phase-06-ba2-extraction, phase-07-ba2-dds, phase-10-ba2-writer, public-api]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN gate commits for parser behavior
    - Test-local deterministic BA2 fixture builders without committed binary archives
    - Bounded binary parser helpers scoped privately to src/ba2_reader.cpp

key-files:
  created:
    - tests/ba2_reader_tests.cpp
  modified:
    - CMakeLists.txt
    - src/ba2_reader.cpp

key-decisions:
  - "Kept BA2 fixture builders local to tests/ba2_reader_tests.cpp per D-15."
  - "Mapped BA2 NameHash and DirHash directly to entry_metadata::name_hash and entry_metadata::directory_hash with a parser-side compatibility comment."
  - "Resolved PackedSize == 0 as raw payload bytes and nonzero PackedSize as deflate metadata for this plan's supported versions."

patterns-established:
  - "BA2 GNRL records expose archive-absolute payload offsets and validated stored byte ranges in public metadata."
  - "BA2 name-table strings are normalized before archive_view construction so malformed names fail structurally instead of disappearing."

requirements-completed: [BA2-01, BA2-02, BA2-04]

duration: 3min
completed: 2026-05-06
---

# Phase 06 Plan 02: BA2 GNRL Metadata Parser Summary

**Fixture-backed BA2 GNRL metadata parser for FO4 v1/v7/v8 and Starfield v2 with normalized names and validated public entry semantics**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-06T02:27:13Z
- **Completed:** 2026-05-06T02:30:04Z
- **Tasks:** 2/2
- **Files modified:** 3

## Accomplishments

- Added `tests/ba2_reader_tests.cpp` with deterministic BA2 GNRL fixture builders and RED coverage for FO4 v1/v7/v8, Starfield v2, FileTableOffset names, `.dds` GNRL metadata, hash mapping, size semantics, compression state, and absolute offsets.
- Wired `libbsa_ba2_reader_tests` into CMake with `unit;fixture;codec` labels and Catch2 discovery.
- Replaced the `open_ba2` unsupported parser path with a bounded parser for supported BA2 GNRL metadata tables.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing BA2 metadata fixture tests** - `76db946` (test)
2. **Task 2 GREEN/REFACTOR: Implement bounded BA2 metadata parser** - `81f92e2` (feat)

**Plan metadata:** committed separately in the final docs commit.

## Files Created/Modified

- `tests/ba2_reader_tests.cpp` - Generated BA2 fixture helpers and metadata parser behavior tests.
- `CMakeLists.txt` - Adds the BA2 reader test target and fixture/codec labels.
- `src/ba2_reader.cpp` - Parses BTDX/GNRL headers, 36-byte records, length-prefixed file names, normalized paths, and BA2 metadata fields.

## Decisions Made

- Kept fixture builders test-local because no writer-phase reuse exists yet.
- Preserved existing public metadata fields rather than adding BA2 reserved/unknown header words.
- Validated payload ranges during metadata parsing so `stored_size` is a trustworthy on-disk byte range for later extraction work.

## TDD Gate Compliance

- RED gate: `76db946 test(06-02): add failing test for BA2 GNRL metadata parsing` — BA2 tests failed because `open_ba2` returned the Plan 01 unsupported parser result.
- GREEN gate: `81f92e2 feat(06-02): implement BA2 GNRL metadata parsing` — BA2 tests passed after implementing bounded parsing.
- REFACTOR gate: not needed; no behavior-neutral cleanup commit was made.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes.

## Issues Encountered

None.

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug` - passed.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` - passed.
- RED run of the same CTest filter failed before implementation with `opened.has_value() == false` from the `open_ba2` unsupported stub - passed for RED gate.
- Acceptance greps for test names, CMake target labels, parser error strings, record/name table fields, hash fields, and raw/deflate compression states - passed.
- `git log --oneline --grep="^test(06-02)" -1` and `git log --oneline --grep="^feat(06-02)" -1` - passed.
- `git status --short TES5Edit` - passed with empty output.

## Known Stubs

None blocking this plan. The pre-existing `extract_ba2_entry` unsupported path remains for later Phase 6 extraction plans and was not part of this metadata parser plan.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for the next Phase 6 extraction plan to consume validated BA2 metadata offsets, sizes, and compression state.

## Self-Check: PASSED

- Found `tests/ba2_reader_tests.cpp`, `CMakeLists.txt`, and `src/ba2_reader.cpp`.
- Found task commits `76db946` and `81f92e2` in git history.
- Confirmed `.planning/STATE.md` was not modified and `.planning/ROADMAP.md` remains only pre-existing dirty orchestrator state.

---
*Phase: 06-ba2-gnrl-read-and-extract*
*Completed: 2026-05-06*
