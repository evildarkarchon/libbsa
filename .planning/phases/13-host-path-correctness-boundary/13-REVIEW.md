---
phase: 13-host-path-correctness-boundary
reviewed: 2026-05-13T00:00:00Z
depth: standard
files_reviewed: 33
files_reviewed_list:
  - src/detail/host_path.hpp
  - src/detail/host_path.cpp
  - src/detail/host_file.hpp
  - src/detail/host_file.cpp
  - src/archive.cpp
  - src/validation.cpp
  - src/formats/bsa/tes3_bsa_reader.hpp
  - src/formats/bsa/tes3_bsa_reader.cpp
  - src/formats/bsa/tes4_bsa_reader.hpp
  - src/formats/bsa/tes4_bsa_reader.cpp
  - src/formats/ba2/ba2_gnrl_reader.hpp
  - src/formats/ba2/ba2_gnrl_reader.cpp
  - src/formats/ba2/ba2_dx10_reader.hpp
  - src/formats/ba2/ba2_dx10_reader.cpp
  - src/formats/bsa/tes4_bsa_prepare.hpp
  - src/formats/bsa/tes4_bsa_prepare.cpp
  - src/formats/ba2/ba2_gnrl_prepare.hpp
  - src/formats/ba2/ba2_gnrl_prepare.cpp
  - src/formats/bsa/tes4_bsa_layout.hpp
  - src/formats/bsa/tes4_bsa_layout.cpp
  - src/formats/ba2/ba2_gnrl_layout.hpp
  - src/formats/ba2/ba2_gnrl_layout.cpp
  - src/formats/bsa/tes4_bsa_serialize.cpp
  - src/formats/ba2/ba2_gnrl_serialize.cpp
  - tests/unit/host_file_tests.cpp
  - tests/unit/archive_reader_tests.cpp
  - tests/unit/validation_api_tests.cpp
  - tests/unit/tes3_bsa_reader_tests.cpp
  - tests/unit/tes4_bsa_writer_tests.cpp
  - tests/unit/ba2_gnrl_reader_tests.cpp
  - tests/unit/ba2_gnrl_writer_tests.cpp
  - tests/unit/ba2_dx10_extraction_tests.cpp
  - tests/unit/writer_stage_tests.cpp
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
status: issues_found
---

# Phase 13: Code Review Report

**Reviewed:** 2026-05-13T00:00:00Z
**Depth:** standard
**Files Reviewed:** 33
**Status:** issues_found

## Summary

Review is **not clean**. The new host-path boundary checks catch rename/replace cases, but the identity model is still too weak to detect same-file, same-size overwrites. That leaves both reader extraction and bounded-memory writer streaming vulnerable to silently consuming modified bytes.

## Critical Issues

### CR-01: BLOCKER - host-file identity does not detect in-place same-size rewrites

**File:** `src/detail/host_file.hpp:27-33`, `src/detail/host_file.cpp:152-174`
**Issue:** `host_file_identity` only records file size, file index, and volume serial number. If a file is modified in place without changing its length, all three values can stay the same, so `validate_host_file_identity()` reports success even though the bytes changed. That breaks the safety guarantee relied on by archive opening/extraction (`src/archive.cpp:164-168, 190-206`) and by raw-disk writer paths (`src/formats/bsa/tes4_bsa_prepare.cpp:184-188`, `src/formats/ba2/ba2_gnrl_prepare.cpp:64-68`, `src/formats/bsa/tes4_bsa_serialize.cpp:83-96`, `src/formats/ba2/ba2_gnrl_serialize.cpp:104-120`). A concurrent same-size overwrite can therefore silently change extracted payloads or published archive contents.
**Fix:** Extend the captured identity with modification metadata from the opened file handle and compare it on every validation path. On Windows, `ftLastWriteTime`/`FILE_BASIC_INFO::LastWriteTime` is the minimum needed signal; using `GetFileInformationByHandleEx(FileIdInfo)` plus `FileBasicInfo` is stronger.

```cpp
struct host_file_identity {
  std::uint64_t size{};
  std::uint64_t file_index{};
  std::uint32_t volume_serial_number{};
  std::uint64_t last_write_time{};
};

return host_file_identity{
    file_size,
    file_index,
    info.dwVolumeSerialNumber,
    (static_cast<std::uint64_t>(info.ftLastWriteTime.dwHighDateTime) << 32U) |
        static_cast<std::uint64_t>(info.ftLastWriteTime.dwLowDateTime)};
```

## Warnings

### WR-01: WARNING - regression coverage still misses the same-size in-place mutation case

**File:** `tests/unit/host_file_tests.cpp:169-206`, `tests/unit/archive_reader_tests.cpp:106-158`, `tests/unit/writer_stage_tests.cpp:425-500`
**Issue:** The added coverage exercises rename/replacement and grow/shrink scenarios, but it never mutates the same file in place while keeping the length unchanged. Because the production identity check only keys on size plus file ID, this gap lets the blocker above ship undetected.
**Fix:** Add a regression that overwrites an already-opened/prepared file in place with different same-length bytes and assert that extraction/streaming fails with the shared `changed_error` diagnostic.

---

_Reviewed: 2026-05-13T00:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
