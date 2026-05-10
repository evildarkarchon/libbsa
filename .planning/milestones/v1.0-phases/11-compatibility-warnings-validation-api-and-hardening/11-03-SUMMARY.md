---
phase: 11-compatibility-warnings-validation-api-and-hardening
plan: 03
subsystem: validation-api
tags: [cpp20, validation-api, compatibility-warnings, tdd, catch2]

requires:
  - phase: 11-02
    provides: Strict-open-backed `validate_archive` implementation and validation API behavior tests.
provides:
  - Representative typed compatibility-warning scenarios across BSA and BA2.
  - Private compatibility-warning policy appended to validation reports.
  - Writer-output warning evidence for target-family mismatch, BSA embedded-name risk, and compressed sound payloads.
affects: [phase-11-validation-api, compatibility-warnings, evidence-catalog]

tech-stack:
  added: []
  patterns:
    - Warning tests assert stable code, severity, and archive_path presence only.
    - Warning policy derives records from parsed metadata and public entry metadata, not file extensions or private parser coordinates.

key-files:
  created:
    - tests/unit/compatibility_warning_tests.cpp
  modified:
    - src/validation.cpp
    - tests/CMakeLists.txt

key-decisions:
  - "Target-family mismatch warnings compare caller expectations against parsed archive metadata only."
  - "Entry-level warning records include optional normalized archive paths but no byte offsets, record indexes, or chunk indexes."
  - "Sound payload warning detection treats compressed entries under sound/ or with .wav, .xwm, or .fuz extensions as advisory compatibility risks."

patterns-established:
  - "Compatibility warnings are appended privately in `src/validation.cpp` after strict open and metadata collection."
  - "Warning tests use writer-produced archives and do not compare full diagnostic message text."

requirements-completed: [COMP-02, COMP-03]

duration: 4 min
completed: 2026-05-10
---

# Phase 11 Plan 03: Compatibility Warning Policy Summary

**Typed compatibility warnings for BA2 target mismatch, BSA embedded names, and compressed sound payloads**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-10T04:12:51Z
- **Completed:** 2026-05-10T04:16:46Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added RED `compatibility_warning` tests using public writer APIs to produce valid BA2 and BSA archives.
- Implemented private validation warning helpers for `target_family_mismatch`, `bsa_embedded_name_compatibility_risk`, and `compressed_sound_payload`.
- Preserved D-09 by keeping warning records limited to stable code, severity, human message, and optional archive path.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED warning scenarios and stable assertion rules** - `d4827dc` (test)
2. **Task 2: GREEN private compatibility warning policy** - `9dfd471` (feat)

_Note: This TDD plan produced RED and GREEN commits. No refactor commit was needed._

## Files Created/Modified

- `tests/unit/compatibility_warning_tests.cpp` - Writer-output warning scenarios and stable code/severity/path assertions.
- `tests/CMakeLists.txt` - Registers the compatibility warning test file in `libbsa_tests`.
- `src/validation.cpp` - Appends private compatibility warnings after strict open, metadata collection, and entry metadata collection.

## Verification

- RED: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- RED: `ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure` - failed as expected before policy implementation; all three cases reported missing expected warnings.
- GREEN: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- GREEN: `ctest --preset windows-msvc-debug-static -R "compatibility_warning|validation_api" --output-on-failure` - passed, 7/7 tests.
- Final repeat: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- Final repeat: `ctest --preset windows-msvc-debug-static -R "compatibility_warning|validation_api" --output-on-failure` - passed, 7/7 tests.
- `rg -n "compatibility_warning" tests/unit/compatibility_warning_tests.cpp` - found lowercase selectable test names.
- `rg -n "target_family_mismatch|bsa_embedded_name_compatibility_risk|compressed_sound_payload" tests/unit/compatibility_warning_tests.cpp src/validation.cpp` - found all test assertions and policy branches.
- `rg -n "warning\\.message\\s*==" tests/unit/compatibility_warning_tests.cpp` - returned no matches.
- `rg -n "record_index|chunk_index|byte_offset|payload_offset" src/validation.cpp include/libbsa/validation.hpp tests/unit/compatibility_warning_tests.cpp` - returned no matches.
- `git status --short TES5Edit` - returned no output.

## Decisions Made

- Used parsed `archive_metadata` for target-family mismatch so warning behavior does not depend on host file extensions.
- Reused public `archive_reader::entries()` metadata to derive warning records, avoiding new public policy classes or logging callbacks.
- Kept warning messages human-readable and untested; tests assert only code, severity, and optional path presence.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 11-04 to catalog the warning evidence and enforce documentation coverage for every public warning code.

## Self-Check: PASSED

- Found summary file: `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-03-SUMMARY.md`.
- Found created/modified files: `tests/unit/compatibility_warning_tests.cpp`, `tests/CMakeLists.txt`, and `src/validation.cpp`.
- Found task commits: `d4827dc` and `9dfd471`.
- Verification commands listed above passed except for the intentional RED failure before implementation.

---
*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Completed: 2026-05-10*
