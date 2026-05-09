---
phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
reviewed: 2026-05-09T02:30:00Z
depth: standard
files_reviewed: 21
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/archive.hpp
  - src/archive.cpp
  - src/formats/ba2/ba2_format_detector.cpp
  - src/formats/ba2/ba2_dx10_parser.hpp
  - src/formats/ba2/ba2_dx10_parser.cpp
  - src/formats/ba2/ba2_dx10_reader.hpp
  - src/formats/ba2/ba2_dx10_reader.cpp
  - src/texture/dds_layout.hpp
  - src/texture/dds_layout.cpp
  - src/texture/directxtex_analyzer.hpp
  - src/texture/directxtex_analyzer.cpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp
  - tests/unit/ba2_dx10_metadata_tests.cpp
  - tests/unit/dds_layout_tests.cpp
  - tests/unit/ba2_dx10_parser_tests.cpp
  - tests/unit/ba2_dx10_extraction_tests.cpp
  - tests/unit/ba2_dx10_malformed_tests.cpp
  - tests/unit/ba2_gnrl_reader_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
findings:
  critical: 1
  warning: 3
  info: 0
  total: 4
status: issues_found
---

# Phase 06: Code Review Report

**Reviewed:** 2026-05-09T02:30:00Z
**Depth:** standard
**Files Reviewed:** 21
**Status:** issues_found

## Summary

Reviewed the Phase 06 DX10/DDS implementation, fixture generator, CMake wiring, and tests. The main production concern is a fail-open memory/DoS boundary in BA2 DX10 filename-table parsing for sparse or malicious archives. Additional warnings cover partial sink mutation before source validation and malformed-test reliability gaps.

## Critical Issues

### CR-01: DX10 open can allocate the whole payload gap as a filename table

**File:** `src/formats/ba2/ba2_dx10_parser.cpp:506-515`
**Issue:** `parse_ba2_dx10_archive_file` computes `name_table_size_u64 = first_payload_offset - FileTableOffset` and reads that entire range into memory before parsing the length-prefixed names. A validly-sized malicious/sparse archive can place the first chunk near EOF, forcing `archive_reader::open` to allocate/read gigabytes even when the actual filename table contains only a few bytes. This violates the phase's bounded-open requirement and creates an untrusted-input DoS risk.
**Fix:** Parse the filename table incrementally from `FileTableOffset`, reading exactly each UInt16 length and name for `file_count` entries, while ensuring the stream position never passes `first_payload_offset`. Do not allocate the entire gap.

```cpp
std::vector<std::string> names;
names.reserve(header.value().file_count);
std::uint64_t cursor = header.value().file_table_offset;
for (std::uint32_t i = 0; i < header.value().file_count; ++i) {
  auto len_bytes = read_file_bytes_at(input, cursor, sizeof(std::uint16_t), "BA2 DX10 filename length");
  if (!len_bytes) return len_bytes.error();
  detail::binary_reader len_reader{len_bytes.value()};
  auto len = len_reader.read_u16_le();
  if (!len) return len.error();
  cursor += sizeof(std::uint16_t);
  if (!span_fits_u64(cursor, len.value(), first_payload_offset.value())) {
    return error{error_code::format_error, "BA2 DX10 filename table overlaps payload data"};
  }
  auto name_bytes = read_file_bytes_at(input, cursor, len.value(), "BA2 DX10 filename bytes");
  if (!name_bytes) return name_bytes.error();
  names.push_back(bytes_to_string(name_bytes.value()));
  cursor += len.value();
}
```

## Warnings

### WR-01: Extraction writes a DDS header before confirming the archive can be opened

**File:** `src/formats/ba2/ba2_dx10_reader.cpp:207-215`
**Issue:** `extract_ba2_dx10_payload` writes the reconstructed DDS header to the caller sink before opening the host archive file. If the archive was deleted, moved, or became unreadable after `archive_reader::open`, extraction returns `io_error` after mutating the sink with a valid-looking partial DDS header.
**Fix:** Open and validate the input stream before building/writing the header.

```cpp
std::ifstream input{std::string{host_path}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open BA2 archive host path for DX10 extraction"};
}

auto header = texture::build_dds_dxt10_header(layout);
if (!header) return header.error();
auto wrote_header = write_all(sink, header.value());
```

### WR-02: Unknown malformed-manifest error names silently become `invalid_argument`

**File:** `tests/unit/ba2_dx10_malformed_tests.cpp:31-38`
**Issue:** `error_code_from_manifest` maps any unknown string to `invalid_argument`. A typo such as `format-eror` would not fail manifest loading; it would silently change the expected result and can mask broken malformed coverage.
**Fix:** Fail the test on unknown manifest values.

```cpp
libbsa::error_code error_code_from_manifest(std::string_view value) {
  if (value == "format_error") return libbsa::error_code::format_error;
  if (value == "unsupported") return libbsa::error_code::unsupported;
  FAIL("unknown error_code in BA2 DX10 malformed manifest: " << value);
}
```

### WR-03: Final malformed DX10 coverage omits required duplicate-path and unsupported-compression cases

**File:** `tests/unit/ba2_dx10_malformed_tests.cpp:86-94`
**Issue:** The Phase 06 closeout plan required malformed coverage for duplicate canonical paths and unsupported compression, but the DX10 required-case list only checks truncation, invalid spans, size mismatch, corrupt compressed data, decoded-size mismatch, mip gaps, and duplicate mip coverage. Duplicate-path and unsupported-compression regressions can now disappear from the generator/manifest without this test failing.
**Fix:** Add generated malformed cases and required-case assertions for duplicate canonical path and unsupported compression routing, matching the Plan 06-06 behavior matrix.

---

_Reviewed: 2026-05-09T02:30:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
