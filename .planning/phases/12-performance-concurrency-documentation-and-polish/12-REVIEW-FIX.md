---
phase: 12-performance-concurrency-documentation-and-polish
fixed_at: 2026-05-10T09:18:43Z
review_path: .planning/phases/12-performance-concurrency-documentation-and-polish/12-REVIEW.md
iteration: 1
findings_in_scope: 5
fixed: 5
skipped: 0
status: all_fixed
---

# Phase 12: Code Review Fix Report

**Fixed at:** 2026-05-10T09:18:43Z
**Source review:** .planning/phases/12-performance-concurrency-documentation-and-polish/12-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 5
- Fixed: 5
- Skipped: 0

## Fixed Issues

### CR-01: Public Worker Count Can Exhaust Threads or Throw Outside Result Contract

**Files modified:** `src/detail/parallel_work.cpp`, `tests/unit/writer_execution_options_tests.cpp`, `tests/unit/bulk_extraction_tests.cpp`
**Commit:** 6121555
**Applied fix:** Added a supported worker-count maximum, capped actual worker threads to task count, mapped worker startup allocation/thread failures to `result` errors, and covered writer/bulk extraction rejection paths.

### CR-02: BA2 GNRL Raw Disk Sources Can Corrupt Output If Mutated During Finalization

**Files modified:** `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.hpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`
**Commit:** c274358
**Applied fix:** Bounded BA2 GNRL disk streaming and dedupe disk comparisons to the prepared size, rejecting growth/truncation before malformed offsets can be published.

### CR-03: BA2 GNRL No-Overwrite Mode Can Replace a Destination Created During the Race Window

**Files modified:** `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_publish.hpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`
**Commit:** bb61d83
**Applied fix:** Rechecked destination existence after writing the temporary archive and used the repo no-replace publish helper for BA2 GNRL no-overwrite mode.

### CR-04: BA2 GNRL Overwrite Rollback Can Leave the Previous Archive Moved Aside

**Files modified:** `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_publish.hpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`
**Commit:** bb61d83
**Applied fix:** Routed failed BA2 GNRL overwrite publish through the shared backup-restore helper so rollback failure is reported distinctly.

### WR-01: `BUILD_TESTING=OFF` Does Not Disable Test Dependencies

**Files modified:** `CMakeLists.txt`, `tests/unit/benchmark_policy_tests.cpp`
**Commit:** 566b8e2
**Applied fix:** Made `LIBBSA_BUILD_TESTS` default to `${BUILD_TESTING}` and gated `add_subdirectory(tests)` on both `BUILD_TESTING` and `LIBBSA_BUILD_TESTS`.

## Skipped Issues

None - all in-scope findings were fixed.

## Verification

- `cmake --preset windows-msvc-debug-static` - passed
- `cmake -S . -B build/build-testing-off -DBUILD_TESTING=OFF -DLIBBSA_BUILD_BENCHMARKS=OFF -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"` - passed
- `cmake --build --preset windows-msvc-debug-static` - passed
- `ctest --test-dir build/windows-msvc-debug-static -C Debug --output-on-failure -R "worker_count|bulk_extraction rejects unsupported large worker counts|BA2 GNRL disk payload streaming rejects source size changes|BA2 GNRL dedupe disk comparisons reject source size changes|BA2 GNRL no-overwrite publish preserves a raced destination|BA2 GNRL overwrite rollback reports failed backup restoration|BA2 GNRL writer routes publish races|build_policy BUILD_TESTING"` - passed, 17/17
- `ctest --test-dir build/windows-msvc-debug-static -C Debug --output-on-failure` - passed, 245/245 with 2 opt-in fixture tests skipped
- `git status --short TES5Edit` - clean

---

_Fixed: 2026-05-10T09:18:43Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 1_
