---
phase: 09
slug: bsa-writers
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-06T23:07:43.3423504-07:00
---

# Phase 09 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 via existing CMake/vcpkg test setup |
| **Config file** | `CMakeLists.txt` with explicit source/test target wiring and `catch_discover_tests` |
| **Quick run command** | `ctest --preset windows-msvc-vcpkg -R "libbsa_bsa_writer_tests|libbsa_writer_tests|libbsa_bsa_reader_tests|libbsa_compression_policy_tests|libbsa.public_header_smoke" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-vcpkg --output-on-failure` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-vcpkg -R "libbsa_bsa_writer_tests|libbsa_writer_tests|libbsa_bsa_reader_tests|libbsa_compression_policy_tests|libbsa.public_header_smoke" --output-on-failure`
- **After every plan wave:** Run `ctest --preset windows-msvc-vcpkg --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green, public-header smoke must pass, and `git status --short TES5Edit` must be empty.
- **Max feedback latency:** 60 seconds for focused tests; full suite before wave/phase gates.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 09-01-01 | 09-01 | 1 | WRT-01 | T-09-01 | BSA writer public API exposes libbsa-owned types only and no private dependency/TES5Edit leakage | public smoke | `ctest --preset windows-msvc-vcpkg -R libbsa.public_header_smoke --output-on-failure` | ✅ W0 | ✅ green |
| 09-02-01 | 09-02 | 2 | WRT-01 | T-09-02 | v103/v104/v105 writers emit native headers/tables/hashes/flags/compression/embedded-name payloads that read back | fixture/roundtrip/codec | `ctest --preset windows-msvc-vcpkg -R libbsa_bsa_writer_tests --output-on-failure` | ✅ W0 | ✅ green |
| 09-03-01 | 09-03 | 3 | WRT-01 | T-09-03 | Disk and memory inputs produce read-back-equivalent BSA archives | fixture/roundtrip | `ctest --preset windows-msvc-vcpkg -R libbsa_bsa_writer_tests --output-on-failure` | ✅ W0 | ✅ green |
| 09-04-01 | 09-04 | 4 | WRT-04 | T-09-04 | TES3 writer emits hash-sorted records, correct name offsets/hash table, raw payloads, and data-section-relative offsets | unit/fixture/roundtrip | `ctest --preset windows-msvc-vcpkg -R libbsa_bsa_writer_tests --output-on-failure` | ✅ W0 | ✅ green |
| 09-05-01 | 09-05 | 5 | WRT-01, WRT-04 | T-09-05 | Invalid inputs, unsupported targets/options, impossible layout, missing disk files, and sink failures return structured errors | unit/negative | `ctest --preset windows-msvc-vcpkg -R "libbsa_bsa_writer_tests|libbsa_writer_tests" --output-on-failure` | ✅ W0 | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `include/libbsa/bsa.hpp` and/or `include/libbsa/writer.hpp` — public BSA writer declarations: explicit target enum/value, options, separate memory/disk entry types, native plan records, and plan/finalize functions.
- [x] `src/bsa_writer.cpp` or equivalent — private BSA writer implementation for TES3/TES4-family planning, table serialization, payload shaping, dedup compatibility, and sink finalization.
- [x] `tests/bsa_writer_tests.cpp` or equivalent — generated native BSA layout assertions and read-after-write round trips.
- [x] `tests/public_header_smoke.cpp` — public smoke creates one TES3, v103, v104, and v105 archive using public headers only.
- [x] `CMakeLists.txt` — explicit source/header/test wiring with no `TES5Edit/` inclusion.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| TES5Edit submodule remains untouched | WRT-01, WRT-04 | Git status over submodule boundary is a repository-state check, not a Catch2 test | Run `git status --short TES5Edit` and require empty output |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 60s for focused tests
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** Phase 09 validation sign-off complete after 09-06 focused and full boundary gates.
