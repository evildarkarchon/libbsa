---
phase: 11
slug: compatibility-warnings-validation-api-and-hardening
status: draft
nyquist_compliant: false
wave_0_complete: false
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
| 11-W0-COMP-01 | TBD | TBD | COMP-01 | T-11-01 / T-11-06 | Fixture/evidence comparisons stay legal and do not require mutable TES5Edit or copyrighted game archives. | fixture/compat | `ctest --preset windows-msvc-debug-static -L "fixture|compat" --output-on-failure` | Partial; fixture tests exist, Phase 11 compatibility catalog is missing | pending |
| 11-W0-COMP-02 | TBD | TBD | COMP-02 | T-11-02 / T-11-03 | Writer-produced archives validate through the public facade and strict open remains authoritative. | unit/roundtrip/compat | `ctest --preset windows-msvc-debug-static -R "validation|writer" --output-on-failure` | Missing `tests/unit/validation_api_tests.cpp` | pending |
| 11-W0-COMP-03 | TBD | TBD | COMP-03 | T-11-05 | Warnings expose stable code/severity/path records without logger ownership or exact message assertions. | unit/compat | `ctest --preset windows-msvc-debug-static -R "compatibility_warning" --output-on-failure` | Missing warning tests | pending |
| 11-W0-COMP-04 | TBD | TBD | COMP-04 | T-11-01 / T-11-02 / T-11-03 / T-11-04 | Malformed archives produce fatal validation diagnostics and never return a usable reader. | malformed/unit | `ctest --preset windows-msvc-debug-static -L malformed --output-on-failure` | Partial; per-family malformed tests exist, consolidated matrix is missing | pending |
| 11-W0-COMP-05 | TBD | TBD | COMP-05 | T-11-01 / T-11-02 / T-11-03 | Parser/decompressor/validation hardening has an additive Clang/GCC sanitizer path. | build/test | `ctest --preset <sanitize-preset> -L "malformed|validation|compression" --output-on-failure` | Missing sanitizer preset or documented command path | pending |
| 11-W0-COMP-06 | TBD | TBD | COMP-06 | T-11-05 / T-11-06 | Every public warning code has a documented compatibility rule and evidence reference. | docs/unit/script | `ctest --preset windows-msvc-debug-static -R "compatibility_evidence" --output-on-failure` | Missing evidence catalog and machine check | pending |

---

## Wave 0 Requirements

- [ ] `include/libbsa/validation.hpp` - public validation API types and Doxygen comments.
- [ ] `src/validation.cpp` - strict-open-backed validation report assembly.
- [ ] `tests/unit/validation_api_tests.cpp` - public API and writer-output validation coverage.
- [ ] `tests/unit/compatibility_warning_tests.cpp` - stable warning-code/severity/path coverage without exact message assertions.
- [ ] `docs/compatibility-evidence.md` plus a CTest-discovered check - warning-code catalog coverage.
- [ ] `tests/fixtures/generated/compatibility_matrix.json` or equivalent - consolidated malformed compatibility matrix.
- [ ] Per-family malformed manifest helpers reject unknown `expected_error` values instead of silently defaulting.
- [ ] Additive sanitizer preset or documented command path for malformed/parser/compression/validation labels.
- [ ] Reconfigure local build tree before validation if CTest still references stale paths from another checkout.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Optional local BSArchPro/game corpus comparison | COMP-01, COMP-02, COMP-06 | Copyrighted game data and TES5Edit output cannot become required committed fixtures. | Keep any local corpus ignored and skipped by default; verify required CI/test paths pass without those files. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify commands or Wave 0 dependencies.
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify.
- [ ] Wave 0 covers all missing references above.
- [ ] No watch-mode flags.
- [ ] Full static/shared Windows suites are green before phase closeout.
- [ ] Sanitizer-oriented malformed/validation path exists and is documented or preset-backed.
- [ ] `nyquist_compliant: true` set in frontmatter after the final plan maps every task to automated coverage.

**Approval:** pending
