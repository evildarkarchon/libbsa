# Phase 06: DDS Boundary and BA2 DX10 Read/Reconstruction - Context

**Gathered:** 2026-05-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 6 turns BA2 `DX10` texture archives from a recognized-but-unsupported subtype into readable archives through the existing `archive_reader` facade. Consumers must be able to open generated Fallout 4 and Starfield v3 BA2 DX10 archives, inspect dependency-light libbsa-owned texture metadata, and extract entries as valid DDS byte streams while DirectXTex remains a private analyzer/validation dependency.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `06-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `06-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- BA2 `DX10` detection/open routing for generated Fallout 4 and Starfield v3 texture archives.
- BA2 DX10 header, record, chunk table, filename/path, and texture metadata parsing needed for read/extract behavior.
- Library-owned texture metadata sufficient to represent dimensions, mip count, DXGI format identifier, array/cubemap state, and chunk layout.
- DDS header reconstruction for extracted BA2 DX10 entries.
- Sink-based and `extract_bytes` extraction of BA2 DX10 entries as complete DDS byte streams.
- Raw, deflate, and Starfield raw-LZ4-block DX10 chunk extraction with exact-size validation.
- DirectXTex-backed validation or analysis behind an internal boundary.
- Generated legal FO4/Starfield DX10 fixtures and malformed cases covering metadata, extraction, compression, cubemaps, and arrays.
- Regression coverage proving existing TES3, TES4-family BSA, BA2 GNRL, public include boundary, and TES5Edit read-only constraints remain intact.

**Out of scope (from SPEC.md):**
- BA2 DX10 write-new support - Phase 9 owns creating texture BA2 archives from DDS input.
- BA2 GNRL write-new support - Phase 8 owns general BA2 archive creation.
- TES4-family or TES3 BSA writing - later writer phases own BSA creation.
- Broad compatibility warnings or lenient recovery for malformed texture archives - Phase 11 owns validation API and hardening beyond focused fail-closed parser checks.
- Performance benchmarking, parallel extraction, or bulk-concurrency guarantees - Phase 12 owns performance and concurrency work.
- Real game archive fixtures as required evidence - generated legal fixtures are required; local game fixtures remain optional.
- Public exposure of DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, or private parser/codec/texture types - public headers must remain dependency-light.
- Editing, formatting, compiling, staging, or using `TES5Edit/` as a fixture workspace - it remains read-only reference material.
- In-place archive mutation - v1 read/write sequencing excludes mutation of existing archives.

</spec_lock>

<decisions>
## Implementation Decisions

### Texture Metadata Surface
- **D-01:** Expose BA2 DX10 texture metadata as a DX10-only optional value on `entry_metadata`, not as a separate public reader object or texture-specific lookup method.
- **D-02:** Keep the public texture metadata dependency-light and minimal. Add the required value structs now, but avoid public helper methods, display names, or large enums that can wait for documentation or validation phases.
- **D-03:** Expose the texture format as a raw numeric DXGI format identifier (`uint32_t` or equivalent), not as a public DXGI type, DirectXTex type, or incomplete libbsa enum.
- **D-04:** Represent cubemap and array state with explicit fields such as dimensions, mip count, array size, and `is_cubemap`. Do not require callers to interpret DDS flags just to answer common questions.
- **D-05:** Expose a public per-entry chunk list for BA2 DX10 entries. Chunk metadata should include enough libbsa-owned information to understand chunk layout, archive-absolute payload offsets, stored/raw sizes, and compression route without exposing private parser structs.
- **D-06:** For BA2 DX10 entries, `entry_metadata::raw_size` should mean the consumer-visible extracted DDS byte size, including the reconstructed DDS header. Archive chunk metadata carries archive payload sizes separately.
- **D-07:** Preserve archive-derived raw DX10 record fields in clearly named libbsa-owned metadata when they affect reconstruction or future compatibility evidence. Keep fields with unknown semantics raw and do not invent meaning before reference evidence exists.
- **D-08:** Populate the optional texture metadata only for BA2 DX10 entries. Existing BSA and BA2 GNRL entries keep `std::nullopt` semantics.

### DDS Output Contract
- **D-09:** Reconstructed BA2 DX10 extraction output should always use standard DDS magic/header plus a `DDS_HEADER_DXT10` extension. Do not spend Phase 6 effort choosing legacy DDS header encodings for formats that could technically fit.
- **D-10:** DirectXTex validation in automated tests must load every generated reconstructed DDS output and report manifest-matching dimensions, mip count, raw DXGI format id, array size, and cubemap state.
- **D-11:** Tests should also verify that decoded payload bytes after the reconstructed header match the expected fixture payload bytes in validated DDS order. Do not require whole-file byte equality for header padding/details beyond the contract needed for metadata and payload proof.
- **D-12:** Normal runtime extraction should not call DirectXTex before returning success. Extraction builds deterministic DDS bytes; DirectXTex remains an internal analyzer/validation dependency used by implementation/test boundaries, not a per-extract runtime gate.
- **D-13:** Sink-based extraction writes the reconstructed DDS header first, then decoded chunk payloads in validated DDS order. Preserve sink-first semantics and keep buffering bounded per texture chunk rather than assembling the whole DDS before writing.
- **D-14:** `extract_bytes(path)` for BA2 DX10 returns exactly the same reconstructed DDS byte stream that `extract(path, sink)` writes, including header and decoded chunks.
- **D-15:** Validate BA2 chunk metadata against the expected DDS mip/array/face order and assemble output in computed DDS order. Do not blindly concatenate archive chunk order if it conflicts with the expected texture layout.
- **D-16:** Malformed DX10 layout problems that can be detected from metadata - truncated tables, invalid spans, inconsistent counts/sizes, mip/face gaps, duplicate coverage, or impossible coverage - should fail during `archive_reader::open` with `error_code::format_error`.
- **D-17:** Bad compressed chunks or exact decoded-size mismatches remain malformed archive content and should return `error_code::format_error`, matching BA2 GNRL behavior.
- **D-18:** DDS DXT10 fields that are not clearly represented in BA2 records should use conservative documented defaults. Researcher/planner should trace reference behavior where available, but Phase 6 should not block on exact reproduction of unclear header metadata that DirectXTex does not require for the locked acceptance criteria.
- **D-19:** Preserve BA2 GNRL path behavior for DX10 entries: canonical lowercase `/` lookup keys, `original_path` preservation, duplicate canonical path rejection, and `not_found` for valid missing paths.

### Fixture Coverage Shape
- **D-20:** Use a small rich generated success matrix rather than one huge archive or many one-case archives. The matrix must cover generated Fallout 4 and Starfield v3 DX10 archives, raw chunks, FO4 deflate chunks, Starfield raw-LZ4-block chunks, cubemap state, array state, and representative mip/chunk layouts.
- **D-21:** Generated texture payloads should be tiny, deterministic, legal, and DirectXTex-loadable after DDS reconstruction. Prioritize compact metadata-valid textures over broad production-like format coverage.
- **D-22:** Mandatory malformed fixture coverage should be focused on the Phase 6 SPEC: truncated DX10 headers/records/chunks, invalid payload spans, inconsistent chunk sizes/counts, bad compressed chunks, decoded-size mismatches, and mip/face coverage gaps or duplicates.
- **D-23:** DX10 fixture manifests should be rich dual metadata manifests. Record archive-side records/chunks, expected public texture metadata, expected DirectXTex-loaded DDS metadata, compression route, payload ordering, and decoded payload hashes or bytes.
- **D-24:** Fixture generation code and committed outputs stay under project test fixture tooling, outside `TES5Edit/`, with legal provenance documented like prior generated fixture phases.

### Analyzer Boundary Reuse
- **D-25:** Add a reusable internal texture-layout/analyzer core in Phase 6. It should support this phase's DDS reconstruction and validation work while giving Phase 9 a clean place to extend writer-side DDS input analysis later.
- **D-26:** Wire DirectXTex through CMake as a private `libbsa` dependency behind the internal analyzer boundary. Installed public headers must remain free of DirectXTex, DXGI, Windows SDK, and other private dependency types.
- **D-27:** Internal analyzer APIs should translate DirectXTex metadata immediately into libbsa-owned internal/public-compatible texture layout values. Do not pass DirectXTex types across module seams.
- **D-28:** Do not implement Phase 9 writer behavior in Phase 6. Names and data shapes may be reusable, but Phase 6 must not parse DDS input for BA2 writing or generate BA2 texture chunks from DDS input.

### Carry-Forward Decisions
- **D-29:** Preserve the established public `archive_reader` facade: `open`, `metadata`, deterministic `entries`, normalized `find`/`contains`, `extract(path, sink)`, and `extract_bytes(path)`.
- **D-30:** Preserve sink-first extraction and one-entry/one-chunk bounded buffering. Whole-archive reads and broad performance/concurrency work remain out of scope.
- **D-31:** Preserve metadata-driven compression routing. Raw, deflate, and Starfield v3 raw-LZ4-block decisions must come from parsed archive/chunk metadata, never host file extension or archive path spelling.
- **D-32:** Preserve exact-size decompression validation for each compressed DX10 chunk.
- **D-33:** Preserve stable error-code assertions in tests. Do not assert diagnostic message text.
- **D-34:** Preserve the TES5Edit boundary: trace reference behavior as needed, but do not edit, format, stage, compile, or use `TES5Edit/` as a fixture workspace.

### the agent's Discretion
- Researcher/planner may choose exact internal file names, helper boundaries, parser structs, and CMake target organization if the decisions above and `06-SPEC.md` are preserved.
- Researcher/planner should trace TES5Edit/BSArchPro and independent BA2/DDS references for DX10 record/chunk layout, mip/array/cubemap ordering, and DDS header reconstruction details before locking implementation tasks.
- Planner may choose exact fixture filenames, entry names, texture dimensions, mip counts, and payload bytes, as long as the small rich matrix and malformed coverage decisions above are satisfied.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/06-dds-boundary-and-ba2-dx10-read-reconstruction/06-SPEC.md` - Locked Phase 6 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` - Phase 6 goal, mapped requirements DDS-01 through DDS-07, success criteria, and downstream Phase 9 boundary.
- `.planning/REQUIREMENTS.md` - BA2 DDS read requirements plus public API, compression, fixture, compatibility, and validation constraints.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from prior phases.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, streaming goals, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/05-ba2-gnrl-read-extract/05-CONTEXT.md` - BA2 detection, metadata optionals, generated BA2 fixture policy, DX10 handoff, v3 compression-method policy, and BA2 GNRL reader/extractor patterns carried into Phase 6.
- `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md` - Archive-absolute public offsets, strict duplicate-path handling, generated fixture proof strategy, and variant-specific reader dispatch patterns.
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md` - Public reader surface, canonical/original path semantics, deterministic listings, sink-first extraction, `not_found` behavior, and generated fixture manifest policy.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for BA2 archive structures, extraction behavior, and Starfield-specific fields; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for archive parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit or use as fixture workspace.

### External Format And Library References
- `https://github.com/microsoft/DirectXTex/wiki/TexMetadata` - DirectXTex metadata fields to translate into libbsa-owned values behind the private boundary.
- `https://github.com/Microsoft/DirectXTex/wiki/DDS-I-O-Functions` - DirectXTex DDS load/metadata validation behavior for generated reconstructed outputs.
- `https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10` - DDS DXT10 header fields: DXGI format, resource dimension, misc flags, array size, and alpha metadata.
- `https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide` - Microsoft DDS file format guidance for header and payload expectations.
- `https://miere.ru/posts/ba2-archive-format/` - Independent BA2 `BTDX`, `GNRL`/`DX10`, filename-table, packed-length, and record-layout notes useful for cross-checking reference tracing.
- `https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html` - Independent BA2/BTDX structure reference, including GNRL vs DX10 split and DDS reconstruction boundary.

### Current Codebase State
- `include/libbsa/archive.hpp` - Public `archive_reader`, `archive_metadata`, `entry_metadata`, `ba2_archive_metadata`, `entry_compression`, and `payload_sink` surface that Phase 6 extends without leaking parser/dependency types.
- `src/archive.cpp` - Current archive open/list/find/contains/extract dispatch; BA2 dispatch currently routes all BA2 archives through GNRL parser/extractor after detector acceptance.
- `src/formats/ba2/ba2_format_detector.cpp` - Current byte-driven BA2 detector recognizes `DX10` but returns `error_code::unsupported`; Phase 6 changes this handoff.
- `src/formats/ba2/ba2_gnrl_parser.cpp` - Existing bounded BA2 metadata/name-table parse pattern and Starfield v2/v3 header handling to mirror for DX10.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - Existing BA2 entries/find/contains/extraction helper pattern, including sink-first raw streaming, compressed payload buffering, exact-size decompression, and stable error behavior.
- `src/detail/compression_router.hpp` and `src/detail/compression_router.cpp` - Internal explicit compression method enum and exact-size decompression API; DX10 chunks should map parsed metadata to this router.
- `src/detail/lz4_block_codec.hpp` and `src/detail/lz4_block_codec.cpp` - Raw LZ4 block codec used by Starfield BA2 v3 method `3`.
- `src/detail/deflate_codec.hpp` and `src/detail/deflate_codec.cpp` - Exact-size deflate codec used by FO4 BA2 and observed non-LZ4 Starfield paths.
- `src/detail/archive_path.hpp` and `src/detail/archive_path.cpp` - Existing archive virtual path normalization rules for canonical lookup and duplicate detection.
- `CMakeLists.txt` - Library source registration and private dependency linkage; currently lacks `find_package`/private link wiring for DirectXTex.
- `tests/CMakeLists.txt` - Existing Catch2/nlohmann-json test wiring, generated fixture tool targets, and `catch_discover_tests` labels; Phase 6 adds DX10 generator/tests here.
- `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` - Existing BA2 generator style, manifest shape, compression helper reuse, malformed fixture approach, and provenance pattern.
- `tests/unit/ba2_gnrl_reader_tests.cpp` - Existing BA2 metadata, lookup, extraction, malformed, and manifest-driven assertion patterns to extend or mirror for DX10.
- `tests/unit/public_include_boundary_tests.cpp` - Public dependency boundary tests that must continue to reject DirectXTex/Windows/private dependency leakage after adding texture metadata.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `libbsa::archive_reader` already exposes the public read surface Phase 6 should reuse: `open`, `metadata`, `entries`, `find`, `contains`, `extract`, and `extract_bytes`.
- `archive_metadata` already has `archive_type::ba2`, `archive_variant::fallout4`, `archive_variant::starfield`, version, flags, file count, default compression, and optional `ba2_archive_metadata`.
- `entry_metadata` already carries canonical/display path data, consumer-visible sizes, archive-absolute payload offset, archive hash, compression mode, record flags, and embedded-name flags. Phase 6 adds a DX10-only optional texture metadata value rather than replacing this shape.
- `detail::normalize_archive_path` already provides lowercase forward-slash archive key semantics and invalid-path rejection for public lookup/extraction inputs.
- `detail::decompress_payload_exact` already routes `none`, `deflate`, and `lz4_block` with exact-size validation needed by BA2 DX10 chunk extraction.
- Existing generated fixture tooling already writes tiny legal archives plus JSON manifests and uses test-only nlohmann-json without leaking runtime/public dependencies.

### Established Patterns
- Public headers remain under `include/libbsa/`; dependency-bearing and parser-specific code stays under `src/` or `src/detail/`.
- Public APIs use Doxygen-style `///` comments; non-obvious BA2/DDS compatibility rules should be documented near implementation/tests with TES5Edit/BSArchPro reference context.
- Tests assert stable `error_code` values and public metadata, not diagnostic message text.
- Generated fixtures must document legal provenance and must not use `TES5Edit/` as a workspace.
- Catch2 tags become CTest labels through existing `catch_discover_tests(... ADD_TAGS_AS_LABELS)` wiring.
- DirectXTex is already in `vcpkg.json`, but CMake currently links only libdeflate and lz4 into `libbsa`.

### Integration Points
- Update BA2 byte detection so `BTDX`/`DX10` routes to DX10 parsing for supported FO4 and Starfield v3 profiles instead of returning Phase 5's unsupported error.
- Add BA2 DX10 parser/reader internals alongside `src/formats/ba2/ba2_gnrl_*` or under a clearer BA2 texture split if planner prefers.
- Add an internal `src/texture/` or equivalent DirectXTex adapter/layout boundary that returns libbsa-owned values and is linked privately.
- Extend `archive_reader::open`, `entries`, `find`, `contains`, and `extract` dispatch so BA2 GNRL and BA2 DX10 use separate private helpers under the same public facade.
- Add generated BA2 DX10 fixture tooling and manifest-driven tests to `tests/CMakeLists.txt`.
- Keep public include boundary and installed-package smoke tests green after adding texture metadata and private DirectXTex linkage.

</code_context>

<specifics>
## Specific Ideas

- The texture metadata should be pleasant for consumers but conservative: optional on entries, explicit dimensions/mips/array/cubemap fields, raw DXGI id, and public chunk metadata without DirectXTex/DXGI types.
- Reconstructed DDS output should be deterministic and DirectXTex-loadable, but Phase 6 does not need to prove byte-for-byte equivalence to an external DDS writer for header padding/details.
- The fixture set should be compact and high-signal: tiny legal textures, FO4 plus Starfield v3, raw/deflate/raw-LZ4 chunks, cubemap/array cases, and focused malformed layout/decode cases.
- The internal analyzer boundary should be reusable for Phase 9, but Phase 6 should not implement writer behavior or DDS-input-to-BA2 chunk generation.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction*
*Context gathered: 2026-05-08*
