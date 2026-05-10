---
phase: 10-tes3-write-support-and-bsa-format-completeness
reviewed: 2026-05-09T00:00:00Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/writer.hpp
  - src/formats/bsa/tes3_bsa_writer.cpp
  - src/formats/bsa/tes3_bsa_writer.hpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json
  - tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp
  - tests/unit/public_include_boundary_tests.cpp
  - tests/unit/tes3_bsa_writer_tests.cpp
findings:
  critical: 2
  warning: 1
  info: 0
  total: 3
status: issues_found
---

# Phase 10: Code Review Report

**Reviewed:** 2026-05-09T00:00:00Z
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Reviewed the TES3 writer public API, implementation, build wiring, generator, manifest, and unit tests after the prior fixes. The previous temp-file collision issue is improved by using a unique temporary directory, and source validation now happens before publish-path reservation. However, the publish logic still has non-atomic path reservation races that can overwrite or strand caller-owned files on platforms where `std::filesystem::rename` replaces existing destination files. One regression test also relies on timing and a 128 MiB payload, so it can pass without proving the race is closed.

## Critical Issues

### CR-01: Non-overwrite publish can still replace a concurrently-created destination

**File:** `src/formats/bsa/tes3_bsa_writer.cpp:471-484`
**Issue:** The non-overwrite path checks `exists(output_path)` and then calls `std::filesystem::rename(temp_path, output_path)`. That check/use pair is not atomic. If another process creates `output_path` after line 476 but before line 481, C++ permits `rename` to replace an existing non-directory destination on POSIX-like platforms. This violates `overwrite_existing == false` and can destroy caller-owned data despite the earlier existence checks.
**Fix:** Publish with an atomic no-replace primitive instead of `exists` + `rename`. If the project keeps a portable C++20 surface, hide the platform-specific publish behind an internal helper and fail when the destination already exists.

```cpp
result<void> publish_without_replace(const std::filesystem::path& temp_path,
                                     const std::filesystem::path& output_path) {
#if defined(_WIN32)
  if (!MoveFileExW(temp_path.c_str(), output_path.c_str(), 0)) {
    return error{error_code::io_error, "TES3 BSA writer failed to publish output host path"};
  }
  return {};
#else
  // Prefer renameat2(..., RENAME_NOREPLACE) where available; otherwise use a
  // documented link/unlink fallback that never replaces an existing output.
#endif
}
```

### CR-02: Backup path selection is not reserved atomically and can clobber caller files

**File:** `src/formats/bsa/tes3_bsa_writer.cpp:366-381,450-455`
**Issue:** `reserve_backup_path` only checks that `<archive>.libbsa-bak-N` does not exist, then the caller later renames the original archive into that path. If another process creates the backup candidate between lines 377 and 451, `std::filesystem::rename(output_path, backup_path)` can replace that file on POSIX-like platforms. This is another caller-owned data loss path in overwrite mode.
**Fix:** Reserve the backup namespace atomically, the same way the temp publish directory is reserved. For example, create a unique backup directory first, then move the old archive inside that directory.

```cpp
result<std::filesystem::path> make_unique_backup_directory(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-bakdir-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    std::error_code fs_error;
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate / filename;
    }
    if (fs_error) {
      return error{error_code::io_error, "TES3 BSA writer failed to reserve backup directory"};
    }
  }
  return error{error_code::io_error, "TES3 BSA writer exhausted backup directory names"};
}
```

## Warnings

### WR-01: Race regression test is timing-dependent and can pass without covering the vulnerable window

**File:** `tests/unit/tes3_bsa_writer_tests.cpp:567-605`
**Issue:** The watcher only creates the destination after it sees the temporary directory. The test depends on a 128 MiB payload making `write_archive_bytes` slow enough for the watcher to win before the implementation's second `exists` check. It does not cover the critical window after line 476 and before the final `rename`, and it can become flaky or falsely reassuring across machines and filesystems.
**Fix:** Make the race deterministic by injecting a test hook/publish strategy into the internal writer path, or factor the publish operation into an internal helper that can be unit-tested with a fake filesystem/publisher. Avoid timing-based synchronization and large payloads for correctness tests.

---

_Reviewed: 2026-05-09T00:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
