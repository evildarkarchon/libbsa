# Phase 7: BA2 DDS Read and DDS Reconstruction - Specification

**Created:** 2026-05-06
**Ambiguity score:** 0.11 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can open, inspect, list, look up, and extract shipped Fallout 4 and Starfield `DX10` BA2 texture archives as valid DDS files with libbsa-owned texture metadata and DirectXTex-validated output.

## Background

The codebase already detects `BTDX` + `DX10` archive identities in `src/detect.cpp` and exposes `fo4_ba2_dds` and `starfield_ba2_dds` through `include/libbsa/archive.hpp`. Phase 6 added the BA2-specific public API in `include/libbsa/ba2.hpp`, metadata-only `ba2_archive` behavior, generated BA2 fixture tests, bounded `FileTableOffset` name-table parsing, and single-entry extraction for `GNRL` archives. The existing `src/ba2_reader.cpp` still rejects non-`GNRL` subtypes, treats `.dds` files inside `GNRL` archives as ordinary bytes, and has no `DX10` texture record parser, texture chunk reader, DDS header reconstruction path, or public texture metadata type. DirectXTex is already a private vcpkg dependency in `CMakeLists.txt`, but no libbsa texture adapter or DirectXTex validation tests exist yet.

## Requirements

1. **Shipped DX10 variant support**: Phase 7 opens, lists, inspects, looks up, and extracts shipped Fallout 4 and Starfield `BTDX` + `DX10` BA2 texture archive variants.
   - Current: `detect_archive` recognizes `DX10` identities, but `open_ba2` returns `unsupported_format` for every non-`GNRL` BA2 subtype.
   - Target: `open_ba2` accepts Fallout 4 `DX10` versions 1, 7, and 8 plus Starfield `DX10` version 3, producing a `ba2_archive` with normalized paths and copied metadata for each texture entry.
   - Acceptance: Generated fixtures for FO4 DX10 v1, v7, v8, and Starfield DX10 v3 open successfully, report the expected `archive_format`, version, subtype, file count, file table offset, paths, and lookup results.

2. **DX10 record and chunk parsing**: Phase 7 parses BA2 texture records, variable chunk metadata, and the associated BA2 name table.
   - Current: Phase 6 parses fixed-size GNRL records and length-prefixed file names at `FileTableOffset`; there is no texture record/chunk model.
   - Target: DX10 entries read their texture dimensions, format, mip/cubemap/array metadata, chunk count, chunk offsets, packed sizes, unpacked sizes, and length-prefixed names using bounded random-access reads.
   - Acceptance: Generated multi-entry fixtures assert exact texture metadata, chunk metadata, normalized path order, and name-to-record association for at least one single-chunk texture and one multi-chunk texture.

3. **DDS header reconstruction**: Phase 7 extraction reconstructs complete DDS files from BA2 DX10 metadata and chunk payloads.
   - Current: BA2 extraction can emit GNRL payload bytes, including `.dds`-named GNRL entries, but no code synthesizes DDS or DDS/DX10 headers for texture archives.
   - Target: `extract_ba2_entry` writes a complete DDS byte stream with the correct DDS magic, legacy DDS header, DX10 extension header when required, dimensions, format, mip count, array size, cubemap flags, and decompressed image payload bytes.
   - Acceptance: DirectXTex `LoadFromDDSMemory` succeeds for extracted fixture outputs and reports expected metadata for dimensions, mip count, format, array size, and cubemap state.

4. **Per-chunk codec routing**: Phase 7 extracts DX10 texture chunks through the correct compression route for the archive variant and chunk metadata.
   - Current: BA2 GNRL extraction routes contiguous entry payloads through raw, deflate, or Starfield v3 raw LZ4 block handling; DX10 chunks are not read or decompressed.
   - Target: Fallout 4 DX10 compressed chunks use the existing libdeflate-backed route, Starfield DX10 v3 compressed chunks with `CompressionMethod == 3` use the raw LZ4 block route, and no DX10 extraction path uses the Skyrim LZ4 frame codec.
   - Acceptance: Generated fixtures prove FO4 deflate chunk extraction, Starfield v3 LZ4-block chunk extraction, raw/unpacked chunk handling where represented by metadata, and codec-confusion failure with no partial sink writes.

5. **Libbsa-owned texture metadata**: Phase 7 exposes texture metadata through public libbsa-owned C++20 types without leaking DirectXTex, Windows SDK, codec, Delphi, or TES5Edit types.
   - Current: `entry_metadata` contains generic path, size, offset, hash, and compression fields but cannot describe texture dimensions, DXGI-format identity, mip count, chunk layout, array size, or cubemap state.
   - Target: Consumers can retrieve texture metadata for DX10 entries through public libbsa API using libbsa-owned value types and structured errors for missing or non-texture metadata.
   - Acceptance: Public-header smoke coverage compiles consumer-style code that includes only public libbsa headers, opens or constructs a BA2 texture archive object, retrieves texture metadata, and references no DirectXTex, Windows SDK, libdeflate, LZ4, or TES5Edit symbols.

6. **Mip, array, and cubemap fidelity**: Phase 7 preserves texture layout semantics that affect DDS loadability and consumer inspection.
   - Current: Existing tests only cover generic archive paths and payload bytes; no tests prove mip ordering, cubemap flags, array size, or texture chunk boundaries.
   - Target: Extracted DDS output and public texture metadata preserve mip levels, chunk ordering, array size, cubemap state, dimensions, and format for representative BA2 DX10 textures.
   - Acceptance: Generated fixtures include at least a one-mip 2D texture, a multi-mip texture, and a cubemap or array texture; DirectXTex validation and metadata assertions pass for each.

7. **Generated DirectXTex-validated corpus**: Phase 7 acceptance is proven through deterministic in-repo BA2 DX10 fixture builders and DirectXTex validation, not external archive samples.
   - Current: Phase 6 BA2 GNRL acceptance uses generated fixtures; there are no BA2 DX10 fixture builders or DirectXTex-backed DDS validation tests.
   - Target: The test suite includes deterministic generated BA2 DX10 fixtures for the locked FO4 and Starfield variant matrix, texture metadata cases, chunk compression routes, malformed tables, and reconstructed DDS loadability.
   - Acceptance: `ctest` can run focused BA2 DDS tests locally without real game archives, BSArchPro execution, external corpora, or mutation of `TES5Edit/`; broader real-sample and BSArchPro comparison remains Phase 11 scope.

8. **Malformed DX10 safety**: Phase 7 rejects malformed DX10 archives and failed reconstruction paths with structured errors and no partial writes.
   - Current: BA2 GNRL parser hardening covers truncated headers, records, name tables, duplicate normalized names, impossible offsets, and extraction range overflow, but those checks do not cover DX10 record/chunk semantics.
   - Target: DX10 parsing and extraction validate header sizes, record ranges, chunk ranges, name-table bounds, duplicate normalized names, chunk count/size consistency, codec support, and DDS reconstruction preconditions before writing output bytes.
   - Acceptance: Malformed DX10 fixtures for truncated records, invalid chunk ranges, mismatched chunk sizes, impossible `FileTableOffset`, duplicate normalized names, unsupported codec routes, and DDS reconstruction failures return structured errors without crashes, unchecked allocations, or partial sink writes.

## Boundaries

**In scope:**
- `BTDX` + `DX10` parsing for Fallout 4 versions 1, 7, and 8.
- `BTDX` + `DX10` parsing for Starfield version 3.
- BA2 DX10 texture record parsing, chunk metadata parsing, and length-prefixed name-table association.
- Normalized path listing, lookup, generic metadata inspection, and public texture metadata inspection for BA2 DX10 entries.
- Single-entry texture extraction through the existing caller-owned `byte_source` and `byte_sink` pattern.
- DDS and DDS/DX10 header reconstruction for extracted texture bytes.
- Deflate chunk extraction for Fallout 4 DX10 archives and raw LZ4-block chunk extraction for Starfield DX10 v3 when `CompressionMethod == 3`.
- Generated deterministic BA2 DX10 fixture builders and DirectXTex-backed validation of reconstructed DDS outputs.
- Structured malformed-input tests for DX10 headers, records, chunks, names, codec routes, and reconstruction preconditions.

**Out of scope:**
- Starfield DX10 v2 as a required texture archive target - shipped texture scope is Starfield DX10 v3 for this phase.
- BA2 GNRL reader changes beyond shared helper reuse - Phase 6 already owns GNRL open/list/lookup/extract behavior.
- Treating `.dds` names inside GNRL archives as DX10 textures - Phase 6 locked them as ordinary GNRL payloads.
- BA2 writers, DDS packing, or mip chunk generation for new archives - writer behavior belongs to Phase 10.
- Texture transcoding, optimization, format conversion, or pixel rewriting - libbsa reconstructs archive-stored DDS output only.
- Safe disk extraction policies, overwrite behavior, traversal policy, or directory cleanup - these are application/tooling or later diagnostics concerns.
- Bulk extraction orchestration, multi-threaded extraction, and performance benchmarking - these belong to Phase 12 after correctness paths are established.
- Real game archive samples or BSArchPro output comparison as a Phase 7 gate - generated DirectXTex-validated fixtures are sufficient here; broader compatibility corpus validation is Phase 11.
- Modifying, formatting, staging, compiling, or moving anything under `TES5Edit/` - the submodule remains read-only reference material.

## Constraints

- Public headers must expose only libbsa-owned C++20 types and must not leak libdeflate, LZ4, DirectXTex, Windows SDK, platform, Delphi, or TES5Edit implementation details.
- DDS metadata and validation may use DirectXTex only through private implementation/test boundaries.
- DX10 extraction must use bounded random-access reads from caller-owned `byte_source` objects and caller-owned `byte_sink` writes.
- Extraction must validate all metadata and reconstruct the complete DDS output before writing to the sink, so failed extraction does not leave partial bytes.
- Fallout 4 DX10 compressed chunks must use the existing libdeflate-backed route with exact output-size validation.
- Starfield DX10 v3 method-3 compressed chunks must use the existing raw LZ4 block route, not the Skyrim LZ4 frame route.
- Archive paths remain normalized archive-virtual paths, not host filesystem paths.
- Generated fixtures are the required acceptance corpus for this phase; real archive and BSArchPro comparison gates are deferred to compatibility validation.
- `TES5Edit/` is read-only and must remain unmodified.

## Acceptance Criteria

- [ ] Generated FO4 DX10 v1, v7, and v8 fixtures open successfully, report expected summary metadata, list normalized paths, and support lookup.
- [ ] Generated Starfield DX10 v3 fixture opens successfully, reports `CompressionMethod == 3` where present, lists normalized paths, and supports lookup.
- [ ] DX10 parser tests assert exact texture dimensions, format identity, mip count, array/cubemap state, chunk count, chunk offsets, packed sizes, and unpacked sizes.
- [ ] `extract_ba2_entry` writes reconstructed DDS bytes for FO4 DX10 deflate fixtures that DirectXTex loads with expected metadata.
- [ ] `extract_ba2_entry` writes reconstructed DDS bytes for Starfield DX10 v3 LZ4-block fixtures that DirectXTex loads with expected metadata.
- [ ] Generated one-mip, multi-mip, and cubemap or array fixtures validate both public texture metadata and extracted DDS metadata.
- [ ] Public-header smoke coverage proves texture metadata APIs compile without private codec, DirectXTex, Windows SDK, TES5Edit, or platform headers.
- [ ] Codec-confusion fixtures fail structurally when DX10 extraction would route Starfield raw LZ4 blocks through LZ4 frame or an unsupported method.
- [ ] Malformed DX10 fixtures for truncated records, invalid chunks, name-table problems, duplicate normalized names, unsupported codec routes, and reconstruction failures return structured errors without partial sink writes.
- [ ] Focused BA2 DDS tests, public-header smoke tests, codec tests, full CTest, public-header dependency leakage grep, CMake source-list gate, and `git status --short TES5Edit` pass.

## Ambiguity Report

| Dimension           | Score | Min    | Status | Notes |
|---------------------|-------|--------|--------|-------|
| Goal Clarity        | 0.91  | 0.75   | PASS   | Shipped DX10 read/extract with DDS reconstruction and texture metadata is locked. |
| Boundary Clarity    | 0.86  | 0.70   | PASS   | Variant matrix, generated-corpus gate, writer/perf/GNRL exclusions, and TES5Edit boundary are explicit. |
| Constraint Clarity  | 0.82  | 0.65   | PASS   | Public metadata ownership, DirectXTex boundary, codec routes, no partial writes, and fixture policy are specified. |
| Acceptance Criteria | 0.83  | 0.70   | PASS   | Version, chunk, metadata, DirectXTex loadability, codec, malformed, and boundary checks are pass/fail. |
| **Ambiguity**       | 0.11  | <=0.20 | PASS   | Gate passed after round 1. |

Status: PASS = met minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Which BA2 DX10 variants should Phase 7 lock as required read/extract support? | Require Fallout 4 DX10 v1/v7/v8 and Starfield DX10 v3, matching roadmap and shipped texture archive scope. |
| 1 | Researcher | What user-visible DDS metadata must Phase 7 expose beyond reconstructed output bytes? | Expose libbsa-owned texture metadata for dimensions, format identity, mip/chunk info, and cubemap/array state without DirectXTex leakage. |
| 1 | Researcher | What should be the acceptance corpus for this spec? | Use deterministic in-repo BA2 DX10 fixture builders plus DirectXTex load/metadata validation; defer external corpus comparison to Phase 11. |
| 1 | Gate | Ambiguity gate reached; proceed to write SPEC.md? | User selected "Yes, write 07-SPEC.md". |

---

*Phase: 07-ba2-dds-read-and-dds-reconstruction*
*Spec created: 2026-05-06*
*Next step: /gsd-discuss-phase 7 - implementation decisions (how to build BA2 DDS parsing, texture metadata, and reconstruction)*
