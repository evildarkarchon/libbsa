---
phase: 11-compatibility-warnings-validation-api-and-hardening
plan: 04
subsystem: validation-policy
tags: [cpp20, validation-api, compatibility-warnings, evidence-catalog, tdd]

requires:
  - phase: 11-03
    provides: Public compatibility warning codes and writer-output warning tests.
provides:
  - Machine-checked compatibility evidence catalog for public warning codes.
  - Fixture policy updates for compatibility evidence and optional local corpus checks.
  - RED/GREEN policy test coverage requiring Rule and Evidence text for every public warning code.
affects: [phase-11-validation-api, compatibility-warnings, fixture-policy, evidence-catalog]

tech-stack:
  added: []
  patterns:
    - Markdown catalog entries use one heading per public warning code so tests can enforce coverage.
    - Optional local corpus checks remain smoke/compare-only under `requires-game-fixture`.

key-files:
  created:
    - docs/compatibility-evidence.md
  modified:
    - tests/unit/validation_policy_tests.cpp
    - tests/fixtures/README.md

key-decisions:
  - "Public compatibility warning evidence is cataloged by one section per warning code so the policy test can enforce local Rule and Evidence coverage."
  - "Optional local game or BSArchPro-derived checks remain smoke/compare-only, require `requires-game-fixture`, and skip when `LIBBSA_GAME_FIXTURES` is unset."

patterns-established:
  - "Compatibility evidence must be backed by generated fixtures, writer-output archives, read-only reference notes, or opt-in local corpus checks."
  - "Generated legal fixtures and writer-output archives remain the mandatory evidence path for default acceptance."

requirements-completed: [COMP-01, COMP-03, COMP-06]

duration: 2 min
completed: 2026-05-10
---

# Phase 11 Plan 04: Compatibility Evidence Catalog Summary

**Machine-checked catalog connecting public compatibility warning codes to generated and writer-output evidence**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-10T04:20:31Z
- **Completed:** 2026-05-10T04:22:44Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added a RED `validation_policy` test that fails when the compatibility evidence catalog is missing or a public warning-code entry lacks `Rule:` and `Evidence:` text.
- Added `docs/compatibility-evidence.md` with catalog entries for `compressed_sound_payload`, `bsa_embedded_name_compatibility_risk`, and `target_family_mismatch`.
- Updated `tests/fixtures/README.md` to point maintainers to the catalog and keep optional local game or BSArchPro-derived checks skipped by default.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED catalog coverage check** - `4c6ba6a` (test)
2. **Task 2: GREEN evidence catalog and fixture policy update** - `c836208` (feat)

_Note: This TDD plan produced RED and GREEN commits. No refactor commit was needed._

## Files Created/Modified

- `docs/compatibility-evidence.md` - Catalogs each public compatibility warning code with rule, evidence, and default gate text.
- `tests/unit/validation_policy_tests.cpp` - Adds the machine check for catalog coverage and entry shape.
- `tests/fixtures/README.md` - Documents compatibility evidence policy and optional local corpus rules.

## Verification

- RED: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- RED: `ctest --preset windows-msvc-debug-static -R "compatibility evidence|validation_policy" --output-on-failure` - failed as expected because `docs/compatibility-evidence.md` was missing.
- GREEN: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- GREEN: `ctest --preset windows-msvc-debug-static -R "compatibility evidence|validation_policy|requires-game-fixture" --output-on-failure` - passed, 2/2 tests.
- Final repeat: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- Final repeat: `ctest --preset windows-msvc-debug-static -R "compatibility evidence|validation_policy|requires-game-fixture" --output-on-failure` - passed, 2/2 tests.
- `rg -n "compatibility evidence catalog documents public warning codes" tests/unit/validation_policy_tests.cpp` - found the new policy test.
- `rg -n "compressed_sound_payload|bsa_embedded_name_compatibility_risk|target_family_mismatch" tests/unit/validation_policy_tests.cpp docs/compatibility-evidence.md` - found all public warning codes.
- `rg -n "Rule:|Evidence:|generated|writer-output|requires-game-fixture|LIBBSA_GAME_FIXTURES" docs/compatibility-evidence.md` - found required catalog and optional corpus policy text.
- `rg -n "compatibility evidence|docs/compatibility-evidence.md|requires-game-fixture" tests/fixtures/README.md` - found fixture policy updates.
- `git status --short TES5Edit` - returned no output.

## Decisions Made

- Used one Markdown section per warning code instead of a compact table so the policy test can verify `Rule:` and `Evidence:` text within each entry.
- Kept optional local corpus checks as supplemental smoke/compare evidence only; default acceptance remains generated fixtures, writer-output archives, and policy tests.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- The first PowerShell wrapper used for GSD state updates indexed a single-string CLI command as characters; reran the updates with direct `gsd-sdk query ...` invocations.
- The positional `state.record-metric` and `state.add-decision` calls reported missing named fields, and the positional session update did not change `Stopped at`; reran them with the SDK's named arguments.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 11-06 to add the consolidated malformed hardening matrix and for Plan 11-07 to build on the compatibility evidence policy when documenting sanitizer validation.

## Self-Check: PASSED

- Found created/modified files: `docs/compatibility-evidence.md`, `tests/unit/validation_policy_tests.cpp`, `tests/fixtures/README.md`, and this SUMMARY.
- Found task commits: `4c6ba6a` and `c836208`.
- Verification commands listed above passed except for the intentional RED failure before the catalog existed.
- Stub scan over touched files returned no TODO, FIXME, placeholder, empty hardcoded UI data, or "coming soon" markers.

---
*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Completed: 2026-05-10*
