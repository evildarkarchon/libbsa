---
phase: 13-host-path-correctness-boundary
plan: 02
subsystem: infra
tags: [host_file_path, host_file, filesystem, writer, catch2, tdd]
requires:
  - phase: 13-host-path-correctness-boundary
    provides: neutral host_file helper seam and migrated writer helper names from Plan 01
provides:
  - shared detail::host_file_path value with original UTF-8 text and resolved filesystem path
  - host_file helper overloads for host_file_path and resolved std::filesystem::path inputs
  - migrated writer prepare/layout reads that resolve once before shared helper I/O
affects: [phase-13-plan-03, archive-open-boundary, validation-boundary, writer-io]
tech-stack:
  added: []
  patterns: [resolve-once host_file_path contract, diagnostics-only original UTF-8 text, resolved-path host_file I/O]
key-files:
  created: [src/detail/host_file_path.hpp, src/detail/host_file_path.cpp]
  modified: [CMakeLists.txt, src/detail/host_file.hpp, src/detail/host_file.cpp, src/formats/bsa/tes4_bsa_prepare.cpp, src/formats/bsa/tes4_bsa_layout.cpp, src/formats/ba2/ba2_gnrl_prepare.cpp, src/formats/ba2/ba2_dx10_prepare.cpp, tests/unit/host_file_tests.cpp, tests/unit/host_file_writer_name_tests.cpp]
key-decisions:
  - "host_file helpers now accept host_file_path or resolved std::filesystem::path inputs so raw UTF-8 text stays diagnostics-only once resolved."
  - "Migrated writer call sites resolve disk-source paths once per operation and keep caller-owned diagnostic strings unchanged."
patterns-established:
  - "Shared host-file work resolves UTF-8 text once, stores both forms, and performs later I/O only through the resolved path."
  - "Writer-side disk-source validation and streaming now use the same detail host-path contract that later reader-side plans will consume."
requirements-completed: [HOST-01]
duration: 5 min
completed: 2026-05-13
---

# Phase 13 Plan 02: Shared host_file_path contract summary

**Shared `host_file_path` state now keeps original UTF-8 diagnostics text beside one resolved Windows path while migrated writer reads reopen only through the resolved boundary.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-13T16:05:07-07:00
- **Completed:** 2026-05-13T16:10:09-07:00
- **Tasks:** 3
- **Files modified:** 11

## Accomplishments
- Added `src/detail/host_file_path.*` as the shared internal value that carries original UTF-8 text plus one resolved filesystem path.
- Reworked `host_file` helpers to accept the shared contract or a resolved `std::filesystem::path` primitive instead of reopening from raw text-only inputs.
- Migrated TES4 and BA2 writer preparation/layout reads onto the shared contract while preserving caller-owned diagnostics and existing writer behavior.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Introduce the shared host_file_path contract and teach host_file helpers to consume it** - `064d854` (test)
2. **Task 1 GREEN: Introduce the shared host_file_path contract and teach host_file helpers to consume it** - `4b791e4` (feat)
3. **Task 2 RED: Migrate writer prepare/layout call sites onto the shared internal host-path contract** - `e8fc51a` (test)
4. **Task 2 GREEN: Migrate writer prepare/layout call sites onto the shared internal host-path contract** - `8fed332` (feat)
5. **Task 3: Prove D-14 convergence and remove raw-text helper drift from the migrated writer seam** - `c613873` (refactor)

**Plan metadata:** `pending`

## Files Created/Modified
- `src/detail/host_file_path.hpp` - Declares the shared internal host-path value and the resolve-once helper contract.
- `src/detail/host_file_path.cpp` - Builds the shared host-path value from caller UTF-8 text.
- `src/detail/host_file.hpp` - Declares resolved-path and `host_file_path` helper overloads with updated doc comments.
- `src/detail/host_file.cpp` - Routes all host-file helper I/O through resolved paths and forwards `host_file_path` callers without reopening from raw text.
- `src/formats/bsa/tes4_bsa_prepare.cpp` - Resolves TES4 writer disk-source paths before validation, prefix reads, and payload sizing.
- `src/formats/bsa/tes4_bsa_layout.cpp` - Resolves TES4 dedupe disk-source paths before chunk and exact-byte comparisons.
- `src/formats/ba2/ba2_gnrl_prepare.cpp` - Resolves BA2 GNRL disk-source paths before size, hash, and validation helper calls.
- `src/formats/ba2/ba2_dx10_prepare.cpp` - Resolves DDS source paths before helper reads and uses resolved snapshot paths directly for chunk assembly.
- `tests/unit/host_file_tests.cpp` - Verifies `host_file` behavior through the shared `host_file_path` contract.
- `tests/unit/host_file_writer_name_tests.cpp` - Pins the helper surface and migrated writer files to the shared contract.
- `CMakeLists.txt` - Adds `src/detail/host_file_path.cpp` to the library build.

## Decisions Made
- Kept `host_file_path::original_utf8` diagnostics-only and funneled actual file opens through `host_file_path::resolved` so future read-side work inherits one Windows path policy.
- Added `std::filesystem::path` helper overloads alongside `host_file_path` overloads so already-resolved internal paths, such as DX10 snapshot files, do not need synthetic UTF-8 round trips.
- Tightened the `host_file_path.hpp` doc comment to state that D-14 convergence is deliberate; later reader open/validation work should reuse this same contract.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Plan 13-03 can now move archive open-time host-path storage onto `detail::host_file_path` without inventing a second internal contract.
- Writer-side prepare/layout seams already prove the shared helper boundary works with preserved diagnostics, reducing risk for the reader/parser migration.

## Known Stubs

None.

## Self-Check: PASSED

- FOUND: `.planning/phases/13-host-path-correctness-boundary/13-02-SUMMARY.md`
- FOUND: `src/detail/host_file_path.hpp`
- FOUND: `src/detail/host_file_path.cpp`
- FOUND: `src/detail/host_file.hpp`
- FOUND: `src/detail/host_file.cpp`
- FOUND COMMIT: `064d854`
- FOUND COMMIT: `4b791e4`
- FOUND COMMIT: `e8fc51a`
- FOUND COMMIT: `8fed332`
- FOUND COMMIT: `c613873`

---
*Phase: 13-host-path-correctness-boundary*
*Completed: 2026-05-13*
