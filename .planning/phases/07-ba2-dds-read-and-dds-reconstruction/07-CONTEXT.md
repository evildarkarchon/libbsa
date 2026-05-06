# Phase 07: ba2-dds-read-and-dds-reconstruction - Context

**Gathered:** 2026-05-05T21:23:25.3309455-07:00
**Status:** Ready for planning

<domain>
## Phase Boundary

This phase delivers BA2 `DX10` texture archive open, inspect, list, lookup, texture metadata retrieval, and single-entry extraction for shipped Fallout 4 and Starfield texture archives. Extraction reconstructs complete DDS output from parsed BA2 texture metadata and chunk payloads, validates output through private DirectXTex boundaries, and preserves libbsa-owned public API types. It does not add BA2 writers, texture transcoding, real archive corpus gates, safe disk extraction policy, bulk extraction orchestration, or any mutation of `TES5Edit/`.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `07-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `07-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- `BTDX` + `DX10` parsing for Fallout 4 versions 1, 7, and 8.
- `BTDX` + `DX10` parsing for Starfield version 3.
- BA2 DX10 texture record parsing, chunk metadata parsing, and length-prefixed name-table association.
- Normalized path listing, lookup, generic metadata inspection, and public texture metadata inspection for BA2 DX10 entries.
- Single-entry texture extraction through the existing caller-owned `byte_source` and `byte_sink` pattern.
- DDS and DDS/DX10 header reconstruction for extracted texture bytes.
- Deflate chunk extraction for Fallout 4 DX10 archives and raw LZ4-block chunk extraction for Starfield DX10 v3 when `CompressionMethod == 3`.
- Generated deterministic BA2 DX10 fixture builders and DirectXTex-backed validation of reconstructed DDS outputs.
- Structured malformed-input tests for DX10 headers, records, chunks, names, codec routes, and reconstruction preconditions.

**Out of scope (from SPEC.md):**
- Starfield DX10 v2 as a required texture archive target - shipped texture scope is Starfield DX10 v3 for this phase.
- BA2 GNRL reader changes beyond shared helper reuse - Phase 6 already owns GNRL open/list/lookup/extract behavior.
- Treating `.dds` names inside GNRL archives as DX10 textures - Phase 6 locked them as ordinary GNRL payloads.
- BA2 writers, DDS packing, or mip chunk generation for new archives - writer behavior belongs to Phase 10.
- Texture transcoding, optimization, format conversion, or pixel rewriting - libbsa reconstructs archive-stored DDS output only.
- Safe disk extraction policies, overwrite behavior, traversal policy, or directory cleanup - these are application/tooling or later diagnostics concerns.
- Bulk extraction orchestration, multi-threaded extraction, and performance benchmarking - these belong to Phase 12 after correctness paths are established.
- Real game archive samples or BSArchPro output comparison as a Phase 7 gate - generated DirectXTex-validated fixtures are sufficient here; broader compatibility corpus validation is Phase 11.
- Modifying, formatting, staging, compiling, or moving anything under `TES5Edit/` - the submodule remains read-only reference material.

</spec_lock>

<decisions>
## Implementation Decisions

### Texture Metadata API
- **D-01:** Add a dedicated `ba2_archive::texture_metadata(path)` public API for BA2 DX10 texture metadata. Keep generic `entry_metadata` stable instead of adding texture-only fields to every archive entry.
- **D-02:** `texture_metadata(path)` returns `result<texture_metadata>` and fails structurally when texture metadata is unavailable, such as non-texture entries, unsupported archive metadata, or missing paths.
- **D-03:** Public `texture_metadata` includes consumer inspection fields plus libbsa-owned chunk summaries: dimensions, format, mip count, array size, cubemap state, and logical chunk information.
- **D-04:** Return `texture_metadata` by value, matching `entry(path)` and preserving simple caller ownership with no archive-object lifetime surprises.

### DXGI Format Representation
- **D-05:** Represent texture format with a libbsa-owned `dxgi_format` value type. It maps to DXGI format identity without including DXGI, Windows SDK, DirectXTex, or platform headers in public API.
- **D-06:** Preserve the raw DXGI-compatible numeric value in public metadata and provide a libbsa-owned known-name helper for recognized values.
- **D-07:** Unknown or future DXGI numeric values remain inspectable in `dxgi_format`; `open_ba2` and `texture_metadata(path)` should not reject them solely because the label is unknown.
- **D-08:** Extraction and DDS reconstruction may fail structurally if an unknown or unsupported format cannot produce a valid DDS output.

### Chunk Model And Extraction Assembly
- **D-09:** Expose public logical chunk summaries in `texture_metadata`, not raw BA2-native records. Useful fields include archive offset, packed size, unpacked size, first mip, and mip count when reliably derivable.
- **D-10:** Extraction must validate ranges, route codecs, decompress chunks, reconstruct the full DDS byte vector, and validate it before writing anything to the caller-owned sink.
- **D-11:** DX10 chunk codec routing is per chunk and derived from archive variant, chunk sizes, and archive `CompressionMethod`. Fallout 4 compressed chunks use deflate; Starfield v3 method-3 chunks use raw LZ4 block; raw chunks remain raw.
- **D-12:** Do not try fallback codecs. Unsupported or inconsistent chunk codec metadata fails with a structured error and no partial sink write.
- **D-13:** Preserve public chunk-to-mip range information when derivable. Inconsistent chunk/mip mapping is malformed and should fail rather than silently inventing layout.

### DDS Reconstruction Boundary
- **D-14:** libbsa reconstructs DDS and DDS/DX10 headers itself from parsed BA2 metadata; DirectXTex remains behind private implementation and test boundaries.
- **D-15:** Use a private DirectXTex validation adapter after reconstruction and before sink writes where practical, so reconstructed output is proven loadable without leaking DirectXTex into public headers.
- **D-16:** DirectXTex validation or DDS reconstruction failure returns a structured malformed/reconstruction-style failure with no caller sink writes.
- **D-17:** Unsupported-but-readable texture layouts can still open and expose metadata; `extract_ba2_entry` fails structurally before writing if safe DDS reconstruction is unsupported.
- **D-18:** DDS reconstruction and validation should live in internal reusable texture helpers, such as private `src/texture/` components, so later writer phases can reuse the boundary. Do not expose public DDS utility APIs in Phase 7.

### Fixture Corpus Shape
- **D-19:** Use a reusable private test helper module for generated BA2 DDS fixtures. Keep it test-internal, but structure it so later writer phases can reuse the fixture builders.
- **D-20:** Positive fixtures must cover both the locked archive variant matrix and representative texture layout/chunk routes: FO4 DX10 v1/v7/v8, Starfield DX10 v3, one-mip 2D, multi-mip, cubemap or array, raw chunks, FO4 deflate chunks, and Starfield raw LZ4-block chunks.
- **D-21:** Malformed DX10 coverage should use targeted generated builders per failure class: truncated records, invalid chunk ranges, name-table problems, duplicate normalized names, unsupported codec routes, inconsistent mip/chunk mapping, and reconstruction failures.
- **D-22:** Every positive extracted DDS fixture should be validated with DirectXTex `LoadFromDDSMemory` or an equivalent private test adapter, asserting dimensions, format, mip count, array size, and cubemap state.
- **D-23:** Prefer semantic DDS equivalence over universal byte-exact comparison. Assert DirectXTex loadability plus expected metadata/layout/payload semantics; add byte-exact checks only where deterministic and meaningful.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may choose exact helper names, file names, and test organization details as long as the decisions above, `07-SPEC.md`, existing public API patterns, and project constraints are satisfied.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Locked Phase Scope
- `.planning/phases/07-ba2-dds-read-and-dds-reconstruction/07-SPEC.md` - Locked Phase 7 requirements, boundaries, constraints, acceptance criteria, and interview decisions.
- `.planning/ROADMAP.md` - Phase ordering, Phase 7 goal, dependencies, success criteria, and requirement mapping.
- `.planning/REQUIREMENTS.md` - v1 requirement IDs and traceability, especially `BA2-05`, `BA2-06`, `BIO-01` through `BIO-05`, `CMP-03`, `CMP-04`, and validation requirements.
- `.planning/PROJECT.md` - Project purpose, core value, public API constraints, streaming-first requirement, and TES5Edit reference context.

### Prior Phase Decisions
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md` - Carry-forward BA2 API shape, metadata-only archive lifetime, file table parsing, BA2 compression semantics, generated fixture style, and malformed BA2 validation decisions.
- `.planning/phases/02-streaming-api-archive-model-detection-and-hashes/02-CONTEXT.md` - Carry-forward caller-owned source/sink lifetimes, copied archive views, archive path normalization, strict detection, and public API ownership decisions.
- `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md` - Carry-forward decisions for C++20 `libbsa::result`, public/private layout, explicit CMake sources, test labels, and TES5Edit boundary.

### Product And Stack Context
- `docs/PRD.md` - Original BA2 DDS read deliverables: `BTDX` + `DX10` records, variable chunks, DDS header reconstruction, per-chunk decompression, cubemap handling, and valid loadable DDS output.
- `AGENTS.md` - Repository instructions, TES5Edit read-only boundary, dependency rules, comment/docstring policy, and validation expectations.
- `.planning/research/STACK.md` - Dependency and architecture guidance for libdeflate, LZ4 frame/block separation, DirectXTex boundary, CMake/vcpkg, public API dependency leakage, and BA2 DDS stack patterns.

### Existing Code To Inspect
- `include/libbsa/ba2.hpp` - Existing BA2 public archive API shape to extend with texture metadata retrieval.
- `include/libbsa/archive.hpp` - Existing `archive_summary`, `entry_metadata`, archive format IDs, and compression-state fields to preserve or extend carefully.
- `include/libbsa/archive_view.hpp` - Metadata-only copied lookup view pattern reused by archive wrappers.
- `include/libbsa/compression.hpp` - Public compression routing contract, including Starfield BA2 `compression_method` behavior and codec-confusion policy.
- `src/ba2_reader.cpp` - Current BA2 GNRL parser/extractor, bounded read helpers, name-table parsing, duplicate-name rejection, payload range validation, and BA2 archive wrapper implementation.
- `src/compression.cpp` - Existing deflate, LZ4 frame, and raw LZ4 block route separation.
- `src/compression/deflate_codec.cpp` - Existing private libdeflate wrapper and exact output-size validation behavior.
- `src/compression/lz4_block_codec.cpp` - Existing private raw LZ4 block wrapper for Starfield BA2 method-3 payloads.
- `src/detect.cpp` - Existing `BTDX` + `DX10` archive detection for FO4 and Starfield identities.
- `tests/ba2_reader_tests.cpp` - Current generated BA2 fixture builder style, extraction assertions, codec route tests, and malformed-input coverage.
- `tests/public_header_smoke.cpp` - Consumer-style public header smoke coverage to extend for `texture_metadata`, `dxgi_format`, and dependency-leakage checks.
- `tests/compression_policy_tests.cpp` - Existing codec-confusion and Starfield compression-method tests.
- `CMakeLists.txt` - Explicit source/header/test wiring; new texture helpers, public headers, and tests must be listed explicitly and must not include `TES5Edit/`.

### Read-Only Reference Areas
- `TES5Edit/BSArchPro.dpr` - Behavioral reference entry point for BSArchPro compatibility tracing; read-only.
- `TES5Edit/BSArch/` - Reference area for BA2/DDS archive behavior; read-only.
- `TES5Edit/Core/wbBSArchive.pas` - Reference area for archive behavior; read-only.
- `TES5Edit/Core/wbBSA.pas` - Reference area for BSA/path/hash behavior; read-only.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ba2_archive`: Already mirrors the BSA metadata-only archive wrapper and is the natural public home for `texture_metadata(path)`.
- `archive_view`: Already owns copied metadata, normalizes paths, returns deterministic path lists, and provides lookup behavior for archive wrappers.
- `archive_summary`: Already carries BA2 version, subtype, file count, `file_table_offset`, and Starfield v3 `compression_method`.
- `entry_metadata`: Already provides generic path, size, packed size, stored size, offset, hashes, and compression state; keep it stable rather than adding texture-only fields.
- `resolve_payload_codec`, `decompress_payload`, `deflate_codec`, and `lz4_block_codec`: Existing private codec dispatch and exact-size validation should be reused for DX10 chunks.
- `memory_source` and `memory_sink`: Existing helper types suit generated fixture tests and no-partial-write assertions.

### Established Patterns
- Public APIs live under `include/libbsa/`; private implementation lives under `src/`; public headers use Doxygen comments and expose only libbsa-owned C++20 types.
- Archive objects are metadata-only wrappers over copied data; payload bytes remain in caller-owned `byte_source` objects until extraction.
- Parsers use bounded random-access reads, explicit overflow/range checks, and structured `result<T>` failures rather than exceptions for malformed data.
- BA2 parser rejects duplicate normalized names before constructing `archive_view`, because duplicate file-table names are archive-format validity rather than generic lookup-view behavior.
- Tests use Catch2, generated source fixtures, CTest labels such as `unit`, `fixture`, and `codec`, plus public-header smoke coverage.
- `CMakeLists.txt` uses explicit source lists; new files must be wired deliberately.

### Integration Points
- Extend `include/libbsa/ba2.hpp` or a closely related public header with `texture_metadata`, `texture_chunk_metadata`, `dxgi_format`, `dxgi_format_name`, and `ba2_archive::texture_metadata(path)`.
- Extend or split `src/ba2_reader.cpp` so DX10 parsing can share safe BA2 helpers without tangling all texture behavior into the GNRL path.
- Add internal reusable texture helpers under `src/texture/` or a similarly private area for DDS reconstruction and DirectXTex validation.
- Add generated BA2 DDS fixture helpers under `tests/` and wire focused BA2 DDS tests explicitly in `CMakeLists.txt`.
- Extend `tests/public_header_smoke.cpp` to prove texture metadata APIs compile without DirectXTex, Windows SDK, libdeflate, LZ4, Delphi, TES5Edit, or platform headers.
- Keep `TES5Edit/` read-only: reference tracing is allowed, but no edits, formatting, source-list inclusion, staging, commits, or submodule pointer changes.

</code_context>

<specifics>
## Specific Ideas

- Texture metadata should be a clear BA2 archive capability, not a generic archive entry field.
- Public DXGI representation should preserve exact numeric identity while staying independent of Windows and DirectXTex headers.
- Chunk metadata should be logical and consumer-useful: offsets, sizes, and mip ranges when reliable, not a raw BA2 record dump.
- Extraction safety is more important than streaming optimization in this phase: complete DDS bytes are built and validated before sink writes.
- Generated DDS fixture helpers should be source-reviewable and reusable for later writer phases without becoming public API.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 07-ba2-dds-read-and-dds-reconstruction*
*Context gathered: 2026-05-05T21:23:25.3309455-07:00*
