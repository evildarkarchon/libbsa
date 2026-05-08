---
phase: 03-format-detection-and-tes4-family-bsa-read-extract
plan: 01
subsystem: public-api
tags: [cpp20, archive-reader, public-api, catch2, nlohmann-json, cmake, vcpkg]

requires:
  - phase: 01-foundation-api-boundary-and-test-harness
    provides: public archive_reader facade, result/error model, Catch2/CTest harness
  - phase: 02-binary-i-o-paths-hashes-and-compression-services
    provides: private path, hash, compression, and sink primitives that remain hidden from public headers
provides:
  - Public Phase 3 archive metadata, entry metadata, compression, and payload sink contracts
  - archive_reader declarations for metadata, entries, lookup, contains, sink extraction, and byte extraction
  - Stable error_code::not_found for valid missing archive paths
  - Test-only nlohmann-json dependency wiring scoped to libbsa_tests
affects: [03-format-detection-and-tes4-family-bsa-read-extract, public-api, fixture-tests, extraction]

tech-stack:
  added: [nlohmann-json]
  patterns: [dependency-light public headers, sink-first extraction contract, compile-only API contract tests]

key-files:
  created: []
  modified:
    - include/libbsa/archive.hpp
    - include/libbsa/result.hpp
    - src/archive.cpp
    - tests/CMakeLists.txt
    - tests/unit/archive_reader_tests.cpp
    - tests/unit/public_include_boundary_tests.cpp
    - vcpkg.json

key-decisions:
  - "Expose Phase 3 reader contracts on archive_reader rather than adding separate public reader/view/extractor objects."
  - "Keep nlohmann-json test-only by finding it in tests/CMakeLists.txt and linking it PRIVATE to libbsa_tests only."
  - "Allow public compression enum names while keeping private codec targets and implementation tokens out of installed headers."

patterns-established:
  - "Public extraction uses a synchronous payload_sink returning accepted byte counts."
  - "archive_reader lookup distinguishes invalid paths from valid-missing paths through result<std::optional<entry_metadata>> plus error_code::not_found for extraction-time absence."
  - "Phase 3 fixture manifest JSON parsing is isolated to tests and does not affect libbsa runtime linkage."

requirements-completed: [FMT-02, FMT-03, FMT-04, FMT-05, FMT-06, BSA-05, BSA-06, BSA-07]

duration: 3min
completed: 2026-05-08
---

# Phase 03 Plan 01: Public Reader Contracts Summary

**Reader-centric archive metadata, entry metadata, sink extraction, missing-path errors, and test-only JSON manifest wiring for Phase 3 BSA work**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-08T07:21:30Z
- **Completed:** 2026-05-08T07:24:41Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Added documented public Phase 3 value types for archive metadata, entry metadata, archive type/variant, entry compression, and synchronous payload sinks.
- Added archive_reader method declarations and unsupported stubs so later parser plans can target a stable public API without linking failures.
- Added `error_code::not_found` and compile-only public API tests, while preserving empty host path validation.
- Added `nlohmann-json` through vcpkg and linked `nlohmann_json::nlohmann_json` only to `libbsa_tests` for future fixture manifests.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add public reader value and sink contracts** - `2e53e64` (feat)
2. **Task 2: Declare archive_reader methods and compile-only contract tests** - `5d85794` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `include/libbsa/archive.hpp` - Defines Phase 3 public archive/entry metadata, compression enums, payload_sink, and reader method declarations.
- `include/libbsa/result.hpp` - Adds the stable `error_code::not_found` category.
- `src/archive.cpp` - Adds unsupported reader method stubs and preserves empty-path invalid argument handling.
- `tests/CMakeLists.txt` - Finds and links nlohmann-json privately to the test executable.
- `tests/unit/archive_reader_tests.cpp` - Adds compile-only reader contract coverage, sink shape coverage, and JSON manifest dependency smoke coverage.
- `tests/unit/public_include_boundary_tests.cpp` - Extends public boundary coverage for new types and keeps private dependency tokens out of public headers.
- `vcpkg.json` - Adds `nlohmann-json` as the documented test/tool manifest dependency.

## Decisions Made

- Exposed the Phase 3 surface directly on `archive_reader`, matching D-01 and avoiding extra public handle/view types before parser state exists.
- Kept all new public types in `archive.hpp` so the existing umbrella include remains sufficient and no additional installed header is needed.
- Scoped nlohmann-json to tests only; the `libbsa` target remains free of JSON runtime/public linkage.

## Deviations from Plan

None - plan executed as written.

## Issues Encountered

- Reconfigured the CMake preset after adding `nlohmann-json` so vcpkg could install and expose the package before the planned build verification.

## TDD Gate Compliance

- RED was verified for both tasks before implementation, but RED and GREEN were not split into separate commits. Task commits remain atomic by plan task.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Later detector, parser, lookup, and extraction plans can now implement against stable public reader contracts.
- Fixture manifest parsing can use nlohmann-json in tests without leaking that dependency into the library.

## Self-Check: PASSED

- Verified all modified/source files and the SUMMARY exist on disk.
- Verified task commits `2e53e64` and `5d85794` exist in git history.

---
*Phase: 03-format-detection-and-tes4-family-bsa-read-extract*
*Completed: 2026-05-08*
