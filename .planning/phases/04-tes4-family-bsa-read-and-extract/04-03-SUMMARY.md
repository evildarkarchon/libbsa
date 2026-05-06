---
phase: 04-tes4-family-bsa-read-and-extract
plan: 03
subsystem: bsa-reader
tags: [bsa, extraction, compression]
requires: [04-02, compression-dispatcher]
provides: [extract_bsa_entry]
affects: [src/bsa_reader.cpp, tests/bsa_reader_tests.cpp]
tech_stack:
  added: []
  patterns: [dispatcher-routing, embedded-prefix-skip]
key_files:
  created: []
  modified: [src/bsa_reader.cpp, tests/bsa_reader_tests.cpp]
decisions:
  - Extraction uses existing compression dispatcher APIs instead of including native codec headers.
metrics:
  duration: 4min
  completed: 2026-05-06
---

# Phase 04 Plan 03: BSA Payload Extraction Summary

Raw, deflate-compressed, frame-compressed, and embedded-name TES4-family BSA entries extract through caller-owned sinks.

## Completed Tasks

| Task | Result | Commit |
|------|--------|--------|
| Add extraction behavior tests for raw, compressed, and embedded entries | Added fixture-backed extraction tests for raw v103, compressed v104/v105, and embedded names. | dcb1048 |
| Implement extraction with prefix handling and dispatcher routing | Implemented `extract_bsa_entry`, payload range reads, embedded-name skipping, codec resolution, decompression, and sink writes. | 27bddd7 |

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] Added payload range validation during extraction**
- **Found during:** Task 2
- **Issue:** Extraction needs to validate `offset + stored_size` before reading untrusted archive bytes.
- **Fix:** Added bounded payload read helper returning `BSA payload range exceeds source size`.
- **Files modified:** `src/bsa_reader.cpp`
- **Commit:** 27bddd7

## Known Stubs

None.

## Self-Check: PASSED

- Files exist: `src/bsa_reader.cpp`, `tests/bsa_reader_tests.cpp`.
- Commits exist: `dcb1048`, `27bddd7`.
