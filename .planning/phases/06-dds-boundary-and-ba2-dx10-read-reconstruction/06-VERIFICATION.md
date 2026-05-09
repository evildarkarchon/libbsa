---
phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
verified: 2026-05-09T02:30:41Z
status: gaps_found
score: 7/9 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Malformed DX10 fixtures fail closed with stable error codes and complete required malformed coverage."
    status: partial
    reason: "Implemented malformed tests cover several stable error-code cases, but the plan-required duplicate canonical path and unsupported compression cases are absent from the generated malformed manifest and required-case assertions. Unknown manifest error strings also silently map to invalid_argument, which can mask fixture/test mistakes."
    artifacts:
      - path: "tests/unit/ba2_dx10_malformed_tests.cpp"
        issue: "Required-case list includes truncation, span, compression-corruption, decoded-size mismatch, mip gap, and duplicate mip-face cases only; duplicate canonical path and unsupported compression are missing. error_code_from_manifest falls through to invalid_argument for unknown strings."
      - path: "tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json"
        issue: "Manifest contains no duplicate canonical path case and no unsupported compression case."
    missing:
      - "Add generated malformed cases for duplicate canonical DX10 paths and unsupported compression routing."
      - "Assert those case IDs in ba2_dx10_malformed_tests.cpp."
      - "Make error_code_from_manifest fail the test on unknown error-code strings instead of returning invalid_argument."
  - truth: "BA2 DX10 open-time parsing is bounded and does not allocate the entire filename-table-to-payload gap."
    status: failed
    reason: "parse_ba2_dx10_archive_file computes first_payload_offset - FileTableOffset and reads that full gap into memory before parsing names; sparse or malicious archives can force very large allocations/reads. This matches 06-REVIEW CR-01 and contradicts the bounded-open parser contract."
    artifacts:
      - path: "src/formats/ba2/ba2_dx10_parser.cpp"
        issue: "Lines 506-515 calculate name_table_size_u64 from first payload offset and call read_file_bytes_at for the entire gap."
    missing:
      - "Parse exactly file_count length-prefixed names incrementally from FileTableOffset, stopping before first_payload_offset, instead of reading the whole gap."
---

# Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction Verification Report

**Phase Goal:** Consumers can inspect BA2 DX10/DDS texture metadata and extract entries as valid DDS files while DirectXTex remains an internal validation/analyzer dependency.
**Verified:** 2026-05-09T02:30:41Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can open Fallout 4 and Starfield BA2 DX10/DDS texture archives and inspect dimensions, mip count, DXGI-derived format data, cubemap/array information, and chunk layout through libbsa-owned types. | ✓ VERIFIED | `archive.cpp:138-147` dispatches `detected_ba2.value().is_dx10` to `parse_ba2_dx10_archive_file`; `ba2_dx10_parser_tests.cpp:130-194` opens FO4/SF v3 fixtures and checks texture fields/chunks. Public types are in `include/libbsa/archive.hpp:60-118`. |
| 2 | Consumer can extract BA2 DDS entries as valid DDS files with reconstructed headers and correct face/mip ordering for cubemaps. | ✓ VERIFIED | `ba2_dx10_reader.cpp:196-234` builds a DDS DXT10 header and writes chunks in parser-validated order; `ba2_dx10_extraction_tests.cpp:141-160` verifies sink/extract_bytes DDS streams and manifest order; `dds_layout.cpp:189-255` validates array/face/mip/source order. |
| 3 | Consumer can extract BA2 DDS chunks compressed with deflate or raw LZ4 block according to archive version and chunk metadata. | ✓ VERIFIED | `ba2_dx10_reader.cpp:122-154` routes deflate/LZ4-block through `decompress_payload_exact`; `ba2_dx10_extraction_tests.cpp:163-190` proves raw, deflate, and LZ4-block fixture routes. |
| 4 | Maintainer can validate reconstructed DDS outputs through DirectXTex metadata loading without exposing DirectXTex types in public headers. | ✓ VERIFIED | `directxtex_analyzer.cpp:26-59` calls `DirectX::GetMetadataFromDDSMemory` privately and translates to `texture_metadata`; `public_include_boundary_tests.cpp:48-75` forbids DirectXTex/DXGI/Windows/private tokens in public headers; `grep` over `include/libbsa` found no forbidden tokens. |
| 5 | Maintainer can generate legal FO4/SF v3 BA2 DX10 fixtures, rich manifests, and CTest labels outside TES5Edit. | ✓ VERIFIED | `generate_ba2_dx10_fixtures.cpp` exists; generated manifests declare DDS-01..DDS-07 and legal provenance; `ctest -N -R "ba2_dx10|dds_layout|public_include_boundary"` listed 18 relevant tests; `git -C TES5Edit status --short` was empty. |
| 6 | Library code can build deterministic DDS magic + DDS_HEADER + DXT10 extension bytes and validate chunk layout without a DirectXTex runtime extraction gate. | ✓ VERIFIED | `dds_layout.cpp:134-187` builds a 148-byte always-DXT10 header; `dds_layout.cpp:202-205` documents no DirectXTex runtime gate; `dds_layout_tests.cpp:41-201` covers constants, cubemap/array order, gaps, duplicates, impossible sizes, and unsupported formats. |
| 7 | Archive facade routes BA2 DX10 open/list/find/contains/extract separately from BA2 GNRL. | ✓ VERIFIED | `archive.cpp:206-235` selects DX10 vs GNRL helpers for list/find/contains; `archive.cpp:240-262` dispatches DX10 extraction to `extract_ba2_dx10_payload`. The key-link SDK miss for `is_dx10.*parse_ba2_dx10_archive_file` is a pattern limitation; manual evidence is `archive.cpp:138-140`. |
| 8 | Malformed DX10 fixtures fail closed with stable error codes and complete required malformed coverage. | ✗ FAILED | Stable-code malformed tests exist (`ba2_dx10_malformed_tests.cpp:55-98`), but required duplicate canonical path and unsupported compression cases from Plan 06-06 are absent from the manifest and required-case list. `error_code_from_manifest` returns `invalid_argument` for unknown strings instead of failing. |
| 9 | BA2 DX10 open-time parsing is bounded and does not allocate the whole filename-table-to-payload gap. | ✗ FAILED | `ba2_dx10_parser.cpp:506-515` reads `first_payload_offset - FileTableOffset` into memory as the filename table; 06-REVIEW CR-01 correctly identifies this as a sparse/malicious archive DoS risk. |

**Score:** 7/9 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/archive.hpp` | Public dependency-light texture metadata | ✓ VERIFIED | Defines `texture_chunk_metadata`, `texture_metadata`, and `std::optional<texture_metadata> texture` without forbidden public dependency tokens. |
| `src/texture/directxtex_analyzer.cpp` | Private DirectXTex metadata analyzer | ✓ VERIFIED | Includes DirectXTex privately and translates `TexMetadata` into libbsa-owned fields. |
| `src/texture/dds_layout.cpp` | DDS DXT10 header construction and layout validation | ✓ VERIFIED | SDK pattern missed literal `DDS_HEADER_DXT10`, but implementation writes DDS/DX10 constants and DXT10 extension fields; tests assert 148-byte header and DXT10 offsets. |
| `src/formats/ba2/ba2_dx10_parser.cpp` | BA2 DX10 parser | ✗ FAILED | Substantive and wired, but not bounded for sparse filename-table gaps (`name_table_size_u64` full-gap read). |
| `src/formats/ba2/ba2_dx10_reader.cpp` | BA2 DX10 extraction | ✓ VERIFIED | Header-first extraction, bounded raw chunk streaming, exact decompression, and LZ4-frame rejection exist. Review WR-01 is a warning: header is written before host file open, matching planned sink-first behavior but risking partial sink mutation if file vanished. |
| `tests/unit/ba2_dx10_malformed_tests.cpp` | Manifest-driven malformed coverage | ⚠️ PARTIAL | Tests stable error codes, but missing required duplicate canonical path and unsupported compression cases; unknown manifest error-code strings silently map to `invalid_argument`. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `tests/CMakeLists.txt` | `generate_ba2_dx10_fixtures.cpp` | generator executable/custom target | ✓ WIRED | SDK verified `generate_ba2_dx10_fixtures_tool`; relevant tests are discoverable. |
| `src/archive.cpp` | `ba2_dx10_parser.cpp` | byte-detected DX10 open dispatch | ✓ WIRED | Manual evidence: `archive.cpp:138-140` checks `is_dx10` and calls `parse_ba2_dx10_archive_file`. |
| `src/archive.cpp` | `ba2_dx10_reader.cpp` | DX10 extraction dispatch | ✓ WIRED | `archive.cpp:260` calls `extract_ba2_dx10_payload` for DX10 BA2 entries. |
| `ba2_dx10_parser.cpp` | `dds_layout.hpp` | `validate_and_order_chunks` | ✓ WIRED | Parser materializes public chunks in validated source order from `logical_texture_segment::source_chunk_index`. |
| `ba2_dx10_reader.cpp` | `compression_router.cpp` | exact decompression | ✓ WIRED | `decompress_payload_exact` used for compressed DX10 chunks. |
| `ba2_dx10_malformed_tests.cpp` | malformed manifest | manifest-driven assertions | ⚠️ PARTIAL | Manifest is loaded, but required cases are incomplete. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `entry_metadata::texture` | `texture_metadata` fields and chunks | BA2 DX10 records/chunk table parsed in `ba2_dx10_parser.cpp` | Yes | ✓ FLOWING — tests compare fixture manifests to opened archive metadata. |
| DX10 extraction bytes | DDS header + decoded chunks | `entry.texture->chunks` from parser and archive chunk payloads via `ifstream` | Yes | ✓ FLOWING — extraction tests compare decoded payload bytes and DirectXTex metadata. |
| DirectXTex validation | `texture_metadata` from DDS bytes | `GetMetadataFromDDSMemory` in private analyzer | Yes | ✓ FLOWING — tests call analyzer on extracted DDS bytes. |
| Malformed coverage matrix | manifest `cases` | generated malformed manifest | Partial | ⚠️ PARTIAL — matrix lacks duplicate canonical path and unsupported compression cases required by 06-06. |
| Filename table parser | name table bytes | Full gap between `FileTableOffset` and first payload | No bounded flow | ✗ HOLLOW/UNBOUNDED — reads arbitrary gap before parsing length-prefixed names. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| DX10/DDS tests are registered | `ctest --preset windows-msvc-debug-static -N -R "ba2_dx10|dds_layout|public_include_boundary"` | Listed 18 tests covering metadata, layout, extraction, compression, DirectXTex, malformed, and public boundary. | ✓ PASS |
| TES5Edit remains read-only | `git -C TES5Edit status --short` | No output. | ✓ PASS |
| Static/full CTest gate | Orchestrator-provided final gate | 92/92 passed; local game fixture skipped by policy. | ✓ PASS (trusted command result from verification context) |
| Shared CTest gate | Plan 06-06 summary and orchestrator context | Shared preset CTest passed after rebuild. | ✓ PASS (reported) |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| DDS-01 | 06-01, 06-04, 06-06 | Consumer can read Fallout 4 BA2 DX10/DDS texture archives. | ✓ SATISFIED | FO4 DX10 fixture opens in `ba2_dx10_parser_tests.cpp:130-142`. |
| DDS-02 | 06-01, 06-04, 06-06 | Consumer can read Starfield BA2 v3 DX10/DDS texture archives. | ✓ SATISFIED | Starfield v3 fixture opens and checks `compression_method == 3` in `ba2_dx10_parser_tests.cpp:144-155`. |
| DDS-03 | 06-02, 06-04, 06-06 | Consumer can inspect dimensions, mip count, DXGI format, cubemap/array info, and chunk layout. | ✓ SATISFIED | Public metadata fields exist in `archive.hpp`; manifest-backed metadata assertions in `ba2_dx10_parser_tests.cpp:94-126` and `157-194`. |
| DDS-04 | 06-03, 06-05, 06-06 | Consumer can extract BA2 DDS entries as valid DDS files with reconstructed headers. | ✓ SATISFIED | Header builder in `dds_layout.cpp`; extraction tests verify DDS prefix, payload, and `extract_bytes` equivalence. |
| DDS-05 | 06-05, 06-06 | Consumer can extract deflate/raw-LZ4 BA2 DDS chunks by archive metadata. | ✓ SATISFIED | `ba2_dx10_reader.cpp` exact decompression routing; `ba2_dx10_compression` test covers raw/deflate/LZ4-block. |
| DDS-06 | 06-02, 06-05, 06-06 | Maintainer can validate reconstructed DDS through DirectXTex without public DirectXTex types. | ✓ SATISFIED | Private analyzer uses DirectXTex; public include boundary tests and grep found no public leakage; extraction tests validate via analyzer. |
| DDS-07 | 06-03, 06-04, 06-05, 06-06 | Consumer can extract cubemap textures with correct DDS metadata and face/mip ordering. | ✓ SATISFIED | `validate_and_order_chunks` computes face/order/source identities; parser/extraction tests compare logical segments and payload order. |

All phase requirement IDs from PLAN frontmatter are accounted for: DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/formats/ba2/ba2_dx10_parser.cpp` | 506-515 | Reads filename-table gap as one vector before parsing names | 🛑 Blocker | Untrusted sparse/malicious archive can force large allocation/read during `open`, violating bounded-open parser expectations. |
| `tests/unit/ba2_dx10_malformed_tests.cpp` | 31-39 | Unknown manifest error-code fallback to `invalid_argument` | ⚠️ Warning | Typos in malformed manifests can silently change expected results instead of failing tests. |
| `tests/unit/ba2_dx10_malformed_tests.cpp` | 86-94 | Required malformed-case list omits duplicate canonical path and unsupported compression | 🛑 Blocker | Plan-required malformed coverage can disappear without test failure. |
| `src/formats/ba2/ba2_dx10_reader.cpp` | 202-215 | Header is written before host archive is opened | ⚠️ Warning | If the file disappears after `open`, caller sink can receive a partial DDS header before `io_error`; advisory review WR-01. |
| `tests/fixtures/generated/*dx10*manifest.json` | various | `sha256_placeholder` informational fields | ℹ️ Info | Tests use `bytes_hex`; not blocking DDS reconstruction validation. |

### Human Verification Required

None. The phase is code/library behavior with fixture-backed tests; remaining issues are directly observable in code and test manifests.

### Gaps Summary

Most core DDS read/reconstruction behavior is implemented and tested: FO4/SF v3 DX10 fixtures open by bytes, texture metadata is public and dependency-light, extraction reconstructs DDS DXT10 streams, DirectXTex validation is private, and codec routing covers raw/deflate/raw-LZ4-block.

However, the phase should not pass yet. Two must-have gaps remain:

1. **Bounded DX10 open parsing is not achieved.** The parser reads the whole gap between `FileTableOffset` and first payload as the filename table. This is a blocker because a sparse/malicious archive can force large allocation/read during metadata inspection.
2. **Malformed closeout coverage is incomplete.** The final malformed manifest/tests do not include duplicate canonical path and unsupported compression cases required by Plan 06-06, and unknown expected error-code names do not fail the test.

---

_Verified: 2026-05-09T02:30:41Z_
_Verifier: the agent (gsd-verifier)_
