---
phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
plan: 06
subsystem: testing
tags: [cpp, catch2, ctest, ba2, dx10, dds, malformed-fixtures]

# Dependency graph
requires:
  - phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
    provides: BA2 DX10 fixtures, parser/routing, DDS extraction, DirectXTex validation boundary
provides:
  - Manifest-driven malformed BA2 DX10 regression coverage
  - Final static/shared Phase 6 acceptance gate results
  - Exact-size DX10 decoded-size mismatch fixture coverage
affects: [phase-06-closeout, phase-09-ba2-dx10-write-new-support, phase-11-hardening]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Manifest-driven malformed archive tests assert stable error_code values only
    - Malformed compressed payload fixtures keep layout metadata valid so extraction-time exact-size checks are exercised

key-files:
  created:
    - .planning/phases/06-dds-boundary-and-ba2-dx10-read-reconstruction/06-06-SUMMARY.md
  modified:
    - tests/unit/ba2_dx10_malformed_tests.cpp
    - tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp
    - tests/fixtures/generated/archives/ba2_dx10_decoded_size_mismatch.ba2

key-decisions:
  - "Malformed BA2 DX10 tests assert libbsa::error_code values from the manifest and do not assert diagnostic text."
  - "Decoded-size mismatch fixture metadata remains layout-valid so exact-size decompression, not open-time layout validation, rejects the archive."

patterns-established:
  - "DX10 malformed coverage mirrors BA2 GNRL manifest-driven open/extraction phases."
  - "Fixture generators must keep malformed-case failure phase aligned with the manifest."

requirements-completed: [DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07]

# Metrics
duration: 6 min
completed: 2026-05-09
---

# Phase 06 Plan 06: Malformed DX10 Hardening and Final Regression Summary

**Manifest-driven BA2 DX10 malformed coverage with final static/shared regression gates for DDS read and reconstruction.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-05-09T02:15:56Z
- **Completed:** 2026-05-09T02:22:00Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Added manifest-driven `ba2_dx10_malformed` tests covering open-time metadata failures and extraction-time corrupt/decoded-size failures.
- Adjusted the decoded-size mismatch fixture so it opens successfully and fails during exact-size decompression as intended.
- Verified focused DX10/public-boundary tests plus full static and shared CTest suites; TES5Edit remained clean.

## Task Commits

Each task was committed atomically when it changed files:

1. **Task 1: Add manifest-driven malformed DX10 regression tests** - `8170b00` (test)
2. **Task 2: Harden parser/extractor until malformed and regression suites pass** - `9777320` (fix)
3. **Task 3: Run final Phase 6 regression gates** - no code commit; verification-only task produced no file changes.

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/ba2_dx10_malformed_tests.cpp` - Loads `ba2_dx10_malformed_manifest.json` and asserts stable `error_code` values for open/extraction failures.
- `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` - Keeps decoded-size mismatch metadata layout-valid while compressed bytes decode short.
- `tests/fixtures/generated/archives/ba2_dx10_decoded_size_mismatch.ba2` - Regenerated malformed fixture matching the manifest's extraction-phase failure.
- `.planning/phases/06-dds-boundary-and-ba2-dx10-read-reconstruction/06-06-SUMMARY.md` - Execution summary and final gate record.

## Verification Results

- `ctest --preset windows-msvc-debug-static -R ba2_dx10_malformed --output-on-failure` — PASS after fixture alignment.
- `ctest --preset windows-msvc-debug-static -R "ba2_dx10|public_include_boundary" --output-on-failure` — PASS, 14/14 tests.
- `ctest --preset windows-msvc-debug-static --output-on-failure` — PASS, 92/92 tests passed with the local game fixture test skipped by policy.
- `ctest --preset windows-msvc-debug-shared --output-on-failure` — PASS after rebuilding the shared preset, 92/92 tests passed with the local game fixture test skipped by policy.
- `git -C TES5Edit status --short` — PASS, no output.

## Decisions Made

- Malformed BA2 DX10 tests assert only stable `libbsa::error_code` values from the fixture manifest, preserving D-33 and avoiding diagnostic text coupling.
- The decoded-size mismatch malformed fixture now preserves layout-valid metadata and stores a compressed payload that decodes shorter than declared, so extraction exercises `decompress_payload_exact`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Aligned decoded-size mismatch fixture with extraction-phase manifest**
- **Found during:** Task 2 (Harden parser/extractor until malformed and regression suites pass)
- **Issue:** The RED malformed test showed `ba2_dx10_decoded_size_mismatch` failed during `archive_reader::open` because the fixture declared metadata that layout validation could reject before extraction.
- **Fix:** Regenerated that malformed fixture with layout-valid metadata and a compressed payload that decodes shorter than the declared raw size.
- **Files modified:** `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, `tests/fixtures/generated/archives/ba2_dx10_decoded_size_mismatch.ba2`
- **Verification:** `ctest --preset windows-msvc-debug-static -R ba2_dx10_malformed --output-on-failure` and focused DX10/public-boundary suite passed.
- **Committed in:** `9777320`

---

**Total deviations:** 1 auto-fixed (1 Rule 1 bug)
**Impact on plan:** The fix kept the planned malformed matrix intact and aligned fixture behavior with the manifest's extraction/open distinction. No scope creep.

## Issues Encountered

- Initial shared-preset CTest run failed because the shared build artifacts were stale after source changes; rebuilding `windows-msvc-debug-shared` and rerunning the preset passed.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Next Phase Readiness

Phase 6 DDS read/reconstruction acceptance is green across static and shared presets. Phase 7 can proceed with TES4-family write-new support; future Phase 9 DX10 writer work should preserve the generator/manifest alignment pattern established here.

## Self-Check: PASSED

- `FOUND: summary`
- `FOUND: 8170b00`
- `FOUND: 9777320`

---
*Phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction*
*Completed: 2026-05-09*
