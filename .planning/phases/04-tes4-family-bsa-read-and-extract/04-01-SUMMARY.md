---
phase: 04-tes4-family-bsa-read-and-extract
plan: 01
subsystem: bsa-reader
tags: [bsa, public-api, fixtures]
requires: [archive_view, io, result]
provides: [bsa_archive, open_bsa, extract_bsa_entry, bsa-fixtures]
affects: [include/libbsa, src, tests, cmake]
tech_stack:
  added: []
  patterns: [archive_view-delegation, in-memory-fixtures]
key_files:
  created: [include/libbsa/bsa.hpp, src/bsa_reader.hpp, src/bsa_reader.cpp, tests/bsa_reader_tests.cpp]
  modified: [CMakeLists.txt, include/libbsa/archive.hpp]
decisions:
  - BSA public metadata lookup delegates to archive_view to reuse normalized path behavior.
metrics:
  duration: 3min
  completed: 2026-05-06
---

# Phase 04 Plan 01: BSA Contracts and Fixtures Summary

Public TES4-family BSA contracts with reusable in-memory fixture helpers and a dedicated Catch2 target.

## Completed Tasks

| Task | Result | Commit |
|------|--------|--------|
| Add public BSA contracts and fixture-builder tests | Added `bsa_archive`, `open_bsa`, `extract_bsa_entry`, fixture constants, and the `libbsa_bsa_reader_tests` target. | 40a2a0b |
| Add private parser constants and source-list wiring | Added private BSA constants plus source scaffolding that delegates metadata lookup to `archive_view`. | 8c68a79 |

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`

## Deviations from Plan

### Auto-fixed Issues

None - plan executed as written.

## Known Stubs

None remaining. Temporary unsupported stubs from the contract task were replaced by later plan implementations.

## Self-Check: PASSED

- Files exist: `include/libbsa/bsa.hpp`, `src/bsa_reader.hpp`, `src/bsa_reader.cpp`, `tests/bsa_reader_tests.cpp`.
- Commits exist: `40a2a0b`, `8c68a79`.
