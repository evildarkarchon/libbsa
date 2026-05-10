---
phase: 11
slug: compatibility-warnings-validation-api-and-hardening
status: ready
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-10
---

# Phase 11 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.14.0 plus CTest |
| **Config file** | `tests/CMakeLists.txt`, `CMakePresets.json` |
| **Quick run command** | `cmake --preset windows-msvc-debug-static && cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "validation|compatibility|malformed|public_include_boundary|package_consumer" --output-on-failure` |
| **Full suite command** | `cmake --preset windows-msvc-debug-static && cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure`; repeat with `windows-msvc-debug-shared` before phase closeout |
| **Estimated runtime** | Quick subset: project-dependent; full static/shared suite: project-dependent |

---

## Sampling Rate

- **After every task commit:** Run the targeted validation, warning, malformed, public include boundary, or package-consumer CTest subset for the touched behavior.
- **After every plan wave:** Run the full `windows-msvc-debug-static` suite.
- **Before `$gsd-verify-work`:** Run full `windows-msvc-debug-static` and `windows-msvc-debug-shared` suites, plus the sanitizer-oriented malformed/validation path when the local compiler supports it.
- **Max feedback latency:** No three consecutive implementation tasks may lack an automated verification command.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 11-01-T1 | 11-01 | 1 | COMP-03 | T-11-01 / T-11-03 | Public validation/warning symbols are asserted before implementation and public headers keep forbidden dependency tokens out. | public-api | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | Existing boundary file extended by task | planned |
| 11-01-T2 | 11-01 | 1 | COMP-03 | T-11-01 / T-11-03 | Public validation contract compiles through dependency-light installed headers. | public-api | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | Planned `include/libbsa/validation.hpp` | planned |
| 11-05-T1 | 11-05 | 1 | COMP-04 | T-11-13 / T-11-14 | Malformed manifest typo paths fail loudly instead of silently remapping public error codes. | malformed/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "tes3_bsa_malformed|tes4_bsa_malformed|ba2_gnrl_malformed|ba2_dx10_malformed" --output-on-failure` | Existing reader tests modified by task | planned |
| 11-05-T2 | 11-05 | 1 | COMP-04 | T-11-14 / T-11-15 | Existing malformed suites still pass after strict manifest oracle hardening. | malformed/unit | `ctest --preset windows-msvc-debug-static -L malformed --output-on-failure` | Existing malformed tests | planned |
| 11-02-T1 | 11-02 | 2 | COMP-02 / COMP-04 | T-11-04 / T-11-05 | Generated fixtures, writer outputs, and malformed cases define validation facade behavior before implementation. | validation/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | Planned `tests/unit/validation_api_tests.cpp` | planned |
| 11-02-T2 | 11-02 | 2 | COMP-02 / COMP-04 | T-11-04 / T-11-06 | `validate_archive` reports fatal diagnostics for inspectable malformed archives without weakening strict open. | validation/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "validation_api|package_consumer|public_include_boundary" --output-on-failure` | Planned `src/validation.cpp` | planned |
| 11-03-T1 | 11-03 | 3 | COMP-02 / COMP-03 | T-11-07 / T-11-08 | Warning scenarios assert stable code/severity/path facts without exact message coupling. | compat/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | Planned `tests/unit/compatibility_warning_tests.cpp` | planned |
| 11-03-T2 | 11-03 | 3 | COMP-02 / COMP-03 | T-11-07 / T-11-09 | BSA and BA2 warning policy emits stable records from generated or writer-output evidence. | compat/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "compatibility_warning|validation_api" --output-on-failure` | Planned private warning policy in `src/validation.cpp` | planned |
| 11-04-T1 | 11-04 | 4 | COMP-01 / COMP-03 / COMP-06 | T-11-10 | Missing or incomplete warning catalog coverage fails machine checks. | docs/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "compatibility evidence|validation_policy" --output-on-failure` | Existing policy test file extended by task | planned |
| 11-04-T2 | 11-04 | 4 | COMP-01 / COMP-03 / COMP-06 | T-11-10 / T-11-11 / T-11-12 | Every public warning code is cataloged after warning tests exist, and optional corpus checks remain skipped by default. | docs/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "compatibility evidence|validation_policy|requires-game-fixture" --output-on-failure` | Planned `docs/compatibility-evidence.md` | planned |
| 11-06-T1 | 11-06 | 4 | COMP-04 / COMP-05 | T-11-16 / T-11-17 | Matrix tests require each row to have manifest-backed or explicit test-backed evidence before matrix content exists. | matrix/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | Planned `tests/unit/compatibility_matrix_tests.cpp` | planned |
| 11-06-T2 | 11-06 | 4 | COMP-04 / COMP-05 | T-11-16 / T-11-17 / T-11-18 | Consolidated malformed matrix covers families/categories, validates manifest rows and test-backed oversized arithmetic evidence, and drives validation-report checks. | matrix/malformed | `python tests/fixtures/generated/validate_fixture_manifests.py && cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "compatibility_matrix|validation_api" --output-on-failure` | Planned `tests/fixtures/generated/compatibility_matrix.json` | planned |
| 11-07-T1 | 11-07 | 5 | COMP-05 / COMP-06 | T-11-19 / T-11-20 / T-11-21 | Policy tests prove the sanitizer path is additive and Windows static/shared CI plus TES5Edit checks remain intact. | policy/unit | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "sanitizer validation path|validation_policy" --output-on-failure` | Existing policy test file extended by task | planned |
| 11-07-T2 | 11-07 | 5 | COMP-05 / COMP-06 | T-11-19 / T-11-20 / T-11-21 | Additive sanitizer preset/docs exist without changing default Windows CI. | build/policy | `cmake --list-presets=all && cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "sanitizer validation path|validation_policy|public_include_boundary" --output-on-failure` | Planned `linux-clang-asan-ubsan` preset/docs | planned |

---

## Wave 0 Requirements

- [x] `include/libbsa/validation.hpp` - planned in 11-01-T2 with public validation API types and Doxygen comments.
- [x] `src/validation.cpp` - planned in 11-02-T2 with strict-open-backed validation report assembly, then extended in 11-03-T2 for private warning policy.
- [x] `tests/unit/validation_api_tests.cpp` - planned in 11-02-T1/T2 and extended in 11-06-T1/T2 for writer-output and malformed matrix coverage.
- [x] `tests/unit/compatibility_warning_tests.cpp` - planned in 11-03-T1/T2 with stable warning-code/severity/path coverage and no exact message assertions.
- [x] `docs/compatibility-evidence.md` plus CTest-discovered checks - planned in 11-04-T1/T2 after warning tests exist.
- [x] `tests/fixtures/generated/compatibility_matrix.json` - planned in 11-06-T1/T2 as a consolidated matrix with manifest-backed and explicit test-backed evidence rows.
- [x] Per-family malformed manifest helpers reject unknown `expected_error` values instead of silently defaulting - planned in 11-05-T1/T2.
- [x] Additive sanitizer preset and documented command path for malformed/parser/compression/validation labels - planned in 11-07-T1/T2.
- [x] Reconfigure local build tree before validation if CTest still references stale paths from another checkout - retained in quick/full validation commands through configure/build presets.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Optional local BSArchPro/game corpus comparison | COMP-01, COMP-02, COMP-06 | Copyrighted game data and TES5Edit output cannot become required committed fixtures. | Keep any local corpus ignored and skipped by default; verify required CI/test paths pass without those files. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify commands.
- [x] Sampling continuity: no 3 consecutive tasks lack automated verification.
- [x] Wave 0 planning covers all missing references above through concrete plan/task mappings.
- [x] No watch-mode flags.
- [x] Full static/shared Windows suites remain required before phase closeout.
- [x] Sanitizer-oriented malformed/validation path is documented and preset-backed in 11-07.
- [x] `nyquist_compliant: true` is set in frontmatter after every task was mapped to automated coverage.

**Approval:** ready for Phase 11 execution
