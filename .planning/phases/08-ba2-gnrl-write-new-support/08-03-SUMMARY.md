---
phase: 08-ba2-gnrl-write-new-support
plan: 03
subsystem: writer-validation
tags: [cpp20, ba2, gnrl, writer, validation, tdd]

requires:
  - phase: 08-ba2-gnrl-write-new-support
    provides: public BA2 GNRL writer contract from Plan 08-01
provides:
  - BA2 GNRL writer-owned entry state for memory and disk entries
  - Write-time duplicate canonical path detection and source validation
  - Extension FourCC validation and transactional marker-output publishing
affects: [phase-08-plan-04, ba2-gnrl-writer, writer-roundtrip]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN validation tests for BA2 GNRL writer state behavior
    - Private BA2 GNRL writer model behind dependency-light public API

key-files:
  created:
    - src/formats/ba2/ba2_gnrl_writer.hpp
    - src/formats/ba2/ba2_gnrl_writer.cpp
    - tests/unit/ba2_gnrl_writer_tests.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Plan 08-03 validates and owns BA2 GNRL writer state but intentionally defers reader-reopenable BA2 serialization to Plan 08-04."
  - "Duplicate canonical BA2 GNRL writer paths are accepted during add and rejected at write time with format_error, preserving D-05."

patterns-established:
  - "Writer entries preserve caller path spelling with slash normalization while canonical paths drive validation and duplicate detection."
  - "Memory entries copy caller spans into writer-owned vectors at add time."

requirements-completed: [WBA2-01]

duration: 2 min
completed: 2026-05-09
---

# Phase 08 Plan 03: BA2 GNRL Writer State and Validation Foundation Summary

**BA2 GNRL writer-owned entry state with path validation, duplicate detection, overwrite guards, FourCC validation, and disk-source error handling.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-09T08:04:20Z
- **Completed:** 2026-05-09T08:06:52Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added RED Catch2 tests covering empty disk host paths, invalid archive paths, write-time duplicate canonical rejection, overwrite defaults, missing disk sources, and explicit Fallout 4 disk archive paths.
- Implemented BA2 GNRL writer-owned state using a private shared state object and private `ba2_gnrl_writer_entry` records.
- Routed public writer methods through validation and a private `write_ba2_gnrl_archive` entry point while keeping codec and TES5Edit types out of public headers.
- Added extension FourCC validation so archive extensions must fit the BA2 GNRL 4-byte field instead of being silently truncated.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add BA2 writer validation foundation tests** - `dfbff1b` (test)
2. **Task 2 GREEN: Implement BA2 writer state, validation, and marker write** - `c80299c` (feat)

## Files Created/Modified

- `tests/unit/ba2_gnrl_writer_tests.cpp` - Writer validation tests tagged `[ba2_gnrl_writer]`.
- `tests/CMakeLists.txt` - Registers BA2 writer tests with the existing Catch2 test executable.
- `src/formats/ba2/ba2_gnrl_writer.hpp` - Declares the private BA2 GNRL writer entry model and write contract.
- `src/formats/ba2/ba2_gnrl_writer.cpp` - Implements public writer methods, writer-owned state, validation, temp-file publish, and marker output.
- `CMakeLists.txt` - Builds the BA2 GNRL writer implementation with libbsa.

## Decisions Made

- Plan 08-03 stops at validation/state ownership and writes only a `BTDX` marker on successful validation; Plan 08-04 owns reader-reopenable BA2 GNRL table, payload, and filename-table serialization.
- Duplicate canonical paths remain add-time permissible and fail during `write_to()` with `error_code::format_error`, matching D-05 and the Phase 7 writer pattern.

## Deviations from Plan

None - plan executed exactly as written.

## TDD Gate Compliance

- RED gate: `dfbff1b test(08-03): add failing BA2 writer validation tests`
- GREEN gate: `c80299c feat(08-03): implement BA2 writer validation state`
- REFACTOR gate: Not needed; no cleanup-only changes were made after GREEN.

## Known Stubs

| File | Line | Reason |
|------|------|--------|
| `src/formats/ba2/ba2_gnrl_writer.cpp` | 204-216 | Successful validation currently writes a `BTDX` marker only. This is intentional for Plan 08-03 because Plan 08-04 owns reader-reopenable BA2 GNRL serialization. |

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — passed after GREEN implementation.
- `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` — passed (6/6 tests).
- `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` — passed (2/2 tests).
- `git -C TES5Edit status --short` — empty; the read-only reference submodule was not modified.

## Next Phase Readiness

Ready for Plan 08-04 to replace the validation marker with reader-reopenable Fallout 4, Starfield v2, and Starfield v3 BA2 GNRL serialization using end-of-archive filename tables.

## Self-Check: PASSED

- Created files exist: `src/formats/ba2/ba2_gnrl_writer.hpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`.
- Task commits exist: `dfbff1b`, `c80299c`.
- Verification commands pass and `TES5Edit/` remains clean.

---
*Phase: 08-ba2-gnrl-write-new-support*
*Completed: 2026-05-09*
