---
phase: 07-tes4-family-bsa-write-new-support
status: issues_found
depth: standard
files_reviewed: 7
findings:
  critical: 0
  warning: 1
  info: 0
  total: 1
created: 2026-05-09
---

# Phase 07 Code Review

## Scope

- `CMakeLists.txt`
- `include/libbsa/libbsa.hpp`
- `include/libbsa/writer.hpp`
- `src/formats/bsa/tes4_bsa_writer.cpp`
- `src/formats/bsa/tes4_bsa_writer.hpp`
- `tests/CMakeLists.txt`
- `tests/unit/public_include_boundary_tests.cpp`
- `tests/unit/tes4_bsa_writer_tests.cpp`

## Findings

### WR-01: `write_to` can throw from filesystem existence checks instead of returning `result<void>`

- Severity: Warning
- File: `src/formats/bsa/tes4_bsa_writer.cpp:618`
- Category: API contract / error handling

`write_tes4_bsa_archive` calls `std::filesystem::exists(output_path)` without the `std::error_code` overload. On filesystem errors such as malformed host paths, inaccessible path components, permission failures during status lookup, or some platform-specific path failures, this overload throws `std::filesystem::filesystem_error`.

The public writer contract in `include/libbsa/writer.hpp` documents that expected I/O and format failures are returned through `result<void>` rather than thrown. This unchecked call can leak exceptions through `tes4_bsa_writer::write_to`, bypassing the library error model and surprising embedders that do not wrap ordinary archive writes in exception handling.

Recommended fix: use `std::filesystem::exists(output_path, fs_error)` and translate `fs_error` to `error_code::io_error` before continuing. Add a focused test for an output path whose parent cannot be status-queried or otherwise exercises the non-throwing filesystem path.

## Notes

- No critical findings were identified in the reviewed writer serialization, compression routing, embedded-name, or deduplication logic.
- Existing tests cover the happy path, duplicate path rejection, overwrite protection, missing disk sources, compression overrides, embedded names, and opt-in stored-payload dedupe.
- Residual risk: the test suite does not currently exercise filesystem status failures on output paths, so this error-model gap can regress unnoticed.
