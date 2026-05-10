---
phase: 08-ba2-gnrl-write-new-support
plan: 05
subsystem: ba2-gnrl-writer-compression
tags: [cpp20, ba2-gnrl, writer, deflate, lz4-block, catch2]

requires:
  - phase: 08-ba2-gnrl-write-new-support
    provides: raw BA2 GNRL writer serialization from Plan 08-04
provides:
  - BA2 GNRL writer compression policy resolution for target defaults and per-entry overrides
  - Deflate payload encoding for Fallout 4 v1, Starfield v2, and Starfield v3 method 0
  - Raw LZ4 block payload encoding for Starfield v3 method 3
  - Reader-verified compression metadata and byte-exact extraction tests
affects: [phase-08, ba2-gnrl-writer, compression-router, archive-reader-validation]

tech-stack:
  added: []
  patterns: [TDD red-green, reader-backed writer validation, metadata-routed BA2 compression]

key-files:
  created:
    - .planning/phases/08-ba2-gnrl-write-new-support/08-05-SUMMARY.md
  modified:
    - tests/unit/ba2_gnrl_writer_tests.cpp
    - src/formats/ba2/ba2_gnrl_writer.cpp

key-decisions:
  - "BA2 GNRL writer compression routes only from explicit target/options metadata: Fallout 4 v1, Starfield v2, and Starfield v3 method 0 use deflate; Starfield v3 method 3 uses raw LZ4 block."
  - "BA2 GNRL target_default now requests compression for non-empty entries, while all_raw and per-entry raw overrides keep PackedSize zero."
  - "Zero-byte BA2 GNRL entries remain raw even under compressed archive defaults so readers never enter decompression for empty payloads."

patterns-established:
  - "BA2 GNRL compressed writer payloads store router-compressed bytes directly, without a BSA-style raw-size prefix."
  - "Starfield v3 per-entry overrides select raw versus compressed only; compressed entries use the archive-wide CompressionMethod."

requirements-completed: [WBA2-02, WBA2-04, WBA2-05]

duration: 3min
completed: 2026-05-09
---

# Phase 08 Plan 05: BA2 GNRL Compression Policy Summary

**Metadata-routed BA2 GNRL writer compression with deflate/raw-LZ4 payloads, per-entry overrides, and reader-backed extraction proof**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T08:16:23Z
- **Completed:** 2026-05-09T08:19:20Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added RED tests proving `archive_compression_policy::all_compressed`, per-entry raw overrides, zero-byte raw preservation, and Starfield v3 method 0/method 3 routing through `archive_reader` metadata and extraction.
- Implemented BA2 GNRL compression policy resolution so target defaults and all-compressed policies encode non-empty payloads while all-raw and raw overrides serialize `PackedSize == 0`.
- Routed compressed BA2 GNRL payloads through `detail::compress_payload` using deflate for FO4/SFv2/SFv3 method 0 and raw LZ4 block for SFv3 method 3, with unsupported v3 methods still rejected.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED compression policy and override tests** - `b165871` (test)
2. **Task 2: GREEN BA2 compression routing implementation** - `5f612d6` (feat)

## Files Created/Modified

- `tests/unit/ba2_gnrl_writer_tests.cpp` - Adds reader-backed BA2 GNRL compression default, override, zero-byte, deflate, and raw-LZ4 assertions.
- `src/formats/ba2/ba2_gnrl_writer.cpp` - Adds policy resolution, target/method codec mapping, compression router calls, packed-size serialization, and raw zero-byte preservation.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-05-SUMMARY.md` - Documents plan execution, verification, deviations, and TDD gate compliance.

## Decisions Made

- Target/options metadata is the sole compression routing input; file extensions remain limited to BA2 record FourCC serialization and never choose codecs.
- BA2 GNRL compressed entries store only compressed bytes and the record raw-size field; no BSA-style raw-size prefix is emitted.
- Starfield v3 `CompressionMethod` remains archive-wide, so per-entry overrides select raw versus compressed while method 0/method 3 choose deflate versus raw LZ4 block for all compressed entries.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected RED test path extension to stay within BA2 GNRL record constraints**
- **Found during:** Task 2 (GREEN compression implementation)
- **Issue:** The new no-extension-inference test used `.payload`, which violates the writer's BA2 GNRL four-byte extension validation before compression routing could be exercised.
- **Fix:** Changed the synthetic path to `.bin`, preserving the no-codec-inference intent while satisfying the format's four-byte extension field.
- **Files modified:** `tests/unit/ba2_gnrl_writer_tests.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure`
- **Committed in:** `5f612d6`

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** The fix corrected test input validity without changing production scope or weakening compression-routing coverage.

## Issues Encountered

- The RED tests failed as expected before implementation because compressed entries were still rejected by the writer.
- After GREEN implementation, the `.payload` test path exposed the pre-existing four-byte BA2 extension validation and was corrected as a test-data issue.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None - compression routing, unsupported method rejection, and exact-size extraction verification were covered by the plan threat model.

## TDD Gate Compliance

- RED gate: `b165871` added failing BA2 GNRL compression routing and override tests.
- GREEN gate: `5f612d6` implemented compression behavior and made the tests pass.
- REFACTOR gate: Not needed; no separate cleanup changes were made after GREEN.

## Verification

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — passed.
- `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` — passed (11/11 tests).
- `ctest --preset windows-msvc-debug-static -R "compression_router|deflate_codec|lz4_codec|ba2_gnrl_reader" --output-on-failure` — passed (8/8 tests).
- `git -C TES5Edit status --short` — clean.

## Self-Check: PASSED

- Found `tests/unit/ba2_gnrl_writer_tests.cpp`.
- Found `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Found `.planning/phases/08-ba2-gnrl-write-new-support/08-05-SUMMARY.md`.
- Found task commit `b165871`.
- Found task commit `5f612d6`.

## Next Phase Readiness

Ready for Plan 08-06. BA2 GNRL compressed stored-payload semantics now round-trip through public reader APIs, so opt-in final-stored-byte deduplication can build on the final raw/compressed byte shape.

---
*Phase: 08-ba2-gnrl-write-new-support*
*Completed: 2026-05-09*
