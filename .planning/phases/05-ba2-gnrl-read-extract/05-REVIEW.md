---
phase: 05-ba2-gnrl-read-extract
reviewed: 2026-05-08T23:11:48Z
depth: standard
files_reviewed: 13
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/archive.hpp
  - src/archive.cpp
  - src/formats/ba2/ba2_format_detector.hpp
  - src/formats/ba2/ba2_format_detector.cpp
  - src/formats/ba2/ba2_gnrl_parser.hpp
  - src/formats/ba2/ba2_gnrl_parser.cpp
  - src/formats/ba2/ba2_gnrl_reader.hpp
  - src/formats/ba2/ba2_gnrl_reader.cpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp
  - tests/unit/ba2_gnrl_reader_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
findings:
  critical: 0
  warning: 3
  info: 0
  total: 3
status: issues_found
---

# Phase 05: Code Review Report

**Reviewed:** 2026-05-08T23:11:48Z
**Depth:** standard
**Files Reviewed:** 13
**Status:** issues_found

## Summary

Reviewed the BA2 GNRL public API additions, detector/parser/reader implementation, fixture generator, and tests. The main defects are in parser robustness: opening a BA2 currently reads the entire payload region into memory, parsed Starfield compression fields can diverge from the detected codec metadata, and payload-span validation can be bypassed on 32-bit builds through a narrowing cast.

## Warnings

### WR-01: BA2 open reads the entire payload region into memory

**Classification:** WARNING
**File:** `src/formats/ba2/ba2_gnrl_parser.cpp:356-366`
**Issue:** `parse_ba2_gnrl_archive_file()` reads from `FileTableOffset` through end-of-file and appends those bytes to `metadata_bytes` before parsing names. For real BA2 archives this includes all payload data, so merely opening/listing a multi-GB archive can allocate multi-GB memory and fail before extraction. This violates the project constraint that large archives use streaming/bounded scratch buffers and makes valid large Starfield/Fallout 4 archives unreliable to open.
**Fix:** Read only the bounded filename table, not payload bytes. One approach is to read the header/record table first, compute the earliest payload offset from non-empty records, and cap the name-table read to `[FileTableOffset, first_payload_offset)` before calling the name parser.

```cpp
// After reading metadata_bytes and records, bound the filename table by the first payload.
std::uint64_t first_payload_offset = archive_size;
for (const auto& record : records) {
  const auto stored_size = record.packed_size != 0U ? record.packed_size : record.size;
  if (stored_size != 0U) {
    first_payload_offset = std::min(first_payload_offset, record.offset);
  }
}
if (first_payload_offset < header.value().file_table_offset) {
  return error{error_code::format_error, "BA2 GNRL filename table overlaps payload data"};
}
const auto name_table_size = checked_size(first_payload_offset - header.value().file_table_offset,
                                          "BA2 GNRL filename table")
                             .value();
auto name_table = read_file_bytes_at(input, header.value().file_table_offset,
                                     name_table_size, "BA2 GNRL filename table");
```

### WR-02: Parsed Starfield compression fields are not validated against detector metadata

**Classification:** WARNING
**File:** `src/formats/ba2/ba2_gnrl_parser.cpp:266-310`
**Issue:** The parser only checks that the parsed header version and file count match the detector result. It then stores `header.value().ba2` in public metadata but uses `detected.default_compression` to assign every compressed entry's codec. If the file changes between prefix detection and the full parse, or an internal caller passes stale/inconsistent `detected_ba2_format`, Starfield v3 metadata can report `compression_method == 3` while entries are routed as deflate, or vice versa. That produces incorrect extraction errors for valid bytes and misleading public metadata.
**Fix:** Validate all version-gated BA2 header fields against the detector before materializing entries, or derive `default_compression` from the parsed header in a single place.

```cpp
if (header.value().version != detected.version ||
    header.value().file_count != detected.file_count ||
    header.value().ba2.starfield_unknown1 != detected.ba2.starfield_unknown1 ||
    header.value().ba2.starfield_unknown2 != detected.ba2.starfield_unknown2 ||
    header.value().ba2.compression_method != detected.ba2.compression_method) {
  return error{error_code::format_error, "BA2 GNRL detected header does not match parsed header"};
}
```

Add a regression test that feeds `parse_ba2_gnrl_archive()` inconsistent Starfield v3 `detected_ba2_format` metadata and expects `format_error`.

### WR-03: Payload-span validation narrows 64-bit offsets before checking bounds

**Classification:** WARNING
**File:** `src/formats/ba2/ba2_gnrl_parser.cpp:201-224`
**Issue:** `materialize_entries()` accepts `archive_size` as `std::size_t` and validates payload spans with `span_fits(static_cast<std::size_t>(records[index].offset), ...)`. On 32-bit builds, a malformed 64-bit BA2 payload offset above `SIZE_MAX` truncates before the bounds check and can be accepted as an in-range span. The parser should reject malformed archives during open instead of deferring failure to extraction or exposing invalid entry metadata.
**Fix:** Keep BA2 archive size and offsets in `std::uint64_t` for archive-format validation, and only narrow after the checked archive offset/size has been proven safe for a host operation.

```cpp
bool span_fits_u64(std::uint64_t start, std::uint64_t length, std::uint64_t total) noexcept {
  return start <= total && length <= total - start;
}

result<std::vector<entry_metadata>> materialize_entries(std::uint64_t archive_size,
                                                        std::uint64_t name_table_end,
                                                        std::span<const gnrl_record> records,
                                                        std::span<const std::string> names,
                                                        detected_ba2_format detected) {
  // ...
  const auto stored_size = records[index].packed_size != 0U ? records[index].packed_size : records[index].size;
  if (!span_fits_u64(records[index].offset, stored_size, archive_size)) {
    return error{error_code::format_error, "BA2 GNRL entry payload span is outside the archive"};
  }
  // ...
}
```

---

_Reviewed: 2026-05-08T23:11:48Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
