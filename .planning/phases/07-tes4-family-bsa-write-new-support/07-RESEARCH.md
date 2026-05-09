# Phase 07: tes4-family-bsa-write-new-support - Research

**Researched:** 2026-05-08  
**Domain:** C++20 TES4-family BSA write-new serialization, compression routing, and round-trip validation  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Writer API Shape
- **D-01:** Expose TES4-family write-new as a public writer object, not a single one-shot function. The writer object should hold target options, accept entries, and produce a host-path archive when finalized.
- **D-02:** Memory-buffer entries are copied into writer-owned storage when added. Callers may release or mutate their original buffers after `add_bytes`-style calls without invalidating the future write.
- **D-03:** Phase 7 exposes host-path output only. Do not add memory-output or generic sink-output writer destinations in this phase.
- **D-04:** Existing destination archive paths fail by default. If overwrite is supported, it must be an explicit option rather than the default behavior.

#### Path Handling
- **D-05:** Disk-file entries require an explicit archive-internal path. Do not infer archive paths from host directory layout as the primary Phase 7 API.
- **D-06:** Preserve caller-provided archive path spelling in serialized folder/file name tables, while using existing archive path normalization only for validation, lookup-key derivation, and duplicate detection.
- **D-07:** Reject duplicate canonical archive paths at write time with a structured error. Do not use last-wins behavior, and do not require incremental add-time duplicate rejection.
- **D-08:** Phase 7 writes file entries only. Empty directory representation is out of scope unless required by reference tracing for files already being written.

#### Compression Controls
- **D-09:** Public per-entry compression selection should use a simple state such as `inherit`, `raw`, or `compressed`. The writer maps `compressed` to deflate for v103/v104 and LZ4 frame for v105 using the explicit target profile.
- **D-10:** Archive-wide compression defaults should follow the selected target profile by default, while still allowing callers to choose all-raw or all-compressed archive policy through named options if planner needs those options to satisfy the SPEC.
- **D-11:** Zero-byte entries are stored raw even when the effective policy requests compression.
- **D-12:** Compression failure or an invalid compression request for the selected target profile fails the write with a structured error. Do not silently fall back to raw, and do not add warning-and-continue behavior before the Phase 11 warning API exists.
- **D-13:** Codec compression-level tuning stays internal in Phase 7. The public API exposes per-entry raw/compressed selection, not libdeflate or LZ4 level knobs.
- **D-14:** The writer should set the archive compression flag to match the archive-wide default and use per-file toggle bits only for entries that deviate from that default.
- **D-15:** For non-empty files, honor the effective compression policy. Do not add a hidden size-threshold rule that stores tiny files raw when the policy says compressed.

#### Compatibility Knobs
- **D-16:** Callers should not set raw archive flag bits directly in Phase 7. The writer derives required flags and exposes only safe named options for compatibility-sensitive behaviors.
- **D-17:** Derive file category flags as the union of known category bits based on entry extensions across the archive. Researcher/planner must trace BSArchPro/TES5Edit for the exact extension-to-flag mapping before implementation.
- **D-18:** Embedded names are off by default and controlled by one global archive option. If enabled, the writer emits embedded name prefixes only where target-compatible; do not expose per-entry embedded-name opt-in in Phase 7 unless reference tracing proves global behavior is unsafe.
- **D-19:** Opt-in deduplication may share payload offsets only for entries whose final stored bytes are identical after compression policy, embedded-name policy, and size-prefix encoding are applied. Do not dedupe merely because source bytes match if stored encoding differs.

#### Carry-Forward Decisions
- **D-20:** Preserve the established dependency-light public boundary: no public libdeflate, lz4, DirectXTex, Windows SDK, `std::expected`, or `TES5Edit/` types.
- **D-21:** Preserve C++20 result/error style for writer failures, reserving exceptions for programmer precondition misuse consistent with the existing `result` behavior.
- **D-22:** Preserve existing archive-reader validation as the writer acceptance oracle: writer tests must reopen produced archives with `archive_reader` and verify public metadata, lookup behavior, extraction bytes, and stable errors.
- **D-23:** Preserve generated legal fixture/test policy. Production writer tests may generate writer-output archives during tests, but committed fixtures and generated manifests must document synthetic provenance and must not use `TES5Edit/` as a mutable workspace.

### the agent's Discretion
- Researcher/planner may choose exact class/function names, source file names, helper boundaries, CMake target organization, and test file organization if the decisions above and `07-SPEC.md` are preserved.
- Researcher/planner should trace TES5Edit/BSArchPro before locking binary layout details: folder/file sorting, folder record offsets, archive flags, file category flags, embedded-name compatibility by version, and compression toggle semantics.
- Planner may choose exact generated writer-output filenames, entry paths, payload bytes, and manifest shape as long as disk-source, memory-source, zero-byte, mixed-case lookup, multi-folder, compression override, embedded-name, and dedupe acceptance cases are covered.

### Deferred Ideas (OUT OF SCOPE)

None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WBSA-01 | Consumer can create new TES4/Oblivion BSA v103 archives from disk files or memory buffers. | Use explicit target profile `v103`, legacy 16-byte folder records, raw/deflate routing, and reader reopen proof. [VERIFIED: `.planning/REQUIREMENTS.md`; `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`] |
| WBSA-02 | Consumer can create new FO3/FNV/Skyrim LE BSA v104 archives from disk files or memory buffers. | Use explicit target profile `v104`, legacy folder records, raw/deflate routing, optional embedded-name prefix only when global option is enabled. [VERIFIED: `.planning/REQUIREMENTS.md`; `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`] |
| WBSA-03 | Consumer can create new Skyrim SE/AE BSA v105 archives from disk files or memory buffers. | Use explicit target profile `v105`, 24-byte SSE folder records, and LZ4-frame routing for compressed entries. [VERIFIED: `.planning/REQUIREMENTS.md`; `TES5Edit/Core/wbBSArchive.pas`; `/lz4/lz4`] |
| WBSA-05 | Writer can generate folder and file indexes sorted by format-compatible hash order. | TES5Edit sorts by folder hash then file hash; libbsa already has `detail::hash_tes4`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/detail/bethesda_hash.cpp`] |
| WBSA-06 | Writer can derive archive flags and file flags from content using compatible behavior. | TES5Edit file flag constants and extension/root mapping are traced below; planner should implement the same union rules with Phase 7 policy overrides. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `TES5Edit/BSArch/wbAssets.pas`] |
| WBSA-07 | Writer can apply per-file compression overrides while respecting target archive defaults. | TES5Edit uses archive `ARCHIVE_COMPRESS` plus per-file `FILE_SIZE_COMPRESS` XOR toggle; Phase 7 locked the same archive-default/toggle model. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md`] |
| WBSA-08 | Writer can write embedded file names where appropriate without triggering known compatibility hazards. | Existing parser only treats embedded names as active for non-v103 archives and TES5Edit writes embedded prefixes only for FO3/SSE when the archive flag is set. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`] |
| WBSA-09 | Writer can optionally deduplicate identical file payloads by content hash. | TES5Edit has shared-data dedupe keyed by data size plus MD5 before writing, but Phase 7 must dedupe final stored bytes per locked D-19. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md`] |
| WBSA-10 | Maintainer can round-trip BSA writer output by packing, reopening, extracting, and byte-comparing source files. | Existing reader and tests already provide metadata, lookup, extraction, and byte comparison patterns for writer-output tests. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`; `.planning/phases/07-tes4-family-bsa-write-new-support/07-SPEC.md`] |
</phase_requirements>

## Summary

Phase 7 should plan a production TES4-family write-new path that reuses libbsa's existing archive path normalization, TES4 hash helper, binary little-endian primitives, and compression router rather than copying test fixture generator code into the public API. [VERIFIED: `src/detail/archive_path.cpp`; `src/detail/bethesda_hash.cpp`; `src/detail/compression_router.cpp`; `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`] The writer must be a public C++20 object with dependency-light options and no public codec or TES5Edit types. [VERIFIED: `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md`; `AGENTS.md`; `include/libbsa/archive.hpp`]

The critical implementation risks are not API naming but binary layout: TES4-family BSA tables require hash-sorted folder/file records, a file-name table after folder blocks, folder offsets that include the later file-name-table contribution, v105/SSE wider folder records, compression size prefixes, and XOR-style per-file compression toggle bits. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`; `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`] Embedded names are compatibility-sensitive: default off is correct for Phase 7, and when enabled the writer should only emit prefixes for non-v103 targets accepted by the existing parser. [VERIFIED: `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md`; `src/formats/bsa/tes4_bsa_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`]

**Primary recommendation:** Plan one public `tes4_bsa_writer`-style object plus one private `tes4_bsa_writer` implementation that builds a normalized entry model, encodes final stored payloads, optionally deduplicates encoded bytes, serializes tables/payloads in one deterministic host-path write, then validates through `archive_reader` round-trip tests. [VERIFIED: `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md`; `src/archive.cpp`; `tests/unit/tes4_bsa_reader_tests.cpp`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Public writer API and options | Public C++ API (`include/libbsa/`) | Private writer implementation | Public headers must expose dependency-light C++20 types and hide libdeflate, lz4, DirectXTex, Windows SDK, and TES5Edit details. [VERIFIED: `AGENTS.md`; `include/libbsa/archive.hpp`; `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md`] |
| Entry validation and duplicate canonical path detection | Private writer model | Existing path helper | Existing normalization lowercases ASCII, converts `\` to `/`, rejects rooted/empty/dot segments, and current readers reject duplicate canonical paths. [VERIFIED: `src/detail/archive_path.cpp`; `src/formats/bsa/tes4_bsa_parser.cpp`] |
| Folder/file hash ordering | Private writer model | Existing hash helper | TES5Edit sorts files by directory hash then file hash; `detail::hash_tes4` mirrors TES5Edit's CreateHashTES4 edge-byte/sdbm logic. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/detail/bethesda_hash.cpp`] |
| Payload compression and size-prefix encoding | Private writer implementation | Existing compression router | `compress_payload` routes `none`, `deflate`, and `lz4_frame`; TES4-family compressed payloads carry a 4-byte uncompressed-size prefix before compressed bytes. [VERIFIED: `src/detail/compression_router.cpp`; `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`] |
| BSA binary table serialization | Private writer implementation | Existing binary I/O helpers | Parser and fixture generator define supported fixed header, folder record, file record, folder-name, file-name, and payload layout. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`] |
| Round-trip validation | Tests / CTest | Existing `archive_reader` | Phase 7 acceptance requires reopen/list/find/contains/extract/byte-compare through public reader APIs. [VERIFIED: `07-SPEC.md`; `tests/unit/tes4_bsa_reader_tests.cpp`] |

## Project Constraints (from AGENTS.md)

- Implementation language is C++; public interfaces should be reusable, portable, and independent of UI/tooling. [VERIFIED: `AGENTS.md`]
- `TES5Edit/` is read-only: do not edit, format, stage, update the submodule pointer, compile it into libbsa, or use it as mutable fixture workspace. [VERIFIED: `AGENTS.md`]
- Preserve BSArchPro/TES5Edit-discovered archive behavior unless a divergence is documented. [VERIFIED: `AGENTS.md`]
- Use `libdeflate` for deflate compression/decompression and official `lz4` for LZ4 compression/decompression. [VERIFIED: `AGENTS.md`; `vcpkg.json`; `CMakeLists.txt`]
- Use `DirectXTex` only for texture analysis; this phase has no TES4-family texture-analysis need beyond extension-derived flags. [VERIFIED: `AGENTS.md`; `07-SPEC.md`]
- Do not introduce speculative external dependencies; if a dependency becomes useful, document need, alternatives, and impact before adding it. [VERIFIED: `AGENTS.md`]
- Never delete accurate comments as cleanup; add comments for non-obvious format compatibility, ownership/lifetime, error handling, and deviations from reference behavior. [VERIFIED: `AGENTS.md`]
- Add Doxygen-compliant C++ comments for public APIs and methods added or substantially rewritten. [VERIFIED: `AGENTS.md`]
- Add focused tests for archive parsing, writing, round-tripping, and compatibility behavior; legal fixtures must not mutate `TES5Edit/`. [VERIFIED: `AGENTS.md`]

## Standard Stack

### Core

| Library / Component | Version | Purpose | Why Standard |
|---------------------|---------|---------|--------------|
| C++ | C++20 via `target_compile_features(libbsa PUBLIC cxx_std_20)` | Public writer API and implementation language | Project requires C++20 and existing targets compile as C++20. [VERIFIED: `CMakeLists.txt`; `AGENTS.md`] |
| CMake | Minimum 3.24; local tool reports 4.3.2 | Build, test, install file-set updates | Repository uses CMake presets and target file sets for public headers. [VERIFIED: `CMakeLists.txt`; `CMakePresets.json`; `cmake --version`] |
| vcpkg manifest mode | `builtin-baseline` committed in `vcpkg.json`; `vcpkg` not on PATH in this shell | Dependency acquisition | Project dependencies are declared in `vcpkg.json`; local environment needs `VCPKG_ROOT`/vcpkg availability before configure. [VERIFIED: `vcpkg.json`; environment audit] |
| libdeflate | vcpkg package `1.25#0`, package page last updated 2025-11-03 | v103/v104 raw DEFLATE compression | Existing `compress_deflate` wraps `libdeflate_deflate_compress_bound` and `libdeflate_deflate_compress`; libdeflate docs warn compressed bytes may differ by version, so tests must compare extracted bytes, not compressed golden bytes. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://github.com/ebiggers/libdeflate/blob/master/libdeflate.h; VERIFIED: `src/detail/deflate_codec.cpp`] |
| lz4 | vcpkg package `1.10.0#0`, package page last updated 2024-07-25 | v105/SSE LZ4 frame compression | Existing `compress_lz4_frame` uses `LZ4F_compressFrameBound` and one-shot `LZ4F_compressFrame`; LZ4 frame docs say this produces a complete LZ4 frame and errors are checked with `LZ4F_isError`. [CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://github.com/lz4/lz4/blob/dev/doc/lz4frame_manual.html; VERIFIED: `src/detail/lz4_frame_codec.cpp`] |
| Catch2 + CTest | Catch2 vcpkg package `3.14.0#0`; CTest local version 4.3.2 | Writer unit/round-trip tests | Existing tests use Catch2, `catch_discover_tests`, and `ADD_TAGS_AS_LABELS`; Catch2 docs confirm `ADD_TAGS_AS_LABELS` maps tags to CTest labels. [CITED: https://vcpkg.io/en/package/catch2.html; CITED: `/catchorg/catch2`; VERIFIED: `tests/CMakeLists.txt`] |

### Supporting

| Library / Component | Version | Purpose | When to Use |
|---------------------|---------|---------|-------------|
| Existing `detail::normalize_archive_path` | Project source | Canonical validation and duplicate detection | Use before final write to reject invalid/duplicate canonical paths while preserving caller spelling for serialized names. [VERIFIED: `src/detail/archive_path.cpp`; `07-CONTEXT.md`] |
| Existing `detail::hash_tes4` | Project source | Folder and file hash computation | Use for folder hash (`folder`, empty extension) and file hash (`file name split into name/ext`) ordering and record values. [VERIFIED: `src/detail/bethesda_hash.cpp`; `TES5Edit/Core/wbBSArchive.pas`] |
| Existing `detail::compress_payload` | Project source | Central compression route | Use `none`, `deflate`, or `lz4_frame`; do not call codec libraries directly from writer table logic. [VERIFIED: `src/detail/compression_router.hpp`; `src/detail/compression_router.cpp`] |
| Existing `archive_reader` | Project source | Acceptance oracle | Reopen writer output and verify metadata, lookup, extraction, and byte equality through public APIs. [VERIFIED: `include/libbsa/archive.hpp`; `tests/unit/tes4_bsa_reader_tests.cpp`; `07-SPEC.md`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Public writer object | Single one-shot function | Locked decision D-01 requires a writer object holding options and entries. [VERIFIED: `07-CONTEXT.md`] |
| Existing compression router | Direct `libdeflate` / `LZ4F_*` calls in writer | Direct calls duplicate error translation and risk public/private dependency leakage; router already centralizes routes. [VERIFIED: `src/detail/compression_router.cpp`; `AGENTS.md`] |
| Reopen/extract test oracle | Byte-compare full archive against golden compressed bytes | libdeflate official header warns compressed bytes are not stable across libdeflate versions, so compare consumer-visible extraction bytes and metadata instead. [CITED: https://github.com/ebiggers/libdeflate/blob/master/libdeflate.h] |
| Final-stored-byte dedupe | Source-byte MD5 before encoding | TES5Edit uses source data size+MD5 for shared data, but locked D-19 requires dedupe after compression/embedded-name/size-prefix encoding. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `07-CONTEXT.md`] |

**Installation:** No new dependency install should be planned; required packages already exist in `vcpkg.json`. [VERIFIED: `vcpkg.json`]

```bash
# Configure through existing presets after VCPKG_ROOT/vcpkg are available.
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure
```

**Version verification:** `libdeflate` `1.25#0`, `lz4` `1.10.0#0`, and `catch2` `3.14.0#0` were verified from vcpkg package pages; CMake/CTest 4.3.2 were verified locally; `vcpkg` and `cl` were not on PATH in this shell. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://vcpkg.io/en/package/catch2.html; VERIFIED: environment audit]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer code
  |
  v
Public TES4-family writer object (target profile + archive policy + copied memory entries + disk entry descriptors)
  |
  v
Entry normalization stage
  |-- reject invalid archive paths / duplicate canonical keys
  |-- split preserved original folder/file spelling for tables
  v
Index construction stage
  |-- compute TES4 folder/file hashes
  |-- sort by folder hash then file hash
  |-- derive archive flags and file category flags
  v
Payload encoding stage
  |-- effective compression policy? raw / deflate / LZ4 frame
  |-- embedded names enabled and target-compatible? prefix encoded stored bytes
  |-- compressed? add 4-byte raw-size prefix before compressed bytes
  |-- dedupe enabled? share offsets for identical final stored bytes
  v
Binary serialization stage
  |-- fixed BSA header
  |-- v103/v104 16-byte or v105 24-byte folder records
  |-- folder name + file records blocks
  |-- file name table
  |-- payload bytes at archive-absolute offsets
  v
Host-path BSA file
  |
  v
Existing archive_reader reopen/list/find/contains/extract byte-compare tests
```

This flow follows the locked write-new host-path API and existing reader validation oracle. [VERIFIED: `07-CONTEXT.md`; `07-SPEC.md`; `include/libbsa/archive.hpp`]

### Recommended Project Structure

```text
include/libbsa/
├── archive.hpp              # existing reader metadata; add writer enums here only if small
├── writer.hpp               # preferred new public writer surface if API is non-trivial
└── libbsa.hpp               # umbrella include must include writer surface

src/formats/bsa/
├── tes4_bsa_writer.hpp      # private writer model and serializer declarations
└── tes4_bsa_writer.cpp      # target profiles, table layout, payload encoding, host-path write

tests/unit/
├── tes4_bsa_writer_tests.cpp              # round-trip behavior via public API
└── public_include_boundary_tests.cpp      # extend installed-header writer smoke coverage
```

This structure matches existing public/private source registration and CMake file-set patterns. [VERIFIED: `CMakeLists.txt`; `include/libbsa/libbsa.hpp`; `tests/CMakeLists.txt`]

### Pattern 1: Writer object with copied memory entries and deferred validation

**What:** Store a target profile, named options, disk-entry descriptors, and copied byte buffers, then validate/sort/encode on `write_to(host_path)`. [VERIFIED: `07-CONTEXT.md`]  
**When to use:** Use for the only Phase 7 public creation path. [VERIFIED: `07-SPEC.md`]

```cpp
// Source: Phase decisions in 07-CONTEXT.md and existing result-style public API in include/libbsa/archive.hpp.
libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::skyrim_se};
writer.set_compression_policy(libbsa::archive_compression_policy::target_default);
writer.set_embedded_names_enabled(true);
writer.set_deduplication_enabled(true);

auto added_disk = writer.add_file("Meshes/Tiny/RawMesh.nif", source_path,
                                  libbsa::entry_compression_policy::raw);
auto added_bytes = writer.add_bytes("Scripts/SSE/PackedScript.pex", script_bytes,
                                    libbsa::entry_compression_policy::compressed);
auto written = writer.write_to(output_path);
```

### Pattern 2: Final stored payloads drive record sizes, toggles, and dedupe

**What:** Build `stored_payload = [embedded-name-prefix?][raw-size-prefix?][payload-bytes]`; compute `stored_size` from that final vector; set `FILE_SIZE_COMPRESS` only when effective compression differs from archive default; dedupe by identical final bytes. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`; `07-CONTEXT.md`]  
**When to use:** Use for every non-empty entry; zero-byte entries are raw by locked decision. [VERIFIED: `07-CONTEXT.md`]

```cpp
// Source: TES5Edit/Core/wbBSArchive.pas PackData and src/formats/bsa/tes4_bsa_parser.cpp raw_size_for.
const bool archive_default_compressed = options.archive_default_compressed;
const bool effective_compressed = entry.raw_size != 0 && resolve_entry_policy(entry) == compressed;
const auto method = effective_compressed ? compression_for_target(profile) : compression_method::none;

byte_buffer stored;
if (emit_embedded_name(profile, options)) {
  stored.u8(static_cast<std::uint8_t>(embedded_name.size()));
  stored.bytes(embedded_name);
}
if (effective_compressed) {
  stored.u32_le(entry.raw_size);
  stored.bytes(compress_payload(method, entry.source_bytes).value());
} else {
  stored.bytes(entry.source_bytes);
}

record.size_flags = checked_u32(stored.size());
if (archive_default_compressed != effective_compressed) {
  record.size_flags |= 0x40000000U;
}
```

### Pattern 3: Hash-sorted folder/file table model

**What:** Group entries by preserved folder spelling but sort by `hash_tes4(folder, empty ext)` and within folder by `hash_tes4(file_name_without_ext, ext)`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/detail/bethesda_hash.cpp`]  
**When to use:** Use before computing table sizes and offsets. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`]

### Anti-Patterns to Avoid

- **Inferring archive paths from host paths as primary API:** D-05 explicitly requires disk-file entries to provide an archive-internal path. [VERIFIED: `07-CONTEXT.md`]
- **Comparing compressed bytes to golden compressed fixtures:** libdeflate documentation says compressed output can differ between versions even at the same level. [CITED: https://github.com/ebiggers/libdeflate/blob/master/libdeflate.h]
- **Deduping source bytes before encoding:** Phase 7 dedupe must consider final stored bytes after compression policy, embedded names, and size-prefix encoding. [VERIFIED: `07-CONTEXT.md`]
- **Using `std::filesystem::path` as archive-internal path type:** Existing archive normalization treats archive paths as virtual slash-separated keys, not host filesystem paths. [VERIFIED: `src/detail/archive_path.cpp`; `AGENTS.md`]
- **Mutating or compiling TES5Edit:** Repository policy forbids using `TES5Edit/` as implementation source or fixture workspace. [VERIFIED: `AGENTS.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| TES4 hash function | New hash implementation | `detail::hash_tes4` | Existing code is traced to TES5Edit `CreateHashTES4` and already fixture-tested. [VERIFIED: `src/detail/bethesda_hash.cpp`; `TES5Edit/Core/wbBSArchive.pas`] |
| Archive path canonicalization | Ad-hoc lowercase/replace logic | `detail::normalize_archive_path` | Existing helper defines invalid path behavior used by readers and tests. [VERIFIED: `src/detail/archive_path.cpp`; `tests/unit/tes4_bsa_reader_tests.cpp`] |
| Deflate/LZ4 routing | Direct codec branches in writer API | `detail::compress_payload` | Router keeps compression methods explicit and private. [VERIFIED: `src/detail/compression_router.cpp`] |
| Round-trip proof | Writer-internal parser/assertions only | `archive_reader::open`, `entries`, `find`, `contains`, `extract_bytes` | Acceptance requires public reader validation; existing tests provide patterns. [VERIFIED: `07-SPEC.md`; `include/libbsa/archive.hpp`; `tests/unit/tes4_bsa_reader_tests.cpp`] |
| Public dependency exposure | Public `libdeflate`, `lz4`, or TES5Edit types | libbsa-owned enums/options/result | Public dependency-light boundary is locked by project and phase decisions. [VERIFIED: `AGENTS.md`; `07-CONTEXT.md`] |

**Key insight:** The deceptively complex part is not file output; it is matching TES4-family table and payload semantics closely enough that the already-strict reader accepts writer output. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`]

## Common Pitfalls

### Pitfall 1: Wrong folder record offset arithmetic
**What goes wrong:** Reopened archives fail with `folder block offset does not match parsed table layout`. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`]  
**Why it happens:** TES5Edit-compatible folder offsets include the later file-name table contribution even though folder blocks are read before file names. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`]  
**How to avoid:** Compute `folder.offset = position_of_folder_block + total_file_name_length`, and for v105 use 24-byte folder records. [VERIFIED: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`; `src/formats/bsa/tes4_bsa_parser.cpp`]  
**Warning signs:** v103 fixture generator works for one folder but multi-folder writer output fails during open. [VERIFIED: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`; `07-SPEC.md`]

### Pitfall 2: Reversing compression toggle semantics
**What goes wrong:** Metadata reports raw entries as compressed or compressed entries as raw. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`]  
**Why it happens:** `FILE_SIZE_COMPRESS` is an XOR toggle against archive default compression, not an absolute compressed bit. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`]  
**How to avoid:** Set archive `ARCHIVE_COMPRESS` for archive-wide default and set per-file toggle only when effective entry compression differs from the archive default. [VERIFIED: `07-CONTEXT.md`; `TES5Edit/Core/wbBSArchive.pas`]  
**Warning signs:** Tests with mixed raw/compressed entries pass only when archive default is raw. [VERIFIED: `07-SPEC.md`]

### Pitfall 3: Embedded names on the wrong targets
**What goes wrong:** Consumer-visible extraction may include prefixes or parser metadata may reject/ignore expected embedded names. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; `tests/unit/tes4_bsa_reader_tests.cpp`]  
**Why it happens:** Existing parser treats embedded names as active only when `version != 0x67` and archive flag `0x0100` is set; TES5Edit writes embedded prefixes for FO3/SSE when the flag is set. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; `TES5Edit/Core/wbBSArchive.pas`]  
**How to avoid:** Default off; if enabled, emit only for v104/v105 unless further reference tracing changes the policy. [VERIFIED: `07-CONTEXT.md`; `src/formats/bsa/tes4_bsa_parser.cpp`]  
**Warning signs:** v103 embedded-name tests report `has_embedded_name == false` despite flag bits. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`]

### Pitfall 4: Invalid file flags for target version
**What goes wrong:** Reopened metadata has unexpected file flags, or compatibility with target game tools diverges. [VERIFIED: `07-SPEC.md`; `TES5Edit/Core/wbBSArchive.pas`]  
**Why it happens:** TES5Edit applies extension/root-derived flags, strips `FILE_MISC` for SSE, and strips XML/TXT/FNT for non-Oblivion. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`]  
**How to avoid:** Implement file flag derivation as a tested helper with version-specific post-processing. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`]  
**Warning signs:** SSE archives containing miscellaneous extensions report `FILE_MISC` or FO3 archives report XML/TXT/FNT flags. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`]

### Pitfall 5: Dedupe offset sharing before encoding
**What goes wrong:** Two entries with same source bytes but different stored encoding share an offset and one extracts incorrectly. [VERIFIED: `07-CONTEXT.md`]  
**Why it happens:** Source-level bytes do not include compression method, raw-size prefix, or embedded-name prefix. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`]  
**How to avoid:** Compute hash/key over final stored bytes and stored size; share only identical final stored vectors. [VERIFIED: `07-CONTEXT.md`]  
**Warning signs:** Dedupe works for all-raw archive but fails when one entry inherits compression and another overrides raw. [VERIFIED: `07-SPEC.md`; `07-CONTEXT.md`]

## Code Examples

Verified patterns from official or project sources:

### LZ4 frame compression for v105

```c
// Source: https://github.com/lz4/lz4/blob/dev/doc/lz4frame_manual.html and src/detail/lz4_frame_codec.cpp
size_t dstCapacity = LZ4F_compressFrameBound(srcSize, NULL);
size_t frameSize = LZ4F_compressFrame(dst, dstCapacity, src, srcSize, NULL);
if (LZ4F_isError(frameSize)) {
  /* map to libbsa::error_code::io_error */
}
```

### Raw DEFLATE compression for v103/v104

```c
// Source: https://github.com/ebiggers/libdeflate/blob/master/libdeflate.h and src/detail/deflate_codec.cpp
struct libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
size_t bound = libdeflate_deflate_compress_bound(compressor, input_size);
size_t actual = libdeflate_deflate_compress(compressor, input, input_size, output, bound);
if (actual == 0) {
  /* map to libbsa::error_code::io_error */
}
libdeflate_free_compressor(compressor);
```

### Reopen/extract writer output

```cpp
// Source: tests/unit/tes4_bsa_reader_tests.cpp and include/libbsa/archive.hpp
auto opened = libbsa::archive_reader::open(output_archive.string());
REQUIRE(opened.has_value());

auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
REQUIRE(metadata.value().version == 105U);

auto bytes = opened.value().extract_bytes("scripts/sse/packedscript.pex");
REQUIRE(bytes.has_value());
REQUIRE(bytes.value() == expected_source_bytes);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Test-only TES4 serializer in fixture generator | Production public writer plus tests that reuse reader oracle | Phase 7 | Do not reuse fixture generator as public API; extract compatible layout knowledge into production code. [VERIFIED: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`; `07-SPEC.md`] |
| Golden compressed byte comparisons | Metadata + extraction byte comparisons | Existing libdeflate guidance | Avoids brittle failures across codec versions. [CITED: https://github.com/ebiggers/libdeflate/blob/master/libdeflate.h] |
| Extension-inferred compression | Explicit target profile plus per-entry policy | Phase 7 locked | v103/v104 compressed means deflate; v105 compressed means LZ4 frame. [VERIFIED: `07-CONTEXT.md`; `src/detail/compression_router.cpp`] |
| Source-byte dedupe | Final stored-byte dedupe | Phase 7 locked | Prevents sharing offsets across incompatible stored encodings. [VERIFIED: `07-CONTEXT.md`] |

**Deprecated/outdated:**
- Using test fixture generator code as the writer implementation is not acceptable because Phase 7 requires consumer-facing public APIs and production implementation outside test-only tooling. [VERIFIED: `07-SPEC.md`; `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`]
- Using zlib wrappers for TES4-family deflate is not supported by the current stack; existing libbsa codec uses raw libdeflate DEFLATE and readers validate exact-size raw deflate. [VERIFIED: `src/detail/deflate_codec.cpp`; `src/formats/bsa/tes4_bsa_parser.cpp`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|

All claims in this research were verified or cited — no user confirmation needed.

## Open Questions — RESOLVED

1. **Exact public type/function names — RESOLVED**
   - What we know: Writer object shape and semantics are locked, but exact names are discretionary. [VERIFIED: `07-CONTEXT.md`]
   - Resolution: Place the writer declarations in new public header `include/libbsa/writer.hpp` and include that header from `include/libbsa/libbsa.hpp`. [RESOLVED: selected by planner discretion; VERIFIED: `include/libbsa/libbsa.hpp`; `CMakeLists.txt`; `07-01-PLAN.md`]
   - Selected public names: `tes4_bsa_target`, `archive_compression_policy`, `entry_compression_policy`, `tes4_bsa_writer_options`, and `tes4_bsa_writer`. [RESOLVED: selected by planner discretion; VERIFIED: `07-01-PLAN.md`]

2. **Archive-wide default policy names — RESOLVED**
   - What we know: Target defaults plus all-raw/all-compressed named options are allowed if needed. [VERIFIED: `07-CONTEXT.md`]
   - Resolution: Use `archive_compression_policy::{target_default, all_raw, all_compressed}` and `entry_compression_policy::{inherit, raw, compressed}`. [RESOLVED: selected by planner discretion; VERIFIED: `07-CONTEXT.md`; `07-01-PLAN.md`]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build/test | ✓ | 4.3.2 | — [VERIFIED: environment audit] |
| CTest | Test execution | ✓ | 4.3.2 | — [VERIFIED: environment audit] |
| Git | Status/submodule boundary checks | ✓ | 2.54.0.windows.1 | — [VERIFIED: environment audit] |
| vcpkg executable | Dependency restore/configure via presets | ✗ on PATH | — | Set `VCPKG_ROOT` and ensure `vcpkg` is available before configure. [VERIFIED: environment audit; `CMakePresets.json`] |
| MSVC `cl` | Windows preset compile | ✗ in current shell PATH | — | Run from Developer PowerShell/VS environment or configure with an available C++20 compiler. [VERIFIED: environment audit; `CMakePresets.json`] |
| Ninja | Optional generator | ✗ on PATH | — | CMake can use Visual Studio/MSBuild generator on Windows. [VERIFIED: environment audit] |

**Missing dependencies with no fallback:**
- A C++20 compiler environment and vcpkg availability are required to execute the planned build/test commands locally. [VERIFIED: environment audit; `CMakePresets.json`]

**Missing dependencies with fallback:**
- Ninja is missing but not required by existing presets. [VERIFIED: environment audit; `CMakePresets.json`]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 vcpkg `3.14.0#0` + CTest 4.3.2 [CITED: https://vcpkg.io/en/package/catch2.html; VERIFIED: environment audit] |
| Config file | `tests/CMakeLists.txt` [VERIFIED: `tests/CMakeLists.txt`] |
| Quick run command | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` [VERIFIED: `CMakePresets.json`; `tests/CMakeLists.txt`] |
| Full suite command | `ctest --preset windows-msvc-debug-static --output-on-failure` [VERIFIED: `CMakePresets.json`] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WBSA-01 | v103 archive from disk and memory sources | integration/unit | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `tests/CMakeLists.txt`; `07-SPEC.md`] |
| WBSA-02 | v104 archive from disk and memory sources | integration/unit | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `07-SPEC.md`] |
| WBSA-03 | v105 archive from disk and memory sources with LZ4 frame compression | integration/unit | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `07-SPEC.md`; `src/detail/lz4_frame_codec.cpp`] |
| WBSA-05 | Hash-sorted multi-folder indexes | unit/integration | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`] |
| WBSA-06 | Archive/file flags for mesh/texture/script-style paths | unit/integration | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`] |
| WBSA-07 | Archive default plus per-entry raw/compressed overrides | integration | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `07-SPEC.md`; `07-CONTEXT.md`] |
| WBSA-08 | Embedded-name disabled/enabled round trips | integration | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `07-SPEC.md`] |
| WBSA-09 | Dedupe disabled/enabled offset behavior | integration | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `07-SPEC.md`; `07-CONTEXT.md`] |
| WBSA-10 | Pack/reopen/find/contains/extract/byte-compare | integration | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer` | ❌ Wave 0 [VERIFIED: `07-SPEC.md`; `tests/unit/tes4_bsa_reader_tests.cpp`] |

### Sampling Rate
- **Per task commit:** `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` once writer tests exist. [VERIFIED: `CMakePresets.json`; `tests/CMakeLists.txt`]
- **Per wave merge:** `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: `CMakePresets.json`]
- **Phase gate:** Static and shared preset tests plus package consumer smoke after public writer headers are installed. [VERIFIED: `CMakePresets.json`; `tests/CMakeLists.txt`; `07-SPEC.md`]

### Wave 0 Gaps
- [ ] `tests/unit/tes4_bsa_writer_tests.cpp` — covers WBSA-01/02/03/05/06/07/08/09/10. [VERIFIED: `07-SPEC.md`; `tests/CMakeLists.txt`]
- [ ] Extend `tests/unit/public_include_boundary_tests.cpp` or package consumer smoke to instantiate writer via installed headers. [VERIFIED: `07-SPEC.md`; `tests/unit/public_include_boundary_tests.cpp`; `tests/CMakeLists.txt`]
- [ ] Add writer test source to `tests/CMakeLists.txt` and ensure Catch tags include `[tes4_bsa_writer]`. [VERIFIED: `tests/CMakeLists.txt`; `/catchorg/catch2`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No identity/authentication capability in local C++ archive writer. [VERIFIED: `07-SPEC.md`] |
| V3 Session Management | no | No session state. [VERIFIED: `07-SPEC.md`] |
| V4 Access Control | no | No multi-user authorization boundary; host filesystem writes still need explicit overwrite policy. [VERIFIED: `07-CONTEXT.md`] |
| V5 Input Validation | yes | Validate archive paths with `detail::normalize_archive_path`, reject duplicate canonical paths, check integer/offset bounds before serialization. [VERIFIED: `src/detail/archive_path.cpp`; `src/formats/bsa/tes4_bsa_parser.cpp`] |
| V6 Cryptography | limited | Do not treat dedupe content hashes as security; use only for deterministic equality/dedupe, not trust/security. [VERIFIED: `07-CONTEXT.md`; `TES5Edit/Core/wbBSArchive.pas`] |

### Known Threat Patterns for TES4-family writer

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Path traversal / rooted archive virtual paths | Tampering | Reject empty, rooted, drive-rooted, `.`/`..`, and doubled-separator archive paths with existing normalization. [VERIFIED: `src/detail/archive_path.cpp`] |
| Integer overflow in table/payload size accounting | Denial of Service / Tampering | Use checked `size_t`/`uint32_t` arithmetic and fail with structured errors before write. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`] |
| Existing destination overwrite | Tampering | Fail existing output by default; require explicit overwrite option if implemented. [VERIFIED: `07-CONTEXT.md`] |
| Compression failure fallback | Tampering | Fail write on compression failure; do not silently store raw when policy requested compressed. [VERIFIED: `07-CONTEXT.md`] |
| Mutable caller buffer after add | Tampering / Race | Copy memory-buffer entries into writer-owned storage at add time. [VERIFIED: `07-CONTEXT.md`] |

## Sources

### Primary (HIGH confidence)
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md` — locked decisions, scope, acceptance policy, canonical refs. [VERIFIED: file read]
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-SPEC.md` — requirements and acceptance criteria. [VERIFIED: file read]
- `.planning/REQUIREMENTS.md` — WBSA requirement IDs and v1 constraints. [VERIFIED: file read]
- `.planning/STATE.md` — carry-forward project decisions and prior phase state. [VERIFIED: file read]
- `AGENTS.md` — TES5Edit read-only boundary, dependency, documentation, validation rules. [VERIFIED: file read]
- `TES5Edit/Core/wbBSArchive.pas` — BSA constants, hash sorting, flags, compression toggles, embedded-name, dedupe reference tracing. [VERIFIED: file read]
- `TES5Edit/BSArch/wbAssets.pas` — BSArchPro asset/root/extension categories. [VERIFIED: file read]
- `src/formats/bsa/tes4_bsa_parser.cpp` — current reader's accepted layout and metadata interpretation. [VERIFIED: file read]
- `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` — existing legal serializer and manifest proof patterns. [VERIFIED: file read]
- `src/detail/archive_path.cpp`, `src/detail/bethesda_hash.cpp`, `src/detail/compression_router.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp` — reusable implementation helpers. [VERIFIED: file read]
- `/lz4/lz4` Context7 + LZ4 frame manual — LZ4 frame one-shot compression APIs. [CITED: https://github.com/lz4/lz4/blob/dev/doc/lz4frame_manual.html]
- libdeflate official header — raw DEFLATE APIs and warning against golden compressed byte tests. [CITED: https://github.com/ebiggers/libdeflate/blob/master/libdeflate.h]

### Secondary (MEDIUM confidence)
- vcpkg package pages for libdeflate, lz4, and Catch2 versions/metadata. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://vcpkg.io/en/package/catch2.html]
- Catch2 Context7 documentation for `catch_discover_tests(ADD_TAGS_AS_LABELS)`. [CITED: `/catchorg/catch2`]

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — existing project files and vcpkg/package/docs verified current dependencies. [VERIFIED: `vcpkg.json`; `CMakeLists.txt`; vcpkg package pages]
- Architecture: HIGH — phase decisions are locked and current reader/parser/generator provide direct layout evidence. [VERIFIED: `07-CONTEXT.md`; `src/formats/bsa/tes4_bsa_parser.cpp`; `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`]
- Pitfalls: HIGH — each pitfall maps to parser checks, TES5Edit code, or locked decisions. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas`; `src/formats/bsa/tes4_bsa_parser.cpp`; `07-CONTEXT.md`]

**Research date:** 2026-05-08  
**Valid until:** 2026-06-07 for project layout and format behavior; 2026-05-15 for environment/version availability.
