---
phase: 05-tes3-bsa-read-and-extract
plan: 01
subsystem: bsa-reader
tags: [tes3, bsa, metadata, tdd]
requires: [BSA-04]
provides: [TES3 BSA metadata parsing]
affects: [include/libbsa/bsa.hpp, src/bsa_reader.hpp, src/bsa_reader.cpp, tests/bsa_reader_tests.cpp]
tech-stack:
  added: []
  patterns: [bounded binary parsing, data-section-relative offset conversion]
key-files:
  created: []
  modified:
    - tests/bsa_reader_tests.cpp
    - include/libbsa/bsa.hpp
    - src/bsa_reader.hpp
    - src/bsa_reader.cpp
decisions:
  - "TES3 archives reuse bsa_archive/open_bsa with absolute entry_metadata offsets derived from data-section-relative records."
metrics:
  duration: 8min
  completed: 2026-05-06T00:00:00Z
---

# Phase 05 Plan 01: TES3 Metadata Parsing Summary

TES3/Morrowind BSA metadata parsing now routes through `open_bsa`, preserving the existing public BSA API while converting TES3 data-section-relative records into absolute payload offsets.

## Completed Tasks

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Add generated TES3 metadata fixture tests | 769333e | `tests/bsa_reader_tests.cpp` |
| 2 | Implement TES3 metadata parsing in open_bsa | 2345bea | `include/libbsa/bsa.hpp`, `src/bsa_reader.hpp`, `src/bsa_reader.cpp` |

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_bsa_reader_tests` — passed
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` — passed (16/16)

## Deviations from Plan

None - plan executed as written.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- Verified modified implementation and test files exist.
- Verified commits `769333e` and `2345bea` exist in git history.
