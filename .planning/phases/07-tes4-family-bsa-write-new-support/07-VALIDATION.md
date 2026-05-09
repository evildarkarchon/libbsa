---
phase: 07
slug: tes4-family-bsa-write-new-support
status: ready
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-08
updated: 2026-05-08
---

# Phase 07 — Validation Strategy

Per-phase validation contract for feedback sampling during execution. Phase 7 validation is automation-first: every writer behavior is proven by Catch2/CTest through public writer APIs and reopened with `archive_reader` where serialization is in scope.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + CTest |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` |
| **Public boundary command** | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` |
| **Reader regression command** | `ctest --preset windows-msvc-debug-static -R tes4_bsa_reader --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **TES5Edit boundary command** | `pwsh -NoProfile -Command '$status = git -C TES5Edit status --short; if ($status) { throw "TES5Edit has unexpected changes: $status" }'` |
| **Estimated runtime** | Quick writer/public/reader slices target < 60s each in a configured C++20/vcpkg environment; full suite runtime is environment-dependent and must complete before verification. |

---

## Sampling Rate

- **After every task commit:** Run the task's listed automated command. Once `tests/unit/tes4_bsa_writer_tests.cpp` exists, prefer `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` for writer changes.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure` plus the TES5Edit boundary command.
- **Before `/gsd-verify-work`:** Full suite, public boundary, reader regression, writer label, and TES5Edit boundary commands must be green.
- **Max feedback latency:** No more than one task may land without a concrete automated command result or an explicitly recorded compiler/vcpkg environment blocker.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-01-02 | Public header forbidden-token scan blocks private dependency leakage. | public boundary RED | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | ✅ existing `tests/unit/public_include_boundary_tests.cpp` | ⬜ pending |
| 07-01-02 | 01 | 1 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-01-01/T-07-01-02/T-07-01-03 | Writer API exposes explicit paths, result-style fallibility, and no private codec/TES5Edit types. | public boundary GREEN | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | ❌ W0 creates `include/libbsa/writer.hpp` | ⬜ pending |
| 07-02-01 | 02 | 2 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-02-01/T-07-02-02/T-07-02-03 | RED tests cover copied memory ownership without extraction, duplicate canonical paths, invalid paths, overwrite guard, missing source, and explicit archive paths. | writer RED | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 creates `tests/unit/tes4_bsa_writer_tests.cpp` and registers it in `tests/CMakeLists.txt` | ⬜ pending |
| 07-02-02 | 02 | 2 | WBSA-01/WBSA-02/WBSA-03/WBSA-10 | T-07-02-01/T-07-02-02/T-07-02-03 | Writer validates paths, rejects duplicates at write time, owns memory bytes, and blocks default overwrite. | writer GREEN | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 creates `src/formats/bsa/tes4_bsa_writer.hpp` and `src/formats/bsa/tes4_bsa_writer.cpp` | ⬜ pending |
| 07-03-01 | 03 | 3 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-10 | T-07-03-01/T-07-03-02/T-07-03-03 | RED tests require all-raw v103/v104/v105 archives to reopen, list/find/contains/extract, preserve copied memory bytes, and expose content-derived flags. | round-trip RED | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 path `tests/unit/tes4_bsa_writer_tests.cpp` | ⬜ pending |
| 07-03-02 | 03 | 3 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-10 | T-07-03-01/T-07-03-02/T-07-03-03/T-07-03-04 | Serializer uses hash ordering, checked offset arithmetic, derived file flags, temp-file publish, and reader-backed raw round trips. | round-trip GREEN | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 path `src/formats/bsa/tes4_bsa_writer.cpp` | ⬜ pending |
| 07-04-01 | 04 | 4 | WBSA-01/WBSA-02/WBSA-03/WBSA-07/WBSA-10 | T-07-04-01/T-07-04-02/T-07-04-03 | RED tests require archive default compression, per-entry overrides, zero-byte raw behavior, and codec-private public surface. | compression RED | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 path `tests/unit/tes4_bsa_writer_tests.cpp` | ⬜ pending |
| 07-04-02 | 04 | 4 | WBSA-01/WBSA-02/WBSA-03/WBSA-07/WBSA-10 | T-07-04-01/T-07-04-02/T-07-04-03 | Writer routes v103/v104 to deflate, v105 to LZ4 frame, writes raw-size prefixes, and sets XOR toggle bits. | compression GREEN | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 path `src/formats/bsa/tes4_bsa_writer.cpp` | ⬜ pending |
| 07-05-01 | 05 | 5 | WBSA-02/WBSA-03/WBSA-08/WBSA-10 | T-07-05-01/T-07-05-02/T-07-05-03 | RED tests require embedded names absent by default, v104/v105 opt-in metadata, v103 compatibility, and extracted-byte stripping. | embedded-name RED | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 path `tests/unit/tes4_bsa_writer_tests.cpp` | ⬜ pending |
| 07-05-02 | 05 | 5 | WBSA-02/WBSA-03/WBSA-08/WBSA-10 | T-07-05-01/T-07-05-02/T-07-05-03 | Writer emits global target-compatible embedded-name prefixes only for v104/v105 and never exposes per-entry raw flag control. | embedded-name GREEN | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 path `src/formats/bsa/tes4_bsa_writer.cpp` | ⬜ pending |
| 07-06-01 | 06 | 6 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-07/WBSA-08/WBSA-09/WBSA-10 | T-07-06-01/T-07-06-02 | RED tests require dedupe disabled by default, opt-in shared offsets for identical final stored bytes, and negative cases for different stored encodings. | dedupe RED | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | ❌ W0 path `tests/unit/tes4_bsa_writer_tests.cpp` | ⬜ pending |
| 07-06-02 | 06 | 6 | WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-07/WBSA-08/WBSA-09/WBSA-10 | T-07-06-01/T-07-06-02/T-07-06-03/T-07-06-04 | Writer dedupes only byte-identical final stored payloads and passes writer, reader, public boundary, and TES5Edit gates. | final GREEN | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` + `ctest --preset windows-msvc-debug-static -R "tes4_bsa_reader|public_include_boundary" --output-on-failure` + TES5Edit boundary command | ❌ W0 paths `src/formats/bsa/tes4_bsa_writer.cpp`, `tests/unit/public_include_boundary_tests.cpp` | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Wave 0 is satisfied by the RED portions of Plans 01 and 02 before GREEN implementation proceeds. These are concrete paths, not placeholders:

- [ ] `tests/unit/public_include_boundary_tests.cpp` — add writer public-boundary compile checks for `tes4_bsa_target`, `archive_compression_policy`, `entry_compression_policy`, `tes4_bsa_writer_options`, `tes4_bsa_writer`, `add_file`, `add_bytes`, and `write_to` covering WBSA-01/WBSA-02/WBSA-03/WBSA-10.
- [ ] `include/libbsa/writer.hpp` — public writer contract path created by Plan 01 GREEN so subsequent tests can include the API.
- [ ] `tests/unit/tes4_bsa_writer_tests.cpp` — writer behavior test path created by Plan 02 RED; later plans extend this same file for WBSA-01/WBSA-02/WBSA-03/WBSA-05/WBSA-06/WBSA-07/WBSA-08/WBSA-09/WBSA-10.
- [ ] `tests/CMakeLists.txt` — register `unit/tes4_bsa_writer_tests.cpp` and ensure Catch tags include `[tes4_bsa_writer]` so `-L tes4_bsa_writer` selects the writer tests.
- [ ] `src/formats/bsa/tes4_bsa_writer.hpp` and `src/formats/bsa/tes4_bsa_writer.cpp` — private writer implementation paths created by Plan 02 GREEN and extended by later waves.

---

## Requirement Coverage Matrix

| Requirement | Plans | Automated Proof |
|-------------|-------|-----------------|
| WBSA-01 | 01, 02, 03, 04, 06 | Public writer target plus v103 disk/memory round-trip through `archive_reader`; `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` |
| WBSA-02 | 01, 02, 03, 04, 05, 06 | v104 disk/memory, deflate, embedded-name, and dedupe round trips; writer label command |
| WBSA-03 | 01, 02, 03, 04, 05, 06 | v105 disk/memory, LZ4-frame, embedded-name, and dedupe round trips; writer label command |
| WBSA-05 | 03, 06 | Multi-folder hash-sorted index tests reopened through `archive_reader`; writer label command |
| WBSA-06 | 03, 06 | Archive/file flag derivation tests with mesh/texture/script/menu/misc-style paths; writer label command |
| WBSA-07 | 04, 06 | Compression default and per-entry override metadata/extraction tests; writer label command |
| WBSA-08 | 05, 06 | Embedded-name enabled/disabled and v103 compatibility tests; writer label command |
| WBSA-09 | 06 | Dedupe disabled/enabled offset and negative stored-encoding tests; writer label command |
| WBSA-10 | 01, 02, 03, 04, 05, 06 | Pack/reopen/find/contains/extract/byte-compare plus public boundary and reader regression commands |

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None expected | WBSA-01..WBSA-10 | All Phase 7 behaviors are covered by generated synthetic data and automated round-trip tests. | N/A |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify commands or concrete Wave 0 file dependencies.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 covers all MISSING references with concrete file paths.
- [x] No watch-mode flags.
- [x] Feedback latency target defined; environment blockers must be recorded rather than ignored.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** ready for execution; commands require a configured C++20 compiler/vcpkg environment.
