---
phase: 05-ba2-gnrl-read-extract
plan: 04
subsystem: archive-reader
tags: [cpp20, ba2, gnrl, extraction, deflate, lz4-block, tdd]

requires:
  - phase: 05-ba2-gnrl-read-extract
    provides: BA2 GNRL fixtures, detector, parser, metadata listing, and lookup from Plans 05-01 through 05-03
provides:
  - BA2 GNRL sink-first extraction for raw, deflate, and Starfield v3 raw LZ4-block payloads
  - archive_reader BA2 extraction dispatch through parsed entry metadata
  - Manifest-backed byte comparisons through extract and extract_bytes
affects: [phase-05-ba2-malformed, phase-06-ba2-dx10, phase-08-ba2-writer, extraction-api]

tech-stack:
  added: []
  patterns: [sink-first BA2 extraction, exact-size codec routing, metadata-driven compression dispatch]

key-files:
  created: []
  modified:
    - tests/unit/ba2_gnrl_reader_tests.cpp
    - src/formats/ba2/ba2_gnrl_reader.hpp
    - src/formats/ba2/ba2_gnrl_reader.cpp
    - src/archive.cpp

key-decisions:
  - "BA2 GNRL extraction routes exclusively by parsed entry_metadata::compression, with deflate and raw LZ4 block decoded through detail::decompress_payload_exact."
  - "archive_reader::extract dispatches BA2 entries to the BA2 helper before TES4-family stored-payload buffering so BA2 extraction remains sink-first and metadata-driven."

patterns-established:
  - "Format-specific extraction helpers own host-file payload reads, chunked sink writes, partial-write errors, and codec selection for their archive family."

requirements-completed: [GNRL-05, GNRL-06, GNRL-07]

duration: 2min
completed: 2026-05-08
---

# Phase 05 Plan 04: BA2 GNRL Extraction Summary

**Sink-first BA2 GNRL extraction with exact-size deflate and Starfield v3 raw LZ4-block decoding through archive_reader.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-08T23:02:20Z
- **Completed:** 2026-05-08T23:04:18Z
- **Tasks:** 2 completed
- **Files modified:** 4

## Accomplishments

- Added RED BA2 GNRL extraction tests covering manifest expected bytes through both `extract(path, sink)` and `extract_bytes(path)` for FO4, Starfield v2, and Starfield v3 fixtures.
- Implemented `extract_ba2_gnrl_payload` with host-archive reads, 64 KiB chunked sink writes, partial sink detection, and exact-size deflate/raw LZ4-block decompression.
- Routed `archive_reader::extract` for BA2 entries through the BA2 helper so extraction follows parsed metadata rather than TES4-family BSA buffering or filename inference.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add BA2 extraction RED tests** - `cd474ec` (test)
2. **Task 2: Implement BA2 extraction helper and archive_reader dispatch** - `d0bbab8` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/ba2_gnrl_reader_tests.cpp` - Adds `[ba2_gnrl_extract]` tests for manifest bytes, zero-byte raw entries, deflate entries, raw LZ4-block entries, `extract_bytes`, and partial sink failures.
- `src/formats/ba2/ba2_gnrl_reader.hpp` - Declares the private BA2 GNRL extraction helper with Doxygen documentation.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - Implements BA2 raw streaming, exact-size deflate/raw LZ4-block decode paths, and partial sink `io_error` handling.
- `src/archive.cpp` - Dispatches BA2 extraction through `extract_ba2_gnrl_payload` before generic TES4-family stored-payload extraction.

## Decisions Made

- BA2 GNRL compressed extraction uses only `entry_metadata::compression` and maps `deflate` / `lz4_block` to `detail::compression_method::deflate` / `detail::compression_method::lz4_block`.
- BA2 GNRL `lz4_frame` metadata fails closed as `format_error`; Phase 5 supports Starfield v3 raw LZ4 blocks, not SSE-style LZ4 frames.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## TDD Gate Compliance

- RED gate: `cd474ec` added BA2 GNRL extraction tests and verified the build failed because `extract_ba2_gnrl_payload` did not exist yet.
- GREEN gate: `d0bbab8` implemented BA2 extraction and verified focused extraction and metadata tests passed.
- REFACTOR gate: not needed; no behavior-preserving cleanup commit was required after GREEN.

## Known Stubs

None. The zero-byte raw fixture entry is intentional archive test coverage, not a stub.

## Threat Flags

None. The extraction trust boundaries are covered by T-05-07 and T-05-08 in the plan threat model.

## Verification

- `cmake --build --preset windows-msvc-debug-static` during RED — failed as expected because `extract_ba2_gnrl_payload` was undeclared.
- `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "ba2_gnrl_extract|ba2_gnrl_metadata" --output-on-failure` — passed after Task 2.
- Acceptance greps for `[ba2_gnrl_extract]`, `extract_bytes`, `partial_sink`, `lz4_block`, `bytes_hex`, `extract_ba2_gnrl_payload`, `decompress_payload_exact`, `compression_method::deflate`, `compression_method::lz4_block`, partial sink `io_error`, and BA2 archive dispatch — passed.
- `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_extract|ba2_gnrl_metadata|ba2_gnrl_lookup" --output-on-failure` — passed.
- `git -C TES5Edit status --short` — no output.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for Plan 05-05 malformed BA2 extraction coverage to validate corrupt compressed payloads, exact-size mismatches, and other failure cases.
- No blockers.

## Self-Check: PASSED

- Found key modified files on disk: `tests/unit/ba2_gnrl_reader_tests.cpp`, `src/formats/ba2/ba2_gnrl_reader.hpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, and `src/archive.cpp`.
- Found task commits `cd474ec` and `d0bbab8` in git history.

---
*Phase: 05-ba2-gnrl-read-extract*
*Completed: 2026-05-08*
