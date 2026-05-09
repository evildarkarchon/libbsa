---
phase: 08-ba2-gnrl-write-new-support
reviewed: 2026-05-09T08:29:06Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/writer.hpp
  - src/formats/ba2/ba2_gnrl_parser.cpp
  - src/formats/ba2/ba2_gnrl_writer.cpp
  - src/formats/ba2/ba2_gnrl_writer.hpp
  - tests/CMakeLists.txt
  - tests/unit/ba2_gnrl_reader_tests.cpp
  - tests/unit/ba2_gnrl_writer_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
findings:
  critical: 2
  warning: 1
  info: 0
  total: 3
status: issues_found
---

# Phase 08: Code Review Report

**Reviewed:** 2026-05-09T08:29:06Z
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Reviewed the BA2 GNRL writer public API, parser changes, build/test integration, and related unit coverage. The implementation contains two BLOCKER-level risks: malformed BA2 archives can point payloads into metadata and still be accepted, and the writer can delete unrelated or existing output-path files during its temporary publish flow. There is also one WARNING where filesystem errors can escape the public `result<void>` API as exceptions.

## Critical Issues

### CR-01: BLOCKER — Parser accepts payloads overlapping the BA2 header/record table

**File:** `src/formats/ba2/ba2_gnrl_parser.cpp:280-286`

**Issue:** `materialize_entries` validates that each payload span is inside the archive and does not overlap the filename table, but it never rejects payload offsets inside the fixed header or GNRL record table. A malicious or malformed archive can set `Offset` to `0` or another metadata byte range, open successfully, and later extract header/record bytes as file data. The parser already computes `records_end` at lines 328-332 and 404-408, but that boundary is not passed into `materialize_entries`.

**Fix:** Pass `records_end`/metadata-end into `materialize_entries` and reject any non-empty payload span that intersects `[0, records_end)` as well as the filename table.

```cpp
result<std::vector<entry_metadata>> materialize_entries(
    std::size_t archive_size,
    std::uint64_t metadata_end,
    std::uint64_t name_table_offset,
    std::uint64_t name_table_end,
    std::span<const gnrl_record> records,
    std::span<const std::string> names,
    detected_ba2_format detected) {
  // ...
  if (spans_overlap_u64(records[index].offset, stored_size, 0U, metadata_end)) {
    return error{error_code::format_error, "BA2 GNRL payload data intersects metadata"};
  }
  if (spans_overlap_u64(records[index].offset, stored_size,
                        name_table_offset, name_table_end - name_table_offset)) {
    return error{error_code::format_error, "BA2 GNRL filename table intersects payload data"};
  }
}
```

### CR-02: BLOCKER — Temporary publish flow can delete caller data

**File:** `src/formats/ba2/ba2_gnrl_writer.cpp:578-596`

**Issue:** `write_ba2_gnrl_archive` unconditionally removes `output_path + ".tmp"` before writing, which can delete an unrelated caller file that happens to use that name. When `overwrite_existing` is true, it also removes the destination before `rename`; if the subsequent publish fails, the original archive has already been lost. This is a data-loss risk in a public write API.

**Fix:** Use a unique temporary path created exclusively in the destination directory, never delete a pre-existing temp-name collision, and avoid removing the destination until the replacement operation can be completed or rolled back.

```cpp
auto temp_path = output_path;
temp_path += ".tmp." + unique_suffix();
if (std::filesystem::exists(temp_path, fs_error) || fs_error) {
  return error{error_code::io_error, "BA2 GNRL writer failed to allocate temporary output path"};
}

auto written = write_archive_bytes(target, options, prepared.value(), version, file_table_offset, temp_path);
if (!written) {
  std::filesystem::remove(temp_path, fs_error);
  return written.error();
}

// Prefer a platform replace/rename path that preserves the old archive if publish fails.
auto published = replace_file_preserving_existing(temp_path, output_path, options.overwrite_existing);
if (!published) {
  std::filesystem::remove(temp_path, fs_error);
  return published.error();
}
```

## Warnings

### WR-01: WARNING — Filesystem exceptions can escape the structured error API

**File:** `src/formats/ba2/ba2_gnrl_writer.cpp:556-558`

**Issue:** The writer calls the throwing `std::filesystem::exists(output_path)` overload. Permission errors, invalid paths, or other filesystem failures can throw `std::filesystem::filesystem_error` through `ba2_gnrl_writer::write_to`, despite the public API contract documenting I/O and validation failures as `result<void>` errors.

**Fix:** Use the `std::error_code` overload for filesystem operations at the public API boundary and translate failures to `error_code::io_error`.

```cpp
std::error_code fs_error;
const bool output_exists = std::filesystem::exists(output_path, fs_error);
if (fs_error) {
  return error{error_code::io_error, "BA2 GNRL writer failed to inspect output host path"};
}
if (!options.overwrite_existing && output_exists) {
  return error{error_code::io_error, "BA2 GNRL output host path already exists"};
}
```

---

_Reviewed: 2026-05-09T08:29:06Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
