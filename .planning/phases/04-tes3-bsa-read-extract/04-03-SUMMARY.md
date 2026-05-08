---
phase: 04-tes3-bsa-read-extract
plan: 03
subsystem: bsa-reader
tags: [cxx20, catch2, tes3, bsa, parser, hashes, lookup]

requires:
  - phase: 04-tes3-bsa-read-extract
    provides: TES3 fixture contracts, archive_hash metadata, and TES3 byte detection from Plans 04-01 and 04-02
provides:
  - TES3 BSA parser for metadata, entries, names, hashes, and archive-absolute payload offsets
  - Strict TES3 hash/order/collision/canonical-path validation for open-time malformed rejection
  - TES3 entries/find/contains helper dispatch through archive_reader
affects: [tes3-reader, bsa-parser, archive-reader, bethesda-hash]

tech-stack:
  added: []
  patterns:
    - Private TES3 parser mirrors TES4 checked-read patterns without exposing new public parser APIs
    - TES3 hash sort helpers make low32/high32 ordering explicit and testable
    - archive_reader dispatches listing and lookup by parsed archive variant

key-files:
  created:
    - src/formats/bsa/tes3_bsa_parser.hpp
    - src/formats/bsa/tes3_bsa_parser.cpp
    - src/formats/bsa/tes3_bsa_reader.hpp
    - src/formats/bsa/tes3_bsa_reader.cpp
    - .planning/phases/04-tes3-bsa-read-extract/04-03-SUMMARY.md
  modified:
    - CMakeLists.txt
    - src/archive.cpp
    - src/detail/bethesda_hash.hpp
    - src/detail/bethesda_hash.cpp
    - tests/unit/bethesda_hash_tests.cpp
    - tests/unit/tes3_bsa_reader_tests.cpp

key-decisions:
  - "TES3 parser stores archive-absolute payload offsets only; raw TES3 data-section offsets are converted and validated during parse."
  - "TES3 hash ordering is represented with explicit low32/high32 helper functions to avoid comparator ambiguity."
  - "TES3 listing and lookup use dedicated private helper names while preserving the established archive_reader public facade."

patterns-established:
  - "TES3 parser validates untrusted table/name/hash/payload spans before materializing reader state."
  - "TDD RED commits can add private-helper contract tests when previous public RED tests already pass through shared fallback behavior."

requirements-completed: [BSA-04, BSA-08]

duration: 6 min
completed: 2026-05-08
---

# Phase 04 Plan 03: TES3 Parser, Metadata, and Lookup Summary

**TES3 BSA parser with archive-absolute payload metadata, strict stored-hash validation, and normalized entries/find/contains dispatch through archive_reader.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-05-08T12:10:05Z
- **Completed:** 2026-05-08T12:16:04Z
- **Tasks:** 3/3
- **Files modified:** 10

## Accomplishments

- Added a private TES3 parser that reads the header, file records, name offsets, zstring names, hash records, and data-section start with checked bounds.
- Converted TES3 raw data-section-relative offsets to archive-absolute `entry_metadata::payload_offset` values during parse validation and documented the TES5Edit/UESP compatibility rule near the conversion.
- Validated stored TES3 hashes against parsed archive names, rejected duplicate/colliding hashes, rejected unsorted hash records, rejected duplicate canonical paths, and rejected overlapping/out-of-bounds payload spans.
- Added TES3 reader helpers for entries, find, and contains, then dispatched `archive_reader` listing/lookup calls by archive variant without changing public API shape.

## Task Commits

Each task was committed atomically with TDD gates where new tests were required:

1. **Task 1: Implement TES3 parser contracts and archive metadata** - `d065b3c` (feat)
2. **Task 2 RED: TES3 hash sort helper tests** - `99796f8` (test)
3. **Task 2 GREEN: Validate TES3 hashes, names, and entry metadata** - `52de321` (feat)
4. **Task 3 RED: TES3 lookup helper test** - `1b13380` (test)
5. **Task 3 GREEN: Add TES3 entries/find/contains delegation** - `5c26b1b` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `src/formats/bsa/tes3_bsa_parser.hpp` - Private parser API and parsed TES3 archive result shape.
- `src/formats/bsa/tes3_bsa_parser.cpp` - TES3 table/name/hash parsing, strict validation, and metadata materialization.
- `src/formats/bsa/tes3_bsa_reader.hpp` - Private TES3 entries/find/contains helper declarations.
- `src/formats/bsa/tes3_bsa_reader.cpp` - TES3 normalized lookup helper implementation over canonical sorted entries.
- `src/archive.cpp` - Routes TES3 open to the TES3 parser and routes entries/find/contains by archive variant.
- `src/detail/bethesda_hash.hpp` / `src/detail/bethesda_hash.cpp` - Adds explicit TES3 low32/high32/sort-key helper functions.
- `tests/unit/bethesda_hash_tests.cpp` - Adds RED/GREEN coverage for TES3 sort helper halves.
- `tests/unit/tes3_bsa_reader_tests.cpp` - Adds direct TES3 lookup-helper contract coverage.
- `CMakeLists.txt` - Registers TES3 parser and reader implementation sources.

## Decisions Made

- TES3 parser materializes only archive-absolute payload offsets in runtime metadata; raw offsets remain parser-local and fixture-manifest-only.
- TES3 hash validation uses `detail::hash_tes3(parsed_archive_name)` before original spelling is normalized for public display/lookup.
- Dedicated TES3 lookup helpers were added instead of broad neutral helper renaming to minimize TES4-family regression risk.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Task 3's original public lookup gate passed unexpectedly because TES3 entries were already sorted and the existing TES4 helper happened to normalize/find them correctly. A private TES3 helper contract test was added as the RED gate before implementing the dedicated helper and dispatch.

## Verification

- RED Task 1: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L tes3_bsa_metadata --output-on-failure` failed as expected before parser implementation because TES3 open returned no value.
- Task 1 GREEN: `ctest --preset windows-msvc-debug-static -L tes3_bsa_metadata --output-on-failure` — passed.
- Task 1 acceptance greps for `parse_tes3_bsa_archive_file`, the D-10 TES5Edit comment, and `data_section_start` — passed.
- Task 2 RED: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L bethesda_hash --output-on-failure` failed as expected because TES3 hash sort helpers did not exist.
- Task 2 GREEN: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "tes3_bsa_entries|tes3_bsa_malformed|bethesda_hash" --output-on-failure` — passed.
- Task 2 acceptance greps for `archive_hash`, `hash_tes3`, and `tes3_hash_low32|tes3_hash_sort_key`; focused TES3 entries gate — passed.
- Task 3 RED: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L tes3_bsa_lookup --output-on-failure` failed as expected because the private TES3 reader helper header did not exist.
- Task 3 GREEN: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "tes3_bsa_lookup|tes3_bsa_entries|tes4_bsa|public_include_boundary" --output-on-failure` — passed.
- Task 3 acceptance greps for `find_tes3_bsa_entry` and `normalize_archive_path`; `ctest -L tes3_bsa_lookup`; `ctest -L tes4_bsa` — passed.
- Plan quick gate: `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` — passed 63/63 runnable tests, with the expected opt-in local game fixture test skipped.
- Plan full gate: `ctest --preset windows-msvc-debug-static --output-on-failure` — passed 64/64 runnable tests, with the expected opt-in local game fixture test skipped.
- `git -C TES5Edit status --short` — produced no output.

## TDD Gate Compliance

- RED gate present: `99796f8` and `1b13380` for new helper contracts; prior TES3 public RED tests from Plan 04-01 supplied the parser metadata/listing/lookup expectations.
- GREEN gate present after RED: `d065b3c`, `52de321`, and `5c26b1b`.
- REFACTOR gate: not needed; no behavior-neutral cleanup was made after GREEN.

## Known Stubs

None. Stub scan found no TODO/FIXME/placeholder text in created or modified implementation and test files; false-positive matches were existing initialized variables unrelated to UI/data stubs.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 04-04. TES3 open/list/lookup metadata is now implemented and strict malformed table/hash/path validation is in place, while raw extraction can build on the archive-absolute offsets already proven here.

## Self-Check: PASSED

- Found created SUMMARY, TES3 parser header/source, and TES3 reader header/source.
- Verified task commits exist: `d065b3c`, `99796f8`, `52de321`, `1b13380`, and `5c26b1b`.

---
*Phase: 04-tes3-bsa-read-extract*
*Completed: 2026-05-08*
