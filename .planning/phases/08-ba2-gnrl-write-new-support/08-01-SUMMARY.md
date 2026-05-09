---
phase: 08-ba2-gnrl-write-new-support
plan: 01
subsystem: public-api
tags: [cpp20, ba2-gnrl-writer, public-api, tdd, boundary-tests]

requires:
  - phase: 07-tes4-family-bsa-write-new-support
    provides: TES4-family writer object pattern, compression policy enums, and public boundary testing approach
provides:
  - Dependency-light BA2 GNRL writer public contract
  - Explicit Fallout 4, Starfield v2, and Starfield v3 BA2 GNRL target profiles
  - Public boundary tests for BA2 GNRL writer type and method contracts
affects: [ba2-gnrl-write-new-support, writer-implementation, public-api]

tech-stack:
  added: []
  patterns: [C++20 result-returning writer facade declarations, public include boundary static assertions]

key-files:
  created: []
  modified: [include/libbsa/writer.hpp, tests/unit/public_include_boundary_tests.cpp]

key-decisions:
  - "Expose BA2 GNRL write-new as a dedicated dependency-light writer object with named target profiles."
  - "Keep Plan 08-01 to public declarations and compile-time boundary checks; later plans provide writer-owned state and serialization bodies."
  - "Default Starfield writer options expose Unknown1=1, Unknown2=0, and CompressionMethod=3 while allowing caller overrides."

patterns-established:
  - "BA2 GNRL writer APIs mirror the Phase 7 writer-object flow using target/options construction, add_file/add_bytes, copied memory semantics, and host-path write_to finalization."
  - "Public boundary tests validate BA2 writer contracts with type traits and requires expressions before private implementation exists."

requirements-completed: [WBA2-01, WBA2-02, WBA2-05]

duration: 3min
completed: 2026-05-09
---

# Phase 08 Plan 01: Public BA2 GNRL Writer Contract Summary

**Dependency-light C++20 BA2 GNRL writer API contract with Fallout 4 and Starfield target profiles, Starfield header options, and public boundary tests.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T07:53:28Z
- **Completed:** 2026-05-09T07:56:27Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added RED public-boundary assertions for `ba2_gnrl_target`, BA2 GNRL writer option types, writer construction, and `add_file`/`add_bytes`/`write_to` result contracts.
- Added Doxygen-documented public BA2 GNRL declarations in `writer.hpp`, including Fallout 4, Starfield v2, and Starfield v3 target profiles.
- Exposed Starfield BA2 GNRL header option defaults and optional per-entry record flags without leaking private codecs, DirectXTex, TES5Edit, or C++23 types.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add public BA2 GNRL writer boundary expectations** - `5a5758d` (test)
2. **Task 2 GREEN: Add dependency-light public BA2 GNRL writer declarations** - `30ab205` (feat)

**Plan metadata:** recorded in final docs commit.

## Files Created/Modified

- `tests/unit/public_include_boundary_tests.cpp` - Adds compile-time BA2 GNRL writer type, target, constructor, and method-return checks.
- `include/libbsa/writer.hpp` - Adds public BA2 GNRL target, writer options, entry options, and writer declaration surface.

## Decisions Made

- Exposed a dedicated `ba2_gnrl_writer` rather than a generic BA2 writer shell, matching Phase 8 D-01 and keeping DX10 writer design out of this plan.
- Reused existing `archive_compression_policy` and `entry_compression_policy` instead of adding codec-specific public knobs.
- Kept this plan declaration-only so downstream implementation plans can add writer state and BA2 serialization without changing the consumer-facing contract.

## Deviations from Plan

None - plan executed exactly as written.

## TDD Gate Compliance

- **RED:** `5a5758d` added failing public-boundary tests; `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` failed on missing BA2 GNRL writer symbols.
- **GREEN:** `30ab205` added the public writer contract; `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` and `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` passed.
- **REFACTOR:** None needed.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None - declarations without method bodies are intentional for this contract-first TDD plan and are implemented by later Phase 8 plans.

## Threat Flags

None - this plan adds public type declarations only; the public dependency boundary is covered by forbidden-token tests.

## Next Phase Readiness

- Ready for Plan 08-02 to adjust BA2 GNRL reader parsing for end-of-archive filename tables.
- Ready for Plan 08-03 to implement BA2 GNRL writer-owned state behind the stable public contract.
- No blockers; `TES5Edit/` remained read-only and clean.

## Self-Check: PASSED

- Verified modified files exist on disk.
- Verified task commits `5a5758d` and `30ab205` exist in git history.
- Verified plan-level public boundary tests pass and `TES5Edit/` status is clean.

---
*Phase: 08-ba2-gnrl-write-new-support*
*Completed: 2026-05-09*
