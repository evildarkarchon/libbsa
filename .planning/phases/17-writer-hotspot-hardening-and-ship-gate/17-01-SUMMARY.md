---
phase: 17-writer-hotspot-hardening-and-ship-gate
plan: 01
subsystem: writer-hardening
tags: [cpp, cmake, catch2, tes4-bsa, dedupe, writer-hotspot-policy]

requires:
  - phase: 16-parser-and-preparer-seam-extraction
    provides: [private seam and policy-test patterns for writer hotspot hardening]
provides:
  - TES4-family BSA dedupe candidate narrowing keyed by final stored-payload identity
  - Source-policy guardrail preventing unkeyed all-prior TES4 dedupe scans
  - Runtime verification that TES4 dedupe sharing and mismatch behavior remains intact
affects: [writer-hotspot-hardening, DEDU-01, tes4-bsa-writer]

tech-stack:
  added: []
  patterns:
    - Format-local keyed dedupe candidate buckets with exact stored-byte equality fallback
    - Catch2 source-policy tests for writer hotspot invariants

key-files:
  created:
    - tests/unit/writer_hotspot_policy_tests.cpp
  modified:
    - src/formats/bsa/tes4_bsa_layout.cpp
    - tests/CMakeLists.txt
    - tests/unit/writer_hotspot_policy_tests.cpp

key-decisions:
  - "TES4 dedupe candidate identity uses stored size plus deterministic final stored-payload fingerprint only as a narrowing filter; tes4_stored_payloads_equal remains the sharing authority."
  - "Writer hotspot policy coverage is a dedicated Catch2 source-policy suite registered in the main libbsa_tests target."

patterns-established:
  - "TES4 dedupe narrowing: bucket by final stored-payload identity, then call exact equality before assigning shared offsets."
  - "Writer hotspot policy tests assert semantic guardrail tokens and forbidden all-prior scan patterns."

requirements-completed: [DEDU-01]

duration: 5 min
completed: 2026-05-15
---

# Phase 17 Plan 01: TES4 Dedupe Candidate Narrowing Summary

**TES4-family BSA writer dedupe now narrows candidates with a format-local keyed bucket while preserving exact final stored-byte equality before shared offsets.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-15T01:04:00Z
- **Completed:** 2026-05-15T01:08:52Z
- **Tasks:** 3 completed
- **Files modified:** 3

## Accomplishments

- Added the Phase 17 writer-hotspot policy suite and registered it with `libbsa_tests`.
- Implemented TES4 dedupe candidate buckets keyed by stored size plus deterministic final stored-payload fingerprint.
- Preserved runtime behavior for shared offsets, compression mismatch separation, embedded-name mismatch separation, disk-backed dedupe, and full Debug suite compatibility.

## TDD Evidence

### RED

- **Commit:** `087c2ad` — `test(17-01): add failing test for TES4 dedupe narrowing`
- **Failing command:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|writer_hotspot_policy"`
- **Expected failure:** `writer_hotspot_policy requires TES4 dedupe candidate narrowing before exact equality` failed because `std::map<tes4_dedupe_identity` was absent from `tes4_assign_offsets`.
- **Failure quality:** The build and test registration succeeded; failure was tied to the planned TES4 dedupe narrowing policy assertion.

### GREEN

- **Commit:** `637b4ea` — `feat(17-01): implement TES4 dedupe candidate narrowing`
- **Passing command:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|writer_hotspot_policy"`
- **Result:** 33/33 focused tests passed.

### REFACTOR

- **Commit:** `c0564b4` — `refactor(17-01): tighten TES4 dedupe policy guardrail`
- **Passing command:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|writer_hotspot_policy"`
- **Result:** 33/33 focused tests passed after making policy assertions more role-based.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED - add TES4 dedupe narrowing runtime and policy tests** - `087c2ad` (test)
2. **Task 2: GREEN - implement TES4 candidate narrowing with exact equality fallback** - `637b4ea` (feat)
3. **Task 3: REFACTOR - tighten TES4 guardrails without changing behavior** - `c0564b4` (refactor)

## Files Created/Modified

- `tests/unit/writer_hotspot_policy_tests.cpp` - New source-policy suite for TES4 dedupe narrowing and exact equality guardrails.
- `tests/CMakeLists.txt` - Registered the writer-hotspot policy suite in the main Catch2 executable.
- `src/formats/bsa/tes4_bsa_layout.cpp` - Added format-local TES4 dedupe identity buckets before exact equality checks.

## Decisions Made

- TES4 dedupe keys use stored size and a deterministic FNV-style fingerprint of final stored bytes, including disk-backed chunks when needed, only to narrow candidates.
- Shared payload offsets remain assigned only after `tes4_stored_payloads_equal(entry, *candidate.entry)` returns true.
- Policy tests assert semantic evidence and forbidden all-prior scan tokens rather than relying solely on the first exact helper type spelling.

## Verification

- **RED gate:** Focused label run failed on `writer_hotspot_policy` as intended after successful build/test registration.
- **Task focused gates:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|writer_hotspot_policy"` passed after GREEN and REFACTOR.
- **Wave gate:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` passed: 391/391 CTest tests passed, with 2 opt-in local-fixture tests skipped.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Removed compiler warning from duplicate local variable naming**
- **Found during:** Task 2 (GREEN - implement TES4 candidate narrowing with exact equality fallback)
- **Issue:** The initial GREEN implementation shadowed a `duplicate` local, producing MSVC warning C4456 during the focused build.
- **Fix:** Renamed the map iterator to `duplicate_bucket` while preserving the exact equality variable name used by the policy guard.
- **Files modified:** `src/formats/bsa/tes4_bsa_layout.cpp`
- **Verification:** Re-ran the focused Debug build and `tes4_bsa_writer|writer_hotspot_policy` label set successfully.
- **Committed in:** `637b4ea`

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** The fix removed a build warning without changing planned behavior or scope.

## Issues Encountered

None beyond the planned RED failure and the warning cleanup documented as an auto-fixed issue.

## Known Stubs

None.

## Threat Flags

None.

## User Setup Required

None - no external service configuration required.

## TDD Gate Compliance

- **RED:** Present (`087c2ad`)
- **GREEN:** Present after RED (`637b4ea`)
- **REFACTOR:** Present after GREEN (`c0564b4`)
- **Status:** Passed

## Self-Check: PASSED

- `tests/unit/writer_hotspot_policy_tests.cpp` exists.
- `src/formats/bsa/tes4_bsa_layout.cpp` contains `tes4_stored_payloads_equal` and keyed candidate bucket evidence.
- Commits `087c2ad`, `637b4ea`, and `c0564b4` exist in git history.
- No files under `TES5Edit/` were modified.
- No public headers under `include/libbsa/` were modified.

## Next Phase Readiness

Ready for Plan 17-02 (BA2 GNRL dedupe hardening). DEDU-01 has runtime and source-policy evidence; DEDU-02, DX10-01, and DX10-02 remain for later Phase 17 plans.

---
*Phase: 17-writer-hotspot-hardening-and-ship-gate*
*Completed: 2026-05-15*
