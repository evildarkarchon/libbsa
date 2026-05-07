---
phase: 10
slug: ba2-writers
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-07
---

# Phase 10 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 through existing CMake/vcpkg test setup |
| **Config file** | `CMakeLists.txt`; configure/build via `CMakePresets.json` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa.public_header_smoke"` |
| **Full suite command** | `ctest --preset windows-msvc-vcpkg --output-on-failure` or `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~60 seconds for focused tests; full suite depends on local build state |

---

## Sampling Rate

- **After every task commit:** Run the quick command for focused BA2 writer, BA2 reader, BA2 DDS reader, and public-header smoke coverage.
- **After every plan wave:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa_writer_tests|libbsa_bsa_writer_tests|libbsa.public_header_smoke"`.
- **Before `/gsd-verify-work`:** Full suite must be green, private-token public-header grep must be clean, and `git status --short TES5Edit` must be empty.
- **Max feedback latency:** 60 seconds for focused BA2 writer checks.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 10-01-01 | TBD | 1 | WRT-02 | T-10-01 | Checked BA2 GNRL header, record, name-table, payload, and size arithmetic before plan success | unit/fixture/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | No - Wave 0 | pending |
| 10-01-02 | TBD | 1 | WRT-02 | T-10-02 | Archive paths validate before host file reads; missing disk inputs fail structurally | fixture/io/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | No - Wave 0 | pending |
| 10-02-01 | TBD | 2 | WRT-03 | T-10-03 | DDS bytes are analyzed during planning through private texture helpers and malformed DDS inputs return structured failures | unit/fixture/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | No - Wave 0 | pending |
| 10-02-02 | TBD | 2 | WRT-03 | T-10-04 | DDS chunks route through explicit raw, deflate, or Starfield method-3 LZ4-block codecs without fallback | fixture/codec/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_dds_reader_tests"` | No - Wave 0 for writer tests; reader helpers exist | pending |
| 10-03-01 | TBD | 3 | WRT-02/WRT-03 | T-10-05 | Public BA2 writer APIs compile and run without leaking private dependency headers | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | File exists; BA2 writer coverage missing | pending |

---

## Wave 0 Requirements

- [ ] `include/libbsa/ba2_writer.hpp` - public BA2 writer target/options/input/plan/finalize declarations.
- [ ] `src/ba2_writer.cpp` - native GNRL and DX10 planning, compression, dedup, and finalization.
- [ ] `src/texture/dds_analysis.hpp` and `src/texture/dds_analysis.cpp` - private DirectXTex input analysis and mip/chunk byte extraction.
- [ ] `tests/ba2_writer_tests.cpp` - layout, read-after-write, disk/memory, DDS, compression, dedup, and failure coverage.
- [ ] `tests/public_header_smoke.cpp` update - public BA2 writer compile/link/runtime smoke without private headers.
- [ ] `CMakeLists.txt` update - explicit new public header, source, private texture helper, and test target wiring with no `TES5Edit/` inclusion.
- [ ] DDS reconstruction format coverage review - current reconstruction helper support must match generated writer fixtures or fail unsupported formats during planning.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| External Archive2/BSArchPro corpus parity | WRT-02/WRT-03 | Phase 11 owns broad external corpus validation after production BA2 writers exist | Do not gate Phase 10 on external corpus parity; generated read-after-write is sufficient here. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
