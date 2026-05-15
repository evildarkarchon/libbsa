---
phase: 15
slug: reader-backend-dispatch-cleanup
status: validated
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-14
audited: 2026-05-14
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
| 15-01-01 | 01 | 0 | DISP-01 | T-15-01/T-15-02/T-15-04 | Public `archive_reader` operations preserve current list/find/contains/extract/extract_bytes/extract_entries semantics after one open-time backend selection | runtime fixture regression + bulk regression | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|bulk_extraction"` | ✅ `tests/unit/archive_reader_dispatch_tests.cpp` | ✅ green |
| 15-01-02 | 01 | 0 | DISP-02 | T-15-05 | Public reader methods fail policy tests if archive-family branching returns outside the centralized open-time seam | source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "archive_reader_dispatch_policy"` | ✅ `tests/unit/archive_reader_dispatch_policy_tests.cpp` | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `tests/unit/archive_reader_dispatch_tests.cpp` — dedicated cross-family runtime suite covering all Phase 15 operations once per representative backend
- [x] `tests/unit/archive_reader_dispatch_policy_tests.cpp` — method-scoped negative guard over `src/archive.cpp`
- [x] `tests/CMakeLists.txt` — registers both Phase 15 test files so Catch2 discovery includes them
- [x] Discoverable test names and tags support the fast regex runs above: `reader_backend_dispatch`, `archive_reader_dispatch_policy`, and `bulk_extraction`

---

## Audit Evidence

| Command | Result |
|---------|--------|
| `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` | ✅ Passed 15/15 tests |
| `ctest --preset windows-msvc-debug-static --output-on-failure` | ✅ Passed 382/382 tests; 2 opt-in tests skipped |

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 30s
- [x] `nyquist_compliant: true` set in frontmatter after final review
- [x] Validation commands are run through the Windows MSVC debug preset so MSVC-backed presets resolve correctly

**Approval:** Nyquist-compliant after audit.

---

## Validation Audit 2026-05-14

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
| Requirements covered | 2 |
| Automated commands rerun | 2 |
