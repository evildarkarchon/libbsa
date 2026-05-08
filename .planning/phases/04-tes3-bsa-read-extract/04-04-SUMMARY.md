---
phase: 04-tes3-bsa-read-extract
plan: 04
subsystem: bsa-reader
tags: [cxx20, catch2, tes3, bsa, extraction, malformed-validation]

requires:
  - phase: 04-tes3-bsa-read-extract
    provides: TES3 parser metadata, archive-absolute offsets, and lookup dispatch from Plan 04-03
provides:
  - TES3 raw extraction dispatch through archive_reader::extract and extract_bytes
  - Raw-only TES3 payload helper with sink partial-write enforcement
  - Strict malformed TES3 archive-path error mapping during open validation
  - Final Phase 4 quick/full validation and TES5Edit read-only gate
affects: [tes3-reader, archive-reader, bsa-extraction, malformed-validation, phase-05]

tech-stack:
  added: []
  patterns:
    - Variant-aware archive_reader extraction dispatch selects TES3 or TES4-family helpers after shared lookup.
    - TES3 extraction rejects non-raw metadata before any codec routing and writes bounded chunks to caller sinks.
    - Malformed archive-owned paths map to format_error while caller path arguments keep invalid_argument semantics.

key-files:
  created:
    - .planning/phases/04-tes3-bsa-read-extract/04-04-SUMMARY.md
  modified:
    - src/archive.cpp
    - src/formats/bsa/tes3_bsa_reader.hpp
    - src/formats/bsa/tes3_bsa_reader.cpp
    - src/formats/bsa/tes3_bsa_parser.cpp
    - tests/unit/tes3_bsa_reader_tests.cpp

key-decisions:
  - "TES3 extraction uses a dedicated raw-only helper instead of reusing TES4-family compression routing, so malformed non-raw TES3 metadata fails with format_error."
  - "Invalid archive-owned TES3 paths are malformed input and therefore map to format_error during open, distinct from invalid_argument for caller-supplied lookup/extraction paths."

patterns-established:
  - "archive_reader::extract dispatches by parsed archive variant while extract_bytes remains a sink-backed convenience wrapper."
  - "TES3 malformed tests can mutate generated fixtures dynamically when a parser-only edge case does not require committed opaque bytes."

requirements-completed: [BSA-04, BSA-08]

duration: 5 min
completed: 2026-05-08
---

# Phase 04 Plan 04: TES3 Extraction and Fail-Closed Validation Summary

**TES3 raw archive extraction through the public reader facade with data-section-relative offset proof, raw-only codec isolation, malformed fail-closed validation, and green Phase 4 regression gates.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-08T12:19:31Z
- **Completed:** 2026-05-08T12:24:21Z
- **Tasks:** 3/3
- **Files modified:** 5

## Accomplishments

- Added `extract_tes3_bsa_payload` as the TES3-specific raw payload helper, enforcing `entry_compression::none`, exact stored payload size, chunked sink writes, and partial-write `io_error` behavior.
- Updated `archive_reader::extract` to dispatch lookup and payload extraction by parsed archive variant while keeping `extract_bytes` backed by the public sink extraction path.
- Added TES3 extraction tests for direct helper behavior, raw-only failure, partial sink writes, fixture payload bytes, zero-byte entries via the fixture manifest, and archive-byte proof at parser-materialized absolute offsets.
- Hardened TES3 parser malformed handling so invalid archive-owned paths fail as `format_error` during open while caller-supplied invalid paths still return `invalid_argument` through lookup/extraction APIs.
- Ran final Phase 4 validation: build, quick unit CTest label, full CTest preset including package smoke, TES5Edit cleanliness, and Phase 4 tag coverage grep.

## Task Commits

Each TDD task was committed atomically:

1. **Task 1 RED: TES3 raw extraction helper tests** - `9d38928` (test)
2. **Task 1 GREEN: TES3 raw extraction routing** - `aa7e1dd` (feat)
3. **Task 2 RED: Invalid TES3 archive-name malformed test** - `17880df` (test)
4. **Task 2 GREEN: Invalid archive names map to format_error** - `2ff8baf` (fix)
5. **Task 3: Final Phase 4 verification** - no code changes; verification passed against committed state

**Plan metadata:** pending final docs commit

_Note: TDD tasks produced RED then GREEN commits._

## Files Created/Modified

- `src/archive.cpp` - Routes extraction lookup/helper selection by archive variant and uses neutral BSA payload-span errors for shared payload reads.
- `src/formats/bsa/tes3_bsa_reader.hpp` - Declares the TES3 raw payload extraction helper.
- `src/formats/bsa/tes3_bsa_reader.cpp` - Implements TES3 metadata copies, lookup/contains, raw-only chunked payload streaming, and partial sink enforcement.
- `src/formats/bsa/tes3_bsa_parser.cpp` - Maps invalid archive-owned TES3 paths to `format_error` during parser materialization.
- `tests/unit/tes3_bsa_reader_tests.cpp` - Adds TES3 raw helper, partial sink, raw-only, and dynamic invalid archive-name malformed coverage.

## Decisions Made

- TES3 extraction deliberately uses a dedicated helper rather than TES4-family `compression_router` paths because TES3 BSA read scope is raw/uncompressed and non-raw metadata is malformed.
- Parser-owned invalid archive paths are archive format failures (`format_error`), while public API caller path mistakes remain `invalid_argument` through `find`, `contains`, `extract`, and `extract_bytes`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Added a stronger RED test when the original TES3 extraction gate already passed**
- **Found during:** Task 1 (Implement TES3 sink-first raw extraction and extract_bytes routing)
- **Issue:** The existing public TES3 extraction test passed through the TES4 raw helper fallback because uncompressed entries and sorted metadata made the shared path accidentally correct.
- **Fix:** Added a direct TES3 helper contract test requiring `extract_tes3_bsa_payload`, raw-only rejection, and partial sink `io_error`, then implemented the helper and variant-aware dispatch.
- **Files modified:** `tests/unit/tes3_bsa_reader_tests.cpp`, `src/archive.cpp`, `src/formats/bsa/tes3_bsa_reader.hpp`, `src/formats/bsa/tes3_bsa_reader.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L tes3_bsa_extract --output-on-failure`
- **Committed in:** `9d38928`, `aa7e1dd`

**2. [Rule 1 - Bug] Mapped invalid stored TES3 archive names to format_error**
- **Found during:** Task 2 (Enforce strict malformed TES3 span and overlap validation)
- **Issue:** Dynamically mutated TES3 archive bytes with a valid matching hash but invalid rooted archive path returned `invalid_argument`, which is appropriate for caller input but not malformed archive bytes.
- **Fix:** Converted parser path-normalization failures inside TES3 entry materialization to `error_code::format_error`.
- **Files modified:** `tests/unit/tes3_bsa_reader_tests.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure`
- **Committed in:** `17880df`, `2ff8baf`

---

**Total deviations:** 2 auto-fixed (2 bugs)
**Impact on plan:** Both fixes tightened planned correctness and fail-closed behavior without expanding public API scope or changing TES4-family behavior.

## Issues Encountered

- The pre-existing TES3 extraction and malformed labels were already green at the start of their TDD tasks. Following the TDD fail-fast rule, stricter RED tests were added before implementing or adjusting code.

## Verification

- Task 1 RED: `cmake --build --preset windows-msvc-debug-static` failed as expected because `extract_tes3_bsa_payload` did not exist.
- Task 1 GREEN: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L tes3_bsa_extract --output-on-failure` — passed 2/2 tests.
- Task 1 acceptance greps for `extract_tes3_bsa_payload`, `entry_compression::none`, absence of `decompress_payload_exact|compression_router` in `tes3_bsa_reader.cpp`, and `ctest -L tes3_bsa_extract` — passed.
- Task 2 RED: `ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure` failed as expected for invalid archive-owned path returning `invalid_argument` instead of `format_error`.
- Task 2 GREEN: `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure` — passed 2/2 tests.
- Task 2 acceptance greps for `tes3_raw_offset_absolute_regression`, `overlap|payload span|name span`, absence of TES3 diagnostic `message` assertions, and `ctest -L tes3_bsa_malformed` — passed.
- Final quick gate: `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` — passed 65/65 runnable tests, with the expected opt-in local game fixture test skipped.
- Final full gate: `ctest --preset windows-msvc-debug-static --output-on-failure` — passed 66/66 runnable tests, with the expected opt-in local game fixture test skipped.
- `pwsh -NoProfile -Command '$status = git -C TES5Edit status --short; if ($status) { throw "TES5Edit has unexpected changes: $status" }'` — passed with no output.
- Phase 4 tag coverage grep for `tes3_bsa_metadata|tes3_bsa_entries|tes3_bsa_lookup|tes3_bsa_extract|tes3_bsa_malformed` — passed.

## TDD Gate Compliance

- RED gate present: `9d38928` and `17880df`.
- GREEN gate present after RED: `aa7e1dd` and `2ff8baf`.
- REFACTOR gate: not needed; no behavior-neutral cleanup was made after GREEN.

## Known Stubs

None. Stub scan found no TODO/FIXME/placeholder text or unwired empty UI/data placeholders in files modified by this plan.

## Threat Flags

None. The extraction, path-input, sink, malformed-archive, and TES5Edit surfaces were already represented in the plan threat model and covered by raw-only routing, parser validation, sink-write checks, and repository cleanliness verification.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 4 is complete. Consumers can open, list, query, and extract generated TES3/Morrowind BSA archives through `archive_reader`; malformed TES3 inputs fail closed; TES4-family regressions and public include boundaries remain green; the project is ready for Phase 5 BA2 GNRL read/extract planning.

## Self-Check: PASSED

- Found created SUMMARY and all key modified files.
- Verified task commits exist: `9d38928`, `aa7e1dd`, `17880df`, and `2ff8baf`.
- Verified final quick/full CTest gates and TES5Edit cleanliness gate passed.

---
*Phase: 04-tes3-bsa-read-extract*
*Completed: 2026-05-08*
