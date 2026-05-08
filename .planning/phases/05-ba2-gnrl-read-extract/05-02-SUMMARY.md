---
phase: 05-ba2-gnrl-read-extract
plan: 02
subsystem: archive-reader
tags: [cpp20, ba2, gnrl, btdx, metadata, detector, tdd]

requires:
  - phase: 05-ba2-gnrl-read-extract
    provides: BA2 GNRL generated fixtures and manifests from Plan 05-01
provides:
  - Public BA2 archive metadata optionals for Starfield v2/v3 header fields
  - Private BA2 BTDX/GNRL/DX10 detector with unsupported-profile routing
  - Metadata-only archive_reader open skeleton for FO4, Starfield v2, and Starfield v3 BA2 GNRL fixtures
affects: [phase-05-ba2-parser, phase-06-ba2-dx10, phase-08-ba2-writer, public-api]

tech-stack:
  added: []
  patterns: [byte-driven BA2 detection, version-gated public optionals, metadata-only reader skeleton]

key-files:
  created:
    - src/formats/ba2/ba2_format_detector.hpp
    - src/formats/ba2/ba2_format_detector.cpp
    - tests/unit/ba2_gnrl_reader_tests.cpp
  modified:
    - include/libbsa/archive.hpp
    - src/archive.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt
    - tests/unit/public_include_boundary_tests.cpp

key-decisions:
  - "BA2 detection runs before BSA fallback whenever archive bytes begin with BTDX, preserving byte-driven routing and clean unsupported errors for DX10."
  - "Starfield BA2 v2/v3 raw header fields are exposed as version-gated std::optional values so absent Fallout 4 fields remain distinct from present zero values."
  - "Plan 05-02 intentionally opens BA2 GNRL archives to metadata-only reader state; full record/name parsing remains Plan 05-03 scope."

patterns-established:
  - "Format-specific detectors live under src/formats/<family>/ and return private detected_* structs consumed by archive_reader::open."
  - "Unsupported valid BA2 profiles, including DX10 and unknown Starfield v3 CompressionMethod values, fail with error_code::unsupported before parser work."

requirements-completed: [GNRL-01, GNRL-02, GNRL-03, GNRL-08]

duration: 5min
completed: 2026-05-08
---

# Phase 05 Plan 02: BA2 Metadata Detector/Open Skeleton Summary

**BTDX-driven BA2 metadata detection with version-gated Starfield optionals and metadata-only FO4/SFv2/SFv3 open routing.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-08T22:50:27Z
- **Completed:** 2026-05-08T22:55:33Z
- **Tasks:** 2 completed
- **Files modified:** 8

## Accomplishments

- Added `ba2_archive_metadata` and `archive_metadata::ba2` to the public C++20 API without exposing libdeflate, lz4, DirectXTex, Windows SDK, or TES5Edit types.
- Added RED BA2 detector tests for Fallout 4, Starfield v2, Starfield v3, DX10 rejection, unsupported v3 compression methods, and public include boundaries.
- Implemented a private BA2 detector that classifies `BTDX`/`GNRL`/`DX10`, reads Starfield v2/v3 header fields, and routes valid BA2 GNRL archives through `archive_reader::open` before BSA fallback.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add public BA2 metadata optionals and detector tests** - `635b48e` (test)
2. **Task 2: Implement BA2 byte detector and open skeleton** - `6551deb` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `include/libbsa/archive.hpp` - Adds public `ba2_archive_metadata` and optional BA2 metadata on `archive_metadata`.
- `src/formats/ba2/ba2_format_detector.hpp` - Declares private BA2 detector result and detection API.
- `src/formats/ba2/ba2_format_detector.cpp` - Implements `BTDX`/`GNRL`/`DX10` classification, Starfield field parsing, and unsupported v3 method handling.
- `src/archive.cpp` - Routes BA2 bytes through the BA2 detector before BSA fallback and creates metadata-only BA2 reader state.
- `CMakeLists.txt` - Adds the BA2 detector implementation to the `libbsa` target.
- `tests/unit/ba2_gnrl_reader_tests.cpp` - Adds fixture-backed BA2 GNRL metadata and unsupported-profile tests.
- `tests/unit/public_include_boundary_tests.cpp` - Covers default construction of the public BA2 metadata type.
- `tests/CMakeLists.txt` - Adds the BA2 GNRL reader tests to `libbsa_tests`.

## Decisions Made

- BA2 detection is dispatched by `BTDX` bytes before BSA detection so valid BA2 unsupported profiles do not get misclassified as generic BSA failures.
- Starfield `Unknown1`, `Unknown2`, and `CompressionMethod` remain raw optional fields because their semantics are not fully established by the current reference evidence.
- Full BA2 record parsing, filename-table parsing, entries, lookup, and extraction are intentionally deferred to later Phase 5 plans; this plan only establishes stable contracts and open skeleton behavior.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- The BA2 detector needed a 36-byte detection prefix to include the full Starfield v3 fixed header (`BTDX`, version, subtype, file count, file table offset, `Unknown1`, `Unknown2`, and `CompressionMethod`). This was resolved during Task 2 before commit.

## TDD Gate Compliance

- RED gate: `635b48e` added BA2 metadata detector tests and verified they failed because BA2 GNRL fixtures did not open yet.
- GREEN gate: `6551deb` implemented BA2 detection/open routing and verified the focused BA2 detector tests passed.

## Known Stubs

- None. The metadata-only BA2 reader state is the intended Plan 05-02 skeleton; Plan 05-03 owns entry parsing and lookup.

## Verification

- `cmake --build --preset windows-msvc-debug-static` — passed.
- `ctest --preset windows-msvc-debug-static -L ba2_gnrl_detector --output-on-failure` — passed.
- `ctest --preset windows-msvc-debug-static -L public-api --output-on-failure` — passed.
- `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_detector|public-api" --output-on-failure` — passed.
- `git -C TES5Edit status --short` — no output.
- Task acceptance greps for public metadata fields, BA2 test labels, detector literals, `error_code::unsupported`, BA2 open dispatch, and CMake source registration — passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for Plan 05-03 to parse BA2 GNRL records and length-prefixed filename tables using the detector/open metadata contract created here.
- No blockers.

## Self-Check: PASSED

- Found summary and key BA2 detector/test files on disk.
- Found task commits `635b48e` and `6551deb` in git history.

---
*Phase: 05-ba2-gnrl-read-extract*
*Completed: 2026-05-08*
