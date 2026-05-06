---
phase: 04-tes4-family-bsa-read-and-extract
plan: 05
subsystem: bsa-reader
tags: [bsa, docs, smoke, validation]
requires: [04-04]
provides: [phase-04-validation]
affects: [tests/public_header_smoke.cpp, README.md, include/libbsa/compression.hpp]
tech_stack:
  added: []
  patterns: [consumer-smoke, boundary-gates]
key_files:
  created: []
  modified: [tests/public_header_smoke.cpp, README.md, include/libbsa/compression.hpp]
decisions:
  - README documents the local Visual Studio 2026 fallback validation commands used for Phase 04.
metrics:
  duration: 3min
  completed: 2026-05-06
---

# Phase 04 Plan 05: Public Smoke, Documentation, and Final Gates Summary

Consumer-style BSA public API coverage, README usage documentation, and final Phase 04 validation gates are complete.

## Completed Tasks

| Task | Result | Commit |
|------|--------|--------|
| Expand public-header smoke for BSA API | Public smoke now includes `libbsa/bsa.hpp`, constructs `bsa_archive`, and checks path lookup. | 333e309 |
| Document TES4-family BSA read/extract support | README documents supported BSA versions, byte source/sink use, compression XOR, embedded-name skipping, and validation commands. | 32e474e |
| Run final Phase 04 gates | Full build/test, targeted BSA, fixture, smoke, public-header token, CMake GLOB, and TES5Edit status gates passed. | verification-only |

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L fixture`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke`
- `rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex|TES5Edit" include/libbsa` returned no matches.
- `rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt` returned no matches.
- `git status --short TES5Edit` returned no output.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Public-header boundary] Rewrote a public-header comment containing uppercase codec wording**
- **Found during:** Task 2/3 boundary validation
- **Issue:** Existing `compression.hpp` documentation used forbidden public-header implementation wording matched by the Phase 04 token gate.
- **Fix:** Reworded the comment to describe raw block codec selection without implementation-token leakage.
- **Files modified:** `include/libbsa/compression.hpp`
- **Commit:** 32e474e

## Known Stubs

None.

## Self-Check: PASSED

- Files exist: `tests/public_header_smoke.cpp`, `README.md`, `include/libbsa/compression.hpp`.
- Commits exist: `333e309`, `32e474e`.
