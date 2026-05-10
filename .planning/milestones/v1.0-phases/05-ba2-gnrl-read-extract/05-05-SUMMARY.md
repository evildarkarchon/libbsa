---
phase: 05-ba2-gnrl-read-extract
plan: 05
subsystem: archive-reader
tags: [cpp20, ba2, gnrl, malformed, regression, public-api, tdd]

requires:
  - phase: 05-ba2-gnrl-read-extract
    provides: BA2 GNRL fixtures, detector, parser, lookup, and extraction from Plans 05-01 through 05-04
provides:
  - Manifest-driven malformed and unsupported BA2 GNRL error-code coverage
  - Explicit BA2 compressed-payload decode failure documentation in the extraction path
  - Final focused, full-suite, public-boundary, cross-format, and TES5Edit read-only verification for Phase 5
affects: [phase-06-ba2-dx10, phase-08-ba2-writer, phase-11-hardening, validation]

tech-stack:
  added: []
  patterns: [manifest-driven malformed tests, stable error-code assertions, verification-only closeout gate]

key-files:
  created:
    - .planning/phases/05-ba2-gnrl-read-extract/05-05-SUMMARY.md
  modified:
    - tests/unit/ba2_gnrl_reader_tests.cpp
    - src/formats/ba2/ba2_gnrl_reader.cpp

key-decisions:
  - "Malformed BA2 tests assert only stable error_code values from the manifest, never diagnostic message text."
  - "Task 2 found the required parser/detector/extractor hardening already present from prior Phase 5 work; the task only documented the existing compressed-payload fail-closed behavior."

patterns-established:
  - "Malformed BA2 fixture manifests drive both open-time unsupported/format_error checks and extraction-time codec failure checks."
  - "Final phase closeout can be verification-only when no direct regression fix is required; no empty task commit is created."

requirements-completed: [GNRL-01, GNRL-02, GNRL-03, GNRL-04, GNRL-05, GNRL-06, GNRL-07, GNRL-08]

duration: 7min
completed: 2026-05-08
---

# Phase 05 Plan 05: Malformed BA2 Validation and Regression Summary

**Manifest-driven malformed BA2 fail-closed coverage with full Phase 5 BA2, BSA regression, public-boundary, and TES5Edit cleanliness gates.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-05-08T23:06:01Z
- **Completed:** 2026-05-08T23:13:00Z
- **Tasks:** 3 completed
- **Files modified:** 3

## Accomplishments

- Added `[ba2_gnrl_malformed]` tests that iterate `ba2_gnrl_malformed_manifest.json` and validate stable `format_error` / `unsupported` codes for open-time and extraction-time failures.
- Verified required malformed IDs are present, including DX10 unsupported, unsupported Starfield v3 compression method, duplicate canonical paths, corrupt compressed payloads, and exact-size mismatch.
- Clarified BA2 compressed extraction keeps codec decode failures as `format_error` and never attempts partial extraction.
- Completed final focused BA2/TES3/TES4/public-boundary regression gates, full CTest, and TES5Edit read-only status verification.

## Task Commits

Each behavior-changing task was committed atomically:

1. **Task 1: Add malformed and unsupported BA2 tests** - `f9de84e` (test)
2. **Task 2: Harden BA2 detector/parser/extractor for malformed cases** - `c8398cf` (refactor/documentation of existing hardening)
3. **Task 3: Run final Phase 5 regression and boundary gates** - verification-only; no code changes and no empty commit created

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/ba2_gnrl_reader_tests.cpp` - Adds manifest-driven `[ba2_gnrl_malformed]` coverage for supported malformed bytes and unsupported BA2 profiles.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - Documents why corrupt compressed BA2 payload decode failures remain stable `format_error` results without partial extraction.
- `.planning/phases/05-ba2-gnrl-read-extract/05-05-SUMMARY.md` - Records execution results, verification, deviations, and self-check.

## Decisions Made

- Malformed BA2 assertions stay manifest-driven and compare only `error().code`, preserving D-11 by avoiding diagnostic message text checks.
- No additional parser or detector behavior was needed because the strict validation required by Task 2 was already present; the task was limited to documenting the existing fail-closed extraction behavior.
- Task 3 remained verification-only because all final regression gates passed and no direct Phase 5 regression was found.

## Deviations from Plan

### Auto-fixed Issues

None - no Rule 1-3 auto-fixes were required.

---

**Total deviations:** 0 auto-fixed.
**Impact on plan:** The plan's required behavior was already satisfied once the malformed tests were added, so scope stayed within Phase 5 closeout.

## Issues Encountered

- The Task 1 RED run passed unexpectedly: the BA2 detector/parser/extractor hardening required by Task 2 had already been implemented by prior Phase 5 parser and extraction work. This was investigated by running the focused malformed label and then the Task 2 acceptance/verification gates; no production hardening gaps remained.

## TDD Gate Compliance

- RED gate: `f9de84e` added the malformed tests, but the RED command passed unexpectedly because the required behavior already existed.
- GREEN gate: no separate `feat(05-05)` commit was needed; Task 2 produced `c8398cf` to document the existing fail-closed decode behavior and focused BA2 verification passed.
- REFACTOR gate: `c8398cf` is a refactor/documentation commit with no behavior change.

## Known Stubs

None. The modified files contain no placeholder/TODO/FIXME stubs or hardcoded empty UI data sources.

## Threat Flags

None. New surface is limited to tests over the plan's malformed BA2 trust boundary and a clarifying comment in the existing extraction path.

## Verification

- `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L ba2_gnrl_malformed --output-on-failure` — passed; this was the unexpected RED result.
- `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "ba2_gnrl_malformed|ba2_gnrl_extract|ba2_gnrl_metadata|ba2_gnrl_lookup" --output-on-failure` — passed.
- Acceptance greps for `[ba2_gnrl_malformed]`, `ba2_dx10_unsupported`, `ba2_unsupported_v3_compression_method`, `ba2_exact_size_mismatch`, and `expected_error` — passed.
- Acceptance grep for `.message` assertions in `tests/unit/ba2_gnrl_reader_tests.cpp` — no matches.
- Acceptance greps for DX10 / `CompressionMethod`, duplicate canonical path / payload and filename table spans, and compressed decode failure handling — passed.
- `ctest --preset windows-msvc-debug-static -L "ba2_gnrl|tes3_bsa|tes4_bsa|public_include_boundary" --output-on-failure` — passed, 39/39 tests.
- `ctest --preset windows-msvc-debug-static --output-on-failure` — passed, 75/75 tests with the expected local-game-fixture skip.
- `git -C TES5Edit status --short` — no output.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 5 BA2 GNRL read/extract support is closed and ready for Phase 6 BA2 DX10/DDS planning.
- BA2 DDS texture record parsing, DDS reconstruction, cubemaps, mip ordering, and DirectXTex validation remain intentionally deferred to Phase 6.
- Broad malformed-input hardening and fuzzing remain deferred to Phase 11.

## Self-Check: PASSED

- Found key modified files on disk: `tests/unit/ba2_gnrl_reader_tests.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, and this summary.
- Found task commits `f9de84e` and `c8398cf` in git history.
- Final verification commands passed and TES5Edit status was clean.

---
*Phase: 05-ba2-gnrl-read-extract*
*Completed: 2026-05-08*
