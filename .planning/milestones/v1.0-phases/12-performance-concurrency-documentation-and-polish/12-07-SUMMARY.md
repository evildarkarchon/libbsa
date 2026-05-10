---
phase: 12-performance-concurrency-documentation-and-polish
plan: 07
subsystem: documentation
tags: [cpp20, integration-examples, package-consumer, target-formats, policy-tests]

requires:
  - phase: 12-performance-concurrency-documentation-and-polish
    provides: bulk extraction API, writer execution options, benchmark policy, and Doxygen/thread-safety docs from Plans 12-01, 12-02, 12-05, and 12-06
provides:
  - Compile-checked package-consumer examples for reader, bulk extraction, writer, result, and validation workflows
  - Consumer integration guide mirroring installed-package example function names
  - Target-format guide for supported BSA/BA2 variants, compression routes, writer policies, and compatibility warnings
  - Source-text policy tests that enforce example, format, compression, warning-code, and fixture-boundary coverage
affects: [phase-12, docs, package-consumer, validation-policy, fixture-policy]

tech-stack:
  added: []
  patterns: [installed-package compile-checked examples, target-format source-text policy tests, public-header warning-code parsing]

key-files:
  created:
    - docs/integration-examples.md
    - docs/target-format-guide.md
    - tests/unit/target_format_policy_tests.cpp
    - .planning/phases/12-performance-concurrency-documentation-and-polish/12-07-SUMMARY.md
  modified:
    - tests/package-consumer/main.cpp
    - tests/CMakeLists.txt
    - tests/fixtures/README.md

key-decisions:
  - "Consumer examples are compile-checked through the installed package smoke source and mirrored by docs headings."
  - "Target-format guidance is machine-checked against required headings, compression route text, and public compatibility_warning_code values parsed from validation.hpp."
  - "Phase 12 benchmark and documentation evidence remains legal synthetic data only and keeps TES5Edit read-only."

patterns-established:
  - "Documentation examples should be represented by package-consumer functions whose names are checked against guide headings."
  - "Target-format docs should use stable headings and public enum parsing so warning-code additions force documentation updates."

requirements-completed: [DOC-02, DOC-03]

duration: 4min
completed: 2026-05-10
---

# Phase 12 Plan 07: Integration Examples and Target-Format Guidance Summary

**Compile-checked consumer examples and machine-checked target-format guidance for supported BSA/BA2 variants and compatibility warnings.**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-10T08:53:51Z
- **Completed:** 2026-05-10T08:57:44Z
- **Tasks:** 3
- **Files modified:** 6 implementation/doc/test files plus this summary

## Accomplishments

- Expanded `tests/package-consumer/main.cpp` with installed-package example functions for opening/listing/extracting, bulk extraction, TES3/TES4/BA2 writer creation, result error handling, and validation.
- Added `docs/integration-examples.md` with headings matching every compile-checked `example_*` function and guidance to keep host output roots separate from archive virtual paths.
- Added `docs/target-format-guide.md` covering every required BSA/BA2 variant, deflate/LZ4 frame/raw LZ4 block routes, writer target policies, DX10 no-transform behavior, Starfield `CompressionMethod == 3`, and public compatibility warnings.
- Added source-text policy tests that parse `compatibility_warning_code` from `include/libbsa/validation.hpp` and enforce guide/example drift checks.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add compile-checked installed consumer examples** - `f80772f` (docs)
2. **Task 2: Add target-format and compatibility guidance** - `b25a1bc` (docs)
3. **Task 3: Add target-format and examples policy tests** - `0401dae` (test)

**Plan metadata:** pending final docs commit.

## Files Created/Modified

- `docs/integration-examples.md` - Consumer guide with headings and snippets matching compile-checked package-consumer functions.
- `docs/target-format-guide.md` - Supported target-format, compression-route, writer-policy, warning-code, and TES5Edit boundary guide.
- `tests/package-consumer/main.cpp` - Installed-package smoke source that compiles representative public API examples while keeping the original missing-path smoke behavior.
- `tests/unit/target_format_policy_tests.cpp` - Policy tests for example/docs parity, target-format headings, compression routes, public warning code coverage, and legal evidence boundaries.
- `tests/CMakeLists.txt` - Registers the target-format policy tests.
- `tests/fixtures/README.md` - Adds Phase 12 benchmark/generated-data policy pointing to `benchmarks/README.md`.

## Decisions Made

- Compile-checked examples live in the package-consumer smoke source so installed package consumers exercise the same public umbrella include and `libbsa::libbsa` target.
- The target-format guide references `docs/compatibility-evidence.md` and uses warning-code headings so the new policy test can enforce public validation-code coverage.
- Fixture policy treats Phase 12 benchmark inputs as legal synthetic data only; no game archives, BSArchPro exports, or `TES5Edit/` fixture workspace use is allowed.

## Verification

- `rg -n "example_open_list_extract|example_bulk_extract|example_create_tes3_bsa|example_create_tes4_bsa|example_create_ba2_gnrl|example_create_ba2_dx10|example_handle_result_errors|example_validate_archive" tests\package-consumer\main.cpp docs\integration-examples.md` - passed.
- `rg -n "extract_entries|write_execution_options|validate_archive|error\(\)\.code" tests\package-consumer\main.cpp docs\integration-examples.md` - passed.
- `rg -n "TES3 BSA|TES4-family BSA v103|TES4-family BSA v104|Skyrim SE/AE BSA v105|Fallout 4 BA2 GNRL|Fallout 4 BA2 DX10|Starfield BA2 v2 GNRL|Starfield BA2 v3 GNRL|Starfield BA2 v3 DX10|deflate|LZ4 frame|raw LZ4 block|compressed_sound_payload|bsa_embedded_name_compatibility_risk|target_family_mismatch" docs\target-format-guide.md` - passed.
- `rg -n "benchmarks/README.md|legal synthetic" tests\fixtures\README.md` - passed.
- `rg -n "TES5Edit.*fixture workspace|read-only" docs\target-format-guide.md tests\fixtures\README.md` - passed.
- `rg -n "target_format_policy|compatibility_warning_code|example_bulk_extract|docs/compatibility-evidence.md" tests\unit\target_format_policy_tests.cpp` - passed.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- `ctest --preset windows-msvc-debug-static -R "package_consumer_smoke" --output-on-failure` - passed during Task 1.
- `ctest --preset windows-msvc-debug-static -R "target_format_policy|package_consumer_smoke|validation_policy" --output-on-failure` - passed, 5/5 tests.
- `git status --short TES5Edit` - no output.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Protected bulk example factory bookkeeping**
- **Found during:** Task 1 (Add compile-checked installed consumer examples)
- **Issue:** The initial package-consumer bulk extraction example used `worker_count = 2` while the example sink factory recorded bookkeeping vectors without synchronization.
- **Fix:** Added a `std::mutex` and short comment explaining that `bulk_extract_sink_factory::create` may be called concurrently.
- **Files modified:** `tests/package-consumer/main.cpp`
- **Verification:** Package-consumer smoke and target-format policy tests passed.
- **Committed in:** `f80772f`

**2. [Rule 1 - Bug] Fixed package-consumer sink factory return conversion**
- **Found during:** Task 1 verification
- **Issue:** The first `package_consumer_smoke` run failed because returning `std::unique_ptr<byte_vector_sink>` did not implicitly construct `result<std::unique_ptr<payload_sink>>` under MSVC.
- **Fix:** Converted the sink to `std::unique_ptr<libbsa::payload_sink>` before returning it from the example factory.
- **Files modified:** `tests/package-consumer/main.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -R "package_consumer_smoke" --output-on-failure` passed after the fix.
- **Committed in:** `f80772f`

---

**Total deviations:** 2 auto-fixed (1 missing critical functionality, 1 bug).
**Impact on plan:** Both fixes kept the examples compile-checked and concurrency-safe without changing the planned public API or documentation scope.

## Issues Encountered

- The first package-consumer smoke run failed on the sink factory return conversion described above; it was fixed and the focused smoke test passed.

## Known Stubs

None. Stub scan found no TODO/FIXME/placeholder/coming-soon/not-available text or hardcoded empty UI-style values in the files created or modified by this plan.

## Threat Flags

None - this plan added documentation, an installed-package example source, and source-text policy tests only. No new network endpoints, auth paths, file-access trust boundaries beyond examples, or schema changes were introduced.

## Auth Gates

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 12 documentation requirements DOC-02 and DOC-03 are complete. All seven Phase 12 plans now have summaries, so the phase is ready for verification or milestone closeout.

## Self-Check: PASSED

- Verified key created files exist: `docs/integration-examples.md`, `docs/target-format-guide.md`, `tests/unit/target_format_policy_tests.cpp`, and this summary.
- Verified task commits `f80772f`, `b25a1bc`, and `0401dae` are reachable.
- Verified `git status --short TES5Edit` returned no output during plan-level verification.

---
*Phase: 12-performance-concurrency-documentation-and-polish*
*Completed: 2026-05-10*
