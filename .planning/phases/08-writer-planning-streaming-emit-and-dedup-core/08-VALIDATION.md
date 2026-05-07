---
phase: 08
slug: writer-planning-streaming-emit-and-dedup-core
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-06
---

# Phase 08 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 `3.14.0#0` via vcpkg |
| **Config file** | `CMakeLists.txt` with explicit test executables and `catch_discover_tests` |
| **Quick run command** | `ctest --preset windows-msvc-vcpkg -R "libbsa_writer_tests|libbsa_compression_policy_tests|libbsa.public_header_smoke" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-vcpkg --output-on-failure` |
| **Estimated runtime** | ~30 seconds for focused writer tests once created; full suite runtime depends on local build state |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-vcpkg -R "libbsa_writer_tests|libbsa_compression_policy_tests|libbsa.public_header_smoke" --output-on-failure` once writer tests exist.
- **After every plan wave:** Run `ctest --preset windows-msvc-vcpkg --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite must be green, public-header smoke must include writer API coverage, CMake source-list gate must pass, and `git status --short TES5Edit` must be empty.
- **Max feedback latency:** 30 seconds for focused writer checks after Wave 0 adds the test target.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 08-01-01 | TBD | 1 | WRT-05 | T-08-01 | Writer finalization propagates sink failure and never reports partial sink writes as success. | unit | `ctest --preset windows-msvc-vcpkg -R libbsa_writer_tests --output-on-failure` | W0 | pending |
| 08-01-02 | TBD | 1 | WRT-06 | T-08-02 | Dedup only groups byte-identical post-policy payload regions when the target capability allows shared regions. | unit/codec | `ctest --preset windows-msvc-vcpkg -R libbsa_writer_tests --output-on-failure` | W0 | pending |
| 08-01-03 | TBD | 1 | WRT-07 | T-08-03 | Generated harness read-back compares planned metadata and extracted payload bytes. | fixture/roundtrip | `ctest --preset windows-msvc-vcpkg -R libbsa_writer_tests --output-on-failure` | W0 | pending |
| 08-01-04 | TBD | 1 | WRT-05/WRT-06/WRT-07 | T-08-04 | Public writer headers expose only libbsa-owned C++20 types and compile in consumer-style code. | smoke | `ctest --preset windows-msvc-vcpkg -R libbsa.public_header_smoke --output-on-failure` | existing smoke; writer coverage W0 | pending |

*Status: pending · green · red · flaky*

---

## Wave 0 Requirements

- [ ] `include/libbsa/writer.hpp` — public writer input, target capability, options, plan preview, and finalize API for WRT-05/WRT-06/WRT-07.
- [ ] `src/writer.cpp` — planning, checked arithmetic, stored payload planning, dedup grouping, and deterministic finalization.
- [ ] `tests/writer_core_tests.cpp` — plan determinism, preview, dedup enabled/disabled/unsupported, compression, finalization, sink failure, and invalid input tests.
- [ ] `tests/writer_harness_helpers.hpp` / `tests/writer_harness_helpers.cpp` — test-only generated harness builder, reader, and assertion helpers.
- [ ] `CMakeLists.txt` — explicit public header, source, helper, and test target wiring.
- [ ] `tests/public_header_smoke.cpp` — consumer-style writer planning/finalization coverage.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| `TES5Edit/` remains untouched | WRT-05/WRT-06/WRT-07 | This is a repository boundary check rather than a unit behavior. | Run `git status --short TES5Edit` and require empty output. |

---

## Validation Sign-Off

- [ ] All tasks have automated verify commands or Wave 0 dependencies.
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify.
- [ ] Wave 0 covers all missing references.
- [ ] No watch-mode flags.
- [ ] Feedback latency < 30s for focused writer checks after Wave 0.
- [ ] `nyquist_compliant: true` set in frontmatter once validation files exist and commands are verified.

**Approval:** pending
