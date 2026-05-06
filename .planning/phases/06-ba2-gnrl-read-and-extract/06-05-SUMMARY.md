---
phase: 06-ba2-gnrl-read-and-extract
plan: 05
subsystem: ba2-parser-hardening
tags: [cpp20, ba2, gnrl, malformed-input, tdd, security]

requires:
  - phase: 06-ba2-gnrl-read-and-extract
    provides: BA2 public API, GNRL metadata parser, extraction path, and Starfield v3 codec routing from plans 06-01 through 06-04
provides:
  - Generated malformed BA2 GNRL fixtures for truncation, name-count mismatch, traversal, and payload range failures
  - Parser hardening for exact name-table consumption before first payload offset
  - Extraction hardening proof for impossible and overflowing BA2 payload ranges with empty sinks
affects: [phase-06-verification, phase-07-ba2-dds, phase-10-ba2-writer, parser-security]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN gate commits for malformed archive behavior
    - BA2 name-table parsing returns both names and consumed byte range for exact-count validation
    - Payload reads remain bounded by checked-add range validation before sink writes

key-files:
  created:
    - .planning/phases/06-ba2-gnrl-read-and-extract/06-05-SUMMARY.md
  modified:
    - tests/ba2_reader_tests.cpp
    - src/ba2_reader.cpp

key-decisions:
  - "Treat extra bytes between the parsed BA2 name table and the first payload offset as a malformed name-count mismatch rather than padding."
  - "Kept malformed BA2 coverage generated in tests/ba2_reader_tests.cpp with no external BA2 blobs or BSArchPro execution."

patterns-established:
  - "BA2 malformed parser tests assert structured error_code::malformed_archive failures for D-16 safety cases."
  - "BA2 extraction failure tests assert sink.bytes().empty() so partial writes remain prohibited."

requirements-completed: [BA2-01, BA2-02, BA2-03, BA2-04]

duration: 5min
completed: 2026-05-06
---

# Phase 06 Plan 05: BA2 GNRL Malformed-Input Safety Summary

**BA2 GNRL malformed-input matrix with structured parser failures, exact name-table validation, and no partial extraction writes**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-06T02:38:00Z
- **Completed:** 2026-05-06T02:43:24Z
- **Tasks:** 2/2
- **Files modified:** 3

## Accomplishments

- Added seven malformed BA2 tests covering truncated headers, truncated records, truncated name tables, mismatched file/name counts, traversal names, impossible payload ranges, and offset overflow without sink writes.
- Hardened `open_ba2` so the parsed name table must end exactly at the first payload offset, causing generated name-count mismatch fixtures to fail with `malformed_archive`.
- Preserved existing bounded payload extraction behavior and verified extraction failures return before `memory_sink` receives bytes.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing malformed BA2 safety tests** - `59ed134` (test)
2. **Task 2 GREEN/REFACTOR: Enforce BA2 malformed-input checks** - `fd8a776` (feat)

**Plan metadata:** committed separately in the final docs commit.

## Files Created/Modified

- `tests/ba2_reader_tests.cpp` - Adds malformed fixture tests and helper support for offset rewrites and direct extraction range-failure archives.
- `src/ba2_reader.cpp` - Tracks consumed BA2 name-table bytes and rejects mismatch gaps before first payload offset.
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-05-SUMMARY.md` - Documents execution, verification, and TDD compliance for this plan.

## Decisions Made

- Treated name-table bytes as exact rather than allowing padding before the first payload because BA2 names are associated by record index; accepting extra bytes can hide file/name count mismatch and unsafe path association.
- Kept all malformed fixtures generated in source-reviewable Catch2 helpers, preserving D-13 and avoiding committed binary archives.

## TDD Gate Compliance

- RED gate: `59ed134 test(06-05): add failing test for BA2 malformed input handling` — BA2 tests failed as expected on `open_ba2 rejects mismatched BA2 file name counts` because the parser accepted extra name-table bytes before implementation.
- GREEN gate: `fd8a776 feat(06-05): harden BA2 malformed input handling` — BA2 tests passed after adding exact name-table consumption validation.
- REFACTOR gate: not needed; no behavior-neutral cleanup commit was made.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes.

## Issues Encountered

None.

## Verification

- RED run: `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` - failed as expected on the new mismatched name-count test.
- GREEN/final run: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` - passed, 26/26 tests.
- Acceptance grep for all seven malformed test names in `tests/ba2_reader_tests.cpp` - passed.
- Acceptance grep for `error_code::malformed_archive` and `sink.bytes().empty()` in `tests/ba2_reader_tests.cpp` - passed.
- Acceptance grep for `truncated BA2 table`, `invalid BA2 name`, `BA2 payload range exceeds source size`, `normalize_archive_path`, and `checked_add` in `src/ba2_reader.cpp` - passed.
- `git status --short TES5Edit` - passed with empty output.
- TDD gate log checks for `test(06-05)` and `feat(06-05)` - passed.

## Known Stubs

None.

## Threat Flags

None. The plan threat model already covered untrusted BA2 table reads, untrusted names, and parsed payload ranges.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 06-06 / phase-level verification with D-16 core malformed coverage in place and no changes to `TES5Edit/`.

## Self-Check: PASSED

- Found `tests/ba2_reader_tests.cpp`, `src/ba2_reader.cpp`, and `.planning/phases/06-ba2-gnrl-read-and-extract/06-05-SUMMARY.md`.
- Found task commits `59ed134` and `fd8a776` in git history.
- Confirmed `.planning/STATE.md` was not modified and `.planning/ROADMAP.md` remains the pre-existing dirty orchestrator artifact, excluded from this executor's commits.

---
*Phase: 06-ba2-gnrl-read-and-extract*
*Completed: 2026-05-06*
