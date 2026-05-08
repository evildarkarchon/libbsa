---
phase: 05-ba2-gnrl-read-extract
plan: 06
subsystem: archive-reader
tags: [cpp20, ba2, gnrl, parser, bounded-io, large-archive, tdd]

requires:
  - phase: 05-ba2-gnrl-read-extract
    provides: BA2 GNRL parser, extraction routing, malformed coverage, and Phase 5 verification gap report
provides:
  - Bounded BA2 GNRL open/list parsing that reads only metadata and filename-table bytes before extraction
  - Sparse large-payload regression coverage for public archive_reader open/list behavior
  - 64-bit BA2 payload span validation before filename-table sizing
affects: [phase-06-ba2-dx10, phase-08-ba2-writer, phase-11-hardening, phase-12-performance]

tech-stack:
  added: []
  patterns: [first-payload-offset bounded filename table, sparse fixture regression, uint64 payload span validation]

key-files:
  created:
    - .planning/phases/05-ba2-gnrl-read-extract/05-06-SUMMARY.md
  modified:
    - tests/unit/ba2_gnrl_reader_tests.cpp
    - src/formats/ba2/ba2_gnrl_parser.cpp

key-decisions:
  - "BA2 GNRL file open computes first_payload_offset from parsed records and reads only [FileTableOffset, first_payload_offset) as the filename table."
  - "Payload span validation uses unsigned 64-bit arithmetic before any filename-table allocation or parse handoff."

patterns-established:
  - "Sparse BA2 fixtures can validate large-offset open/list behavior through archive_reader without adding copyrighted fixtures."
  - "File-backed BA2 parsing centralizes final metadata materialization by appending only bounded filename-table bytes before calling parse_ba2_gnrl_archive_impl."

requirements-completed: [GNRL-01, GNRL-02, GNRL-03, GNRL-04]

duration: 6min
completed: 2026-05-08
---

# Phase 05 Plan 06: Bounded BA2 GNRL Open/List Summary

**BA2 GNRL open/list now bounds filename-table reads by first payload offset, with sparse 8 GiB-offset regression coverage through the public reader API.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-05-08T23:33:00Z
- **Completed:** 2026-05-08T23:39:10Z
- **Tasks:** 2 completed
- **Files modified:** 3

## Accomplishments

- Added `[ba2_gnrl_bounded_open]` regression coverage that builds a temporary sparse Fallout 4 BA2 GNRL archive with a payload offset at `0x0000000200000000` and validates public `archive_reader::open` plus `entries()` metadata.
- Refactored `parse_ba2_gnrl_archive_file` so file-backed open validates the detected header, parses records from bounded metadata, computes `first_payload_offset`, and reads only the filename-table interval.
- Hardened BA2 payload span checks to use unsigned 64-bit arithmetic before narrowing and preserved centralized duplicate path, sorting, metadata, and error-code behavior through `parse_ba2_gnrl_archive_impl`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add bounded-open regression coverage** - `a28a150` (test)
2. **Task 2: Bound BA2 filename-table reads before payload data** - `acc6b62` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/ba2_gnrl_reader_tests.cpp` - Adds sparse large-payload BA2 GNRL open/list regression coverage and small test-only binary-writing helpers.
- `src/formats/ba2/ba2_gnrl_parser.cpp` - Bounds filename-table reads using `first_payload_offset`, validates file parser header consistency, and checks payload spans with `std::uint64_t` arithmetic.
- `.planning/phases/05-ba2-gnrl-read-extract/05-06-SUMMARY.md` - Records execution results, deviations, verification, and self-check.

## Decisions Made

- BA2 GNRL open/list treats payload bytes as extraction-only data; file parsing reads `[0, FileTableOffset)` plus `[FileTableOffset, first_payload_offset)` and nothing past that at open time.
- Zero-sized entries remain valid in payload span checks but do not lower the filename-table upper bound; archives with only zero-sized entries use `archive_size` as the upper bound.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed synthetic BA2 record extension width**
- **Found during:** Task 2 (Bound BA2 filename-table reads before payload data)
- **Issue:** The RED test initially wrote `"BIN\0"` through an implicit `std::string_view`, which truncated at the NUL and produced a 35-byte GNRL record instead of the required 36-byte record.
- **Fix:** Passed an explicit `std::string_view{"BIN\0", 4U}` so the skipped extension field remains four bytes.
- **Files modified:** `tests/unit/ba2_gnrl_reader_tests.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L ba2_gnrl_bounded_open --output-on-failure` passed.
- **Committed in:** `acc6b62`

---

**Total deviations:** 1 auto-fixed (1 bug).
**Impact on plan:** The fix corrected the planned synthetic fixture and did not change production scope.

## Issues Encountered

- The bounded-open test failed after the production parser change until the test fixture's embedded-NUL extension field was fixed. The failure was a test fixture construction bug, not a parser design issue.

## TDD Gate Compliance

- RED gate: `a28a150` added the bounded-open sparse fixture test and verified it failed because open returned an error while attempting to read the payload-region suffix.
- GREEN gate: `acc6b62` implemented bounded filename-table reads and verified the bounded-open, focused BA2, TES3/TES4, public-boundary, and TES5Edit gates passed.
- REFACTOR gate: not needed; cleanup was limited to Doxygen comments on new test helpers before the GREEN commit.

## Known Stubs

None. Modified files contain no TODO/FIXME/placeholder stubs or hardcoded empty UI data sources.

## Threat Flags

None. The parser trust-boundary work is covered by T-05-06-01 through T-05-06-03 in the plan threat model.

## Verification

- `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L ba2_gnrl_bounded_open --output-on-failure` — failed in RED as expected before Task 2, then passed after bounded parsing and fixture correction.
- `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "ba2_gnrl_bounded_open|ba2_gnrl" --output-on-failure` — passed, 10/10 tests.
- `ctest --preset windows-msvc-debug-static -L "tes3_bsa|tes4_bsa|public_include_boundary" --output-on-failure` — passed, 30/30 tests.
- `git -C TES5Edit status --short` — no output.
- Acceptance greps for `ba2_gnrl_bounded_open`, `0x0000000200000000`, `std::filesystem::resize_file`, `archive_reader::open`, `first_payload_offset`, `filename table overlaps payload data`, and the bounded-open comment — passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 5's remaining bounded-open verification gap is closed, so BA2 GNRL read/list/query/extract support is ready to inform Phase 6 BA2 DX10/DDS work.
- Future writer and performance phases should preserve the explicit metadata/name-table/payload separation established here.

## Self-Check: PASSED

- Found summary and key modified files on disk: `05-06-SUMMARY.md`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `src/formats/ba2/ba2_gnrl_parser.cpp`.
- Found task commits `a28a150` and `acc6b62` in git history.
- Final verification commands passed and TES5Edit status was clean.

---
*Phase: 05-ba2-gnrl-read-extract*
*Completed: 2026-05-08*
