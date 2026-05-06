---
phase: 04-tes4-family-bsa-read-and-extract
plan: 04
subsystem: bsa-reader
tags: [bsa, hardening, compatibility]
requires: [04-03]
provides: [malformed-input-regression-tests]
affects: [tests/bsa_reader_tests.cpp]
tech_stack:
  added: []
  patterns: [structured-errors, provenance-comments]
key_files:
  created: []
  modified: [tests/bsa_reader_tests.cpp, src/bsa_reader.cpp]
decisions:
  - Compatibility regression tests cite TES5Edit provenance without reading the submodule at runtime.
metrics:
  duration: 2min
  completed: 2026-05-06
---

# Phase 04 Plan 04: Parser and Extractor Hardening Summary

Malformed archive and compatibility regression coverage locks structured failures for TES4-family BSA parsing/extraction.

## Completed Tasks

| Task | Result | Commit |
|------|--------|--------|
| Add malformed archive and compatibility regression tests | Added tests for unsupported versions, truncated tables, impossible payload ranges, XOR compression, and embedded-name truncation. | ed30ced |
| Implement structured hardening paths | Already satisfied by Plan 02/03 bounded parsing and extraction helpers; verification passed with no extra source changes. | 0ebf483, 27bddd7 |

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] Hardening implementation was completed early**
- **Found during:** Plan 04 Task 1
- **Issue:** The newly added hardening tests passed immediately because payload/table validation had already been added while implementing extraction correctness.
- **Fix:** Recorded the behavior as early mitigation rather than adding redundant source changes.
- **Files modified:** `tests/bsa_reader_tests.cpp`
- **Commit:** ed30ced

## TDD Gate Compliance

- RED gate warning: Plan 04 hardening tests passed immediately because the mitigation was implemented in earlier commits (`0ebf483`, `27bddd7`).
- GREEN gate source changes were therefore not repeated.

## Known Stubs

None.

## Self-Check: PASSED

- Files exist: `tests/bsa_reader_tests.cpp`, `src/bsa_reader.cpp`.
- Commits exist: `ed30ced`, `0ebf483`, `27bddd7`.
