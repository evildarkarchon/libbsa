---
phase: 07-tes4-family-bsa-write-new-support
plan: 01
subsystem: public-api
tags: [cpp20, bsa-writer, public-api, tdd, cmake]

requires:
  - phase: 03-format-detection-and-tes4-family-bsa-read-extract
    provides: TES4-family reader/result public API patterns and boundary tests
provides:
  - Dependency-light TES4-family writer public contract
  - Writer API exposure through the umbrella header and CMake header file set
  - Public include boundary coverage for writer types and fallible methods
affects: [tes4-family-bsa-write-new-support, writer-implementation, public-api]

tech-stack:
  added: []
  patterns: [C++20 result-returning writer facade declarations, public include boundary static assertions]

key-files:
  created: [include/libbsa/writer.hpp]
  modified: [tests/unit/public_include_boundary_tests.cpp, include/libbsa/libbsa.hpp, CMakeLists.txt]

key-decisions:
  - "Expose TES4-family write-new as a dependency-light writer object with named target and compression policy enums."
  - "Keep Phase 7 Plan 01 writer methods declared only; later implementation plans provide method bodies while this plan verifies the public contract without executing them."

patterns-established:
  - "Public writer headers use libbsa-owned enums and result<void> rather than codec, DirectX, Windows, TES5Edit, or C++23 types."
  - "Public boundary tests validate writer compile-time contracts with type traits and requires expressions."

requirements-completed: [WBSA-01, WBSA-02, WBSA-03, WBSA-10]

duration: 3min
completed: 2026-05-09
---

# Phase 07 Plan 01: Public TES4-Family Writer Contract Summary

**Dependency-light C++20 TES4-family writer API contract with target profiles, compression policies, and public boundary tests.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T06:08:58Z
- **Completed:** 2026-05-09T06:11:08Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added RED public-boundary assertions for TES4-family writer types and method return contracts, then verified they failed because the API did not exist.
- Added `include/libbsa/writer.hpp` with Doxygen-documented target/profile enums, writer options, and `tes4_bsa_writer` declarations using `libbsa::result<void>`.
- Exported the writer API through `libbsa.hpp` and the public CMake header file set while preserving forbidden-token boundary checks.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing public writer boundary expectations** - `53f821d` (test)
2. **Task 2 GREEN: Add dependency-light writer API contracts** - `8a37859` (feat)

**Plan metadata:** recorded in final docs commit.

## Files Created/Modified

- `include/libbsa/writer.hpp` - New public writer contract for TES4-family write-new archives.
- `include/libbsa/libbsa.hpp` - Umbrella include now exposes the writer API.
- `CMakeLists.txt` - Public header file set now installs `writer.hpp`.
- `tests/unit/public_include_boundary_tests.cpp` - Public boundary coverage now asserts writer API types and method contracts.

## Decisions Made

- Exposed the writer surface as `tes4_bsa_writer` plus named target/compression-policy enums, matching the Phase 7 decision to avoid raw archive flag bits in public APIs.
- Kept public writer methods declaration-only in this plan so later implementation plans can define storage and serialization internals without changing the consumer-facing contract.

## Deviations from Plan

None - plan executed exactly as written.

## TDD Gate Compliance

- **RED:** `53f821d` added failing public-boundary tests; `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` failed on missing writer symbols.
- **GREEN:** `8a37859` added the public writer contract and exported header; public boundary tests passed.
- **REFACTOR:** None needed.

## Issues Encountered

- `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` initially passed against stale binaries during RED. The RED gate was correctly verified by rebuilding `libbsa_tests`, which failed on the missing writer symbols before Task 2.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None - this plan adds public type declarations only; implementation trust-boundary validation is covered by the plan threat model and deferred implementation plans.

## Next Phase Readiness

- Ready for Plan 07-02 to define private writer storage and initial add-entry behavior behind the stable public contract.
- No blockers; `TES5Edit/` remained read-only.

## Self-Check: PASSED

- Verified created/modified files exist on disk.
- Verified task commits `53f821d` and `8a37859` exist in git history.
- Verified plan-level public boundary tests pass and `TES5Edit/` status is clean.

---
*Phase: 07-tes4-family-bsa-write-new-support*
*Completed: 2026-05-09*
