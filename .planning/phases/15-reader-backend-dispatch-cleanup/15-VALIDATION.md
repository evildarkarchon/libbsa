---
phase: 15
slug: reader-backend-dispatch-cleanup
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-14
---

# Phase 15 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3 via `Catch2::Catch2WithMain` |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"`
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 15-01-01 | 01 | 0 | DISP-01 | T-15-01 | Public `archive_reader` operations preserve current list/find/contains/extract semantics after one open-time backend selection | runtime fixture regression | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch"` | ❌ W0 | ⬜ pending |
| 15-01-02 | 01 | 0 | DISP-02 | T-15-02 | Public reader methods fail policy tests if archive-family branching returns outside the centralized open-time seam | source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "archive_reader_dispatch_policy"` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/unit/archive_reader_dispatch_tests.cpp` — dedicated cross-family runtime suite covering all Phase 15 operations once per representative backend
- [ ] `tests/unit/archive_reader_dispatch_policy_tests.cpp` — method-scoped negative guard over `src/archive.cpp`
- [ ] `tests/CMakeLists.txt` — register the new test file(s) so Catch2 discovery includes them
- [ ] Standardize discoverable test names or tags for the fast regex runs above

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
- [ ] `nyquist_compliant: true` set in frontmatter after final review
- [ ] Validation commands are run from a Visual Studio developer shell so MSVC-backed presets resolve correctly

**Approval:** pending
