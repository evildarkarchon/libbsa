---
phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
plan: 08
subsystem: testing
tags: [cpp, ba2, dx10, malformed-fixtures, compression, canonical-paths]
requires:
  - phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
    provides: BA2 DX10 parser, malformed fixture harness, and DDS extraction validation
provides:
  - Generated malformed DX10 duplicate canonical path fixture
  - Generated malformed DX10 unsupported CompressionMethod fixture
  - Strict malformed manifest expected_error handling
affects: [ba2-dx10-parser, malformed-archive-hardening, fixture-generation]
tech-stack:
  added: []
  patterns: [required malformed case assertions, strict manifest enum conversion]
key-files:
  created:
    - tests/fixtures/generated/archives/ba2_dx10_duplicate_canonical_path.ba2
    - tests/fixtures/generated/archives/ba2_dx10_unsupported_compression.ba2
  modified:
    - tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp
    - tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json
    - tests/unit/ba2_dx10_malformed_tests.cpp
key-decisions:
  - "Malformed DX10 manifests now fail tests on unknown expected_error strings instead of silently mapping typos to invalid_argument."
  - "Unsupported Starfield v3 DX10 CompressionMethod coverage uses detector-time unsupported errors with generated repository-owned bytes."
patterns-established:
  - "Malformed fixture manifests must list required case IDs explicitly in tests so coverage gaps fail loudly."
requirements-completed: [DDS-01, DDS-02, DDS-03, DDS-05]
duration: 2min
completed: 2026-05-09
---

# Phase 06 Plan 08: Malformed BA2 DX10 Coverage Summary

**Generated duplicate canonical path and unsupported compression DX10 fixtures with strict manifest error-code validation**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-09T04:56:00Z
- **Completed:** 2026-05-09T04:58:12Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Required malformed DX10 tests now include `ba2_dx10_duplicate_canonical_path` and `ba2_dx10_unsupported_compression`.
- Unknown malformed manifest `expected_error` strings now call Catch2 `FAIL`, preventing typo-masked test passes.
- Fixture generation now emits committed duplicate canonical path and unsupported CompressionMethod BA2 byproducts plus manifest entries.

## Task Commits

1. **Task 1: Require missing malformed DX10 cases and strict error strings** - `fbaf5e3` (test)
2. **Task 2: Generate duplicate canonical path and unsupported compression fixtures** - `f2386a2` (feat)

**Plan metadata:** pending final metadata commit

## Files Created/Modified

- `tests/unit/ba2_dx10_malformed_tests.cpp` - Requires missing case IDs and fails loudly on unknown manifest error strings.
- `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` - Adds generation for duplicate canonical path and unsupported compression malformed archives.
- `tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json` - Lists the new malformed cases and stable expected errors.
- `tests/fixtures/generated/archives/ba2_dx10_duplicate_canonical_path.ba2` - Generated duplicate canonical-path fixture.
- `tests/fixtures/generated/archives/ba2_dx10_unsupported_compression.ba2` - Generated unsupported CompressionMethod fixture.

## Decisions Made

- Unsupported DX10 compression coverage uses a structurally valid Starfield v3 archive with `CompressionMethod` 99 so detection fails closed with `error_code::unsupported` before path or extension inference can occur.
- Duplicate canonical path coverage uses two generated texture names that differ only by case, proving open-time canonical lookup rejects collisions with `error_code::format_error`.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Threat Flags

Omitted - no new network endpoints, auth paths, or file access trust boundaries beyond planned generated fixture/test surfaces.

## Known Stubs

None.

## Next Phase Readiness

- Phase 6 malformed DX10 coverage now includes duplicate canonical paths, unsupported compression metadata, and strict manifest error-code conversion.
- DDS read reconstruction is ready for phase-level verification.

## Self-Check: PASSED

- Found `tests/unit/ba2_dx10_malformed_tests.cpp`.
- Found `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.
- Found `tests/fixtures/generated/archives/ba2_dx10_duplicate_canonical_path.ba2`.
- Found `tests/fixtures/generated/archives/ba2_dx10_unsupported_compression.ba2`.
- Found commit `fbaf5e3`.
- Found commit `f2386a2`.
- `git -C TES5Edit status --short` produced no output.

---
*Phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction*
*Completed: 2026-05-09*
