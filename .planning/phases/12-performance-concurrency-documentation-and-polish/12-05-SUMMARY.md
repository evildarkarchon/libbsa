---
phase: 12-performance-concurrency-documentation-and-polish
plan: 05
subsystem: benchmarks
tags: [cpp20, cmake, benchmark-policy, ctest, synthetic-data]

requires:
  - phase: 12-performance-concurrency-documentation-and-polish
    provides: bulk extraction API and writer execution worker_count support from plans 12-01 through 12-04
provides:
  - Buildable libbsa_benchmarks executable with synthetic serial and parallel scenarios
  - Explicit libbsa_benchmark_report CMake target emitting JSON and Markdown reports
  - Benchmark README documenting commands, report schema, synthetic data policy, and no speed threshold gate
  - Source-text benchmark policy tests guarding report schema and default CTest boundaries
affects: [phase-12, benchmarks, cmake, ctest, performance-policy]

tech-stack:
  added: []
  patterns: [custom C++20 benchmark runner, explicit CMake report target, source-text policy tests]

key-files:
  created:
    - benchmarks/libbsa_benchmarks.cpp
    - benchmarks/README.md
    - tests/unit/benchmark_policy_tests.cpp
    - .planning/phases/12-performance-concurrency-documentation-and-polish/12-05-SUMMARY.md
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Benchmark evidence uses a custom C++20 runner with no new benchmark dependency."
  - "Benchmark timing is report-only; correctness, report generation, and explicit target wiring are the automated contract."
  - "Benchmark data is generated legal synthetic temp data and never uses TES5Edit or game archives."

patterns-established:
  - "Benchmark tooling should remain an explicit custom target outside default CTest timing gates."
  - "Benchmark report policy is enforced with source-text tests for CMake, README, runner, and CTest discovery boundaries."

requirements-completed: [PERF-05]

duration: 7min
completed: 2026-05-10
---

# Phase 12 Plan 05: Benchmark Harness Summary

**Synthetic benchmark report tooling for serial versus opt-in parallel packing and extraction, with correctness checks and no fixed speed threshold gate.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-05-10T08:33:53Z
- **Completed:** 2026-05-10T08:40:34Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments

- Added `libbsa_benchmarks`, a standalone C++20 benchmark runner using generated synthetic data and public libbsa reader/writer APIs.
- Added `libbsa_benchmark_report`, an explicit CMake custom target that emits `libbsa-benchmark.json` and `libbsa-benchmark.md`.
- Covered `tes4_bsa_pack_extract`, `ba2_gnrl_pack_extract`, `ba2_dx10_pack_extract`, and `bulk_extract` scenarios for worker counts `1` and `4`.
- Added source-text benchmark policy tests for target wiring, report schema, legal synthetic data wording, no fixed speed threshold gates, and default CTest exclusion.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: benchmark policy and report schema tests** - `383ac6c` (test)
2. **Task 2 GREEN: synthetic benchmark runner and explicit report target** - `98fb026` (feat)
3. **Task 3 REFACTOR: default-test boundary and report-policy checks** - `a7c5b85` (refactor)

**Plan metadata:** final docs commit records this summary and state updates.

## Files Created/Modified

- `benchmarks/libbsa_benchmarks.cpp` - Synthetic benchmark runner, worker-count scenarios, correctness checks, and JSON/Markdown report writers.
- `benchmarks/README.md` - Benchmark command, report schema, legal synthetic data policy, and D-20 no-speed-threshold policy.
- `tests/unit/benchmark_policy_tests.cpp` - Source-text policy checks for benchmark wiring, reports, README policy, and default CTest exclusion.
- `tests/CMakeLists.txt` - Registers the benchmark policy test file.
- `CMakeLists.txt` - Adds `LIBBSA_BUILD_BENCHMARKS`, `libbsa_benchmarks`, and `libbsa_benchmark_report`.

## Decisions Made

- The benchmark harness uses only standard C++ and libbsa public APIs, avoiding a new benchmark dependency.
- Reports include timing values for maintainers, but default tests assert policy and correctness rather than speedup.
- Benchmark inputs are generated under a temporary directory from repository-owned bytes, preserving the legal fixture and TES5Edit boundaries.

## TDD Gate Compliance

- RED gate present: `383ac6c` added failing benchmark policy tests before runner, target, and README implementation.
- GREEN gate present after RED: `98fb026` added the benchmark executable, explicit report target, report generation, and README.
- REFACTOR gate present after GREEN: `a7c5b85` added the default CTest boundary guard.

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_benchmark_report` - passed and emitted both reports.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- `ctest --preset windows-msvc-debug-static -R "benchmark_policy" --output-on-failure` - passed, 5/5 tests.
- `ctest --preset windows-msvc-debug-static --show-only=json-v1 | rg "libbsa_benchmark_report|elapsed_ms"` - no matches.
- `git status --short TES5Edit` - no output.
- Acceptance `rg` checks for scenario/report tokens, target wiring, README policy text, and absence of source-level speedup gates passed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Created the bulk benchmark temp directory before writing its source archive**
- **Found during:** Task 2 (GREEN synthetic benchmark runner and explicit report target)
- **Issue:** The first report-target run failed because the bulk extraction scenario wrote a TES3 archive into a worker-count temp subdirectory that had not been created.
- **Fix:** Created the bulk scenario temp directory before finalizing the synthetic source archive.
- **Files modified:** `benchmarks/libbsa_benchmarks.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static --target libbsa_benchmark_report` passed and emitted both reports.
- **Committed in:** `98fb026`

---

**Total deviations:** 1 auto-fixed blocking issue.
**Impact on plan:** The fix was limited to benchmark setup correctness and did not change the planned public or build surface.

## Issues Encountered

- The Task 2 report target initially failed on missing temp-directory setup for the bulk extraction scenario; it was fixed inline and verified before the GREEN commit.

## Known Stubs

None - stub scan found no TODO/FIXME/placeholder text or hardcoded empty UI-style placeholders in the files created or modified by this plan.

## Auth Gates

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 12-06 can reference the benchmark target names and README policy when adding API documentation and thread-safety guidance.
- PERF-05 is complete; remaining Phase 12 work should stay scoped to docs, examples, and target-format guidance.

---
*Phase: 12-performance-concurrency-documentation-and-polish*
*Completed: 2026-05-10*

## Self-Check: PASSED

- Verified key created and modified files exist.
- Verified task commits `383ac6c`, `98fb026`, and `a7c5b85` are reachable.
- Verified `git status --short TES5Edit` returned no output.
