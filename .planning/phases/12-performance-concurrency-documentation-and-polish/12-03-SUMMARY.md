---
phase: 12-performance-concurrency-documentation-and-polish
plan: 03
subsystem: bsa-writer-finalization
tags: [cpp20, bsa-writer, concurrency, streaming, safe-publish, catch2]
requires:
  - phase: 12-performance-concurrency-documentation-and-polish
    provides: write_execution_options public contract from Plan 12-02
  - phase: 10-tes3-write-support-and-bsa-format-completeness
    provides: TES3 public writer, reader-backed reopen tests, and safe publish helpers
  - phase: 07-tes4-family-bsa-write-new-support
    provides: TES4-family public writer, compression routing, dedupe, and reader-backed tests
provides:
  - BSA writer final archive publication without final whole-archive byte vectors
  - TES3 disk-backed payload streaming through bounded scratch buffers
  - TES4 worker-count entry preparation through detail::run_indexed_work
  - TES4 final payload streaming for raw disk-backed entries
  - Worker-count failure coverage proving readable archive preservation
affects: [phase-12, bsa-writer, packing, concurrency, safe-publish]
tech-stack:
  added: []
  patterns: [TDD red-green-refactor commits, bounded scratch streaming, deterministic indexed work, safe temporary publish]
key-files:
  created:
    - tests/unit/bsa_writer_execution_tests.cpp
    - .planning/phases/12-performance-concurrency-documentation-and-polish/12-03-SUMMARY.md
  modified:
    - src/formats/bsa/tes3_bsa_writer.cpp
    - src/formats/bsa/tes4_bsa_writer.cpp
    - src/formats/bsa/tes4_bsa_writer.hpp
    - tests/CMakeLists.txt
    - tests/unit/bounded_memory_policy_tests.cpp
key-decisions:
  - "BSA writers stream final archive publication directly to the temporary output instead of materializing a final whole-archive byte vector."
  - "TES4 worker_count is consumed during independent entry preparation with deterministic indexed result collection before grouping, sorting, offsets, and publish."
  - "TES4 overwrite publication uses the same safe temporary publish model as other writers so failed worker-count paths preserve readable destination archives."
patterns-established:
  - "Bounded-memory policy tests should reject source-level final archive writer.bytes() publication for BSA writers."
  - "Writer execution tests should reopen generated archives and compare extracted payloads, not only assert write success."
requirements-completed: [PERF-02, PERF-03]
duration: 10min
completed: 2026-05-10
---

# Phase 12 Plan 03: BSA Writer Finalization Summary

**BSA writer finalization now streams disk-backed output through bounded buffers and consumes `write_execution_options` for TES4 entry preparation.**

## Performance

- **Duration:** 10 min
- **Started:** 2026-05-10T08:04:11Z
- **Completed:** 2026-05-10T08:14:09Z
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments

- Added BSA writer execution tests for TES3 large disk-backed writes, TES4 serial-vs-worker output compatibility, and worker-count failure rollback.
- Converted TES3 final publication away from `writer.bytes()` and into direct metadata plus payload streaming with a 64 KiB scratch buffer.
- Converted TES4 final publication away from final whole-archive byte vectors for raw disk-backed entries while preserving compressed-entry payload materialization.
- Routed TES4 entry preparation through `detail::run_indexed_work` when `write_execution_options.worker_count` is greater than one, then kept grouping, sorting, offsets, and publish deterministic.
- Added bounded-memory policy checks for final BSA publication, source-policy text, and public-header helper leakage.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: failing BSA writer execution tests** - `1044abf` (test)
2. **Task 2 GREEN: stream BSA writer finalization** - `9b6f256` (feat)
3. **Task 3 REFACTOR: BSA publish failure and policy coverage** - `d9ae329` (refactor)

**Plan metadata:** final docs commit records this summary and state updates.

## Files Created/Modified

- `src/formats/bsa/tes3_bsa_writer.cpp` - Keeps disk-backed TES3 sources path-backed through table sizing and streams payload bytes to the temporary archive with bounded scratch space.
- `src/formats/bsa/tes4_bsa_writer.cpp` - Adds worker-count preparation, raw disk-backed payload streaming, safe temporary publish, and source-policy cleanup.
- `src/formats/bsa/tes4_bsa_writer.hpp` - Passes `worker_count` through the internal TES4 writer entry point.
- `tests/CMakeLists.txt` - Registers `bsa_writer_execution_tests.cpp` and `bounded_memory_policy_tests.cpp`.
- `tests/unit/bsa_writer_execution_tests.cpp` - Covers large TES3 streaming, TES4 worker compatibility, missing-source rollback, and overwrite failure rollback.
- `tests/unit/bounded_memory_policy_tests.cpp` - Covers BSA final publish source policy, streaming text, and public-header helper boundaries.

## Decisions Made

- Final BSA archive publication should be stream-first; final whole-archive vectors remain forbidden by policy tests.
- TES4 parallel work is limited to independent entry preparation in this plan. Table assembly and publish stay deterministic and serial after worker results are collected.
- Safe publish semantics apply to TES4 overwrite paths as well as TES3: failed replacement attempts must leave the existing destination readable.

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` passed.
- `ctest --preset windows-msvc-debug-static -R "bsa_writer_execution|bounded_memory_policy|tes3_bsa_writer|TES4 BSA writer|public_include_boundary" --output-on-failure` passed: 49 tests, 0 failed.
- `ctest --preset windows-msvc-debug-static --output-on-failure` passed: 208 tests, 0 failed, 2 optional local-fixture tests skipped.
- `rg -n "TES5Edit" src/formats/bsa/tes3_bsa_writer.cpp src/formats/bsa/tes4_bsa_writer.cpp` returned no matches.
- `git status --short TES5Edit` returned no output.

## TDD Gate Compliance

- RED gate commit exists: `1044abf` added failing BSA writer execution and bounded-memory tests. The RED run failed on the final whole-archive vector policy as expected.
- GREEN gate commit exists after RED: `9b6f256` implemented BSA streaming finalization and TES4 worker-count preparation.
- REFACTOR gate commit exists after GREEN: `d9ae329` added failure-path policy coverage and removed the remaining BSA writer source-policy token.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Used the active CTest preset instead of a stale Task 3 build directory**
- **Found during:** Task 3 (REFACTOR publish rollback and source policy)
- **Issue:** The Task 3 verification command referenced `build/local-vs2026-vcpkg`, but this checkout only had the active `windows-msvc-debug-static` preset/build directory.
- **Fix:** Confirmed the plan-specific directory was absent, then ran the full suite with `ctest --preset windows-msvc-debug-static --output-on-failure`.
- **Files modified:** None
- **Verification:** Full preset CTest passed: 208 tests, 0 failed, 2 optional local-fixture tests skipped.
- **Committed in:** N/A - verification command adjustment only

---

**Total deviations:** 1 auto-fixed blocking verification mismatch.
**Impact on plan:** The repo-native preset exercised the same committed test binary and all 12-03 acceptance tests passed.

## Issues Encountered

- The exact Task 3 `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` command could not run because that directory does not exist in this checkout; the active preset command was used and passed.
- Two implementation comments were rewritten because the code they described changed or because the plan required source-policy cleanup: the TES3 source-validation comment now describes path-backed D-16 behavior, and the TES4 folder-offset comment now says "Reference-compatible" instead of naming the read-only reference submodule.

## Known Stubs

None found. Stub scan checked touched source, header, test, and CMake files for TODO/FIXME/placeholder text and hardcoded empty UI-style values.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 12-04 can reuse the bounded-memory policy pattern and `write_execution_options` contract for BA2 writer finalization.
- Documentation and benchmark plans can cite the new BSA writer execution tests as evidence for worker-count compatibility and safe publish rollback.

---

*Phase: 12-performance-concurrency-documentation-and-polish*
*Completed: 2026-05-10*

## Self-Check: PASSED

- Verified key created and modified files exist.
- Verified task commits `1044abf`, `9b6f256`, and `d9ae329` are reachable.
- Verified `git status --short TES5Edit` returned no output.
