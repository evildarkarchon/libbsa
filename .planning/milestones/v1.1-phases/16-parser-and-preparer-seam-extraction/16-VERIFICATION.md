---
phase: 16-parser-and-preparer-seam-extraction
verified: 2026-05-14T23:43:30Z
status: passed
score: 9/9 must-haves verified
overrides_applied: 0
---

# Phase 16: Parser and Preparer Seam Extraction Verification Report

**Phase Goal:** The targeted TES4 parser and BA2 DX10 preparer hotspots become smaller internal seams that are safer to change because behavior is locked down by focused regression coverage.
**Verified:** 2026-05-14T23:43:30Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Maintainer can modify the targeted TES4 parser hotspot through smaller internal helpers with focused regression coverage catching behavior drift. | ✓ VERIFIED | `tes4_bsa_parser.cpp` includes `tes4_bsa_table.hpp` and `tes4_bsa_payload_descriptor.hpp`; `parse_tes4_bsa_archive_impl` delegates to `read_tes4_bsa_raw_table`, then materializes entries via `make_tes4_bsa_payload_descriptor`. Focused `[parser_preparer_seam]` TES4 tests passed. |
| 2 | Maintainer can change TES4 table/header parsing without editing payload descriptor rules. | ✓ VERIFIED | `tes4_bsa_table.hpp/.cpp` own checked header/table sizing, folder records/blocks, and filename parsing; payload functions are isolated in `tes4_bsa_payload_descriptor.hpp/.cpp`. |
| 3 | Maintainer can change TES4 embedded-name/raw-size/payload-span rules without editing raw table parsing. | ✓ VERIFIED | `tes4_bsa_payload_descriptor` owns compression, embedded prefix, raw-size prefix reads, stored payload sizing, and metadata-overlap rejection; raw table code returns only raw table structures and names. |
| 4 | Existing TES4 success and malformed fixture behavior remains unchanged. | ✓ VERIFIED | Affected suite `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"` passed 106/106, including TES4 malformed, extraction, metadata, and writer regression tests. Full debug suite passed 390/390. |
| 5 | Maintainer can modify the targeted BA2 DX10 preparer and staging hotspot through smaller internal helpers with focused regression coverage catching behavior drift. | ✓ VERIFIED | `ba2_dx10_prepare.cpp` delegates snapshot creation to `ba2_dx10_build_writer_entry_snapshot` and chunk work to `ba2_dx10_assemble_chunk` / `ba2_dx10_assemble_planned_entry`. Focused BA2 `[parser_preparer_seam]` tests passed. |
| 6 | Maintainer can change BA2 DX10 DDS snapshot staging without editing chunk compression assembly. | ✓ VERIFIED | `ba2_dx10_snapshot_builder.hpp/.cpp` own DDS host-file load, texture analysis handoff, target validation, snapshot directory reservation, and snapshot file writes; chunk assembly/compression is in a separate seam. |
| 7 | Maintainer can change BA2 DX10 chunk plan/assembly/compression rules without editing DDS host-file load and snapshot creation. | ✓ VERIFIED | `ba2_dx10_chunk_assembler.hpp/.cpp` own planned chunk collection, streamed snapshot byte assembly, raw-size validation, `compress_payload` routing, and indexed work-result placement. |
| 8 | BA2 DX10 snapshot immutability, chunk ordering, compression routing, and pre-publish failure behavior remain unchanged. | ✓ VERIFIED | `ba2_dx10_preparer_seam_tests.cpp` directly tests snapshot immutability, target validation, multi-mip/array/cubemap ordering, missing/truncated snapshot failure preserving a sentinel output, and Fallout 4/Starfield v3 compression routing. Affected suite passed 106/106. |
| 9 | Maintainer gets source-policy failures if TES4 or BA2 DX10 responsibilities collapse back into broad mixed coordinators. | ✓ VERIFIED | `parser_preparer_seam_policy_tests.cpp` reads source files from `LIBBSA_SOURCE_DIR`, requires private seam evidence, and forbids direct collapse tokens in `parse_tes4_bsa_archive_impl`, `ba2_dx10_prepare_chunk`, and `ba2_dx10_prepare_entries`. Policy label passed 2/2. |

**Score:** 9/9 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `src/formats/bsa/tes4_bsa_table.hpp` | Private raw table bundle seam | ✓ VERIFIED | Declares `tes4_bsa_header_fields`, `tes4_bsa_raw_table`, `read_tes4_bsa_header`, `tes4_bsa_metadata_table_size`, and `read_tes4_bsa_raw_table`; excludes final `entry_metadata` APIs. |
| `src/formats/bsa/tes4_bsa_payload_descriptor.hpp` | Private payload descriptor seam | ✓ VERIFIED | Declares `tes4_bsa_payload_descriptor`, compression/embedded-name helpers, stored payload validation, and callback-backed descriptor construction. `gsd-sdk verify.artifacts` missed the exact spaced phrase, but manual inspection verifies the required role. |
| `tests/unit/tes4_bsa_parser_seam_tests.cpp` | Focused TES4 seam regression coverage | ✓ VERIFIED | Registered test file contains `[parser_preparer_seam]` tests for table sizing, folder/file-name offsets, embedded prefix, raw size, compression interpretation, and metadata overlap rejection. |
| `src/formats/ba2/ba2_dx10_snapshot_builder.hpp` | Private snapshot builder seam | ✓ VERIFIED | Declares target-format validation, snapshot directory ensure, and DDS source snapshot entry construction without exposing DirectXTex public API. |
| `src/formats/ba2/ba2_dx10_chunk_assembler.hpp` | Private plan/assemble/compression seam | ✓ VERIFIED | Declares `ba2_dx10_assemble_chunk` and `ba2_dx10_assemble_planned_entry` consuming snapshot handles and `texture::planned_texture_chunk`. |
| `tests/unit/ba2_dx10_preparer_seam_tests.cpp` | Focused BA2 DX10 preparer seam coverage | ✓ VERIFIED | Contains `[parser_preparer_seam]` tests for snapshot immutability, streamed chunk assembly, array/cubemap ordering, failure paths, and compression routing. |
| `tests/unit/parser_preparer_seam_policy_tests.cpp` | Role-based source guard | ✓ VERIFIED | Contains `[parser_preparer_seam_policy]`; reads TES4/BA2 source files and checks seam evidence plus negative collapse invariants. |
| `tests/CMakeLists.txt` | Test discovery registration | ✓ VERIFIED | Lists `unit/ba2_dx10_preparer_seam_tests.cpp`, `unit/parser_preparer_seam_policy_tests.cpp`, and `unit/tes4_bsa_parser_seam_tests.cpp` in `libbsa_tests`. |
| `CMakeLists.txt` | Library source registration | ✓ VERIFIED | `libbsa_library_sources` includes `tes4_bsa_table.cpp`, `tes4_bsa_payload_descriptor.cpp`, `ba2_dx10_snapshot_builder.cpp`, and `ba2_dx10_chunk_assembler.cpp`. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `tes4_bsa_parser.cpp` | `tes4_bsa_table.hpp` | Parser coordinator calls raw table seam before materializing entries | ✓ WIRED | Include plus `read_tes4_bsa_raw_table(...)` call in `parse_tes4_bsa_archive_impl`. |
| `tes4_bsa_parser.cpp` | `tes4_bsa_payload_descriptor.hpp` | Entry materialization asks payload descriptor seam for compression/raw-size/span facts | ✓ WIRED | Include plus `make_tes4_bsa_payload_descriptor(...)` call in `materialize_entries`. |
| `ba2_dx10_prepare.cpp` | `ba2_dx10_snapshot_builder.hpp` | Entry creation delegates DDS snapshot creation | ✓ WIRED | Include plus `ba2_dx10_build_writer_entry_snapshot(...)` call in `ba2_dx10_make_writer_entry`. |
| `ba2_dx10_prepare.cpp` | `ba2_dx10_chunk_assembler.hpp` | Chunk and entry preparation delegate to assembler seam | ✓ WIRED | Include plus `ba2_dx10_assemble_chunk(...)` and `ba2_dx10_assemble_planned_entry(...)` calls. |
| `parser_preparer_seam_policy_tests.cpp` | `tes4_bsa_parser.cpp` | Source-reading negative invariant | ✓ WIRED | Test reads parser source and extracts `parse_tes4_bsa_archive_impl` body. |
| `parser_preparer_seam_policy_tests.cpp` | `ba2_dx10_prepare.cpp` | Source-reading negative invariant | ✓ WIRED | Test reads preparer source and extracts coordinator bodies. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `tes4_bsa_parser.cpp` + `tes4_bsa_table` | `tes4_bsa_raw_table` | `read_tes4_bsa_raw_table(table_bytes, archive_size, detected)` parses archive bytes through `binary_reader` | Yes | ✓ FLOWING |
| `tes4_bsa_parser.cpp` + `tes4_bsa_payload_descriptor` | `tes4_bsa_payload_descriptor` | Raw file records plus payload prefix callback reading archive/host file bytes | Yes | ✓ FLOWING |
| `ba2_dx10_prepare.cpp` + snapshot builder | `ba2_dx10_writer_entry` | Resolved DDS host file bytes, `texture::analyze_dds_source`, snapshot files | Yes | ✓ FLOWING |
| `ba2_dx10_prepare.cpp` + chunk assembler | `ba2_dx10_prepared_chunk` / `ba2_dx10_prepared_entry` | Snapshot files streamed by `detail::for_each_host_file_chunk`, planned chunks from `texture::plan_dx10_chunks`, compression router | Yes | ✓ FLOWING |
| `parser_preparer_seam_policy_tests.cpp` | Source text under `LIBBSA_SOURCE_DIR` | Runtime file reads of actual repository sources | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused seam and policy tests pass | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam|parser_preparer_seam_policy"` | 8/8 tests passed | ✓ PASS |
| Affected parser/preparer public suites remain stable | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"` | 106/106 tests passed | ✓ PASS |
| Full debug suite remains green | `ctest --preset windows-msvc-debug-static --output-on-failure` | 390/390 tests passed; two opt-in fixture tests skipped | ✓ PASS |
| ASan hardening lane exposes focused seam tests | `ctest --preset windows-msvc-asan-static --output-on-failure -L "parser_preparer_seam|parser_preparer_seam_policy"` | 8/8 tests passed | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
|---|---|---|---|
| Conventional probes | Not run | Step 7c skipped: no `scripts/*/tests/probe-*.sh` probes are declared for this C++ refactor phase. | SKIPPED |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| REFA-01 | `16-01-PLAN.md`, `16-03-PLAN.md` | Maintainer can modify the targeted TES4 parser hotspot through smaller internal helpers with focused regression coverage. | ✓ SATISFIED | Private table and payload descriptor seams exist, are wired into parser coordinator, have direct seam tests, policy guardrails, and affected TES4 suite coverage. |
| REFA-02 | `16-02-PLAN.md`, `16-03-PLAN.md` | Maintainer can modify the targeted BA2 DX10 preparer/staging hotspot through smaller internal helpers with focused regression coverage. | ✓ SATISFIED | Private snapshot builder and chunk assembler seams exist, are wired into preparer coordinator, have direct seam tests, policy guardrails, and affected BA2 DX10/writer-stage coverage. |

No orphaned Phase 16 requirements were found in `.planning/REQUIREMENTS.md`; REFA-01 and REFA-02 are both mapped to Phase 16 and marked complete. Later Phase 17 requirements (`DEDU-01`, `DEDU-02`, `DX10-01`, `DX10-02`) remain pending and are outside this phase.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| Phase 16 modified files | n/a | TODO/FIXME/XXX/HACK/PLACEHOLDER/not-implemented scan | None | No blocking debt markers or placeholders found in targeted phase files. Grep matches for `return {}` were normal `result<void>` success returns, not empty implementations. |

### Supporting Context

- Advisory review artifact `16-REVIEW.md` is clean with `findings_count: 0`; it reviewed the Phase 16 seam, test, CMake, and post-wave fix surfaces.
- Post-wave fix commit `70b963f` exists (`fix: resolve post-merge conflicts from wave 2`) and touches `tests/unit/host_file_writer_name_tests.cpp`; it does not undermine the parser/preparer seam evidence.
- Working tree was clean before this verification file was written.

### Human Verification Required

None. The phase goal is internal C++ seam extraction and regression coverage; file-level wiring, role separation, CTest behavior, and requirements coverage were verifiable programmatically.

### Gaps Summary

No blocking gaps found. The targeted TES4 parser and BA2 DX10 preparer hotspots are split into private, substantive, wired seams; regression and policy tests are registered and passing; affected public fixture behavior remains stable.

---

_Verified: 2026-05-14T23:43:30Z_
_Verifier: the agent (gsd-verifier)_
