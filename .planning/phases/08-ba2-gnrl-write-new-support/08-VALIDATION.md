---
phase: 08
slug: ba2-gnrl-write-new-support
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-09
---

# Phase 08 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 through CTest |
| **Config file** | `tests/CMakeLists.txt`, root `CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` |
| **Full suite command** | `ctest --test-dir <build-dir> --output-on-failure` |
| **Estimated runtime** | ~60 seconds for targeted writer tests once built; full suite runtime depends on configured preset |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` and include public include boundary tests when public headers change.
- **After every plan wave:** Run `ctest --test-dir <build-dir> --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite must be green and `git -C TES5Edit status --short` must be empty.
- **Max feedback latency:** one task commit.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 08-W0-01 | TBD | 0 | WBA2-01 | T-08-01 | Public API creates disk-file and memory-buffer archives without dependency leakage | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` | missing W0 | pending |
| 08-W0-02 | TBD | 0 | WBA2-02 | T-08-02 | Explicit FO4 v1, Starfield v2, and Starfield v3 targets drive header/version/compression policy | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` | missing W0 | pending |
| 08-W0-03 | TBD | 0 | WBA2-03 | T-08-03 | End filename tables reopen safely without overlapping payload spans | parser regression | `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` | missing W0 | pending |
| 08-W0-04 | TBD | 0 | WBA2-04 | T-08-04 | Deflate and raw LZ4 block payloads round-trip through metadata-driven routing | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` | missing W0 | pending |
| 08-W0-05 | TBD | 0 | WBA2-05 | T-08-05 | Starfield header fields reopen through public metadata with documented defaults/overrides | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` | missing W0 | pending |

*Status: pending until Wave 0 creates the missing test files and CMake registrations.*

---

## Wave 0 Requirements

- [ ] `tests/unit/ba2_gnrl_writer_tests.cpp` - writer-output tests for WBA2-01 through WBA2-05, dedupe, zero-byte, end filename table, and round-trip extraction.
- [ ] `tests/unit/public_include_boundary_tests.cpp` - extend public API boundary coverage for BA2 GNRL writer types/options.
- [ ] `src/formats/ba2/ba2_gnrl_writer.hpp` and `src/formats/ba2/ba2_gnrl_writer.cpp` - production writer implementation source targets.
- [ ] `src/formats/ba2/ba2_gnrl_parser.cpp` - adjust host-file parsing to accept end-of-archive filename tables.
- [ ] `CMakeLists.txt` and `tests/CMakeLists.txt` - register writer sources and tests.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Developer toolchain availability | All | Current shell may lack vcpkg CLI, Ninja, or MSVC `cl`; test commands require an already configured build dir or developer shell | Before execution, confirm a valid build preset/build dir exists or run from a configured developer environment |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all missing references
- [ ] No watch-mode flags
- [ ] Feedback latency is one task commit
- [ ] `nyquist_compliant: true` set in frontmatter after Wave 0 is complete

**Approval:** pending
