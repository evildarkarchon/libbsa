# Architecture Research

**Domain:** Reusable C++20 Bethesda BSA/BA2 archive-format library  
**Researched:** 2026-05-07  
**Confidence:** HIGH for component boundaries and build order; MEDIUM for Starfield edge-case details until fixture validation expands

## Standard Architecture

### System Overview

libbsa should be structured as a small public facade over format-specific readers/writers, shared binary primitives, and dependency adapters. The public API should describe archives, entries, options, streams, and errors in libbsa-owned types only. Compression libraries, DirectXTex, host filesystem details, and TES5Edit/Delphi concepts belong behind internal boundaries.

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              Public API layer                                │
│  archive_reader  archive_writer  archive_entry  archive_metadata  result<T> │
│  input_source    output_sink     read_options   write_options     errors    │
├──────────────────────────────────────┬───────────────────────────────────────┤
│                 Facade / dispatch layer                                      │
│  detect_archive() → format registry → selected reader/writer implementation  │
├──────────────────────────────────────┴───────────────────────────────────────┤
│                         Format implementation layer                          │
│  tes3_bsa   tes4_bsa/v103   fo3_sse_bsa/v104-v105   ba2_gnrl   ba2_dx10     │
│  parsers    serializers     index builders         path/hash rules          │
├──────────────────────────────────────────────────────────────────────────────┤
│                         Shared internal services                             │
│  binary reader/writer  endian helpers  offset/size guards  normalized paths  │
│  hash algorithms       index model     payload planner      diagnostics      │
├───────────────────────────────┬───────────────────────────────┬──────────────┤
│       Compression adapters     │       Texture/DDS adapter      │   I/O core   │
│  deflate_codec (libdeflate)    │  dds_analyzer (DirectXTex)     │  file/stream │
│  lz4_frame_codec (SSE BSA)     │  dds_header_builder            │  bounded buf │
│  lz4_block_codec (SF BA2 v3)   │  mip_chunk_planner             │  sinks       │
└───────────────────────────────┴───────────────────────────────┴──────────────┘
```

### Component Responsibilities

| Component | Responsibility | Ownership / Boundary | Typical Implementation |
|-----------|----------------|----------------------|------------------------|
| Public facade | Open archives, create writers, list/query entries, extract to sinks, finalize new archives | Owns stable user-facing API and ABI policy; no third-party headers | `include/libbsa/archive.hpp`, `reader.hpp`, `writer.hpp`, `types.hpp`, `result.hpp` |
| Format registry / detector | Read magic/version/type fields and choose TES3, TES4-family BSA, BA2 GNRL, or BA2 DX10 implementation | Internal only; selected by bytes, not file extension | Small ordered detection table plus format-specific `probe()` functions |
| Format readers | Parse headers, indexes, names, hashes, offsets, flags, and payload descriptors | Own parsed archive model; borrow I/O source for extraction | `tes3_reader`, `bsa_reader`, `ba2_gnrl_reader`, `ba2_dx10_reader` |
| Format writers | Convert caller inputs into sorted indexes, records, payloads, file tables, and final headers | Own write plan and payload manifest until finalize | Builder/finalizer split: collect entries → plan → stream payloads → emit tables/header |
| Internal archive model | Canonical in-memory representation for entries, folders, payload chunks, compression method, and texture metadata | Internal compatibility model; public metadata is a projection | POD/value structs with explicit integer widths and checked offsets |
| Binary I/O core | Bounded reads/writes, little-endian primitives, exact-size reads, seek validation, overflow checks | Internal; no whole-archive loading as default | `input_source`, `random_access_reader`, `output_sink`, `binary_reader`, `binary_writer` |
| Path normalization | Convert user paths to archive virtual paths; implement case/separator rules per family | Internal rules; public accepts UTF-8/string-like virtual paths | `archive_path` value type internally; avoid `std::filesystem::path` for archive names |
| Hash algorithms | TES3 hash, TES4 folder/file hash, FO4 CRC/hash behavior | Internal but heavily unit-tested because lookup/order compatibility depends on it | Pure functions in `format/*/hash.*` |
| Compression adapters | Exact-size compress/decompress for deflate, LZ4 frame, and raw LZ4 block | Internal; owns library handles/buffers; public sees only compression metadata | `deflate_codec`, `lz4_frame_codec`, `lz4_block_codec` |
| DDS analysis boundary | Parse DDS metadata, reconstruct DDS headers, plan BA2 DX10 mip chunks | Internal; translates DirectXTex data into libbsa-native structs | `texture/dds_analyzer`, `dds_header_builder`, `mip_chunk_planner` |
| Compatibility oracle process | Trace TES5Edit/BSArchPro behavior and encode findings as tests/comments | Reference only; never compile or copy TES5Edit source | Research notes, fixture expected values, focused tests |

## Recommended Project Structure

```
include/libbsa/
├── archive.hpp             # Public reader/writer factories and archive facade
├── reader.hpp              # Public read/query/extract API
├── writer.hpp              # Public archive creation/finalization API
├── types.hpp               # Public enums, metadata, entry descriptors, options
├── result.hpp              # C++20-compatible result/error API
└── version.hpp             # Library version and feature macros

src/
├── public/                 # Thin facade implementations; no format logic
├── io/                     # Random-access input, output sinks, binary reader/writer
├── core/                   # Errors, ranges, checked arithmetic, normalized paths
├── registry/               # Format probing and dispatch
├── compression/            # libdeflate/lz4 wrappers only
├── texture/                # DirectXTex adapter and DDS-native metadata
└── formats/
    ├── tes3/               # Morrowind BSA parser/writer/hash/offset rules
    ├── bsa/                # TES4/FO3/FNV/Skyrim LE/SSE BSA shared implementation
    └── ba2/
        ├── common/         # BTDX headers, versions, file table helpers
        ├── gnrl/           # FO4/SF general BA2 records and payloads
        └── dx10/           # Texture BA2 records, chunk records, DDS reconstruction

tests/
├── unit/                   # Hash, binary I/O, compression, serialization units
├── fixtures/               # Small immutable archives and source file sets
├── integration/            # Open/extract/list tests by format family
├── roundtrip/              # Pack → read → extract → compare source
└── compat/                 # BSArchPro/official-tool comparison expectations
```

### Structure Rationale

- **Public headers stay flat and minimal:** Consumers should not include `lz4.h`, `libdeflate.h`, DirectXTex headers, Windows headers, or format-record internals just to open an archive.
- **Format families own quirks:** TES3 data-section-relative offsets, TES4 embedded-name handling, BSA hash ordering, BA2 file table offsets, and Starfield `CompressionMethod` branches should live with their format implementation rather than in shared generic code.
- **Shared services are boring and testable:** Binary I/O, overflow checks, normalized archive paths, and compression adapters should be independent of archive families so malformed-input hardening can be tested once and reused everywhere.
- **DDS is a boundary, not a public concept:** Public APIs can expose texture metadata as libbsa enums/integers. DirectXTex is an implementation detail used to analyze DDS files and reconstruct headers for BA2 DX10.

## Architectural Patterns

### Pattern 1: Public Facade + Internal Format Strategy

**What:** Public `archive_reader`/`archive_writer` delegates to an internal `archive_impl` chosen by `detect_archive()` or an explicit target format.  
**When to use:** Always for archive open/create operations.  
**Trade-offs:** Adds one indirection, but prevents format-specific details from leaking into the public API and allows new versions to be added by registering a new internal strategy.

```cpp
// Public shape: callers depend on libbsa types only.
libbsa::result<libbsa::archive_reader> archive_reader::open(input_source source,
                                                            read_options options);

// Internal shape: detection selects a concrete implementation.
auto probe = registry.probe(source);
return make_reader_impl(probe.format, std::move(source), options);
```

### Pattern 2: Payload Descriptor Before Payload Bytes

**What:** Parsers produce `payload_descriptor` values containing offset, packed size, unpacked size, compression method, embedded-name expectations, and chunk metadata before any extraction occurs.  
**When to use:** All readers; especially BA2 DX10 where one logical DDS file spans multiple chunks.  
**Trade-offs:** Requires a richer internal model, but makes random access, streaming extraction, validation, and future parallel extraction straightforward.

```cpp
struct payload_descriptor {
  std::uint64_t offset;
  std::uint64_t packed_size;
  std::uint64_t unpacked_size;
  compression_kind compression;
  std::vector<chunk_descriptor> chunks; // Empty for single-payload files.
};
```

### Pattern 3: Dependency Adapter With Exact-Size Contracts

**What:** Wrap third-party compression libraries in small internal adapters that require expected output sizes and return libbsa errors on mismatch.  
**When to use:** Every compression/decompression call.  
**Trade-offs:** Slight wrapper code, but it centralizes allocation, library error mapping, and frame-vs-block separation.

```cpp
result<std::vector<std::byte>> lz4_block_codec::decompress(std::span<const std::byte> packed,
                                                           std::uint32_t expected_size);
```

### Pattern 4: Read-First, Write-From-Parsed-Model

**What:** Build readers and parsed internal models before writers. Writers should reuse serialization, hash, ordering, compression, and DDS planning tests derived from reader fixtures.  
**When to use:** Roadmap/phase ordering.  
**Trade-offs:** Delays archive creation features, but sharply reduces compatibility risk because write output can be re-opened by libbsa and compared against fixture behavior.

### Pattern 5: Reference Behavior as Tests, Not Source

**What:** Trace TES5Edit/BSArchPro behavior for compatibility constraints, then encode the discovered behavior as test fixtures, expected hashes/orderings, and comments explaining non-obvious WHY.  
**When to use:** Any behavior that is undocumented or surprising: sorting, embedded names, flags, offset bases, compression method branches, DDS chunk layout.  
**Trade-offs:** Requires careful documentation, but preserves the hard boundary: no TES5Edit edits, vendoring, formatting, or compiled dependency.

## Public vs Internal API Separation

| Keep Public | Keep Internal |
|-------------|---------------|
| `archive_reader`, `archive_writer`, `archive_entry`, `archive_metadata` | Raw on-disk header/record structs |
| `input_source` / `output_sink` abstractions or factory helpers | Concrete file handles, memory mapping details, buffering strategy |
| `archive_format`, `compression_kind`, `read_options`, `write_options` | Format probe table, version-specific parser classes |
| `result<T>`, `error`, `error_code`, diagnostic strings | Third-party library status codes and handles |
| UTF-8 archive virtual paths and entry IDs | Host `std::filesystem::path` normalization rules for archive-internal paths |
| Texture metadata as libbsa values where useful | DirectXTex types, DXGI helper calls, DDS scratch images |
| Stable metadata: sizes, flags, hashes, offsets if intentionally exposed | Mutable parse model, sort keys, file table builders, dedup maps |

Public APIs should avoid promising byte-for-byte writer layout knobs until compatibility behavior is proven. Expose high-level intent first: target format, compression policy, entry path, source bytes/stream, and finalize. Add advanced controls only when tests prove they are necessary for real tools.

## Data Flow

### Read / Extract Flow

```
Caller opens source
    ↓
archive_reader::open(source)
    ↓
Format detector reads magic/version/type
    ↓
Format parser reads header/index/name tables into internal archive model
    ↓
Caller lists entries or requests extract(path/hash)
    ↓
Path normalizer + hash/index lookup selects payload descriptor
    ↓
I/O core reads bounded payload bytes/chunks
    ↓
Compression adapter decompresses when descriptor requires it
    ↓
BA2 DX10 only: DDS header builder combines metadata + chunks
    ↓
Bytes stream to caller-provided output sink
```

Direction is intentionally one-way: public request → internal lookup/model → bounded I/O → adapter transform → caller sink. Format parsers should not call public APIs, and compression/DDS adapters should not know about caller objects.

### Write / Pack Flow

```
Caller creates archive_writer(target_format)
    ↓
Caller adds entries from memory or host files with virtual archive paths
    ↓
Path normalizer + format rules compute flags, hashes, and validation diagnostics
    ↓
For BA2 DX10: DDS analyzer creates libbsa-native texture layout and mip chunks
    ↓
Planner sorts indexes, chooses compression methods, detects duplicates, reserves offsets
    ↓
Payload writer streams source data through compression adapters into output sink
    ↓
Serializer emits format-specific records, file tables, and final header values
    ↓
Post-write validation re-opens output where practical for round-trip tests
```

Direction is collect → plan → stream payloads → serialize metadata. Avoid in-place mutation in early phases because archive tables, offsets, compressed sizes, and deduplication make safe update semantics substantially harder than write-new finalization.

### BA2 DDS Extraction Flow

```
BA2 DX10 records + chunk records
    ↓
texture_layout (internal libbsa struct, not DirectXTex)
    ↓
Read/decompress chunks in record order
    ↓
DDS header reconstruction from stored metadata
    ↓
Header + mip payloads stream to output sink
```

### BA2 DDS Creation Flow

```
Input DDS bytes/file
    ↓
DirectXTex adapter parses metadata internally
    ↓
Translate to libbsa texture_layout
    ↓
Mip chunk planner creates BA2 DX10 chunk descriptors
    ↓
Compress chunks according to target BA2 version/method
    ↓
Serialize DX10 records and payload chunks
```

## Suggested Build Order and Dependencies

| Order | Build Slice | Depends On | Why This Reduces Compatibility Risk |
|-------|-------------|------------|-------------------------------------|
| 1 | CMake/vcpkg/Catch2 skeleton, public header shell, `result<T>`, error model | None | Locks down C++20/public dependency boundary before format code grows. |
| 2 | Binary I/O core, checked arithmetic, endian helpers, bounded buffers | 1 | Every parser/writer needs safe offset and size handling; early hardening prevents repeated bugs. |
| 3 | Path normalization and hash units for TES3/TES4/FO4 | 1-2 | Lookup and writer ordering compatibility depend on hashes; test them independently before parsing. |
| 4 | Compression adapters: deflate, LZ4 frame, LZ4 block | 1-2 | Separating frame vs raw block up front avoids SSE/Starfield corruption classes. |
| 5 | Format detector and TES4-family BSA read path | 1-4 | Covers multiple high-value formats and validates deflate/LZ4 frame plus embedded-name behavior. |
| 6 | TES3 read path | 1-3 | Adds offset-base variation without compression complexity. |
| 7 | BA2 GNRL read path | 1-4 | Adds BTDX records, file table parsing, FO4/SF versions, and raw LZ4 block branch. |
| 8 | DDS analysis boundary and BA2 DX10 read path | 1-4, 7 | Texture archives are more complex; build after common BA2 and compression are proven. |
| 9 | TES4-family write support | 1-5 | First writer should target the best-understood read model so round-trip can validate immediately. |
| 10 | BA2 GNRL write support | 1-7, 9 | Reuses BA2 read validation and compression adapters before adding DDS chunk complexity. |
| 11 | BA2 DX10 write support | 1-8, 10 | DDS chunk planning needs the texture boundary and BA2 serializer to be stable. |
| 12 | TES3 write support | 1-3, 6, 9 | Simpler format but unique offset/sort behavior; implement once writer framework is proven. |
| 13 | Performance/multi-threading | All correctness slices | Parallelism should only optimize proven single-threaded behavior to avoid nondeterministic compatibility bugs. |
| 14 | Hardening/fuzzing/docs/examples | All core features | Malformed input and API docs are most useful once behavior is stable. |

Recommended roadmap implication: keep the PRD's read-first family sequencing, but ensure the foundation explicitly includes binary I/O, path/hash tests, and compression adapters before the first archive parser. This makes later writer phases less risky because writers can reuse proven readers for round-trip validation.

## Compatibility Constraints From TES5Edit Reference Behavior

These should be treated as compatibility requirements to verify against TES5Edit/BSArchPro and official tool output, not as permission to copy source.

| Constraint | Architectural Placement | Validation Strategy |
|------------|-------------------------|---------------------|
| TES5Edit is read-only reference material | Process boundary; never under `src/`, never modified | Git status must not show submodule edits; tests store independent expected values/fixtures. |
| Hash algorithms and sorted index order drive lookup and writer compatibility | `formats/*/hash.*` and index builders | Unit tests for known path/hash pairs; fixture parse order tests; writer table order round-trips. |
| TES3 offsets are data-section-relative | `formats/tes3` only | Fixture extraction with non-zero data section base and offset overflow tests. |
| TES4/FO3/SSE BSA embedded names affect payload offsets and extraction bytes | `formats/bsa` payload descriptor and extractor | Fixtures with/without embedded names; exact extracted bytes compared to reference output. |
| SSE BSA compressed payloads use LZ4 frame handling, not raw block handling | `compression/lz4_frame_codec` selected by BSA v105 rules | Unit tests reject routing through raw block codec; extraction fixtures. |
| Starfield BA2 v3 `CompressionMethod == 3` uses raw LZ4 block; other observed methods retain deflate behavior | `formats/ba2` compression selector plus `lz4_block_codec` | Version/method matrix tests with compressed-size/raw-size checks. |
| BA2 GNRL file names live in a length-prefixed table at `FileTableOffset` | `formats/ba2/common` file table parser | Fixtures with table offset validation, truncation tests, duplicate/empty path diagnostics. |
| BA2 DX10 logical files are assembled from texture chunk records and reconstructed DDS headers | `formats/ba2/dx10` + `texture` boundary | Extracted DDS must load and match expected metadata/source bytes where possible. |
| Known Bethesda quirks may require warnings rather than hard failures | Error/diagnostic model | Use structured warnings for compatibility hazards; do not hide them in logs. |

## Scaling and Performance Considerations

| Concern | Baseline Architecture | Later Optimization |
|---------|----------------------|--------------------|
| Large Starfield archives | Random-access source plus bounded scratch buffers; no whole-archive reads | Optional memory-mapped source adapter, readahead, larger buffer tuning |
| Bulk extraction | Single-threaded deterministic flow using payload descriptors | Parallel decompression/extraction by independent descriptors after correctness is proven |
| Packing large inputs | Stream input files through compression into output sink | Parallel compression jobs writing into planned payload slots or staged temp chunks |
| DDS archives | Chunk descriptors allow per-chunk processing | Parallel chunk compression/decompression once chunk order and final serialization are stable |
| Public ABI/API stability | Pimpl/internal impl pointers where needed; libbsa-owned types | Add advanced knobs conservatively after compatibility tests justify them |

## Anti-Patterns

### Anti-Pattern 1: Leaking Dependency Types Into Public Headers

**What people do:** Public methods return DirectXTex metadata, accept `libdeflate`/`LZ4F` handles, or expose platform handles.  
**Why it's wrong:** Consumers inherit implementation dependencies, portability worsens, and changing adapters becomes a breaking API change.  
**Do this instead:** Translate all dependency data into libbsa-owned values at internal boundaries.

### Anti-Pattern 2: Generic Archive Abstraction Too Early

**What people do:** Build a ZIP-like generic archive framework and force BSA/BA2 variants into it.  
**Why it's wrong:** Bethesda formats depend on variant-specific hashes, sorting, offsets, flags, file tables, compression modes, and DDS chunk records. Generic abstractions hide compatibility rules.  
**Do this instead:** Use a shared facade and internal services, but keep format-specific parser/writer modules explicit.

### Anti-Pattern 3: Whole-Archive Memory Loading

**What people do:** Read the full archive into memory, then parse/extract from spans.  
**Why it's wrong:** Large BA2/Starfield archives can be many gigabytes; memory loading prevents streaming and later parallel I/O.  
**Do this instead:** Parse only metadata eagerly; read payload ranges on demand into bounded buffers or stream them to sinks.

### Anti-Pattern 4: Inferring Compression From Extension or Game Name

**What people do:** Use `.bsa`, `.ba2`, or a caller-supplied game enum as the compression selector.  
**Why it's wrong:** Version and per-record metadata matter; Starfield BA2 v3 has explicit `CompressionMethod`, and BA2 records use packed-size semantics.  
**Do this instead:** Route compression from parsed archive format/version/record fields and target writer options.

### Anti-Pattern 5: Implementing Writers Before Readers Are Trusted

**What people do:** Add archive creation APIs before extraction and parse models are validated.  
**Why it's wrong:** Writers need hash ordering, offsets, compression, file tables, flags, and DDS chunking; without a trusted reader, failures are hard to diagnose.  
**Do this instead:** Build read support and fixture tests first, then require every writer to pass re-open/extract/compare round-trip tests.

## Integration Points

### External Libraries

| Library | Integration Pattern | Notes |
|---------|---------------------|-------|
| libdeflate | Internal `deflate_codec` with exact-size compression/decompression methods | Use for TES4/FO3/Skyrim LE BSA and FO4/SF BA2 deflate payloads. Map library failures into libbsa errors. |
| official lz4 | Two internal adapters: `lz4_frame_codec` and `lz4_block_codec` | Never share routing between SSE frame payloads and Starfield raw block payloads. |
| DirectXTex | Internal `dds_analyzer` that emits libbsa `texture_layout` | Do not expose DirectXTex/DXGI types in public headers; keep non-Windows portability path possible. |
| Catch2/CTest | Test-only dependency | Tests should prove hash values, parser behavior, extraction bytes, writer round-trips, and compatibility quirks. |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Public facade ↔ format impl | Type-erased/pimpl internal calls returning `result<T>` | Public API remains stable while implementation modules evolve. |
| Format parser ↔ binary I/O | Checked read/seek methods | Parser never performs unchecked pointer arithmetic on archive bytes. |
| Format impl ↔ compression | `compression_kind` + byte spans + expected sizes | Keeps third-party library status and handles internal. |
| BA2 DX10 ↔ texture adapter | `texture_layout` and `chunk_descriptor` values | DirectXTex only appears inside `src/texture`. |
| Writer planner ↔ serializer | Immutable write plan | Reduces offset bugs by separating decision-making from byte emission. |
| Reference research ↔ implementation | Tests, comments, documented constraints | TES5Edit behavior informs results, not source ownership. |

## Sources

- Project context: `J:\libbsa-gsd\.planning\PROJECT.md` (read 2026-05-07).
- Source PRD: `J:\libbsa-gsd\docs\PRD.md` (read 2026-05-07).
- Project constraints: `J:\libbsa-gsd\AGENTS.md` (read 2026-05-07).
- Existing stack research embedded in project context: C++20, CMake/vcpkg, libdeflate, lz4, DirectXTex, Catch2, and dependency-boundary decisions.
- Behavioral reference locations identified by project docs: `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, `TES5Edit/Core/wbBSA.pas` (reference only; not edited or compiled).

---
*Architecture research for: libbsa reusable C++20 BSA/BA2 archive library*  
*Researched: 2026-05-07*
