---
phase: 10-ba2-writers
plan: 02
subsystem: writer
tags: [cpp, ba2, gnrl, tdd, roundtrip]

requires:
  - phase: 10-ba2-writers
    provides: BA2 writer API seam and focused test target from plan 10-01
provides:
  - Native BA2 GNRL memory planning for Fallout 4 v1/v7/v8 and Starfield v2/v3
  - Plan-owned BTDX/GNRL tables, name tables, payload regions, and finalization bytes
  - Read-after-write GNRL coverage through open_ba2 and extract_ba2_entry
affects: [ba2-writers, ba2-gnrl, writer-finalization, phase-10]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN commits for native BA2 writer behavior
    - Plan/finalize writer API with plan-owned table and payload bytes
    - BA2 GNRL raw semantics using native PackedSize zero with preview stored sizes

key-files:
  created:
    - .planning/phases/10-ba2-writers/10-02-SUMMARY.md
  modified:
    - tests/ba2_writer_tests.cpp
    - src/ba2_writer.cpp

key-decisions:
  - "Native BA2 GNRL planning sorts normalized archive paths before layout so generated records and name tables are deterministic."
  - "Raw BA2 GNRL records write native PackedSize as zero while data-region previews retain the actual stored byte count."
  - "BA2 finalization streams frozen plan table bytes followed by plan-owned payload regions without recomputing layout."

patterns-established:
  - "GNRL writer tests prove generated archives by reopening through open_ba2 and extracting through extract_ba2_entry."
  - "Starfield v3 GNRL planning exposes CompressionMethod in native target preview and rejects unsupported values."

requirements-completed: [WRT-02]

duration: 3min
completed: 2026-05-07
---

# Phase 10 Plan 02: Native BA2 GNRL Memory Layout Summary

**Native BTDX/GNRL memory writer planning for Fallout 4 v1/v7/v8 and Starfield v2/v3 with read-after-write extraction proof**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-07T10:47:42Z
- **Completed:** 2026-05-07T10:50:37Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added RED tests for required BA2 GNRL targets that assert BTDX/GNRL headers, version-specific header sizes, FileTableOffset, native records, preview fields, and extraction round trips.
- Implemented memory-backed GNRL planning with normalized path sorting, duplicate rejection, FO4-style path-part hashes, checked layout arithmetic, name-table emission, data-region preview, and raw PackedSize semantics.
- Implemented BA2 finalization over plan-owned table bytes and data regions, plus focused regression coverage with existing BA2 reader tests.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing GNRL native layout and read-after-write tests** - `0c15c45` (test)
2. **Task 2 GREEN: Implement native GNRL memory planning and finalization** - `2b4f601` (feat)

**Plan metadata:** pending final docs commit

_Note: This TDD plan produced the required RED and GREEN commits._

## Files Created/Modified

- `tests/ba2_writer_tests.cpp` - Adds GNRL layout, preview, and read-after-write tests for all required versions; updates finalization smoke behavior.
- `src/ba2_writer.cpp` - Implements native BA2 GNRL memory planning, disk ingestion bridge, checked layout serialization, raw/compressed payload planning, and finalization streaming.
- `.planning/phases/10-ba2-writers/10-02-SUMMARY.md` - Records plan execution, commits, verification, and follow-up context.

## Decisions Made

- Native BA2 GNRL planning sorts normalized archive paths before layout so generated records and name tables are deterministic.
- Raw BA2 GNRL records write native PackedSize as zero while data-region previews retain the actual stored byte count.
- BA2 finalization streams frozen plan table bytes followed by plan-owned payload regions without recomputing layout.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Replaced obsolete finalization placeholder test after implementing shared finalization**
- **Found during:** Task 2 (GREEN implementation)
- **Issue:** The existing finalization placeholder test still expected `finalize_ba2_write` to fail even though GNRL success requires real finalization.
- **Fix:** Updated the test to assert an empty plan finalizes successfully without writing payload bytes.
- **Files modified:** `tests/ba2_writer_tests.cpp`
- **Verification:** `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` passed.
- **Committed in:** `2b4f601`

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** The adjustment was required for the planned finalizer behavior and did not expand scope beyond GNRL writer correctness.

## Issues Encountered

- The RED tests initially failed as intended because GNRL writer planning still returned the unsupported placeholder.
- After GREEN implementation, the obsolete finalization placeholder test failed and was updated as a correctness requirement.

## User Setup Required

None - no external service configuration required.

## Known Stubs

| File | Line | Reason |
|------|------|--------|
| `src/ba2_writer.cpp` | DDS writer placeholder functions | BA2 DDS/DX10 writing is intentionally deferred to later Phase 10 plans. |
| `tests/ba2_writer_tests.cpp` | DDS placeholder test | Keeps the deferred DDS API behavior explicit until the DDS writer TDD plans replace it. |

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: file_access | `src/ba2_writer.cpp` | `plan_ba2_gnrl_write_from_disk` now reads caller-provided host files into plan-owned memory; later disk-input failure tests should harden this path further. |

## Verification

- `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests` - passed.
- `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` - passed.
- `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests"` - passed (34/34).
- Acceptance grep for native GNRL implementation terms and removal of active GNRL placeholder references - passed.

## TDD Gate Compliance

- RED commit present: `0c15c45` (`test(10-02): add failing GNRL writer tests`)
- GREEN commit present after RED: `2b4f601` (`feat(10-02): implement BA2 GNRL writer planning`)
- REFACTOR commit: not needed

## Next Phase Readiness

- Ready for 10-03 to add GNRL disk-input coverage, compression routes, deduplication, and structured failure tests on top of the native GNRL memory serializer.
- DDS/DX10 writer placeholders remain intentionally deferred for plans 10-04 and 10-05.

## Self-Check: PASSED

- Found summary and key modified files on disk.
- Verified task commits `0c15c45` and `2b4f601` exist in git history.

---
*Phase: 10-ba2-writers*
*Completed: 2026-05-07*
