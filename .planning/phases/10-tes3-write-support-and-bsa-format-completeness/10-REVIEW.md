---
phase: 10-tes3-write-support-and-bsa-format-completeness
reviewed: 2026-05-09T12:00:00Z
depth: standard
files_reviewed: 11
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/writer.hpp
  - src/detail/atomic_file_ops.hpp
  - src/formats/bsa/tes3_bsa_writer.cpp
  - src/formats/bsa/tes3_bsa_writer.hpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/archives/tes3_writer_canonical.bsa
  - tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json
  - tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp
  - tests/unit/public_include_boundary_tests.cpp
  - tests/unit/tes3_bsa_writer_tests.cpp
findings:
  critical: 1
  warning: 0
  info: 0
  total: 1
status: issues_found
---

# Phase 10: Code Review Report

**Reviewed:** 2026-05-09T12:00:00Z
**Depth:** standard
**Files Reviewed:** 11
**Status:** issues_found

## Summary

Reviewed the TES3 writer API, atomic publish helper, writer implementation, build wiring, generated fixture manifest/generator, and unit tests. The prior no-replace publish finding is resolved by `publish_file_without_replace`, and the prior backup-reservation finding is resolved by reserving a writer-owned backup directory before moving the existing archive. The deterministic helper tests now directly cover no-replace publish and backup directory reservation.

One overwrite-mode data-loss race remains: the implementation moves the old archive out of the destination name before publishing the replacement, then refuses to restore it if another actor creates the destination during that gap. That can leave the caller's original archive stranded in a backup directory while `write_to` reports failure.

## Critical Issues

### CR-01: Overwrite publish can strand the original archive when another writer wins the publish gap

**Classification:** BLOCKER
**File:** `src/formats/bsa/tes3_bsa_writer.cpp:439-458`
**Issue:** In overwrite mode, the writer first renames the existing archive into `backup_path` (lines 439-445), then publishes the new archive with no-replace semantics (line 447). If another process creates `output_path` between those operations, `publish_file_without_replace` correctly fails, but the rollback branch only restores the backup when `output_path` does **not** exist (lines 449-455). In the interloper case, the function returns an error while leaving the caller's original archive under the backup directory and leaving the interloper's file at the requested archive path. That is a data-loss/atomicity failure for `overwrite_existing = true`.
**Fix:** Do not remove the original destination name before publishing the replacement. Use a platform-specific atomic replace operation for overwrite mode so failure leaves the old archive at `output_path`; if rollback/backups are required, use APIs that replace while preserving a backup without an externally observable missing-destination gap.

```cpp
inline result<void> replace_file_atomically(const std::filesystem::path& temp_path,
                                           const std::filesystem::path& output_path) {
#if defined(_WIN32)
  if (!MoveFileExW(temp_path.c_str(), output_path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    return error{error_code::io_error, "failed to atomically replace output host path"};
  }
#else
  std::error_code fs_error;
  std::filesystem::rename(temp_path, output_path, fs_error); // POSIX rename replaces atomically.
  if (fs_error) {
    return error{error_code::io_error, "failed to atomically replace output host path"};
  }
#endif
  return {};
}
```

Then route the `options.overwrite_existing && output_exists` branch through that helper instead of renaming the old file to a backup before publish. Add a regression test that deterministically creates `output_path` after backup reservation but before publish, and assert the original archive is not stranded away from the requested path on failure.

---

_Reviewed: 2026-05-09T12:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
