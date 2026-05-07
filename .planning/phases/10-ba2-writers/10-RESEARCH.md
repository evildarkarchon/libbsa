# Phase 10: ba2-writers - Research

**Researched:** 2026-05-07  
**Domain:** C++20 BA2 GNRL and DX10/DDS native writer planning, DDS analysis, compression, chunking, deduplication, and read-after-write validation  
**Confidence:** MEDIUM-HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### BA2 API Shape
- **D-01:** Add production BA2 writer entry points in a dedicated public `ba2_writer.hpp` or equivalent BA2-writer-specific header. This should mirror Phase 9's `bsa_writer.hpp` discoverability and avoid crowding reader-focused `ba2.hpp`.
- **D-02:** Represent memory-backed and disk-backed BA2 inputs with separate public value types. Do not use one variant-like entry with invalid byte/path combinations, and do not add a stateful builder just to mix input sources.
- **D-03:** Use subtype-specific public input types for GNRL payload entries versus DDS input entries. GNRL entries carry ordinary payload bytes/files, while DDS entries carry actual DDS bytes/files that planning analyzes.
- **D-04:** Use plan/finalize operation pairs for BA2 writers, parallel to `plan_bsa_write`, `plan_bsa_write_from_disk`, and `finalize_bsa_write`. Exact function/type names are planner discretion, but the API must preserve plan-owned bytes and caller-owned sink finalization.

### Target Options
- **D-05:** BA2 target selection should use exact variant-safe values for every required target: Fallout 4 GNRL v1/v7/v8, Starfield GNRL v2/v3, Fallout 4 DX10 v1/v7/v8, and Starfield DX10 v3. Do not rely on loose game-name strings or target inference from file extensions.
- **D-06:** Starfield v3 `CompressionMethod` should be selected as an archive-level writer option for Starfield v3 targets. Support method 0 and method 3 only; mixed per-entry compression methods are not allowed because the native field is archive-level.
- **D-07:** BA2 writer options should provide safe target defaults plus explicit per-entry or per-chunk compression policy overrides. Do not infer compression behavior from archive paths or source file extensions.
- **D-08:** BA2 write plans should echo native target fields needed for inspection, including subtype, version, header size, and compression method. Tests and dry-run consumers should not need to parse emitted bytes just to confirm the basic native target.

### Native Preview Detail
- **D-09:** Use subtype-specific preview sections inside the BA2 write plan for GNRL versus DDS/DX10 layout. A single flat entry list is not enough because DDS chunk and mip information is not meaningful for GNRL entries.
- **D-10:** GNRL plan preview should expose full native record and table detail needed for compatibility tests and dry-run inspection: name hashes, directory hashes, `FileTableOffset`, table regions, payload offsets, packed sizes, unpacked sizes, and compression state.
- **D-11:** DDS/DX10 plan preview should expose texture and chunk detail: texture record metadata, DXGI format, dimensions, mip count, array/cubemap state, chunk regions, mip ranges, chunk offsets, packed sizes, unpacked sizes, and chunk compression.
- **D-12:** BA2 deduplication preview should use stable shared data-region IDs plus archive-absolute offsets for both GNRL payloads and DDS chunks. Do not expose public content hashes; Phase 8's digest-neutral public surface still applies.

### DDS Input Handling
- **D-13:** BA2 DDS writer inputs should contain actual DDS bytes or explicit disk DDS files plus archive-virtual paths. Public callers should not supply BA2-native texture descriptors as the normal Phase 10 API.
- **D-14:** DDS analysis and validation happen during planning. A successful plan owns the derived metadata, chunk payload bytes, and stored payload bytes; finalization must not retain or reopen DDS input files and must only stream planned bytes.
- **D-15:** DDS chunking should be automatic in Phase 10. libbsa derives mip/chunk layout from DDS analysis and target rules; do not expose manual native chunk descriptor controls or optional chunk hints unless research proves they are required.
- **D-16:** Malformed, unsupported, or unrepresentable DDS inputs must fail structurally during planning with no partial plan. This includes malformed DDS bytes, unsupported layouts, impossible mip/chunk mapping, and formats that cannot be safely emitted without transcoding.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may choose exact type names, function names, helper file names, and test organization details as long as the decisions above, `10-SPEC.md`, existing public API patterns, and project constraints are satisfied.

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WRT-02 | Consumer can create Fallout 4 and Starfield BA2 GNRL archives with version-specific headers, file tables, offsets, and compression metadata. [VERIFIED: `.planning/REQUIREMENTS.md`] | Implement a BA2-specific plan/finalize public surface, serialize native `BTDX` + `GNRL` headers for FO4 v1/v7/v8 and Starfield v2/v3, emit 36-byte records plus length-prefixed name tables at `FileTableOffset`, route raw/deflate/Starfield method-3 LZ4-block payloads through existing compression dispatch, and prove output through `open_ba2` / `extract_ba2_entry`. [VERIFIED: `10-SPEC.md`; VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `src/compression.cpp`] |
| WRT-03 | Consumer can create Fallout 4 and Starfield BA2 DDS archives from DDS inputs with proper metadata analysis and mipmap chunking. [VERIFIED: `.planning/REQUIREMENTS.md`] | Add private DDS input analysis using DirectXTex `LoadFromDDSMemory` / `TexMetadata` / `ScratchImage`, translate metadata into libbsa-owned values, chunk mip payload ranges automatically, serialize native `BTDX` + `DX10` texture and chunk records, and validate read-after-write extraction with existing private DDS validation. [VERIFIED: `10-SPEC.md`; CITED: `/microsoft/directxtex`; VERIFIED: `src/texture/dds_validation.cpp`; VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`] |
</phase_requirements>

## Summary

Phase 10 should implement a production BA2 writer, not another generic writer harness. [VERIFIED: `10-SPEC.md`; VERIFIED: `10-CONTEXT.md`] The writer should mirror Phase 9's successful operation-style API: a BA2-specific public header, exact target values, separate memory/disk input types, deterministic plan objects that expose native layout, and finalization that only streams plan-owned bytes to a caller-owned `byte_sink`. [VERIFIED: `include/libbsa/bsa_writer.hpp`; VERIFIED: `src/bsa_writer.cpp`; VERIFIED: `10-CONTEXT.md`]

The highest-risk areas are BA2 DX10 DDS analysis and chunking, Starfield v3 archive-level `CompressionMethod`, native `PackedSize == 0` raw semantics, `FileTableOffset` / name-table placement, and keeping DirectXTex/LZ4/libdeflate out of public headers. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `src/compression.cpp`; VERIFIED: `10-SPEC.md`; VERIFIED: `AGENTS.md`] Use existing reader metadata and extraction as the immediate compatibility oracle: every generated writer fixture should reopen through `open_ba2`, inspect `ba2_archive` metadata, extract through `extract_ba2_entry`, and validate DDS outputs through the private DirectXTex-backed helper. [VERIFIED: `10-SPEC.md`; VERIFIED: `include/libbsa/ba2.hpp`; VERIFIED: `src/texture/dds_validation.cpp`]

**Primary recommendation:** Add `include/libbsa/ba2_writer.hpp`, `src/ba2_writer.cpp`, private `src/texture/dds_analysis.*`, and `tests/ba2_writer_tests.cpp`; implement GNRL first, then DDS/DX10 planning from actual DDS bytes, then compression/dedup/failure/public-smoke gates. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `CMakeLists.txt`; ASSUMED]

## Project Constraints (from AGENTS.md)

- The implementation language is C++; public interfaces should be clear, reusable, portable C++ rather than Delphi/Pascal transliteration. [VERIFIED: `AGENTS.md`]
- `TES5Edit/` is read-only: do not edit, format, generate changes under, update the submodule pointer, stage, commit, compile, or vendor it. [VERIFIED: `AGENTS.md`]
- Preserve archive-format behavior discovered from BSArchPro/TES5Edit unless a documented reason exists to diverge. [VERIFIED: `AGENTS.md`]
- When porting behavior, trace the reference first and record non-obvious compatibility constraints near the new implementation. [VERIFIED: `AGENTS.md`]
- Use `libdeflate` for deflate, official `lz4` for LZ4, `DirectXTex` for texture analysis, and `vcpkg` for dependency management; do not introduce speculative dependencies. [VERIFIED: `AGENTS.md`; VERIFIED: `vcpkg.json`]
- Public headers must not expose libdeflate, LZ4, DirectXTex, platform/Windows SDK, Delphi, UI, or TES5Edit implementation details. [VERIFIED: `AGENTS.md`; VERIFIED: `10-SPEC.md`]
- Archive paths are archive-virtual paths; do not treat them as host `std::filesystem::path` semantics except at explicit disk input boundaries. [VERIFIED: `10-SPEC.md`; VERIFIED: `include/libbsa/archive_path.hpp`]
- Never delete accurate comments as cleanup; add comments for non-obvious format compatibility, ownership/lifetime, error-handling, threading/cancellation, and deliberate deviations. [VERIFIED: `AGENTS.md`]
- Add Doxygen-compliant C++ doc comments for public APIs and substantially rewritten methods. [VERIFIED: `AGENTS.md`]
- Add focused fixture-based tests for archive parsing, writing, round-tripping, and compatibility behavior; do not use `TES5Edit/` as a mutable fixture. [VERIFIED: `AGENTS.md`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| BA2 writer public API | API / Library Core | Tests | BA2 creation is a reusable library operation parallel to `open_ba2` / `extract_ba2_entry` and Phase 9's `bsa_writer.hpp`; tests must compile it through public headers only. [VERIFIED: `include/libbsa/ba2.hpp`; VERIFIED: `include/libbsa/bsa_writer.hpp`; VERIFIED: `10-CONTEXT.md`] |
| Disk input ingestion | API / Library Core | Host filesystem boundary | Planning reads explicit host-file plus archive-path mappings and retains no file handles for finalization. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/bsa_writer.cpp`] |
| Archive path normalization and duplicate rejection | API / Library Core | — | Writer planning must normalize archive-virtual paths and reject duplicate normalized paths before map-like archive views can overwrite entries. [VERIFIED: `include/libbsa/archive_path.hpp`; VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `10-SPEC.md`] |
| BA2 GNRL table/layout planning | API / Library Core | Existing BA2 reader / tests | Native header sizes, 36-byte records, `FileTableOffset`, name table, offsets, packed sizes, and raw/compressed semantics are compatibility-critical plan decisions. [VERIFIED: `src/ba2_reader.cpp`; CITED: https://miere.ru/posts/ba2-archive-format/] |
| DDS input metadata and payload analysis | API / Library Core | Private DirectXTex adapter | DirectXTex can load DDS bytes and report width, height, mip levels, array size, DXGI format, cubemap state, and image pixels, but those types must be translated behind private boundaries. [CITED: `/microsoft/directxtex`; VERIFIED: `src/texture/dds_validation.cpp`] |
| BA2 DX10 chunk planning | API / Library Core | DDS analysis / tests | Chunk records encode archive-absolute offsets, packed/unpacked sizes, and start/end mip ranges; plan preview must expose those details before finalization. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`; CITED: https://miere.ru/posts/ba2-archive-format/] |
| Compression routing | API / Library Core | Private codec adapters | Planning should call existing `resolve_write_compression`, `resolve_payload_codec`, and `compress_payload`; codec wrappers remain private and route Starfield method-3 to raw LZ4 blocks only. [VERIFIED: `src/compression.cpp`; CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h] |
| Dedup sharing | API / Library Core | Phase 8/9 writer precedent | BA2 records/chunks can point at archive-absolute offsets, so opt-in sharing should use byte-identical post-policy stored bytes and public stable region IDs, not content hashes. [VERIFIED: `10-SPEC.md`; VERIFIED: `src/bsa_writer.cpp`; ASSUMED] |
| Finalization | API / Library Core | Caller-owned sink | Finalization streams table bytes and stored data regions from the plan, returning the first sink error unchanged and retaining no sink/input lifetime. [VERIFIED: `include/libbsa/io.hpp`; VERIFIED: `src/bsa_writer.cpp`; VERIFIED: `10-SPEC.md`] |
| Read-after-write proof | Test tier | API / Library Core | Phase 10 acceptance is generated read-back through libbsa readers, not Phase 11 external corpus comparison. [VERIFIED: `10-SPEC.md`; VERIFIED: `.planning/ROADMAP.md`] |

## Standard Stack

### Core

| Library / Component | Version / Status | Purpose | Why Standard |
|---------------------|------------------|---------|--------------|
| C++ | C++20 via `target_compile_features(libbsa PUBLIC cxx_std_20)`. [VERIFIED: `CMakeLists.txt`] | Public API and implementation. [VERIFIED: `AGENTS.md`] | Matches project constraints and existing `libbsa::result` C++20 error model. [VERIFIED: `include/libbsa/result.hpp`; VERIFIED: `.planning/STATE.md`] |
| BA2 reader API | `include/libbsa/ba2.hpp`, `open_ba2`, `extract_ba2_entry`. [VERIFIED: codebase] | Read-after-write oracle and metadata contract. [VERIFIED: `10-SPEC.md`] | Writer output must satisfy current reader summaries, entry metadata, texture metadata, and extraction semantics. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `include/libbsa/ba2.hpp`] |
| BSA writer precedent | `include/libbsa/bsa_writer.hpp`, `src/bsa_writer.cpp`. [VERIFIED: codebase] | Dedicated writer header, plan/finalize shape, disk input planning, native table bytes, dedup, finalization. [VERIFIED: codebase] | Phase 10 context explicitly asks BA2 writers to mirror Phase 9 discoverability and operation style. [VERIFIED: `10-CONTEXT.md`] |
| Phase 8 writer core | `include/libbsa/writer.hpp`, `src/writer.cpp`. [VERIFIED: codebase] | Compression policy resolution, stored payload ownership, checked layout, dedup precedent, sink emission semantics. [VERIFIED: codebase] | BA2 writers can reuse/adapt these semantics but must emit native BA2 bytes, not `LBSW`. [VERIFIED: `10-SPEC.md`; VERIFIED: `src/writer.cpp`] |
| DirectXTex | vcpkg `directxtex 2026-03-31#0`, last updated 2026-04-01. [CITED: https://vcpkg.io/en/package/directxtex.html] | Private DDS metadata/pixel analysis and validation. [VERIFIED: `AGENTS.md`] | DirectXTex docs expose `LoadFromDDSMemory`, `GetMetadataFromDDSMemory`, `TexMetadata`, `ScratchImage`, and per-image access needed to derive mip payload ranges. [CITED: `/microsoft/directxtex`] |

### Supporting

| Library / Component | Version / Status | Purpose | When to Use |
|---------------------|------------------|---------|-------------|
| libdeflate | vcpkg `libdeflate 1.25#0`, package last updated 2025-11-03. [CITED: https://vcpkg.io/en/package/libdeflate.html] | Deflate compression for FO4 BA2 and Starfield method-0 compressed GNRL payloads and DDS chunks. [VERIFIED: `src/compression.cpp`] | Use only through `compress_payload(compression_algorithm::deflate, ...)`; libdeflate docs warn compressed bytes may differ between versions, so tests should validate round-trip and size fields rather than golden compressed byte streams. [CITED: https://raw.githubusercontent.com/ebiggers/libdeflate/master/libdeflate.h] |
| lz4 | vcpkg `lz4 1.10.0#0`, package last updated 2024-07-25. [CITED: https://vcpkg.io/en/package/lz4.html] | Starfield v3 raw LZ4-block compression when archive `CompressionMethod == 3`. [VERIFIED: `src/compression.cpp`] | Use only through `compress_payload(compression_algorithm::lz4_block, ...)`; official docs state LZ4 block APIs are not frame APIs and require external compressed/decompressed sizes, which BA2 records provide. [CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h] |
| Catch2 | vcpkg `catch2 3.14.0#0`, package last updated 2026-04-06. [CITED: https://vcpkg.io/en/package/catch2.html] | Unit, fixture, codec, round-trip, and public-header tests. [VERIFIED: `CMakeLists.txt`] | Add a focused `libbsa_ba2_writer_tests` target with `unit;fixture;codec;roundtrip` labels and reuse existing smoke/public header gates. [ASSUMED; VERIFIED: `CMakeLists.txt`] |
| `memory_source` / `memory_sink` / `byte_sink` | Existing public I/O contracts. [VERIFIED: `include/libbsa/io.hpp`] | In-memory generated archives, read-after-write, extraction, and finalization failure tests. [VERIFIED: `tests/public_header_smoke.cpp`] | Use for tests and consumer examples; use a custom failing sink for first-error propagation. [VERIFIED: `src/bsa_writer.cpp`; VERIFIED: `10-SPEC.md`] |
| Existing BA2 DDS fixture helpers | `tests/ba2_dds_fixture_helpers.*`. [VERIFIED: codebase] | Source-reviewable DX10 record/chunk layout examples and DDS semantic assertions. [VERIFIED: codebase] | Reuse fixture style and constants for BA2 writer byte-level assertions; do not expose test descriptors in public writer APIs. [VERIFIED: `10-CONTEXT.md`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Dedicated `ba2_writer.hpp` | Extend `ba2.hpp` | Rejected by locked D-01 because BA2 writer types are large and should not crowd reader-focused APIs. [VERIFIED: `10-CONTEXT.md`] |
| Exact target enum/value | Raw subtype + version + game strings | Rejected by locked D-05 because invalid combinations like Starfield DX10 v2 become representable and would need extra failure paths. [VERIFIED: `10-CONTEXT.md`] |
| Separate GNRL/DDS input types | Single variant-like BA2 entry | Rejected by locked D-02/D-03 because GNRL payloads and DDS source bytes have different validation and derived metadata. [VERIFIED: `10-CONTEXT.md`] |
| Direct DirectXTex in public headers | Public DXGI/DirectXTex descriptors | Rejected by project and SPEC boundary; translate into libbsa-owned `dxgi_format`, texture preview records, and plan values. [VERIFIED: `AGENTS.md`; VERIFIED: `10-SPEC.md`; VERIFIED: `include/libbsa/ba2.hpp`] |
| Manual DDS chunk descriptors | Public chunk hints/native descriptors | Rejected by locked D-13/D-15 for the normal Phase 10 API; chunking is derived from actual DDS bytes. [VERIFIED: `10-CONTEXT.md`] |
| Golden compressed byte assertions | Compare exact deflate/LZ4 bytes | Rejected because libdeflate explicitly does not guarantee stable compressed bytes across versions; assert native sizes, compression states, and extracted bytes instead. [CITED: https://raw.githubusercontent.com/ebiggers/libdeflate/master/libdeflate.h] |

**Installation:** no new dependency installation is recommended; `vcpkg.json` already lists `libdeflate`, `lz4`, `directxtex`, and `catch2`. [VERIFIED: `vcpkg.json`]

```bash
cmake --preset windows-msvc-vcpkg
cmake --build --preset windows-msvc-vcpkg
ctest --preset windows-msvc-vcpkg --output-on-failure
```

**Version verification:** vcpkg package pages verified `libdeflate 1.25#0`, `lz4 1.10.0#0`, `directxtex 2026-03-31#0`, and `catch2 3.14.0#0`; local tools report CMake/CTest 4.3.2, Git 2.54.0.windows.1, and `C:\vcpkg\vcpkg.exe` version `2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1`. [CITED: vcpkg package pages; VERIFIED: shell audit]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer selects exact BA2 target + writer options
        |
        v
Subtype-specific inputs
  - GNRL memory entries OR disk file mappings
  - DDS memory bytes OR disk DDS mappings
        |
        v
Planning phase
  - validate target/version/subtype and Starfield method option
  - validate and normalize archive-virtual paths; reject duplicates
  - read disk bytes now; retain no file handles
  - for GNRL: treat payload bytes as ordinary file data
  - for DDS: LoadFromDDSMemory -> TexMetadata/ScratchImage -> libbsa metadata
  - derive DDS mip/chunk payload bytes and reject unsupported layouts
  - resolve compression policy per GNRL entry or DDS chunk
  - compress raw/deflate/Starfield method-3 LZ4-block through existing dispatcher
  - optional dedup: group byte-identical post-policy stored GNRL payloads / DDS chunks
  - compute checked BA2 header, record table, chunk table, name table, payload offsets
        |-- invalid/unsupported/overflow/missing file/malformed DDS --> result error, no sink access
        v
ba2_write_plan
  - target fields: subtype, version, header_size, compression_method
  - table regions + data regions with owned bytes
  - GNRL preview records OR DDS texture/chunk preview records
        |
        v
finalize_ba2_write(plan, byte_sink&)
        |-- first sink failure --> original structured error
        v
Native BA2 bytes
        |
        v
Test verification: memory_source -> open_ba2 -> metadata compare -> extract_ba2_entry
        |
        v
DDS outputs: private validate_dds confirms loadability and metadata
```

All data-shaping, compression, dedup, and layout decisions belong to planning; finalization is a deterministic sink emitter over already-owned table and payload bytes. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/bsa_writer.cpp`; VERIFIED: `src/writer.cpp`]

### Recommended Project Structure

```text
include/libbsa/
├── ba2.hpp                    # Existing BA2 reader/texture metadata API. [VERIFIED]
├── ba2_writer.hpp             # New BA2 target/options/input/plan/finalize declarations. [ASSUMED]
src/
├── ba2_writer.cpp             # Native GNRL and DX10 planning/finalization. [ASSUMED]
├── ba2_reader.cpp             # Existing read-back oracle; avoid tangling writer code into this file. [VERIFIED]
└── texture/
    ├── dds_analysis.hpp       # Private DDS input analyzer wrapping DirectXTex. [ASSUMED]
    ├── dds_analysis.cpp       # Translates TexMetadata/ScratchImage to libbsa-owned data. [ASSUMED]
    ├── dds_reconstruction.*   # Existing extraction reconstruction helper. [VERIFIED]
    └── dds_validation.*       # Existing DirectXTex validation helper. [VERIFIED]
tests/
├── ba2_writer_tests.cpp       # Native BA2 writer layout, roundtrip, failures. [ASSUMED]
├── ba2_dds_fixture_helpers.*  # Existing fixture style to reuse for byte assertions. [VERIFIED]
└── public_header_smoke.cpp    # Extend with BA2 writer consumer coverage. [VERIFIED]
```

### Pattern 1: BA2-specific public plan/finalize API

**What:** Add a BA2-specific target enum/value, options, GNRL memory/disk entry types, DDS memory/disk entry types, subtype-specific plan preview sections, and `plan_*` / `finalize_*` functions. [VERIFIED: `10-CONTEXT.md`]  
**When to use:** All production BA2 write paths; do not expose the Phase 8 `LBSW` harness for BA2 output. [VERIFIED: `10-SPEC.md`; VERIFIED: `src/writer.cpp`]  
**Example:**

```cpp
// Source: Phase 10 D-01 through D-12 and Phase 9 public writer precedent. [VERIFIED: 10-CONTEXT.md; VERIFIED: include/libbsa/bsa_writer.hpp]
enum class ba2_write_target {
    fallout4_gnrl_v1,
    fallout4_gnrl_v7,
    fallout4_gnrl_v8,
    starfield_gnrl_v2,
    starfield_gnrl_v3,
    fallout4_dx10_v1,
    fallout4_dx10_v7,
    fallout4_dx10_v8,
    starfield_dx10_v3,
};

struct ba2_gnrl_memory_entry {
    std::string path;
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

struct ba2_dds_memory_entry {
    std::string path;
    std::vector<std::byte> dds_bytes;
    compression_policy compression{compression_policy::archive_default};
};
```

### Pattern 2: Native GNRL serialization from current reader contract

**What:** Emit little-endian `BTDX`, version, `GNRL`, file count, `FileTableOffset`, optional Starfield v2/v3 tail words, then one 36-byte record per file, then name table, then payloads. [VERIFIED: `src/ba2_reader.cpp`; CITED: https://miere.ru/posts/ba2-archive-format/]  
**When to use:** FO4 GNRL v1/v7/v8 and Starfield GNRL v2/v3. [VERIFIED: `10-SPEC.md`]  
**Example:**

```cpp
// Source: current BA2 reader parse fields. [VERIFIED: src/ba2_reader.cpp]
append_magic(table, "BTDX");
append_u32(table, version);
append_magic(table, "GNRL");
append_u32(table, file_count);
append_u64(table, file_table_offset);
if (version == 2) { append_u32(table, 0); append_u32(table, 0); }
if (version == 3) { append_u32(table, 0); append_u32(table, 0); append_u32(table, compression_method); }

// Record shape: name hash, extension bytes, directory hash, unknown, offset, packed size, size, tail.
append_u32(table, entry.name_hash);
append_extension4(table, entry.path);
append_u32(table, entry.directory_hash);
append_u32(table, 0);
append_u64(table, entry.offset);
append_u32(table, entry.compression == compression_state::raw ? 0U : checked_u32(entry.stored_size));
append_u32(table, checked_u32(entry.unpacked_size));
append_u32(table, 0xBAADF00DU);
```

### Pattern 3: DDS analysis during planning

**What:** Load DDS bytes with DirectXTex, copy metadata into libbsa-owned types, and copy image payload ranges into plan-owned chunk bytes before compression and dedup. [CITED: `/microsoft/directxtex`; VERIFIED: `10-CONTEXT.md`]  
**When to use:** Every BA2 DX10/DDS memory or disk input. [VERIFIED: `10-SPEC.md`]  
**Example:**

```cpp
// Source: DirectXTex DDS I/O docs and existing private validation boundary. [CITED: /microsoft/directxtex; VERIFIED: src/texture/dds_validation.cpp]
DirectX::TexMetadata metadata{};
DirectX::ScratchImage image{};
const auto hr = DirectX::LoadFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
if (hr < 0) {
    return failure<planned_dds_input>({error_code::malformed_archive, "DDS analysis failed"});
}

planned.width = checked_u16(metadata.width);
planned.height = checked_u16(metadata.height);
planned.mip_count = checked_u8(metadata.mipLevels);
planned.array_size = checked_u16(metadata.arraySize);
planned.is_cubemap = metadata.IsCubemap();
planned.format = dxgi_format{static_cast<std::uint32_t>(metadata.format)};
```

### Pattern 4: Conservative automatic DDS chunking

**What:** Derive chunks from DDS mip ranges, not public native descriptors. For Phase 10 generated read-back, prefer one chunk per mip level containing that mip's images across all array/cube items; optionally group contiguous low-resolution mip levels into a final chunk only after tests prove output remains semantically equivalent. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/ba2_reader.cpp`; ASSUMED]  
**When to use:** BA2 DDS planning after DirectXTex loads `ScratchImage`. [CITED: `/microsoft/directxtex`]  
**Example:**

```cpp
// Source: DirectXTex TexMetadata::ComputeIndex and BA2 chunk start/end mip records. [CITED: /microsoft/directxtex; VERIFIED: src/ba2_reader.cpp]
for (std::uint32_t mip = 0; mip < planned.mip_count; ++mip) {
    std::vector<std::byte> chunk_payload;
    for (std::uint32_t item = 0; item < planned.array_size; ++item) {
        const auto* image_at_mip = image.GetImage(mip, item, 0);
        if (image_at_mip == nullptr) {
            return failure<planned_dds_input>({error_code::malformed_archive, "DDS mip image missing"});
        }
        append_bytes(chunk_payload, image_at_mip->pixels, image_at_mip->slicePitch);
    }
    chunks.push_back({.start_mip = mip, .end_mip = mip, .payload = std::move(chunk_payload)});
}
```

### Anti-Patterns to Avoid

- **Expose DirectXTex/DXGI headers publicly:** Public BA2 writer headers must expose libbsa-owned values only. [VERIFIED: `AGENTS.md`; VERIFIED: `10-SPEC.md`]
- **Infer GNRL/DDS subtype or compression from file extensions:** Target and compression policies are explicit; GNRL `.dds` entries are ordinary payloads. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `06-RESEARCH.md`]
- **Allow mixed Starfield v3 compression methods:** BA2 v3 `CompressionMethod` is archive-level in current reader summary and context D-06. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `10-CONTEXT.md`]
- **Set BA2 raw `PackedSize` to raw size:** Current reader treats GNRL `PackedSize == 0` as raw and uses `Size` as the stored length; DX10 reader treats raw chunks as `packed_size == size`. [VERIFIED: `src/ba2_reader.cpp`]
- **Compress during finalization:** This can make plan preview offsets/sizes drift from emitted bytes and violates D-14 for DDS inputs. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/bsa_writer.cpp`]
- **Assert exact compressed bytes:** libdeflate warns output is not stable across versions; tests should assert read-back equivalence and native metadata. [CITED: https://raw.githubusercontent.com/ebiggers/libdeflate/master/libdeflate.h]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Structured writer failures | Exceptions, booleans, or warning-only downgrades | `libbsa::result<T>` / `result<void>` | Existing project error model uses structured failures for I/O/format failures. [VERIFIED: `include/libbsa/result.hpp`; VERIFIED: `10-SPEC.md`] |
| Archive path normalization | `std::filesystem::path`, host canonicalization, or ad hoc lowercase | `normalize_archive_path` | Archive paths are virtual paths and must reject invalid/traversal inputs before host file I/O where possible. [VERIFIED: `include/libbsa/archive_path.hpp`; VERIFIED: `src/bsa_writer.cpp`] |
| Deflate or LZ4 codecs | Direct public libdeflate/LZ4 calls, custom codecs, or fallback probing | `resolve_write_compression`, `resolve_payload_codec`, `compress_payload` | Existing dispatch keeps dependency headers private and prevents LZ4 frame/block confusion. [VERIFIED: `src/compression.cpp`; CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h] |
| DDS parsing and metadata | Hand-written full DDS parser or public caller descriptors | Private DirectXTex-backed DDS analysis | DirectXTex already loads DDS memory, exposes metadata/image data, and is a required dependency. [VERIFIED: `AGENTS.md`; CITED: `/microsoft/directxtex`] |
| DDS reconstruction validation | Trust writer output without opening/extracting | Existing `extract_ba2_entry` + `detail::validate_dds` | Phase 10 acceptance requires read-after-write and DDS validation through private helpers. [VERIFIED: `10-SPEC.md`; VERIFIED: `src/texture/dds_validation.cpp`] |
| Test fixture strategy | Committed binary blobs only | Source-built generated fixtures and memory sources/sinks | Existing BA2 reader/DDS tests use source-reviewable builders; Phase 10 accepts generated read-back before Phase 11 corpus validation. [VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`; VERIFIED: `10-SPEC.md`] |
| Output emission | Returned whole archive image only or `std::ostream` | `byte_sink` finalization | WRT-05 requires streaming output and existing writer finalizers propagate `byte_sink` errors. [VERIFIED: `.planning/REQUIREMENTS.md`; VERIFIED: `src/bsa_writer.cpp`] |

**Key insight:** BA2 writing is a native table construction problem plus DDS payload analysis. The risky part is freezing all byte-affecting choices in the plan: DDS chunk bytes, compression state, dedup sharing, `PackedSize`/`Size`, and archive-absolute offsets. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `src/bsa_writer.cpp`]

## Common Pitfalls

### Pitfall 1: Treating GNRL `PackedSize == 0` as an empty stored payload
**What goes wrong:** Raw GNRL entries extract as zero bytes or have invalid payload ranges. [VERIFIED: `src/ba2_reader.cpp`]  
**Why it happens:** BA2 GNRL uses `PackedSize == 0` as the raw marker, with `Size` carrying the raw payload length. [VERIFIED: `src/ba2_reader.cpp`; CITED: https://miere.ru/posts/ba2-archive-format/]  
**How to avoid:** For raw GNRL records, write `PackedSize = 0`, `Size = unpacked_size`, and store raw bytes at `Offset`; plan preview `stored_size` should still expose the raw byte count for finalization and tests. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `10-SPEC.md`]  
**Warning signs:** A raw non-empty entry has native `PackedSize != 0` or libbsa read-back reports `stored_size == 0`. [VERIFIED: `10-SPEC.md`]

### Pitfall 2: Applying GNRL packed-size semantics to DX10 chunks
**What goes wrong:** Raw DDS chunks with `packed_size == 0` fail `range_fits`, or reader interprets chunk compression incorrectly. [VERIFIED: `src/ba2_reader.cpp`]  
**Why it happens:** Current DX10 reader derives raw chunk compression when `packed_size == size`; unlike GNRL, chunk records are validated using `packed_size` as the stored byte range. [VERIFIED: `src/ba2_reader.cpp`]  
**How to avoid:** For raw DX10 chunks, write `packed_size == size == stored_payload.size()`. For compressed chunks, write compressed byte count in `packed_size` and uncompressed chunk byte count in `size`. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`]  
**Warning signs:** Raw DDS extraction fails before decompression or `compression_for_dx10_chunk` reports `unknown`. [VERIFIED: `src/ba2_reader.cpp`]

### Pitfall 3: Letting Starfield v3 method 3 affect raw entries/chunks
**What goes wrong:** Raw data is routed through LZ4-block decompression because the archive-level method is 3. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `src/compression.cpp`]  
**Why it happens:** Starfield v3 stores native codec selection at the archive level, but raw entries/chunks are still raw by their size markers. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `10-CONTEXT.md`]  
**How to avoid:** Resolve per-entry/per-chunk compression state first; only compressed data in method-3 Starfield archives should become `compression_state::lz4_block`. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `src/compression.cpp`]  
**Warning signs:** Force-raw tests fail in a method-3 archive unless raw bytes happen to be valid LZ4 blocks. [VERIFIED: `10-SPEC.md`]

### Pitfall 4: Reconstructing DDS chunks from header bytes instead of image payload bytes
**What goes wrong:** BA2 DX10 stores headerless texture payload chunks, so storing a full DDS file including header in each chunk makes extraction reconstruct invalid or duplicated headers. [VERIFIED: `src/texture/dds_reconstruction.cpp`; CITED: https://miere.ru/posts/ba2-archive-format/]  
**Why it happens:** Public inputs are actual DDS files/bytes, but native BA2 DX10 archives store texture data and metadata separately. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/ba2_reader.cpp`]  
**How to avoid:** Use DirectXTex `ScratchImage` image data for chunk payloads and native DX10 records for dimensions/format/mips; validate extracted reconstructed DDS through `detail::validate_dds`. [CITED: `/microsoft/directxtex`; VERIFIED: `src/texture/dds_validation.cpp`]  
**Warning signs:** Extracted DDS output contains two DDS headers or DirectXTex validation fails after read-back. [VERIFIED: `10-SPEC.md`]

### Pitfall 5: Incorrect DDS array/cubemap interpretation
**What goes wrong:** Cubemaps or arrays reopen with wrong `array_size`, `is_cubemap`, or image payload ordering. [VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`; VERIFIED: `src/ba2_reader.cpp`]  
**Why it happens:** DirectXTex reports cubemaps through `TexMetadata::IsCubemap()` and array size, while DDS DX10 headers represent cubemap arrays as cube counts in `DDS_HEADER_DXT10::arraySize`. [CITED: `/microsoft/directxtex`; CITED: https://learn.microsoft.com/windows/win32/direct3ddds/dds-header-dxt10]  
**How to avoid:** Translate DirectXTex metadata into the existing libbsa `texture_metadata` semantics and reuse reconstruction validation tests for one-mip, multi-mip, cubemap, and array inputs. [VERIFIED: `include/libbsa/ba2.hpp`; VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`]  
**Warning signs:** Cubemap read-back reports `array_size == 1` or fails DDS validation despite valid source DDS input. [VERIFIED: `10-SPEC.md`]

### Pitfall 6: Name-table / payload offset mismatch
**What goes wrong:** `open_ba2` rejects writer output as a truncated BA2 table because the name table end does not equal the first payload offset. [VERIFIED: `src/ba2_reader.cpp`]  
**Why it happens:** Current reader validates that name-table bytes end exactly at the first payload offset for non-empty GNRL and DX10 archives. [VERIFIED: `src/ba2_reader.cpp`]  
**How to avoid:** Compute `FileTableOffset` after header + record/chunk tables, emit exactly one uint16-length-prefixed name per entry, then start payloads immediately after the name table. [VERIFIED: `src/ba2_reader.cpp`; CITED: https://miere.ru/posts/ba2-archive-format/]  
**Warning signs:** Byte-level tests pass header fields but read-back fails with `truncated BA2 table`. [VERIFIED: `src/ba2_reader.cpp`]

## Code Examples

Verified patterns from project and official sources:

### Finalization: planned chunks only

```cpp
// Source: Phase 9 finalizer pattern. [VERIFIED: src/bsa_writer.cpp]
result<void> finalize_ba2_write(const ba2_write_plan& plan, byte_sink& sink)
{
    auto written = sink.write(std::span<const std::byte>{plan.table_bytes});
    if (!written.has_value()) {
        return failure<void>(written.error());
    }
    for (const auto& region : plan.data_regions) {
        written = sink.write(std::span<const std::byte>{region.stored_payload});
        if (!written.has_value()) {
            return failure<void>(written.error());
        }
    }
    return success();
}
```

### Compression route for Starfield v3 writers

```cpp
// Source: existing compression dispatcher. [VERIFIED: src/compression.cpp]
payload_codec_request request{};
request.format = target_is_dds ? archive_format::starfield_ba2_dds : archive_format::starfield_ba2_gnrl;
request.entry_state = chunk_or_entry_compression; // raw, deflate, or lz4_block
request.compression_method = options.compression_method; // 0 or 3 only for Starfield v3
auto algorithm = resolve_payload_codec(request);
if (!algorithm.has_value()) {
    return failure<ba2_write_plan>(algorithm.error());
}
auto stored = compress_payload(algorithm.value(), std::span<const std::byte>{unpacked_payload});
```

### Public-header smoke shape

```cpp
// Source: existing public smoke and Phase 10 context. [VERIFIED: tests/public_header_smoke.cpp; VERIFIED: 10-CONTEXT.md]
#include <libbsa/ba2_writer.hpp>

libbsa::ba2_gnrl_memory_entry gnrl{};
gnrl.path = "meshes/public_smoke.nif";
gnrl.payload = {std::byte{0x01}, std::byte{0x02}};
gnrl.compression = libbsa::compression_policy::force_raw;

auto plan = libbsa::plan_ba2_gnrl_write(
    libbsa::ba2_write_target::fallout4_gnrl_v1,
    std::span<const libbsa::ba2_gnrl_memory_entry>{&gnrl, 1});
libbsa::memory_sink sink;
auto finalized = plan.has_value() ? libbsa::finalize_ba2_write(plan.value(), sink)
                                  : libbsa::failure<void>(plan.error());
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Generic Phase 8 `LBSW` writer harness | Native production BA2 serialization | Phase 10 scope lock on 2026-05-07. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `10-SPEC.md`] | Planner must emit `BTDX` BA2 bytes and keep the harness out of public BA2 claims. [VERIFIED: `src/writer.cpp`; VERIFIED: `10-SPEC.md`] |
| BA2 reading/extraction only | BA2 GNRL and DX10 writer APIs with read-after-write proof | Phase 10 requirements WRT-02/WRT-03. [VERIFIED: `.planning/REQUIREMENTS.md`] | Add writer-specific public surface and tests without changing reader lifetimes. [VERIFIED: `include/libbsa/ba2.hpp`; VERIFIED: `10-CONTEXT.md`] |
| Caller-supplied texture descriptors | Actual DDS bytes/files analyzed during planning | Locked by D-13/D-14. [VERIFIED: `10-CONTEXT.md`] | Planner must include private DDS analyzer work before DX10 serialization tasks. [VERIFIED: `10-SPEC.md`] |
| Deflate-only BA2 compressed assumption | Starfield v3 method-3 raw LZ4-block support | Already implemented by reader/compression phases. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `src/compression.cpp`] | Writer options must expose archive-level method 0/3 and reject unsupported methods. [VERIFIED: `10-CONTEXT.md`] |
| External corpus as writer proof | Generated read-back is sufficient for Phase 10 | Phase 11 owns broad compatibility corpus validation. [VERIFIED: `10-SPEC.md`; VERIFIED: `.planning/ROADMAP.md`] | Focus this phase on source-built fixtures, metadata inspection, extraction, DDS validation, and failure gates. [VERIFIED: `10-SPEC.md`] |

**Deprecated/outdated:**
- Public C++23 `std::expected` remains inappropriate for the current C++20 public API; use `libbsa::result`. [VERIFIED: `.planning/STATE.md`; VERIFIED: `include/libbsa/result.hpp`]
- Using LZ4 frame APIs for Starfield BA2 method-3 payloads is wrong; official LZ4 docs distinguish raw block APIs from frame APIs. [CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h]
- Treating DDS `DX10` writer input as caller-supplied BA2-native descriptors is out of date for Phase 10; context locks actual DDS bytes/files and planning-time analysis. [VERIFIED: `10-CONTEXT.md`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Exact new file names such as `include/libbsa/ba2_writer.hpp`, `src/ba2_writer.cpp`, and `src/texture/dds_analysis.*`. | Summary / Project Structure | Low: context leaves exact helper file names to planner discretion. |
| A2 | A focused `libbsa_ba2_writer_tests` target is preferable to extending `libbsa_writer_tests`. | Standard Stack / Validation Architecture | Low: CMake organization is planner discretion if labels and gates remain clear. |
| A3 | BA2 dedup sharing is safe when byte-identical post-policy GNRL payloads or DDS chunks point to the same archive-absolute bytes. | Architectural Responsibility Map | Medium: current reader can extract by offset/size, but external tool compatibility is deferred to Phase 11. Mitigate with generated read-back now and corpus comparison later. |
| A4 | Conservative one-chunk-per-mip automatic DDS chunking is acceptable for Phase 10 generated read-back unless reference tracing proves stricter target chunk grouping is required. | Pattern 4 | Medium: Archive2 uses texture-streaming heuristics; Step documentation says high mips and low mips are grouped by size thresholds. Mitigate by making chunking internal and testing semantic read-back, while noting Phase 11 corpus comparison may refine grouping. |

## Open Questions

1. **Exact production-compatible DDS chunk grouping heuristic**
   - What we know: BA2 chunk records have `start_mip` / `end_mip`, and public decisions require automatic chunking from DDS analysis. [VERIFIED: `src/ba2_reader.cpp`; VERIFIED: `10-CONTEXT.md`]
   - What's unclear: Official Archive2's exact current chunking heuristic is not first-party documented; Step documentation reports texture archives separate mips above 256x256 and group 256x256 and below, while also noting related settings are not fully understood. [CITED: https://wiki.step-project.com/Guide:Archive2]
   - Recommendation: Implement one-chunk-per-mip or a simple documented internal threshold algorithm that is fully validated by generated read-back in Phase 10; do not expose public chunk controls. Leave exact external-tool parity to Phase 11 corpus validation. [ASSUMED; VERIFIED: `10-SPEC.md`]

2. **Supported DDS formats beyond current reconstruction helper coverage**
   - What we know: `dxgi_format_name` lists several known DXGI values, but current `reconstruct_dds` supports only BC1 value 71. [VERIFIED: `include/libbsa/ba2.hpp`; VERIFIED: `src/texture/dds_reconstruction.cpp`]
   - What's unclear: Phase 10 acceptance names mip, cubemap, DXGI format, chunking, and compression fixtures, but does not enumerate every DXGI format required in v1. [VERIFIED: `10-SPEC.md`]
   - Recommendation: Plan an early DDS analyzer/reconstruction gap task: support at least the formats needed by generated fixtures, fail unsupported formats structurally during planning, and avoid texture transcoding. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `10-SPEC.md`]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build and explicit source wiring | ✓ | 4.3.2 [VERIFIED: shell audit] | — |
| CTest | Test execution | ✓ | 4.3.2 [VERIFIED: shell audit] | — |
| Git | TES5Edit boundary/status gate and optional docs workflow | ✓ | 2.54.0.windows.1 [VERIFIED: shell audit] | — |
| vcpkg executable | Dependency restore via toolchain | ✓ | `2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1` at `C:\vcpkg\vcpkg.exe` [VERIFIED: shell audit] | Existing preset uses `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`; set `VCPKG_ROOT=C:\vcpkg` if needed. [VERIFIED: `CMakePresets.json`] |
| DirectXTex package | DDS analysis/validation | ✓ via vcpkg manifest | `directxtex 2026-03-31#0` package page [CITED: https://vcpkg.io/en/package/directxtex.html] | None recommended; dependency is required by project. [VERIFIED: `AGENTS.md`] |
| MSVC `cl` on current PATH | Direct local compile from plain shell | ✗ | — [VERIFIED: shell audit] | Use Visual Studio generator/preset or developer shell. [VERIFIED: `CMakePresets.json`] |
| Local VS2026 build directory | Fast local CTest commands | ✓ | `build/local-vs2026-vcpkg` exists [VERIFIED: shell audit] | Use configured preset if local directory is stale. [ASSUMED] |

**Missing dependencies with no fallback:**
- None for planning. [VERIFIED: environment audit]

**Missing dependencies with fallback:**
- `cl` is not on PATH in this shell; use configured Visual Studio/CMake preset or developer shell. [VERIFIED: shell audit; VERIFIED: `CMakePresets.json`]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 via existing CMake/vcpkg test setup; vcpkg package page lists `catch2 3.14.0#0`. [VERIFIED: `CMakeLists.txt`; CITED: https://vcpkg.io/en/package/catch2.html] |
| Config file | `CMakeLists.txt` with explicit `add_executable`, `target_sources`, and `catch_discover_tests`; `CMakePresets.json` provides `windows-msvc-vcpkg`. [VERIFIED: `CMakeLists.txt`; VERIFIED: `CMakePresets.json`] |
| Quick run command | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa.public_header_smoke"` [ASSUMED based on existing local build pattern] |
| Full suite command | `ctest --preset windows-msvc-vcpkg --output-on-failure` or `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`. [VERIFIED: `CMakePresets.json`; VERIFIED: shell audit] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| WRT-02 | FO4 v1/v7/v8 and Starfield v2/v3 GNRL writer emits native headers, records, name tables, payload offsets, hashes, compression metadata, and read-after-write succeeds. [VERIFIED: `10-SPEC.md`] | unit/fixture/codec/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` [ASSUMED] | ❌ Wave 0 [VERIFIED: `tests/` glob] |
| WRT-02 | GNRL disk and memory inputs produce read-back-equivalent archives and missing disk inputs fail structurally. [VERIFIED: `10-SPEC.md`] | fixture/io/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` [ASSUMED] | ❌ Wave 0 [VERIFIED: `tests/` glob] |
| WRT-03 | DDS memory and disk inputs are analyzed during planning and emit FO4 v1/v7/v8 and Starfield v3 DX10 texture records/chunks. [VERIFIED: `10-SPEC.md`] | unit/fixture/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_writer_tests` [ASSUMED] | ❌ Wave 0 [VERIFIED: `tests/` glob] |
| WRT-03 | One-mip, multi-mip, cubemap, array, raw, deflate, and LZ4-block DDS outputs extract and validate as DDS. [VERIFIED: `10-SPEC.md`] | fixture/codec/roundtrip | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_dds_reader_tests"` [ASSUMED] | ❌ Wave 0 for writer tests; ✅ existing reader helper coverage [VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`] |
| WRT-02/WRT-03 | Public-header smoke creates/finalizes one FO4 and one Starfield GNRL and DDS archive through public headers only. [VERIFIED: `10-SPEC.md`] | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` [VERIFIED: `CMakeLists.txt`] | ✅ file exists; ❌ BA2 writer coverage gap [VERIFIED: `tests/public_header_smoke.cpp`] |

### Sampling Rate

- **Per task commit:** Run focused BA2 writer tests plus public-header smoke; add BA2 reader/DDS reader tests when changing shared parsing or DDS helpers. [ASSUMED; VERIFIED: `10-SPEC.md`]
- **Per wave merge:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa_writer_tests|libbsa_bsa_writer_tests|libbsa.public_header_smoke"`. [ASSUMED; VERIFIED: existing CMake targets]
- **Phase gate:** Full CTest green, BA2 writer tests green, BA2 reader/DDS reader tests green, writer-core and BSA writer regressions green, public-header private-token grep clean, and `git status --short TES5Edit` empty. [VERIFIED: `10-SPEC.md`; VERIFIED: `AGENTS.md`]

### Wave 0 Gaps

- [ ] `include/libbsa/ba2_writer.hpp` — public BA2 writer target/options/input/plan/finalize declarations. [VERIFIED: `10-CONTEXT.md`]
- [ ] `src/ba2_writer.cpp` — native GNRL and DX10 table planning, compression, dedup, and finalization. [VERIFIED: `10-SPEC.md`]
- [ ] `src/texture/dds_analysis.hpp/.cpp` — private DirectXTex input analysis and mip/chunk byte extraction. [VERIFIED: `10-SPEC.md`; CITED: `/microsoft/directxtex`]
- [ ] `tests/ba2_writer_tests.cpp` — layout, read-after-write, disk/memory, DDS, compression, dedup, and failure coverage. [VERIFIED: `10-SPEC.md`]
- [ ] `tests/public_header_smoke.cpp` update — public BA2 writer compile/link/runtime smoke without private headers. [VERIFIED: `10-SPEC.md`; VERIFIED: current file]
- [ ] `CMakeLists.txt` update — explicit new public header, source, private texture helper, and test target wiring with no `TES5Edit/` inclusion. [VERIFIED: `CMakeLists.txt`; VERIFIED: `AGENTS.md`]
- [ ] DDS reconstruction format coverage review — current reconstruction helper only supports DXGI value 71; Phase 10 fixtures may need additional supported formats or structured planning failures. [VERIFIED: `src/texture/dds_reconstruction.cpp`; VERIFIED: `10-SPEC.md`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | No authentication surface in local archive writer library. [VERIFIED: `10-SPEC.md`] |
| V3 Session Management | no | No session or network state. [VERIFIED: `10-SPEC.md`] |
| V4 Access Control | no | No authorization boundary; callers provide files/bytes and application tooling owns traversal/filtering policy. [VERIFIED: `10-SPEC.md`; VERIFIED: `10-CONTEXT.md`] |
| V5 Input Validation | yes | Normalize archive paths, reject duplicates/missing disk files/malformed DDS/unsupported targets/options, and check all table/payload arithmetic before finalization. [VERIFIED: `10-SPEC.md`; VERIFIED: `include/libbsa/archive_path.hpp`] |
| V6 Cryptography | no | BA2 name/directory hashes and dedup comparisons are compatibility/integrity metadata, not cryptographic controls; do not expose public content hashes. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/hash.cpp`] |

### Known Threat Patterns for C++ BA2 Writer

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Layout integer overflow or truncation to native 32-bit fields | Tampering / Denial of Service | Central checked add/multiply and `uint32_t` / `uint16_t` / `uint8_t` fit checks before plan success. [VERIFIED: `10-SPEC.md`; VERIFIED: `src/bsa_writer.cpp`] |
| Host path and archive path confusion | Tampering | Separate host disk paths from archive virtual paths; validate archive paths before opening host files. [VERIFIED: `src/bsa_writer.cpp`; VERIFIED: `10-SPEC.md`] |
| Malformed DDS input causing invalid output | Tampering / Denial of Service | Analyze and validate DDS during planning through private DirectXTex boundary; return structured failure and no partial plan. [VERIFIED: `10-CONTEXT.md`; CITED: `/microsoft/directxtex`] |
| Codec confusion between deflate, LZ4-frame, and raw LZ4-block | Tampering / Denial of Service | Route only through existing dispatcher with explicit archive format, compression state, and `CompressionMethod`; reject unsupported method values. [VERIFIED: `src/compression.cpp`; VERIFIED: `10-CONTEXT.md`] |
| Partial or ignored sink failure | Repudiation / Integrity | Return the first `byte_sink::write` failure unchanged and cover with a custom failing sink. [VERIFIED: `include/libbsa/io.hpp`; VERIFIED: `src/bsa_writer.cpp`] |
| Untrusted disk file mutation between planning and finalization | Tampering | Read disk bytes during planning; finalization never reopens files. [VERIFIED: `10-CONTEXT.md`; VERIFIED: `src/bsa_writer.cpp`] |
| Public dependency leakage | Information disclosure / Portability | Public header smoke and grep gate over `include/libbsa` for DirectXTex/LZ4/libdeflate/Windows/TES5Edit tokens. [VERIFIED: `10-SPEC.md`; VERIFIED: `AGENTS.md`] |

## Sources

### Primary (HIGH confidence)
- `.planning/phases/10-ba2-writers/10-CONTEXT.md` — locked implementation decisions, scope, canonical refs, and integration points. [VERIFIED]
- `.planning/phases/10-ba2-writers/10-SPEC.md` — locked requirements, boundaries, constraints, and acceptance criteria. [VERIFIED]
- `.planning/REQUIREMENTS.md` — WRT-02/WRT-03 definitions and traceability. [VERIFIED]
- `.planning/ROADMAP.md` — Phase 10 goal, dependencies, success criteria, and Phase 11/12 boundaries. [VERIFIED]
- `.planning/STATE.md` — carry-forward writer-core/BSA writer decisions and local build context. [VERIFIED]
- `AGENTS.md` — TES5Edit boundary, dependency policy, public API policy, documentation/comment policy, and validation expectations. [VERIFIED]
- `include/libbsa/ba2.hpp`, `src/ba2_reader.cpp` — current BA2 GNRL/DX10 reader metadata and extraction semantics. [VERIFIED]
- `include/libbsa/bsa_writer.hpp`, `src/bsa_writer.cpp`, `include/libbsa/writer.hpp`, `src/writer.cpp` — public writer precedent, plan/finalize semantics, disk planning, dedup, finalization. [VERIFIED]
- `include/libbsa/archive_path.hpp`, `include/libbsa/io.hpp`, `include/libbsa/compression.hpp`, `src/compression.cpp` — path, sink/source, compression contracts. [VERIFIED]
- `src/texture/dds_reconstruction.cpp`, `src/texture/dds_validation.cpp`, `tests/ba2_dds_fixture_helpers.cpp`, `tests/public_header_smoke.cpp`, `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json` — private DDS/test/build/dependency patterns. [VERIFIED]
- Context7 `/microsoft/directxtex` — DDS I/O, `TexMetadata`, `ScratchImage`, `Image`, `BitsPerPixel`, and metadata APIs. [CITED]
- Microsoft Learn DDS DX10 docs — `DDS_HEADER_DXT10` fields, cubemap array semantics. [CITED: https://learn.microsoft.com/windows/win32/direct3ddds/dds-header-dxt10]
- vcpkg package pages for libdeflate, lz4, DirectXTex, and Catch2 versions/update dates. [CITED]
- Official libdeflate and LZ4 headers — compression stability warning, raw block vs frame distinction, safe block API requirements. [CITED]

### Secondary (MEDIUM confidence)
- `https://miere.ru/posts/ba2-archive-format/` — BA2 GNRL/DX10 record field cross-check and name table shape. [CITED]
- `https://wiki.step-project.com/Guide:Archive2` — Archive2 texture chunking settings and streaming rationale; useful but not first-party format specification. [CITED]
- Prior phase research `06-RESEARCH.md`, `07-RESEARCH.md`, `08-RESEARCH.md`, and `09-RESEARCH.md` — carry-forward findings verified against then-current code/reference. [VERIFIED]

### Tertiary (LOW confidence)
- Exact planner-chosen file names, test target names, and conservative one-mip-per-chunk algorithm; all are listed in the Assumptions Log. [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all dependencies already exist in manifest/CMake and versions were checked through vcpkg package pages and local tool audit. [VERIFIED/CITED]
- Architecture: HIGH — API shape, plan/finalize semantics, input types, and preview requirements are locked by context and supported by existing Phase 8/9 code. [VERIFIED]
- BA2 GNRL layout: HIGH — current reader and external BA2 layout reference agree on core header/record/name-table semantics. [VERIFIED/CITED]
- BA2 DDS layout: MEDIUM-HIGH — current reader/test helpers verify native DX10 record/chunk fields, but exact production chunk grouping heuristic remains an open compatibility detail for Phase 11. [VERIFIED/CITED]
- DDS input analysis: MEDIUM-HIGH — DirectXTex APIs are verified, but implementation must bridge ScratchImage data ordering to current reconstruction assumptions. [CITED/VERIFIED]
- Pitfalls/security: HIGH — most pitfalls are direct consequences of current reader semantics and locked decisions. [VERIFIED]

**Research date:** 2026-05-07  
**Valid until:** 2026-06-06 for project/layout decisions; re-check package/tool versions before dependency or CI changes. [ASSUMED]
