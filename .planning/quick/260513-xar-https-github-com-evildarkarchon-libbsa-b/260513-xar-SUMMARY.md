---
status: complete
quick_id: 260513-xar
completed: 2026-05-14
---

# Quick Task 260513-xar Summary

## Result

Decoded public UTF-8 host paths to native Windows filesystem paths at the shared resolver boundary so reader open, validation, and reopen flows no longer depend on the process ANSI code page.

## Research

The safe Windows/MSVC seam is `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)` into a wide `std::filesystem::path`, while keeping the original UTF-8 bytes untouched for diagnostics.

## Changes

- Added `tests/unit/host_file_path_tests.cpp` and registered it in `tests/CMakeLists.txt` to lock the seam behavior.
- Added a deterministic source-policy assertion for strict UTF-8 decoding and a malformed UTF-8 regression that failed before the fix.
- Updated `src/detail/host_file_path.cpp` to decode UTF-8 explicitly with `MultiByteToWideChar`, preserve `original_utf8`, reject malformed bytes with `error_code::invalid_argument`, and avoid narrow `std::filesystem::path` construction.

## Task-Level Commits

- `cb9684c` `test(260513-xar): lock UTF-8 host path seam behavior`
- `cb1caeb` `fix(260513-xar): decode host paths as UTF-8 on Windows`

## Verification

- Red phase: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex host_file_path`
- Green phase: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "host_file_path|host_path_correctness_boundary"`

All final verification passed, and no files under `TES5Edit/` were touched.
