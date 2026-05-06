---
phase: 06-ba2-gnrl-read-and-extract
reviewed: 2026-05-06T02:49:57Z
depth: standard
files_reviewed: 6
files_reviewed_list:
  - include/libbsa/ba2.hpp
  - src/ba2_reader.cpp
  - tests/ba2_reader_tests.cpp
  - tests/public_header_smoke.cpp
  - CMakeLists.txt
  - README.md
findings:
  critical: 0
  warning: 2
  info: 0
  total: 2
status: issues_found
---

# Phase 06: Code Review Report

**Reviewed:** 2026-05-06T02:49:57Z
**Depth:** standard
**Files Reviewed:** 6
**Status:** issues_found

## Summary

Reviewed the Phase 6 BA2 public API, parser/extractor, generated fixture tests, CMake wiring, public-header smoke coverage, and README documentation. The implementation is generally bounded and keeps codec routing behind the existing dispatcher, but two malformed-archive edge cases remain: duplicate normalized names are silently collapsed, and empty archives can report an unchecked out-of-range `FileTableOffset` as valid metadata.

## Warnings

### WR-01: Duplicate BA2 names silently overwrite earlier entries

**Classification:** WARNING
**File:** `src/ba2_reader.cpp:243-265`
**Issue:** The parser normalizes each BA2 name and pushes it into `entries` without checking for duplicate normalized paths. `archive_view` then uses `insert_or_assign`, so a malformed BA2 with two records named the same path silently drops one entry while `summary.file_count` still reports the original count. That makes listing/lookup metadata inconsistent and can extract the wrong record for a duplicated path instead of failing structurally.
**Fix:** Reject duplicate normalized BA2 paths before constructing `ba2_archive`.

```cpp
#include <unordered_set>

std::unordered_set<std::string> seen_paths;
seen_paths.reserve(file_count);

// inside the metadata loop, after normalize_archive_path succeeds:
auto path = normalized.value().string();
if (!seen_paths.insert(path).second) {
    return failure<ba2_archive>({error_code::malformed_archive, "duplicate BA2 name"});
}
metadata.path = std::move(path);
```

### WR-02: Empty BA2 archives can accept impossible FileTableOffset metadata

**Classification:** WARNING
**File:** `src/ba2_reader.cpp:220-241`
**Issue:** `read_name_table` performs no reads when `file_count == 0`, and the subsequent exact-name-table validation is skipped because `records.empty()`. As a result, an empty BA2 whose header sets `FileTableOffset` beyond `source.size()` opens successfully and exposes invalid `summary.file_table_offset` metadata. Non-empty archives do not have this gap because name-table and payload-offset reads force range validation.
**Fix:** Validate `file_table_offset` independently, even for zero-entry archives, before or immediately after `read_name_table`.

```cpp
if (file_table_offset > source.size()) {
    return failure<ba2_archive>(truncated_table_error());
}

auto names = read_name_table(source, file_table_offset, file_count);
```

---

_Reviewed: 2026-05-06T02:49:57Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
