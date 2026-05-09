---
phase: 08-ba2-gnrl-write-new-support
plan: 04
subsystem: writer
tags: [cpp20, ba2, gnrl, writer, raw, roundtrip, tdd]

requires:
  - phase: 08-ba2-gnrl-write-new-support
    provides: public BA2 GNRL writer state validation and end filename-table reader support
provides:
  - Raw Fallout 4 v1, Starfield v2, and Starfield v3 BA2 GNRL serialization
  - Reader-backed round-trip tests for disk, memory, zero-byte, Starfield metadata, and record flags
  - End-of-archive UInt16-prefixed filename table layout with payloads before names
affects: [phase-08-compression, phase-08-deduplication, ba2-gnrl-writer]

tech-stack:
  added: []
  patterns: [detail::binary_writer BA2 serialization, canonical-path record ordering, reader-backed writer validation]

key-files:
  created:
    - .planning/phases/08-ba2-gnrl-write-new-support/08-04-SUMMARY.md
  modified:
    - src/formats/ba2/ba2_gnrl_writer.cpp
    - tests/unit/ba2_gnrl_writer_tests.cpp

key-decisions:
  - "Raw BA2 GNRL writer output is validated through archive_reader reopen/list/find/contains/extract APIs rather than writer internals."
  - "Phase 8 Plan 04 keeps compressed BA2 GNRL entries unsupported until Plan 08-05, while raw entries serialize fully with target-profile metadata."

patterns-established:
  - "BA2 GNRL records and filename-table names are serialized in deterministic canonical-path order while preserving original spelling by index."
  - "Zero-byte BA2 GNRL entries use a safe zero-length payload span anchored at the first payload byte so end filename tables remain after real payload data."

requirements-completed: [WBA2-01, WBA2-02, WBA2-03, WBA2-05]

duration: 15min
completed: 2026-05-09
---

# Phase 08 Plan 04: Raw BA2 GNRL Serialization Summary

**Raw FO4 v1 and Starfield v2/v3 BA2 GNRL archives now serialize as reader-reopenable BTDX/GNRL files with end filename tables.**

## Performance

- **Duration:** 15 min
- **Started:** 2026-05-09T07:58:00Z
- **Completed:** 2026-05-09T08:13:43Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added RED tests proving raw BA2 GNRL writer output must reopen through `archive_reader`, preserve copied memory after caller mutation, and expose Starfield default/override metadata.
- Implemented BA2 `BTDX`/`GNRL` header, record table, raw payload, and final UInt16-prefixed filename-table serialization using checked size and offset arithmetic.
- Preserved caller-provided BA2 record flags and Starfield `Unknown1`/`Unknown2` overrides through public reopened metadata.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add raw target-profile round-trip and layout tests** - `cbb8b0a` (test)
2. **Task 2 GREEN: Serialize raw BA2 GNRL archives with end filename tables** - `803004e` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/ba2_gnrl_writer_tests.cpp` - Adds raw FO4/SFv2/SFv3 round-trip, end table, copied-memory, Starfield override, and record-flag tests.
- `src/formats/ba2/ba2_gnrl_writer.cpp` - Serializes BA2 GNRL raw archives with checked header, records, payloads, hashes, sentinel, and filename table.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-04-SUMMARY.md` - Captures execution results, deviations, verification, and TDD gate compliance.

## Decisions Made

- Raw Plan 08-04 output uses deterministic canonical-path physical order because reference evidence did not lock a stricter BA2 record sort order; the filename-table entries are paired by the same index.
- Compressed BA2 GNRL entries remain a structured `unsupported` path until Plan 08-05 adds target-routed deflate/raw-LZ4 stored payloads.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Adjusted zero-byte payload offsets for strict end-table layout assertions**
- **Found during:** Task 2 (GREEN verification)
- **Issue:** Zero-byte entries sorted after non-empty entries had `payload_offset == FileTableOffset`, which is safe for a zero-length span but did not satisfy the test's strict end-table-after-payload-offset assertion.
- **Fix:** Anchored empty entries at the first payload byte while preserving zero raw/stored sizes, so no payload bytes are read and the filename table remains after all real payload data.
- **Files modified:** `src/formats/ba2/ba2_gnrl_writer.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure`
- **Committed in:** `803004e`

---

**Total deviations:** 1 auto-fixed (1 Rule 1 bug)
**Impact on plan:** The fix tightened layout compatibility for zero-byte raw entries without adding scope beyond Plan 08-04.

## Issues Encountered

- `ctest --preset windows-msvc-debug-static -R ba2_gnrl_reader --output-on-failure` matched no tests in the current registry; the reader regression was verified with the existing `ba2_gnrl_reader` label instead.

## User Setup Required

None - no external service configuration required.

## Known Stubs

- `src/formats/ba2/ba2_gnrl_writer.cpp` - Compressed entries currently return `unsupported` with the message that compression is implemented in the compression plan. This is intentional because Plan 08-05 owns deflate/raw-LZ4 BA2 GNRL payload writing.

## Threat Flags

None beyond the planned writer state→binary archive boundary covered by T-08-04-01 through T-08-04-03.

## TDD Gate Compliance

- RED gate: `cbb8b0a test(08-04): add failing BA2 GNRL raw writer tests`
- GREEN gate: `803004e feat(08-04): serialize raw BA2 GNRL archives`
- REFACTOR gate: not needed

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — PASS
- `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` — PASS (9/9)
- `ctest --preset windows-msvc-debug-static -L ba2_gnrl_reader --output-on-failure` — PASS (1/1)
- `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` — PASS (2/2)
- `git -C TES5Edit status --short` — PASS (empty)

## Next Phase Readiness

Ready for Plan 08-05 to layer target-routed deflate and raw-LZ4 block compression on top of the raw BA2 GNRL serializer.

## Self-Check: PASSED

- Found summary file: `.planning/phases/08-ba2-gnrl-write-new-support/08-04-SUMMARY.md`
- Found task commits: `cbb8b0a`, `803004e`
- Key modified files exist: `src/formats/ba2/ba2_gnrl_writer.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`

---
*Phase: 08-ba2-gnrl-write-new-support*
*Completed: 2026-05-09*
