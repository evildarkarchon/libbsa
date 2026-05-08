---
phase: 03-format-detection-and-tes4-family-bsa-read-extract
plan: 02
subsystem: testing
tags: [cpp20, cmake, fixtures, bsa, tes4, deflate, lz4]

requires:
  - phase: 02-binary-i-o-paths-hashes-and-compression-services
    provides: Internal TES4 hash and compression routing helpers used by the generator.
  - phase: 03-format-detection-and-tes4-family-bsa-read-extract
    provides: Public reader contracts and test-only dependency boundaries from 03-01.
provides:
  - Deterministic TES4-family success fixture generator target.
  - Committed legal v103, v104, and v105 BSA success archives.
  - Machine-readable manifests for metadata, lookup, compression, embedded-name, and expected extraction bytes.
affects: [03-03 malformed fixtures, 03-04 detector parser, 03-05 metadata lookup, 03-06 extraction]

tech-stack:
  added: []
  patterns: [CMake custom fixture generation target, synthetic legal archive manifests]

key-files:
  created:
    - tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp
    - tests/fixtures/generated/archives/tes4_v103.bsa
    - tests/fixtures/generated/archives/tes4_v103_manifest.json
    - tests/fixtures/generated/archives/tes4_v104.bsa
    - tests/fixtures/generated/archives/tes4_v104_manifest.json
    - tests/fixtures/generated/archives/tes4_v105.bsa
    - tests/fixtures/generated/archives/tes4_v105_manifest.json
  modified:
    - tests/CMakeLists.txt
    - tests/fixtures/README.md

key-decisions:
  - "Fixture generation uses a C++ test tool linked to libbsa internals so archive hashes and compression payloads are produced by the same helpers later parser tests will validate."
  - "Success manifests store both canonical paths and original archive spelling so later tests can verify lookup normalization without trusting host filenames."

patterns-established:
  - "Generated archive fixtures live under tests/fixtures/generated/archives and are regenerated through a named CMake target."
  - "Fixture manifests include expected consumer-visible bytes after embedded-name stripping and decompression."

requirements-completed: [FMT-01, FMT-02, FMT-03, FMT-04, FMT-05, BSA-01, BSA-02, BSA-03, BSA-05, BSA-06, BSA-07]

duration: 8min
completed: 2026-05-08
---

# Phase 03 Plan 02: TES4-Family Success Fixture Generation Summary

**Deterministic legal BSA fixture generation for TES4 v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 with JSON extraction contracts**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-08T07:23:00Z
- **Completed:** 2026-05-08T07:31:20Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- Added a C++ fixture generator target, `generate_tes4_bsa_fixtures`, that writes success archives under `tests/fixtures/generated/archives`.
- Committed tiny synthetic v103, v104, and v105 BSA archives covering raw, deflate, LZ4-frame, mixed case/separator lookups, and embedded-name payload layouts.
- Added JSON manifests with archive metadata, canonical/original paths, lookup variants, hashes, record flags, offsets, compression methods, embedded-name prefix sizes, and expected extracted bytes/hashes.
- Documented the generator command, manifest schema, legal provenance, and TES5Edit read-only boundary in the fixture README.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create concrete generator target for success archives** - `588c7db` (feat)
2. **Task 2: Generate and verify v103/v104/v105 success manifests** - `c77939a` (feat)

**Plan metadata:** Pending final docs commit.

## Files Created/Modified

- `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` - C++ fixture generator with success and malformed modes.
- `tests/CMakeLists.txt` - Adds the generator executable and `generate_tes4_bsa_fixtures` custom target.
- `tests/fixtures/README.md` - Documents regeneration, manifest fields, provenance, and TES5Edit boundary.
- `tests/fixtures/generated/archives/tes4_v103.bsa` - TES4/Oblivion v103 success archive.
- `tests/fixtures/generated/archives/tes4_v103_manifest.json` - v103 metadata and extraction manifest.
- `tests/fixtures/generated/archives/tes4_v104.bsa` - FO3/FNV/Skyrim LE v104 success archive.
- `tests/fixtures/generated/archives/tes4_v104_manifest.json` - v104 metadata and extraction manifest.
- `tests/fixtures/generated/archives/tes4_v105.bsa` - Skyrim SE/AE v105 success archive.
- `tests/fixtures/generated/archives/tes4_v105_manifest.json` - v105 metadata and extraction manifest.

## Decisions Made

- Used a C++ generator instead of scripts so success fixtures reuse `libbsa::detail::hash_tes4` and `compress_payload` rather than duplicating codec/hash behavior.
- Stored expected bytes as hex plus FNV-1a checksums to support both exact byte assertions and compact diagnostics in later tests.
- Kept malformed generation as a callable mode in the same generator so Plan 03-03 can extend the fixture tooling without adding a second entry point.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Automated verification built and ran the generator target, checked all six success outputs and manifest coverage tokens, and confirmed `TES5Edit` remained unmodified.

## Known Stubs

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-03 can extend `--malformed` generation from the same tool to create malformed fixtures.
- Plans 03-04 through 03-06 can use the committed success manifests as byte-driven parser, metadata, lookup, and extraction contracts.

## Self-Check: PASSED

- Verified all created generator, archive, manifest, and summary files exist.
- Verified task commits `588c7db` and `c77939a` exist in git history.

---
*Phase: 03-format-detection-and-tes4-family-bsa-read-extract*
*Completed: 2026-05-08*
