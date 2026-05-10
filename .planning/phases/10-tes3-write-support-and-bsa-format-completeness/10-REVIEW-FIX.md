---
phase: 10-tes3-write-support-and-bsa-format-completeness
fixed_at: 2026-05-10T02:02:32Z
review_path: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-REVIEW.md
iteration: 2
findings_in_scope: 3
fixed: 3
skipped: 0
status: all_fixed
---

# Phase 10: Code Review Fix Report

**Fixed at:** 2026-05-10T02:02:32Z
**Source review:** .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-REVIEW.md
**Iteration:** 2

**Summary:**
- Findings in scope: 3
- Fixed: 3
- Skipped: 0

## Fixed Issues

### CR-01: Non-overwrite publish can still replace a concurrently-created destination

**Files modified:** `src/detail/atomic_file_ops.hpp`, `src/formats/bsa/tes3_bsa_writer.cpp`
**Commit:** dce8878
**Applied fix:** Added an internal no-replace publish helper and routed non-overwrite final publication through it so an existing destination fails instead of being replaced.

### CR-02: Backup path selection is not reserved atomically and can clobber caller files

**Files modified:** `src/formats/bsa/tes3_bsa_writer.cpp`
**Commit:** 30a000a
**Applied fix:** Moved overwrite backups into a writer-owned unique backup directory and used the same no-replace publish path for replacement publication.

### WR-01: Race regression test is timing-dependent and can pass without covering the vulnerable window

**Files modified:** `src/detail/atomic_file_ops.hpp`, `tests/unit/tes3_bsa_writer_tests.cpp`
**Commit:** adcba6f
**Applied fix:** Removed the watcher/large-payload race test and replaced it with deterministic coverage for no-replace publishing and writer-owned backup-directory reservation.

## Skipped Issues

None.

## Verification

- `cmake --preset windows-msvc-debug-static && cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — passed; MSBuild emitted only temporary-directory incremental-build warnings for the isolated worktree location.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests --clean-first && build/windows-msvc-debug-static/tests/Debug/libbsa_tests.exe "[tes3_bsa_writer]"` — passed, 345 assertions in 15 test cases.
- `build/windows-msvc-debug-static/tests/Debug/libbsa_tests.exe "[tes3_bsa_writer]"` — passed after commits, 345 assertions in 15 test cases.

---

_Fixed: 2026-05-10T02:02:32Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 2_
