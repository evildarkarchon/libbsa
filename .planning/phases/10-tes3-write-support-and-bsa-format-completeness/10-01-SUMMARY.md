---
phase: 10-tes3-write-support-and-bsa-format-completeness
plan: 01
subsystem: public-api
tags: [cpp20, tes3, bsa, writer, public-boundary, tdd]

requires:
  - phase: 04-tes3-bsa-read-extract
    provides: TES3 path normalization and reader behavior used as the future writer oracle
  - phase: 07-tes4-family-bsa-write-new-support
    provides: Existing BSA writer public/private bridge pattern
provides:
  - Dependency-light public TES3 writer contract
  - Private TES3 writer entry state and serializer bridge
  - Public include boundary tests for the TES3 writer API
affects: [phase-10, tes3-writer, public-api, bsa-completeness]

tech-stack:
  added: []
  patterns: [TDD public contract, shared_ptr-backed writer state, dependency-light public headers]

key-files:
  created:
    - src/formats/bsa/tes3_bsa_writer.hpp
    - src/formats/bsa/tes3_bsa_writer.cpp
  modified:
    - include/libbsa/writer.hpp
    - CMakeLists.txt
    - tests/unit/public_include_boundary_tests.cpp

key-decisions:
  - "TES3 writer is a dedicated raw/uncompressed public writer with only overwrite_existing options."
  - "TES3 add_file validates archive path and non-empty host path immediately while deferring source existence checks to write_to."
  - "TES3 preserved archive names convert backslashes to slashes while preserving caller-provided case."

patterns-established:
  - "Public writer bridge: public writer.hpp declarations delegate to private formats::bsa serializer state."
  - "Boundary-first TDD: public include contract fails before API symbols exist and passes after implementation."

requirements-completed: [WBSA-04]

duration: 3min
completed: 2026-05-10
---

# Phase 10 Plan 01: TES3 Writer Public Contract Summary

**Dependency-light raw TES3/Morrowind BSA writer API with private writer-state bridge and public boundary tests.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T23:59:28Z
- **Completed:** 2026-05-10T00:02:49Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments

- Added RED public include assertions for `tes3_bsa_writer_options`, `tes3_bsa_writer`, and exact `options`, `add_file`, `add_bytes`, and `write_to` return types.
- Added Doxygen-documented public TES3 writer declarations that explicitly keep TES3 output raw/uncompressed with no compression, dedupe, or embedded-name controls.
- Added private `tes3_writer_entry` state and a minimal `write_tes3_bsa_archive` bridge registered in the library target.
- Verified the public header forbidden-token scan remains green and `TES5Edit/` remains untouched.

## TDD Cycle

- **RED:** `84e245b` added public boundary assertions; the build failed because `libbsa::tes3_bsa_writer_options` and `libbsa::tes3_bsa_writer` did not exist.
- **GREEN:** `382baca` added the public API, private bridge, and CMake source registration; the focused public boundary suite passed.
- **REFACTOR:** `770cdb3` removed an unused include and documented why TES3 separator normalization preserves case while converting `\\` to `/`; the focused public boundary suite still passed.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED public TES3 writer boundary contract** - `84e245b` (test)
2. **Task 2: GREEN public API and private bridge** - `382baca` (feat)
3. **Task 3: REFACTOR public comments and boundary hygiene** - `770cdb3` (refactor)

**Plan metadata:** committed after this summary is finalized.

## Files Created/Modified

- `include/libbsa/writer.hpp` - Adds raw-only TES3 writer options and public writer contract with Doxygen comments.
- `src/formats/bsa/tes3_bsa_writer.hpp` - Adds private TES3 writer entry state and serializer declaration.
- `src/formats/bsa/tes3_bsa_writer.cpp` - Adds writer-owned state, add-time archive path validation, memory copying, and minimal serializer bridge.
- `CMakeLists.txt` - Registers the TES3 writer implementation in the `libbsa` target.
- `tests/unit/public_include_boundary_tests.cpp` - Adds compile-time public contract assertions for the TES3 writer.

## Decisions Made

- Kept TES3 writer public API dedicated and raw-only per D-01 through D-04 rather than adding a target enum or generic BSA writer redesign.
- Used the existing preserved-path pattern (`\\` to `/`, case preserved) while storing a canonical normalized key for later duplicate validation and lookup semantics.
- Left non-empty TES3 serialization as an explicit `unsupported` bridge because Plan 02+ own source ownership, publish behavior, and full serializer finalization.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

| Stub | File | Reason |
|------|------|--------|
| Non-empty `write_tes3_bsa_archive` returns `error_code::unsupported` | `src/formats/bsa/tes3_bsa_writer.cpp` | Intentional Plan 01 bridge only; Plan 02 owns source ownership/finalization validation and later plans own byte serialization. Empty-entry `invalid_argument` is implemented as required by this plan. |

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: local-file-output-api | `include/libbsa/writer.hpp` | New public writer API accepts caller-provided host output/source paths; mitigations for path validation are in this plan and finalization/publish safety is assigned to subsequent Phase 10 plans. |

## Issues Encountered

- The repository had no top-level `build` CMake directory, so verification used the configured `windows-msvc-debug-static` preset build directory.

## User Setup Required

None - no external service configuration required.

## Verification

- `cmake --build build/windows-msvc-debug-static --config Debug --target libbsa_tests` — passed.
- `ctest --test-dir build/windows-msvc-debug-static -C Debug -R public_include_boundary --output-on-failure` — passed (3/3 tests).
- `git -C TES5Edit status --short` — passed with no output.
- `git log --oneline --grep="10-01"` — confirmed RED/GREEN/REFACTOR commits.

## Next Phase Readiness

- Ready for Plan 10-02 to implement TES3 source ownership, duplicate validation, and safe publish behavior behind the public bridge.

## Self-Check: PASSED

- Found all created/modified files listed in this summary.
- Found task commits `84e245b`, `382baca`, and `770cdb3` in git history.

---
*Phase: 10-tes3-write-support-and-bsa-format-completeness*
*Completed: 2026-05-10*
