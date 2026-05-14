---
phase: 14-verification-lane-truthfulness
plan: 02
subsystem: testing
tags: [ci, github-actions, cmake, ctest, msvc, asan]
requires:
  - phase: 14-verification-lane-truthfulness
    provides: Checked-in release and ASan preset families plus the shared verification-lane contract from plan 01
provides:
  - Truthful Windows CI debug/release matrix that runs checked-in presets directly
  - Separate parallel MSVC AddressSanitizer hardening job bound to windows-msvc-asan-static
  - Workflow-topology policy tests for matrix coverage and ASan job separation
affects: [14-03, ci, docs]
tech-stack:
  added: []
  patterns: [role-aware matrix.include rows, repo-reading workflow topology assertions]
key-files:
  created: [.planning/phases/14-verification-lane-truthfulness/14-02-SUMMARY.md]
  modified: [.github/workflows/ci.yml, tests/unit/validation_policy_tests.cpp]
key-decisions:
  - "The main Windows CI job uses matrix.include rows so workflow output can expose both lane role and concrete preset without changing the checked-in preset contract."
  - "The MSVC AddressSanitizer lane stays a separate top-level job that runs the exact windows-msvc-asan-static preset triad instead of becoming a fifth matrix row."
patterns-established:
  - "Pattern: keep CI truthfulness checks in repo-reading Catch2 policy tests so workflow topology drift fails locally before GitHub execution."
  - "Pattern: express human-facing CI lane names from matrix role plus preset while keeping configure/build/test commands bound to preset names only."
requirements-completed: [VER-01, VER-02]
duration: 2 min
completed: 2026-05-14
---

# Phase 14 Plan 02: Verification Lane Truthfulness Summary

**Role-aware Windows CI matrix plus a separate MSVC AddressSanitizer hardening job that both run the checked-in preset contract**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-14T05:21:44Z
- **Completed:** 2026-05-14T05:23:18Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Expanded the main Windows CI matrix to run debug static/shared and release static/shared via the same checked-in preset triads maintainers use locally.
- Added a separate parallel `windows-msvc-asan-static` hardening job with the existing checkout, CMake 4.3.2, build, test, and TES5Edit read-only guard sequence.
- Tightened `validation_policy_tests.cpp` so workflow topology drift around matrix coverage, role-aware names, and ASan job separation fails in local policy runs.

## Task Commits

Each task was committed atomically:

1. **Task 1: Expand the main Windows matrix and lock its topology in policy tests** - `9a72851` (feat)
2. **Task 2: Add the separate parallel MSVC AddressSanitizer hardening job** - `8ee2bb9` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified
- `.github/workflows/ci.yml` - expands the main Windows matrix to debug/release lanes with role-aware names and adds the dedicated ASan hardening job.
- `tests/unit/validation_policy_tests.cpp` - adds workflow-topology assertions that lock the main matrix contract and the separate ASan job shape.

## Decisions Made
- Used `matrix.include` rows instead of a plain preset list so GitHub job names can show both lane role and preset while still invoking `cmake --preset`, `cmake --build --preset`, and `ctest --preset` through one matrix variable.
- Kept the ASan lane as a standalone job with direct `windows-msvc-asan-static` commands so workflow output cannot imply hardening coverage through a generic matrix row.
- Marked only `VER-01` and `VER-02` complete here because plan 03 still owns the remaining documentation/planning truthfulness work needed to finish `VER-03`.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- The first workflow-topology assertion approach was too brittle for the main matrix block, so the final policy gate locks the matrix through exact role/preset tokens and count-based topology facts while keeping block extraction only where job separation matters.
- The GSD state helpers advanced plan counters and appended decisions correctly but left a few human-facing STATE.md summary fields stale, so the final execution metadata patch corrected the note, milestone-plan count, latest-execution line, and frontmatter percent truthfully before the docs commit.

## Known Stubs

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03 can now align README, fixture guidance, and planning summaries against the CI and preset contract that plans 01 and 02 made real.
- `VER-03` remains intentionally open until plan 03 finishes the cross-surface docs and planning truthfulness gate.

## Self-Check: PASSED

- Found summary file: `.planning/phases/14-verification-lane-truthfulness/14-02-SUMMARY.md`
- Found commit: `9a72851`
- Found commit: `8ee2bb9`

---
*Phase: 14-verification-lane-truthfulness*
*Completed: 2026-05-14*
