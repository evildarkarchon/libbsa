---
phase: 06-ba2-gnrl-read-and-extract
plan: 04
subsystem: ba2-extraction
tags: [cpp20, ba2, starfield, lz4-block, deflate, tdd]

requires:
  - phase: 06-ba2-gnrl-read-and-extract
    provides: BA2 public API, bounded GNRL metadata parser, and raw/deflate extraction from plans 06-01 through 06-03
provides:
  - Starfield BA2 GNRL v3 header parsing with archive compression_method metadata
  - Starfield v3 method-3 raw LZ4-block extraction route for compressed entries
  - Fixture coverage proving raw method-3 entries stay raw, default compressed entries use deflate, and unsupported codec routes fail without sink writes
affects: [phase-07-ba2-dds, phase-10-ba2-writer, phase-11-compat-validation, compression-policy]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN gate commits for Starfield v3 BA2 codec routing
    - BA2 parser resolves public entry compression state before extraction reaches the central codec dispatcher
    - Unsupported Starfield v3 compression methods map to structured unsupported extraction failures without fallback codecs

key-files:
  created:
    - .planning/phases/06-ba2-gnrl-read-and-extract/06-04-SUMMARY.md
  modified:
    - tests/ba2_reader_tests.cpp
    - src/ba2_reader.cpp

key-decisions:
  - "Represented Starfield BA2 v3 method-3 compressed GNRL entries as compression_state::lz4_block while keeping raw entries compression_state::raw regardless of archive compression_method."
  - "Mapped unsupported nonzero Starfield v3 compression methods to compression_state::unknown so extraction fails through the existing dispatcher before sink writes."

patterns-established:
  - "BA2 v3 fixture builders always emit the 36-byte header and set CompressionMethod at offset 32."
  - "BA2 reader code names LZ4 block routing through compression_state only and does not call or reference LZ4 frame APIs."

requirements-completed: [BA2-03]

duration: 2min
completed: 2026-05-06
---

# Phase 06 Plan 04: Starfield v3 BA2 Compression Routing Summary

**Starfield BA2 GNRL v3 extraction routes compressed method-3 payloads through raw LZ4 blocks while preserving raw and deflate behavior**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-06T02:35:18Z
- **Completed:** 2026-05-06T02:37:37Z
- **Tasks:** 2/2
- **Files modified:** 3

## Accomplishments

- Added RED fixture tests for Starfield v3 `CompressionMethod` metadata, method-3 raw LZ4-block extraction, raw method-3 entries, default deflate extraction, and codec-confusion failure with an empty sink.
- Extended `open_ba2` to accept Starfield GNRL v3, read its 36-byte header, and expose `archive_summary::compression_method` from byte offset 32.
- Implemented per-entry Starfield v3 compression-state resolution so method-3 compressed entries use `compression_state::lz4_block`, raw entries remain raw, default compressed entries remain deflate, and unsupported methods fail via the central dispatcher without partial writes.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing Starfield v3 route tests** - `47b4934` (test)
2. **Task 2 GREEN/REFACTOR: Implement Starfield v3 route resolution** - `24d5c22` (feat)

**Plan metadata:** committed separately in the final docs commit.

## Files Created/Modified

- `tests/ba2_reader_tests.cpp` - Adds Starfield v3 fixture header support, raw LZ4-block compressed payload generation, and five route/codec-confusion tests.
- `src/ba2_reader.cpp` - Parses Starfield v3 headers, stores `compression_method`, and resolves BA2 GNRL entry compression states for raw, deflate, LZ4 block, and unsupported routes.
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-04-SUMMARY.md` - Documents plan execution, verification, and TDD compliance.

## Decisions Made

- Kept Starfield v3 `CompressionMethod == 3` handling in BA2 metadata mapping rather than direct codec calls, preserving the existing `resolve_payload_codec` boundary.
- Used `compression_state::unknown` for unsupported nonzero v3 compression methods so extraction fails with a structured unsupported route before any sink write.
- Added a compatibility comment documenting the non-obvious constraint that Starfield BA2 v3 method 3 uses raw LZ4 blocks, not LZ4 frames, and raw entries remain raw.

## TDD Gate Compliance

- RED gate: `47b4934 test(06-04): add failing test for Starfield v3 BA2 compression routing` — codec-labeled BA2 tests failed because `open_ba2` rejected Starfield v3 as an unsupported BA2 version.
- GREEN gate: `24d5c22 feat(06-04): implement Starfield v3 BA2 compression routing` — codec-labeled tests passed after implementing v3 header parsing and compression-state routing.
- REFACTOR gate: not needed; no behavior-neutral cleanup commit was made.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes.

## Issues Encountered

None.

## Verification

- RED run: `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L codec` - failed as expected on the five new Starfield v3 tests because v3 was not yet supported by `open_ba2`.
- GREEN/final run: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L codec` - passed, 16/16 tests.
- Acceptance grep for all five required Starfield v3 tests in `tests/ba2_reader_tests.cpp` - passed.
- Acceptance grep for `compression_algorithm::lz4_block` and `compression_method` in `tests/ba2_reader_tests.cpp` - passed.
- Acceptance grep for `compression_method =`, `compression_state::lz4_block`, `raw entries remain raw`, and `not LZ4 frames` in `src/ba2_reader.cpp` - passed.
- Acceptance grep for `LZ4F|lz4frame|lz4_frame` in `src/ba2_reader.cpp` - passed with no matches.
- `git status --short TES5Edit` - passed with empty output.

## Known Stubs

None.

## Threat Flags

None. The plan threat model already covered v3 compression-method parsing, codec confusion, and raw LZ4-block decompression bounds.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 06-05/06-06 to complete remaining BA2 GNRL malformed coverage and phase-level verification using the now-covered Starfield v3 compression matrix.

## Self-Check: PASSED

- Found `tests/ba2_reader_tests.cpp`, `src/ba2_reader.cpp`, and `.planning/phases/06-ba2-gnrl-read-and-extract/06-04-SUMMARY.md`.
- Found task commits `47b4934` and `24d5c22` in git history.
- Confirmed `.planning/STATE.md` was not modified and `.planning/ROADMAP.md` remains only the pre-existing dirty orchestrator artifact.

---
*Phase: 06-ba2-gnrl-read-and-extract*
*Completed: 2026-05-06*
