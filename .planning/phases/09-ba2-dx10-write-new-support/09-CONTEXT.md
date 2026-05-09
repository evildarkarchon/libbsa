# Phase 09: ba2-dx10-write-new-support - Context

**Gathered:** 2026-05-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 9 adds public write-new support for Fallout 4 and Starfield BA2 `DX10` texture archives from DDS host files. Consumers add DDS files through a dependency-light C++20 writer, libbsa analyzes and snapshots valid DDS bytes internally, plans BA2-compatible mip/chunk records, writes compressed texture chunks, optionally deduplicates eligible stored chunks, publishes safely through the established write-new flow, and proves output by reopening and extracting through public reader APIs plus DirectXTex validation.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**11 requirements are locked.** See `09-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `09-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**Important correction from discussion:** `09-SPEC.md` Requirement 7 currently requires raw/compressed overrides for entries or chunks. The user corrected this: BA2 DX10 writer output must be public-API compressed-only at archive level because uncompressed DX10 BA2 archives are unstable in the game engine. Researcher/planner MUST update or explicitly account for this SPEC correction before planning. Do not plan public entry-level or chunk-level compression overrides for DX10.

**In scope (from SPEC.md):**
- Public BA2 DX10 write-new API for DDS host-file entries.
- Mandatory FO4 v1 and Starfield v3 DX10 target profiles.
- DirectXTex-backed internal DDS analysis translated into libbsa-owned metadata.
- Support for the locked common DDS format set: `BC1_UNORM`, `BC1_UNORM_SRGB`, `BC3_UNORM`, `BC4_UNORM`, `BC5_UNORM`, `BC5_SNORM`, `BC6H_UF16`, `BC7_UNORM`, `R8G8B8A8_UNORM_SRGB`, `B8G8R8A8_UNORM`, `R8_UNORM`, and `R8G8B8A8_SNORM`.
- BA2 DX10 header, texture record, chunk record, payload, and filename-table serialization.
- Mip, array, cubemap, and configurable chunk-limit planning for supported DDS inputs.
- Target-default compression for generated texture chunks, corrected by this CONTEXT to compressed-only public output.
- FO4 deflate chunk writing and Starfield v3 method `3` raw-LZ4-block chunk writing.
- Opt-in final-stored-byte chunk payload deduplication.
- Safe write-new filesystem finalization matching Phase 8 publish safety expectations.
- Reader-backed tests that reopen writer output, inspect metadata, list/find/contains entries, extract DDS bytes, and validate those bytes through DirectXTex.
- Generated legal DDS source fixtures, BA2 writer outputs, and manifests outside `TES5Edit/`.

**Out of scope (from SPEC.md):**
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

</spec_lock>

<decisions>
## Implementation Decisions

### Public Writer Surface
- **D-01:** Mirror the existing BA2 GNRL writer shape with a dedicated BA2 DX10 writer object. Expected shape is target/options construction, explicit archive-internal paths, DDS host-file add methods, and `write_to`-style host-path finalization.
- **D-02:** DDS host-file validation and DirectXTex analysis happen at add time, not only during `write_to`. Empty source paths, missing/unreadable files, malformed DDS files, and unsupported DDS formats should fail through structured `result` errors when the DDS file is added.
- **D-03:** After successful add-time validation, snapshot the DDS bytes into writer-owned state. Later source-file changes or deletion must not change the eventual archive output.
- **D-04:** Keep the public input surface DDS-host-file-only. The public API does not need memory-buffer DDS input in Phase 9, even though the implementation stores validated DDS bytes internally.
- **D-05:** Correct the SPEC compression override language: public BA2 DX10 writer output is compressed-only at archive level. Do not expose entry-specific or chunk-specific raw/compressed overrides because uncompressed DX10 BA2 archives are unstable in the game engine.
- **D-06:** Preserve the established dependency-light public boundary: no public DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, private parser, private codec, or C++23 `std::expected` types.

### Compression And Metadata
- **D-07:** The public DX10 writer exposes compressed-only behavior. FO4 compressed chunks route to deflate. Starfield v3 compressed chunks route through the archive-level Starfield `CompressionMethod`, with method `3` as the default raw LZ4 block path and method `0` available where valid by option.
- **D-08:** Starfield BA2 DX10 header options mirror BA2 GNRL options: deterministic defaults for `Unknown1`, `Unknown2`, and v3 `CompressionMethod`, with archive-level overrides where valid.
- **D-09:** The BA2 DX10 texture record `unknown_tex` byte is writer-owned and default-only in the public API. Researcher/planner should trace reference behavior for the default, but do not expose a caller override in Phase 9.
- **D-10:** Cubemap and array record fields are derived from DirectXTex DDS metadata and known BA2 constants. Do not expose raw public overrides for `cube_maps_raw` or array interpretation in Phase 9.
- **D-11:** Compression selection remains metadata-driven and target-routed. Do not infer compression, target version, or texture archive behavior from host filename extensions.

### Chunk Planning
- **D-12:** Expose one archive-wide max decoded chunk-byte control for chunk planning. Do not expose max-mips or named policy controls as the primary public knob.
- **D-13:** Chunk planning configuration is archive-wide only. Do not expose per-entry chunk-limit overrides in Phase 9.
- **D-14:** Default chunking should be reference-derived. Researcher/planner must trace BSArchPro/xEdit and known BA2 behavior first; if evidence is inconclusive, use conservative contiguous mip groups and document the fallback.
- **D-15:** For array textures and cubemaps, use a uniform chunk pattern across every slice or face. The same mip-range split should repeat per slice/face so validation remains predictable.
- **D-16:** Chunk planning must preserve the locked no-transform rule: no resizing, transcoding, mip generation, repair, or image-data reordering beyond BA2-required chunk serialization.

### Dedupe Granularity
- **D-17:** DX10 deduplication remains an archive-level boolean option, disabled by default, matching the TES4-family and BA2 GNRL writer pattern.
- **D-18:** When enabled, dedupe is archive-wide. Any eligible chunk in the archive may share a payload offset while each texture record, chunk record, and filename-table entry remains distinct.
- **D-19:** Dedupe runs after chunk planning and archive-level compression. Chunk splitting must not change to improve dedupe opportunities.
- **D-20:** Dedupe eligibility requires matching final stored bytes plus matching chunk metadata for raw size, stored size, and compression route. It does not require matching texture dimensions, DXGI format, cubemap/array state, or mip range if the eligible chunk bytes and chunk sizes/compression match.
- **D-21:** The first eligible chunk in deterministic record/chunk order owns the physical payload bytes. Later eligible duplicate chunks reuse that payload offset.
- **D-22:** Complete duplicate DDS entries may share every eligible chunk offset when dedupe is enabled, while still preserving separate texture records and names.
- **D-23:** Tests must prove disabled output has distinct eligible offsets, enabled output shares eligible offsets, and both modes reopen and extract valid DDS bytes.

### DDS Proof Matrix
- **D-24:** Split the locked DDS format set across FO4 and Starfield v3 writer tests. Every locked format must appear at least once across the two targets, and each target still needs representative success coverage.
- **D-25:** Use dedicated structural DDS cases for multi-mip, array, and cubemap behavior rather than one mega-texture or making every format structurally complex.
- **D-26:** Commit generated legal DDS source fixtures and manifests outside `TES5Edit/`, matching prior fixture policy. Runtime-only generation is not the primary proof path.
- **D-27:** Writer-produced extracted DDS success is proven by DirectXTex-loaded metadata matching the source plus source image payload bytes matching after DDS-order extraction. Full DDS byte equality is not required because extraction may canonicalize DDS headers.
- **D-28:** Unsupported DDS formats and malformed DDS files fail before final archive output is finalized. Because DDS bytes are validated at add time, these should normally fail during add-file calls with stable structured errors.
- **D-29:** Keep existing public include-boundary tests green after adding DX10 writer APIs. Public headers must not transitively include DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, or private implementation headers.

### the agent's Discretion
- Researcher/planner may choose exact public names for the DX10 target enum, options struct, writer class, and add method, provided the surface mirrors the established BA2 GNRL writer shape and stays DDS-host-file-only.
- Researcher/planner may choose exact private file layout, CMake registration, helper decomposition, and Catch2 test file organization if the public dependency boundary remains clean.
- Researcher/planner may choose exact generated DDS dimensions, filenames, archive paths, and distribution of locked formats between FO4 and Starfield v3, provided the DDS proof matrix decisions above are satisfied.
- Researcher/planner must choose exact reference-derived defaults for chunking, `unknown_tex`, `Unknown1`, `Unknown2`, and Starfield `CompressionMethod` after tracing reference code and must document non-obvious compatibility constraints near the implementation.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md` - Locked Phase 9 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning, but apply this CONTEXT's compression correction to Requirement 7.
- `.planning/ROADMAP.md` - Phase 9 goal, mapped requirements WBA2-06 through WBA2-11, success criteria, and Phase 10/11/12 boundaries.
- `.planning/REQUIREMENTS.md` - BA2 DX10 write requirements plus public API, compression, fixture, compatibility, validation, and out-of-scope constraints.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, key decisions, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from prior phases.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, DDS write scope, streaming goals, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/08-ba2-gnrl-write-new-support/08-CONTEXT.md` - BA2 writer object shape, Starfield header option defaults/overrides, archive-level compression routing, end filename tables, safe publish, dedupe, and reader-backed validation patterns to carry into DX10 writer work.
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md` - Writer API shape, write-time validation patterns, compression policy history, final-stored-byte dedupe, safe output behavior, and public dependency boundary decisions.
- `.planning/phases/06-dds-boundary-and-ba2-dx10-read-reconstruction/06-CONTEXT.md` - BA2 DX10 metadata surface, DirectXTex-private boundary, DDS reconstruction contract, validated chunk ordering, generated fixture policy, and Phase 9 handoff.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for BA2 archive structures, DX10 record/chunk handling, Starfield fields, and compression-method behavior; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for archive parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for pack/write option behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit, format, stage, compile, or use as fixture workspace.

### External Format And DDS References
- `https://miere.ru/posts/ba2-archive-format/` - Public BA2 `BTDX`, `GNRL`/`DX10`, header, record, payload, and filename-table notes useful for cross-checking reference tracing.
- `https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html` - Independent BA2/BTDX structure reference, including GNRL vs DX10 split and parser behavior.
- `https://github.com/wrye-bash/wrye-bash/issues/667` - Starfield BA2 discussion noting v2/v3 header and raw LZ4 block distinction from SSE LZ4 frame behavior.
- `https://learn.microsoft.com/windows/win32/direct3ddds/dx-graphics-dds-pguide` - Microsoft DDS layout, DXT10 header, pitch/block sizing, arrays, and validation guidance.
- `https://learn.microsoft.com/windows/win32/direct3ddds/dds-file-layout-for-cubic-environment-maps` - Microsoft cubemap face order and mip ordering guidance.
- `https://github.com/microsoft/DirectXTex/wiki/TexMetadata` - DirectXTex metadata fields to translate into libbsa-owned values behind the private boundary.
- `https://github.com/Microsoft/DirectXTex/wiki/DDS-I-O-Functions` - DirectXTex DDS metadata/load validation behavior for generated source and extracted DDS outputs.

### Current Codebase State
- `include/libbsa/writer.hpp` - Existing public TES4-family and BA2 GNRL writer surfaces, target/options patterns, compression policy enums, dedupe option, and safe public doc-comment style to mirror for BA2 DX10.
- `include/libbsa/libbsa.hpp` - Umbrella public include that must expose the BA2 DX10 writer surface once added.
- `include/libbsa/archive.hpp` - Public archive metadata, BA2 metadata optionals, texture metadata, texture chunk metadata, entry compression enum, and reader facade used for writer-output validation.
- `src/formats/ba2/ba2_gnrl_writer.cpp` - Existing BA2 writer-owned state, validation, compression routing, end filename table serialization, final-stored-byte dedupe, unique temporary publish directory, overwrite backup/rollback, and comments documenting compatibility constraints.
- `src/formats/ba2/ba2_gnrl_writer.hpp` - Private BA2 GNRL writer entry shape and `write_ba2_gnrl_archive` internal API that Phase 9 can mirror with a DX10-specific writer entry.
- `src/formats/ba2/ba2_dx10_parser.hpp` and `src/formats/ba2/ba2_dx10_parser.cpp` - Existing BA2 DX10 header/record/chunk/name parsing, fixed 24-byte chunk header rule, `BAADF00D` sentinel validation, texture metadata materialization, filename-table validation, and chunk compression classification.
- `src/formats/ba2/ba2_dx10_reader.hpp` and `src/formats/ba2/ba2_dx10_reader.cpp` - Existing BA2 DX10 entries/find/contains/extraction helper pattern and exact-size chunk extraction used as the round-trip oracle.
- `src/texture/dds_layout.hpp` and `src/texture/dds_layout.cpp` - Existing DDS DXT10 header builder and metadata-only chunk coverage/order validator. Phase 9 must extend supported format sizing beyond Phase 6 fixture-backed formats.
- `src/texture/directxtex_analyzer.hpp` and `src/texture/directxtex_analyzer.cpp` - Existing private DirectXTex metadata boundary. Phase 9 must extend this or add adjacent helpers to snapshot/analyze source DDS bytes without leaking DirectXTex types.
- `src/detail/archive_path.hpp` and `src/detail/archive_path.cpp` - Existing archive virtual path normalization and validation rules for canonical keys and duplicate detection.
- `src/detail/bethesda_hash.hpp` and `src/detail/bethesda_hash.cpp` - Existing FO4/BA2 hash helper expected to compute DX10 name/directory hash fields.
- `src/detail/binary_io.hpp` and `src/detail/binary_io.cpp` - Checked little-endian binary primitives for safe DX10 serialization.
- `src/detail/compression_router.hpp` and `src/detail/compression_router.cpp` - Explicit compression routing for `deflate` and raw `lz4_block`; DX10 writer must not route Starfield method `3` through LZ4 frame.
- `src/detail/deflate_codec.hpp` and `src/detail/deflate_codec.cpp` - Deflate codec for FO4 DX10 compressed chunks and Starfield method `0` where valid.
- `src/detail/lz4_block_codec.hpp` and `src/detail/lz4_block_codec.cpp` - Raw LZ4 block codec for Starfield v3 method `3` compressed DX10 chunks.
- `tests/unit/ba2_gnrl_writer_tests.cpp` - Existing writer-output tests for add validation, safe publish, Starfield options, compression routing, dedupe offsets, and reader-backed round trips to mirror for DX10.
- `tests/unit/public_include_boundary_tests.cpp` - Public dependency boundary tests that must stay green after adding BA2 DX10 writer APIs.
- `tests/CMakeLists.txt` - Catch2, generated fixture, and CTest label wiring for adding DX10 writer tests and fixture generation.
- `CMakeLists.txt` - Library public header file set and private source registration for adding DX10 writer implementation files.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ba2_gnrl_writer` already provides the closest public/private writer pattern: dedicated writer object, writer-owned state, explicit archive paths, options, safe publish, compression routing, dedupe, and reader-backed tests.
- `archive_compression_policy` and `entry_compression_policy` exist, but Phase 9 should not blindly reuse entry-level compression controls for DX10 because the user corrected DX10 to compressed-only public output.
- `detail::normalize_archive_path` provides canonical lowercase `/` path validation while preserving original archive spelling for filename-table serialization.
- `detail::hash_fo4` or existing FO4/BA2 hash support should compute DX10 record name and directory hashes. Callers should not provide hash overrides.
- `detail::binary_writer`, `detail::compress_payload`, `deflate`, and raw `lz4_block` adapters support the needed serialization and target-routed compressed chunk encoding.
- `texture::analyze_dds_metadata`, `texture::build_dds_dxt10_header`, and `texture::validate_and_order_chunks` are the existing texture boundary pieces Phase 9 should extend for DDS source analysis, format sizing, chunk planning, and output validation.

### Established Patterns
- Public APIs live under `include/libbsa/` and use Doxygen-style `///` comments for public types and methods.
- Format-specific implementation lives under `src/formats/ba2/`, with texture helpers under `src/texture/` and shared primitives under `src/detail/`.
- Public APIs return `libbsa::result<T>` for expected I/O, validation, and format failures; tests assert stable `error_code` values rather than diagnostic text.
- Writer-output acceptance is reader-backed: create archive, reopen through `archive_reader`, inspect public metadata, list/find/contains entries, extract bytes, and verify physical layout facts only where acceptance requires them.
- Generated fixtures and manifests are repository-owned, legal, and outside `TES5Edit/`; `TES5Edit/` remains read-only reference material.
- Safe publish uses unique sibling temporary directories and backup/rollback for explicit overwrite mode, established in Phase 8.

### Integration Points
- Add a BA2 DX10 writer public surface in `include/libbsa/writer.hpp` or a new public writer header included by `include/libbsa/libbsa.hpp`.
- Add private DX10 writer implementation under `src/formats/ba2/`, likely mirroring `ba2_gnrl_writer` while using DX10 texture record/chunk serialization.
- Extend DirectXTex analyzer and DDS layout helpers to cover the locked common-game DXGI format set and source DDS payload slicing.
- Extend or add tests in `tests/unit/` for public API construction, add-time DDS validation/snapshotting, safe publish, FO4/SFv3 compressed output, Starfield options, chunk limits, dedupe, and reader-backed DirectXTex validation.
- Extend fixture generation to create committed legal DDS source fixtures and manifests covering the locked format set plus dedicated multi-mip, array, cubemap, unsupported-format, and malformed-source cases.
- Keep public include boundary, BA2 DX10 reader/extraction, BA2 GNRL writer/reader, TES4-family writer, cross-format tests, and `git -C TES5Edit status --short` clean after Phase 9 work.

</code_context>

<specifics>
## Specific Ideas

- User-supplied compatibility constraint: uncompressed BA2 DX10 archives are unstable in the game engine, so public Phase 9 DX10 writing must be compressed-only even though the current SPEC says raw/compressed overrides.
- The writer should validate and snapshot DDS files at add time, which intentionally differs from the BA2 GNRL path-only disk-source behavior.
- Chunk planning should be simple for consumers: one archive-wide max decoded chunk-byte limit plus reference-compatible defaults.
- DirectXTex validation should compare metadata and image payload bytes, not full DDS files, because libbsa may canonicalize reconstructed DDS headers during extraction.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope. The SPEC Requirement 7 correction is an in-scope planning prerequisite, not a deferred idea.

</deferred>

---

*Phase: 09-ba2-dx10-write-new-support*
*Context gathered: 2026-05-09*
