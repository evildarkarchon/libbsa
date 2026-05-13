---
phase: 13-host-path-correctness-boundary
plan: 02
subsystem: filesystem
tags: [windows, unicode, archive-reader, parser, host-path]
requires:
  - phase: 13-01
    provides: shared `host_file` helpers and `detail::host_file_path`
provides:
  - One-time UTF-8 host-path resolution in `archive_reader::open`.
  - Parser contracts that consume stored `detail::host_file_path` instead of raw caller text.
  - Detection-prefix and file-size inspection routed through the shared host-file boundary.
affects: [phase-13-03, phase-13-04, phase-14]
tech-stack:
  added: []
  patterns:
    - Reader state owns resolved host-path data after open succeeds.
    - Parser entry points consume stored host-path state rather than reopening from raw UTF-8 text.
key-files:
  created: []
  modified:
    - src/archive.cpp
    - src/formats/bsa/tes3_bsa_parser.hpp
    - src/formats/bsa/tes3_bsa_parser.cpp
    - src/formats/bsa/tes4_bsa_parser.hpp
    - src/formats/bsa/tes4_bsa_parser.cpp
    - src/formats/ba2/ba2_gnrl_parser.hpp
    - src/formats/ba2/ba2_gnrl_parser.cpp
    - src/formats/ba2/ba2_dx10_parser.hpp
    - src/formats/ba2/ba2_dx10_parser.cpp
key-decisions:
  - "Resolve the public UTF-8 host path exactly once inside `archive_reader::open`."
  - "Require parser families to consume `const detail::host_file_path&` so they cannot bypass the stored boundary."
patterns-established:
  - "Open-time state stores both diagnostics text and the resolved Windows path for later parser reuse."
  - "Detection-prefix reads and parser opens use the same shared host-file helper boundary."
requirements-completed: [HOST-01, HOST-02]
duration: unavailable
completed: 2026-05-13
---

# Phase 13 Plan 02: Host Path Correctness Boundary Summary

**`archive_reader::open` now owns one-time UTF-8 host-path resolution and passes stored `detail::host_file_path` state into every migrated parser family.**

## Performance

- **Duration:** Unavailable — current evidence does not prove a per-plan execution duration.
- **Started:** Unavailable — no attributable executor timestamp or task commit survives in reachable history.
- **Completed:** 2026-05-13 at phase level only — the date is supported, but the plan-specific completion timestamp is not.
- **Tasks:** 1 planned task.
- **Files modified:** 9 files listed in the plan frontmatter.

## Accomplishments

- `archive_reader::open` became the single point that resolves and stores Windows-correct host-path state.
- Detection-prefix reads and size inspection moved onto shared host-file helpers instead of ad hoc raw-string opens.
- TES3, TES4, BA2 GNRL, and BA2 DX10 parser contracts migrated to `const detail::host_file_path&`.

## Task Commits

Reachable git history does not prove atomic task commits for this plan:

1. **Task 1: Migrate open-time state ownership plus parser contracts and parser opens to `detail::host_file_path` in one buildable slice** - **Unavailable** (reachable history contains no attributable `13-02` task commit).

**Plan metadata:** **Unavailable** for this summary. Related phase-level docs commit `d15b9ad` does not prove a committed `13-02-SUMMARY.md`.

## Files Created/Modified

- `src/archive.cpp` - Stores resolved host-path state and dispatches detection/parser work through it.
- `src/formats/bsa/tes3_bsa_parser.hpp` - Accepts stored host-path input.
- `src/formats/bsa/tes3_bsa_parser.cpp` - Opens TES3 archives through shared host-file helpers.
- `src/formats/bsa/tes4_bsa_parser.hpp` - Accepts stored host-path input.
- `src/formats/bsa/tes4_bsa_parser.cpp` - Opens TES4 archives through shared host-file helpers.
- `src/formats/ba2/ba2_gnrl_parser.hpp` - Accepts stored host-path input.
- `src/formats/ba2/ba2_gnrl_parser.cpp` - Opens BA2 GNRL archives through shared host-file helpers.
- `src/formats/ba2/ba2_dx10_parser.hpp` - Accepts stored host-path input.
- `src/formats/ba2/ba2_dx10_parser.cpp` - Opens BA2 DX10 archives through shared host-file helpers.

## Decisions Made

- Keep the public API input unchanged while moving all internal parser entry points to the stored host-path type.
- Preserve parser bounds/error logic while replacing raw-string file opens with shared host-file helpers.

## Deviations from Plan

Execution-history evidence is incomplete rather than cleanly preserved.

- Reachable git history does not prove the task commit required by the executor contract.
- The summary itself existed only as an untracked placeholder draft before this repair.
- Current repository state still contains uncommitted Phase 13 implementation changes, so this plan cannot be presented as a fully committed close-out.

**Total deviations:** Unavailable from executor history; current repair evidence proves documentation and close-out drift.
**Impact on plan:** The parser-migration story is well supported by the plan and verification report, but commit provenance and timing data are not.

## Issues Encountered

- `13-REVIEW.md` carries CR-01 and WR-01 at the phase level, so downstream readers must not assume a clean hardening pass even though the parser-boundary behavior was verified.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03 can reuse the stored host-path ownership model established here.
- Phase 14 planning should wait for the repaired Phase 13 artifacts to be committed, and it should carry forward the current uncommitted-workspace drift plus CR-01 / WR-01 context.

## Self-Check: FAILED

- Current evidence does not prove a `13-02` task commit hash.
- Current evidence does not prove per-plan start/end timestamps or duration.
- The repository still contains uncommitted Phase 13 implementation changes and an open review blocker, so a clean close-out claim would be overstated.
