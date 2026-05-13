---
phase: 13-host-path-correctness-boundary
plan: 04
subsystem: testing
tags: [windows, unicode, validation, extraction, regression-tests]
requires:
  - phase: 13-03
    provides: stored-path extraction contracts and reader reopen behavior
provides:
  - Validation setup unified behind `archive_reader::open`.
  - Dedicated non-ASCII host-path regression coverage for representative TES4 and BA2 archives.
  - Public-surface proof of open, validate-with-extractability, and canonical extraction on the locked archive matrix.
affects: [phase-14, milestone-v1.1-hardening]
tech-stack:
  added: []
  patterns:
    - Validation setup reuses the same strict open boundary as ordinary archive opens.
    - Representative host-path coverage stays black-box and manifest-backed.
key-files:
  created:
    - tests/unit/host_path_correctness_boundary_tests.cpp
  modified:
    - src/validation.cpp
    - tests/CMakeLists.txt
key-decisions:
  - "Make `archive_reader::open` the single validation setup boundary instead of keeping a duplicate readability preflight."
  - "Prove non-ASCII host-path behavior with one dedicated public-surface regression suite over the locked representative matrix."
patterns-established:
  - "Validation setup errors return direct result failures while readable malformed archives still become validation diagnostics."
  - "Only the archive under test is copied into the non-ASCII temp location; manifests and expected bytes stay in committed fixtures."
requirements-completed: [HOST-01, HOST-02, HOST-03]
duration: unavailable
completed: 2026-05-13
---

# Phase 13 Plan 04: Host Path Correctness Boundary Summary

**Validation setup now reuses `archive_reader::open`, and a dedicated non-ASCII regression suite exercises representative TES4 and BA2 open/validate/extract flows through the public API surface.**

## Performance

- **Duration:** Unavailable — current evidence does not prove a per-plan execution duration.
- **Started:** Unavailable — no attributable executor timestamp or task commit survives in reachable history.
- **Completed:** 2026-05-13 at phase level only — the date is supported, but the plan-specific completion timestamp is not.
- **Tasks:** 3 planned tasks.
- **Files modified:** 3 files listed in the plan frontmatter.

## Accomplishments

- Removed validation's duplicate readability preflight so setup flows through the same strict open boundary as ordinary archive usage.
- Added a dedicated non-ASCII host-path regression suite for TES4 v103/v104/v105, Fallout 4 BA2 GNRL, Fallout 4 BA2 DX10, and Starfield BA2 GNRL coverage.
- Verified open, validation with extractability enabled, and canonical extraction behavior from the public API surface in the verification snapshot.

## Task Commits

Reachable git history does not prove atomic task commits for this plan:

1. **Task 1: Remove validation's duplicate readability preflight while preserving result/report semantics** - **Unavailable** (reachable history contains no attributable `13-04` task commit).
2. **Task 2: Add and register the dedicated non-ASCII host-path suite with smoke coverage** - **Unavailable** (reachable history contains no attributable `13-04` task commit).
3. **Task 3: Cover the locked representative archive matrix for open, validate, and canonical extraction** - **Unavailable** (reachable history contains no attributable `13-04` task commit).

**Plan metadata:** **Unavailable** for this summary. Related phase-level docs commit `d15b9ad` does not prove a committed `13-04-SUMMARY.md`.

## Files Created/Modified

- `src/validation.cpp` - Removes the duplicate preflight and keeps setup/result/report semantics on the shared open boundary.
- `tests/unit/host_path_correctness_boundary_tests.cpp` - Adds representative non-ASCII open/validate/extract regression coverage.
- `tests/CMakeLists.txt` - Registers the dedicated host-path correctness suite and smoke subset.

## Decisions Made

- Validation setup must trust `archive_reader::open` instead of maintaining a second host-path readability gate.
- Host-path correctness proof stays black-box and public-surface only.

## Deviations from Plan

Execution-history evidence is incomplete rather than cleanly preserved.

- Reachable git history does not prove the per-task commits required by the executor contract.
- The summary itself existed only as an untracked placeholder draft before this repair.
- Current repository state still contains uncommitted Phase 13 implementation changes, so this plan cannot be presented as a fully committed close-out.
- `13-REVIEW.md` still records CR-01 and WR-01, so the phase did not end in a clean review state.

**Total deviations:** Unavailable from executor history; current repair evidence proves documentation drift and unresolved review findings.
**Impact on plan:** The behavior and test-scope story are well supported by plan and verification artifacts, but commit provenance, timing data, and clean close-out claims are not.

## Issues Encountered

- CR-01 remains a blocker: host-file identity does not detect same-size in-place rewrites.
- WR-01 remains a warning: regression coverage still misses the same-size mutation case called out by the review.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The verification snapshot supports the technical Phase 13 handoff: open, validation, parser entry, and extraction all share the repaired host-file boundary.
- The planning handoff is not clean yet: current repository state still shows uncommitted Phase 13 implementation changes, the repaired summaries were missing until this quick task, and CR-01 / WR-01 remain mandatory downstream context.

## Self-Check: FAILED

- Current evidence does not prove `13-04` task commit hashes.
- Current evidence does not prove per-plan start/end timestamps or duration.
- The repository still contains uncommitted Phase 13 implementation changes and an open review blocker, so a clean close-out claim would be overstated.
