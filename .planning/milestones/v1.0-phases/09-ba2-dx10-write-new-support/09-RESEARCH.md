# Phase 09: ba2-dx10-write-new-support - Research

**Researched:** 2026-05-09  
**Domain:** BA2 DX10/DDS texture write-new support in a C++20 archive library  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Public Writer Surface
- **D-01:** Mirror the existing BA2 GNRL writer shape with a dedicated BA2 DX10 writer object. Expected shape is target/options construction, explicit archive-internal paths, DDS host-file add methods, and `write_to`-style host-path finalization.
- **D-02:** DDS host-file validation and DirectXTex analysis happen at add time, not only during `write_to`. Empty source paths, missing/unreadable files, malformed DDS files, and unsupported DDS formats should fail through structured `result` errors when the DDS file is added.
- **D-03:** After successful add-time validation, snapshot the DDS bytes into writer-owned state. Later source-file changes or deletion must not change the eventual archive output.
- **D-04:** Keep the public input surface DDS-host-file-only. The public API does not need memory-buffer DDS input in Phase 9, even though the implementation stores validated DDS bytes internally.
- **D-05:** Correct the SPEC compression override language: public BA2 DX10 writer output is compressed-only at archive level. Do not expose entry-specific or chunk-specific raw/compressed overrides because uncompressed DX10 BA2 archives are unstable in the game engine.
- **D-06:** Preserve the established dependency-light public boundary: no public DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, private parser, private codec, or C++23 `std::expected` types.

#### Compression And Metadata
- **D-07:** The public DX10 writer exposes compressed-only behavior. FO4 compressed chunks route to deflate. Starfield v3 compressed chunks route through the archive-level Starfield `CompressionMethod`, with method `3` as the default raw LZ4 block path and method `0` available where valid by option.
- **D-08:** Starfield BA2 DX10 header options mirror BA2 GNRL options: deterministic defaults for `Unknown1`, `Unknown2`, and v3 `CompressionMethod`, with archive-level overrides where valid.
- **D-09:** The BA2 DX10 texture record `unknown_tex` byte is writer-owned and default-only in the public API. Researcher/planner should trace reference behavior for the default, but do not expose a caller override in Phase 9.
- **D-10:** Cubemap and array record fields are derived from DirectXTex DDS metadata and known BA2 constants. Do not expose raw public overrides for `cube_maps_raw` or array interpretation in Phase 9.
- **D-11:** Compression selection remains metadata-driven and target-routed. Do not infer compression, target version, or texture archive behavior from host filename extensions.

#### Chunk Planning
- **D-12:** Expose one archive-wide max decoded chunk-byte control for chunk planning. Do not expose max-mips or named policy controls as the primary public knob.
- **D-13:** Chunk planning configuration is archive-wide only. Do not expose per-entry chunk-limit overrides in Phase 9.
- **D-14:** Default chunking should be reference-derived. Researcher/planner must trace BSArchPro/xEdit and known BA2 behavior first; if evidence is inconclusive, use conservative contiguous mip groups and document the fallback.
- **D-15:** For array textures and cubemaps, use a uniform chunk pattern across every slice or face. The same mip-range split should repeat per slice/face so validation remains predictable.
- **D-16:** Chunk planning must preserve the locked no-transform rule: no resizing, transcoding, mip generation, repair, or image-data reordering beyond BA2-required chunk serialization.

#### Dedupe Granularity
- **D-17:** DX10 deduplication remains an archive-level boolean option, disabled by default, matching the TES4-family and BA2 GNRL writer pattern.
- **D-18:** When enabled, dedupe is archive-wide. Any eligible chunk in the archive may share a payload offset while each texture record, chunk record, and filename-table entry remains distinct.
- **D-19:** Dedupe runs after chunk planning and archive-level compression. Chunk splitting must not change to improve dedupe opportunities.
- **D-20:** Dedupe eligibility requires matching final stored bytes plus matching chunk metadata for raw size, stored size, and compression route. It does not require matching texture dimensions, DXGI format, cubemap/array state, or mip range if the eligible chunk bytes and chunk sizes/compression match.
- **D-21:** The first eligible chunk in deterministic record/chunk order owns the physical payload bytes. Later eligible duplicate chunks reuse that payload offset.
- **D-22:** Complete duplicate DDS entries may share every eligible chunk offset when dedupe is enabled, while still preserving separate texture records and names.
- **D-23:** Tests must prove disabled output has distinct eligible offsets, enabled output shares eligible offsets, and both modes reopen and extract valid DDS bytes.

#### DDS Proof Matrix
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

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within phase scope. The SPEC Requirement 7 correction is an in-scope planning prerequisite, not a deferred idea.
</user_constraints>

## Summary

Phase 9 should add a dedicated dependency-light `ba2_dx10_writer` surface that mirrors the existing `ba2_gnrl_writer`, but with DDS host-file-only inputs validated and snapshotted at `add_file` time. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; `include/libbsa/writer.hpp`] The planner must explicitly override SPEC Requirement 7's raw/compressed override wording: public BA2 DX10 output is compressed-only, with no entry- or chunk-level raw override API. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]

The implementation should reuse Phase 8's safe publish, Starfield header option, compression routing, dedupe, and reader-backed validation patterns, while adding a DX10-specific planner that reads DirectXTex metadata and DDS image bytes, derives texture records, groups mip ranges, compresses every chunk, and serializes `BTDX`/`DX10` texture records plus chunk records. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `src/formats/ba2/ba2_dx10_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`] DirectXTex must remain private; public metadata should continue to use libbsa-owned numeric texture fields. [VERIFIED: `src/texture/directxtex_analyzer.cpp`; `include/libbsa/archive.hpp`; `tests/unit/public_include_boundary_tests.cpp`]

**Primary recommendation:** Plan the phase as a public contract + add-time DDS snapshot/analyzer wave, a DDS layout/chunk planner wave, a DX10 serializer/compression wave, then dedupe/safe-publish/reader-backed validation waves. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; `tests/unit/ba2_gnrl_writer_tests.cpp`]

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WBA2-06 | Consumer can create new Fallout 4 BA2 DX10/DDS texture archives from DDS files. | Use `ba2_dx10_target::fallout4`, version `1`, subtype `DX10`, deflate-compressed chunks, and reader-backed reopen/extract tests. [VERIFIED: `.planning/REQUIREMENTS.md`; `TES5Edit/Core/wbBSArchive.pas`; `src/formats/ba2/ba2_dx10_parser.cpp`] |
| WBA2-07 | Consumer can create new Starfield BA2 v3 DX10/DDS texture archives from DDS files. | Use `ba2_dx10_target::starfield_v3`, version `3`, default `Unknown1=1`, `Unknown2=0`, `CompressionMethod=3`, and raw-LZ4-block compressed chunks. [VERIFIED: `.planning/REQUIREMENTS.md`; `TES5Edit/Core/wbBSArchive.pas`; `src/detail/compression_router.cpp`] |
| WBA2-08 | Writer can analyze DDS input through DirectXTex and generate BA2 texture records from library-owned metadata. | Extend `directxtex_analyzer` to return DDS metadata plus image payload layout while keeping DirectXTex includes private. [VERIFIED: `src/texture/directxtex_analyzer.cpp`; Context7 `/microsoft/directxtex`] |
| WBA2-09 | Writer can split DDS textures into compatible mip/chunk records with configurable chunk limits. | Extend `dds_layout` with locked-format size calculations and a chunk planner that emits contiguous mip ranges repeated uniformly across arrays/faces. [VERIFIED: `src/texture/dds_layout.cpp`; `TES5Edit/Core/wbBSArchive.pas`; Microsoft Learn DDS guide] |
| WBA2-10 | Writer can apply per-chunk compression and serialize chunk metadata so extracted DDS output remains valid. | Use `detail::compress_payload` with deflate or raw LZ4 block selected from target/header metadata, write fixed 24-byte chunk records and `BAADF00D` sentinels, then validate through existing DX10 extraction. [VERIFIED: `src/detail/compression_router.cpp`; `src/formats/ba2/ba2_dx10_reader.cpp`; `TES5Edit/Core/wbBSArchive.pas`] |
| WBA2-11 | Maintainer can round-trip BA2 writer output by packing, reopening, extracting, and byte-comparing or metadata-validating source files. | Use public `archive_reader` plus DirectXTex metadata checks and DDS payload-byte comparisons, not writer internals alone. [VERIFIED: `tests/unit/ba2_dx10_extraction_tests.cpp`; `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] |
</phase_requirements>

## Project Constraints (from AGENTS.md)

- Implementation language is C++; public APIs must stay idiomatic C++20 and reusable. [VERIFIED: `AGENTS.md`]
- `TES5Edit/` is read-only: do not edit, format, stage, compile, or use it as fixture workspace. [VERIFIED: `AGENTS.md`]
- Preserve BSArchPro-compatible behavior discovered from TES5Edit unless a documented reason to diverge exists. [VERIFIED: `AGENTS.md`]
- Use `libdeflate`, official `lz4`, `DirectXTex`, and `vcpkg`; do not introduce speculative external dependencies. [VERIFIED: `AGENTS.md`; `vcpkg.json`]
- Public headers must avoid DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, private implementation types, and C++23 `std::expected`. [VERIFIED: `AGENTS.md`; `tests/unit/public_include_boundary_tests.cpp`]
- Add Doxygen-compliant doc comments for new public APIs and substantially rewritten methods; comments explaining non-obvious compatibility constraints should be added near ported behavior. [VERIFIED: `AGENTS.md`]
- Add focused fixture-based tests for parsing, writing, round-tripping, and compatibility behavior; never use `TES5Edit/` as a mutable fixture. [VERIFIED: `AGENTS.md`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Public DX10 writer API | Library public API | Private BA2 writer | Constructors/options/add/write contracts belong in installed headers, while serialization remains private. [VERIFIED: `include/libbsa/writer.hpp`; `src/formats/ba2/ba2_gnrl_writer.cpp`] |
| DDS validation and metadata extraction | Private texture adapter | Test validation | DirectXTex is already isolated in `src/texture/directxtex_analyzer.*` and must not leak into public headers. [VERIFIED: `src/texture/directxtex_analyzer.cpp`; `tests/unit/public_include_boundary_tests.cpp`] |
| Mip/chunk planning | Private texture/layout module | Private BA2 writer | Chunk planning depends on DDS format sizing and existing `validate_and_order_chunks` logic. [VERIFIED: `src/texture/dds_layout.cpp`] |
| Compression routing | Private detail services | Private BA2 writer | Codec choice is selected by BA2 target/header metadata and implemented through `detail::compress_payload`. [VERIFIED: `src/detail/compression_router.cpp`] |
| Safe finalization | Private writer filesystem layer | Tests | Existing BA2 GNRL publish logic owns temp directory reservation, overwrite guard, backup, and rollback. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `tests/unit/ba2_gnrl_writer_tests.cpp`] |
| Round-trip proof | Tests / validation | Public reader | Acceptance requires public reader APIs plus DirectXTex validation of extracted DDS output. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`; `tests/unit/ba2_dx10_extraction_tests.cpp`] |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++ | C++20 | Public API and implementation language. [VERIFIED: `CMakeLists.txt`; `AGENTS.md`] | Project constraint; avoids C++23-only `std::expected` in public headers. [VERIFIED: `AGENTS.md`; `include/libbsa/result.hpp`] |
| CMake | Minimum 4.0; local CLI 4.3.2 | Build registration, CTest, installed headers. [VERIFIED: `CMakeLists.txt`; command `cmake --version`] | Existing project build system and presets require CMake. [VERIFIED: `CMakePresets.json`] |
| vcpkg manifest mode | Baseline `12dcccadfe573d0eaa6c67a968413ded7805d256` | Dependency acquisition. [VERIFIED: `vcpkg.json`] | Existing project policy and manifest already provide required dependencies. [VERIFIED: `AGENTS.md`; `vcpkg.json`] |
| DirectXTex | vcpkg `2026-03-31#0`, last updated 2026-04-01 | DDS metadata and validation behind private boundary. [CITED: https://vcpkg.io/en/package/directxtex.html] | Official DDS library supports `GetMetadataFromDDSMemory`, `LoadFromDDSMemory`, `TexMetadata`, and `ScratchImage`. [CITED: Context7 `/microsoft/directxtex`] |
| libdeflate | vcpkg `1.25#0`, last updated 2025-11-03 | FO4 DX10 deflate chunk compression. [CITED: https://vcpkg.io/en/package/libdeflate.html] | Project requires libdeflate and existing router already supports deflate compression/decompression. [VERIFIED: `AGENTS.md`; `src/detail/compression_router.cpp`] |
| lz4 | vcpkg `1.10.0#0`, last updated 2024-07-25 | Starfield v3 method `3` raw LZ4 block compression. [CITED: https://vcpkg.io/en/package/lz4.html] | Project requires official lz4 and existing router exposes raw block compression separately from frame compression. [VERIFIED: `AGENTS.md`; `src/detail/compression_router.cpp`] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Catch2 | vcpkg `3.14.0#0`, last updated 2026-04-06 | Unit, writer, fixture, and public-boundary tests. [CITED: https://vcpkg.io/en/package/catch2.html] | Add `ba2_dx10_writer_tests.cpp`, extend public boundary tests, and validate reader-backed round trips. [VERIFIED: `tests/CMakeLists.txt`; `tests/unit/public_include_boundary_tests.cpp`] |
| CTest | Bundled with CMake | Test orchestration and labels. [VERIFIED: `tests/CMakeLists.txt`] | Existing `catch_discover_tests(... ADD_TAGS_AS_LABELS ...)` turns Catch2 tags into CTest labels. [VERIFIED: `tests/CMakeLists.txt`] |
| nlohmann-json | Existing test-only dependency | Fixture manifests. [VERIFIED: `vcpkg.json`; `tests/CMakeLists.txt`] | Use only in tests/generators; public/runtime dependency boundary remains private. [VERIFIED: `tests/unit/ba2_dx10_extraction_tests.cpp`; `tests/CMakeLists.txt`] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| DirectXTex metadata/load | Hand-written DDS parser | Rejected because DDS arrays, cubemaps, legacy headers, and DXGI formats are already handled by DirectXTex and project policy requires it. [VERIFIED: `AGENTS.md`; Context7 `/microsoft/directxtex`] |
| libdeflate + lz4 router | zlib wrapper, LZ4 frame API, filename-based inference | Rejected because Phase 2/8 established explicit metadata routing; Starfield method `3` must use raw LZ4 block, not LZ4 frame. [VERIFIED: `src/detail/compression_router.cpp`; `.planning/STATE.md`] |
| Entry/chunk compression overrides | Reuse `entry_compression_policy` in DX10 API | Rejected by CONTEXT correction: public DX10 output must be archive-level compressed-only. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] |
| Runtime-only generated DDS fixtures | Commit generated source DDS and manifests | Rejected by user decision D-26; committed legal fixtures are the primary proof path. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] |

**Installation:** no new runtime dependency should be added. [VERIFIED: `vcpkg.json`; `AGENTS.md`]

```bash
# Existing manifest already lists required dependencies.
vcpkg install --triplet x64-windows
```

**Version verification:** DirectXTex `2026-03-31#0`, libdeflate `1.25#0`, lz4 `1.10.0#0`, and Catch2 `3.14.0#0` were checked against vcpkg package pages during this research. [CITED: https://vcpkg.io/en/package/directxtex.html; https://vcpkg.io/en/package/libdeflate.html; https://vcpkg.io/en/package/lz4.html; https://vcpkg.io/en/package/catch2.html]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer code
  |
  v
ba2_dx10_writer(target, options)  -- archive-wide options only --> target profile
  |
  v
add_file(archive_path, dds_host_path)
  |--> normalize/archive-path validation
  |--> read DDS host file into writer-owned bytes
  |--> DirectXTex metadata/load validation behind private boundary
  |--> supported-format + no-transform checks
  v
Writer-owned DDS entries
  |
  v
write_to(output.ba2)
  |--> duplicate canonical path validation
  |--> DDS mip/array/cubemap chunk planning
  |--> per-chunk compression: FO4 deflate OR SFv3 method 0 deflate / method 3 raw LZ4 block
  |--> optional final-stored-byte chunk dedupe
  |--> serialize BTDX/DX10 header, records, chunk records, payloads, filename table
  |--> safe publish via unique temp dir + overwrite backup/rollback
  v
archive_reader::open(output.ba2)
  |--> metadata/list/find/contains
  |--> extract DDS bytes
  v
DirectXTex validation + source payload-byte comparison in tests
```

### Recommended Project Structure

```text
include/libbsa/
├── writer.hpp                         # add ba2_dx10_target/options/writer declarations [VERIFIED: current pattern]
src/formats/ba2/
├── ba2_dx10_writer.hpp                # private writer entry/prepared entry API [VERIFIED: mirrors ba2_gnrl_writer.hpp]
├── ba2_dx10_writer.cpp                # DX10 serialization, compression, dedupe, publish [VERIFIED: mirrors ba2_gnrl_writer.cpp]
src/texture/
├── directxtex_analyzer.*              # extend private DDS load/snapshot metadata helpers [VERIFIED: existing file]
├── dds_layout.*                       # extend format sizing and chunk planning [VERIFIED: existing file]
tests/unit/
├── ba2_dx10_writer_tests.cpp          # public API, add-time validation, round-trip, dedupe, publish tests [VERIFIED: existing tests pattern]
tests/fixtures/generated/
├── generate_ba2_dx10_writer_fixtures.cpp or extend generate_ba2_dx10_fixtures.cpp [VERIFIED: existing fixture generator]
tests/fixtures/generated/source/
└── *.dds + manifest(s)                # committed legal DDS source fixtures [VERIFIED: fixture policy]
```

### Pattern 1: Dedicated public writer object

**What:** Add a DX10 writer object with target/options construction, immutable options, `add_file(archive_path, dds_host_path)`, and `write_to(host_path)`. [VERIFIED: `include/libbsa/writer.hpp`; `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**When to use:** Use for all public BA2 DX10 write-new archive creation in Phase 9. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`]  
**Example:**

```cpp
// Source: existing public writer shape in include/libbsa/writer.hpp and Phase 9 CONTEXT D-01/D-05.
libbsa::ba2_dx10_writer_options options{};
options.overwrite_existing = true;
options.deduplicate_payloads = false;
options.max_decoded_chunk_bytes = 0; // 0 means reference-derived default policy.

libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4, options};
auto added = writer.add_file("textures/generated/albedo.dds", "fixtures/source/albedo.dds");
if (!added) return added.error();
return writer.write_to("textures.ba2");
```

### Pattern 2: Add-time DDS snapshot and validation

**What:** `add_file` should reject empty/missing/unreadable/malformed/unsupported DDS inputs immediately and store the validated DDS bytes in writer-owned state. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**When to use:** Every DX10 DDS entry addition. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`]  
**Example:**

```cpp
// Source: DirectXTex DDS-I/O docs and existing directxtex_analyzer.cpp boundary.
DirectX::TexMetadata metadata{};
DirectX::ScratchImage image{};
const HRESULT hr = DirectX::LoadFromDDSMemory(
    dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
if (hr < 0) {
  return libbsa::error{libbsa::error_code::format_error, "DDS could not be loaded"};
}
```

### Pattern 3: Format-size table for chunk planning

**What:** Extend `dds_layout` with a private format descriptor table for the locked DXGI set: bytes-per-pixel for uncompressed formats and block bytes for BC formats. [VERIFIED: `src/texture/dds_layout.cpp`; Microsoft Learn DDS guide; Microsoft Learn DXGI enum search]  
**When to use:** Computing raw chunk size, validating source payload coverage, and predicting chunk ranges. [VERIFIED: `src/texture/dds_layout.cpp`]  
**Locked DXGI values:** `BC1_UNORM=71`, `BC1_UNORM_SRGB=72`, `BC3_UNORM=77`, `BC4_UNORM=80`, `BC5_UNORM=83`, `BC5_SNORM=84`, `BC6H_UF16=95`, `BC7_UNORM=98`, `R8G8B8A8_UNORM_SRGB=29`, `B8G8R8A8_UNORM=87`, `R8_UNORM=61`, `R8G8B8A8_SNORM=31`. [CITED: Microsoft Learn DXGI_FORMAT enumeration]

### Pattern 4: Reference-derived default chunking plus one public byte cap

**What:** Reference default uses up to 4 chunks, splits one mip per chunk while dimensions are at least `512x512`, and puts all remaining mips in the last chunk. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 848-853, 990-1005, 2059-2085]  
**How to adapt to D-12:** Expose one archive-wide `max_decoded_chunk_bytes`; use reference defaults when unset/zero, and when set, split only at mip boundaries into contiguous mip ranges whose decoded byte total is at most the cap where representable. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; Microsoft Learn DDS guide]  
**Why:** This honors the user's public API simplification while preserving a reference-compatible default path. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; `TES5Edit/Core/wbBSArchive.pas`]

### Anti-Patterns to Avoid

- **Reusing BA2 GNRL per-entry compression overrides for DX10:** It contradicts the user correction that public DX10 output is compressed-only. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]
- **Calling LZ4 frame APIs for Starfield BA2 v3 method `3`:** Existing project decisions require raw LZ4 block routing. [VERIFIED: `.planning/STATE.md`; `src/detail/compression_router.cpp`]
- **Inferring archive target or compression from `.dds` or `.ba2` filenames:** Target and compression are metadata/options-driven. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; `src/formats/ba2/ba2_dx10_parser.cpp`]
- **Full DDS byte equality as the only success criterion:** Extraction may canonicalize DDS headers, so compare metadata and image payload bytes. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; `tests/unit/ba2_dx10_extraction_tests.cpp`]
- **Touching `TES5Edit/` fixtures or submodule state:** Project policy forbids modifying the reference submodule. [VERIFIED: `AGENTS.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| DDS parsing/validation | Custom DDS parser as the source of truth | DirectXTex `GetMetadataFromDDSMemory` / `LoadFromDDSMemory` behind `directxtex_analyzer` | DirectXTex officially supports DDS I/O and returns `TexMetadata`/`ScratchImage`. [CITED: Context7 `/microsoft/directxtex`] |
| Deflate chunk compression | zlib/miniz/custom compressor | Existing `detail::compress_payload(deflate)` backed by libdeflate | Project already routes deflate through private compression services. [VERIFIED: `src/detail/compression_router.cpp`; `vcpkg.json`] |
| Starfield v3 LZ4 chunks | LZ4 frame wrapper or custom LZ4 bytes | Existing `detail::compress_payload(lz4_block)` backed by official lz4 | Method `3` is raw LZ4 block in project decisions and reader behavior. [VERIFIED: `src/detail/compression_router.cpp`; `.planning/STATE.md`] |
| Virtual path normalization | Host filesystem paths for archive keys | `detail::normalize_archive_path` | Existing reader/writer behavior uses canonical archive virtual paths, not platform paths. [VERIFIED: `src/detail/archive_path.cpp`; `src/formats/ba2/ba2_gnrl_writer.cpp`] |
| Hash fields | Caller-provided hashes | `detail::hash_fo4(file_name)` and `detail::hash_fo4(directory)` | Existing BA2 writer computes name and directory hashes internally. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `src/detail/bethesda_hash.cpp`] |
| Safe publish | Deterministic `<output>.tmp` replacement | Phase 8 unique temp directory + backup/rollback pattern | Existing tests protect temp-name siblings and overwrite rollback behavior. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `tests/unit/ba2_gnrl_writer_tests.cpp`] |

**Key insight:** The hard parts are not record writing alone; correctness depends on DDS subresource interpretation, mip byte sizing, target-routed compression, and extracted DDS validation, so custom shortcuts create silent corruption risk. [VERIFIED: Microsoft Learn DDS guide; Context7 `/microsoft/directxtex`; `src/formats/ba2/ba2_dx10_reader.cpp`]

## Common Pitfalls

### Pitfall 1: Planning raw/compressed public overrides from SPEC Requirement 7
**What goes wrong:** The plan adds public entry/chunk compression override APIs that the user explicitly rejected. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**Why it happens:** `09-SPEC.md` still contains stale Requirement 7 text. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`; `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**How to avoid:** Treat CONTEXT D-05/D-07 as the correction: public DX10 output is compressed-only; only archive-level Starfield compression method options remain. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**Warning signs:** New `ba2_dx10_entry_options::compression`, chunk override callbacks, or tests for raw DX10 output. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]

### Pitfall 2: Assuming DDS payload starts at byte 148 for all valid inputs without validation
**What goes wrong:** Legacy DDS headers and DXT10 headers can differ; using raw offsets without DirectXTex validation can mis-slice image data. [CITED: Microsoft Learn DDS guide; Context7 `/microsoft/directxtex`]  
**Why it happens:** Current extraction always reconstructs a DXT10 header, but writer input can be a DDS host file with legacy or DXT10 headers. [VERIFIED: `src/texture/dds_layout.cpp`; Microsoft Learn DDS guide]  
**How to avoid:** Use DirectXTex load metadata/ScratchImage for validation and either canonicalize image payload extraction through subresource images or implement a header-aware DDS image span parser cross-checked by DirectXTex. [CITED: Context7 `/microsoft/directxtex`; Microsoft Learn DDS guide]  
**Warning signs:** Tests only use existing Phase 6 reconstructed DXT10 fixtures and never a legacy BC1/BC3/BC5 DDS source. [VERIFIED: `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`; Microsoft Learn DDS guide]

### Pitfall 3: Incorrect block-compressed size math
**What goes wrong:** Raw chunk sizes mismatch mip ranges, causing `validate_and_order_chunks` or extraction decompression to fail. [VERIFIED: `src/texture/dds_layout.cpp`; `src/formats/ba2/ba2_dx10_reader.cpp`]  
**Why it happens:** BC1/BC4 use 8-byte 4x4 blocks; other BC formats in scope use 16-byte blocks; small mips still occupy at least one block. [CITED: Microsoft Learn DDS guide]  
**How to avoid:** Implement format descriptors and use `max(1, (dimension+3)/4)` block counts for BC formats. [CITED: Microsoft Learn DDS guide]  
**Warning signs:** Using `width * height * bits_per_pixel / 8` for BC formats without block rounding. [CITED: Microsoft Learn DDS guide]

### Pitfall 4: Cubemap/array ordering drift
**What goes wrong:** Extracted DDS payload bytes no longer match source order even when metadata is valid. [VERIFIED: `src/texture/dds_layout.cpp`; `tests/unit/ba2_dx10_extraction_tests.cpp`]  
**Why it happens:** DDS cubemap faces are ordered +X, -X, +Y, -Y, +Z, -Z with mips per face, and arrays iterate array element then mip. [CITED: Microsoft Learn cube map DDS layout; Microsoft Learn DDS guide]  
**How to avoid:** Repeat the same mip split pattern for every array slice or cubemap face and validate with existing `validate_and_order_chunks`. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; `src/texture/dds_layout.cpp`]  
**Warning signs:** Chunk planner treats all mip 0s for all faces first, or changes chunk split per face based on compression output. [CITED: Microsoft Learn cube map DDS layout; VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]

### Pitfall 5: Dedupe key omits chunk metadata
**What goes wrong:** Different logical chunks share an offset even though raw size, stored size, or compression route differs. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**Why it happens:** Phase 8 GNRL dedupe keyed only stored bytes, but DX10 dedupe decision D-20 adds metadata requirements. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**How to avoid:** Key dedupe on final stored bytes plus raw size, stored size, and compression route. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]  
**Warning signs:** A `std::map<std::vector<std::byte>, payload_assignment>` copied unchanged from GNRL. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]

## Code Examples

### Private DirectXTex load boundary

```cpp
// Source: Context7 /microsoft/directxtex DDS I/O Functions; current src/texture/directxtex_analyzer.cpp.
result<dds_source_analysis> analyze_dds_source(std::span<const std::byte> dds_bytes) {
  DirectX::TexMetadata metadata{};
  DirectX::ScratchImage image{};
  const HRESULT hr = DirectX::LoadFromDDSMemory(
      dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
  if (hr < 0) {
    return error{error_code::format_error, "DDS source could not be loaded"};
  }
  // Translate metadata and image layout into libbsa-owned values before returning.
}
```

### DX10 texture chunk record serialization

```cpp
// Source: TES5Edit/Core/wbBSArchive.pas DX10 write loop and src/formats/ba2/ba2_dx10_parser.cpp parser.
writer.write_u64_le(chunk.payload_offset);
writer.write_u32_le(chunk.packed_size);  // compressed-only public output: non-zero for non-empty chunks.
writer.write_u32_le(chunk.raw_size);
writer.write_u16_le(chunk.start_mip);
writer.write_u16_le(chunk.end_mip);
writer.write_u32_le(0xBAAD'F00DU);
```

### Format-size descriptor shape

```cpp
// Source: Microsoft DDS pitch/block-size guidance and current dds_layout.cpp mip_size_for_format.
struct dxgi_format_descriptor {
  std::uint32_t format;
  std::uint32_t block_width;
  std::uint32_t block_height;
  std::uint32_t bytes_per_block;
};
// BC1/BC4: 4x4, 8 bytes. BC3/BC5/BC6H/BC7: 4x4, 16 bytes. R8: 1x1, 1 byte. RGBA/BGRA: 1x1, 4 bytes.
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Phase 6 fixture-backed DXGI sizing only supported `R8G8B8A8_UNORM` and BC1 variants. [VERIFIED: `src/texture/dds_layout.cpp`] | Phase 9 must support the locked common-game set, including BC3/BC4/BC5/BC6H/BC7/R8/BGRA/RGBA variants. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`] | Phase 9 scope. [VERIFIED: `.planning/ROADMAP.md`] | Planner must allocate a layout/helper task before writer serialization. [VERIFIED: `src/texture/dds_layout.cpp`] |
| DX10 extraction validated archives generated by fixture tools. [VERIFIED: `tests/unit/ba2_dx10_extraction_tests.cpp`] | DX10 writer output must be validated by reopening, extracting, and DirectXTex-loading generated output. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`] | Phase 9 scope. [VERIFIED: `.planning/ROADMAP.md`] | Writer tests should not inspect only internal prepared records. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] |
| SPEC Requirement 7 required raw/compressed overrides. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`] | CONTEXT correction requires public compressed-only DX10 output. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] | 2026-05-09 discussion. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] | Planner must not add raw override APIs or tests. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] |

**Deprecated/outdated:** SPEC lines requiring raw/compressed DX10 overrides are superseded by CONTEXT D-05/D-07. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md`; `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|

**If this table is empty:** All claims in this research were verified or cited — no user confirmation needed.

## Open Questions (RESOLVED)

1. **Should `max_decoded_chunk_bytes = 0` mean reference-derived default, and nonzero mean byte cap?**
   - What we know: User locked one archive-wide max decoded chunk-byte control and requested reference-derived defaults. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]
   - What's unclear: The exact public sentinel/default name is discretionary. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]
   - Recommendation: Use `std::optional<std::uint32_t> max_decoded_chunk_bytes` or `0 means default`, document the sentinel, and test both default and forced-split behavior. [VERIFIED: `include/libbsa/writer.hpp` existing options style; `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`]
   - **RESOLVED:** Use `std::uint32_t max_decoded_chunk_bytes = 0` in `ba2_dx10_writer_options`; `0` means the reference-derived default chunking policy and nonzero values enforce the archive-wide decoded-byte cap where representable. This follows the existing public options style in `writer.hpp`, satisfies D-12/D-14, avoids adding a second optional state, and must be documented/tested in the public API and writer tests.
2. **How should array textures encode `cube_maps_raw` when not cubemaps?**
   - What we know: Existing parser infers array size by counting chunks with `start_mip == 0` unless `cube_maps_raw == 2049`; TES5Edit uses `$800` for ordinary textures and `$801` for cubemaps. [VERIFIED: `src/formats/ba2/ba2_dx10_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`]
   - What's unclear: Whether Bethesda has a separate array raw field convention beyond current parser inference. [VERIFIED: `src/formats/ba2/ba2_dx10_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`]
   - Recommendation: Preserve current reader contract: write `$800` for non-cubemaps, `$801` (`2049`) for cubemaps, and rely on repeated `start_mip==0` chunks for array-size inference unless fixture evidence proves another rule. [VERIFIED: `src/formats/ba2/ba2_dx10_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`]
   - **RESOLVED:** Write `cube_maps_raw = 2048` (`$800`) for all non-cubemap textures, including arrays, and `cube_maps_raw = 2049` (`$801`) for cubemaps. Array size remains represented by the uniform repeated chunk pattern with `start_mip == 0` per slice/face, matching the existing reader contract and D-10/D-15 without exposing raw public overrides.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Configure/build/test | ✓ | 4.3.2 | — [VERIFIED: command `cmake --version`] |
| Git | Submodule cleanliness and status checks | ✓ | 2.54.0.windows.1 | — [VERIFIED: command `git --version`] |
| Ninja | Optional generator | ✗ | — | Use Visual Studio/MSBuild generator through CMake presets or install Ninja. [VERIFIED: command probe] |
| vcpkg CLI | Manifest dependency install | ✗ on PATH | — | Set `VCPKG_ROOT`/PATH before configure; project already references `$env{VCPKG_ROOT}`. [VERIFIED: command probe; `CMakePresets.json`] |
| MSVC `cl` | Windows build compiler | ✗ in current shell PATH | — | Run from Developer PowerShell/Command Prompt or configure with another C++20 compiler supported by dependencies. [VERIFIED: command probe; `CMakePresets.json`] |

**Missing dependencies with no fallback:** none for research; execution must ensure `VCPKG_ROOT` and an MSVC developer environment are available before build/test. [VERIFIED: command probe; `CMakePresets.json`]

**Missing dependencies with fallback:** Ninja is absent, but the presets do not require it explicitly. [VERIFIED: command probe; `CMakePresets.json`]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 via vcpkg `3.14.0#0` and CTest discovery. [CITED: https://vcpkg.io/en/package/catch2.html; VERIFIED: `tests/CMakeLists.txt`] |
| Config file | `tests/CMakeLists.txt`, `CMakePresets.json`. [VERIFIED: file reads] |
| Quick run command | `ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure` after configure/build. [VERIFIED: `tests/CMakeLists.txt` label pattern] |
| Full suite command | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: `CMakePresets.json`] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WBA2-06 | FO4 v1 DX10 writer output from DDS source reopens and extracts. | integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*fo4 --output-on-failure` | ❌ Wave 0 [VERIFIED: no `ba2_dx10_writer_tests.cpp` in glob results] |
| WBA2-07 | Starfield v3 DX10 writer output with method `3` reopens and extracts. | integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*starfield --output-on-failure` | ❌ Wave 0 [VERIFIED: no `ba2_dx10_writer_tests.cpp` in glob results] |
| WBA2-08 | Add-time DirectXTex DDS validation and public metadata boundary. | unit/integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*dds --output-on-failure` | ❌ Wave 0 [VERIFIED: existing analyzer tests cover extraction, not writer add-time validation] |
| WBA2-09 | Mip/array/cubemap chunk planning with configurable cap. | unit | `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` | ⚠️ Extend existing [VERIFIED: `tests/unit/dds_layout_tests.cpp`; `src/texture/dds_layout.cpp`] |
| WBA2-10 | Per-chunk compressed serialization and exact-size extraction. | integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*compression --output-on-failure` | ❌ Wave 0 [VERIFIED: existing extraction tests cover fixtures, not writer output] |
| WBA2-11 | Pack/reopen/list/find/contains/extract/DirectXTex-validate. | integration | `ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure` | ❌ Wave 0 [VERIFIED: no writer test file] |

### Sampling Rate
- **Per task commit:** run focused labels/regex for touched area, e.g. `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer|dds_layout|public_include" --output-on-failure`. [VERIFIED: `tests/CMakeLists.txt`; `tests/unit/public_include_boundary_tests.cpp`]
- **Per wave merge:** run `ctest --preset windows-msvc-debug-static -L unit --output-on-failure`. [VERIFIED: `tests/CMakeLists.txt`]
- **Phase gate:** run full static preset, package consumer smoke, fixture generator targets, and `git -C TES5Edit status --short`. [VERIFIED: `CMakePresets.json`; `tests/CMakeLists.txt`; `AGENTS.md`]

### Wave 0 Gaps
- [ ] `tests/unit/ba2_dx10_writer_tests.cpp` — covers WBA2-06 through WBA2-11. [VERIFIED: glob results]
- [ ] Public boundary test additions for `ba2_dx10_target`, options, and writer methods. [VERIFIED: `tests/unit/public_include_boundary_tests.cpp`]
- [ ] DDS source fixture files and manifests under `tests/fixtures/generated/source/` and/or `archives/`. [VERIFIED: fixture glob]
- [ ] Extend `tests/CMakeLists.txt` for the new writer test and fixture generator byproducts. [VERIFIED: `tests/CMakeLists.txt`]
- [ ] Extend `dds_layout_tests.cpp` for locked DXGI format size math and chunk planner behavior. [VERIFIED: `src/texture/dds_layout.cpp`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No auth surface in local archive library. [VERIFIED: `.planning/ROADMAP.md`] |
| V3 Session Management | no | No session state. [VERIFIED: `.planning/ROADMAP.md`] |
| V4 Access Control | no | Local caller controls paths; no multi-user authorization model. [VERIFIED: `.planning/ROADMAP.md`] |
| V5 Input Validation | yes | Validate host paths, archive paths, DDS headers, DDS format support, chunk sizes, integer ranges, duplicate canonical paths, and output overwrite state. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `src/texture/directxtex_analyzer.cpp`; `src/texture/dds_layout.cpp`] |
| V6 Cryptography | no | Dedupe hash/equality is not security cryptography; do not add crypto. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`] |

### Known Threat Patterns for BA2 DX10 writer stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed DDS causing out-of-bounds slicing | Tampering/DoS | DirectXTex validation plus checked size arithmetic before chunk planning. [CITED: Context7 `/microsoft/directxtex`; VERIFIED: `src/texture/dds_layout.cpp`] |
| Oversized texture dimensions or mip counts overflowing 32-bit BA2 fields | DoS/Tampering | Checked `u16`/`u32` conversions and fail with structured `format_error`. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `src/formats/ba2/ba2_dx10_parser.cpp`] |
| Output path clobbering or temp collision deletion | Tampering | Reuse unique temp directory and overwrite backup/rollback pattern. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`] |
| Public dependency leakage | Information disclosure/build supply-chain risk | Public include boundary tests scan forbidden tokens. [VERIFIED: `tests/unit/public_include_boundary_tests.cpp`] |
| Compression bomb-like exact-size mismatch | DoS/Tampering | Existing extraction uses exact-size decompression for compressed chunks. [VERIFIED: `src/formats/ba2/ba2_dx10_reader.cpp`; `src/detail/compression_router.cpp`] |

## Sources

### Primary (HIGH confidence)
- `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md` — locked implementation decisions and SPEC correction. [VERIFIED: file read]
- `.planning/phases/09-ba2-dx10-write-new-support/09-SPEC.md` — locked requirements, with Requirement 7 corrected by CONTEXT. [VERIFIED: file read]
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md` — requirement traceability and prior decisions. [VERIFIED: file reads]
- `AGENTS.md` — project constraints and TES5Edit boundary. [VERIFIED: file read]
- `include/libbsa/writer.hpp`, `include/libbsa/archive.hpp` — public API and metadata patterns. [VERIFIED: file reads]
- `src/formats/ba2/ba2_gnrl_writer.cpp` — writer state, compression, dedupe, safe publish pattern. [VERIFIED: file read]
- `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp` — DX10 record parsing and extraction oracle. [VERIFIED: file reads]
- `src/texture/dds_layout.cpp`, `src/texture/directxtex_analyzer.cpp` — DDS layout and private DirectXTex boundary. [VERIFIED: file reads]
- `TES5Edit/Core/wbBSArchive.pas`, `TES5Edit/Core/wbDDS.pas` — read-only reference for DX10 records, defaults, DDS format sizing, and chunk defaults. [VERIFIED: file reads]
- Context7 `/microsoft/directxtex` — DirectXTex DDS I/O functions and `TexMetadata`. [CITED: Context7]
- Microsoft Learn DDS guide and cubemap layout — DDS header, DXT10, pitch/block sizing, array/cubemap order. [CITED: https://learn.microsoft.com/windows/win32/direct3ddds/dx-graphics-dds-pguide; https://learn.microsoft.com/windows/win32/direct3ddds/dds-file-layout-for-cubic-environment-maps]
- Microsoft Learn DXGI_FORMAT enumeration — numeric DXGI format IDs. [CITED: https://learn.microsoft.com/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format]

### Secondary (MEDIUM confidence)
- miere.ru BA2 format notes — independent BA2 header/texture/chunk field cross-check. [CITED: https://miere.ru/posts/ba2-archive-format/]
- bethesda-structs BA2 docs — independent GNRL/DX10 split and DDS header reconstruction context. [CITED: https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html]
- Rust `ba2` docs.rs page — independent ecosystem note that texture files are split into chunks for streaming mips and Starfield introduces lz4. [CITED: https://docs.rs/ba2/latest/ba2/fo4/index.html]
- NifSkope header snippet from Exa search — independent C++ structure names and field widths for FO4 texture records. [CITED: https://github.com/niftools/nifskope/blob/develop/lib/fsengine/bsa.h]

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — dependencies, versions, and project wiring were verified from `vcpkg.json`, vcpkg package pages, and CMake files. [VERIFIED: `vcpkg.json`; CITED: vcpkg package pages]
- Architecture: HIGH — existing BA2 GNRL writer and DX10 reader/parser provide direct implementation patterns. [VERIFIED: `src/formats/ba2/ba2_gnrl_writer.cpp`; `src/formats/ba2/ba2_dx10_parser.cpp`; `src/formats/ba2/ba2_dx10_reader.cpp`]
- Pitfalls: HIGH — each pitfall maps to locked user decisions, official DDS docs, or current code constraints. [VERIFIED: `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md`; CITED: Microsoft Learn DDS guide]

**Research date:** 2026-05-09  
**Valid until:** 2026-06-08 for project/codebase decisions; 2026-05-16 for fast-moving dependency version pages.
