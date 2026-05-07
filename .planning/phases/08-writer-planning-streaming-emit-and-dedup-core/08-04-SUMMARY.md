---
phase: 08-writer-planning-streaming-emit-and-dedup-core
plan: 04
subsystem: writer-finalization
tags: [cpp20, writer, tdd, catch2, streaming, byte-sink]

requires:
  - phase: 08-writer-planning-streaming-emit-and-dedup-core
    provides: Deterministic write plans with table regions, data regions, post-policy stored payloads, and optional deduplication
provides:
  - Streaming finalization from frozen write plans through caller-owned byte_sink implementations
  - Deterministic harness header, entry table, data-region table, and stored-payload emission order
  - First sink-failure propagation as structured io_failure without partial-success reporting
affects: [phase-08-harness-readback, phase-09-bsa-writers, phase-10-ba2-writers]

tech-stack:
  added: []
  patterns:
    - RED/GREEN TDD for writer finalization behavior
    - Little-endian table chunk serialization with caller-owned sink writes
    - Finalization emits plan-owned stored_payload without recompression or renormalization

key-files:
  created:
    - .planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-04-SUMMARY.md
  modified:
    - include/libbsa/writer.hpp
    - src/writer.cpp
    - tests/writer_core_tests.cpp

key-decisions:
  - "Writer finalization streams chunks in frozen plan order: 16-byte LBSW header, entry table, data-region table, then data regions."
  - "Finalization serializes entry table records using the same fixed-fields-plus-path-bytes contract used by layout planning."
  - "Finalization propagates the first byte_sink write error unchanged and performs no compression, normalization, sorting, or dedup decisions."

patterns-established:
  - "Entry table emission verifies serialized byte count against the planned entry_table region size before writing."
  - "Data-region payload emission iterates plan.data_regions and writes each stored_payload directly."

requirements-completed: [WRT-05, WRT-07]

duration: 3min
completed: 2026-05-07
---

# Phase 08 Plan 04: Streaming Finalization Emitter Summary

**LBSW writer finalization streams frozen plan tables and stored payloads through caller-owned sinks with first-failure propagation**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-07T00:55:41Z
- **Completed:** 2026-05-07T00:57:59Z
- **Tasks:** 2 completed
- **Files modified:** 4

## Accomplishments

- Added RED Catch2 coverage for successful memory-sink finalization, variable path-byte entry-table sizing, and injected sink failure propagation.
- Implemented `finalize_archive_write` to emit deterministic header/table/payload chunks through `byte_sink::write` from the already-planned `write_plan`.
- Preserved planning/finalization separation by avoiding compression, path normalization, sorting, and dedup recomputation during finalization.

## TDD Evidence

- **RED:** `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_writer_tests && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` failed because `finalize_archive_write` still returned `unsupported_format` with `writer finalization is not implemented`.
- **GREEN:** The same focused build and CTest command passed after implementing ordered sink emission and first-failure propagation.
- **REFACTOR:** No separate refactor commit was needed; the GREEN implementation remained focused.

## Task Commits

Each TDD gate was committed atomically:

1. **Task 1: RED finalization and sink-failure tests** - `cf88cbf` (test)
2. **Task 2: GREEN implement ordered sink emitter** - `9d8060d` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/writer_core_tests.cpp` - Adds finalization tests for memory-sink streaming output, exact variable path-byte entry-table sizing, and injected sink failure propagation.
- `src/writer.cpp` - Adds little-endian append helpers, deterministic header/table builders, sink write helper, and streaming finalization over `planned_data_region::stored_payload`.
- `include/libbsa/writer.hpp` - Updates finalization documentation to describe first-failure propagation and sink lifetime behavior.
- `.planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-04-SUMMARY.md` - Records execution results and TDD evidence.

## Decisions Made

- Kept the Phase 8 harness header at the planned 16-byte size by recording the checked total size in the header chunk while retaining full `write_plan::total_size` in preview metadata.
- Serialized entry-table records as `data_region_id`, `offset`, `size`, `stored_size`, path length, and exact normalized path bytes to match Plan 02 layout sizing.
- Returned the first `byte_sink::write` failure unchanged so caller-owned sink failures remain structurally observable.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None - the Plan 03 finalization placeholder was replaced and no blocking stubs remain in the modified files.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Verification

- `rg -n "class failing_sink final|finalizes planned bytes to caller-owned sink|emits entry table with planned variable path byte size|returns sink failure during finalization|injected sink failure|LBSW" tests/writer_core_tests.cpp` - passed.
- `rg -n "append_u32|append_u64|sink\.write|LBSW|stored_payload|return failure<void>\(.*error" src/writer.cpp` - passed.
- `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_writer_tests && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` - RED failed before implementation, then GREEN passed 14/14 writer tests.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` - passed 14/14 writer tests after both task commits.
- `git log --oneline --grep="^test(08-04)" -1` and `git log --oneline --grep="^feat(08-04)" -1` found the RED and GREEN commits.
- `git status --short TES5Edit` reported no changes.

## TDD Gate Compliance

- **RED gate:** `cf88cbf test(08-04): add failing streaming finalization tests`
- **GREEN gate:** `9d8060d feat(08-04): implement streaming writer finalization`
- **REFACTOR gate:** Not needed.

## Next Phase Readiness

Ready for Plan 08-05 to add generated harness read-after-write validation over the emitted LBSW header/table/data stream.

## Self-Check: PASSED

- Created/modified files exist: `include/libbsa/writer.hpp`, `src/writer.cpp`, `tests/writer_core_tests.cpp`, and this summary.
- Task commits exist: `cf88cbf` and `9d8060d`.

---
*Phase: 08-writer-planning-streaming-emit-and-dedup-core*
*Completed: 2026-05-07*
