---
phase: 07-tes4-family-bsa-write-new-support
plan: 05
subsystem: bsa-writer-embedded-names
tags: [cpp20, tes4-bsa, writer, embedded-names, catch2]

requires:
  - phase: 07-tes4-family-bsa-write-new-support
    provides: target-routed stored payload encoding from Plan 07-04
provides:
  - Explicit opt-in TES4-family embedded-name prefix emission for v104/v105 writer output
  - Reader-backed proof that embedded-name prefixes remain absent by default and for v103 compatibility
  - Extraction proof that embedded-name prefixes do not leak into consumer-visible bytes
affects: [phase-07, tes4-family-bsa-writer, archive-reader-validation]

tech-stack:
  added: []
  patterns: [TDD red-green, reader-backed writer validation, target-compatible embedded-name option]

key-files:
  created: []
  modified:
    - tests/unit/tes4_bsa_writer_tests.cpp
    - src/formats/bsa/tes4_bsa_writer.cpp

key-decisions:
  - "Embedded names remain off by default and are emitted only through the global `embed_file_names` writer option."
  - "v103 output ignores the embedded-name option because the existing parser treats embedded names as active only for non-v103 archives."
  - "Embedded-name prefixes use the preserved file-name spelling serialized in the file-name table and are encoded before compression/raw payload bytes."

patterns-established:
  - "Stored payload encoding now supports an optional one-byte length plus preserved file-name prefix before raw or compressed content."
  - "Embedded-name acceptance is proven through reopened `archive_reader` metadata and `extract_bytes` byte comparison."

requirements-completed: [WBSA-02, WBSA-03, WBSA-08, WBSA-10]

duration: 5min
completed: 2026-05-09
---

# Phase 07 Plan 05: TES4-Family Embedded Name Writing Summary

**Opt-in v104/v105 embedded-name prefixes with reader-visible metadata and prefix-free extracted bytes**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-09T06:31:00Z
- **Completed:** 2026-05-09T06:35:38Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added RED tests for disabled embedded-name behavior, v104/v105 opt-in prefix metadata, extraction byte preservation, and v103 compatibility.
- Implemented target-compatible embedded-name prefix encoding in the TES4-family writer while keeping the public surface as one safe global option.
- Verified writer output by reopening archives through `archive_reader`, checking `has_embedded_name`/`embedded_name_prefix_size`, and comparing extracted bytes to source payloads.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED embedded-name enabled/disabled tests** - `7ab3a60` (test)
2. **Task 2: GREEN embedded-name prefix emission** - `799cbd5` (feat)

## Files Created/Modified

- `tests/unit/tes4_bsa_writer_tests.cpp` - Adds reader-backed embedded-name disabled, enabled, and v103 compatibility assertions; fixes the writer-test metadata helper to return by value.
- `src/formats/bsa/tes4_bsa_writer.cpp` - Adds optional embedded-name prefix encoding and gates archive flag/prefix emission to non-v103 targets.

## Decisions Made

- Embedded names are a global writer option, not a per-entry setting or raw archive-flag escape hatch.
- Prefixes use only the preserved file-name portion (`Model.nif`, `Quest.pex`), not the folder path, matching the serialized file-name table spelling.
- v103 archives remain prefix-free even when `embed_file_names` is true, preserving target compatibility with the existing reader semantics.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed dangling metadata reference in writer test helper**
- **Found during:** Task 2 (GREEN embedded-name implementation)
- **Issue:** `require_entry` returned a reference to an `entry_metadata` inside a temporary `std::optional`, producing unstable metadata assertions once embedded-name checks used the returned object after the helper returned.
- **Fix:** Changed `require_entry` to return `entry_metadata` by value so tests inspect stable metadata copies from the public reader API.
- **Files modified:** `tests/unit/tes4_bsa_writer_tests.cpp`
- **Verification:** `cmake --build --preset windows-msvc-debug-static` and `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure`
- **Committed in:** `799cbd5`

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** The fix was required for reliable reader-backed acceptance assertions and did not expand feature scope.

## Issues Encountered

- The RED test command initially reported the previously built 9-test suite as passing until the test binary was rebuilt; after `cmake --build --preset windows-msvc-debug-static`, the new opt-in embedded-name test failed as expected before implementation.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None - the new writer option and stored payload prefix surface were covered by the plan threat model.

## TDD Gate Compliance

- RED gate: `7ab3a60` added failing embedded-name writer tests.
- GREEN gate: `799cbd5` implemented target-compatible embedded-name prefix emission and made the tests pass.
- REFACTOR gate: Not needed; no separate cleanup changes were made after GREEN.

## Verification

- `cmake --build --preset windows-msvc-debug-static` — passed.
- `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` — passed (12/12 tests).
- `ctest --preset windows-msvc-debug-static -R tes4_bsa_reader --output-on-failure` — completed with no matching tests in the current CTest registry.
- `git -C TES5Edit status --short` — clean.

## Self-Check: PASSED

- Found `tests/unit/tes4_bsa_writer_tests.cpp`.
- Found `src/formats/bsa/tes4_bsa_writer.cpp`.
- Found `.planning/phases/07-tes4-family-bsa-write-new-support/07-05-SUMMARY.md`.
- Found task commit `7ab3a60`.
- Found task commit `799cbd5`.

## Next Phase Readiness

Ready for Plan 07-06. Embedded-name storage is now part of final stored payload encoding, so optional deduplication can compare fully encoded stored bytes as required by D-19.

---
*Phase: 07-tes4-family-bsa-write-new-support*
*Completed: 2026-05-09*
