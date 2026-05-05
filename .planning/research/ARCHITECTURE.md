# Architecture Research

**Domain:** Portable C++20 Bethesda BSA/BA2 archive library  
**Researched:** 2026-05-05  
**Confidence:** HIGH for project-specific structure and compatibility seams; MEDIUM for external library integration details until validated in code.

## Recommended Architecture

libbsa should be a layered archive-format library with a small public API, format-specific codecs behind internal boundaries, and compatibility fixtures exercising each seam independently. The important architectural choice is to separate **archive model**, **binary layout parsing/serialization**, **payload transforms**, and **I/O** so BSA/BA2 variants can be added incrementally without coupling TES3, TES4-family, BA2 GNRL, and BA2 DDS behavior into one large archive class.

The BSArchPro reference (`TES5Edit/Core/wbBSArchive.pas`) currently concentrates detection, records, hashing, packing, compression, DDS handling, deduplication, and threading in one class. libbsa should preserve the observed behavior but not the structure: use BSArchPro as a read-only behavioral oracle, then record each compatibility-sensitive rule near the C++ implementation and in tests.

### System Overview

```text
┌──────────────────────────────────────────────────────────────────────┐
│                              Public API                               │
│  ArchiveReader │ ArchiveWriter │ ArchiveInfo │ EntryHandle │ Options  │
└───────────────┬───────────────────────────────┬──────────────────────┘
                │                               │
┌───────────────▼───────────────────────────────▼──────────────────────┐
│                         Format Orchestration                          │
│  DetectionRegistry │ FormatCodec<TES3/TES4/BA2> │ ArchiveModel Mapper │
└───────────────┬───────────────────────────────┬──────────────────────┘
                │                               │
┌───────────────▼───────────────────────────────▼──────────────────────┐
│                         Binary Format Layer                           │
│  BinaryReader/Writer │ Endian/offset helpers │ Layout structs         │
│  Hash algorithms     │ Path normalization    │ Record serialization   │
└───────────────┬───────────────────────────────┬──────────────────────┘
                │                               │
┌───────────────▼───────────────────────────────▼──────────────────────┐
│                        Payload Transform Layer                        │
│  DeflateCodec(libdeflate) │ Lz4FrameCodec │ Lz4BlockCodec │ DdsService │
└───────────────┬───────────────────────────────┬──────────────────────┘
                │                               │
┌───────────────▼───────────────────────────────▼──────────────────────┐
│                              I/O Layer                                │
│  RandomAccessReader │ OutputSink │ FileSource │ MemorySource │ Spans    │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│                                Tests                                  │
│  Unit tests │ Fixture parsers │ Round-trip packs │ BSArchPro compares  │
└──────────────────────────────────────────────────────────────────────┘
```

### Component Boundaries

| Component | Responsibility | Communicates With | Must Not Know About |
|-----------|----------------|-------------------|---------------------|
| Public API | Stable C++20 reader/writer/query surface; value-oriented handles; error-return contract | Format orchestration, archive model | libdeflate, lz4, DirectXTex headers, Pascal/BSArchPro types, concrete file streams |
| `DetectionRegistry` | Detect archive family from magic, version, and BA2 subtype markers | BinaryReader, FormatCodec factory | Compression implementation details, DDS reconstruction |
| `FormatCodec` interface | Internal polymorphic boundary for `read_index`, `extract`, `plan_write`, `write_archive` | Binary format layer, payload transforms, I/O | Public API internals beyond model/options |
| TES3 codec | Morrowind BSA headers, name table, hash table, data-section-relative offsets | Hash service, BinaryReader/Writer, I/O | TES4 folder tree, BA2 file table |
| TES4-family codec | Oblivion/FO3/FNV/Skyrim/SSE BSA folder/file records, flags, hashes, embedded names | Hash service, DeflateCodec, Lz4FrameCodec, I/O | BA2 DDS chunking, DirectXTex |
| BA2 GNRL codec | BTDX+GNRL versions 1/2/3/7/8, file table at `FileTableOffset`, FO4 hashes, per-file compression | DeflateCodec, Lz4BlockCodec, path table parser | DDS header reconstruction |
| BA2 DDS codec | BTDX+DX10 records, texture chunk records, per-chunk extraction and writing | DdsService, DeflateCodec, Lz4BlockCodec, I/O | TES3/TES4 record layouts |
| BinaryReader/BinaryWriter | Checked little-endian primitive reads/writes, bounds/short-read handling, offset seeks | All codecs, I/O layer | Archive policy decisions |
| Archive model | Normalized internal representation of entries, folders, sizes, offsets, hashes, compression state | Public API, codecs, tests | External dependency types |
| Path and hash service | Canonical path splitting, slash normalization, TES3/TES4/FO4 hash algorithms | All format codecs, tests | Stream ownership and compression |
| Compression service | Uniform interface over raw deflate, LZ4 frame, and raw LZ4 block | TES4, BA2 codecs | Archive path lookup, DDS semantics |
| DdsService | DDS metadata analysis for writing and DDS header reconstruction for BA2 DDS extraction | BA2 DDS codec, DirectXTex | Archive detection, non-DDS archive formats |
| I/O abstractions | Random-access reads and streaming output without whole-archive loading | Binary layer, codecs, public API | Format-specific flags and hashes |
| Test fixtures | Byte-level, metadata-level, round-trip, malformed input, BSArchPro comparison outputs | Public/internal test hooks | Mutable TES5Edit source |

## Recommended Project Structure

```text
include/libbsa/
├── archive.hpp              # public reader/writer/query API
├── archive_info.hpp         # archive type/version/flags summary types
├── entry.hpp                # public entry metadata and path handles
├── errors.hpp               # error enum/category and expected aliases
├── io.hpp                   # public source/sink abstractions if exposed
└── options.hpp              # read/write/extract options

src/
├── api/                     # thin public API adapters over internal codecs
├── core/
│   ├── archive_model.*      # normalized model shared by readers/writers
│   ├── detection.*          # magic/version/subtype detection table
│   ├── errors.*             # error mapping and diagnostics
│   └── paths.*              # path normalization, splitting, extension handling
├── io/
│   ├── binary_reader.*      # checked little-endian reads and bounded seeks
│   ├── binary_writer.*      # serialization and offset patching helpers
│   ├── file_source.*        # filesystem-backed random-access input
│   ├── memory_source.*      # test/in-memory random-access input
│   └── sinks.*              # file, memory, callback/streaming output sinks
├── formats/
│   ├── format_codec.hpp     # internal codec interface and common contracts
│   ├── tes3/                # Morrowind BSA parser/writer/hash seams
│   ├── tes4/                # TES4/FO3/SSE BSA parser/writer/hash seams
│   └── ba2/
│       ├── common.*         # BTDX headers, version handling, file table
│       ├── gnrl.*           # BA2 general files
│       └── dds.*            # BA2 texture chunks and DDS reconstruction
├── transforms/
│   ├── compression.hpp      # internal compression interface
│   ├── deflate_libdeflate.* # raw/zlib deflate adapter as validated per format
│   ├── lz4_frame.*          # SSE BSA LZ4 frame path
│   └── lz4_block.*          # Starfield BA2 raw LZ4 block path
├── textures/
│   ├── dds_metadata.*       # DirectXTex-backed metadata extraction
│   └── dds_header.*         # BA2 DDS header reconstruction logic
└── writer/
    ├── archive_builder.*    # high-level write planning
    ├── layout_planner.*     # offsets, tables, sorting, patch positions
    └── dedup.*              # content-hash deduplication, optional

tests/
├── unit/                    # hash, binary reader, compression, DDS header tests
├── fixtures/                # small immutable archives and expected outputs
├── integration/             # parse/extract/write flows per archive family
├── compatibility/           # comparisons against BSArchPro-generated outputs
└── fuzz/                    # optional hardening targets for binary parsers
```

### Structure Rationale

- **Public headers stay minimal.** Consumers should not include `lz4frame.h`, `libdeflate.h`, or DirectXTex headers just to open an archive. This preserves portability and keeps ABI/API churn localized.
- **Format codecs are isolated by archive family.** TES3 offsets, TES4 folder trees, and BA2 file tables have different invariants; forcing them through one record hierarchy would hide compatibility rules and make tests less targeted.
- **Transforms are shared but explicit.** LZ4 frame and LZ4 block are different APIs and must not be selected by a generic “LZ4” flag. Deflate also needs format-specific validation for raw-vs-wrapper behavior.
- **DDS is a texture service, not a BA2 parser concern.** BA2 DDS records describe chunks and metadata; DirectXTex should be isolated behind `DdsService` so future non-Windows portability has one boundary to replace.
- **Writer planning is separate from writer emission.** Archive writing needs sorted indexes, offsets, file table placement, compression decisions, and optional deduplication before bytes are finalized. A layout planner makes this testable without writing full archives.

## Data Flow

### Read and Extract Flow

```text
Caller opens source
    ↓
ArchiveReader::open(source)
    ↓
DetectionRegistry reads magic/version/subtype
    ↓
FormatCodec parses headers and index tables through BinaryReader
    ↓
ArchiveModel stores normalized entries, offsets, sizes, hashes, compression metadata
    ↓
Caller lists entries or requests extraction
    ↓
FormatCodec locates payload offsets and format-specific prefixes/chunks
    ↓
Compression service decompresses only when record flags require it
    ↓
DdsService reconstructs DDS header only for BA2 DX10 extraction
    ↓
OutputSink receives bytes incrementally
```

Direction is intentionally one-way: public API → codec → binary/I/O/transforms. Lower layers return data and errors upward but do not call public API or mutate global state.

### Write Flow

```text
Caller creates ArchiveWriter(target format, options)
    ↓
Add file sources or memory buffers
    ↓
ArchiveBuilder normalizes paths and builds candidate entries
    ↓
FormatCodec validates target-format constraints and computes hashes/flags
    ↓
LayoutPlanner sorts records, chooses table placement, schedules offset patches
    ↓
Payload pipeline analyzes DDS if needed, chunks if needed, compresses if useful/forced
    ↓
BinaryWriter emits placeholder header/index, payload data, trailing BA2 file table if needed
    ↓
BinaryWriter patches offsets/counts/sizes and finalizes stream
    ↓
Round-trip test reopens produced archive and extracts all entries
```

### Compatibility Fixture Flow

```text
Fixture archive or generated file set
    ↓
libbsa parse/extract/write
    ↓
Expected metadata/output bytes loaded from tests/fixtures or BSArchPro-produced baseline
    ↓
Byte-level compare for payloads; metadata-level compare for offsets/flags/hashes/tables
    ↓
Failure reports include format, entry path, record field, offset, and transform used
```

Do not run tests by modifying or compiling `TES5Edit/`. Use the submodule only as read-only reference code and, where needed, compare against outputs produced externally and stored as immutable fixtures.

## Suggested Build Order

The roadmap should build vertical compatibility slices, but each slice depends on shared foundations. Recommended order:

1. **Core scaffolding and binary I/O**
   - `expected`/error model, file/memory sources, sinks, checked `BinaryReader`, `BinaryWriter`, detection skeleton.
   - Enables malformed-input tests before any full format support.

2. **Hash/path services and archive model**
   - Implement TES4/TES3/FO4 hash functions and canonical path splitting with unit tests traced from BSArchPro.
   - Needed by all readers and writers.

3. **TES4-family read slice**
   - Parse `BSA\0` v103/v104/v105 headers, folder/file records, compression flags, embedded names.
   - Add deflate and LZ4 frame adapters behind transform interfaces.
   - This is the best first full slice because PRD milestone 1 targets TES4-family read support and exercises detection, indexes, compression, and extraction without DDS.

4. **TES3 read slice**
   - Reuse binary I/O and hash/path services; add data-section-relative offsets and simpler file tables.
   - Keep separate from TES4 to avoid contaminating offset semantics.

5. **BA2 GNRL read slice**
   - Add BTDX detection, GNRL subtype/version handling, file table parsing at `FileTableOffset`, FO4 hashes, Starfield `CompressionMethod` handling.
   - Add LZ4 block adapter only when v3 path demands it.

6. **BA2 DDS read slice**
   - Add DX10 records, chunk extraction, DDS header reconstruction, per-chunk decompression.
   - Introduce DirectXTex-backed validation only behind `DdsService`.

7. **Writer foundations**
   - Layout planner, record sorting, offset patching, compression policy, dedup abstraction.
   - Test with dry-run plans before byte-emitting writers.

8. **Writers by format family**
   - TES4-family writer first, then BA2 GNRL, BA2 DDS, TES3.
   - Each writer should have round-trip tests plus compatibility tests for ordering, flags, hashes, and target game quirks.

9. **Performance and parallelism**
   - Add `std::jthread`/worker scheduling only after single-threaded compatibility is stable.
   - Keep compression jobs independent and synchronize only on output layout/dedup state.

10. **Hardening and compatibility polish**
    - Malformed archive handling, fuzz targets, edge-case fixtures, known Bethesda quirks, API documentation.

## Architectural Patterns

### Pattern 1: Format Codec Interface with Internal Dispatch

**What:** `ArchiveReader::open` detects the format, then delegates to one internal codec implementing common operations such as parse index, find entry, extract entry, and write archive.

**When to use:** Always for format-specific behavior. Use static helpers or internal classes per family; the public API should not expose inheritance.

**Trade-offs:** A codec boundary adds plumbing, but prevents the BSArchPro-style mega-class from reappearing and makes fixture failures local to one archive family.

```cpp
class FormatCodec {
public:
  virtual ~FormatCodec() = default;
  virtual expected<ArchiveModel, ArchiveError> read_index(RandomAccessReader& input) const = 0;
  virtual expected<void, ArchiveError> extract_entry(
      RandomAccessReader& input,
      const ArchiveEntry& entry,
      OutputSink& output) const = 0;
};
```

### Pattern 2: Checked Binary Reader/Writer, Not Packed Struct Reinterpretation

**What:** Read little-endian primitives and records field-by-field with bounds checks and explicit offsets. Keep packed layout structs internal only as documentation or serialization helpers when safe.

**When to use:** All on-disk parsing and writing.

**Trade-offs:** More verbose than `reinterpret_cast`, but catches truncated archives, avoids alignment/packing portability bugs, and allows precise error diagnostics.

### Pattern 3: Transform Selection by Format Semantics

**What:** Choose `DeflateCodec`, `Lz4FrameCodec`, or `Lz4BlockCodec` from archive family/version/record metadata, not from file extension or a generic compression enum alone.

**When to use:** Extraction, packing, DDS chunks, and Starfield v3 handling.

**Trade-offs:** Slightly more branching in codecs, but prevents silent corruption from LZ4 frame/block confusion. Context7 and official LZ4 docs distinguish frame APIs (`LZ4F_*`) from raw block/streaming APIs (`LZ4_*`), so libbsa should model them as separate adapters.

### Pattern 4: Plan Then Emit for Writers

**What:** Convert input files into a deterministic write plan containing sorted records, compression decisions, dedup references, offsets to patch, and payload jobs. Emit bytes only after the plan is validated.

**When to use:** All archive writing phases.

**Trade-offs:** Requires temporary metadata storage but makes offset math, sorting, deduplication, and compatibility validation testable independent of I/O speed.

### Pattern 5: Compatibility Seams with Golden Tests

**What:** Every behavior traced from BSArchPro gets a named seam and a golden test: hash output, path normalization, compression flag interpretation, embedded-name handling, BA2 file table offsets, DDS headers, chunking, and deduplication.

**When to use:** Any time code differs from obvious binary parsing.

**Trade-offs:** More fixtures up front, much less risk of format regressions later.

## Compatibility-Sensitive Seams to Trace Against BSArchPro

| Seam | Why Sensitive | Reference Starting Point | Test Shape |
|------|---------------|--------------------------|------------|
| Archive detection | Magic/version/subtype decides all downstream parsing | `wbBSArchive.pas` load logic around TES3/BSA/BTDX version branching | Feed minimal headers; assert detected family/version/subtype/errors |
| TES3 offsets | TES3 stores file offsets relative to the data section | `fDataOffset` assignment after TES3 names/hashes | Fixture with known file offset; assert physical seek = data offset + record offset |
| TES3/TES4/FO4 hashes | Lookup, sorting, and compatibility depend on exact hash algorithms | `CreateHashTES3`, `CreateHashTES4`, `CreateHashFO4` | Known path vectors from reference; include slash/case/extension variants |
| TES4 compression bit | File size high bit inverts archive default compression state | `TwbBSFileTES4.Compressed`, `PackData` size flag update | Matrix of archive compressed/uncompressed x file override |
| TES4 embedded names | FO3/SSE embedded file names are stored before payload and can affect engine behavior | `ARCHIVE_EMBEDNAME`, extraction and `PackData` branches | Extract files with/without embedded names; verify payload stripping/writing |
| SSE LZ4 frame | SSE BSA uses LZ4 frame, not raw LZ4 block | `fType = baSSE` sets `ctLZ4Frame` | LZ4 frame fixture; fail gracefully on raw-block mismatch |
| BA2 file table | Names are length-prefixed at `FileTableOffset`, separate from records | `fHeaderFO4.FileTableOffset` seek and `ReadStringLen16` | Fixture with shifted table offset; assert names parse from table, not record order assumptions |
| BA2 GNRL record sentinel/unknowns | `BAADF00D`, unknown fields, and SF header fields may need preservation | BA2 GNRL read/write records | Parse/write metadata-level compare preserving unknown fields where possible |
| Starfield v3 compression | `CompressionMethod == 3` selects raw LZ4 block; other observed values use deflate | `fHeaderSFv3.CompressionMethod` branch | v3 fixtures for deflate and LZ4 block; assert adapter selected |
| BA2 DDS chunk records | Texture payload is per-chunk, not a single file blob | DX10 read loop and `TwbBSTexChunkRec` | Multi-mip fixture; assert chunk ranges, sizes, offsets, decompression |
| DDS reconstruction | Extracted BA2 DDS needs synthesized DDS/DX10 headers | DDS reconstruction logic around DXGI/cubemap fields | Validate header fields and loadability through DirectXTex metadata APIs |
| DDS write chunking | Mipmap chunk grouping controls engine loadability | `GetDDSMipChunkNum`, DDS add-file logic | Pack known DDS; extract; compare metadata and payload layout expectations |
| Deduplication | Shared data changes offsets and record references | `CalcDataHash`, `FindPackedData`, `AddPackedData` | Duplicate input files; assert records point to shared payload only when enabled |
| Compression usefulness threshold | BSArchPro avoids compression unless it saves enough bytes unless forced | `zStream.Size + 32 < aSize` logic | Small payload tests for forced/default compression decisions |
| Multithreaded dedup race | Reference rechecks dedup after compression | `FindPackedData` recheck after compression | Future concurrency tests; keep single-threaded deterministic first |

## Anti-Patterns to Avoid

### Anti-Pattern 1: One Archive Class Owns Everything

**What people do:** Put detection, all record types, extraction, writing, compression, DDS, dedup, and threading in one class.

**Why it is wrong:** This repeats the reference structure instead of creating a reusable library. It makes incremental format support hard to test and hides variant-specific invariants.

**Do this instead:** Keep a narrow public facade and separate internal codecs per archive family.

### Anti-Pattern 2: Generic Compression Flags

**What people do:** Store `compression = lz4` and route all LZ4 through one code path.

**Why it is wrong:** SSE BSA uses LZ4 frame, while Starfield BA2 v3 can use raw LZ4 block. Mixing them can produce valid-looking but corrupt payloads.

**Do this instead:** Use explicit transform adapters: `DeflateCodec`, `Lz4FrameCodec`, `Lz4BlockCodec`, selected by format/version/record metadata.

### Anti-Pattern 3: Whole-Archive Memory Loading

**What people do:** Read the entire archive into memory for convenience.

**Why it is wrong:** Starfield archives can be large, and the PRD requires streaming I/O. It also makes extraction APIs less reusable for asset pipelines.

**Do this instead:** Parse indexes into memory, then seek and stream payloads entry-by-entry into caller-provided sinks.

### Anti-Pattern 4: Public API Leaks Dependencies

**What people do:** Expose DirectXTex, libdeflate, or lz4 types in public headers.

**Why it is wrong:** It makes consumers absorb implementation dependencies and complicates future portability.

**Do this instead:** Hide dependencies in `.cpp` files and internal adapters; expose libbsa-owned value types.

### Anti-Pattern 5: Treating TES5Edit as Vendored Source

**What people do:** Compile or edit Pascal reference code to “reuse” behavior.

**Why it is wrong:** Project constraints explicitly make `TES5Edit/` read-only and non-vendored. It would also leak UI/Delphi design into libbsa.

**Do this instead:** Read and trace reference behavior, then implement clean C++ equivalents outside `TES5Edit/` with compatibility tests.

## Scaling and Performance Considerations

| Concern | Foundation / 100 users | Large archives / 10K tool users | Heavy pipelines / 1M+ operations |
|---------|-------------------------|----------------------------------|----------------------------------|
| Index memory | Store normalized metadata only | Avoid caching payload bytes; support filtered listing | Add memory profiling and compact entry storage if needed |
| Extraction I/O | Seek and stream one file at a time | Batch extraction with bounded buffers | Parallel extraction after compatibility is stable |
| Compression | Single-threaded adapters first | Parallelize independent file/chunk compression | Work-stealing or bounded job queues with deterministic output ordering |
| Writer offsets | Plan all metadata before emit | Patch offsets safely; avoid buffering full archive | Spill large staging data to temp files only if needed |
| DDS processing | Metadata-only validation where possible | Chunk and stream mip payloads | Cache repeated metadata analysis by file identity if profiling justifies it |
| Compatibility fixtures | Small curated archives | Add real-world corpus outside repo if licensing requires | CI split: fast unit fixtures vs optional large compatibility corpus |

## Integration Points

### External Libraries

| Library | Integration Pattern | Architecture Notes | Confidence |
|---------|---------------------|--------------------|------------|
| libdeflate | Internal whole-buffer deflate adapter | Official README states libdeflate is optimized for whole-buffer DEFLATE/zlib/gzip and does not support streaming; archive code should pass bounded file/chunk payloads with known uncompressed sizes. | HIGH |
| lz4 | Separate `lz4_frame` and `lz4_block` adapters | Context7 LZ4 docs show `LZ4F_*` frame streaming APIs separately from `LZ4_*` block/stream APIs; model them separately to avoid SSE/Starfield confusion. | HIGH |
| DirectXTex | Internal `DdsService` for metadata and validation | Context7 DirectXTex docs show `GetMetadataFromDDS*`, `LoadFromDDS*`, `TexMetadata`, mip levels, array size, DXGI format, and cubemap support. Keep this out of public headers. | HIGH |
| vcpkg | Dependency acquisition | Project constraint; use CMake find/package integration but keep dependency details outside API. | HIGH |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Public API ↔ Format codecs | Value types and `expected` errors | No exceptions for normal I/O/format failures; exceptions only for programmer preconditions. |
| Format codecs ↔ Binary I/O | Primitive read/write methods and explicit offsets | No direct `std::ifstream` use in codecs. |
| Format codecs ↔ Compression | Span/buffer input and output with declared raw/compressed sizes | Keep per-format compression selection in codecs, not in compression adapters. |
| BA2 DDS codec ↔ DdsService | DDS metadata/header structs owned by libbsa | DdsService may use DirectXTex internally but returns libbsa-owned types. |
| Writer planner ↔ Binary writer | Deterministic write plan with patch points | Enables dry-run tests for offsets before full archive byte comparisons. |
| Tests ↔ Internals | Friend test hooks or internal test target | Unit-test hash/layout/compression seams without exposing them publicly. |

## Roadmap Implications

- Start with reusable binary I/O, detection, paths, errors, and archive model before any single format. This lowers risk for every subsequent phase.
- Make TES4-family read support the first vertical slice because it exercises detection, hierarchical index parsing, compression, embedded names, and extraction without BA2 DDS complexity.
- Keep TES3 read separate even though it is simpler; its data-section-relative offsets are a different invariant and should have dedicated tests.
- Delay writer phases until read compatibility and transform adapters are stable. Writers multiply compatibility risk because sorting, flags, compression decisions, offsets, and file table placement all interact.
- Delay multithreading until after deterministic single-threaded writers pass round-trip and compatibility tests. Parallelism should optimize independent payload jobs, not change archive layout semantics.

## Sources

- Project context: `.planning/PROJECT.md` (requirements, constraints, supported formats, known quirks).
- Product requirements: `docs/PRD.md` (milestones, API principles, dependencies, testing strategy, risks).
- Project constraints: `AGENTS.md` (TES5Edit read-only boundary, dependencies, documentation, validation expectations).
- BSArchPro reference, read-only: `TES5Edit/Core/wbBSArchive.pas` lines around record definitions, archive loading, compression/packing, BA2 tables, DDS reconstruction.
- LZ4 documentation via Context7: `/lz4/lz4`, frame APIs (`LZ4F_*`) and block/stream APIs (`LZ4_*`) are distinct.
- DirectXTex documentation via Context7: `/microsoft/directxtex`, DDS metadata/load APIs and support for DXGI, mip levels, arrays, cubemaps, and block-compressed DDS formats.
- libdeflate official README: https://github.com/ebiggers/libdeflate (whole-buffer DEFLATE/zlib/gzip API; no streaming support; CMake/MSVC support).

---
*Architecture research for: libbsa reusable C++20 Bethesda archive library*  
*Researched: 2026-05-05*
