---
status: complete
quick_id: 260510-8aa
completed: 2026-05-10
---

# Quick Task 260510-8aa Summary

## Result

Confirmed the BA2 GNRL metadata-overlap review finding against TES5Edit/BSArchPro behavior and made libbsa reject non-empty GNRL payload spans that intersect the fixed header or record table.

## Research

TES5Edit reads GNRL record offsets directly and then seeks to those offsets during extraction, but its writer reserves the fixed header and record table before any payload bytes, refuses GNRL records whose `Offset` is zero, writes payloads after that metadata prefix, and writes the filename table afterward. That means offsets into `[0, records_end)` are not a supported compatibility case; in libbsa's strict parser they should fail before a usable reader is materialized.

## Changes

- Updated `src/formats/ba2/ba2_gnrl_parser.cpp` so entry materialization receives `records_end` and rejects stored payload spans that overlap `[0, records_end)`.
- Added a regression test in `tests/unit/ba2_gnrl_reader_tests.cpp` for a non-empty raw GNRL payload at offset zero.

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`
- Red test before parser fix: `ctest --preset windows-msvc-debug-static -R "ba2_gnrl_detector rejects non-empty payload spans in fixed metadata" --output-on-failure`
- `ctest --preset windows-msvc-debug-static -R "ba2_gnrl_detector rejects non-empty payload spans in fixed metadata|ba2_gnrl_end_table opens archives with payloads before the filename table|ba2_gnrl_malformed manifest cases" --output-on-failure`
- `ctest --preset windows-msvc-debug-static -R "ba2_gnrl|validation_api|compatibility_matrix" --output-on-failure`
- `ctest --preset windows-msvc-debug-static -R validate_fixture_manifests --output-on-failure`
- `git diff --check`
- `git status --short TES5Edit`

All final verification passed, and `TES5Edit/` remained untouched.
