---
phase: 12
slug: performance-concurrency-documentation-and-polish
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-10
---

# Phase 12 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.x with CTest; Python 3 for manifest/report policy scripts |
| **Config file** | `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Full suite command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` plus phase-specific benchmark/docs policy targets when added |
| **Estimated runtime** | Existing CTest suite runtime plus Phase 12 policy checks; benchmark speed runs are not default CTest gates |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` or the narrow Catch2/CTest target introduced by the task.
- **After every plan wave:** Run the full CTest suite and any wave-owned policy target added by the completed plans.
- **Before `$gsd-verify-work`:** Full suite, package-consumer checks, docs policy checks, benchmark report validation, and `git status --short TES5Edit` must be green.
- **Max feedback latency:** Keep task-level checks under roughly 120 seconds by using focused tests before running the full suite.

---

## Per-Task Verification Map

Plan-specific task IDs are created by `12-*-PLAN.md`. Every implementation task must include an automated verification command or be tied to one of these phase gates:

| Area | Requirement | Required Verification |
|------|-------------|-----------------------|
| Bounded streaming extraction and packing | PERF-01, PERF-02, PERF-03 | Unit/fixture tests proving large synthetic archives do not require whole-archive buffers and still round-trip/extract correctly |
| Opt-in parallel decompression/compression | PERF-04 | Serial-default and parallel-opt-in tests, deterministic result ordering, per-entry error aggregation, and stable `result` error surfaces |
| Benchmark harness | PERF-05 | Buildable benchmark target plus report validation; benchmark timing is not a default CI pass/fail threshold |
| Thread-safety contract | PERF-06 | Machine-checked docs/API policy tests covering readers, writers, entries, callbacks, sinks, validation APIs, and bulk/parallel types |
| Doxygen/API docs | DOC-01 | Optional `FindDoxygen`/docs target plus public-header documentation policy checks; missing local Doxygen must not break normal builds |
| Integration examples | DOC-02 | Compile-checked examples or package-consumer tests for open/list/extract, bulk extraction, writer creation, `result` errors, and validation |
| Target-format guidance | DOC-03 | Docs policy tests requiring supported formats, compression routes, compatibility warnings, and writer target policies to be documented |

---

## Wave 0 Requirements

Existing infrastructure covers the phase baseline:

- [x] Catch2 and CTest are already available through vcpkg and repo CMake.
- [x] Package-consumer smoke coverage already exists and can be extended for compile-checked examples.
- [x] Generated legal fixtures are the expected validation model; do not use `TES5Edit/` as a mutable fixture workspace.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Doxygen rendering quality | DOC-01 | Local Doxygen may be absent and visual output quality is not a default runtime dependency | If Doxygen is installed, build the docs target and confirm generated public API pages exist for public headers only |
| Benchmark performance interpretation | PERF-05 | Timing varies by machine and should not be a deterministic CI threshold | Run the benchmark target manually when performance numbers matter, then compare single-threaded and multi-threaded reports |

---

## Validation Sign-Off

- [x] All planned implementation areas have automated verify gates or an explicitly manual-only rationale.
- [x] Sampling continuity requires no 3 consecutive implementation tasks without an automated verify command.
- [x] Wave 0 uses existing test infrastructure; no new framework bootstrap is required.
- [x] No watch-mode flags are part of the validation contract.
- [x] Feedback latency target is documented for task-level checks.
- [x] `nyquist_compliant: true` is set in frontmatter.

**Approval:** pending
