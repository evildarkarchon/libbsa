---
phase: 12-performance-concurrency-documentation-and-polish
plan: 01
subsystem: archive-api
tags: [cpp20, bulk-extraction, concurrency, bounded-memory, catch2]
requires:
  - phase: 04-tes3-bsa-read-extract
    provides: archive_reader extraction dispatch and bounded raw TES3 extraction pattern
  - phase: 05-ba2-gnrl-read-extract
    provides: BA2 GNRL reader and extraction routing
  - phase: 06-ba2-dx10-read-extract
    provides: BA2 DX10 reader and DDS reconstruction path
  - phase: 10-archive-writing
    provides: public writer helpers for legal synthetic extraction fixtures
provides:
  - Public archive_reader::extract_entries bulk extraction API
  - Deterministic indexed worker helper for opt-in parallel extraction
  - Request-order result aggregation with per-entry failure reporting
  - Bounded raw extraction proof across TES3, TES4-family, BA2 GNRL, and BA2 DX10
affects: [phase-12, archive_reader, extraction, concurrency, public-api]
tech-stack:
  added: []
  patterns: [TDD red-green-refactor commits, std::jthread indexed workers, per-entry sink factory, source-policy memory checks]
key-files:
  created:
    - src/detail/parallel_work.hpp
    - src/detail/parallel_work.cpp
    - tests/unit/bulk_extraction_tests.cpp
    - .planning/phases/12-performance-concurrency-documentation-and-polish/12-01-SUMMARY.md
  modified:
    - include/libbsa/archive.hpp
    - src/archive.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt
key-decisions:
  - "Bulk extraction uses archive_reader::extract_entries with an explicit positive worker_count and request-order result records."
  - "Bulk extraction sink factories must return distinct per-entry sinks and may be called concurrently when worker_count is greater than 1."
  - "Raw extraction bounded-memory proof uses large synthetic TES3, TES4, BA2 GNRL, and generated raw BA2 DX10 archives plus reader source-policy checks."
patterns-established:
  - "Public archive batch APIs should preserve caller request order while reporting independent per-entry failures."
  - "Private worker helpers may use std::jthread and atomics, but must not invoke caller callbacks while holding internal mutexes."
  - "Phase 12 memory proofs can combine large legal fixtures with source-policy checks when RSS gates would be brittle."
requirements-completed: [PERF-01, PERF-04]
duration: 9min
completed: 2026-05-10
---

# Phase 12 Plan 01: Bulk Extraction Summary

**Bulk archive extraction with deterministic per-entry results, opt-in `std::jthread` workers, and bounded raw extraction proof.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-05-10T07:42:10Z
- **Completed:** 2026-05-10T07:50:46Z
- **Tasks:** 3
- **Files modified:** 7

## Accomplishments

- Added a documented public `archive_reader::extract_entries` API with `bulk_extract_options`, `bulk_extract_request`, `bulk_extract_sink_factory`, and `bulk_extract_entry_result`.
- Implemented deterministic serial and opt-in parallel extraction using a private indexed worker helper while preserving request-order result records.
- Recorded per-entry lookup, sink factory, and extraction failures without aborting independent sibling entries.
- Proved raw extraction uses bounded 64 KiB chunks across TES3, TES4-family, BA2 GNRL, and BA2 DX10, while documenting the permitted compressed per-entry/per-texture-chunk allocation boundary.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: bulk extraction contract and behavior tests** - `d210727` (test)
2. **Task 2 GREEN: public API and deterministic worker implementation** - `de0ff2b` (feat)
3. **Task 3 REFACTOR: bounded extraction proof and source-policy checks** - `b26cf94` (test)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `include/libbsa/archive.hpp` - Adds the public bulk extraction types, Doxygen comments, thread-safety contract, and `archive_reader::extract_entries` declaration.
- `src/archive.cpp` - Implements request-order bulk extraction by delegating each requested entry through existing single-entry extraction.
- `src/detail/parallel_work.hpp` - Declares the private deterministic indexed worker helper.
- `src/detail/parallel_work.cpp` - Implements serial and `std::jthread` worker scheduling with positive worker-count validation.
- `CMakeLists.txt` - Builds the new worker helper source.
- `tests/CMakeLists.txt` - Registers the bulk extraction unit test file.
- `tests/unit/bulk_extraction_tests.cpp` - Covers RED contract checks, serial/parallel parity, per-entry failures, partial sink errors, and bounded raw extraction behavior.

## Decisions Made

- Bulk extraction remains an `archive_reader` member so it can reuse the existing archive format routing and public sink abstraction.
- `worker_count == 0U` is an outer argument error; per-entry lookup, factory, and write failures stay inside the result vector so sibling entries can continue.
- The sink factory contract explicitly requires one distinct sink per entry and documents concurrent `create` calls for `worker_count > 1`.
- BA2 DX10 raw coverage uses a generated legal fixture because the current public DX10 writer does not expose a raw-output mode.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed focused CTest discoverability for bulk extraction tests**
- **Found during:** Task 2 (GREEN public API and deterministic worker implementation)
- **Issue:** The RED tests initially used Catch2 tags such as `[bulk_extraction]`, but `ctest -R "bulk_extraction"` matches CTest test names, so the plan's focused command would not reliably select them.
- **Fix:** Renamed the Catch2 test case names to include the `bulk_extraction` token while preserving the tags.
- **Files modified:** `tests/unit/bulk_extraction_tests.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -R "bulk_extraction|public_include_boundary" --output-on-failure` selected and passed the expected tests.
- **Committed in:** `de0ff2b` (part of Task 2 commit)

**2. [Rule 2 - Missing Critical] Guarded null successful sink factory results**
- **Found during:** Task 2 (GREEN public API and deterministic worker implementation)
- **Issue:** The plan required sink factory failures to become per-entry failures, but a factory could also incorrectly return success with a null sink.
- **Fix:** Treated a null successful sink as a per-entry `error_code::invalid_argument` instead of dereferencing it.
- **Files modified:** `src/archive.cpp`
- **Verification:** Bulk extraction tests passed, including per-entry failure behavior and sibling continuation.
- **Committed in:** `de0ff2b` (part of Task 2 commit)

---

**Total deviations:** 2 auto-fixed (1 bug, 1 missing critical functionality)
**Impact on plan:** Both fixes were required for the planned verification and API safety. No architectural scope change.

## Issues Encountered

- The BA2 DX10 writer did not provide a public raw archive mode, so Task 3 generated a minimal legal raw DX10 BA2 fixture in the test helper path as allowed by the plan.

## Known Stubs

None found. Stub scan checked the touched source, header, test, and CMake files for TODO/FIXME/placeholder text and hardcoded empty UI-style values.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- PERF-01 and PERF-04 are ready for later Phase 12 plans to build on the public bulk extraction API and its deterministic concurrency contract.
- Compressed extraction still intentionally uses bounded per-entry or per-texture-chunk codec buffers per D-14; no streaming codec rewrite was introduced in this plan.

---

*Phase: 12-performance-concurrency-documentation-and-polish*
*Completed: 2026-05-10*

## Self-Check: PASSED

- Verified key created and modified files exist.
- Verified task commits `d210727`, `de0ff2b`, and `b26cf94` are reachable.
