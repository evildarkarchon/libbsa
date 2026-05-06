---
phase: 06-ba2-gnrl-read-and-extract
plan: 07
subsystem: parser-validation
tags: [cpp20, ba2, gnrl, malformed-input, tdd, ctest]

requires:
  - phase: 06-ba2-gnrl-read-and-extract
    provides: BA2 GNRL parser/extractor, malformed-input coverage, and verification gap report from plans 06-01 through 06-06
provides:
  - Parser-side rejection for duplicate BA2 names that normalize to the same archive path
  - Parser-side range validation for zero-entry BA2 FileTableOffset metadata
  - Generated regression tests closing D-16, D-17, D-18, D-19, and BA2-04 verification gaps
affects: [phase-07-ba2-dds, phase-10-ba2-writer, phase-11-compat-validation, malformed-input-validation]

tech-stack:
  added: []
  patterns:
    - BA2 format validation happens before archive_view construction when malformed metadata could be collapsed by generic lookup storage
    - Empty archive metadata is range-validated independently of whether file records or names are present

key-files:
  created:
    - .planning/phases/06-ba2-gnrl-read-and-extract/06-07-SUMMARY.md
  modified:
    - tests/ba2_reader_tests.cpp
    - src/ba2_reader.cpp

key-decisions:
  - "Kept duplicate normalized BA2 name rejection in parse_ba2_gnrl because duplicate name-table associations are BA2 malformed-input validation, not a generic archive_view policy."
  - "Validated FileTableOffset against byte_source::size even for zero-entry BA2 archives so empty archives remain allowed but impossible metadata is rejected."

patterns-established:
  - "Verification-gap closure tests use existing generated BA2 fixture helpers rather than binary fixtures or TES5Edit mutation."
  - "Parser-local duplicate detection guards archive_view::insert_or_assign from hiding malformed BA2 name-table associations."

requirements-completed: [BA2-04]

duration: 12min
completed: 2026-05-06
---

# Phase 06 Plan 07: BA2 GNRL Verification Gap Closure Summary

**BA2 malformed name-table hardening with duplicate normalized name rejection and zero-entry FileTableOffset bounds validation**

## Performance

- **Duration:** 12 min
- **Started:** 2026-05-06T03:05:48Z
- **Completed:** 2026-05-06T03:17:48Z
- **Tasks:** 2/2
- **Files modified:** 3

## Accomplishments

- Added three generated-fixture regression tests for duplicate normalized BA2 names, impossible zero-entry `FileTableOffset`, and valid empty BA2 archives.
- Hardened `parse_ba2_gnrl` to validate `file_table_offset > source.size()` independently of `file_count`.
- Added parser-local duplicate normalized path detection before constructing `ba2_archive`, preventing `archive_view::insert_or_assign` from silently hiding malformed name associations.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add verification-gap BA2 malformed tests** - `4f733b4` (test)
2. **Task 2 GREEN/REFACTOR: Reject duplicate BA2 names and impossible empty offsets** - `cdf8db1` (fix)

**Plan metadata:** committed separately in the final docs commit.

## Files Created/Modified

- `tests/ba2_reader_tests.cpp` - Adds duplicate normalized BA2 name rejection and empty-archive FileTableOffset regression tests.
- `src/ba2_reader.cpp` - Adds independent FileTableOffset range validation and parser-local duplicate normalized path rejection with a compatibility comment.
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-07-SUMMARY.md` - Records execution, validation evidence, decisions, and self-check results.

## Decisions Made

- Kept duplicate normalized BA2 name rejection in `parse_ba2_gnrl` because duplicate name-table associations are BA2 malformed-input validation, not a generic `archive_view` policy.
- Validated `FileTableOffset` against `byte_source::size` even when `file_count == 0`, preserving valid empty archives while rejecting impossible metadata.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes.

## Issues Encountered

- The initial RED `ctest` run without rebuilding still reported the prior 26-test executable as passing; rebuilding `libbsa_ba2_reader_tests` exposed the intended failing RED tests. This was handled within the planned RED verification flow.

## Validation Evidence

- `rg -n "duplicate normalized BA2 names|empty BA2 archives with impossible FileTableOffset|accepts empty BA2 archives with in-range FileTableOffset" tests/ba2_reader_tests.cpp` - passed; all three test names were present.
- `rg -n "Meshes/Armor/Iron\.NIF|meshes\\\\armor\\\\iron\.nif|write_u64\(bytes, 16U|paths\(\)\.empty" tests/ba2_reader_tests.cpp` - passed; all required fixture patterns were present.
- `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_ba2_reader_tests && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` during RED - produced the expected RED result with 2/29 non-passing cases: duplicate normalized names and impossible empty FileTableOffset were still accepted.
- `rg -n "#include <algorithm>|duplicate BA2 name|insert_or_assign|normalized_paths|file_table_offset > source\.size" src/ba2_reader.cpp` - passed; all parser hardening patterns were present.
- `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_ba2_reader_tests && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` during GREEN - passed, 29/29 tests.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` - passed, 89/89 tests.
- `git status --short TES5Edit` - passed with no output.
- `rg -n '=\[\]|=\{\}|=null|=""|not available|coming soon|placeholder|TODO|FIXME|mock' tests/ba2_reader_tests.cpp src/ba2_reader.cpp` - passed with no output.

## Known Stubs

None.

## Threat Flags

None. The plan hardened existing untrusted BA2 parser trust boundaries already captured in the plan threat model and introduced no new network endpoints, auth paths, file access patterns beyond existing byte-source parsing, or schema changes.

## TDD Gate Compliance

- RED gate commit present: `4f733b4` (`test(06-07): add failing tests for BA2 verification gaps`).
- GREEN gate commit present after RED: `cdf8db1` (`fix(06-07): close BA2 verification gaps`).
- No separate refactor commit was needed; the GREEN change was minimal and already passed focused and full CTest.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 6 BA2 GNRL verification gaps D-16 through D-19 are closed for duplicate normalized names and zero-entry impossible `FileTableOffset` metadata. Phase 7 BA2 DDS work can rely on BA2 GNRL parser validation being complete for BA2-04 name-table association semantics.

## Self-Check: PASSED

- Found `tests/ba2_reader_tests.cpp`, `src/ba2_reader.cpp`, and `.planning/phases/06-ba2-gnrl-read-and-extract/06-07-SUMMARY.md`.
- Found task commits `4f733b4` and `cdf8db1` in git history.
- Confirmed `TES5Edit/` status produced no output.

---
*Phase: 06-ba2-gnrl-read-and-extract*
*Completed: 2026-05-06*
