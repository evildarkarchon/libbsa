---
phase: 08-writer-planning-streaming-emit-and-dedup-core
plan: 02
subsystem: writer-planning
tags: [cpp20, writer, tdd, catch2, compression, layout]

requires:
  - phase: 08-writer-planning-streaming-emit-and-dedup-core
    provides: Public writer plan/finalize API skeleton and focused writer test target from Plan 01
provides:
  - Deterministic writer planning from normalized in-memory entries
  - Sink-independent writer input validation for invalid paths, duplicate normalized paths, unsupported targets, unsupported compression capability, and layout overflow
  - Exact archive-absolute layout preview regions and plan-owned stored payload bytes for raw/deflate-capable targets
affects: [phase-08-writer-finalization, phase-08-dedup, phase-09-bsa-writers, phase-10-ba2-writers]

tech-stack:
  added: []
  patterns:
    - RED/GREEN TDD for writer planning behavior
    - Checked layout arithmetic with structured writer failures
    - Compression-policy planning through existing codec dispatcher

key-files:
  created:
    - .planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-02-SUMMARY.md
  modified:
    - include/libbsa/writer.hpp
    - src/writer.cpp
    - tests/writer_core_tests.cpp

key-decisions:
  - "Writer planning normalizes and sorts entries before layout, preserving deterministic preview order independent of caller input order."
  - "Plan-owned stored payload bytes are created during planning through resolve_write_compression, resolve_payload_codec, and compress_payload."
  - "Checked arithmetic failures are surfaced as malformed_archive / writer layout overflow before any finalization sink can be touched."

patterns-established:
  - "Layout preview uses fixed 16-byte header, 32-byte entry records plus normalized path bytes, 32-byte data-region records, and archive-absolute payload offsets."
  - "Duplicate normalized writer paths are rejected before any map-like storage can overwrite invalid input."

requirements-completed: [WRT-05, WRT-07]

duration: 4min
completed: 2026-05-07
---

# Phase 08 Plan 02: Deterministic Writer Planning and Validation Summary

**Deterministic writer planning with normalized sorted entries, checked layout preview, and codec-routed stored payload ownership**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-07T00:42:42Z
- **Completed:** 2026-05-07T00:45:58Z
- **Tasks:** 2 completed
- **Files modified:** 4

## Accomplishments

- Added RED Catch2 coverage for deterministic path ordering, exact table-region layout, duplicate/invalid path failures, unsupported target/compression failures, and impossible layout arithmetic.
- Replaced the writer planning placeholder with normalization, sorting, compression-policy resolution, stored payload creation, archive-absolute data-region offsets, and checked total-size calculation.
- Kept finalization behavior intentionally deferred while ensuring planning now owns post-policy payload bytes so later emit work cannot drift from preview metadata.

## TDD Evidence

- **RED:** `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_writer_tests && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` failed because the existing `plan_archive_write` placeholder still returned `unsupported_format` / `writer planning is not implemented`.
- **GREEN:** The same focused build and CTest command passed after implementing deterministic writer planning.
- **REFACTOR:** No separate refactor commit was needed; implementation remained straightforward after GREEN.

## Task Commits

Each TDD gate was committed atomically:

1. **Task 1: RED deterministic planning and invalid input tests** - `7402227` (test)
2. **Task 2: GREEN implement deterministic planner and checked validation** - `c1dc218` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/writer_core_tests.cpp` - Adds seven behavior tests for deterministic planning, exact layout preview, and structured invalid-input failures; updates the writer API surface test for successful planning.
- `src/writer.cpp` - Implements normalized/sorted writer planning, duplicate rejection, compression dispatch, plan-owned stored payloads, table/data region layout, and checked arithmetic failures.
- `include/libbsa/writer.hpp` - Updates `plan_archive_write` documentation from placeholder planning to deterministic payload-owning planning behavior.
- `.planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-02-SUMMARY.md` - Records execution results and TDD evidence.

## Decisions Made

- Used a reserved `writer_target::compression_method == UINT32_MAX` test seam to force layout arithmetic near `UINT64_MAX` without allocating oversized path or payload buffers.
- Assigned one data region per sorted entry for this plan because deduplication is intentionally deferred to the later Phase 8 dedup plan.
- Continued to keep `finalize_archive_write` as a structured placeholder because Plan 04 owns finalization behavior.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

| File | Line | Stub | Reason |
|------|------|------|--------|
| `src/writer.cpp` | 42 | `writer finalization is not implemented` structured failure | Intentional carry-forward from Plan 01; Plan 04 owns streaming finalization behavior. |
| `include/libbsa/writer.hpp` | 118 | Finalization documentation references later placeholder replacement | Intentional carry-forward until Plan 04 replaces finalization behavior. |

## Issues Encountered

- The plan text referenced `archive_format::unknown`, but the enum has no such value. The unsupported-target test uses `static_cast<archive_format>(255)` to exercise the same invalid target descriptor behavior.

## User Setup Required

None - no external service configuration required.

## Verification

- `rg -n "plans entries deterministically independent of caller order|exposes exact planned table regions before finalization|rejects duplicate normalized writer paths before layout|rejects invalid writer paths before layout|rejects unsupported writer target|rejects compression when target disallows compression|rejects impossible writer layout arithmetic" tests/writer_core_tests.cpp` - passed.
- `rg -n "checked_add|writer layout overflow|unsupported writer target|invalid writer path|duplicate writer path|normalize_archive_path|resolve_write_compression|compress_payload|entry_record_fixed_size|data_region_record_size" src/writer.cpp` - passed.
- `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_writer_tests` - passed.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` - passed, 8/8 tests.
- `git log --oneline --grep="^test(08-02)" -1` and `git log --oneline --grep="^feat(08-02)" -1` found the RED and GREEN commits.
- `git status --short TES5Edit` reported no changes.

## Next Phase Readiness

Ready for Plan 08-03 to build on deterministic planning for deduplication and/or the next writer-core behavior slice.

## Self-Check: PASSED

- Created/modified files exist: `include/libbsa/writer.hpp`, `src/writer.cpp`, `tests/writer_core_tests.cpp`, and this summary.
- Task commits exist: `7402227` and `c1dc218`.

---
*Phase: 08-writer-planning-streaming-emit-and-dedup-core*
*Completed: 2026-05-07*
