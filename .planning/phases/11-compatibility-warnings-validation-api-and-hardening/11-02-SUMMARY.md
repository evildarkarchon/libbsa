---
phase: 11-compatibility-warnings-validation-api-and-hardening
plan: 02
subsystem: validation-api
tags: [cpp20, validation-api, tdd, catch2, cmake]

requires:
  - phase: 11-01
    provides: Dependency-light public validation API contract and public include-boundary assertions.
provides:
  - Strict-open-backed `validate_archive` implementation.
  - Validation API tests for setup failures, generated fixtures, writer-produced archives, and malformed open failures.
  - Installed package-consumer smoke coverage for the validation API.
affects: [phase-11-validation-api, compatibility-warnings, malformed-hardening, package-consumer]

tech-stack:
  added: []
  patterns:
    - Strict `archive_reader::open` reuse as the validation parser source of truth.
    - Result-level setup failures separated from inspectable archive diagnostics in `validation_report.errors`.
    - Opt-in extractability validation through public `entries()` and `extract_bytes()` APIs.

key-files:
  created:
    - src/validation.cpp
    - tests/unit/validation_api_tests.cpp
  modified:
    - CMakeLists.txt
    - include/libbsa/validation.hpp
    - tests/CMakeLists.txt
    - tests/package-consumer/main.cpp

key-decisions:
  - "validate_archive reuses archive_reader::open as the single strict parser source of truth."
  - "Empty and unreadable host paths remain result-level failures, while readable unsupported or malformed archive bytes become fatal validation report diagnostics."
  - "Entry extractability validation stays opt-in and uses public reader APIs without exposing entry listings, extraction bytes, or archive_reader from validation_report."

patterns-established:
  - "Validation implementation lives in `src/validation.cpp` and depends only on public reader/report contracts."
  - "Validation API tests assert stable error_code values and report structure rather than diagnostic message text."

requirements-completed: [COMP-02, COMP-04]

duration: 5 min
completed: 2026-05-10
---

# Phase 11 Plan 02: Strict Validation Reports Summary

**Strict-open-backed public validation reports for generated fixtures, writer output, and inspectable malformed archives**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-10T04:04:29Z
- **Completed:** 2026-05-10T04:09:25Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added RED `validation_api` tests covering result-level setup errors, generated TES3/TES4/BA2 fixtures, writer-produced TES3/TES4/BA2 archives, and malformed open failures.
- Implemented `validate_archive` as a facade over strict `archive_reader::open`, with readable malformed archives reported through `validation_report.errors`.
- Added opt-in extractability validation using public `entries()` and `extract_bytes()` without exposing a reader or entry listing from the report.
- Updated the package-consumer smoke path to prove installed consumers can call `validate_archive`.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED validation API behavior tests** - `e88d153` (test)
2. **Task 2: GREEN strict-open-backed validation implementation** - `df94272` (feat)

_Note: This TDD plan produced RED and GREEN commits. No refactor commit was needed._

## Files Created/Modified

- `tests/unit/validation_api_tests.cpp` - Validation behavior tests for setup errors, success fixtures, writer outputs, and malformed reports.
- `tests/CMakeLists.txt` - Registers the validation API test file.
- `tests/package-consumer/main.cpp` - Uses `libbsa::validate_archive` in the installed consumer smoke.
- `src/validation.cpp` - Implements `validation_report::is_valid()` and `validate_archive`.
- `include/libbsa/validation.hpp` - Moves `validation_report::is_valid()` to the implementation file.
- `CMakeLists.txt` - Registers `src/validation.cpp` in the libbsa target.

## Verification

- RED: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - failed as expected with unresolved `libbsa::validate_archive`.
- GREEN: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- `ctest --preset windows-msvc-debug-static -R "validation_api|package_consumer|public_include_boundary" --output-on-failure` - passed, 8/8 tests.
- Plan-level repeat: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- Plan-level repeat: `ctest --preset windows-msvc-debug-static -R "validation_api|package_consumer|public_include_boundary" --output-on-failure` - passed, 8/8 tests.
- `rg -n "validation_api" tests/unit/validation_api_tests.cpp` - found lowercase CTest-selectable test names.
- `rg -n "tes3_success.bsa|tes4_v103.bsa|ba2_gnrl_fo4.ba2|ba2_dx10_fo4.ba2" tests/unit/validation_api_tests.cpp` - found generated fixture coverage.
- `rg -n "tes3_bsa_writer|tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer" tests/unit/validation_api_tests.cpp` - found all writer-output validation families.
- `rg -n "validate_archive" tests/package-consumer/main.cpp` - found installed-consumer API usage.
- `rg -n "archive_reader::open" src/validation.cpp` - confirmed strict reader reuse.
- `rg -n "validate_entry_extractability" src/validation.cpp include/libbsa/validation.hpp tests/unit/validation_api_tests.cpp` - confirmed option implementation and coverage.
- `rg -n "validation_report::is_valid" src/validation.cpp` - found the report helper.
- `rg -n "entries\\(\\)|extract_bytes" src/validation.cpp` - confirmed extraction-style validation uses public reader APIs.
- `rg -n "archive_reader" include/libbsa/validation.hpp` - returned no matches.
- `git status --short TES5Edit` - returned no output.

## Decisions Made

- Used `archive_reader::open` as the only parser source of truth, so validation cannot accidentally become a lenient second reader.
- Kept setup failures at the `result<validation_report>` level and converted only readable archive parse failures into fatal report diagnostics.
- Used generic validation diagnostic messages in `src/validation.cpp` so tests and consumers branch on stable `error_code` values rather than private parser wording.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 11-03 to add private compatibility-warning policy on top of the strict validation facade.

## Self-Check: PASSED

- Found summary file: `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-02-SUMMARY.md`.
- Found created files: `src/validation.cpp` and `tests/unit/validation_api_tests.cpp`.
- Found task commits: `e88d153` and `df94272`.
- Verification commands listed above passed except for the intentional RED failure before implementation.

---
*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Completed: 2026-05-10*
