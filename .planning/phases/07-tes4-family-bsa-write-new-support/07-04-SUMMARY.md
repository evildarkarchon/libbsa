---
phase: 07-tes4-family-bsa-write-new-support
plan: 04
subsystem: bsa-writer-compression
tags: [cpp20, tes4-bsa, writer, deflate, lz4-frame, catch2]

requires:
  - phase: 07-tes4-family-bsa-write-new-support
    provides: raw TES4-family BSA writer serialization from Plan 07-03
provides:
  - TES4-family writer compression policy resolution for archive defaults and per-entry overrides
  - Deflate payload encoding for v103/v104 and LZ4-frame payload encoding for v105
  - Reader-verified compression metadata and extraction tests for default, override, and zero-byte cases
affects: [phase-07, tes4-family-bsa-writer, archive-reader-validation]

tech-stack:
  added: []
  patterns: [TDD red-green, reader-backed writer validation, target-routed compression]

key-files:
  created: []
  modified:
    - tests/unit/tes4_bsa_writer_tests.cpp
    - src/formats/bsa/tes4_bsa_writer.cpp

key-decisions:
  - "TES4-family writer compression routes only from explicit target profile and public policies: v103/v104 use deflate, v105 uses LZ4 frame."
  - "Zero-byte entries are forced raw and receive the compression XOR toggle when needed so readers never expect a compressed size prefix."
  - "Stored payload sizes are capped below the compression-toggle bit to avoid ambiguous size_flags serialization."

patterns-established:
  - "Compressed writer payloads are encoded as uint32 little-endian raw size followed by router-compressed bytes."
  - "Per-file size_flags toggle remains an XOR deviation marker against the archive compression default."

requirements-completed: [WBSA-01, WBSA-02, WBSA-03, WBSA-07, WBSA-10]

duration: 3min
completed: 2026-05-09
---

# Phase 07 Plan 04: TES4-Family Compression Policy Summary

**Target-routed TES4-family BSA compression with deflate/LZ4-frame payloads, XOR override bits, and zero-byte raw preservation**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T06:27:11Z
- **Completed:** 2026-05-09T06:30:16Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added RED tests proving all-compressed archives, raw overrides, zero-byte inherited entries, and all-raw archives with compressed overrides through `archive_reader` metadata and extraction.
- Implemented compression policy resolution and routed compressed non-empty entries through `detail::compress_payload` using deflate for v103/v104 and LZ4 frame for v105.
- Serialized compressed payloads with a 4-byte little-endian raw-size prefix and set archive/default plus per-file XOR compression bits so reader-visible metadata matches policy.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED compression policy tests** - `fb46c85` (test)
2. **Task 2: GREEN compression routing implementation** - `11cc408` (feat)

## Files Created/Modified

- `tests/unit/tes4_bsa_writer_tests.cpp` - Adds reader-backed compression default, override, zero-byte, deflate, and LZ4-frame assertions.
- `src/formats/bsa/tes4_bsa_writer.cpp` - Adds policy resolution, compression router calls, raw-size prefix encoding, size-flag bounds, and XOR toggle handling.

## Decisions Made

- Target profile alone selects the compression codec for compressed entries; no public codec or compression-level knobs were added.
- Zero-byte entries remain raw even under compressed archive defaults, with the per-file toggle set when necessary for reader compatibility.
- Stored payload sizes now fail before setting the reserved compression-toggle bit, preventing ambiguous record serialization.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Guarded stored payload sizes against the compression toggle bit**
- **Found during:** Task 2 (GREEN compression implementation)
- **Issue:** Compression added meaningful `size_flags` high-bit semantics, but the existing size check only enforced `uint32_t`, allowing oversized stored payloads to collide with `0x40000000U`.
- **Fix:** Added a dedicated stored-payload size check capped below the toggle bit.
- **Files modified:** `src/formats/bsa/tes4_bsa_writer.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure`
- **Committed in:** `11cc408`

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** The fix is a serialization correctness guard required by the compression toggle format; no scope creep.

## Issues Encountered

- The RED test initially did not run until the test binary was rebuilt; after `cmake --build --preset windows-msvc-debug-static`, the new tests failed for the expected missing compression behavior.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None - compression routing and raw-size prefix handling were already covered by the plan threat model.

## TDD Gate Compliance

- RED gate: `fb46c85` added failing compression policy tests.
- GREEN gate: `11cc408` implemented compression behavior and made the tests pass.
- REFACTOR gate: Not needed; no separate cleanup changes were made after GREEN.

## Verification

- `cmake --build --preset windows-msvc-debug-static` — passed.
- `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` — passed (9/9 tests).
- `ctest --preset windows-msvc-debug-static -R "compression_router|deflate_codec|lz4_codec|tes4_bsa_reader" --output-on-failure` — passed (8/8 tests).
- `git -C TES5Edit status --short` — clean.

## Self-Check: PASSED

- Found `tests/unit/tes4_bsa_writer_tests.cpp`.
- Found `src/formats/bsa/tes4_bsa_writer.cpp`.
- Found `.planning/phases/07-tes4-family-bsa-write-new-support/07-04-SUMMARY.md`.
- Found task commit `fb46c85`.
- Found task commit `11cc408`.

## Next Phase Readiness

Ready for Plan 07-05. Compression defaults and overrides now round-trip through the public reader, so embedded-name and dedupe writer behavior can build on final stored payload semantics.

---
*Phase: 07-tes4-family-bsa-write-new-support*
*Completed: 2026-05-09*
