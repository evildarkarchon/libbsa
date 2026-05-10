---
phase: 12-performance-concurrency-documentation-and-polish
reviewed: 2026-05-10T09:05:06Z
depth: standard
files_reviewed: 34
files_reviewed_list:
  - CMakeLists.txt
  - benchmarks/README.md
  - benchmarks/libbsa_benchmarks.cpp
  - docs/Doxyfile.in
  - docs/api-mainpage.md
  - docs/integration-examples.md
  - docs/target-format-guide.md
  - docs/thread-safety.md
  - include/libbsa/archive.hpp
  - include/libbsa/validation.hpp
  - include/libbsa/writer.hpp
  - src/archive.cpp
  - src/detail/parallel_work.cpp
  - src/detail/parallel_work.hpp
  - src/formats/ba2/ba2_dx10_writer.cpp
  - src/formats/ba2/ba2_dx10_writer.hpp
  - src/formats/ba2/ba2_gnrl_writer.cpp
  - src/formats/ba2/ba2_gnrl_writer.hpp
  - src/formats/bsa/tes3_bsa_writer.cpp
  - src/formats/bsa/tes4_bsa_writer.cpp
  - src/formats/bsa/tes4_bsa_writer.hpp
  - tests/CMakeLists.txt
  - tests/fixtures/README.md
  - tests/package-consumer/main.cpp
  - tests/unit/ba2_writer_execution_tests.cpp
  - tests/unit/benchmark_policy_tests.cpp
  - tests/unit/bounded_memory_policy_tests.cpp
  - tests/unit/bsa_writer_execution_tests.cpp
  - tests/unit/bulk_extraction_tests.cpp
  - tests/unit/docs_policy_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
  - tests/unit/target_format_policy_tests.cpp
  - tests/unit/thread_safety_docs_policy_tests.cpp
  - tests/unit/writer_execution_options_tests.cpp
findings:
  critical: 4
  warning: 1
  info: 0
  total: 5
status: issues_found
---

# Phase 12: Code Review Report

**Reviewed:** 2026-05-10T09:05:06Z
**Depth:** standard
**Files Reviewed:** 34
**Status:** issues_found

## Summary

Standard-depth review covered the listed source, public header, documentation, benchmark, and test changes. The highest-risk defects are in worker-count handling and BA2 GNRL finalization: public `worker_count` values can drive unbounded thread creation, raw disk-backed BA2 GNRL entries can corrupt the archive if the source changes during finalization, and BA2 GNRL publishing does not preserve the same no-replace/rollback guarantees as the other writer families.

## Critical Issues

### CR-01: Public Worker Count Can Exhaust Threads or Throw Outside Result Contract

**Classification:** BLOCKER
**File:** `src/detail/parallel_work.cpp:57-60`
**Issue:** `run_indexed_work` accepts a public `std::uint32_t worker_count`, then reserves and starts exactly that many `std::jthread` objects after only checking for zero. A caller can pass a huge positive worker count through `write_execution_options` or `bulk_extract_options`, causing massive allocation/thread creation or `std::system_error`/`std::bad_alloc` to escape the library's `result` contract.
**Fix:**
```cpp
constexpr std::uint32_t max_worker_count = 1024U;
if (worker_count > max_worker_count) {
  return error{error_code::invalid_argument, "worker_count exceeds the supported maximum"};
}

const auto actual_worker_count =
    static_cast<std::uint32_t>(std::min<std::size_t>(task_count, worker_count));

try {
  std::vector<std::jthread> workers;
  workers.reserve(actual_worker_count);
  for (std::uint32_t worker = 0; worker < actual_worker_count; ++worker) {
    workers.emplace_back(run_worker);
  }
} catch (const std::system_error&) {
  return error{error_code::io_error, "failed to start worker thread"};
} catch (const std::bad_alloc&) {
  return error{error_code::io_error, "failed to allocate worker state"};
}
```

### CR-02: BA2 GNRL Raw Disk Sources Can Corrupt Output If Mutated During Finalization

**Classification:** BLOCKER
**File:** `src/formats/ba2/ba2_gnrl_writer.cpp:399-419`
**File:** `src/formats/ba2/ba2_gnrl_writer.cpp:718-720`
**Issue:** Raw disk-backed BA2 GNRL entries are sized during `prepare_entry`, offsets and `FileTableOffset` are computed from that size, but `stream_disk_payload` later streams until EOF with no expected-size bound. If the source file grows or shrinks between preparation and streaming, the writer emits bytes that no longer match the record sizes and name-table offset, producing a malformed archive. The TES3/TES4 writers already guard this mutation case; BA2 GNRL does not.
**Fix:**
```cpp
result<void> stream_disk_payload(const std::string& host_path,
                                 std::uint32_t expected_size,
                                 std::ostream& output) {
  std::ifstream input{host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }

  std::array<char, 64U * 1024U> scratch{};
  std::uint64_t remaining = expected_size;
  while (remaining > 0U) {
    const auto requested = std::min<std::size_t>(scratch.size(), remaining);
    input.read(scratch.data(), static_cast<std::streamsize>(requested));
    if (input.gcount() != static_cast<std::streamsize>(requested)) {
      return error{error_code::io_error, "BA2 GNRL disk source changed during finalization"};
    }
    output.write(scratch.data(), static_cast<std::streamsize>(requested));
    if (!output) {
      return error{error_code::io_error, "BA2 GNRL writer failed while streaming disk source"};
    }
    remaining -= requested;
  }

  char extra = '\0';
  if (input.get(extra)) {
    return error{error_code::io_error, "BA2 GNRL disk source changed during finalization"};
  }
  return {};
}
```
Pass `entry.raw_size` when streaming, and add the same EOF check to the disk dedupe comparison helpers so changed-but-deduped sources cannot bypass the guard.

### CR-03: BA2 GNRL No-Overwrite Mode Can Replace a Destination Created During the Race Window

**Classification:** BLOCKER
**File:** `src/formats/ba2/ba2_gnrl_writer.cpp:879-885`
**File:** `src/formats/ba2/ba2_gnrl_writer.cpp:958-962`
**Issue:** When `overwrite_existing == false`, BA2 GNRL checks the destination before preparing and writing a temporary archive, but it publishes with `std::filesystem::rename(temp_path, output_path)`. On POSIX-style filesystems, `rename` replaces an existing destination, so a file created after the initial check can be overwritten despite the caller explicitly leaving overwrite disabled. TES3/TES4 use no-replace publishing, and BA2 DX10 uses `copy_file(..., copy_options::none)` for this case.
**Fix:**
```cpp
if (!options.overwrite_existing) {
  output_exists = path_exists_noexcept(output_path);
  if (!output_exists) {
    cleanup_publish_directory(temp_dir.value());
    return output_exists.error();
  }
  if (output_exists.value()) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "BA2 GNRL output host path already exists"};
  }

  auto published = detail::publish_file_without_replace(temp_path, output_path);
  if (!published) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "BA2 GNRL writer failed to publish output host path without overwrite"};
  }
  cleanup_publish_directory(temp_dir.value());
  return {};
}
```

### CR-04: BA2 GNRL Overwrite Rollback Can Leave the Previous Archive Moved Aside

**Classification:** BLOCKER
**File:** `src/formats/ba2/ba2_gnrl_writer.cpp:944-949`
**Issue:** The overwrite path renames the existing archive to a backup, then renames the temporary archive into place. If the second rename fails, the code attempts to restore the backup but ignores `rollback_error` and always returns the same generic publish error. A failed rollback can leave the original archive absent from the requested path and stranded under a backup name, which is a data-loss/availability failure. BA2 DX10 has a helper that reports this distinction; BA2 GNRL does not use it.
**Fix:**
```cpp
std::filesystem::rename(temp_path, output_path, fs_error);
if (fs_error) {
  cleanup_publish_directory(temp_dir.value());
  return publish_detail::restore_backup_after_publish_failure(
      backup_path.value(),
      output_path,
      [](const std::filesystem::path& from, const std::filesystem::path& to, std::error_code& error) {
        std::filesystem::rename(from, to, error);
      });
}
```
Alternatively, replace the manual backup flow with the shared atomic replace helper used by the BSA writers.

## Warnings

### WR-01: `BUILD_TESTING=OFF` Does Not Disable Test Dependencies

**Classification:** WARNING
**File:** `CMakeLists.txt:12-13`
**File:** `CMakeLists.txt:157-163`
**File:** `tests/CMakeLists.txt:1-3`
**Issue:** The top-level build uses a separate `LIBBSA_BUILD_TESTS` option defaulting to `ON`, so a standard consumer configure with `-DBUILD_TESTING=OFF` still descends into `tests/` unless they also know to set the project-specific option. That forces Catch2, nlohmann_json, and Python discovery in what should be the normal CTest-disabled consumer path.
**Fix:**
```cmake
include(CTest)
option(LIBBSA_BUILD_TESTS "Build libbsa tests" ${BUILD_TESTING})

if(BUILD_TESTING AND LIBBSA_BUILD_TESTS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  endif()
endif()
```

---

_Reviewed: 2026-05-10T09:05:06Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
