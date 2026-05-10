# Phase 9: BA2 DX10 Write-New Support - Specification

**Created:** 2026-05-09
**Ambiguity score:** 0.13 (gate: <= 0.20)
**Requirements:** 11 locked

## Goal

Consumers can create new Fallout 4 BA2 DX10 v1 and Starfield BA2 DX10 v3 texture archives from DDS host files through a public C++20 writer, with internal DDS analysis, valid mip/chunk metadata, target-routed compression, opt-in deduplication, safe write-new finalization, and reader-backed DirectXTex validation.

## Background

libbsa already reads and extracts BA2 DX10 texture archives through `archive_reader`: generated Fallout 4 and Starfield v3 fixtures open by bytes, expose dependency-light texture metadata, validate chunk coverage/order, extract complete DDS byte streams, and route raw, deflate, and Starfield raw-LZ4-block chunks. `src/texture/dds_layout.*` can build DXT10 headers and validate chunk layouts, while `src/texture/directxtex_analyzer.*` loads DDS metadata behind an internal DirectXTex boundary. Phase 8 added a public BA2 GNRL writer with target profiles, compression policy, opt-in deduplication, reader-backed round trips, and filesystem-safe publish behavior. No public BA2 DX10 writer, DX10 serialization path, DDS host-file packing flow, DDS mip/chunk planner, texture-writer fixture set, or DX10 writer tests exist today.

## Requirements

1. **Public BA2 DX10 writer API**: The library exposes a dedicated public writer for new BA2 DX10 archives from DDS host files.
   - Current: `include/libbsa/writer.hpp` exposes TES4-family BSA and BA2 GNRL writers only; BA2 DX10 behavior exists only in reader/parser/extraction code and generated fixture helpers.
   - Target: Consumers can construct a BA2 DX10 writer, add DDS host-file entries with explicit archive-internal paths, select a target profile and options, and finalize a new archive with `result<void>` error reporting through public C++20 headers.
   - Acceptance: Public include-boundary tests compile using installed libbsa headers, create a BA2 DX10 writer, add at least one DDS host-file entry, and call `write_to` without including private headers, DirectXTex, DXGI, Windows SDK, libdeflate, lz4, or TES5Edit headers.

2. **DDS host-file input only**: Phase 9 accepts DDS source files from host paths and does not require memory-buffer DDS sources.
   - Current: BA2 GNRL and TES4-family writers support memory buffers, but Phase 9 roadmap requirements specify creating DX10 archives from DDS files and no DX10 writer input surface exists.
   - Target: The required DX10 writer source API is host-file DDS input; empty, missing, unreadable, non-DDS, or invalid DDS source paths fail with structured errors.
   - Acceptance: Tests prove a valid DDS host file can be packed, while empty source paths return `error_code::invalid_argument` and missing/unreadable or malformed DDS files return stable structured errors without throwing expected caller-data failures.

3. **Target profiles**: The writer supports mandatory Fallout 4 BA2 DX10 v1 and Starfield BA2 DX10 v3 target output.
   - Current: The reader detects and parses generated FO4 v1 and Starfield v3 DX10 archives; no writer can serialize either target profile.
   - Target: The public DX10 writer requires an explicit target profile and serializes `BTDX`/`DX10` headers for FO4 v1 and Starfield v3, including Starfield v3 metadata fields exposed through documented options or deterministic defaults.
   - Acceptance: Writer-output tests create one FO4 v1 DX10 archive and one Starfield v3 DX10 archive; reopening each reports `archive_type::ba2`, expected public variant, expected version, expected file count, expected default compression, and expected BA2 metadata optionals.

4. **DDS analysis and supported format set**: DDS inputs are analyzed through the internal DirectXTex boundary and must support the locked common-game DDS format set.
   - Current: `analyze_dds_metadata` translates DirectXTex metadata into libbsa-owned fields, and `dds_layout` supports only the fixture-backed formats used by existing read tests.
   - Target: Writer DDS analysis accepts at least these DXGI formats by name: `BC1_UNORM`, `BC1_UNORM_SRGB`, `BC3_UNORM`, `BC4_UNORM`, `BC5_UNORM`, `BC5_SNORM`, `BC6H_UF16`, `BC7_UNORM`, `R8G8B8A8_UNORM_SRGB`, `B8G8R8A8_UNORM`, `R8_UNORM`, and `R8G8B8A8_SNORM`. Unsupported DDS formats fail with a structured error before archive bytes are finalized.
   - Acceptance: Generated DDS writer fixtures include every locked DXGI format at least once across FO4 and Starfield v3 tests, and each unsupported-format fixture fails before writing a readable archive with the expected stable `error_code`.

5. **Texture record serialization**: The writer serializes BA2 DX10 texture records and filename tables compatible with existing reader behavior.
   - Current: `ba2_dx10_parser` reads `BTDX`/`DX10` headers, texture records, fixed 24-byte chunk headers, UInt16 filename tables, and `BAADF00D` sentinels; no production writer emits this structure.
   - Target: Writer output emits one texture record per DDS entry with compatible name/directory hashes, four-byte extension field, `unknown_tex`, chunk count, fixed 24-byte chunk header size, dimensions, mip count, DXGI format id, cubemap/array field, chunk records, payload offsets/sizes, and required `BAADF00D` sentinels.
   - Acceptance: Reopened writer output lists every input DDS path exactly once by normalized canonical path, preserves original archive spelling with `/` separators, reports expected `entry_metadata::texture` values, and exposes chunk metadata matching the writer-produced physical record table.

6. **Mip, array, cubemap, and chunk planning**: The writer splits DDS image data into valid BA2 DX10 chunk records with configurable chunk limits.
   - Current: `validate_and_order_chunks` can validate parsed chunk coverage and order during read/extract, but no writer plans chunks from DDS mip/array/cubemap data.
   - Target: Writer chunk planning covers every mip of every array slice or cubemap face exactly once, emits chunk `start_mip`/`end_mip` ranges in BA2-compatible order, respects caller-configurable chunk-size or mip-range limits, and rejects layouts that cannot be represented without gaps, overlaps, or unsupported format sizing.
   - Acceptance: Tests pack multi-mip, array, and cubemap DDS inputs; reopened chunk metadata passes `validate_and_order_chunks`, covers all mip ranges without gaps or duplicates, and changes chunk count/ranges predictably when the configured chunk limit is changed.

7. **Chunk compression policy and overrides**: The writer applies target-default compression plus caller overrides to generated texture chunks.
   - Current: BA2 DX10 extraction routes raw, deflate, and Starfield raw-LZ4-block chunks from parsed metadata; BA2 GNRL writer compression policy exists but no DX10 writer compression exists.
   - Target: DX10 writer options provide target-default, all-raw, all-compressed, and override behavior for texture entries or chunks. FO4 compressed chunks use deflate. Starfield v3 compressed chunks use the selected Starfield compression method, with method `3` using raw LZ4 block. Raw chunks serialize with `PackedSize == 0`.
   - Acceptance: Writer-output tests create raw and compressed DX10 chunks for FO4 and Starfield v3 method `3`; reopening reports expected `entry_compression`/chunk compression metadata, and extracted DDS output validates after exact-size decompression.

8. **No DDS transforms**: The writer packs existing DDS content and does not modify texture data to make it fit.
   - Current: Existing DX10 extraction reconstructs DDS headers from archive metadata, but no writer accepts DDS source bytes or preserves image data.
   - Target: The writer does not resize textures, transcode formats, generate missing mip levels, alter DXGI formats, reorder caller texture content beyond BA2-required chunk serialization, or repair malformed DDS files.
   - Acceptance: For generated source DDS files, reopened/extracted DDS metadata matches the source metadata, extracted image payload bytes match the source image payload bytes in DDS order, and malformed or missing-mip DDS inputs fail rather than being repaired.

9. **Opt-in texture payload deduplication**: The writer optionally deduplicates identical final stored DX10 chunk payloads when requested.
   - Current: BA2 GNRL writer has opt-in final-stored-byte deduplication; DX10 writer state and chunk payload ownership do not exist.
   - Target: DX10 deduplication is disabled by default. When enabled, eligible byte-identical final stored chunks may share payload offsets while preserving distinct texture records, chunk records, and filename-table entries.
   - Acceptance: Tests pack duplicate DDS inputs with deduplication disabled and enabled; disabled output has distinct eligible chunk payload offsets, enabled output shares eligible identical final stored chunk offsets, and both modes reopen and extract valid DDS bytes for every entry.

10. **Reader-backed validation**: Writer output is proven through public reader APIs and DirectXTex validation rather than writer internals.
    - Current: DX10 reader/extraction tests validate generated fixtures, and Phase 8 writer tests validate BA2 GNRL output by reopening; no DX10 writer output is round-tripped.
    - Target: Phase 9 tests pack, reopen, inspect metadata, list, find, contains, extract, and DirectXTex-validate writer-produced DX10 archives for FO4 and Starfield v3.
    - Acceptance: CTest writer tests pass for FO4 v1 and Starfield v3 output, every writer-produced archive reopens through `archive_reader::open`, every entry extracts through both sink and `extract_bytes`, and DirectXTex-loaded metadata from extracted DDS bytes matches the source DDS metadata.

11. **Safe write-new finalization and legal fixtures**: DX10 writer finalization preserves caller filesystem data and validation uses repository-owned generated inputs.
    - Current: Phase 8 BA2 GNRL writer uses unique temporary publish directories and backup/rollback overwrite behavior after a verification gap; no DX10 writer finalization exists.
    - Target: DX10 `write_to` refuses existing outputs unless overwrite is enabled, rejects non-regular overwrite targets, publishes through collision-safe temporary paths, rolls back failed replacements where possible, and uses generated/legal DDS source fixtures and manifests outside `TES5Edit/`.
    - Acceptance: Tests prove temp-name sibling files are not deleted, existing outputs are preserved when overwrite is false, non-regular overwrite targets fail safely, generated DDS/BA2 fixtures are committed outside `TES5Edit/`, and `git -C TES5Edit status --short` produces no output after phase work.

## Boundaries

**In scope:**
- Public BA2 DX10 write-new API for DDS host-file entries.
- Mandatory FO4 v1 and Starfield v3 DX10 target profiles.
- DirectXTex-backed internal DDS analysis translated into libbsa-owned metadata.
- Support for the locked common DDS format set: `BC1_UNORM`, `BC1_UNORM_SRGB`, `BC3_UNORM`, `BC4_UNORM`, `BC5_UNORM`, `BC5_SNORM`, `BC6H_UF16`, `BC7_UNORM`, `R8G8B8A8_UNORM_SRGB`, `B8G8R8A8_UNORM`, `R8_UNORM`, and `R8G8B8A8_SNORM`.
- BA2 DX10 header, texture record, chunk record, payload, and filename-table serialization.
- Mip, array, cubemap, and configurable chunk-limit planning for supported DDS inputs.
- Target-default compression plus raw/compressed overrides for generated texture chunks.
- FO4 deflate chunk writing and Starfield v3 method `3` raw-LZ4-block chunk writing.
- Opt-in final-stored-byte chunk payload deduplication.
- Safe write-new filesystem finalization matching Phase 8 publish safety expectations.
- Reader-backed tests that reopen writer output, inspect metadata, list/find/contains entries, extract DDS bytes, and validate those bytes through DirectXTex.
- Generated legal DDS source fixtures, BA2 writer outputs, and manifests outside `TES5Edit/`.

**Out of scope:**
- Memory-buffer DDS input - user selected DDS host files only for Phase 9.
- Mandatory Starfield v2 DX10 writer output - Phase 9 acceptance requires FO4 v1 and Starfield v3 only.
- Arbitrary DDS formats beyond the locked common-game set - additional formats can be added later with fixture evidence.
- DDS resizing, format transcoding, mip generation, alpha-mode conversion, or repair of malformed DDS files - writer packs existing valid DDS content only.
- In-place mutation of existing BA2 archives - v1 write support remains write-new.
- Parallel packing, parallel compression, large-archive streaming guarantees, and benchmarks - Phase 12 owns performance and concurrency after correctness.
- Broad compatibility-warning APIs and sanitizer-backed malformed-input hardening beyond writer-required checks - Phase 11 owns those surfaces.
- Real game archive or texture fixtures as mandatory evidence - optional local checks may be added later but are not required for acceptance.
- Public exposure of DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, or private parser/codec/texture types - public headers remain dependency-light.
- Editing, formatting, compiling, staging, or using `TES5Edit/` as a fixture workspace - it remains read-only reference material.

## Constraints

- Public APIs remain C++20-compatible and must not expose `std::expected`, DirectXTex, DXGI, Windows SDK, TES5Edit, libdeflate, lz4, or private implementation types.
- DDS analysis may use DirectXTex internally, but all public texture metadata and writer options must be libbsa-owned values.
- Archive-internal paths remain normalized virtual paths, not `std::filesystem::path` values.
- Target profile and compression routing must be explicit; the writer must not infer archive version, compression method, or target family solely from host filename extensions.
- Starfield v3 `CompressionMethod == 3` compressed chunks must use the raw LZ4 block API, not the LZ4 frame API.
- Compressed chunks must round-trip through existing exact-size decompression validation after reopening.
- DDS writer output may canonicalize reconstructed DDS headers during extraction, but texture metadata and image payload bytes must preserve the source DDS content for supported inputs.
- Single-threaded correctness is sufficient for Phase 9; broad bounded-memory packing and parallel compression guarantees are deferred to Phase 12 unless needed to avoid correctness bugs.
- Writer finalization must use non-throwing filesystem probes for caller-controlled paths where practical and return structured `result` errors for expected I/O failures.
- Fixture evidence must be generated/legal and committed outside `TES5Edit/`; optional local game textures cannot be mandatory acceptance inputs.

## Acceptance Criteria

- [ ] Public installed-header tests construct a BA2 DX10 writer, add DDS host-file entries, and finalize output without private or third-party headers.
- [ ] Empty DDS source paths fail with `error_code::invalid_argument`; missing/unreadable and malformed DDS sources fail with stable structured errors.
- [ ] Writer-output FO4 v1 and Starfield v3 DX10 archives reopen through `archive_reader::open` with expected archive type, variant, version, file count, default compression, and BA2 metadata.
- [ ] Generated DDS writer fixtures cover `BC1_UNORM`, `BC1_UNORM_SRGB`, `BC3_UNORM`, `BC4_UNORM`, `BC5_UNORM`, `BC5_SNORM`, `BC6H_UF16`, `BC7_UNORM`, `R8G8B8A8_UNORM_SRGB`, `B8G8R8A8_UNORM`, `R8_UNORM`, and `R8G8B8A8_SNORM`.
- [ ] Unsupported DDS formats fail before finalizing a readable archive.
- [ ] Reopened DX10 entries expose expected normalized paths, original path spelling, texture metadata, chunk records, chunk offsets/sizes, and `BAADF00D` sentinel-backed record structure.
- [ ] Multi-mip, array, and cubemap DDS inputs extract with matching DirectXTex metadata and validated mip/face/array ordering.
- [ ] Configurable chunk limits change generated chunk ranges predictably while preserving complete mip coverage.
- [ ] FO4 deflate-compressed chunks and Starfield v3 method `3` raw-LZ4-block chunks extract with exact-size validation.
- [ ] Raw/compressed overrides produce expected `PackedSize`, chunk compression metadata, and valid extracted DDS output.
- [ ] The writer does not resize, transcode, generate mips, alter DXGI formats, or repair malformed DDS inputs.
- [ ] Extracted DDS metadata matches source DDS metadata for every writer success fixture, and generated source image payload bytes match after DDS-order extraction.
- [ ] Deduplication disabled output uses distinct eligible chunk offsets; deduplication enabled output shares eligible byte-identical final stored chunk offsets without losing distinct entries.
- [ ] Writer output is validated only through public reader APIs plus DirectXTex metadata loading, not by inspecting writer internals as the sole proof.
- [ ] Safe publish tests prove temp-name siblings are preserved, no-overwrite mode preserves existing outputs, and non-regular overwrite targets fail without replacement.
- [ ] Existing BA2 DX10 reader/extraction, BA2 GNRL writer/reader, TES4-family writer, public include-boundary, and cross-format tests remain green after Phase 9 support is added.
- [ ] Generated fixtures and manifests are legal, committed outside `TES5Edit/`, and no Phase 9 step mutates the `TES5Edit/` submodule.
- [ ] `git -C TES5Edit status --short` produces no output after Phase 9 execution.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.94  | 0.75  | PASS   | Public DX10 writer, DDS host-file input, FO4/SFv3 targets, compression, dedupe, and validation proof are locked. |
| Boundary Clarity    | 0.84  | 0.70  | PASS   | Memory input, Starfield v2 mandate, transforms, broad formats, mutation, performance, and public dependency leakage are excluded. |
| Constraint Clarity  | 0.82  | 0.65  | PASS   | DDS format set, DirectXTex boundary, no transforms, raw-LZ4 method routing, safe publish, and generated fixture constraints are locked. |
| Acceptance Criteria | 0.82  | 0.70  | PASS   | Pass/fail checks cover API, inputs, targets, formats, records, chunks, compression, dedupe, validation, safety, and submodule cleanliness. |
| **Ambiguity**       | 0.13  | <=0.20| PASS   | Gate passed after round 3. |

Status: PASS = meets minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Which consumer-visible input surface must Phase 9 lock? | DDS host files only are required; memory DDS input is out of scope. |
| 1 | Researcher | Which target profiles are mandatory pass/fail outputs? | Fallout 4 DX10 v1 and Starfield DX10 v3 are required. |
| 1 | Researcher | What validation proof counts as done? | Reopen writer output, extract DDS, and validate extracted metadata through DirectXTex. |
| 2 | Researcher + Simplifier | What is the minimum viable DDS breadth? | Common game DDS support is required, not fixture-only trivial DDS support. |
| 2 | Researcher + Simplifier | Which compression controls are required? | Target defaults plus raw/compressed overrides are in scope; FO4 uses deflate and Starfield v3 method `3` uses raw LZ4 block. |
| 2 | Simplifier | Should payload deduplication be part of the core? | Yes, opt-in deduplication is in scope for DX10 writer payloads. |
| 3 | Boundary Keeper | How should common game DDS support be made falsifiable? | Lock the named FO4/Starfield format set: BC1/BC3/BC4/BC5/BC6H/BC7/R8/RGBA8 variants listed in Requirements. |
| 3 | Boundary Keeper | What DDS transformations are explicitly out of scope? | No resizing, transcoding, mip generation, format alteration, or malformed DDS repair. |
| 3 | Boundary Keeper | What is the final consumer-visible deliverable? | A dedicated public BA2 DX10 writer. |
| 3 | Gate | Ambiguity reached 0.13. Proceed with SPEC.md? | User selected `Yes - write SPEC.md`. |

---

*Phase: 09-ba2-dx10-write-new-support*
*Spec created: 2026-05-09*
*Next step: /gsd-discuss-phase 9 - implementation decisions only*
