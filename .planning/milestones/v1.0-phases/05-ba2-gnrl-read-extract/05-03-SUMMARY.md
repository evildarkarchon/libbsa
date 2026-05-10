---
phase: 05-ba2-gnrl-read-extract
plan: 03
subsystem: archive-reader
tags: [cpp20, ba2, gnrl, parser, filename-table, metadata, lookup, tdd]

requires:
  - phase: 05-ba2-gnrl-read-extract
    provides: BA2 detector/open metadata skeleton and generated GNRL fixtures from Plans 05-01 and 05-02
provides:
  - Checked BA2 GNRL record and UInt16 filename-table parsing
  - Deterministic BA2 GNRL entry listing sorted by canonical archive path
  - Normalized BA2 GNRL find and contains behavior through archive_reader
affects: [phase-05-ba2-extraction, phase-05-ba2-malformed, phase-06-ba2-dx10, phase-08-ba2-writer]

tech-stack:
  added: []
  patterns: [private BA2 parser/reader split, filename-table materialization, lower_bound lookup]

key-files:
  created:
    - src/formats/ba2/ba2_gnrl_parser.hpp
    - src/formats/ba2/ba2_gnrl_parser.cpp
    - src/formats/ba2/ba2_gnrl_reader.hpp
    - src/formats/ba2/ba2_gnrl_reader.cpp
  modified:
    - tests/unit/ba2_gnrl_reader_tests.cpp
    - src/archive.cpp
    - CMakeLists.txt

key-decisions:
  - "BA2 GNRL parsing remains private under src/formats/ba2 and materializes only public archive_metadata and entry_metadata values."
  - "BA2 GNRL lookup reuses canonical archive path normalization and lower_bound semantics established by BSA readers."

patterns-established:
  - "BA2 GNRL FileTableOffset parsing reads UInt16 length-prefixed names and normalizes original separators to `/` while preserving archive-derived spelling."
  - "PackedSize == 0 maps to raw entries; non-zero PackedSize uses detector-selected archive compression metadata."

requirements-completed: [GNRL-01, GNRL-02, GNRL-03, GNRL-04, GNRL-05, GNRL-08]

duration: 3min
completed: 2026-05-08
---

# Phase 05 Plan 03: BA2 GNRL Parser and Lookup Summary

**Checked BA2 GNRL record and filename-table parsing with manifest-backed metadata listing and normalized lookup dispatch.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-08T22:57:20Z
- **Completed:** 2026-05-08T23:00:49Z
- **Tasks:** 2 completed
- **Files modified:** 7

## Accomplishments

- Added RED manifest-driven tests proving BA2 GNRL entry metadata, sorted listings, lookup variants, valid missing paths, invalid caller paths, and raw/deflate/raw-LZ4 metadata classification.
- Added private BA2 GNRL parser files that validate `FileTableOffset`, record table spans, filename table spans, `BAADF00D` sentinels, payload spans, and duplicate canonical paths.
- Added private BA2 GNRL reader helpers and routed `archive_reader::entries`, `find`, and `contains` through BA2-specific helper dispatch when `metadata.type == archive_type::ba2`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add BA2 metadata/listing/lookup RED tests** - `75cf55a` (test)
2. **Task 2: Implement BA2 GNRL parser and reader dispatch** - `9b3dc0d` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/ba2_gnrl_reader_tests.cpp` - Adds BA2 GNRL metadata and lookup tests over all generated success manifests.
- `src/formats/ba2/ba2_gnrl_parser.hpp` - Declares the private BA2 GNRL parser contract.
- `src/formats/ba2/ba2_gnrl_parser.cpp` - Implements checked BA2 GNRL header, record, filename-table, payload-span, and duplicate-path parsing.
- `src/formats/ba2/ba2_gnrl_reader.hpp` - Declares private BA2 GNRL entries/find/contains helpers.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - Implements deterministic entries copies and normalized lower_bound lookup helpers.
- `src/archive.cpp` - Routes BA2 open through the parser and dispatches entries/find/contains to BA2 helpers.
- `CMakeLists.txt` - Registers the BA2 GNRL parser and reader implementation sources.

## Decisions Made

- BA2 GNRL parser internals remain private; the public API surface continues to expose generic `archive_metadata` and `entry_metadata` only.
- BA2 compressed-entry classification is metadata-driven: `PackedSize == 0` is raw, otherwise the detector-selected default compression controls deflate versus raw LZ4 block.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## TDD Gate Compliance

- RED gate: `75cf55a` added BA2 GNRL metadata/listing/lookup tests and verified they failed because BA2 GNRL entries were not yet parsed.
- GREEN gate: `9b3dc0d` implemented BA2 GNRL parser/reader dispatch and verified focused BA2 detector/metadata/lookup tests passed.
- REFACTOR gate: not needed; no behavior-preserving cleanup commit was required after GREEN.

## Known Stubs

None.

## Threat Flags

None. New parser and lookup trust-boundary surface is covered by the plan's T-05-05 and T-05-06 mitigations.

## Verification

- `cmake --build --preset windows-msvc-debug-static` — passed.
- `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_metadata|ba2_gnrl_lookup" --output-on-failure` — failed in RED as expected before Task 2 because BA2 entries were still empty/missing.
- `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_detector|ba2_gnrl_metadata|ba2_gnrl_lookup" --output-on-failure` — passed after Task 2 and again at plan completion.
- Acceptance greps for `[ba2_gnrl_metadata]`, `[ba2_gnrl_lookup]`, `lookup_variants`, `payload_offset`, `record_flags`, raw/deflate/lz4_block coverage, `FileTableOffset`, `PackedSize`, `BAADF00D`, `normalize_archive_path`, BA2 helper names, and BA2 archive dispatch — passed.
- `git -C TES5Edit status --short` — no output.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for Plan 05-04 to implement BA2 GNRL raw, deflate, and Starfield raw-LZ4-block extraction using parsed entry metadata.
- No blockers.

## Self-Check: PASSED

- Found summary and key BA2 parser/reader files on disk.
- Found task commits `75cf55a` and `9b3dc0d` in git history.

---
*Phase: 05-ba2-gnrl-read-extract*
*Completed: 2026-05-08*
