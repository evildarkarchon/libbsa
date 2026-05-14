---
phase: 15-reader-backend-dispatch-cleanup
plan: 01
subsystem: reader
tags: [reader, dispatch, tests, catch2, ctest]
requires:
  - phase: 14-verification-lane-truthfulness
    provides: truthful Windows validation lanes and policy-test patterns
provides:
  - one open-time-selected file-local backend dispatch seam for archive_reader
  - dedicated runtime regression coverage for representative reader backends
  - source-policy coverage that blocks per-method family dispatch regressions
affects: [src/archive.cpp, tests/unit/archive_reader_dispatch_tests.cpp, tests/unit/archive_reader_dispatch_policy_tests.cpp, tests/CMakeLists.txt]
tech-stack:
  added: []
  patterns: [open-time backend table selection, facade-owned contains and bulk orchestration, repo-reading dispatch policy tests]
key-files:
  created: [tests/unit/archive_reader_dispatch_tests.cpp, tests/unit/archive_reader_dispatch_policy_tests.cpp, .planning/phases/15-reader-backend-dispatch-cleanup/15-01-SUMMARY.md]
  modified: [src/archive.cpp, tests/CMakeLists.txt]
key-decisions:
  - "archive_reader now selects a file-local backend table once during open and reuses it for entries, find, and payload extraction."
  - "contains remains a facade wrapper over find so invalid archive-path input keeps its existing invalid_argument behavior."
  - "extract_entries keeps duplicate exact-request coalescing and result mirroring in shared facade code while backend callbacks own only lookup and payload reads."
patterns-established:
  - "Phase-scoped source-policy tests can enforce negative dispatch invariants without freezing one helper or callback name."
  - "Representative cross-family reader regression suites can lock both not_found and invalid-path behavior alongside success-path extraction."
requirements-completed: [DISP-01, DISP-02]
duration: 10m
completed: 2026-05-14
---

# Phase 15 Plan 01: Reader Backend Dispatch Cleanup Summary

**archive_reader now chooses one backend seam at open time, then reuses it across lookup and extraction while dedicated runtime and policy suites lock the unchanged reader contract.**

## Performance

- **Duration:** 10m
- **Started:** 2026-05-14T09:08:17Z
- **Completed:** 2026-05-14T09:18:20Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments

- Added `tests/unit/archive_reader_dispatch_tests.cpp` to prove `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries` still behave correctly for representative TES3, TES4, BA2 GNRL, Starfield BA2 GNRL v3, and BA2 DX10 fixtures.
- Added `tests/unit/archive_reader_dispatch_policy_tests.cpp` to read `src/archive.cpp` directly and fail if public reader methods regain archive-family branching tokens or direct family helper calls.
- Reworked `src/archive.cpp` so `archive_reader::open` selects a file-local backend table once, `contains()` wraps `find()`, and payload reopens still use the stored resolved `detail::host_file_path`.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED — add focused dispatch regression and policy suites** - `b8b830a` (test)
2. **Task 2: GREEN — replace repeated public reader dispatch with one open-time-selected backend table** - `a7255e4` (feat)
3. **Task 3: REFACTOR — tighten seam readability and run the full validation contract** - `4fde08e` (refactor)

## Validation

- **Task 1 RED:** `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` - failed for the expected policy reason while the new runtime suite and existing bulk extraction coverage ran successfully.
- **Task 2 GREEN:** `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` - passed.
- **Task 3 quick loop:** `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` - passed.
- **Task 3 phase gate:** `ctest --preset windows-msvc-debug-static --output-on-failure` - passed with 377 tests and 2 expected opt-in skips.

## Files Created/Modified

- `src/archive.cpp` - adds the file-local backend identity/table seam, moves shared lookup and payload dispatch behind it, and keeps bulk orchestration facade-owned.
- `tests/unit/archive_reader_dispatch_tests.cpp` - adds the dedicated representative-family runtime dispatch regression suite.
- `tests/unit/archive_reader_dispatch_policy_tests.cpp` - adds the method-scoped source-policy guard over public reader methods.
- `tests/CMakeLists.txt` - registers both new Phase 15 suites in `libbsa_tests` discovery.

## Decisions Made

- Keep the backend seam file-local to `src/archive.cpp` instead of introducing a wider private dispatch API.
- Keep `contains()` as `find()` plus `has_value()` so malformed archive-internal paths still surface `invalid_argument` rather than collapsing to `false`.
- Keep duplicate-request grouping, sink creation, and per-request result mirroring inside `extract_entries()` so backend callbacks stay limited to lookup and payload extraction primitives.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Relaxed manifest assumptions in the new runtime dispatch suite**
- **Found during:** Task 1 verification
- **Issue:** The first runtime test draft assumed every representative fixture exposed `lookup_variants`, `raw_size`, and `stored_size`, which is not true for BA2 DX10 manifest entries.
- **Fix:** Added helper logic that falls back to canonical path lookup when variants are absent and compares size metadata only when the manifest actually exposes those fields.
- **Files modified:** `tests/unit/archive_reader_dispatch_tests.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"`
- **Committed in:** `b8b830a`

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** The fix removed a false-negative test assumption without changing the locked Phase 15 dispatch contract.

## Issues Encountered

- A free helper that returned `archive_reader::state` from outside the class hit C++ access control during Task 2. The implementation was adjusted to keep final state assembly inside `archive_reader::open` via a local lambda while preserving the single open-time selection seam.

## Known Stubs

None.

## Next Phase Readiness

- Phase 15 now locks the reader-dispatch seam with both runtime and source-policy coverage, so Phase 16 can split parser and preparer hotspots without reintroducing public reader branching.
- The representative dispatch suite gives later hardening work a fast signal if not-found handling, invalid-path handling, duplicate coalescing, or resolved-host-path payload reopens drift.

## Self-Check: PASSED

- FOUND: `.planning/phases/15-reader-backend-dispatch-cleanup/15-01-SUMMARY.md`
- FOUND COMMIT: `b8b830a`
- FOUND COMMIT: `a7255e4`
- FOUND COMMIT: `4fde08e`

---
*Phase: 15-reader-backend-dispatch-cleanup*
*Completed: 2026-05-14*
