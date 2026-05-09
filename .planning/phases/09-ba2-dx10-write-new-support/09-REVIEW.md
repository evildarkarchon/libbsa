---
phase: 09-ba2-dx10-write-new-support
reviewed: 2026-05-09T00:00:00Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/writer.hpp
  - src/formats/ba2/ba2_dx10_writer.cpp
  - src/formats/ba2/ba2_dx10_writer.hpp
  - src/texture/dds_layout.cpp
  - src/texture/dds_layout.hpp
  - src/texture/directxtex_analyzer.cpp
  - src/texture/directxtex_analyzer.hpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp
  - tests/fixtures/generated/source/ba2_dx10_array_bc5_unorm_2slice.dds
  - tests/fixtures/generated/source/ba2_dx10_cubemap_bc1_unorm_6face.dds
  - tests/fixtures/generated/source/ba2_dx10_multi_mip_bc7_unorm.dds
  - tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json
  - tests/unit/ba2_dx10_writer_tests.cpp
  - tests/unit/dds_layout_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
findings:
  critical: 1
  warning: 2
  info: 0
  total: 3
status: issues_found
---

# Phase 09: Code Review Report

**Reviewed:** 2026-05-09T00:00:00Z
**Depth:** standard
**Files Reviewed:** 17
**Status:** issues_found

## Summary

Reviewed the BA2 DX10 writer implementation, DDS layout/analyzer code, fixture generator, manifest, and unit tests. The changed DDS artifacts are binary and were not deeply decoded, but their intended semantics were checked against the generator, manifest, and tests. I found one data-loss risk in publish semantics and two robustness/test-coverage issues.

## Critical Issues

### CR-01: `overwrite_existing = false` can still overwrite a concurrently-created destination on POSIX

**Classification:** BLOCKER
**File:** `src/formats/ba2/ba2_dx10_writer.cpp:666-672,744`

**Issue:** `write_ba2_dx10_archive` checks `exists(output_path)` before writing the temp archive, then later publishes with `std::filesystem::rename(temp_path, output_path)`. On POSIX platforms, `rename` replaces an existing regular-file destination atomically. If another process creates `output_path` after the initial check but before line 744, the writer can replace caller-owned data even when `ba2_dx10_writer_options::overwrite_existing` is false. The project explicitly preserves a future Linux/macOS path, so this is a real cross-platform data-loss risk.

**Fix:** For the non-overwrite path, publish using a no-replace operation instead of `rename` to an unchecked destination. A portable option is to sacrifice atomicity for safety by copying with `copy_options::none`, then deleting the temp file only after the copy succeeds; alternatively add platform-specific exclusive create/link handling behind a helper.

```cpp
if (!options.overwrite_existing) {
  std::filesystem::copy_file(temp_path, output_path, std::filesystem::copy_options::none, fs_error);
  if (fs_error) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path without overwrite"};
  }
  cleanup_publish_directory(temp_dir.value());
  return {};
}
```

## Warnings

### WR-01: Writer rejects `DXGI_FORMAT_R8G8B8A8_UNORM` despite layout support

**Classification:** WARNING
**File:** `src/texture/directxtex_analyzer.cpp:25-43`

**Issue:** `dds_layout.cpp` includes format `28U` (`DXGI_FORMAT_R8G8B8A8_UNORM`) in the locked format descriptors, so chunk planning/header reconstruction supports it. `is_supported_writer_source_format`, however, omits `28U`, causing `ba2_dx10_writer::add_file` to reject valid R8G8B8A8_UNORM DDS sources. This inconsistency creates an avoidable correctness gap for a common uncompressed DDS format.

**Fix:** Add format `28U` to `is_supported_writer_source_format`, add a generated source DDS/manifest case for it, and include it in the writer proof matrix.

```cpp
case 28U: // R8G8B8A8_UNORM
  return true;
```

### WR-02: Rollback/publish test asserts source substrings instead of behavior

**Classification:** WARNING
**File:** `tests/unit/ba2_dx10_writer_tests.cpp:644-652`

**Issue:** The test named `BA2 DX10 writer safe publish implementation keeps backup rollback hooks` passes if the source file merely contains the strings `reserve_backup_path`, `backup`, and `rollback`. It would still pass if rollback logic were removed but those words remained in comments or dead code, so it does not reliably protect the data-preservation behavior it claims to cover.

**Fix:** Replace the source-text assertion with a behavioral test that forces publish failure after the backup is created, then verifies the original archive bytes are restored and no backup remains. If fault injection is not available yet, expose a small internal test hook around the publish helper or refactor publish into an injectable helper that can simulate a failing second rename.

---

_Reviewed: 2026-05-09T00:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
