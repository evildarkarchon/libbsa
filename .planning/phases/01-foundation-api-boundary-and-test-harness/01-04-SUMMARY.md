---
phase: 01-foundation-api-boundary-and-test-harness
plan: 04
subsystem: testing
tags: [fixtures, ctest, policy, tes5edit-boundary]
requires: []
provides:
  - Legal generated fixture directory layout
  - Ignored local game fixture boundary
  - Fixture provenance and label policy
affects: [phase-01, testing, future-fixtures]
tech-stack:
  added: []
  patterns: [generated-fixture-provenance, local-fixture-ignore-boundary]
key-files:
  created: [tests/fixtures/README.md, tests/fixtures/generated/source/.gitkeep, tests/fixtures/generated/archives/.gitkeep, tests/fixtures/local/.gitkeep]
  modified: [.gitignore]
key-decisions:
  - "Local game-derived fixture data is ignored under tests/fixtures/local while preserving a committed .gitkeep."
  - "Fixture policy requires provenance and explicitly blocks TES5Edit as fixture workspace or fixture source."
patterns-established:
  - "Committed generated fixtures must document generator/source recipe, legal provenance, and behavior proven."
requirements-completed: [FND-07]
duration: 5 min
completed: 2026-05-08
---

# Phase 01 Plan 04: Fixture Policy and Layout Summary

**Legal generated fixture layout with ignored local game-data boundary and TES5Edit fixture prohibition**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-08T00:32:00Z
- **Completed:** 2026-05-08T00:37:00Z
- **Tasks:** 2 completed
- **Files modified:** 5

## Accomplishments

- Added committed generated fixture source/archive directories and an ignored local fixture directory.
- Appended `.gitignore` rules that ignore local fixture contents while preserving `tests/fixtures/local/.gitkeep`.
- Documented fixture provenance requirements, local game fixture setup, test labels, and the `TES5Edit/` boundary.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create fixture directory skeleton with local-only ignore boundary** - `d3c178e` (chore)
2. **Task 2: Document fixture provenance policy and label taxonomy** - `c2930ab` (docs)

## Files Created/Modified

- `.gitignore` - Local game-derived fixture ignore rules.
- `tests/fixtures/README.md` - Fixture policy, provenance requirements, TES5Edit boundary, and label taxonomy.
- `tests/fixtures/generated/source/.gitkeep` - Committed generated fixture source directory marker.
- `tests/fixtures/generated/archives/.gitkeep` - Committed generated archive fixture directory marker.
- `tests/fixtures/local/.gitkeep` - Ignored local fixture directory marker.

## Decisions Made

- Preserved existing `.gitignore` comments and appended fixture-specific rules at the end.
- Kept Phase 1 fixture work documentation-only for generated binary data; actual fixture archives are deferred until provenance/recipe can be documented.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Fixture directories and policy are ready for Plan 03 local fixture skip tests and later archive-format fixture expansion.

## Self-Check: PASSED

- Found `tests/fixtures/README.md`.
- Found generated and local `.gitkeep` markers.
- Found commits `d3c178e` and `c2930ab`.
- Verified `git check-ignore tests/fixtures/local/example.bsa` reports the file ignored.

---
*Phase: 01-foundation-api-boundary-and-test-harness*
*Completed: 2026-05-08*
