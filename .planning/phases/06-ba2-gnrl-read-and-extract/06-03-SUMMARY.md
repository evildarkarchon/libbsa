---
phase: 06-ba2-gnrl-read-and-extract
plan: 03
subsystem: ba2-extraction
tags: [cpp20, ba2, gnrl, extraction, deflate, tdd]

requires:
  - phase: 06-ba2-gnrl-read-and-extract
    provides: BA2 public API and bounded GNRL metadata parser from plans 06-01 and 06-02
provides:
  - Fixture-backed BA2 GNRL raw extraction for Fallout 4 v1/v7/v8
  - Fixture-backed BA2 GNRL deflate extraction for Fallout 4 v1/v7/v8 and Starfield v2
  - Lookup-failure extraction behavior that writes no partial sink bytes
affects: [phase-06-ba2-starfield-v3, phase-07-ba2-dds, phase-10-ba2-writer, public-api]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN gate commits for BA2 payload extraction behavior
    - BA2 extraction routed through existing payload codec dispatcher and byte_sink contract

key-files:
  created:
    - .planning/phases/06-ba2-gnrl-read-and-extract/06-03-SUMMARY.md
  modified:
    - tests/ba2_reader_tests.cpp
    - src/ba2_reader.cpp

key-decisions:
  - "Kept BA2 extraction independent of file extensions; GNRL .dds names are ordinary payload bytes."
  - "Used BA2 record PackedSize and Size fields directly for deflate extraction with no BSA embedded-size prefix."

patterns-established:
  - "BA2 extraction first looks up copied metadata, then reads exactly stored_size bytes from the caller-owned source."
  - "Codec routing uses archive format, entry_metadata::compression, and archive compression_method without fallback codecs."

requirements-completed: [BA2-01, BA2-02, BA2-04]

duration: 2min
completed: 2026-05-06
---

# Phase 06 Plan 03: BA2 GNRL Raw and Deflate Extraction Summary

**BA2 GNRL raw and deflate single-entry extraction using record sizes, metadata compression state, and caller-owned sinks**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-06T02:31:38Z
- **Completed:** 2026-05-06T02:33:35Z
- **Tasks:** 2/2
- **Files modified:** 3

## Accomplishments

- Added RED fixture tests for Fallout 4 v1/v7/v8 raw extraction, Fallout 4 v1/v7/v8 deflate extraction without a BSA prefix, Starfield v2 deflate extraction, GNRL `.dds` payload transparency, and lookup failures with empty sinks.
- Implemented `extract_ba2_entry` to read validated `entry_metadata::stored_size` payload ranges, resolve compression through `resolve_payload_codec`, decode with `decompress_payload`, and write complete output bytes to `byte_sink`.
- Preserved D-11/D-12 behavior by using BA2 record `PackedSize` and `Size` fields directly, avoiding fallback codecs, and returning structured errors before any sink write on lookup/range/codec failures.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing raw and deflate extraction tests** - `11b14b4` (test)
2. **Task 2 GREEN/REFACTOR: Implement raw and deflate BA2 extraction** - `ef7e50e` (feat)

**Plan metadata:** committed separately in the final docs commit.

## Files Created/Modified

- `tests/ba2_reader_tests.cpp` - Adds compressed BA2 fixture support and extraction behavior coverage for raw, deflate, `.dds` GNRL payloads, Starfield v2, and lookup failures.
- `src/ba2_reader.cpp` - Implements BA2 GNRL extraction through metadata lookup, bounded payload reads, compression dispatch, decompression, and sink writes.
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-03-SUMMARY.md` - Documents plan execution, verification, and TDD gate compliance.

## Decisions Made

- Kept BA2 GNRL extraction extension-agnostic so `.dds` names do not branch into Phase 7 DX10 texture reconstruction behavior.
- Used the existing compression dispatcher rather than direct libdeflate calls in `ba2_reader.cpp`, preserving central route validation and unsupported-route failures.
- Treated sink writes as the final step only after lookup, payload range validation, codec resolution, and decompression succeed.

## TDD Gate Compliance

- RED gate: `11b14b4 test(06-03): add failing test for BA2 raw and deflate extraction` — BA2 extraction tests failed because `extract_ba2_entry` still returned the Plan 01 unsupported extraction stub.
- GREEN gate: `ef7e50e feat(06-03): implement BA2 raw and deflate extraction` — BA2 extraction tests passed after implementing metadata-driven payload reads and codec dispatch.
- REFACTOR gate: not needed; no behavior-neutral cleanup commit was made.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes.

## Issues Encountered

None.

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug` - passed before RED and GREEN test runs.
- RED run: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` - failed as expected on extraction tests because `extract_ba2_entry` returned the unsupported stub.
- GREEN/final run: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` - passed, 14/14 tests.
- Acceptance greps for all nine extraction test names and absence of BA2 compressed-payload prefix insertion - passed.
- Acceptance greps for extraction implementation patterns and absence of `.dds`/DX10/texture branches in `src/ba2_reader.cpp` - passed.
- `git status --short TES5Edit` - passed with empty output.

## Known Stubs

None.

## Threat Flags

None. The plan's threat model already covered metadata-controlled payload range reads, compression dispatcher boundaries, and caller sink writes.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for the next Phase 6 plan to add Starfield v3 compression-method routing and LZ4-block extraction coverage on top of the metadata-driven extraction path.

## Self-Check: PASSED

- Found `tests/ba2_reader_tests.cpp` and `src/ba2_reader.cpp`.
- Found task commits `11b14b4` and `ef7e50e` in git history.
- Confirmed `.planning/STATE.md` was not modified and `.planning/ROADMAP.md` remains only the pre-existing dirty orchestrator artifact.

---
*Phase: 06-ba2-gnrl-read-and-extract*
*Completed: 2026-05-06*
