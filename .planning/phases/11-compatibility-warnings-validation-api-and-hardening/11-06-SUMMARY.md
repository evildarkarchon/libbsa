---
phase: 11-compatibility-warnings-validation-api-and-hardening
plan: 06
subsystem: testing
tags: [cpp20, catch2, malformed-fixtures, validation-api, tdd]

requires:
  - phase: 11-02
    provides: Strict-open-backed validation reports and validation API tests.
  - phase: 11-03
    provides: Compatibility warning policy and validation report integration.
  - phase: 11-05
    provides: Strict malformed manifest expected-error mapping.
provides:
  - Consolidated malformed hardening matrix across TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10.
  - C++ matrix tests for family/category coverage and manifest/test evidence references.
  - Validation API malformed-report coverage driven by matrix manifest rows.
  - Python validator checks for matrix schema, evidence references, and stable expected-error values.
affects: [phase-11-malformed-matrix, validation-api-tests, sanitizer-hardening-path]

tech-stack:
  added: []
  patterns:
    - Manifest-backed matrix rows reference generated legal archives and manifest case IDs.
    - Test-backed matrix rows reference explicit in-repo test files and test-name tokens.
    - Validation report matrix coverage branches on stable public error_code values, not diagnostic text.

key-files:
  created:
    - tests/fixtures/generated/compatibility_matrix.json
    - tests/unit/compatibility_matrix_tests.cpp
    - .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-06-SUMMARY.md
  modified:
    - tests/fixtures/generated/validate_fixture_manifests.py
    - tests/unit/validation_api_tests.cpp
    - tests/CMakeLists.txt

key-decisions:
  - "The consolidated malformed matrix uses manifest evidence for generated archive rows and test evidence for the TES4 oversized arithmetic regression."
  - "Validation API matrix coverage runs each manifest-backed row through strict open or opt-in extractability validation according to the row phase."
  - "The Python validator rejects unknown matrix families, categories, expected errors, phases, evidence types, and unresolved evidence references."

patterns-established:
  - "Matrix evidence rows use exactly one evidence type: generated manifest/archive/case or explicit test file/test token."
  - "Malformed validation report checks assert public error_code values and report invalidity without exact message coupling."

requirements-completed: [COMP-04, COMP-05]

duration: 4 min
completed: 2026-05-10
---

# Phase 11 Plan 06: Malformed Hardening Matrix Summary

**Consolidated malformed coverage matrix with machine-checked evidence and validation-report enforcement**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-10T04:25:56Z
- **Completed:** 2026-05-10T04:29:53Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added RED `compatibility_matrix` tests that fail when the consolidated matrix is missing, incomplete, or points at unresolved evidence.
- Added `compatibility_matrix.json` covering TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, compression failures, oversized arithmetic, unsupported routes, and DDS chunk/layout cases.
- Extended `validation_api` malformed coverage so manifest-backed matrix rows become invalid validation reports while strict `archive_reader::open` remains fail-closed.
- Extended the Python manifest validator to reject unknown matrix values and broken manifest/test evidence references.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED malformed matrix and validation-report tests** - `652c34d` (test)
2. **Task 2: GREEN consolidated matrix and Python manifest validator** - `da889d0` (feat)

_Note: This TDD plan produced RED and GREEN commits. No refactor commit was needed._

## Files Created/Modified

- `tests/fixtures/generated/compatibility_matrix.json` - Consolidated Phase 11 malformed hardening matrix.
- `tests/fixtures/generated/validate_fixture_manifests.py` - Matrix schema, category, expected-error, and evidence-reference validation.
- `tests/unit/compatibility_matrix_tests.cpp` - C++ checks for matrix families, categories, and evidence references.
- `tests/unit/validation_api_tests.cpp` - Matrix-driven malformed validation-report checks.
- `tests/CMakeLists.txt` - Registers the compatibility matrix test file.

## Verification

- RED: `ctest --preset windows-msvc-debug-static -R "compatibility_matrix|validation_api" --output-on-failure` - failed as expected before matrix creation; 3 matrix-dependent tests failed because `compatibility_matrix.json` was missing.
- GREEN: `python tests/fixtures/generated/validate_fixture_manifests.py` - passed.
- GREEN: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- GREEN: `ctest --preset windows-msvc-debug-static -R "compatibility_matrix|validation_api" --output-on-failure` - passed, 7/7 tests.
- Plan-level: `ctest --preset windows-msvc-debug-static -L malformed --output-on-failure` - passed, 26/26 tests.
- `rg -n "phase11_malformed_hardening" tests/fixtures/generated/compatibility_matrix.json` - found matrix kind.
- `rg -n "tes3_bsa|tes4_bsa|ba2_gnrl|ba2_dx10" tests/fixtures/generated/compatibility_matrix.json` - found all family tokens.
- `rg -n "truncated_structure|duplicate_canonical_path|invalid_payload_span|unsupported_route|decompression_failure|oversized_arithmetic|dds_chunk_layout" tests/fixtures/generated/compatibility_matrix.json` - found all required categories.
- `rg -n "\"evidence_type\": \"test\"|tests/unit/tes4_bsa_reader_tests.cpp|test_name" tests/fixtures/generated/compatibility_matrix.json` - found the explicit test-backed oversized arithmetic row.
- `rg -n "validate_compatibility_matrix" tests/fixtures/generated/validate_fixture_manifests.py` - found the Python validator.
- `git status --short TES5Edit` - returned no output.

## Decisions Made

- Used generated manifest evidence for all archive-file rows to keep mandatory evidence legal and reproducible.
- Used a test-backed row for TES4 oversized arithmetic because the existing regression mutates a generated fixture in-memory rather than storing another generated archive.
- Kept validation API matrix assertions at the report/error-code layer, avoiding exact message assertions and avoiding any usable reader in malformed reports.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed Catch2 assertion shape in RED matrix tests**
- **Found during:** Task 1 (RED malformed matrix and validation-report tests)
- **Issue:** Catch2 rejected a chained `||` expression inside `CHECK(...)`, blocking the RED test build before the intended missing-matrix failure.
- **Fix:** Wrapped the boolean expression so Catch2 could compile the assertion.
- **Files modified:** `tests/unit/compatibility_matrix_tests.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` passed after the fix, then the focused RED CTest failed for the intended missing-matrix reason.
- **Committed in:** `652c34d`

**Total deviations:** 1 auto-fixed (1 blocking issue)
**Impact on plan:** The fix preserved the planned RED behavior and did not broaden scope.

## Issues Encountered

- The initial RED build failed on Catch2 assertion syntax before exercising the missing matrix. This was corrected in the RED task commit and documented as a Rule 3 deviation.

## Known Stubs

None. Stub scan found only test helper default initializers, not placeholder behavior or UI-data stubs.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 11-07 to add the additive sanitizer-oriented preset/documentation path on top of the malformed matrix and validation label coverage.

## Self-Check: PASSED

- Found summary file: `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-06-SUMMARY.md`.
- Found created/modified files: `tests/fixtures/generated/compatibility_matrix.json`, `tests/unit/compatibility_matrix_tests.cpp`, `tests/fixtures/generated/validate_fixture_manifests.py`, `tests/unit/validation_api_tests.cpp`, and `tests/CMakeLists.txt`.
- Found task commits: `652c34d` and `da889d0`.
- Verification commands listed above passed except for the intentional RED failure before matrix creation.
- Stub scan found no placeholders that block the plan goal.
- Threat surface scan found no new network endpoints, auth paths, runtime file-access surfaces, or schema changes at trust boundaries.

---
*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Completed: 2026-05-10*
