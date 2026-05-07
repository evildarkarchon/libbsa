---
phase: 10
slug: ba2-writers
status: draft
nyquist_compliant: true
wave_0_complete: true
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
| **Quick run command** | `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa.public_header_smoke"` |
| **Full suite command** | `ctest --preset windows-msvc-vcpkg --output-on-failure` or `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~60 seconds for focused tests; full suite depends on local build state and is end-of-phase validation, not per-task sampling |

---

## Sampling Rate

- **After every task commit:** Run the task's focused `<automated>` command. Prefer the quick command for focused BA2 writer, BA2 reader, BA2 DDS reader, and public-header smoke coverage when the task touches shared BA2 or public-header paths.
- **After every plan wave:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa_writer_tests|libbsa_bsa_writer_tests|libbsa.public_header_smoke"`.
- **Before `/gsd-verify-work`:** Full suite must be green, private-token public-header negative grep must be clean, CMake no-glob and no-`TES5Edit` negative greps must be clean, stale BA2 writer placeholder negative grep must be clean, and `git status --short TES5Edit` must be empty.
- **Max feedback latency:** 60 seconds for focused BA2 writer checks.
- **End-of-phase exception:** Full-suite CTest and repository-wide boundary gates are retained as final validation even if they exceed the focused-task feedback target.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 10-01-01 | 10-01 Task 1 | 1 | WRT-02/WRT-03 | T-10-01 | Public BA2 writer contracts expose only libbsa-owned types and reject private dependency leakage | build/smoke/static | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_public_header_smoke` | planned create/update: `include/libbsa/ba2_writer.hpp` | complete - PASS |
| 10-01-02 | 10-01 Task 2 | 1 | WRT-02/WRT-03 | T-10-02/T-10-03 | Placeholder implementation and explicit CMake wiring avoid TES5Edit inclusion and fail structurally until behavior is implemented | unit/build | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | planned create/update: `src/ba2_writer.cpp`, `tests/ba2_writer_tests.cpp`, `CMakeLists.txt` | complete - PASS |
| 10-02-01 | 10-02 Task 1 | 2 | WRT-02 | T-10-04/T-10-05 | RED tests lock native GNRL headers, FileTableOffset/name table, raw `PackedSize == 0`, and read-after-write behavior | unit/fixture/roundtrip | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | planned update: `tests/ba2_writer_tests.cpp`, `include/libbsa/ba2_writer.hpp` | complete - PASS |
| 10-02-02 | 10-02 Task 2 | 2 | WRT-02 | T-10-04/T-10-05/T-10-06 | GNRL planner uses checked native layout arithmetic and finalizer streams plan-owned bytes with first sink error propagation | unit/fixture/roundtrip | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | planned update: `src/ba2_writer.cpp`, `include/libbsa/ba2_writer.hpp`, `tests/ba2_writer_tests.cpp` | complete - PASS |
| 10-03-01 | 10-03 Task 1 | 3 | WRT-02 | T-10-07/T-10-08/T-10-09 | RED tests lock disk-input ownership, explicit compression routing, dedup sharing, invalid inputs, and sink failures | unit/fixture/io/codec | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | planned update: `tests/ba2_writer_tests.cpp` | complete - PASS |
| 10-03-02 | 10-03 Task 2 | 3 | WRT-02 | T-10-07/T-10-08/T-10-09 | GNRL disk planning validates archive paths before host I/O, routes raw/deflate/LZ4 without fallback, and dedups only exact post-policy bytes | unit/fixture/io/codec/roundtrip | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_compression_policy_tests"` | planned update: `src/ba2_writer.cpp`, `tests/ba2_writer_tests.cpp` | complete - PASS |
| 10-04-01 | 10-04 Task 1 | 4 | WRT-03 | T-10-10/T-10-11/T-10-12 | RED tests lock DDS memory analysis, DX10 layout, target-rule chunk previews, and private validation behavior | unit/fixture/roundtrip | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | planned update: `tests/ba2_writer_tests.cpp`, `CMakeLists.txt` | complete - PASS |
| 10-04-02 | 10-04 Task 2 | 4 | WRT-03 | T-10-10/T-10-11/T-10-12 | DDS analyzer keeps DirectXTex private, derives target-rule chunks, and fails unsupported/malformed inputs before any partial plan | unit/fixture/roundtrip/static | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_dds_reader_tests"` | planned create/update: `src/texture/dds_analysis.hpp`, `src/texture/dds_analysis.cpp`, `src/ba2_writer.cpp`, `CMakeLists.txt`, `tests/ba2_writer_tests.cpp` | complete - PASS |
| 10-05-01 | 10-05 Task 1 | 5 | WRT-03 | T-10-13/T-10-14/T-10-15 | RED tests lock DDS disk ownership, raw/deflate/LZ4 chunk semantics, chunk dedup, malformed-input failures, and sink failures | unit/fixture/io/codec | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` | planned update: `tests/ba2_writer_tests.cpp` | complete - PASS |
| 10-05-02 | 10-05 Task 2 | 5 | WRT-03 | T-10-13/T-10-14/T-10-15 | DDS disk planning validates paths before I/O, compresses chunks through explicit routes, dedups exact stored chunks, and rejects unsafe DDS structurally | unit/fixture/io/codec/roundtrip | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_ba2_writer_tests && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_dds_reader_tests|libbsa_compression_policy_tests"` | planned update: `src/ba2_writer.cpp`, `src/texture/dds_analysis.cpp`, `tests/ba2_writer_tests.cpp` | complete - PASS |
| 10-06-01 | 10-06 Task 1 | 6 | WRT-02/WRT-03 | T-10-16 | Public smoke creates/finalizes/reopens/extracts FO4 and Starfield GNRL and DDS archives using only public headers | smoke/roundtrip | `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_public_header_smoke && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | planned update: `tests/public_header_smoke.cpp` | complete - PASS |
| 10-06-02 | 10-06 Task 2 | 6 | WRT-02/WRT-03 | T-10-17 | README and validation describe exact supported BA2 writer targets and Phase 10 deferred boundaries without overclaiming | docs/regression | `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa_writer_tests|libbsa_bsa_writer_tests|libbsa.public_header_smoke"` | planned update: `README.md`, `10-VALIDATION.md` | complete - PASS |
| 10-06-03 | 10-06 Task 3 | 6 | WRT-02/WRT-03 | T-10-16/T-10-17/T-10-18 | End-of-phase fail-fast gates prove full tests, public-header boundary, explicit CMake wiring, no stale placeholders, and untouched TES5Edit | full-suite/end-of-phase/static/boundary | `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; rg -n "DirectXTex|DXGI_FORMAT|Windows\.h|libdeflate|TES5Edit|lz4\.h|lz4frame\.h|LZ4_" include/libbsa; if ($LASTEXITCODE -ne 1) { exit 1 }; rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt; if ($LASTEXITCODE -ne 1) { exit 1 }; rg -n "^[^#]*TES5Edit" CMakeLists.txt; if ($LASTEXITCODE -ne 1) { exit 1 }; rg -n "BA2 (GNRL|DDS) writer planning is not implemented|BA2 writer finalization is not implemented" src/ba2_writer.cpp tests/ba2_writer_tests.cpp tests/public_header_smoke.cpp; if ($LASTEXITCODE -ne 1) { exit 1 }; $tes5 = git status --short TES5Edit; if ($LASTEXITCODE -ne 0 -or $tes5) { $tes5; exit 1 }` | planned update: `10-VALIDATION.md` | pending final gate |

---

## Wave 0 Requirements

- [x] `include/libbsa/ba2_writer.hpp` - planned in 10-01 Task 1 with automated build/static checks.
- [x] `src/ba2_writer.cpp` - planned across 10-01 through 10-05 with focused writer/reader/codec checks.
- [x] `src/texture/dds_analysis.hpp` and `src/texture/dds_analysis.cpp` - planned in 10-04 Task 2 with private DirectXTex boundary checks.
- [x] `tests/ba2_writer_tests.cpp` - planned across 10-01 through 10-05 for layout, read-after-write, disk/memory, DDS, compression, dedup, and failure coverage.
- [x] `tests/public_header_smoke.cpp` update - planned in 10-06 Task 1 with public-header smoke CTest.
- [x] `CMakeLists.txt` update - planned in 10-01 and 10-04, with final no-glob/no-`TES5Edit` negative grep in 10-06 Task 3.
- [x] DDS reconstruction format coverage review - planned in 10-04/10-05: generated fixture formats must be supported privately, while unsupported formats fail structurally during planning.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| External Archive2/BSArchPro corpus parity | WRT-02/WRT-03 | Phase 11 owns broad external corpus validation after production BA2 writers exist | Do not gate Phase 10 on external corpus parity; generated read-after-write is sufficient here. |

---

## Validation Sign-Off

- [x] All 13 submitted tasks have `<automated>` verify commands
- [x] Sampling continuity: no task lacks automated verify, so no 3 consecutive tasks can go unverified
- [x] Wave 0 gaps are mapped to concrete plan/task IDs before execution
- [x] No watch-mode flags
- [x] Focused task feedback target remains < 60s where practical; full-suite validation is explicitly end-of-phase only
- [x] `nyquist_compliant: true` set in frontmatter because this map covers all six plans and thirteen tasks

**Approval:** ready for execution; task statuses remain pending until `/gsd-execute-phase 10` records command evidence.

## Execution Evidence

- **10-06 Task 1 public smoke:** `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_public_header_smoke && ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` — PASS; 1/1 smoke test passed.
- **10-06 Task 2 focused regression lane:** `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa_writer_tests|libbsa_bsa_writer_tests|libbsa.public_header_smoke"` — PASS; 110/110 tests passed.
