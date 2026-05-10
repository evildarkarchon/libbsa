---
phase: 12-performance-concurrency-documentation-and-polish
plan: 06
subsystem: documentation
tags: [cpp20, doxygen, thread-safety, docs-policy, ctest]

requires:
  - phase: 12-performance-concurrency-documentation-and-polish
    provides: bulk extraction API, writer execution options, and benchmark target names from Plans 12-01, 12-02, and 12-05
provides:
  - Optional libbsa_docs Doxygen target for public API documentation
  - Public-header-only Doxyfile excluding private implementation and TES5Edit
  - Canonical thread-safety guidance for readers, writers, sinks, validation, bulk extraction, and benchmarks
  - Default policy tests for documentation scope and thread-safety coverage
affects: [phase-12, public-api-docs, thread-safety, ctest, cmake]

tech-stack:
  added: []
  patterns: [optional Doxygen target, public-header docs boundary, source-text docs policy tests]

key-files:
  created:
    - docs/Doxyfile.in
    - docs/api-mainpage.md
    - docs/thread-safety.md
    - tests/unit/docs_policy_tests.cpp
    - tests/unit/thread_safety_docs_policy_tests.cpp
    - .planning/phases/12-performance-concurrency-documentation-and-polish/12-06-SUMMARY.md
  modified:
    - CMakeLists.txt
    - include/libbsa/archive.hpp
    - include/libbsa/writer.hpp
    - include/libbsa/validation.hpp
    - tests/CMakeLists.txt

key-decisions:
  - "Doxygen remains optional through find_package(Doxygen QUIET); normal configure/build paths continue when the tool is missing."
  - "Public API documentation is generated from include/libbsa plus docs pages only, with src, tests, build output, and TES5Edit excluded."
  - "docs/thread-safety.md is the canonical D-23 thread-safety reference and public headers point readers to it instead of duplicating every rule inline."
  - "Docs generation and thread-safety coverage are enforced by default source-text policy tests."

patterns-established:
  - "Optional documentation tooling should expose an explicit target while remaining absent-safe for normal builds."
  - "Concurrency documentation should name public types directly and use source-text policy tests to prevent coverage drift."

requirements-completed: [PERF-06, DOC-01]

duration: 5min
completed: 2026-05-10
---

# Phase 12 Plan 06: Public API Documentation and Thread-Safety Summary

**Optional Doxygen public API generation with machine-checked thread-safety guidance for libbsa consumers.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-10T08:43:31Z
- **Completed:** 2026-05-10T08:48:48Z
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments

- Added an optional `libbsa_docs` CMake target that configures `docs/Doxyfile.in` only when Doxygen is installed.
- Added Doxygen public API entry pages covering the reader, bulk extraction, writers, write execution options, validation, thread-safety, and target-format guide references.
- Added `docs/thread-safety.md` with required D-23 sections for readers, sinks, sink factories, bulk results/options, writers, validation, and benchmark tooling.
- Added default Catch2 policy tests for Doxygen input/exclusion rules and thread-safety coverage.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add optional Doxygen public API target** - `86101e9` (feat)
2. **Task 2: Add canonical thread-safety documentation and public header notes** - `a3473c9` (docs)
3. **Task 3: Add docs and thread-safety policy tests** - `e2d54ee` (test)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `CMakeLists.txt` - Adds optional Doxygen discovery, Doxyfile configuration, `libbsa_docs`, and the missing-tool status path.
- `docs/Doxyfile.in` - Defines public-header documentation inputs and excludes private implementation, tests, build output, and `TES5Edit`.
- `docs/api-mainpage.md` - Provides the Doxygen `@mainpage` for public reader, writer, validation, bulk extraction, execution option, and docs links.
- `docs/thread-safety.md` - Documents the canonical thread-safety rules for public API consumers and benchmark tooling.
- `include/libbsa/archive.hpp` - Adds public Doxygen thread-safety references for readers, sinks, factories, options, and result records.
- `include/libbsa/writer.hpp` - Adds public Doxygen thread-safety references for writer objects and write execution options.
- `include/libbsa/validation.hpp` - Adds public Doxygen thread-safety references for validation reports and `validate_archive`.
- `tests/CMakeLists.txt` - Registers documentation policy test sources.
- `tests/unit/docs_policy_tests.cpp` - Checks Doxygen target/config policy, public-only inputs, exclusions, and mainpage coverage.
- `tests/unit/thread_safety_docs_policy_tests.cpp` - Checks required public type sections and key D-23 concurrency rule wording.

## Decisions Made

- Doxygen generation is an optional explicit target, not a required configure/build dependency.
- The docs boundary is public-header-first: `include/libbsa`, `docs/api-mainpage.md`, and `docs/thread-safety.md` are included; `src`, `tests`, build output, and `TES5Edit` are excluded.
- Thread-safety rules live in one canonical document, with public header comments pointing consumers to that document.

## Verification

- `rg -n "find_package\(Doxygen QUIET\)|libbsa_docs|Doxyfile" CMakeLists.txt` - passed.
- `rg -n "INPUT = .*include/libbsa|EXCLUDE = .*src.*TES5Edit|GENERATE_HTML = YES|WARN_IF_UNDOCUMENTED = YES" docs/Doxyfile.in` - passed.
- `rg -n "@mainpage|archive_reader|extract_entries|write_execution_options|validate_archive" docs/api-mainpage.md` - passed.
- `rg -n "## archive_reader|## payload_sink|## bulk_extract_sink_factory|## tes3_bsa_writer|## tes4_bsa_writer|## ba2_gnrl_writer|## ba2_dx10_writer|## validate_archive|## libbsa_benchmarks" docs/thread-safety.md` - passed.
- `rg -n "Thread-safety" include/libbsa/archive.hpp include/libbsa/writer.hpp include/libbsa/validation.hpp` - passed.
- `rg -n "docs_policy|Doxyfile|TES5Edit|WARN_IF_UNDOCUMENTED" tests/unit/docs_policy_tests.cpp` - passed.
- `rg -n "thread_safety_policy|bulk_extract_sink_factory|write_execution_options|distinct sink|no global mutable state" tests/unit/thread_safety_docs_policy_tests.cpp` - passed.
- `cmake --preset windows-msvc-debug-static` - passed during Task 1 verification.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- `ctest --preset windows-msvc-debug-static -R "docs_policy|thread_safety_policy|public_include_boundary" --output-on-failure` - passed, 10/10 tests.
- `pwsh -NoProfile -Command "if (Get-Command doxygen -ErrorAction SilentlyContinue) { cmake --build --preset windows-msvc-debug-static --target libbsa_docs } else { Write-Output 'Doxygen not installed; optional docs build skipped as designed' }"` - Doxygen absent; optional docs build skipped as designed.
- `git status --short TES5Edit` - no output.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## Known Stubs

None. Stub scan found no TODO/FIXME/placeholder/coming-soon text in the created or modified files. The only `not available` text is the plan-required CMake status message for missing optional Doxygen.

## Threat Flags

None - new documentation target and tests stay within the plan threat model for Doxygen input scope, missing local Doxygen, and D-23 thread-safety coverage.

## Auth Gates

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 12-07 can proceed to consumer examples and target-format guidance. Public API docs and thread-safety policy coverage are now in place for the remaining documentation work.

## Self-Check: PASSED

- Verified key created files exist.
- Verified task commits `86101e9`, `a3473c9`, and `e2d54ee` are reachable.
- Verified `git status --short TES5Edit` returned no output during plan-level verification.

---
*Phase: 12-performance-concurrency-documentation-and-polish*
*Completed: 2026-05-10*
