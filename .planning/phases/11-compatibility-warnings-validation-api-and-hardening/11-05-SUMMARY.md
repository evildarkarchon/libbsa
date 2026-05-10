---
phase: 11-compatibility-warnings-validation-api-and-hardening
plan: 05
subsystem: testing
tags: [cpp20, catch2, malformed-fixtures, manifest-validation]

requires:
  - phase: 03-through-06
    provides: Existing TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10 malformed fixture tests.
  - phase: 11-plan-01
    provides: Stable public validation/error-code direction for Phase 11.
provides:
  - Strict TES3 malformed manifest expected-error mapping.
  - Strict TES4-family BSA malformed manifest expected-error mapping.
  - Strict BA2 GNRL malformed manifest expected-error mapping.
  - Malformed label verification evidence after helper hardening.
affects: [phase-11-malformed-matrix, compatibility-hardening, validation-api-tests]

tech-stack:
  added: []
  patterns:
    - Catch2 `FAIL(...)` guard for unknown malformed manifest `expected_error` strings.
    - Manifest-driven tests branch on stable public `libbsa::error_code` values.

key-files:
  created:
    - .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-05-SUMMARY.md
  modified:
    - tests/unit/tes3_bsa_reader_tests.cpp
    - tests/unit/tes4_bsa_reader_tests.cpp
    - tests/unit/ba2_gnrl_reader_tests.cpp

key-decisions:
  - "Unknown malformed manifest expected_error values are treated as test-oracle failures instead of being remapped to invalid_argument."

patterns-established:
  - "Per-family malformed manifest helpers preserve known `format_error` and `unsupported` mappings, then fail loudly on any unknown category."

requirements-completed: [COMP-04]

duration: 2 min
completed: 2026-05-10
---

# Phase 11 Plan 05: Malformed Manifest Mapping Summary

**Strict malformed manifest expected-error guards for TES3, TES4-family BSA, and BA2 GNRL reader tests**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-10T03:59:36Z
- **Completed:** 2026-05-10T04:01:34Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Replaced silent `invalid_argument` fallback mappings in TES3, TES4-family BSA, and BA2 GNRL malformed manifest helpers.
- Added family-specific Catch2 failure messages for unknown `expected_error` values so malformed fixture typos fail loudly.
- Verified the targeted malformed reader tests and the full `malformed` CTest label set still pass with stable public error-code assertions.
- Confirmed `TES5Edit/` stayed untouched.

## Task Commits

Each task was committed atomically:

1. **Task 1: Replace silent malformed expected_error fallbacks** - `fb6a6c5` (test)
2. **Task 2: Verify malformed reader suites and strict mapping gates** - `98872dc` (test, empty verification commit)

_Note: Task 2 was verification-only, so its commit intentionally records the completed gate without a file delta._

## Files Created/Modified

- `tests/unit/tes3_bsa_reader_tests.cpp` - Unknown TES3 malformed `expected_error` strings now fail through `FAIL(...)`.
- `tests/unit/tes4_bsa_reader_tests.cpp` - Unknown TES4-family malformed `expected_error` strings now fail through `FAIL(...)`.
- `tests/unit/ba2_gnrl_reader_tests.cpp` - Unknown BA2 GNRL malformed `expected_error` strings now fail through `FAIL(...)`.

## Verification

- `rg -n "unknown TES3 malformed expected_error" tests/unit/tes3_bsa_reader_tests.cpp` - passed.
- `rg -n "unknown TES4 malformed expected_error" tests/unit/tes4_bsa_reader_tests.cpp` - passed.
- `rg -n "unknown BA2 GNRL malformed expected_error" tests/unit/ba2_gnrl_reader_tests.cpp` - passed.
- `rg -n "return libbsa::error_code::invalid_argument;" tests/unit/tes3_bsa_reader_tests.cpp tests/unit/tes4_bsa_reader_tests.cpp tests/unit/ba2_gnrl_reader_tests.cpp` - returned no matches.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- `ctest --preset windows-msvc-debug-static -R "tes3_bsa_malformed|tes4_bsa_malformed|ba2_gnrl_malformed|ba2_dx10_malformed" --output-on-failure` - passed, 10/10 tests.
- `ctest --preset windows-msvc-debug-static -L malformed --output-on-failure` - passed, 22/22 tests.
- `git status --short TES5Edit` - returned no output.

## Decisions Made

- Unknown malformed manifest `expected_error` strings are treated as test-oracle failures, not as another public `error_code` category.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 11-02 and Plan 11-06 to build on strict malformed manifest behavior for validation reports and the consolidated malformed matrix.

## Self-Check: PASSED

- Found created/modified files: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and this SUMMARY.
- Found task commits: `fb6a6c5` and `98872dc`.
- Verification commands listed above passed.
- Stub scan over modified files found no TODO/FIXME/placeholder or hardcoded empty UI-data stubs.
- Threat surface scan found no new network endpoints, auth paths, file-access behavior outside tests, or schema changes.

---
*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Completed: 2026-05-10*
