---
phase: 05
slug: ba2-gnrl-read-extract
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-08
---

# Phase 05 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + CTest |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L ba2_gnrl --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~60 seconds |

## Sampling Rate

- **After every task commit:** Run focused label from the task verify block.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static -L "ba2_gnrl|public_include_boundary" --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite and TES5Edit clean status must be green.
- **Max feedback latency:** 60 seconds for focused labels.

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 05-01-01 | 01 | 1 | GNRL-08 | T-05-01 | Generated legal fixtures only | generator | `cmake --build --preset windows-msvc-debug-static --target generate_ba2_gnrl_fixtures` | ❌ W0 | ⬜ pending |
| 05-02-01 | 02 | 2 | GNRL-01,GNRL-02,GNRL-03,GNRL-08 | T-05-02 | Unsupported/malformed split | unit | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_detector --output-on-failure` | ❌ W0 | ⬜ pending |
| 05-03-01 | 03 | 3 | GNRL-04,GNRL-05,GNRL-08 | T-05-03 | Checked spans and duplicate rejection | unit | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_metadata|ba2_gnrl_lookup" --output-on-failure` | ❌ W0 | ⬜ pending |
| 05-04-01 | 04 | 4 | GNRL-06,GNRL-07 | T-05-04 | Exact-size codec routing | unit | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_extract --output-on-failure` | ❌ W0 | ⬜ pending |
| 05-05-01 | 05 | 5 | GNRL-01,GNRL-03,GNRL-08 | T-05-05 | Fail-closed malformed cases | unit/regression | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_malformed|public_include_boundary|tes3_bsa|tes4_bsa" --output-on-failure` | ❌ W0 | ⬜ pending |

## Wave 0 Requirements

- [ ] `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` — generated BA2 fixture source.
- [ ] `tests/unit/ba2_gnrl_reader_tests.cpp` — BA2 detector, metadata, lookup, extraction, malformed tests.
- [ ] `src/formats/ba2/` — private BA2 parser/reader implementation directory.

## Manual-Only Verifications

All phase behaviors have automated verification.

## Validation Sign-Off

- [x] All tasks have `<automated>` verify commands.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 covers all initially missing BA2 test/fixture files.
- [x] No watch-mode flags.
- [x] Feedback latency target < 60s for focused labels.

**Approval:** pending execution
