---
phase: 16-parser-and-preparer-seam-extraction
plan: 01
subsystem: parser
tags: [tes4-bsa, parser, seam, tdd, catch2, ctest]
requires:
  - phase: 15-reader-backend-dispatch-cleanup
    provides: open-time reader backend separation and source-policy patterns
provides:
  - private TES4 raw table bundle seam for checked header/table state, folder blocks, and file-name strings
  - private TES4 payload descriptor seam for compression, embedded-name prefix, raw-size, and payload-span rules
  - focused parser_preparer_seam regression coverage for TES4 table and payload descriptor behavior
affects: [src/formats/bsa, tests/unit, CMakeLists.txt, tests/CMakeLists.txt]
tech-stack:
  added: []
  patterns: [private format seams, direct internal seam tests, TDD RED-GREEN-REFACTOR]
key-files:
  created: [src/formats/bsa/tes4_bsa_table.hpp, src/formats/bsa/tes4_bsa_table.cpp, src/formats/bsa/tes4_bsa_payload_descriptor.hpp, src/formats/bsa/tes4_bsa_payload_descriptor.cpp, tests/unit/tes4_bsa_parser_seam_tests.cpp]
  modified: [src/formats/bsa/tes4_bsa_parser.cpp, CMakeLists.txt, tests/CMakeLists.txt]
key-decisions:
  - "TES4 table/header parsing now lives in a private raw table seam that returns checked raw records and stored names only."
  - "TES4 payload prefix, raw-size, compression, and span validation now lives in a private payload descriptor seam."
  - "Public TES4 parser entrypoints remain unchanged while parse_tes4_bsa_archive_impl coordinates seams and entry materialization."
patterns-established:
  - "Direct internal seam tests can include src/formats private headers to prove parser responsibilities without public API expansion."
  - "File-backed parser entrypoints size metadata through the raw table header seam, then parse the fully loaded table through the same coordinator path."
requirements-completed: [REFA-01]
duration: 8m
completed: 2026-05-14
---

# Phase 16 Plan 01: TES4 Raw Table and Payload Descriptor Seams Summary

**TES4-family BSA parsing now routes checked raw table parsing and payload descriptor rules through private seams with direct Catch2 regression coverage.**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-14T23:03:03Z
- **Completed:** 2026-05-14T23:11:26Z
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- Added `tes4_bsa_table` as a private raw table bundle seam for fixed header parsing, metadata table sizing, folder records/blocks, and file-name strings without canonical path or `entry_metadata` construction.
- Added `tes4_bsa_payload_descriptor` as a private payload seam for stored-size masking, metadata-overlap rejection, embedded-name prefixes, raw-size prefix reads, and TES4 v103/v104/v105 compression interpretation.
- Added focused `[parser_preparer_seam]` Catch2 coverage that directly exercises the new raw table and payload descriptor seams while preserving existing TES4 reader and malformed behavior.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED — add focused TES4 seam regression tests** - `e0b8d2d` (test)
2. **Task 2: GREEN — extract raw table and payload descriptor seams** - `366a3a0` (feat)
3. **Task 3: REFACTOR — tighten TES4 coordinator and preserve public behavior** - `29290ed` (refactor)

_Note: Plan metadata is committed separately after state and roadmap updates._

## TDD Gate Compliance

- **RED:** `e0b8d2d` added the focused seam tests and failed before implementation with unresolved private seam headers.
- **GREEN:** `366a3a0` added the table and payload descriptor seams and made focused plus affected TES4 tests pass.
- **REFACTOR:** `29290ed` removed duplicate file-backed header sizing logic from the parser coordinator while focused and affected TES4 tests continued to pass.

## Validation

- **RED command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` failed before implementation because `formats/bsa/tes4_bsa_payload_descriptor.hpp` did not exist.
- **GREEN focused command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` passed with 2/2 tests.
- **GREEN affected command:** `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|parser_preparer_seam"` passed with 60/60 tests.
- **REFACTOR focused command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` passed with 2/2 tests.
- **Plan final command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|parser_preparer_seam"` passed with 60/60 tests.

## Files Created/Modified

- `src/formats/bsa/tes4_bsa_table.hpp` - declares the private raw table data structures and table/header sizing seam.
- `src/formats/bsa/tes4_bsa_table.cpp` - implements checked header, table size, folder record/block, and file-name parsing.
- `src/formats/bsa/tes4_bsa_payload_descriptor.hpp` - declares the private payload descriptor type and template helper for callback-backed prefix reads.
- `src/formats/bsa/tes4_bsa_payload_descriptor.cpp` - implements compression interpretation and payload-span/metadata-overlap validation.
- `src/formats/bsa/tes4_bsa_parser.cpp` - becomes a thinner coordinator that materializes public entries from raw table and payload descriptor seam outputs.
- `tests/unit/tes4_bsa_parser_seam_tests.cpp` - adds direct table and payload descriptor seam regression tests with `[parser_preparer_seam]` labels.
- `CMakeLists.txt` - registers the new private TES4 seam implementation files.
- `tests/CMakeLists.txt` - registers the new focused seam test file.

## Decisions Made

- Kept both new seams under `src/formats/bsa/` and did not add or modify public headers under `include/libbsa/`.
- Kept duplicate canonical path rejection, hash validation, canonical path construction, final sorting, and `archive_metadata` construction in the parser coordinator.
- Kept the file-backed parser path on the Phase 13 `detail::host_file_path` / `read_file_bytes_at` boundary while sharing header sizing through the raw table seam.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes; the implementation stayed within the locked TES4 parser seam boundary.

## Issues Encountered

- Initial RED test expected specific fixture spellings and sizes incorrectly after the new seam was implemented; the assertions were corrected to match the generated fixture's stored table spelling and manifest-derived payload facts before the GREEN commit.

## Known Stubs

None.

## Threat Flags

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for 16-02: the TES4 parser hotspot now has private seams and direct regression coverage, so the next plan can focus on BA2 DX10 preparer seams without reopening TES4 parser responsibilities.
- Phase 17 dedupe and DX10 temp-lifecycle requirements remain out of scope and pending.

## Self-Check: PASSED

- FOUND: `src/formats/bsa/tes4_bsa_table.hpp`
- FOUND: `src/formats/bsa/tes4_bsa_table.cpp`
- FOUND: `src/formats/bsa/tes4_bsa_payload_descriptor.hpp`
- FOUND: `src/formats/bsa/tes4_bsa_payload_descriptor.cpp`
- FOUND: `tests/unit/tes4_bsa_parser_seam_tests.cpp`
- FOUND COMMIT: `e0b8d2d`
- FOUND COMMIT: `366a3a0`
- FOUND COMMIT: `29290ed`

---
*Phase: 16-parser-and-preparer-seam-extraction*
*Completed: 2026-05-14*
