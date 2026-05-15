---
phase: 17
slug: writer-hotspot-hardening-and-ship-gate
status: draft
nyquist_compliant: true
wave_0_complete: false
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
| **Estimated runtime** | Existing Debug build/test suite runtime |

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
| 17-01-01 | TBD | TBD | DEDU-01 | T-17-02 | TES4 dedupe uses keyed or bounded candidate narrowing while exact stored-byte equality remains the final shared-offset gate. | runtime + source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure -L tes4_bsa_writer` plus `writer_hotspot_policy` after Wave 0 | Runtime file exists; policy file W0 | pending |
| 17-01-02 | TBD | TBD | DEDU-02 | T-17-02 / T-17-03 | BA2 GNRL dedupe uses explicit staged identity or digest narrowing while exact equality and disk-source change rejection remain final gates. | runtime + source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure -L ba2_gnrl_writer` plus `writer_hotspot_policy` after Wave 0 | Runtime file exists; policy file W0 | pending |
| 17-01-03 | TBD | TBD | DX10-01 | T-17-01 / T-17-04 | BA2 DX10 cleans snapshot temp data on success, ordinary failure, failed add reservation, and consumes the writer after write attempts. | runtime | `ctest --preset windows-msvc-debug-static --output-on-failure -L ba2_dx10_writer` | Extend existing file | pending |
| 17-01-04 | TBD | TBD | DX10-02 | T-17-01 | Lifecycle docs and verification artifacts truthfully describe cleanup guarantees and residual abnormal-termination risk. | policy + docs verification | `ctest --preset windows-msvc-debug-static --output-on-failure -L writer_hotspot_policy` | W0 | pending |

---

## Wave 0 Requirements

- [ ] `tests/unit/writer_hotspot_policy_tests.cpp` — covers DEDU-01, DEDU-02, and DX10-02 source/docs guardrails.
- [ ] `tests/CMakeLists.txt` — registers `writer_hotspot_policy_tests.cpp` and label coverage.
- [ ] `tests/unit/ba2_dx10_writer_tests.cpp` — extends snapshot-directory cleanup and consumed-state coverage for success and ordinary failures.
- [ ] Existing TES4 and BA2 GNRL runtime tests — extend only where current coverage does not prove the new narrowing guard.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Residual abnormal-termination snapshot cleanup risk | DX10-02 | Process termination cannot be fully proven by ordinary unit tests without adding broad fault-injection infrastructure. | Verify docs explicitly state the best-effort cleanup boundary and the remaining abnormal-termination risk. |

---

## Validation Sign-Off

- [ ] All tasks have automated verification or Wave 0 dependencies.
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify.
- [ ] Wave 0 covers all missing test and policy references.
- [ ] No watch-mode flags.
- [ ] Focused gates are used before full Debug, ASan, and Release package proof gates.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** pending
