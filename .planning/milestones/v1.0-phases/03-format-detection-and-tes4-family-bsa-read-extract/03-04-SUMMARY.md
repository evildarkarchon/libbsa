---
phase: 03-format-detection-and-tes4-family-bsa-read-extract
plan: 04
subsystem: archive-parser
tags: [cpp20, bsa, tes4, detection, parser, catch2]

requires:
  - phase: 03-format-detection-and-tes4-family-bsa-read-extract
    provides: generated TES4-family success and malformed fixture archives
provides:
  - Byte-driven private BSA magic/version detector for v103, v104, and v105
  - TES4-family parser skeleton exposing archive metadata from checked bytes
  - archive_reader open state for metadata inspection before entry parsing
affects: [tes4-bsa-listing, tes4-bsa-lookup, tes4-bsa-extraction]

tech-stack:
  added: []
  patterns: [private format detector, binary_reader parser skeleton, manifest-driven malformed tests]

key-files:
  created:
    - src/formats/bsa/bsa_format_detector.hpp
    - src/formats/bsa/bsa_format_detector.cpp
    - src/formats/bsa/tes4_bsa_parser.hpp
    - src/formats/bsa/tes4_bsa_parser.cpp
    - tests/unit/tes4_bsa_reader_tests.cpp
  modified:
    - CMakeLists.txt
    - include/libbsa/archive.hpp
    - src/archive.cpp
    - tests/CMakeLists.txt
    - tests/unit/archive_reader_tests.cpp

key-decisions:
  - "TES4-family BSA variants share archive_variant::tes4 for now; exact version and default compression distinguish v103/v104/v105 behavior without expanding the public enum."
  - "Duplicate canonical path malformed fixture remains deferred to full entry parsing in Plan 03-05; Plan 03-04 only exposes archive metadata state."

patterns-established:
  - "archive_reader::open reads host bytes, delegates to private detector/parser, and stores library-owned metadata state."
  - "Malformed open tests assert stable error_code values and avoid diagnostic string coupling."

requirements-completed: [FMT-01, FMT-02, FMT-06]

duration: 35min
completed: 2026-05-08
---

# Phase 03 Plan 04: Byte-Driven TES4-Family BSA Open Metadata Summary

**Byte-driven TES4-family BSA detection with checked metadata-only reader state for v103/v104/v105 fixtures**

## Performance

- **Duration:** 35 min
- **Started:** 2026-05-08T08:10:00Z
- **Completed:** 2026-05-08T08:45:00Z
- **Tasks:** 2 completed
- **Files modified:** 10

## Accomplishments

- Added private BSA format detector that classifies `BSA\0` bytes and versions `0x67`, `0x68`, and `0x69` without trusting host filename extensions.
- Added TES4-family parser skeleton using `detail::binary_reader` to validate fixed headers, folder records, folder/file table spans, and count-derived lengths before exposing metadata.
- Wired `archive_reader::open` to return a usable reader state whose `metadata()` reports archive type, family variant, version, flags, file count, and default compression behavior.
- Added Catch2 selectors for `tes4_bsa_detection`, `tes4_bsa_metadata`, `unsupported_future_bsa`, and `tes4_bsa_malformed_open`.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Create private byte-driven detector and open-state skeleton tests** - `8d6630b` (test)
2. **Task 1 GREEN: Create private byte-driven detector and open-state skeleton** - `8d15065` (feat)
3. **Task 2 RED: Harden skeleton bounds checks tests** - `99ef560` (test)
4. **Task 2 GREEN: Harden skeleton bounds checks for malformed open cases** - `fa9ad84` (feat)

**Plan metadata:** final `docs(03-04)` commit

## Files Created/Modified

- `src/formats/bsa/bsa_format_detector.hpp` / `.cpp` - Private magic/version detector returning supported version metadata and default compression routing.
- `src/formats/bsa/tes4_bsa_parser.hpp` / `.cpp` - Checked metadata-only TES4-family parser skeleton using `binary_reader`.
- `src/archive.cpp` - Public open facade now reads host bytes, delegates to detector/parser, and stores opened metadata state.
- `include/libbsa/archive.hpp` - Adds private reader state storage needed for successful opens.
- `CMakeLists.txt` - Registers private BSA detector/parser sources.
- `tests/CMakeLists.txt` - Registers TES4 BSA reader tests.
- `tests/unit/archive_reader_tests.cpp` - Updates old unsupported-stub expectation for missing host files.
- `tests/unit/tes4_bsa_reader_tests.cpp` - Adds fixture-backed detection, metadata, future-version, and malformed-open tests.

## Decisions Made

- TES4-family BSA open metadata currently reports `archive_variant::tes4` for v103/v104/v105, with `version` and `default_compression` carrying the subtype-specific behavior. This avoids speculative public enum expansion before the full variant model is needed.
- The duplicate canonical path malformed fixture is intentionally skipped in Plan 03-04 malformed-open coverage because duplicate detection requires full entry path parsing deferred to Plan 03-05.

## Deviations from Plan

None - plan executed as written for the metadata-open slice. `include/libbsa/archive.hpp` was modified to add private owned state, which is required by the planned `archive_reader::open`/`metadata()` behavior.

## Known Stubs

- `src/archive.cpp` — `entries()`, `find()`, `contains()`, `extract()`, and `extract_bytes()` still return `error_code::unsupported`; this is intentional because entry listing/lookup/extraction are scheduled for later Phase 03 plans.

## Issues Encountered

- The malformed manifest includes `duplicate_canonical_path` as an open-phase case, but Plan 03-04 explicitly limits reader state to metadata and defers entry/name validation. The test documents this by skipping that manifest case until Plan 03-05.

## User Setup Required

None - no external service configuration required.

## Verification

- `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -R "tes4_bsa_detection|tes4_bsa_metadata|unsupported_future_bsa|tes4_bsa_malformed_open|public_include_boundary" --output-on-failure && git -C "TES5Edit" status --short` — PASSED.

## TDD Gate Compliance

- RED commits present: `8d6630b`, `99ef560`.
- GREEN commits present after RED: `8d15065`, `fa9ad84`.
- Refactor commit: not needed.

## Self-Check: PASSED

- Created files exist on disk.
- Task commits are present in git log.
- Final selector verification passed.

## Next Phase Readiness

Ready for Plan 03-05 to parse full TES4-family entry tables, canonical paths, duplicate detection, and listing metadata on top of the detector/parser seam created here.

---
*Phase: 03-format-detection-and-tes4-family-bsa-read-extract*
*Completed: 2026-05-08*
