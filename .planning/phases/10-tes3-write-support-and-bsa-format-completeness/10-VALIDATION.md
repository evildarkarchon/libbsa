---
phase: 10
slug: tes3-write-support-and-bsa-format-completeness
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-09
---

# Phase 10 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 via vcpkg manifest with CTest discovery |
| **Config file** | `tests/CMakeLists.txt` plus root `CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir <build-dir> -R "tes3_bsa_writer|public_include_boundary" --output-on-failure` |
| **Full suite command** | `ctest --test-dir <build-dir> --output-on-failure` plus `git -C TES5Edit status --short` |
| **Estimated runtime** | TBD after first TES3 writer test target lands |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir <build-dir> -R "tes3_bsa_writer|public_include_boundary" --output-on-failure` once TES3 writer tests exist.
- **After every plan wave:** Run `ctest --test-dir <build-dir> -R "tes3_bsa_writer|tes3_bsa_reader|tes4_bsa_writer|public_include_boundary" --output-on-failure`.
- **Before `/gsd-verify-work`:** Run the full CTest suite and verify `git -C TES5Edit status --short` is empty.
- **Max feedback latency:** TBD after tests are introduced; planner should keep focused labels available for sub-minute iteration where practical.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 10-W0-01 | TBD | 0 | WBSA-04 | T-10-01 | Public TES3 writer API compiles without private/dependency types | compile/unit | `ctest --test-dir <build-dir> -R public_include_boundary --output-on-failure` | existing file, needs TES3 writer assertions | pending |
| 10-W0-02 | TBD | 0 | WBSA-04 | T-10-02 | Disk and memory entries produce raw TES3 archives that reopen and extract | unit/integration | `ctest --test-dir <build-dir> -R tes3_bsa_writer --output-on-failure` | missing | pending |
| 10-W0-03 | TBD | 0 | WBSA-04 | T-10-03 | Header/table/hash/name/raw-offset bytes match TES3 layout constraints | unit | `ctest --test-dir <build-dir> -R tes3_bsa_writer --output-on-failure` | missing | pending |
| 10-W0-04 | TBD | 0 | WBSA-04 | T-10-04 | Committed writer fixture evidence is generated from synthetic public-writer output | fixture | `cmake --build <build-dir> --target generate_tes3_bsa_writer_fixtures` or chosen equivalent | missing | pending |
| 10-W0-05 | TBD | 0 | WBSA-04 | T-10-05 | Existing TES3 reader, TES4 writer, public boundary, and TES5Edit read-only checks remain green | regression | `ctest --test-dir <build-dir> -R "tes3_bsa_reader|tes4_bsa_writer|public_include_boundary" --output-on-failure` and `git -C TES5Edit status --short` | existing tests | pending |

---

## Wave 0 Requirements

- [ ] `src/formats/bsa/tes3_bsa_writer.hpp` - private writer entry state and serializer declaration.
- [ ] `src/formats/bsa/tes3_bsa_writer.cpp` - public bridge, validation, serialization, and safe publish behavior.
- [ ] `tests/unit/tes3_bsa_writer_tests.cpp` - public API, layout, path/hash/order, disk/memory ownership, overwrite, missing sources, and round-trip tests.
- [ ] Writer fixture generator target or existing generator extension - committed public-writer fixture evidence.
- [ ] `tests/unit/public_include_boundary_tests.cpp` TES3 writer compile/public-boundary assertions.
- [ ] Root `CMakeLists.txt` and `tests/CMakeLists.txt` source/target registrations.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None | WBSA-04 | All Phase 10 acceptance criteria should have automated coverage | N/A |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify commands or Wave 0 dependencies.
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify.
- [ ] Wave 0 covers all missing validation references.
- [ ] No watch-mode flags.
- [ ] Feedback latency is measured once focused tests exist.
- [ ] `nyquist_compliant: true` set in frontmatter after plans map every behavior to tests.

**Approval:** pending
