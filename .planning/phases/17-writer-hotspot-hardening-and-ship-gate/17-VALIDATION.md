---
phase: 17
slug: writer-hotspot-hardening-and-ship-gate
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-14
---

# Phase 17 - Validation Strategy

> Per-phase validation contract for writer hotspot hardening and ship-gate evidence.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3 through `Catch2::Catch2WithMain`, discovered by CTest |
| **Config file** | `tests/CMakeLists.txt`; presets in `CMakePresets.json` |
| **Quick run command** | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|writer_hotspot_policy"` |
| **Full suite command** | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | Existing Debug build/test suite runtime; final post-review Debug gate passed 403/403 tests |

---

## Sampling Rate

- **After every task commit:** Run the focused affected runtime or policy labels for the touched writer area.
- **After every plan wave:** Run `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure`.
- **Before `/gsd-verify-work`:** Run focused Debug writer/runtime tests, focused MSVC ASan hardening tests for risky writer paths, and Release package proof.
- **Max feedback latency:** Use the focused label set before full-suite or Release proof gates.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 17-01-01 | 17-01 | Wave 1 | DEDU-01 | T-17-02 | TES4 dedupe uses keyed or bounded candidate narrowing while exact stored-byte equality remains the final shared-offset gate. | runtime + source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure -L tes4_bsa_writer` plus `writer_hotspot_policy`; final ship gate recorded in `17-VERIFICATION.md` | Runtime and policy files exist | complete |
| 17-01-02 | 17-02 | Wave 2 | DEDU-02 | T-17-02 / T-17-03 | BA2 GNRL dedupe uses explicit staged identity or digest narrowing while exact equality and disk-source change rejection remain final gates. | runtime + source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure -L ba2_gnrl_writer` plus `writer_hotspot_policy`; final ship gate recorded in `17-VERIFICATION.md` | Runtime and policy files exist | complete |
| 17-01-03 | 17-03 | Wave 1 | DX10-01 | T-17-01 / T-17-04 | BA2 DX10 cleans snapshot temp data on success, ordinary failure, failed add reservation, and consumes the writer after write attempts. | runtime | `ctest --preset windows-msvc-debug-static --output-on-failure -L ba2_dx10_writer`; final ship gate recorded in `17-VERIFICATION.md` | Extended existing file | complete |
| 17-01-04 | 17-04 | Wave 3 | DX10-02 | T-17-01 | Lifecycle docs and verification artifacts truthfully describe cleanup guarantees and residual abnormal-termination risk. | policy + docs verification | `ctest --preset windows-msvc-debug-static --output-on-failure -L writer_hotspot_policy`; final ship gate recorded in `17-VERIFICATION.md` | Policy file exists | complete |

---

## Wave 0 Requirements

- [x] `tests/unit/writer_hotspot_policy_tests.cpp` — covers DEDU-01, DEDU-02, and DX10-02 source/docs guardrails.
- [x] `tests/CMakeLists.txt` — registers `writer_hotspot_policy_tests.cpp` and label coverage.
- [x] `tests/unit/ba2_dx10_writer_tests.cpp` — extends snapshot-directory cleanup and consumed-state coverage for success and ordinary failures.
- [x] Existing TES4 and BA2 GNRL runtime tests — extend only where current coverage does not prove the new narrowing guard.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Residual abnormal-termination snapshot cleanup risk | DX10-02 | Process termination cannot be fully proven by ordinary unit tests without adding broad fault-injection infrastructure. | Verify docs explicitly state the best-effort cleanup boundary and the remaining abnormal-termination risk. |

---

## Validation Sign-Off

- [x] All tasks have automated verification or Wave 0 dependencies.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 covers all missing test and policy references.
- [x] No watch-mode flags.
- [x] Focused gates are used before full Debug, ASan, and Release package proof gates.
- [x] Post-review remediation gate clean: `17-REVIEW.md` is `status: clean` after fix commit `f1d59c9`, with the BA2 GNRL non-ASCII disk-source dedupe regression included in the final Debug and ASan gates.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** passed — Phase 17 ship-gate evidence is recorded in `17-VERIFICATION.md`.
