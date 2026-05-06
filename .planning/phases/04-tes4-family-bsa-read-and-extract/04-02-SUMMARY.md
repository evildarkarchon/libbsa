---
phase: 04-tes4-family-bsa-read-and-extract
plan: 02
subsystem: bsa-reader
tags: [bsa, parser, metadata]
requires: [04-01]
provides: [tes4-family-table-parser]
affects: [src/bsa_reader.cpp, tests/bsa_reader_tests.cpp]
tech_stack:
  added: []
  patterns: [bounded-read-helpers, xor-compression-state]
key_files:
  created: []
  modified: [src/bsa_reader.cpp, tests/bsa_reader_tests.cpp]
decisions:
  - TES4-family compression state is derived using archive default XOR file size flag during metadata parsing.
metrics:
  duration: 5min
  completed: 2026-05-06
---

# Phase 04 Plan 02: TES4-family Table Parser Summary

Bounded metadata parsing for Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 BSA table layouts.

## Completed Tasks

| Task | Result | Commit |
|------|--------|--------|
| Add failing parser behavior tests for v103, v104, and v105 | Added fixture-backed `open_bsa` tests covering versions, normalized paths, table sizes, and compression state. | f287ce5 |
| Implement bounded TES4-family table parsing | Implemented header, folder record, folder name, file record, filename table, metadata, and unsupported-version parsing. | 0ebf483 |

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`

## Deviations from Plan

### Auto-fixed Issues

None - plan executed as written.

## Known Stubs

None.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: parser | `src/bsa_reader.cpp` | New untrusted archive table parser validates reads against `byte_source::size()`. |

## Self-Check: PASSED

- Files exist: `src/bsa_reader.cpp`, `tests/bsa_reader_tests.cpp`.
- Commits exist: `f287ce5`, `0ebf483`.
