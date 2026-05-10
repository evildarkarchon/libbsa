---
phase: 03-format-detection-and-tes4-family-bsa-read-extract
plan: 06
subsystem: archive-reader
tags: [cpp20, bsa, tes4, extraction, deflate, lz4-frame, catch2]

requires:
  - phase: 03-format-detection-and-tes4-family-bsa-read-extract
    provides: TES4-family detection, metadata parsing, lookup normalization, and generated fixtures
provides:
  - TES4-family sink-first extraction by archive path
  - Raw, deflate, and LZ4-frame payload routing with exact-size validation
  - Embedded-name prefix skipping for consumer-visible payload bytes
  - Bounded extract_bytes convenience API backed by sink extraction
  - Malformed compression, missing path, invalid path, and partial sink coverage
affects: [phase-04, bsa-readers, extraction-api, compression-routing]

tech-stack:
  added: []
  patterns:
    - Path-first archive extraction delegates from public archive_reader to TES4-family reader helpers.
    - Compressed BSA extraction consumes a per-entry uncompressed-size prefix before exact-size codec routing.
    - In-memory convenience extraction is implemented as a bounded payload_sink adapter.

key-files:
  created:
    - .planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-06-SUMMARY.md
  modified:
    - src/archive.cpp
    - src/formats/bsa/tes4_bsa_reader.hpp
    - src/formats/bsa/tes4_bsa_reader.cpp
    - tests/unit/tes4_bsa_reader_tests.cpp

key-decisions:
  - "TES4-family extraction remains path-first on archive_reader and returns not_found for valid missing paths while preserving invalid_argument for malformed archive paths."
  - "extract_bytes is a convenience adapter over extract(path, sink), not a separate decoding path, so embedded-name and compression behavior stay identical."

patterns-established:
  - "Selected-entry payload slices are validated against parser metadata before raw copy or compression decode."
  - "Caller sink partial writes are treated as io_error and never reported as partial success."

requirements-completed: [BSA-01, BSA-02, BSA-03, BSA-06, BSA-07]

duration: 24min
completed: 2026-05-08
---

# Phase 03 Plan 06: TES4-family Extraction Summary

**TES4-family BSA extraction by path with raw, deflate, LZ4-frame, embedded-name, bounded byte, and fail-closed sink/error handling**

## Performance

- **Duration:** 24 min
- **Started:** 2026-05-08T07:41:00Z
- **Completed:** 2026-05-08T08:05:26Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments

- Added path-first TES4-family extraction through `archive_reader::extract(path, payload_sink&)`.
- Routed v103/v104 compressed payloads through exact-size deflate and v105 compressed payloads through exact-size LZ4-frame decoding.
- Skipped embedded BSA names before exposing consumer-visible bytes for v104/v105 fixtures.
- Added `extract_bytes(path)` as a bounded vector sink convenience helper backed by the same extraction code path.
- Verified malformed compressed payloads, compressed size mismatches, missing paths, invalid paths, and partial sink writes fail with stable error categories.

## Task Commits

Each implementation task was committed atomically. TDD tasks include RED then GREEN commits:

1. **Task 1 RED: Sink extraction tests** - `e59d78b` (test)
2. **Task 1 GREEN: Sink extraction implementation** - `0824e91` (feat)
3. **Task 2 RED: extract_bytes tests** - `01cc399` (test)
4. **Task 2 GREEN: bounded extract_bytes implementation** - `0d4227f` (feat)
5. **Task 3: Final Phase 3 verification** - no code changes; verification passed against committed state

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `src/archive.cpp` - Stores archive bytes in reader state, delegates sink extraction, and implements bounded vector-backed `extract_bytes`.
- `src/formats/bsa/tes4_bsa_reader.hpp` - Declares TES4-family path extraction helper.
- `src/formats/bsa/tes4_bsa_reader.cpp` - Validates selected payload spans, skips embedded-name prefixes, routes compression, and checks sink writes.
- `tests/unit/tes4_bsa_reader_tests.cpp` - Adds fixture-backed extraction, embedded-name, malformed compression, missing/invalid path, and partial sink tests.
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-06-SUMMARY.md` - Records execution outcome.

## Decisions Made

- TES4-family extraction remains path-first on `archive_reader`; callers do not need entry handles for extraction.
- `extract_bytes` reuses the public sink extraction path through a local vector sink adapter so byte convenience behavior cannot drift from streaming extraction.
- Existing open-state archive bytes are used as the extraction backing store for this phase; extraction validates and slices only the selected entry span before decoding or copying.

## Deviations from Plan

None - plan executed as written.

## Issues Encountered

- The final verification PowerShell snippet initially had shell quoting stripped by the outer PowerShell invocation. CTest had already passed; the TES5Edit read-only status gate was rerun with single-quoted PowerShell command text and passed with no output.

## TDD Gate Compliance

- RED gate present: `e59d78b`, `01cc399`
- GREEN gate present after RED: `0824e91`, `0d4227f`
- Refactor gate: not needed; no behavior-neutral cleanup changes were made after GREEN.

## Verification

- `cmake --build --preset windows-msvc-debug-static` — passed
- `ctest --preset windows-msvc-debug-static --output-on-failure` — passed, 49 tests passed and 1 opt-in local fixture test skipped
- `pwsh -NoProfile -Command '$status = git -C TES5Edit status --short; if ($status) { throw "TES5Edit has unexpected changes: $status" }'` — passed

## Known Stubs

None found in files created or modified by this plan.

## Threat Flags

None. The new extraction trust surfaces were already represented in the plan threat model and covered by span validation, exact-size decompression, and partial sink failure tests.

## Self-Check: PASSED

- Verified key files exist: `src/archive.cpp`, `src/formats/bsa/tes4_bsa_reader.hpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, and this summary.
- Verified task commits exist: `e59d78b`, `0824e91`, `01cc399`, `0d4227f`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 3 now has a green consumer-visible TES4-family read/extract slice for detection, metadata, listing, lookup, extraction, embedded names, malformed compression, and public include boundaries.
- Later reader phases can build on the same path-first extraction and compression-routing patterns for TES3/BA2 variants.

---
*Phase: 03-format-detection-and-tes4-family-bsa-read-extract*
*Completed: 2026-05-08*
