---
phase: 09-bsa-writers
reviewed: 2026-05-07T00:00:00Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - CMakeLists.txt
  - README.md
  - include/libbsa/bsa.hpp
  - include/libbsa/bsa_writer.hpp
  - src/bsa_reader.cpp
  - src/bsa_writer.cpp
  - tests/bsa_reader_tests.cpp
  - tests/bsa_writer_tests.cpp
  - tests/public_header_smoke.cpp
findings:
  critical: 2
  warning: 0
  info: 0
  total: 2
status: issues_found
---

# Phase 09: Code Review Report

**Reviewed:** 2026-05-07T00:00:00Z
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Reviewed the BSA writer/reader API, implementation, CMake wiring, README updates, and generated tests. The implementation has two correctness defects: TES4-family writer headers emit an incompatible folder-name length field, and the TES4 reader validates the declared filename-table size but then ignores that bound while parsing names.

## Critical Issues

### CR-01: BLOCKER - TES4-family writer stores the wrong header value for total folder name length

**File:** `src/bsa_writer.cpp:452`
**Issue:** The TES4-family BSA header field at offset 24 is `TotalFolderNameLength`, but the writer serializes `folder_blocks_size`, which includes every folder name record plus every 16-byte file record in each folder block. The reader tests only reopen through libbsa, and `open_bsa` ignores this header field, so the generated archives round-trip internally while carrying a header value that external BSA readers/tools can reject or misinterpret. This violates the Phase 09 claim of native TES4-family headers and compatibility.
**Fix:** Track the sum of serialized folder name records separately from folder blocks and write that value to the header. Add an assertion in `tests/bsa_writer_tests.cpp` that checks header offset 24 equals only the total folder-name bytes.

```cpp
std::uint64_t folder_blocks_size = 0;
std::uint64_t folder_names_size = 0;
std::uint64_t file_names_size = 0;
// ...
if (!checked_add(folder.folder.size(), 2, folder_name_record_size) ||
    !checked_add(folder_names_size, folder_name_record_size, folder_names_size) ||
    !checked_mul(16, folder.entries.size(), file_records_size) ||
    !checked_add(folder_name_record_size, file_records_size, folder_block_size) ||
    !checked_add(folder_blocks_size, folder_block_size, folder_blocks_size) ||
    !checked_add(cursor, folder_block_size, cursor)) {
    return failure<bsa_write_plan>(writer_layout_overflow());
}

// Header field 24: total folder-name bytes, not folder block bytes.
append_u32(plan.table_bytes, static_cast<std::uint32_t>(folder_names_size));
```

### CR-02: BLOCKER - TES4 reader ignores the declared file-name table length after checking it

**File:** `src/bsa_reader.cpp:416-419`
**Issue:** `open_tes4_bsa` checks `range_fits(file_names_offset, total_file_name_length, source.size())`, but `read_file_names` receives only the start offset and file count. It then scans until it finds `file_count` NUL terminators, even if those bytes are outside the declared filename block. A malformed archive with `TotalFileNameLength == 0` but names placed immediately after the table is accepted instead of rejected, and the parser can consume payload bytes as table bytes.
**Fix:** Make filename parsing bounded by `file_names_offset + total_file_name_length`, reject unterminated names within that bound, and add a malformed test that shrinks the header's total filename length while leaving source bytes present.

```cpp
result<std::vector<std::string>> read_file_names(const byte_source& source,
                                                 std::uint64_t offset,
                                                 std::uint64_t limit,
                                                 std::uint32_t count)
{
    std::vector<std::string> names;
    names.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::string name;
        while (offset < limit) {
            auto byte = read_bytes(source, offset++, 1);
            if (!byte.has_value()) {
                return failure<std::vector<std::string>>(byte.error());
            }
            const auto ch = static_cast<char>(std::to_integer<unsigned char>(byte.value()[0]));
            if (ch == '\0') {
                names.push_back(std::move(name));
                goto next_name;
            }
            name.push_back(ch);
        }
        return failure<std::vector<std::string>>(truncated_table_error());
    next_name:;
    }
    return success(std::move(names));
}

std::uint64_t file_names_end = 0;
if (!checked_add(file_names_offset, total_file_name_length, file_names_end) ||
    file_names_end > source.size()) {
    return failure<bsa_archive>(truncated_table_error());
}
auto names = read_file_names(source, file_names_offset, file_names_end, file_count);
```

---

_Reviewed: 2026-05-07T00:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
