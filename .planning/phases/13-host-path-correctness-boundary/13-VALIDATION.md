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
| **Quick run command** | `ctest --preset windows-msvc-debug-static -R "host_file|host_path_correctness_boundary_smoke" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~20 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static -R "host_file|host_path_correctness_boundary_smoke" --output-on-failure`
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 20 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 13-W0-01 | 13-04 task 1 | 0 | HOST-01 | T-13-12 | `archive_reader::open` succeeds from non-ASCII Windows host paths for the locked representative TES4 and BA2 fixtures | fixture regression | `ctest --preset windows-msvc-debug-static -R "host_file|host_path_correctness_boundary_smoke" --output-on-failure` | ❌ W0 | ⬜ pending |
| 13-W0-02 | 13-04 task 1 | 0 | HOST-02 | T-13-13 | `validate_archive(..., {.validate_entry_extractability = true})` succeeds from the same non-ASCII host paths without reintroducing a separate setup path | fixture regression | `ctest --preset windows-msvc-debug-static -R "host_file|host_path_correctness_boundary_smoke" --output-on-failure` | ❌ W0 | ⬜ pending |
| 13-W0-03 | 13-04 task 2 | 0 | HOST-03 | T-13-14 | One canonical payload extraction per representative archive succeeds from the non-ASCII host path and matches manifest-backed expected bytes | fixture regression | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/unit/host_path_correctness_boundary_tests.cpp` — dedicated cross-family non-ASCII host-path regression suite
- [ ] `tests/CMakeLists.txt` — register the new suite in `libbsa_tests`
- [ ] File-local helpers for non-ASCII temp directory/filename setup, archive copy, and manifest-backed expected payload lookup
- [ ] Stable tag naming containing `host_path_correctness_boundary` for the targeted quick-run filter
- [ ] One stable smoke subset/tag containing `host_path_correctness_boundary_smoke` so task-level feedback stays under the preferred Nyquist latency target

Execution alignment: Plans `13-01` through `13-03` keep the shared host-file boundary buildable and runnable before `13-04` satisfies these Wave 0 coverage gaps.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
