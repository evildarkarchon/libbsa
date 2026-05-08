---
phase: 03-format-detection-and-tes4-family-bsa-read-extract
reviewed: 2026-05-08T08:47:08Z
depth: standard
files_reviewed: 22
files_reviewed_list:
  - CMakeLists.txt
  - cmake/libbsaConfig.cmake.in
  - include/libbsa/archive.hpp
  - include/libbsa/result.hpp
  - src/archive.cpp
  - src/formats/bsa/bsa_format_detector.cpp
  - src/formats/bsa/bsa_format_detector.hpp
  - src/formats/bsa/tes4_bsa_parser.cpp
  - src/formats/bsa/tes4_bsa_parser.hpp
  - src/formats/bsa/tes4_bsa_reader.cpp
  - src/formats/bsa/tes4_bsa_reader.hpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp
  - tests/fixtures/generated/validate_fixture_manifests.py
  - tests/fixtures/README.md
  - tests/package-consumer/main.cpp
  - tests/package-consumer/smoke.cmake
  - tests/unit/archive_reader_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
  - tests/unit/tes4_bsa_reader_tests.cpp
  - tests/unit/result_tests.cpp
  - vcpkg.json
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
status: clean
---

# Phase 03: Code Review Report

**Reviewed:** 2026-05-08T08:47:08Z
**Depth:** standard
**Files Reviewed:** 22
**Status:** clean

## Summary

Reviewed the Phase 3 public API, CMake/package surface, TES4-family BSA detector/parser/reader implementation, fixture generator/validator, and unit/package-consumer tests at standard depth.

The bounded parsing/extraction fix closes the prior review concerns. `archive_reader::state` stores only metadata, sorted entry metadata, and the host path; `archive_reader::open()` reads an 8-byte detection prefix, obtains file size, and parses only the bounded metadata table plus one-byte/four-byte payload prefixes needed for embedded-name and compressed-size metadata. `archive_reader::extract()` reopens the host archive and reads only the selected entry's stored payload before dispatching to the TES4-family payload extractor; there is no remaining whole-archive resident buffer path in reader state, open, or extract.

Metadata parsing now derives table size with checked multiplication/addition before allocation, validates table spans against archive size, checks folder/file count consistency before reserving record vectors, and bounds embedded-name and compressed-size prefix reads to the selected payload span. Extraction validates seek/read bounds for the selected stored payload, strips embedded-name prefixes safely, checks compressed exact-size prefixes against parsed metadata, reports partial sink writes as `io_error`, and keeps the convenience `extract_bytes()` path bounded to a single entry.

The previously reported issues remain closed: count-derived allocation is preceded by bounded table-span/count validation, folder and payload offsets are checked before use, the public `result::error()` misuse path throws `std::logic_error` instead of terminating, public archive-reader documentation matches the implemented Phase 3 behavior, and the whole-archive resident buffer in reader open/state/extract has been removed.

All reviewed files meet quality standards. No issues found.

---

_Reviewed: 2026-05-08T08:47:08Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
