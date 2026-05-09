---
phase: 09-ba2-dx10-write-new-support
reviewed: 2026-05-09T00:00:00Z
depth: standard
files_reviewed: 13
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/writer.hpp
  - src/formats/ba2/ba2_dx10_writer.hpp
  - src/formats/ba2/ba2_dx10_writer.cpp
  - src/texture/directxtex_analyzer.hpp
  - src/texture/directxtex_analyzer.cpp
  - src/texture/dds_layout.hpp
  - src/texture/dds_layout.cpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp
  - tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json
  - tests/unit/ba2_dx10_writer_tests.cpp
  - tests/unit/dds_layout_tests.cpp
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
**Files Reviewed:** 13
**Status:** issues_found

## Summary

Reviewed the Phase 09 BA2 DX10 writer, DDS analysis/layout helpers, generated fixture source/manifest, and unit-test coverage. The implementation has one malformed-archive validation bug in shared DDS layout code and two test/fixture reliability defects that can let compatibility regressions slip through.

## Critical Issues

### CR-01: Block-compressed mip-size arithmetic can wrap for hostile dimensions

**Classification:** BLOCKER
**File:** `src/texture/dds_layout.cpp:93-96`
**Issue:** `described_mip_size` computes block counts for BC formats with `(width + 3U) / 4U` and `(height + 3U) / 4U` while `width` and `height` are `std::uint32_t`. Archive metadata is untrusted; a BA2 record can advertise dimensions near `UINT32_MAX`, causing `width + 3U` or `height + 3U` to wrap before promotion to `std::uint64_t`. That underestimates expected mip sizes, so `validate_and_order_chunks` can accept impossible chunk metadata instead of failing closed.
**Fix:** Promote before adding and use the descriptor block dimensions rather than hard-coded `4U`.
```cpp
const auto block_width = static_cast<std::uint64_t>(descriptor.block_width);
const auto block_height = static_cast<std::uint64_t>(descriptor.block_height);
const auto mip_width = static_cast<std::uint64_t>(width);
const auto mip_height = static_cast<std::uint64_t>(height);
const std::uint64_t blocks_wide =
    descriptor.block_width == 1U ? mip_width : (mip_width + block_width - 1U) / block_width;
const std::uint64_t blocks_high =
    descriptor.block_height == 1U ? mip_height : (mip_height + block_height - 1U) / block_height;
```
Add a regression test with BC dimensions such as `UINT32_MAX` that currently produce a tiny accepted raw size.

## Warnings

### WR-01: Generated DX10 fixture name hashes do not match writer/parser convention

**Classification:** WARNING
**File:** `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp:400-404`
**Issue:** `prepare_texture` hashes `texture.path` (the full canonical archive path) into the BA2 record `name_hash`, while the implemented BA2 writers hash only the file-name portion and hash the directory separately. The parser exposes the stored hash without validating it, so these generated fixtures can silently encode and document the wrong hash value while tests still pass.
**Fix:** Split the canonical path and hash only the file name, mirroring `ba2_dx10_writer.cpp` and `ba2_gnrl_writer.cpp`.
```cpp
std::uint32_t hash_file_name(std::string_view canonical_path) {
  const auto slash = canonical_path.find_last_of('/');
  return libbsa::detail::hash_fo4(slash == std::string_view::npos ? canonical_path
                                                                  : canonical_path.substr(slash + 1U));
}

texture.name_hash = hash_file_name(texture.path);
texture.directory_hash = hash_folder(texture.path);
```
Regenerate the DX10 fixtures and manifests after fixing the generator.

### WR-02: Publish safety test asserts source-code strings instead of rollback behavior

**Classification:** WARNING
**File:** `tests/unit/ba2_dx10_writer_tests.cpp:644-652`
**Issue:** The rollback test passes if the source file contains the words `reserve_backup_path`, `backup`, and `rollback`; it does not exercise the overwrite failure path or prove the old archive remains restorable. Renaming helpers or comments can fail the test, while broken rollback logic can still pass as long as those strings remain.
**Fix:** Replace the source-text inspection with behavioral coverage. For example, inject or arrange a publish failure after the backup rename, then assert that the original output bytes are restored and no partially written archive is left at the destination. If direct fault injection is not available, factor the publish operation behind a small internal seam and test the rollback branch through that seam.

---

_Reviewed: 2026-05-09T00:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
