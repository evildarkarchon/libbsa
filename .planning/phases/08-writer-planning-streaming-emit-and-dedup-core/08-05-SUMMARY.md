---
phase: 08-writer-planning-streaming-emit-and-dedup-core
plan: 05
subsystem: writer-harness-readback
tags: [cpp20, writer, tdd, catch2, harness, codec-routing]

requires:
  - phase: 08-writer-planning-streaming-emit-and-dedup-core
    provides: Streaming LBSW finalization with deterministic header, entry table, data-region table, and payload emission
provides:
  - Test-only LBSW parser for generated writer harness bytes
  - Metadata comparison between finalized harness bytes and write_plan preview records
  - Raw and deflate read-after-write extraction through the real codec dispatcher
  - Starfield compression-method route rejection coverage to prevent silent deflate fallback
affects: [phase-09-bsa-writers, phase-10-ba2-writers]

tech-stack:
  added: []
  patterns:
    - RED/GREEN TDD for generated writer read-back validation
    - Test-only parsed harness structs under libbsa::test
    - Codec extraction tests using resolve_payload_codec and decompress_payload

key-files:
  created:
    - .planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-05-SUMMARY.md
  modified:
    - src/writer.cpp
    - tests/writer_core_tests.cpp
    - tests/writer_harness_helpers.hpp
    - tests/writer_harness_helpers.cpp

key-decisions:
  - "The LBSW parser stays test-only in tests/writer_harness_helpers.* and no writer_harness API is exposed under include/libbsa or src."
  - "Parsed entry compression is derived from the referenced data-region record because finalized entry records intentionally store data_region_id rather than duplicating codec state."
  - "Starfield writer planning preserves nonzero archive compression_method routing through lz4_block so unsupported method values fail through resolve_payload_codec instead of falling back to deflate."

patterns-established:
  - "require_harness_matches_plan compares table regions, entries, data regions, stored sizes, offsets, compression states, and stored payload bytes."
  - "extract_writer_harness_entry resolves and decompresses parsed stored payloads through production codec dispatch."

requirements-completed: [WRT-05, WRT-06, WRT-07]

duration: 11min
completed: 2026-05-06
---

# Phase 08 Plan 05: Generated Harness Read-After-Write Summary

**Finalized LBSW harness bytes can be parsed, compared back to write_plan metadata, and extracted through real codec routes in tests only**

## Performance

- **Duration:** 11 min
- **Started:** 2026-05-06T17:55:00-07:00
- **Completed:** 2026-05-06T18:06:19-07:00
- **Tasks:** 2 completed
- **Files modified:** 5

## Accomplishments

- Added RED Catch2 coverage for finalized harness read-back, compressed round-trip extraction, archive-default compression stored-size checks, and unsupported codec route rejection.
- Implemented `read_writer_harness`, `require_harness_matches_plan`, and `extract_writer_harness_entry` in test helpers only.
- Added Starfield writer compression-method routing so method-specific compressed writer planning cannot silently use a deflate fallback when the target method is unsupported.

## TDD Evidence

- **RED:** `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_writer_tests` failed at link time with unresolved `read_writer_harness`, `extract_writer_harness_entry`, and `require_harness_matches_plan` symbols after the declarations and tests were added.
- **GREEN:** `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` passed 18/18 writer tests after implementing the helper parser and codec route behavior.
- **REFACTOR:** No separate refactor commit was needed; the GREEN implementation remained focused.

## Task Commits

Each TDD gate was committed atomically:

1. **Task 1: RED harness read-back tests** - `903b6a0` (test)
2. **Task 2: GREEN implement test-only harness parser and assertions** - `cb2e355` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/writer_core_tests.cpp` - Adds read-after-write harness tests for dedup raw payloads, deflate payloads, archive-default compression metadata, and unsupported codec route failure.
- `tests/writer_harness_helpers.hpp` - Declares test-only parsed harness structs and helper APIs.
- `tests/writer_harness_helpers.cpp` - Parses LBSW bytes, validates plan metadata, and extracts payloads through real codec dispatch.
- `src/writer.cpp` - Preserves Starfield compression-method routing for compressed writer planning.
- `.planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-05-SUMMARY.md` - Records execution results and TDD evidence.

## Decisions Made

- Kept the generated harness parser in `libbsa::test` under `tests/` so Phase 8 does not expose a public fake archive format.
- Derived parsed entry compression from each referenced data-region record, matching the emitted LBSW table contract from Plan 04.
- Routed Starfield compressed writer planning with nonzero `compression_method` through `compression_state::lz4_block`; method `3` is supported by the existing codec dispatcher and other values return `unsupported compression route`.

## Deviations from Plan

- The helper parser derives entry compression from data regions rather than parsing it from entry records because Plan 04 entry records do not serialize a per-entry compression field.
- The unsupported-route test required a small production writer routing fix in `src/writer.cpp`, not only test-helper code, because the existing planner accepted the invalid Starfield method by falling back to deflate.

## Known Stubs

None.

## Issues Encountered

- Initial RED compile hit a helper-order typo before the intended missing-symbol failure; the test helper order was corrected and RED was re-run to the expected unresolved helper symbols.

## User Setup Required

None - no external service configuration required.

## Verification

- `rg -n "parsed_writer_harness|read_writer_harness|extract_writer_harness_entry|require_harness_matches_plan" tests/writer_harness_helpers.hpp` - passed.
- `rg -n "reads finalized harness bytes back and compares plan metadata|round trips compressed harness payloads through real codec routes|archive default compression records exact stored size in plan and harness|writer planning rejects unsupported codec route without fallback" tests/writer_core_tests.cpp` - passed.
- `rg -n "read_u32|read_u64|LBSW|require_harness_matches_plan|extract_writer_harness_entry|decompress_payload|resolve_payload_codec" tests/writer_harness_helpers.cpp` - passed.
- `rg -n "writer_harness" include/libbsa src` - no matches.
- `gsd-sdk query verify.key-links ".planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-05-PLAN.md"` - passed 1/1 key links.
- `cmake --build build/windows-vs2026-vcpkg --config Debug && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug` - passed 132/132 tests.

## TDD Gate Compliance

- **RED gate:** `903b6a0 test(08-05): add failing writer harness read-back tests`
- **GREEN gate:** `cb2e355 feat(08-05): implement writer harness read-back validation`
- **REFACTOR gate:** Not needed.

## Next Phase Readiness

Ready for Plan 08-06 to add public smoke coverage, documentation updates, and final boundary validation gates.

## Self-Check: PASSED

- Created/modified files exist: `src/writer.cpp`, `tests/writer_core_tests.cpp`, `tests/writer_harness_helpers.hpp`, `tests/writer_harness_helpers.cpp`, and this summary.
- Task commits exist: `903b6a0` and `cb2e355`.

---
*Phase: 08-writer-planning-streaming-emit-and-dedup-core*
*Completed: 2026-05-06*
