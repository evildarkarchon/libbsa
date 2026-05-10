---
phase: 04-tes3-bsa-read-extract
plan: 02
subsystem: bsa-reader
tags: [cxx20, catch2, tes3, bsa, detector, public-api]

requires:
  - phase: 04-tes3-bsa-read-extract
    provides: TES3 generated fixture contracts and RED public reader tests from Plan 04-01
provides:
  - Format-neutral public entry hash metadata through entry_metadata::archive_hash
  - Archive-absolute payload_offset documentation for all archive variants
  - TES3 BSA magic/version detection for Plan 04-03 parser dispatch
affects: [tes3-reader, bsa-detector, public-api, archive-metadata]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN commits for public metadata rename and detector classification
    - Detector-only TES3 coverage before parser/open dispatch implementation

key-files:
  created:
    - .planning/phases/04-tes3-bsa-read-extract/04-02-SUMMARY.md
  modified:
    - include/libbsa/archive.hpp
    - src/formats/bsa/bsa_format_detector.hpp
    - src/formats/bsa/bsa_format_detector.cpp
    - tests/unit/tes3_bsa_reader_tests.cpp
    - tests/unit/tes4_bsa_reader_tests.cpp

key-decisions:
  - "entry_metadata now exposes archive_hash instead of tes4_hash so TES3 and TES4-family entries share format-neutral public metadata."
  - "TES3 detection is limited to byte classification of little-endian 0x00000100 and deliberately defers parser/open routing to Plan 04-03."

patterns-established:
  - "TES3 detector tests can include the internal BSA detector header and use the generated TES3 success fixture prefix without requiring parser dispatch."
  - "Public payload offsets are documented as archive-absolute while TES3 raw data-section-relative offsets remain parser-internal or manifest-only."

requirements-completed: [BSA-04, BSA-08]

duration: 3 min
completed: 2026-05-08
---

# Phase 04 Plan 02: Metadata Rename and TES3 Detector Summary

**Format-neutral archive_hash metadata with archive-absolute payload offset docs and TES3 0x00000100 byte classification for parser handoff.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-08T12:01:20Z
- **Completed:** 2026-05-08T12:04:33Z
- **Tasks:** 2/2
- **Files modified:** 5

## Accomplishments

- Renamed the public entry hash field from `tes4_hash` to `archive_hash` without adding a compatibility duplicate.
- Documented `entry_metadata::payload_offset` as an archive-absolute byte offset for every archive variant.
- Added detector-only TES3 tests and implemented `detect_bsa_format` classification for little-endian `0x00000100` Morrowind BSA prefixes.

## Task Commits

Each task was committed atomically with TDD RED/GREEN gates:

1. **Task 1 RED: Rename public hash metadata tests** - `c2e0489` (test)
2. **Task 1 GREEN: Rename entry hash metadata** - `0cad906` (feat)
3. **Task 2 RED: TES3 detector tests** - `8e475c5` (test)
4. **Task 2 GREEN: TES3 detector implementation** - `6399221` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `include/libbsa/archive.hpp` - Renamed `entry_metadata::archive_hash` and clarified archive-absolute payload offset semantics.
- `src/formats/bsa/bsa_format_detector.hpp` - Generalized detector result wording from TES4-family-only to BSA variant classification.
- `src/formats/bsa/bsa_format_detector.cpp` - Recognizes TES3 magic/version `0x00000100` before TES4-family `BSA\0` handling.
- `tests/unit/tes3_bsa_reader_tests.cpp` - Uses `archive_hash` and adds `tes3_bsa_detector` coverage for generated TES3 bytes and unrelated unsupported bytes.
- `tests/unit/tes4_bsa_reader_tests.cpp` - Preserves existing TES4-family hash behavior through the neutral `archive_hash` field.

## Decisions Made

- `archive_hash` is the sole public hash field because Phase 4 is pre-v1 API cleanup and TES3 entries need the same metadata slot as TES4-family entries.
- TES3 `0x00000100` detection returns `entry_compression::none`; parser dispatch and full TES3 open behavior remain intentionally deferred to Plan 04-03.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- The plan-level quick `unit` label still includes intentional RED TES3 parser/open tests from Plan 04-01. This plan therefore used its specified focused gates (`tes3_bsa_detector`, `tes4_bsa`, and `public-api`) while leaving parser/open GREEN work for Plan 04-03.

## Verification

- RED Task 1: `cmake --build --preset windows-msvc-debug-static` failed as expected because tests referenced `entry_metadata::archive_hash` before the public field existed.
- Task 1 GREEN: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "tes4_bsa|public_include_boundary" --output-on-failure` — passed TES4-family regression tests; `ctest --preset windows-msvc-debug-static -L public-api --output-on-failure` — passed public include boundary tests.
- Task 1 acceptance greps for `archive_hash`, absence of `tes4_hash` in `include src tests`, and `archive-absolute` payload documentation — passed.
- RED Task 2: `ctest --preset windows-msvc-debug-static -L tes3_bsa_detector --output-on-failure` failed as expected before detector implementation for generated TES3 success bytes.
- Task 2 GREEN: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "tes3_bsa_detector|tes4_bsa" --output-on-failure` — passed.
- Final focused gate: `ctest --preset windows-msvc-debug-static -L "tes3_bsa_detector|tes4_bsa|public-api" --output-on-failure` — passed 32/32 tests.
- `git -C TES5Edit status --short` — produced no output.

## TDD Gate Compliance

- RED gate present: `c2e0489`, `8e475c5`
- GREEN gate present after RED: `0cad906`, `6399221`
- REFACTOR gate: not needed; no behavior-neutral cleanup was made after GREEN.

## Known Stubs

None. Stub scan found no TODO/FIXME/placeholder text in created or modified implementation and test files.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 04-03. Public metadata and detector classification now provide the required handoff for TES3 parser/open dispatch work.

## Self-Check: PASSED

- Found created SUMMARY and all key modified files.
- Verified task commits exist: `c2e0489`, `0cad906`, `8e475c5`, and `6399221`.

---
*Phase: 04-tes3-bsa-read-extract*
*Completed: 2026-05-08*
