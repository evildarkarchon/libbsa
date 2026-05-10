---
phase: 10-tes3-write-support-and-bsa-format-completeness
fixed_at: 2026-05-10T01:52:13Z
review_path: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-REVIEW.md
iteration: 1
findings_in_scope: 5
fixed: 5
skipped: 0
status: all_fixed
---

# Phase 10: Code Review Fix Report

**Fixed at:** 2026-05-10T01:52:13Z
**Source review:** .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 5
- Fixed: 5
- Skipped: 0

## Fixed Issues

### CR-01: BLOCKER - `overwrite_existing=false` can still replace a concurrently-created output

**Files modified:** `src/formats/bsa/tes3_bsa_writer.cpp`, `tests/unit/tes3_bsa_writer_tests.cpp`
**Commit:** a2f034e, 6d7517b
**Applied fix:** Rechecked the destination immediately before the non-overwrite publish rename and added a regression test that creates the destination while the writer is staging its temporary archive.

### CR-02: BLOCKER - Archive paths containing NUL produce unreadable/self-inconsistent archives

**Files modified:** `src/formats/bsa/tes3_bsa_writer.cpp`, `tests/unit/tes3_bsa_writer_tests.cpp`
**Commit:** 36a1b08
**Applied fix:** Rejected embedded NUL bytes at TES3 writer entry creation for both memory and disk entries, with unit coverage for the invalid path case.

### WR-01: WARNING - Fixture generator does not verify writes completed successfully

**Files modified:** `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp`
**Commit:** 7f97f18
**Applied fix:** Checked output stream state after binary and text fixture writes and throw a provenance-generation error on write failure.

### WR-02: WARNING - Fixture generator uses `std::tolower` without including `<cctype>`

**Files modified:** `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp`
**Commit:** 4da851a
**Applied fix:** Added the explicit `<cctype>` include required for `std::tolower`.

### WR-03: WARNING - Tests and fixture targets are anchored to the top-level source directory

**Files modified:** `tests/CMakeLists.txt`
**Commit:** c7dc42f
**Applied fix:** Replaced libbsa-owned `${CMAKE_SOURCE_DIR}` test include, definition, fixture output, and byproduct paths with `${PROJECT_SOURCE_DIR}`.

## Skipped Issues

None.

## Verification

- `cmake --preset windows-msvc-debug-static && cmake --build --preset windows-msvc-debug-static --target libbsa_tests generate_tes3_bsa_writer_fixtures_tool` — passed; MSBuild emitted only temporary-directory incremental-build warnings for the isolated worktree location.
- `build/windows-msvc-debug-static/tests/Debug/libbsa_tests.exe "tes3_bsa_writer refuses a destination created during non-overwrite publish"` — passed after stabilizing the race regression test.
- `build/windows-msvc-debug-static/tests/Debug/libbsa_tests.exe "[tes3_bsa_writer]"` — passed, 323 assertions in 13 test cases.
- `build/windows-msvc-debug-static/tests/Debug/generate_tes3_bsa_writer_fixtures_tool.exe --output build/windows-msvc-debug-static/tes3-writer-fixture-check` — passed after creating the output directory.

---

_Fixed: 2026-05-10T01:52:13Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 1_
