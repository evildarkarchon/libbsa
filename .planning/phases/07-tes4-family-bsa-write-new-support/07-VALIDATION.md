---
phase: 07
slug: tes4-family-bsa-write-new-support
status: verified
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-08
updated: 2026-05-09
---

# Phase 07 — Validation Strategy

Per-phase validation contract for feedback sampling during execution. Phase 7 validation is automation-first: every writer behavior is proven by Catch2/CTest through public writer APIs, reopened with `archive_reader` where serialization is in scope, and byte-inspected where table/header layout is the requirement.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + CTest |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` |
| **Public boundary command** | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` |
| **Reader regression command** | `ctest --preset windows-msvc-debug-static -R "tes4_bsa_|unsupported_future_bsa" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **TES5Edit boundary command** | `git -C "TES5Edit" status --short` |
| **Estimated runtime** | Writer/public/reader slices complete in under 2s each in the configured Windows MSVC static preset; full suite runtime is environment-dependent and must complete before release verification. |

The previous reader command, `ctest --preset windows-msvc-debug-static -R tes4_bsa_reader --output-on-failure`, is retired because it matches no tests in the current Catch2-discovered CTest registry. The replacement `tes4_bsa_|unsupported_future_bsa` regex selects the effective TES4-family reader regression tests by behavioral test name and was verified green.

---

## Sampling Rate

- **After every task commit:** Run the task's listed automated command. Writer changes use `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure`.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure` plus the TES5Edit boundary command.
- **Before `/gsd-verify-work`:** Full suite, public boundary, effective reader regression, writer label, and TES5Edit boundary commands must be green.
- **Max feedback latency:** No more than one task may land without a concrete automated command result or an explicitly recorded compiler/vcpkg environment blocker.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-01-02 | Public header forbidden-token scan blocks private dependency leakage. | public boundary RED/GREEN | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | ✅ `tests/unit/public_include_boundary_tests.cpp` | ✅ green |
| 07-01-02 | 01 | 1 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-01-01/T-07-01-02/T-07-01-03 | Writer API exposes explicit paths, result-style fallibility, and no private codec/TES5Edit types. | public boundary GREEN | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | ✅ `include/libbsa/writer.hpp` | ✅ green |
| 07-02-01 | 02 | 2 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-02-01/T-07-02-02/T-07-02-03 | Tests cover copied memory ownership, duplicate canonical paths, invalid paths, overwrite guard, missing source, and explicit archive paths. | writer validation | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `tests/unit/tes4_bsa_writer_tests.cpp` | ✅ green |
| 07-02-02 | 02 | 2 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-02-01/T-07-02-02/T-07-02-03 | Writer validates paths, rejects duplicates at write time, owns memory bytes, and blocks default overwrite. | writer validation | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `src/formats/bsa/tes4_bsa_writer.cpp` | ✅ green |
| 07-03-01 | 03 | 3 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-10 | T-07-03-01/T-07-03-02/T-07-03-03 | Tests require all-raw v103/v104/v105 archives to reopen, list/find/contains/extract, preserve copied memory bytes, and expose content-derived flags. | round-trip + byte inspection | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `tests/unit/tes4_bsa_writer_tests.cpp` | ✅ green |
| 07-03-02 | 03 | 3 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-10 | T-07-03-01/T-07-03-02/T-07-03-03/T-07-03-04 | Serializer uses hash ordering, checked offset arithmetic, derived file flags, temp-file publish, and reader-backed raw round trips. | round-trip + byte inspection | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `src/formats/bsa/tes4_bsa_writer.cpp` | ✅ green |
| 07-04-01 | 04 | 4 | WBSA-01/WBSA-02/WBSA-03/WBSA-07/WBSA-10 | T-07-04-01/T-07-04-02/T-07-04-03 | Tests require archive default compression, target-default inheritance, per-entry overrides, zero-byte raw behavior, and codec-private public surface. | compression round-trip | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `tests/unit/tes4_bsa_writer_tests.cpp` | ✅ green |
| 07-04-02 | 04 | 4 | WBSA-01/WBSA-02/WBSA-03/WBSA-07/WBSA-10 | T-07-04-01/T-07-04-02/T-07-04-03 | Writer routes v103/v104 to deflate when compressed, v105 to LZ4 frame, writes raw-size prefixes, and sets XOR toggle bits. | compression round-trip | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `src/formats/bsa/tes4_bsa_writer.cpp` | ✅ green |
| 07-05-01 | 05 | 5 | WBSA-02/WBSA-03/WBSA-08/WBSA-10 | T-07-05-01/T-07-05-02/T-07-05-03 | Tests require embedded names absent by default, v104/v105 opt-in metadata, v103 compatibility, and extracted-byte stripping. | embedded-name round-trip | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `tests/unit/tes4_bsa_writer_tests.cpp` | ✅ green |
| 07-05-02 | 05 | 5 | WBSA-02/WBSA-03/WBSA-08/WBSA-10 | T-07-05-01/T-07-05-02/T-07-05-03 | Writer emits global target-compatible embedded-name prefixes only for v104/v105 and never exposes per-entry raw flag control. | embedded-name round-trip | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `src/formats/bsa/tes4_bsa_writer.cpp` | ✅ green |
| 07-06-01 | 06 | 6 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-07/WBSA-08/WBSA-09/WBSA-10 | T-07-06-01/T-07-06-02 | Tests require dedupe disabled by default, opt-in shared offsets for identical final stored bytes, and negative cases for different stored encodings. | dedupe round-trip | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ✅ `tests/unit/tes4_bsa_writer_tests.cpp` | ✅ green |
| 07-06-02 | 06 | 6 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-07/WBSA-08/WBSA-09/WBSA-10 | T-07-06-01/T-07-06-02/T-07-06-03/T-07-06-04 | Writer dedupes only byte-identical final stored payloads and passes writer, reader, public boundary, and TES5Edit gates. | final regression | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` + `ctest --preset windows-msvc-debug-static -R "tes4_bsa_|unsupported_future_bsa|public_include_boundary" --output-on-failure` + TES5Edit boundary command | ✅ `src/formats/bsa/tes4_bsa_writer.cpp`, `tests/unit/public_include_boundary_tests.cpp` | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Wave 0 is complete. All planned test and implementation paths exist and have been exercised by the writer/public/reader verification commands:

- [x] `tests/unit/public_include_boundary_tests.cpp` — writer public-boundary compile checks and forbidden-token scan.
- [x] `include/libbsa/writer.hpp` — public writer contract.
- [x] `tests/unit/tes4_bsa_writer_tests.cpp` — writer behavior and byte-level validation tests.
- [x] `tests/CMakeLists.txt` — registers `unit/tes4_bsa_writer_tests.cpp`; Catch tags include `[tes4_bsa_writer]` so `-L tes4_bsa_writer` selects writer tests.
- [x] `src/formats/bsa/tes4_bsa_writer.hpp` and `src/formats/bsa/tes4_bsa_writer.cpp` — private writer implementation paths.

---

## Requirement Coverage Matrix

| Requirement | Plans | Automated Proof |
|-------------|-------|-----------------|
| WBSA-01 | 01, 02, 03, 04, 06 | Public writer target plus v103 disk/memory round-trip through `archive_reader`; `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` |
| WBSA-02 | 01, 02, 03, 04, 05, 06 | v104 disk/memory, deflate target-default, embedded-name, and dedupe round trips; writer label command |
| WBSA-03 | 01, 02, 03, 04, 05, 06 | v105 disk/memory, LZ4-frame target-default, embedded-name, and dedupe round trips; writer label command |
| WBSA-05 | 03, 06 | `TES4 BSA writer serializes derived file flags and hash-sorted tables` inspects generated bytes and asserts sorted folder/file record hashes; writer label command |
| WBSA-06 | 03, 06 | Header `file_flags` byte inspection for mesh/texture/script/menu/misc paths plus archive flag assertions; writer label command |
| WBSA-07 | 04, 06 | `target_default` v103 raw/v104 deflate/v105 LZ4-frame round-trip, all-compressed overrides, all-raw compressed override, and zero-byte raw behavior; writer label command |
| WBSA-08 | 05, 06 | Embedded-name enabled/disabled and v103 compatibility tests; writer label command |
| WBSA-09 | 06 | Dedupe disabled/enabled offset and negative stored-encoding tests; writer label command |
| WBSA-10 | 01, 02, 03, 04, 05, 06 | Pack/reopen/find/contains/extract/byte-compare plus explicit overwrite-success test, public boundary, effective reader regression, and TES5Edit boundary commands |

---

## Latest Verification Evidence

| Command | Result |
|---------|--------|
| `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | PASS |
| `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | PASS — 19/19 writer tests |
| `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | PASS — 3/3 public boundary tests |
| `ctest --preset windows-msvc-debug-static -R "tes4_bsa_|unsupported_future_bsa" --output-on-failure` | PASS — 21/21 effective TES4 reader regression tests |
| `git -C "TES5Edit" status --short` | PASS — no output |

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None expected | WBSA-01..WBSA-10 | All Phase 7 behaviors are covered by generated synthetic data and automated round-trip or byte-level tests. | N/A |

---

## Validation Sign-Off

- [x] All tasks have effective automated verify commands.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 paths exist and are verified.
- [x] No watch-mode flags.
- [x] Stale no-match reader command retired and replaced with a verified effective command.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** verified; Phase 7 validation gaps filled by passing automated tests and updated evidence.
