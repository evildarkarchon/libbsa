---
phase: 09-ba2-dx10-write-new-support
verified: 2026-05-09T00:00:00Z
status: gaps_found
score: 3/4 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Writer can split textures into compatible mip/chunk records with configurable chunk limits and per-chunk compression."
    status: partial
    reason: "Normal fixture-backed planning and writer output pass, but block-compressed mip-size arithmetic is not fail-closed for hostile uint32 dimensions because width+3/height+3 are computed in uint32 before promotion."
    artifacts:
      - path: "src/texture/dds_layout.cpp"
        issue: "described_mip_size lines 90-96 use (width + 3U) / 4U and (height + 3U) / 4U for BC formats; this can wrap for untrusted metadata near UINT32_MAX and understate expected chunk raw_size."
      - path: "tests/unit/dds_layout_tests.cpp"
        issue: "No regression test covers oversized block-compressed dimensions or proves validate_and_order_chunks rejects the wrapped-size case."
    missing:
      - "Promote width/height to uint64 before adding block_width - 1/block_height - 1 and use descriptor.block_width/block_height instead of hard-coded 4U."
      - "Add a malformed/hostile DDS layout regression with UINT32_MAX-scale BC dimensions proving mip_size_for_format/validate_and_order_chunks fail closed or compute the correct uint64 size."
---

# Phase 09: BA2 DX10 Write-New Support Verification Report

**Phase Goal:** Consumers can create Fallout 4 and Starfield BA2 DX10/DDS texture archives from DDS files with valid mip/chunk metadata and extraction-preserving output.
**Verified:** 2026-05-09T00:00:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can create Fallout 4 and Starfield BA2 DX10/DDS texture archives from DDS inputs. | ✓ VERIFIED | `include/libbsa/writer.hpp` defines `ba2_dx10_target`, `ba2_dx10_writer_options`, and `ba2_dx10_writer` with DDS-host-file `add_file` and `write_to`; `src/formats/ba2/ba2_dx10_writer.cpp` implements add/write; focused CTest passed FO4, Starfield method 3, and Starfield method 0 writer round trips. |
| 2 | Writer can analyze DDS input through DirectXTex internally and expose only library-owned texture metadata. | ✓ VERIFIED | `src/texture/directxtex_analyzer.cpp` uses private `DirectX::LoadFromDDSMemory`; `src/texture/directxtex_analyzer.hpp` exposes `dds_source_analysis` with libbsa-owned metadata/vectors and no DirectX/DXGI public types; public boundary tests passed. |
| 3 | Writer can split textures into compatible mip/chunk records with configurable chunk limits and per-chunk compression. | ✗ FAILED | Normal `dds_layout` and writer tests pass, but advisory review blocker is confirmed in code: `src/texture/dds_layout.cpp:90-96` computes BC block counts using `(width + 3U)` / `(height + 3U)` in uint32, so hostile dimensions can wrap before uint64 sizing. This violates fail-closed checked sizing for mip/chunk records. |
| 4 | Maintainer can pack, reopen, extract, and byte-compare or metadata-validate BA2 DDS writer output against source DDS files. | ✓ VERIFIED | `tests/unit/ba2_dx10_writer_tests.cpp` reopens via `archive_reader::open`, validates list/find/contains, extracts through both `payload_sink` and `extract_bytes`, and compares DirectXTex-loaded metadata plus `image_payload_bytes`; focused writer tests passed. |

**Score:** 3/4 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/writer.hpp` | Public dependency-light BA2 DX10 writer API | ✓ VERIFIED | Defines target/options/writer and no DX10 `add_bytes` or per-entry/chunk compression override. |
| `src/texture/directxtex_analyzer.hpp/.cpp` | Private DDS source analysis and snapshots | ✓ VERIFIED | Header exposes libbsa-owned structs; `.cpp` privately includes DirectXTex and copies DDS/subresource bytes. |
| `src/texture/dds_layout.hpp/.cpp` | Locked-format sizing and chunk planning | ✗ BLOCKER | Planner exists and tests pass for normal cases, but BC block count arithmetic can wrap for hostile dimensions. |
| `src/formats/ba2/ba2_dx10_writer.hpp/.cpp` | Writer state, serialization, compression, dedupe, safe publish | ✓ VERIFIED | Implements add-time validation, chunk planning, compression routing, BA2 DX10 records/sentinel, dedupe, and unique temp publish. |
| `tests/fixtures/generated/source/*` | Legal committed DDS source fixture matrix | ✓ VERIFIED | Manifest covers locked format IDs plus structural multi-mip/array/cubemap cases and invalid source cases. |
| `tests/unit/ba2_dx10_writer_tests.cpp` | Reader-backed writer, DDS, dedupe, publish tests | ⚠️ WARNING | Substantive coverage exists and passes; rollback test currently checks source-code strings rather than exercising rollback behavior (review WR-02). |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `tests/unit/public_include_boundary_tests.cpp` | `include/libbsa/writer.hpp` | static_assert/requires public API checks | ✓ VERIFIED | Manual check: lines 70-78 assert DX10 target/options/add/write surface. `gsd-sdk` pattern missed because the symbols span lines. |
| `src/texture/directxtex_analyzer.cpp` | `src/texture/directxtex_analyzer.hpp` | `result<dds_source_analysis>` | ✓ VERIFIED | Declared in header, implemented with `LoadFromDDSMemory`. |
| `src/formats/ba2/ba2_dx10_writer.cpp` | `src/texture/directxtex_analyzer.cpp` | `texture::analyze_dds_source` during add_file | ✓ VERIFIED | `make_entry` calls analyzer before pushing writer-owned entry state. |
| `src/formats/ba2/ba2_dx10_writer.cpp` | `src/detail/compression_router.cpp` | `detail::compress_payload` | ✓ VERIFIED | Compression selected by target/options metadata: FO4 deflate, Starfield method 3 LZ4 block, method 0 deflate. |
| `tests/unit/ba2_dx10_writer_tests.cpp` | `src/formats/ba2/ba2_dx10_reader.cpp` | `archive_reader::open` + `extract` + `extract_bytes` | ✓ VERIFIED | Reader-backed tests validate both extraction paths and metadata. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `ba2_dx10_writer::add_file` | `ba2_dx10_writer_entry::source/dds_bytes` | Host DDS file read then `texture::analyze_dds_source` | Yes | ✓ FLOWING |
| `write_ba2_dx10_archive` | `prepared_entry/chunks` | Writer-owned source metadata + `plan_dx10_chunks` + `compress_payload` | Yes for normal inputs | ⚠️ PARTIAL due unchecked BC dimension wrap in upstream size math |
| Writer tests | Extracted DDS bytes | `archive_reader::open` then sink/extract_bytes | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused writer/layout/public boundary suites | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer|dds_layout|public_include_boundary" --output-on-failure` | 21/21 tests passed | ✓ PASS |
| DX10 dedupe and publish suites | `ctest --preset windows-msvc-debug-static -R "BA2 DX10 writer.*(deduplicate|dedupe|publish|overwrite|temp|non-regular)" --output-on-failure` | 6/6 tests passed | ✓ PASS |
| TES5Edit cleanliness | `git -C TES5Edit status --short` | Empty output | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| WBA2-06 | 09-01, 09-05 | Consumer can create new Fallout 4 BA2 DX10/DDS texture archives from DDS files. | ✓ SATISFIED | Public `ba2_dx10_target::fallout4`; FO4 writer round-trip test passed. |
| WBA2-07 | 09-01, 09-05 | Consumer can create new Starfield BA2 v3 DX10/DDS texture archives from DDS files. | ✓ SATISFIED | Public `ba2_dx10_target::starfield_v3`; Starfield method 3/method 0 round-trip tests passed. |
| WBA2-08 | 09-02, 09-04 | Writer can analyze DDS input through DirectXTex and generate BA2 texture records from library-owned metadata. | ✓ SATISFIED | `analyze_dds_source` uses DirectXTex privately and writer derives records from `texture_metadata`. |
| WBA2-09 | 09-03 | Writer can split DDS textures into compatible mip/chunk records with configurable chunk limits. | ✗ BLOCKED | Normal planner behavior exists, but block-compressed mip-size arithmetic can wrap for hostile dimensions, undermining checked compatible sizing. |
| WBA2-10 | 09-05, 09-06 | Writer can apply per-chunk compression and serialize chunk metadata so extracted DDS output remains valid. | ✓ SATISFIED with warning | Writer serializes compressed chunks and tests extract valid DDS; warning remains that invalid dimension metadata can be under-validated in shared layout code. |
| WBA2-11 | 09-02, 09-05, 09-06 | Maintainer can round-trip BA2 writer output by packing, reopening, extracting, and byte-comparing or metadata-validating source files. | ✓ SATISFIED | Reader-backed tests compare metadata and payload bytes through both extraction paths. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/texture/dds_layout.cpp` | 93-96 | uint32 pre-promotion addition in BC block count: `(width + 3U) / 4U` and `(height + 3U) / 4U` | 🛑 Blocker | Can accept impossible raw chunk sizes for hostile dimensions instead of failing closed. |
| `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` | 400-404 | Fixture `name_hash` uses full canonical path, not filename only | ⚠️ Warning | Generated fixture metadata can encode the wrong convention while parser tests still pass. Does not block writer output because writer hashes filename/directory separately. |
| `tests/unit/ba2_dx10_writer_tests.cpp` | 644-652 | Rollback test searches source text for hook names | ⚠️ Warning | Test can pass without exercising overwrite rollback behavior. Implementation contains rollback code, but behavioral fault-injection coverage is incomplete. |

### Human Verification Required

None. The gap is directly observable in code and does not need visual/manual product testing.

### Gaps Summary

Phase 09 delivers the public DX10 writer surface, add-time DDS validation/snapshotting, normal chunk planning, compressed FO4/Starfield serialization, optional dedupe, safe publish basics, and reader-backed extraction proof. However, the advisory review's blocker is confirmed: shared DDS layout sizing still performs block-compressed `(dimension + 3)` arithmetic in `uint32_t`, which can wrap for hostile metadata before checked `uint64_t` multiplication. Because Phase 09's chunk planning contract explicitly depends on checked compatible mip sizing, this blocks goal completion until the arithmetic and regression coverage are fixed.

---

_Verified: 2026-05-09T00:00:00Z_
_Verifier: the agent (gsd-verifier)_
