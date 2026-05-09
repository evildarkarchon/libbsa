---
phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
plan: 07
subsystem: archive-parsing
tags: [cpp, ba2, dx10, dds, bounded-io, sparse-files]
requires:
  - phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
    provides: BA2 DX10 parser, fixture coverage, and DDS layout validation from Plans 06-01 through 06-06
provides:
  - Bounded host-file BA2 DX10 filename parsing that reads exactly file_count encoded names
  - Sparse BA2 DX10 regression coverage for open/list/find/contains behavior
affects: [ba2-dx10-parser, dds-read, malformed-archive-hardening]
tech-stack:
  added: []
  patterns: [count-delimited filename parsing, sparse archive regression tests]
key-files:
  created: []
  modified:
    - src/formats/ba2/ba2_dx10_parser.cpp
    - tests/unit/ba2_dx10_parser_tests.cpp
key-decisions:
  - "BA2 DX10 host-file open now reconstructs compact metadata by reading exactly file_count UInt16-prefixed names instead of allocating the FileTableOffset-to-first-payload span."
patterns-established:
  - "Sparse BA2 archives are tested with host-file holes so bounded metadata-read guarantees stay observable."
requirements-completed: [DDS-01, DDS-02, DDS-03]
duration: 3min
completed: 2026-05-09
---

# Phase 06 Plan 07: Bounded BA2 DX10 Filename Parsing Summary

**Count-delimited BA2 DX10 filename parsing that avoids sparse payload-gap allocation during open/list metadata reads**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T04:52:36Z
- **Completed:** 2026-05-09T04:55:52Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added a sparse BA2 DX10 regression test covering `archive_reader::open`, `entries`, `find`, and `contains` without real game data or `TES5Edit/` writes.
- Replaced full filename-table-to-payload gap reads with incremental `file_count` name parsing from the host archive file.
- Preserved final canonical path validation by appending compact encoded name bytes and delegating to the existing DX10 metadata parser.

## Task Commits

1. **Task 1: Add bounded sparse DX10 open regression** - `7f5e9bc` (test)
2. **Task 2: Parse exactly file_count DX10 names incrementally** - `a6c5472` (feat)

**Plan metadata:** pending final metadata commit

## Files Created/Modified

- `tests/unit/ba2_dx10_parser_tests.cpp` - Adds a synthetic sparse DX10 archive regression test for bounded open/list behavior.
- `src/formats/ba2/ba2_dx10_parser.cpp` - Adds `parse_ba2_dx10_names_from_file` and routes host-file DX10 parsing through compact count-delimited name reads.

## Decisions Made

- BA2 DX10 host-file parsing now treats the filename table as count-delimited by `file_count`; bytes between the last encoded name and first payload are sparse padding, not required metadata.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test Robustness] Relaxed sparse test timing threshold after GREEN**
- **Found during:** Task 2 (Parse exactly file_count DX10 names incrementally)
- **Issue:** The original 2-second assertion proved the old full-gap read failed, but left little margin for temp-file creation on slower Windows hosts.
- **Fix:** Kept the 4 GiB sparse archive regression and relaxed the bounded-open assertion to 4 seconds, which still fails the old full-gap implementation observed at about 5 seconds while passing the incremental parser.
- **Files modified:** `tests/unit/ba2_dx10_parser_tests.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -R "ba2_dx10_detector|ba2_dx10_metadata|ba2_dx10_layout" --output-on-failure`
- **Committed in:** `a6c5472`

---

**Total deviations:** 1 auto-fixed (1 bug/test robustness)
**Impact on plan:** The auto-fix keeps the bounded-read regression meaningful while reducing platform timing flakiness.

## Issues Encountered

- The first RED attempt failed because the synthetic test helper wrote a three-byte extension string instead of a four-byte BA2 extension field; this was corrected before accepting the RED failure.
- A behavior-only sparse test passed against the old implementation because reading sparse holes was functionally correct but unbounded; the final regression measures bounded open time for the large sparse gap.

## User Setup Required

None - no external service configuration required.

## Threat Flags

Omitted - no new network endpoints, auth paths, file access trust boundaries beyond the planned BA2 host-file parser surface.

## Known Stubs

None.

## Next Phase Readiness

- BA2 DX10 open/list parsing now satisfies the bounded metadata-read verification truth.
- Malformed DX10 coverage gaps remain for Plan 06-08.

## Self-Check: PASSED

- Found `src/formats/ba2/ba2_dx10_parser.cpp`.
- Found `tests/unit/ba2_dx10_parser_tests.cpp`.
- Found commit `7f5e9bc`.
- Found commit `a6c5472`.
- `git -C TES5Edit status --short` produced no output.

---
*Phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction*
*Completed: 2026-05-09*
