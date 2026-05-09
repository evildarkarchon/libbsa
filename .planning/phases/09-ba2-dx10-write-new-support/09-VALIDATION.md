---
phase: 09
slug: ba2-dx10-write-new-support
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-09
---

# Phase 09 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 via vcpkg and CTest discovery |
| **Config file** | `tests/CMakeLists.txt`, `CMakePresets.json` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure` |
| **Full suite command** | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | Unknown until writer fixtures exist |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer|dds_layout|public_include" --output-on-failure` when the build is current.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static -L unit --output-on-failure`.
- **Before `/gsd-verify-work`:** Full static preset and `git -C TES5Edit status --short` must be green.
- **Max feedback latency:** Unknown until Phase 9 test suite exists.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 09-01-01 | TBD | TBD | WBA2-06 | T-09-01 | DDS host-file writer output reopens and extracts for FO4 v1 | integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*fo4 --output-on-failure` | no Wave 0 | pending |
| 09-01-02 | TBD | TBD | WBA2-07 | T-09-02 | Starfield v3 method `3` output reopens and extracts with raw LZ4 block routing | integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*starfield --output-on-failure` | no Wave 0 | pending |
| 09-01-03 | TBD | TBD | WBA2-08 | T-09-03 | Add-time DDS validation rejects malformed or unsupported DDS without public DirectXTex leakage | unit/integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*dds --output-on-failure` | no Wave 0 | pending |
| 09-01-04 | TBD | TBD | WBA2-09 | T-09-04 | Chunk planner covers mip/array/cubemap layouts without gaps or overlaps | unit | `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` | extend existing | pending |
| 09-01-05 | TBD | TBD | WBA2-10 | T-09-05 | Compressed chunk serialization extracts with exact-size validation | integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*compression --output-on-failure` | no Wave 0 | pending |
| 09-01-06 | TBD | TBD | WBA2-11 | T-09-06 | Pack/reopen/list/find/contains/extract/DirectXTex validation proves writer output | integration | `ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure` | no Wave 0 | pending |

*Status: pending / green / red / flaky*

---

## Wave 0 Requirements

- [ ] `tests/unit/ba2_dx10_writer_tests.cpp` - covers WBA2-06 through WBA2-11.
- [ ] `tests/unit/public_include_boundary_tests.cpp` - additions for `ba2_dx10_target`, options, and writer methods.
- [ ] `tests/fixtures/generated/` DDS source fixtures and manifests - committed legal inputs outside `TES5Edit/`.
- [ ] `tests/CMakeLists.txt` - new writer test and fixture generator byproducts wired into Catch2/CTest.
- [ ] `tests/unit/dds_layout_tests.cpp` - locked DXGI format size math and chunk planner coverage.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Game-engine stability for compressed-only DX10 archives | WBA2-06, WBA2-07 | Real game loading is optional local compatibility evidence and cannot be mandatory repo CI input | If local FO4/SF environments are available, pack generated textures, load in target game/tooling, and record observations separately from mandatory tests |

---

## Validation Sign-Off

- [ ] All tasks have automated verify commands or Wave 0 dependencies.
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify.
- [ ] Wave 0 covers all missing references.
- [ ] No watch-mode flags.
- [ ] Feedback latency documented after tests exist.
- [ ] `nyquist_compliant: true` set in frontmatter.

**Approval:** pending
