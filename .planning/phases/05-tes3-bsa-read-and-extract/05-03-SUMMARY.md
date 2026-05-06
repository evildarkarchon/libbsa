---
phase: 05-tes3-bsa-read-and-extract
plan: 03
subsystem: bsa-reader
tags: [tes3, bsa, hardening, malformed-input]
requires: [05-02, BSA-04]
provides: [TES3 malformed input coverage]
affects: [tests/bsa_reader_tests.cpp]
tech-stack:
  added: []
  patterns: [structured malformed_archive failures, bounded payload range validation]
key-files:
  created: []
  modified:
    - tests/bsa_reader_tests.cpp
decisions:
  - "Plan 05-01 parser bounds checks already reject malformed TES3 table/name/hash ranges; Plan 05-03 adds regression coverage."
metrics:
  duration: 4min
  completed: 2026-05-06T00:00:00Z
---

# Phase 05 Plan 03: TES3 Malformed Archive Hardening Summary

Malformed TES3/Morrowind BSA cases are now covered with fixture mutations for truncated tables, invalid name offsets, invalid hash offsets, and impossible payload ranges.

## Completed Tasks

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Add malformed TES3 table and payload tests | 1ec39dd | `tests/bsa_reader_tests.cpp` |
| 2 | Tighten TES3 bounds checks until malformed tests pass | N/A | Existing Plan 05-01 bounds checks already passed the new malformed coverage |

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_bsa_reader_tests` — passed
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` — passed (22/22)

## Deviations from Plan

None - no production-code change was needed after the hardening tests passed.

## TDD Gate Compliance

- RED for Task 1 did not fail because Plan 05-01's TES3 parser already used bounded reads and range checks. The test-only commit preserves the malformed-input contract.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- Verified modified test file exists.
- Verified commit `1ec39dd` exists in git history.
