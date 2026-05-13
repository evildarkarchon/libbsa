---
phase: 13
slug: host-path-correctness-boundary
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-13
---

# Phase 13 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3 via `Catch2::Catch2WithMain` |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -R "host_file|archive_reader|validation_api|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` |
| **Phase-close host-path command** | `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary|host_path_correctness_boundary_smoke" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~20 seconds |

---

## Sampling Rate

- **After every task commit:** Run the focused command from the active task's `<verify>` block. Before Plan 13-05 lands the dedicated non-ASCII suite, the stable quick gate is `ctest --preset windows-msvc-debug-static -R "host_file|archive_reader|validation_api|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure`.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 20 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 13-P3-01 | 13-03 task 2 | 3 | HOST-01 | T-13-03-01 | `archive_reader::open` resolves once and parser entry reads use the shared host-file boundary without narrow-string drift | focused reader/parser regression | `ctest --preset windows-msvc-debug-static -R "archive_reader|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` | ✅ existing | ⬜ pending |
| 13-P4-01 | 13-04 task 2 | 4 | HOST-02 | T-13-04-02 | `validate_archive` reuses `archive_reader::open`, keeps the result/report split, and still drives extractability through the public reader path | focused validation/runtime regression | `ctest --preset windows-msvc-debug-static -R "validation_api|archive_reader|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` | ✅ existing | ⬜ pending |
| 13-P5-01 | 13-05 task 2 | 5 | HOST-01 / HOST-02 / HOST-03 | T-13-05-01 | Dedicated non-ASCII regression suite proves open, validate, and one canonical extraction per representative archive from public APIs | fixture regression | `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary" --output-on-failure` | ❌ plan 13-05 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Plan 13-05 Suite Requirements

- [ ] `tests/unit/host_path_correctness_boundary_tests.cpp` — dedicated cross-family non-ASCII host-path regression suite
- [ ] `tests/CMakeLists.txt` — register the new suite in `libbsa_tests`
- [ ] File-local helpers for non-ASCII temp directory/filename setup, archive copy, and manifest-backed expected payload lookup
- [ ] Stable tag naming containing `host_path_correctness_boundary` for the targeted quick-run filter
- [ ] One stable smoke subset/tag containing `host_path_correctness_boundary_smoke` so task-level feedback stays under the preferred Nyquist latency target

Execution alignment: Plans `13-01` through `13-04` rely on existing focused helper, reader, and validation gates while the runtime boundary is being built. Plan `13-05` then adds the dedicated non-ASCII public-proof suite and smoke subset as the phase-close regression surface.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or an earlier existing focused regression gate
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] The dedicated non-ASCII suite lands in Plan 13-05 and closes the remaining phase-specific proof obligations
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
