---
phase: 16-parser-and-preparer-seam-extraction
plan: 03
subsystem: testing
tags: [parser-preparer-seam, source-policy, catch2, ctest, msvc-asan]
requires:
  - phase: 16-parser-and-preparer-seam-extraction
    provides: TES4 parser and BA2 DX10 preparer private seams from plans 16-01 and 16-02
provides:
  - dedicated parser/preparer seam source-policy guard for TES4 and BA2 DX10 role separation
  - focused affected-suite, full debug, and MSVC ASan verification evidence for Phase 16 closure
affects: [tests/unit, tests/CMakeLists.txt, phase-16-validation]
tech-stack:
  added: []
  patterns: [role-based source policy tests, phase-gate debug and ASan verification]
key-files:
  created: [tests/unit/parser_preparer_seam_policy_tests.cpp]
  modified: [tests/CMakeLists.txt]
key-decisions:
  - "Parser/preparer seam guardrails live in a dedicated source-policy Catch2 suite instead of expanding unrelated validation-policy tests."
  - "Phase 16 closure evidence uses focused affected suites, full debug CTest, and the supported MSVC ASan lane while leaving Phase 17 dedupe and DX10 temp-lifecycle requirements pending."
patterns-established:
  - "Role-based seam policies assert private seam evidence and negative collapse invariants without freezing exact helper function names."
  - "Verification-only tasks with clean working trees are recorded in summary evidence instead of creating empty commits."
requirements-completed: [REFA-01, REFA-02]
duration: 5m
completed: 2026-05-14
---

# Phase 16 Plan 03: Parser and Preparer Seam Policy Guard Summary

**Dedicated source-policy guardrails now lock TES4 parser and BA2 DX10 preparer seam separation, with focused, full-debug, and MSVC ASan validation evidence for Phase 16 closure.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-14T23:29:05Z
- **Completed:** 2026-05-14T23:34:03Z
- **Tasks:** 3
- **Files modified:** 2

## Accomplishments

- Added `parser_preparer_seam_policy_tests.cpp` with `[parser_preparer_seam_policy]` Catch2 coverage for both targeted hotspots.
- Registered the dedicated policy test in `tests/CMakeLists.txt` without expanding `validation_policy_tests.cpp`.
- Ran the focused affected suite, full debug suite, and MSVC ASan static hardening lane successfully before closing Phase 16.

## Task Commits

Each source-changing task was committed atomically:

1. **Task 1: Add parser/preparer seam policy guard** - `ea2a7b6` (test)
2. **Task 2: Run focused and full debug verification** - no source commit; verification-only task completed with a clean working tree.
3. **Task 3: Run MSVC ASan hardening lane and close phase evidence** - no source commit; verification-only task completed with a clean working tree.

_Note: Plan metadata is committed separately after state and roadmap updates._

## TDD Gate Compliance

- **RED:** Pre-guard label check found no `parser_preparer_seam_policy` tests before this plan, establishing the missing guardrail. The first compiled policy draft failed on the BA2 DX10 coordinator invariant because a comment contained the broad `run_indexed_work` token.
- **GREEN:** `ea2a7b6` added the dedicated policy suite, refined negative invariants to detect call-site collapse without failing on explanatory comments, registered the test, and made the focused policy label pass.
- **REFACTOR:** No separate refactor commit was needed.

## Validation

- **Task 1 focused policy command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` passed with 2/2 tests.
- **Affected-suite wave command:** `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"` passed with 106/106 tests in 3.22 seconds.
- **Full debug suite:** `ctest --preset windows-msvc-debug-static --output-on-failure` passed with 390/390 tests in 18.80 seconds; the two opt-in local/BSArchPro fixture tests remained skipped.
- **MSVC ASan hardening lane:** `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure` passed with 390/390 tests in 45.27 seconds; the same two opt-in fixture tests remained skipped.
- **Phase 17 scope check:** `.planning/REQUIREMENTS.md` still leaves `DEDU-01`, `DEDU-02`, `DX10-01`, and `DX10-02` unchecked, and Phase 16 summaries do not claim BA2 DX10 abnormal-termination temp-data cleanup proof.

## Files Created/Modified

- `tests/unit/parser_preparer_seam_policy_tests.cpp` - source-reading policy tests that require TES4 table/payload and BA2 DX10 snapshot/chunk seam evidence, then reject broad coordinator responsibility collapse.
- `tests/CMakeLists.txt` - registers `unit/parser_preparer_seam_policy_tests.cpp` in `libbsa_tests` so Catch2 tag discovery exposes the policy label.

## Decisions Made

- Kept parser/preparer seam policy coverage in a new dedicated test file per D-14 rather than adding more assertions to `validation_policy_tests.cpp`.
- Encoded D-15 and D-16 as role-based negative invariants: coordinators may explain or call seams, but they must not directly re-own raw table parsing, payload descriptor calculation, snapshot creation, chunk planning, compression routing, and sorting as mixed flows.
- Treated focused/full/ASan verification tasks as evidence-only tasks; no empty commits were created when they produced no source changes.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Refined BA2 DX10 policy token matching to avoid comment-only false positives**
- **Found during:** Task 1 (Add parser/preparer seam policy guard)
- **Issue:** The first policy draft rejected `run_indexed_work` anywhere in `ba2_dx10_prepare_entries`, including a comment documenting that the chunk seam owns indexed work.
- **Fix:** Changed BA2 DX10 collapse checks to look for call-shaped tokens such as `run_indexed_work(`, `plan_dx10_chunks(`, and `compress_payload(` so explanatory comments remain allowed while direct responsibility collapse is still blocked.
- **Files modified:** `tests/unit/parser_preparer_seam_policy_tests.cpp`
- **Verification:** Focused policy command passed with 2/2 tests.
- **Committed in:** `ea2a7b6`

---

**Total deviations:** 1 auto-fixed (1 bug).
**Impact on plan:** The fix improved guardrail accuracy without weakening the intended collapse detection or expanding scope.

## Issues Encountered

- TDD RED was constrained by dependency order: the TES4 and BA2 DX10 private seams already existed from Plans 16-01 and 16-02, so the missing behavior at plan start was the dedicated policy suite/label rather than a missing production seam.
- The initial policy draft failed on a comment-only BA2 token match; this was corrected before the task commit.

## Known Stubs

None.

## Threat Flags

None.

## Authentication Gates

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 16 is ready for verification: REFA-01 and REFA-02 now have focused runtime seam tests, dedicated source-policy guardrails, full debug evidence, and MSVC ASan hardening evidence.
- Phase 17 remains responsible for `DEDU-01`, `DEDU-02`, `DX10-01`, and `DX10-02`; Phase 16 did not claim dedupe optimization or BA2 DX10 abnormal-termination cleanup proof.

## Self-Check: PASSED

- FOUND: `tests/unit/parser_preparer_seam_policy_tests.cpp`
- FOUND: `tests/CMakeLists.txt`
- FOUND COMMIT: `ea2a7b6`
- PASS: `tests/unit/parser_preparer_seam_policy_tests.cpp` contains `[parser_preparer_seam_policy]`.
- PASS: `tests/CMakeLists.txt` lists `unit/parser_preparer_seam_policy_tests.cpp`.
- PASS: No files under `TES5Edit/` or `include/libbsa/` were modified.

---
*Phase: 16-parser-and-preparer-seam-extraction*
*Completed: 2026-05-14*
