---
phase: 13-host-path-correctness-boundary
plan: 05
subsystem: testing
tags: [catch2, ctest, host_path_correctness_boundary, non_ascii_paths, regression]
requires:
  - phase: 13-host-path-correctness-boundary
    provides: stored host_file_path reader reopen and validation boundary from Plan 04
provides:
  - dedicated cross-family non-ASCII host-path regression suite for TES4 and representative BA2 archives
  - focused host_path_correctness_boundary and host_path_correctness_boundary_smoke CTest coverage
  - manifest-backed canonical extraction checks from copied non-ASCII archive paths
affects: [phase-14-verification-lanes, host-path-boundary, regression-coverage]
tech-stack:
  added: []
  patterns: [manifest-backed canonical extraction proof, explicit UTF-8 public-path test input, phase-scoped source-policy smoke gate]
key-files:
  created: [tests/unit/host_path_correctness_boundary_tests.cpp, .planning/phases/13-host-path-correctness-boundary/13-05-SUMMARY.md]
  modified: [tests/CMakeLists.txt]
key-decisions:
  - "The suite constructs non-ASCII temp paths with filesystem-native components, then converts back to explicit UTF-8 before calling the public API."
  - "BA2 DX10 canonical extraction expectations are reconstructed from manifest-backed DDS header metadata plus committed payload bytes instead of copied expected trees."
  - "A smoke policy test locks the suite to public reader and validation APIs so Phase 13 does not drift into TES3, writer, or Phase 14 coverage."
patterns-established:
  - "Windows non-ASCII path regressions can create files with native path components while still feeding the public API an explicit UTF-8 string."
  - "Dedicated phase-close fixture suites can use manifest-backed canonical entries plus smoke-tagged policy checks for fast focused feedback."
requirements-completed: [HOST-01, HOST-02, HOST-03]
duration: 7 min
completed: 2026-05-13
---

# Phase 13 Plan 05: Dedicated host-path regression proof summary

**A dedicated Catch2 suite now proves non-ASCII Windows host-path open, validation, and canonical extraction across TES4 and representative BA2 archives from the public API.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-05-13T23:45:28Z
- **Completed:** 2026-05-13T23:52:14Z
- **Tasks:** 3
- **Files modified:** 2

## Accomplishments
- Added `tests/unit/host_path_correctness_boundary_tests.cpp` as the dedicated Phase 13 cross-family regression suite.
- Registered the suite in `tests/CMakeLists.txt` so both `host_path_correctness_boundary` and `host_path_correctness_boundary_smoke` selectors discover fast focused coverage.
- Proved `archive_reader::open`, `validate_archive(..., {.validate_entry_extractability = true})`, and one canonical manifest-backed extraction for TES4 v103/v104/v105, Fallout 4 BA2 GNRL, Fallout 4 BA2 DX10, and Starfield BA2 GNRL v3 from copied non-ASCII paths.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add the dedicated cross-family non-ASCII host-path suite and register its focused filters** - `4df3fe3` (test)
2. **Task 1 GREEN: Add the dedicated cross-family non-ASCII host-path suite and register its focused filters** - `2c9c75f` (feat)
3. **Task 2 RED: Cover the full locked representative matrix for open, validate, and one canonical extraction each** - `279133c` (test)
4. **Task 2 GREEN: Cover the full locked representative matrix for open, validate, and one canonical extraction each** - `bd53324` (feat)
5. **Task 3: Keep the suite phase-scoped, deterministic, and aligned with the validation contract** - `a7d9bbd` (refactor)

**Plan metadata:** pending state/roadmap commit

## Files Created/Modified
- `tests/unit/host_path_correctness_boundary_tests.cpp` - Dedicated non-ASCII host-path regression suite with smoke setup, representative matrix coverage, DX10 manifest-backed DDS byte materialization, and phase-scope policy checks.
- `tests/CMakeLists.txt` - Registers the dedicated suite in `libbsa_tests` discovery so focused selectors can run it directly.

## Decisions Made
- Built the copied temp directory and renamed archive filename with filesystem-native non-ASCII path components, then converted those paths back to explicit UTF-8 before calling the unchanged public API.
- Reconstructed BA2 DX10 canonical expected bytes from manifest-backed DDS layout metadata plus committed payload hex so the suite stays black-box and does not copy expected payload trees into the temp root.
- Added a smoke policy test that asserts the suite stays on `archive_reader::open`, `validate_archive`, and `extract_bytes` while excluding TES3 and writer-side coverage from the Phase 13 close-out proof.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Switched non-ASCII temp-path creation to filesystem-native components**
- **Found during:** Task 1 (Add the dedicated cross-family non-ASCII host-path suite and register its focused filters)
- **Issue:** Initial smoke setup built the temp root from narrow UTF-8 strings, which threw on Windows before the public API proof could run.
- **Fix:** Constructed temp-directory and copied-filename components with native `std::filesystem::path` values, then preserved the explicit UTF-8 conversion only at the public API call boundary.
- **Files modified:** `tests/unit/host_path_correctness_boundary_tests.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary" --output-on-failure`
- **Committed in:** `2c9c75f` (part of task commit)

**2. [Rule 1 - Bug] Replaced temporary representative-case storage with a stable static matrix**
- **Found during:** Task 1 (Add the dedicated cross-family non-ASCII host-path suite and register its focused filters)
- **Issue:** Returning a temporary container and taking `.front()` created a dangling reference risk in the smoke setup.
- **Fix:** Promoted the representative archive matrix to a static `std::array` returned by reference.
- **Files modified:** `tests/unit/host_path_correctness_boundary_tests.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary" --output-on-failure`
- **Committed in:** `2c9c75f` (part of task commit)

---

**Total deviations:** 2 auto-fixed (2 bug)
**Impact on plan:** Both fixes kept the suite deterministic on Windows and avoided false failures in the exact non-ASCII path boundary the plan was meant to prove.

## Issues Encountered
- The first RED pass used `std::string::contains`, which is unavailable under the project’s C++20 contract; the test seed was corrected to `find(...) != npos` before continuing the TDD cycle.
- The first GREEN pass used `result<T>` as if it exposed `operator->`; the suite was corrected to use `.value()` before rerunning the representative matrix.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 13 now has direct public-API proof for HOST-01, HOST-02, and HOST-03, so Phase 14 can focus on verification-lane truthfulness instead of host-path correctness gaps.
- The new smoke selector gives later hardening work a fast targeted guard if future changes regress non-ASCII reader or validation behavior.

## Known Stubs

None.

## Self-Check: PASSED

- FOUND: `.planning/phases/13-host-path-correctness-boundary/13-05-SUMMARY.md`
- FOUND: `tests/unit/host_path_correctness_boundary_tests.cpp`
- FOUND: `tests/CMakeLists.txt`
- FOUND COMMIT: `4df3fe3`
- FOUND COMMIT: `2c9c75f`
- FOUND COMMIT: `279133c`
- FOUND COMMIT: `bd53324`
- FOUND COMMIT: `a7d9bbd`

---
*Phase: 13-host-path-correctness-boundary*
*Completed: 2026-05-13*
