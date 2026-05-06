---
phase: 06
slug: ba2-gnrl-read-and-extract
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-05
---

# Phase 06 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 `3.14.0#0` via vcpkg and CTest |
| **Config file** | `CMakeLists.txt`, `CMakePresets.json` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_reader_tests|libbsa.public_header_smoke"` |
| **Full suite command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~30 seconds for quick BA2/smoke sampling; full suite depends on local build state |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_reader_tests|libbsa.public_header_smoke"`
- **After every plan wave:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L "unit|fixture|codec|smoke"`
- **Before `/gsd-verify-work`:** Full suite must be green with `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`
- **Max feedback latency:** 30 seconds for quick BA2/smoke sampling on a configured local build

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 06-01-01 | 01 | 1 | BA2-01, BA2-02, BA2-03, BA2-04 | T-06-01 | Public BA2 API compiles without private dependency leakage | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | No - Wave 0 adds `include/libbsa/ba2.hpp` | pending |
| 06-01-02 | 01 | 1 | BA2-01, BA2-02, BA2-03, BA2-04 | T-06-02 | BA2 fixtures are deterministic and source-reviewable | fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` | No - Wave 0 adds `tests/ba2_reader_tests.cpp` | pending |
| 06-02-01 | 02 | 2 | BA2-01, BA2-02, BA2-04 | T-06-03 | Parser rejects truncated tables and invalid offsets with structured errors | unit/fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` | No - Wave 0 adds parser tests | pending |
| 06-03-01 | 03 | 3 | BA2-01, BA2-03 | T-06-04 | Compression routing never falls back across deflate, LZ4 frame, and raw LZ4 block | fixture/codec | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L codec` | No - Wave 0 adds BA2 route tests | pending |

*Status: pending, green, red, flaky*

---

## Wave 0 Requirements

- [ ] `include/libbsa/ba2.hpp` - public API for `ba2_archive`, `open_ba2`, and `extract_ba2_entry`
- [ ] `src/ba2_reader.cpp` - BA2 GNRL parser and extractor implementation
- [ ] `tests/ba2_reader_tests.cpp` - generated BA2 fixture builders and malformed cases
- [ ] `tests/public_header_smoke.cpp` - include `ba2.hpp`, name `ba2_archive`, and take `open_ba2` / `extract_ba2_entry` addresses
- [ ] `CMakeLists.txt` - explicit new public header, source file, and BA2 reader test target wiring

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| `TES5Edit/` remains unmodified | BA2-01 through BA2-04 | Git submodule cleanliness is a repository-state check, not a CTest assertion | Run `git status --short TES5Edit` and require empty output |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s for quick BA2/smoke sampling
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
