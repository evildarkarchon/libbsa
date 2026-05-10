---
phase: 10-tes3-write-support-and-bsa-format-completeness
fixed_at: 2026-05-10T02:18:26Z
review_path: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 10: Code Review Fix Report

**Fixed at:** 2026-05-10T02:18:26Z
**Source review:** `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-REVIEW.md`
**Iteration:** 1

**Summary:**
- Findings in scope: 2
- Fixed: 2
- Skipped: 0

## Fixed Issues

### CR-01: Host paths accept embedded NUL bytes and can target a different filesystem path

**Files modified:** `src/formats/bsa/tes3_bsa_writer.cpp`, `tests/unit/tes3_bsa_writer_tests.cpp`
**Commit:** 37b718d
**Applied fix:** Added shared host-path validation before source paths are stored and before output paths are converted to `std::filesystem::path`; added focused source/output embedded-NUL regression tests.

### WR-01: TES3 writer tests reuse deterministic temp filenames without cleanup

**Files modified:** `tests/unit/tes3_bsa_writer_tests.cpp`
**Commit:** be25883
**Applied fix:** Made `output_path` allocate a monotonically unique subdirectory for each requested test output path, preventing stale deterministic files from changing expected validation errors.

## Verification

- `cmake --preset windows-msvc-debug-static` — passed.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — passed for CR-01 and again after WR-01.
- `ctest --preset windows-msvc-debug-static -R tes3_bsa_writer --output-on-failure` — passed for CR-01: 18/18 tests.
- `ctest --preset windows-msvc-debug-static -R tes3_bsa_writer --output-on-failure` — passed after WR-01: 18/18 tests.

## Skipped Issues

None.

---

_Fixed: 2026-05-10T02:18:26Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 1_
