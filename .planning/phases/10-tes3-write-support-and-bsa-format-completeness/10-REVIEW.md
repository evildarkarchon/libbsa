---
phase: 10-tes3-write-support-and-bsa-format-completeness
reviewed: 2026-05-10T00:33:13Z
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
  warning: 2
  info: 0
  total: 4
status: issues_found
---

# Phase 10: Code Review Report

**Reviewed:** 2026-05-10T00:33:13Z
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Reviewed the TES3 public writer implementation, build integration, committed manifest provenance, fixture generator, and public/unit tests. The raw `.bsa` fixture was intentionally not read as source; its provenance and validation coverage were reviewed through the generator, manifest, and tests. Two correctness/data-loss blockers were found in the writer path handling and publish logic, plus two test/fixture-generator robustness issues.

## Critical Issues

### CR-01: BLOCKER - `overwrite_existing=false` can still replace a concurrently-created output

**File:** `src/formats/bsa/tes3_bsa_writer.cpp:389-396,467-470`

**Issue:** The writer checks whether `output_path` exists before validating/loading sources and writing the temporary archive, but the non-overwrite publish path does not re-check before `std::filesystem::rename(temp_path, output_path)`. On platforms where rename replaces an existing regular file, a file created after the initial check can be overwritten even though `overwrite_existing` is false. That violates the public contract and creates a data-loss race.

**Fix:** Re-check the destination immediately before the non-overwrite rename and refuse to publish if it now exists; preferably use a platform-specific no-replace publish primitive when available.

```cpp
auto final_exists = path_exists_noexcept(output_path);
if (!final_exists) {
  cleanup_publish_directory(temp_dir.value());
  return final_exists.error();
}
if (final_exists.value()) {
  cleanup_publish_directory(temp_dir.value());
  return error{error_code::io_error, "TES3 BSA output host path already exists"};
}

std::filesystem::rename(temp_path, output_path, fs_error);
```

### CR-02: BLOCKER - Archive paths containing NUL produce unreadable/self-inconsistent archives

**File:** `src/formats/bsa/tes3_bsa_writer.cpp:35-44,214-221`

**Issue:** `make_entry` accepts any path that `normalize_archive_path` accepts, and the current normalization path does not reject embedded `\0` bytes. `write_string_terminated` then serializes the name as a null-terminated string, so `"Meshes/A.nif\0Suffix"` is stored as a truncated visible name while the writer computes the TES3 hash from the full original string. The reader recomputes hashes from the parsed null-terminated name and will reject the writer's own archive as a stored-hash mismatch. This is caller-triggered incorrect output and can also hide duplicate/truncated names in the serialized name table.

**Fix:** Reject embedded NULs before preserving/hashing/serializing the archive path. Ideally enforce this centrally in `normalize_archive_path`; at minimum guard the TES3 writer input path.

```cpp
result<formats::bsa::tes3_writer_entry> make_entry(std::string_view archive_path) {
  if (archive_path.find('\0') != std::string_view::npos) {
    return error{error_code::invalid_argument, "TES3 BSA archive path must not contain NUL bytes"};
  }
  auto canonical = detail::normalize_archive_path(archive_path);
  // ...
}
```

## Warnings

### WR-01: WARNING - Fixture generator does not verify writes completed successfully

**File:** `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp:127-143`

**Issue:** `write_file` and `write_text` only check that the output stream opened. They do not check the stream after writing. A disk-full, permission, or flush error can leave a truncated source or manifest while the generator exits successfully, reducing fixture provenance reliability.

**Fix:** Check stream state after each write and throw on failure.

```cpp
out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
if (!out) {
  throw std::runtime_error("failed to write " + path.string());
}

out << text;
if (!out) {
  throw std::runtime_error("failed to write " + path.string());
}
```

### WR-02: WARNING - Fixture generator uses `std::tolower` without including `<cctype>`

**File:** `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp:54-58`

**Issue:** The generator calls `std::tolower` but does not include the standard header that declares it. This can compile only by relying on transitive includes, which is non-portable and may break on another standard library/toolchain.

**Fix:** Add the required header explicitly.

```cpp
#include <cctype>
```

---

_Reviewed: 2026-05-10T00:33:13Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
