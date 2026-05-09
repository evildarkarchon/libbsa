# Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction - Specification

**Created:** 2026-05-08
**Ambiguity score:** 0.11 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can open Fallout 4 and Starfield BA2 DX10 texture archives, inspect library-owned DDS texture metadata, and extract entries as DirectXTex-loadable DDS files without exposing DirectXTex or platform types in public headers.

## Background

Phase 5 completed BA2 GNRL support through `archive_reader`: byte-driven BA2 detection, bounded metadata/name-table parsing, normalized lookup, and sink-based extraction for raw, deflate, and Starfield raw-LZ4-block payloads. Current BA2 detection recognizes `DX10` bytes but deliberately returns `error_code::unsupported` with the message that texture archives are deferred to the DDS phase. No BA2 DX10 parser, texture metadata model, DDS header reconstruction path, texture fixture generator, `src/texture/` implementation area, or DirectXTex CMake wiring exists today. DirectXTex is already an allowed vcpkg dependency and must remain an internal analyzer/validator boundary rather than a public API type.

## Requirements

1. **BA2 DX10 open support**: `archive_reader::open` accepts generated Fallout 4 BA2 DX10 and Starfield BA2 v3 DX10 texture archives based on archive bytes rather than host filename extension.
   - Current: `detect_ba2_format` recognizes `DX10` subtype but returns `error_code::unsupported`; `archive_reader::open` only routes BA2 archives through the GNRL parser.
   - Target: `BTDX`/`DX10` archives for Fallout 4 and Starfield v3 route to a DX10/DDS parser and report `archive_type::ba2`, the correct public variant, version, file count, and dependency-light BA2 metadata.
   - Acceptance: Generated FO4 DX10 and Starfield v3 DX10 fixtures open successfully through `archive_reader::open`; unsupported BA2 versions or structurally invalid DX10 headers fail with stable `error_code` values.

2. **Texture metadata inspection**: Consumers can inspect texture metadata for each BA2 DX10 entry through libbsa-owned public or documented metadata types.
   - Current: `entry_metadata` exposes generic archive payload fields only; no dimensions, mip count, DXGI format identifier, array size, cubemap state, or chunk layout metadata is available.
   - Target: BA2 DX10 entries expose texture width, height, mip count, DXGI format identifier, array/cubemap information, and chunk layout without including DirectXTex, DXGI, Windows SDK, or private parser types from public headers.
   - Acceptance: Unit tests over generated FO4 and Starfield DX10 fixtures assert expected dimensions, mip counts, format identifiers, array/cubemap flags, and per-entry chunk counts using only libbsa public or documented library-owned types.

3. **DX10 chunk layout parsing**: BA2 DX10 record and chunk metadata are parsed with checked offsets, sizes, and counts before payload extraction.
   - Current: BA2 GNRL records use a fixed 36-byte layout and a single payload span; no texture chunk record model exists.
   - Target: BA2 DX10 parsing validates record and chunk table bounds, exposes archive-absolute chunk payload offsets/sizes where appropriate, and rejects overlapping, truncated, oversized, or internally inconsistent chunk metadata before extraction succeeds.
   - Acceptance: Generated malformed DX10 fixtures for truncated headers, truncated records/chunks, invalid payload spans, and inconsistent size/count metadata fail at open or extraction with the expected stable `error_code` values.

4. **DDS reconstruction and extraction**: BA2 DX10 entries extract as complete DDS files with reconstructed headers plus chunk payload bytes in the correct texture order.
   - Current: `archive_reader::extract` dispatches BA2 entries to `extract_ba2_gnrl_payload`; DX10 archives cannot be opened or extracted.
   - Target: `extract(path, sink)` and `extract_bytes(path)` on BA2 DX10 entries produce valid DDS byte streams with headers reconstructed from BA2 texture metadata and payload bytes assembled in the expected mip/array order.
   - Acceptance: Extracting every generated DX10 fixture entry through both sink-based and byte-vector APIs produces bytes that DirectXTex can load for metadata inspection, and the loaded metadata matches the manifest for dimensions, mip count, format, array size, and cubemap state.

5. **DX10 compression routing**: BA2 DX10 chunk payloads extract correctly when stored raw, deflate-compressed, or Starfield raw-LZ4-block-compressed according to archive version and chunk metadata.
   - Current: Exact-size deflate and raw LZ4 block services exist and BA2 GNRL routes by parsed metadata; there is no DX10 chunk-level codec routing.
   - Target: DX10 extraction chooses raw, deflate, or raw LZ4 block handling from parsed archive/chunk metadata and validates exact decompressed byte counts for each compressed chunk.
   - Acceptance: Generated fixtures include at least one raw DX10 texture chunk, one deflate-compressed FO4 DX10 chunk, and one Starfield v3 raw-LZ4-block DX10 chunk; extracted DDS output matches expected content metadata and corrupted or size-mismatched compressed chunks fail closed.

6. **DirectXTex boundary validation**: DirectXTex is used only behind an internal texture analysis/validation boundary.
   - Current: `vcpkg.json` includes `directxtex`, but CMake does not call `find_package` for it and no code uses DirectXTex today.
   - Target: DirectXTex-backed validation or analysis exists internally for DDS metadata loading, while installed public headers remain free of `DirectXTex`, `DXGI`, Windows SDK, and implementation-specific texture types.
   - Acceptance: Public include boundary tests continue to reject DirectXTex/Windows leakage; DirectXTex metadata loading validates reconstructed DDS fixture outputs during automated tests.

7. **Cubemap and array ordering**: Cubemap and array textures extract with correct DDS metadata and face/mip ordering.
   - Current: No BA2 DX10 texture reconstruction exists, and Phase 5 explicitly deferred cubemaps, mip ordering, and chunk limits to this phase.
   - Target: BA2 DX10 cubemap and array entries reconstruct DDS metadata and payload ordering so consumers receive valid DDS files matching the texture layout represented by BA2 chunk records.
   - Acceptance: Generated cubemap fixture extraction loads through DirectXTex with cubemap metadata set and the expected face/mip counts; generated array fixture extraction loads with the expected array size and mip count.

8. **Legal fixture-backed DX10 validation**: Phase 6 behavior is validated by generated repository-owned BA2 DX10 fixtures and manifests rather than copyrighted game archives.
   - Current: Generated BA2 fixtures cover GNRL only; the only DX10 fixture is an unsupported-header malformed case from Phase 5.
   - Target: Maintainers can generate small legal FO4 and Starfield BA2 DX10 fixtures with manifests covering texture metadata, chunk layout, raw/deflate/raw-LZ4 payloads, cubemap/array cases, and malformed DX10 cases.
   - Acceptance: CTest builds and uses generated DX10 fixtures automatically for Phase 6 tests; optional local game fixtures are not required for Phase 6 acceptance and no fixture generation writes under `TES5Edit/`.

## Boundaries

**In scope:**
- BA2 `DX10` detection/open routing for generated Fallout 4 and Starfield v3 texture archives.
- BA2 DX10 header, record, chunk table, filename/path, and texture metadata parsing needed for read/extract behavior.
- Library-owned texture metadata sufficient to represent dimensions, mip count, DXGI format identifier, array/cubemap state, and chunk layout.
- DDS header reconstruction for extracted BA2 DX10 entries.
- Sink-based and `extract_bytes` extraction of BA2 DX10 entries as complete DDS byte streams.
- Raw, deflate, and Starfield raw-LZ4-block DX10 chunk extraction with exact-size validation.
- DirectXTex-backed validation or analysis behind an internal boundary.
- Generated legal FO4/Starfield DX10 fixtures and malformed cases covering metadata, extraction, compression, cubemaps, and arrays.
- Regression coverage proving existing TES3, TES4-family BSA, BA2 GNRL, public include boundary, and TES5Edit read-only constraints remain intact.

**Out of scope:**
- BA2 DX10 write-new support - Phase 9 owns creating texture BA2 archives from DDS input.
- BA2 GNRL write-new support - Phase 8 owns general BA2 archive creation.
- TES4-family or TES3 BSA writing - later writer phases own BSA creation.
- Broad compatibility warnings or lenient recovery for malformed texture archives - Phase 11 owns validation API and hardening beyond focused fail-closed parser checks.
- Performance benchmarking, parallel extraction, or bulk-concurrency guarantees - Phase 12 owns performance and concurrency work.
- Real game archive fixtures as required evidence - generated legal fixtures are required; local game fixtures remain optional.
- Public exposure of DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, or private parser/codec/texture types - public headers must remain dependency-light.
- Editing, formatting, compiling, staging, or using `TES5Edit/` as a fixture workspace - it remains read-only reference material.
- In-place archive mutation - v1 read/write sequencing excludes mutation of existing archives.

## Constraints

- Public APIs remain C++20-compatible and must not expose `std::expected`, DirectXTex, DXGI, Windows SDK, TES5Edit, libdeflate, lz4, or private implementation types.
- BA2 DX10 detection must be byte-driven and must not rely on `.ba2`, `.dds`, or other host filename extensions.
- Archive paths remain normalized archive virtual paths, not `std::filesystem::path` values.
- DirectXTex may be used internally for DDS metadata validation/analysis, but public headers and installed consumer builds must not require DirectXTex includes.
- Deflate and raw LZ4 block chunk extraction must validate exact decompressed output size against archive/chunk metadata.
- Starfield v3 raw LZ4 must use the raw LZ4 block API, not the LZ4 frame API.
- Open/list operations should preserve the existing bounded metadata-read direction and must not load whole archive payloads just to list entries.
- Fixture evidence must be generated and legal to commit; optional local game archive checks cannot be mandatory for acceptance.
- `TES5Edit/` must remain read-only and have no git status output after phase execution.

## Acceptance Criteria

- [ ] `archive_reader::open` succeeds for generated Fallout 4 BA2 DX10 and Starfield BA2 v3 DX10 fixtures.
- [ ] Unsupported or malformed BA2 DX10 fixtures fail with stable `error_code` values.
- [ ] BA2 DX10 archives report `archive_type::ba2`, the correct public variant, version, file count, and dependency-light BA2 metadata.
- [ ] Consumers can inspect dimensions, mip count, DXGI format identifier, array/cubemap state, and chunk layout using only libbsa-owned types.
- [ ] `entries`, `find`, and `contains` work for BA2 DX10 paths using normalized archive virtual path semantics.
- [ ] BA2 DX10 record and chunk parsing rejects truncated, overlapping, oversized, or internally inconsistent metadata before unsafe reads occur.
- [ ] Raw BA2 DX10 chunks extract into valid reconstructed DDS output.
- [ ] Deflate BA2 DX10 chunks extract with exact decompressed size validation.
- [ ] Starfield v3 raw-LZ4-block BA2 DX10 chunks extract with exact decompressed size validation.
- [ ] `extract(path, sink)` and `extract_bytes(path)` produce DirectXTex-loadable DDS byte streams for every generated success fixture entry.
- [ ] DirectXTex-loaded metadata for reconstructed DDS outputs matches manifest dimensions, mip count, format identifier, array size, and cubemap state.
- [ ] Generated cubemap fixture extraction preserves expected cubemap metadata and face/mip counts.
- [ ] Generated array texture fixture extraction preserves expected array size and mip count.
- [ ] Generated BA2 DX10 fixtures and manifests are legal, committed outside `TES5Edit/`, and sufficient for automated CTest validation.
- [ ] Existing BA2 GNRL, TES3/TES4-family BSA, and public include boundary tests remain green after Phase 6 support is added.
- [ ] `git -C TES5Edit status --short` produces no output after Phase 6 execution.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.93  | 0.75  | PASS   | Primary deliverable locked as FO4 and Starfield DX10 open, metadata inspection, and DDS extraction. |
| Boundary Clarity    | 0.86  | 0.70  | PASS   | Write-new, broad hardening, performance, real archive fixtures, and public DirectXTex exposure excluded. |
| Constraint Clarity  | 0.86  | 0.65  | PASS   | DirectXTex boundary, generated fixtures, byte-driven detection, codec routing, and public API limits locked. |
| Acceptance Criteria | 0.88  | 0.70  | PASS   | Pass/fail checks cover open, metadata, chunks, DDS validation, compression, cubemaps/arrays, fixtures, and regressions. |
| **Ambiguity**       | 0.11  | <=0.20| PASS   | Gate passed after round 1. |

Status: PASS = met minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What is the primary Phase 6 deliverable? | Open and extract DX10: BA2 DX10/DDS open, metadata inspection, and extraction as valid DDS files are all required. |
| 1 | Researcher | Which archive variants must Phase 6 prove? | Fallout 4 DX10 and Starfield v3 DX10 both require fixture/test coverage. |
| 1 | Researcher | What validation level should reconstructed DDS output meet? | Reconstructed DDS bytes must load through DirectXTex metadata validation without exposing DirectXTex types publicly. |
| 1 | Gate | Ambiguity reached 0.11. Proceed with SPEC.md? | User selected `Yes - write SPEC.md`. |

---

*Phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction*
*Spec created: 2026-05-08*
*Next step: /gsd-discuss-phase 6 - implementation decisions (how to build what's specified above)*
