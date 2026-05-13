---
phase: 13-host-path-correctness-boundary
plan: 01
subsystem: filesystem
tags: [windows, unicode, host-file, writer, testing]
requires: []
provides:
  - Shared `host_file` helper boundary and `detail::host_file_path` naming family.
  - Writer-side prepare/layout callers migrated onto the shared host-file path contract.
  - Renamed helper regression coverage under `tests/unit/host_file_tests.cpp`.
affects: [phase-13-02, phase-13-03, phase-13-04, phase-14]
tech-stack:
  added: []
  patterns:
    - Single shared host-file helper family replaces `writer_disk_source`.
    - `detail::host_file_path` separates diagnostics UTF-8 text from resolved Windows I/O state.
key-files:
  created:
    - src/detail/host_file.hpp
    - src/detail/host_file.cpp
    - src/detail/host_path.hpp
    - src/detail/host_path.cpp
    - tests/unit/host_file_tests.cpp
  modified:
    - src/formats/bsa/tes4_bsa_prepare.cpp
    - src/formats/bsa/tes4_bsa_layout.cpp
    - src/formats/ba2/ba2_gnrl_prepare.cpp
    - src/formats/ba2/ba2_dx10_prepare.cpp
    - tests/CMakeLists.txt
key-decisions:
  - "Use one neutral host-file helper family instead of keeping a parallel writer-only name."
  - "Carry both original UTF-8 diagnostics text and the resolved filesystem path in `detail::host_file_path`."
patterns-established:
  - "Shared host-file calls, not writer-specific shims, own Windows file open/inspect/read behavior."
  - "Writer callers adopt the shared host-path value in the same migration slice rather than deferring live call-site convergence."
requirements-completed: [HOST-01, HOST-02]
duration: unavailable
completed: 2026-05-13
---

# Phase 13 Plan 01: Host Path Correctness Boundary Summary

**Shared `host_file` helpers and `detail::host_file_path` replaced `writer_disk_source` so writer-side host I/O could converge on one Windows-correct boundary.**

## Performance

- **Duration:** Unavailable — current evidence does not prove a per-plan execution duration.
- **Started:** Unavailable — no attributable executor timestamp or task commit survives in reachable history.
- **Completed:** 2026-05-13 at phase level only — the repository proves the phase completion date, not a per-plan completion timestamp.
- **Tasks:** 2 planned tasks.
- **Files modified:** 11 files listed in the plan frontmatter, plus current working-tree evidence of `src/detail/host_path.cpp` in the same migration slice.

## Accomplishments

- Replaced the old helper naming family with shared `host_file` declarations and implementation files.
- Introduced `detail::host_file_path` so caller-owned diagnostics text and resolved Windows I/O state travel together.
- Migrated writer prepare/layout call sites and renamed unit coverage to the new helper family.

## Task Commits

Reachable git history does not prove atomic task commits for this plan:

1. **Task 1: Introduce `host_file` / `host_file_path` and migrate live writer callers onto that shared path type in the same buildable slice** - **Unavailable** (reachable history contains no attributable `13-01` task commit).
2. **Task 2: Remove the old helper family and finish test/CMake cleanup once writer callers are already on `host_file_path`** - **Unavailable** (reachable history contains no attributable `13-01` task commit).

**Plan metadata:** **Unavailable** for this summary. Related phase-level docs commit `d15b9ad` updated `13-VERIFICATION.md`, `.planning/STATE.md`, and `.planning/ROADMAP.md`, but it did not commit `13-01-SUMMARY.md`.

## Files Created/Modified

- `src/detail/host_file.hpp` - Declares the neutral shared host-file helper family.
- `src/detail/host_file.cpp` - Implements shared file open/inspect/read helpers.
- `src/detail/host_path.hpp` - Declares the shared diagnostics-plus-resolved-path value.
- `src/detail/host_path.cpp` - Resolves UTF-8 host paths into Windows filesystem paths.
- `src/formats/bsa/tes4_bsa_prepare.cpp` - Adopts shared host-file-path input for TES4 writer preparation.
- `src/formats/bsa/tes4_bsa_layout.cpp` - Carries shared host-file-path usage into TES4 layout work.
- `src/formats/ba2/ba2_gnrl_prepare.cpp` - Adopts shared host-file-path input for BA2 GNRL preparation.
- `src/formats/ba2/ba2_dx10_prepare.cpp` - Adopts shared host-file-path input for BA2 DX10 preparation.
- `tests/unit/host_file_tests.cpp` - Holds renamed helper regression coverage.
- `tests/CMakeLists.txt` - Registers the renamed helper test suite.

## Decisions Made

- Use one shared host-file boundary immediately instead of preserving a compatibility helper family.
- Keep diagnostics text caller-owned while the resolved filesystem path becomes the only real I/O input.

## Deviations from Plan

Execution-history evidence is incomplete rather than cleanly preserved.

- Reachable git history does not prove the per-task commits required by the executor contract.
- The summary itself existed only as an untracked placeholder draft before this repair.
- Current repository state still contains uncommitted Phase 13 implementation changes, so a clean plan close-out cannot be claimed retroactively.

**Total deviations:** Unavailable from executor history; current repair evidence proves documentation and close-out drift.
**Impact on plan:** The shipped behavior can be described from plan and verification artifacts, but provenance, timings, and clean close-out claims cannot be reconstructed honestly.

## Issues Encountered

- `13-REVIEW.md` records CR-01 (same-size in-place rewrite detection is insufficient) and WR-01 (coverage misses the same-size mutation case), so the broader phase review was not clean.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The shared host-file naming boundary described by Plan 01 is usable context for later Phase 13 plans.
- Phase 14 should not inherit a false "clean close-out" story: the current repository still shows uncommitted Phase 13 implementation changes, repaired summaries were previously missing, and CR-01 / WR-01 remain important downstream context.

## Self-Check: FAILED

- Current evidence does not prove `13-01` task commit hashes.
- Current evidence does not prove per-plan start/end timestamps or duration.
- The repository still contains uncommitted Phase 13 implementation changes and an open review blocker, so a clean close-out claim would be overstated.
