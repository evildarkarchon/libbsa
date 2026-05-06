---
phase: 05-tes3-bsa-read-and-extract
plan: 04
subsystem: bsa-reader
tags: [tes3, docs, smoke, validation]
requires: [05-03, BSA-04]
provides: [TES3 public documentation, public BSA smoke coverage, final validation]
affects: [README.md, tests/public_header_smoke.cpp]
tech-stack:
  added: []
  patterns: [public-header boundary gate, TES5Edit clean-status gate]
key-files:
  created: []
  modified:
    - README.md
    - tests/public_header_smoke.cpp
decisions:
  - "Document TES3 offsets as data-section-relative on disk while exposing absolute offsets in entry_metadata."
metrics:
  duration: 5min
  completed: 2026-05-06T00:00:00Z
---

# Phase 05 Plan 04: TES3 Documentation and Validation Summary

Consumer-facing documentation now states TES3/Morrowind BSA read/extract support, and the public header smoke test references the BSA open/extract API without private headers.

## Completed Tasks

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Document TES3 BSA support and compatibility constraints | e6fb810 | `README.md` |
| 2 | Expand public smoke and run final boundary gates | f82c635 | `tests/public_header_smoke.cpp` |

## Verification

- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` — passed
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` — passed (22/22)
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_path_hash_tests` — passed (5/5)
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` — passed (60/60)
- `rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex|TES5Edit" include/libbsa` — no matches
- `rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt` — no matches
- `git status --short TES5Edit` — no output

## Deviations from Plan

None - plan executed as written.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- Verified modified documentation and smoke files exist.
- Verified commits `e6fb810` and `f82c635` exist in git history.
