---
phase: 09-ba2-dx10-write-new-support
fixed_at: 2026-05-09T11:22:03.2151115Z
review_path: .planning/phases/09-ba2-dx10-write-new-support/09-REVIEW.md
iteration: 3
findings_in_scope: 1
fixed: 1
skipped: 0
status: all_fixed
---

# Phase 09: Code Review Fix Report

**Fixed at:** 2026-05-09T11:22:03.2151115Z
**Source review:** .planning/phases/09-ba2-dx10-write-new-support/09-REVIEW.md
**Iteration:** 3

**Summary:**
- Findings in scope: 1
- Fixed: 1
- Skipped: 0

**Focused verification:** Configured `windows-msvc-debug-static`, built `libbsa_tests`, ran the focused publish rollback test, and confirmed no `LIBBSA_ENABLE_TEST_FAULT_INJECTION` or `LIBBSA_TEST_FAIL_BA2_DX10_PUBLISH_AFTER_BACKUP` references remain.

## Fixed Issues

### CR-01: Default builds compile test fault injection into the library

**Files modified:** `CMakeLists.txt`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_publish.hpp`, `tests/unit/ba2_dx10_writer_tests.cpp`
**Commit:** 0e4b22d
**Applied fix:** Removed the test fault-injection compile definition from the production `libbsa` target, deleted the environment-variable failure hook from the writer, and moved rollback coverage to an internal test-only publish helper seam used by the writer and exercised directly by the unit test.

---

_Fixed: 2026-05-09T11:22:03.2151115Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 3_
