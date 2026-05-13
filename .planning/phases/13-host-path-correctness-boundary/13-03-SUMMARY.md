---
phase: 13-host-path-correctness-boundary
plan: 03
subsystem: filesystem
tags: [windows, unicode, extraction, reader, host-path]
requires:
  - phase: 13-02
    provides: stored host-path ownership for open-time reader state and parser contracts
provides:
  - Extraction dispatch wired to stored `detail::host_file_path` state.
  - Reader contracts that reopen from the shared host-file boundary instead of raw caller text.
  - Post-open payload reads aligned with the same resolved Windows path used during open.
affects: [phase-13-04, phase-14]
tech-stack:
  added: []
  patterns:
    - Post-open extraction reuses stored host-path state only.
    - Reader backends reopen through `detail::open_host_file(...)` rather than direct narrow opens.
key-files:
  created: []
  modified:
    - src/archive.cpp
    - src/formats/bsa/tes3_bsa_reader.hpp
    - src/formats/bsa/tes3_bsa_reader.cpp
    - src/formats/bsa/tes4_bsa_reader.hpp
    - src/formats/bsa/tes4_bsa_reader.cpp
    - src/formats/ba2/ba2_gnrl_reader.hpp
    - src/formats/ba2/ba2_gnrl_reader.cpp
    - src/formats/ba2/ba2_dx10_reader.hpp
    - src/formats/ba2/ba2_dx10_reader.cpp
key-decisions:
  - "Carry stored host-path state through extraction dispatch instead of reopening from raw input text."
  - "Keep extraction behavior public-surface stable while changing only the host-file boundary underneath it."
patterns-established:
  - "Open-time and post-open file access share one resolved Windows path boundary."
  - "Reader contracts encode stored-path ownership so later validation work can reuse the same reopen paths."
requirements-completed: [HOST-01, HOST-02]
duration: unavailable
completed: 2026-05-13
---

# Phase 13 Plan 03: Host Path Correctness Boundary Summary

**Extraction dispatch and reader reopen paths now consume stored `detail::host_file_path` state so post-open payload reads stay on the same Windows-correct boundary as `archive_reader::open`.**

## Performance

- **Duration:** Unavailable — current evidence does not prove a per-plan execution duration.
- **Started:** Unavailable — no attributable executor timestamp or task commit survives in reachable history.
- **Completed:** 2026-05-13 at phase level only — the date is supported, but the plan-specific completion timestamp is not.
- **Tasks:** 1 planned task.
- **Files modified:** 9 files listed in the plan frontmatter.

## Accomplishments

- `archive.cpp` extraction dispatch now carries stored host-path state instead of reusing raw UTF-8 input text.
- TES3, TES4, BA2 GNRL, and BA2 DX10 reader interfaces migrated to `const detail::host_file_path&`.
- Reader reopen implementations route through shared host-file helpers so extraction reuses the same resolved Windows path model as open-time parsing.

## Task Commits

Reachable git history does not prove atomic task commits for this plan:

1. **Task 1: Migrate extraction dispatch plus reader contracts and reader reopens to the stored `detail::host_file_path`** - **Unavailable** (reachable history contains no attributable `13-03` task commit).

**Plan metadata:** **Unavailable** for this summary. Related phase-level docs commit `d15b9ad` does not prove a committed `13-03-SUMMARY.md`.

## Files Created/Modified

- `src/archive.cpp` - Passes stored host-path state through extraction dispatch.
- `src/formats/bsa/tes3_bsa_reader.hpp` - Accepts stored host-path input for TES3 extraction.
- `src/formats/bsa/tes3_bsa_reader.cpp` - Reopens TES3 archives through shared host-file helpers.
- `src/formats/bsa/tes4_bsa_reader.hpp` - Accepts stored host-path input for TES4 extraction.
- `src/formats/bsa/tes4_bsa_reader.cpp` - Reopens TES4 archives through shared host-file helpers.
- `src/formats/ba2/ba2_gnrl_reader.hpp` - Accepts stored host-path input for BA2 GNRL extraction.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - Reopens BA2 GNRL archives through shared host-file helpers.
- `src/formats/ba2/ba2_dx10_reader.hpp` - Accepts stored host-path input for BA2 DX10 extraction.
- `src/formats/ba2/ba2_dx10_reader.cpp` - Reopens BA2 DX10 archives through shared host-file helpers.

## Decisions Made

- Reuse stored reader-state path ownership for extraction rather than letting each reader family reopen differently.
- Preserve public extraction semantics while aligning all touched reader backends to the shared host-file helper boundary.

## Deviations from Plan

Execution-history evidence is incomplete rather than cleanly preserved.

- Reachable git history does not prove the task commit required by the executor contract.
- The summary itself existed only as an untracked placeholder draft before this repair.
- Current repository state still contains uncommitted Phase 13 implementation changes, so this plan cannot be presented as a fully committed close-out.

**Total deviations:** Unavailable from executor history; current repair evidence proves documentation and close-out drift.
**Impact on plan:** The extraction-boundary behavior is well supported by the plan and verification report, but commit provenance and timing data are not.

## Issues Encountered

- `13-REVIEW.md` carries CR-01 and WR-01 at the phase level, so downstream readers must not interpret the verified extraction migration as a clean hardening finish.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04 can reuse the stored-path extraction contracts established here when proving validation and canonical extraction from non-ASCII paths.
- Phase 14 planning should wait for the repaired Phase 13 artifacts to be committed, and it should carry forward the current uncommitted-workspace drift plus CR-01 / WR-01 context.

## Self-Check: FAILED

- Current evidence does not prove a `13-03` task commit hash.
- Current evidence does not prove per-plan start/end timestamps or duration.
- The repository still contains uncommitted Phase 13 implementation changes and an open review blocker, so a clean close-out claim would be overstated.
