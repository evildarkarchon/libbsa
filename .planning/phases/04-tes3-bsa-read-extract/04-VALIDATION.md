---
phase: 04
slug: tes3-bsa-read-extract
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-08
---

# Phase 04 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 through `Catch2::Catch2WithMain`, discovered by CTest with tags as labels. |
| **Config file** | `tests/CMakeLists.txt` plus `CMakePresets.json`. |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` after build.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite must be green, generated fixture outputs committed, public include boundary tests pass, and `git -C TES5Edit status --short` must be empty.
- **Max feedback latency:** 60 seconds for quick unit feedback.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 04-01-01 | 01 | 1 | BSA-04 | T-04-01 | Valid TES3 archive opens and exposes metadata without unsupported/TES4 misclassification. | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_metadata --output-on-failure` | ❌ W0 | ⬜ pending |
| 04-01-02 | 01 | 1 | BSA-04 | T-04-02 | TES3 entries list deterministic metadata with canonical paths, original paths, sizes, absolute offsets, and archive hashes. | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_entries --output-on-failure` | ❌ W0 | ⬜ pending |
| 04-01-03 | 01 | 1 | BSA-04 | T-04-03 | TES3 `find()` and `contains()` preserve normalized lookup and invalid path error semantics. | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_lookup --output-on-failure` | ❌ W0 | ⬜ pending |
| 04-02-01 | 02 | 2 | BSA-08 | T-04-04 | TES3 extraction converts data-section-relative raw offsets to validated archive-absolute payload offsets. | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_extract --output-on-failure` | ❌ W0 | ⬜ pending |
| 04-02-02 | 02 | 2 | BSA-04/BSA-08 | T-04-05 | Malformed TES3 tables, names, payload spans, duplicate paths, hash mismatch/collision/unsorted records fail closed. | unit + malformed fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure` | ❌ W0 | ⬜ pending |
| 04-03-01 | 03 | 3 | BSA-04/BSA-08 | T-04-06 | Existing TES4-family behavior and public include boundaries survive variant dispatch and `archive_hash` rename. | regression | `ctest --preset windows-msvc-debug-static -L tes4_bsa --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` — generates rich TES3 success archive, raw-offset manifest, hash halves, and malformed fixtures.
- [ ] `tests/fixtures/generated/archives/tes3_*.bsa` and `tes3_*_manifest.json` — committed generated outputs for default CI.
- [ ] `tests/unit/tes3_bsa_reader_tests.cpp` — covers BSA-04 and BSA-08 detection/list/lookup/extract/malformed behavior.
- [ ] `tests/CMakeLists.txt` — add TES3 unit source and fixture generator target/custom target.
- [ ] Existing tests and public header assertions need `tes4_hash` to `archive_hash` updates.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| TES5Edit submodule remains read-only | BSA-04/BSA-08 | Git submodule cleanliness is a repository state check, not a Catch2 assertion. | Run `git -C TES5Edit status --short` and verify it produces no output. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
