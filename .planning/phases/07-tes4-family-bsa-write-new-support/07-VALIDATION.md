---
phase: 07
slug: tes4-family-bsa-write-new-support
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-08
---

# Phase 07 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + CTest |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `{quick command}` |
| **Full suite command** | `{full command}` |
| **Estimated runtime** | ~{N} seconds |

---

## Sampling Rate

- **After every task commit:** Run `{quick run command}`
- **After every plan wave:** Run `{full suite command}`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** {N} seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | WBSA-01 | — | N/A | unit | `{command}` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `{writer API public-boundary test file}` — stubs for WBSA-01 through WBSA-03.
- [ ] `{TES4-family writer round-trip test file}` — stubs for WBSA-05 through WBSA-10.
- [ ] `{CMake test wiring}` — add writer test sources and labels if missing.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None expected | WBSA-01..WBSA-10 | All Phase 7 behaviors should be covered by generated fixtures and automated round-trip tests. | N/A |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < {N}s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
