# Phase 06: ba2-gnrl-read-and-extract - Research

**Researched:** 2026-05-05  
**Domain:** C++20 BA2 GNRL archive parsing, metadata lookup, and payload extraction  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

### BA2 API Shape
- **D-01:** Add a separate public BA2 header and type surface mirroring the BSA reader API: `include/libbsa/ba2.hpp`, `ba2_archive`, `open_ba2`, and `extract_ba2_entry`. Do not put BA2 APIs into `bsa.hpp` and do not introduce a family-neutral archive API in Phase 6.
- **D-02:** Keep `ba2_archive` parallel to `bsa_archive`: expose `summary()`, `paths()`, `contains()`, and `entry()` and rely on `archive_summary` and `entry_metadata` for BA2-specific data.
- **D-03:** Public smoke coverage must include `ba2.hpp`, name `ba2_archive`, and take addresses of `open_ba2` and `extract_ba2_entry` from consumer-style code.
- **D-04:** `ba2_archive` is metadata-only. It owns copied metadata and does not retain or own a `byte_source`; callers pass a `byte_source` again to extraction, matching the existing BSA lifetime model.

### Metadata Exposure
- **D-05:** Use existing `archive_summary` fields for BA2 archive-level metadata: format, version, subtype, file count, file table offset, and Starfield v3 compression method. Do not add public fields for unknown or reserved header words unless research proves they are semantically required.
- **D-06:** Map BA2 hash components into existing `entry_metadata::name_hash` and `entry_metadata::directory_hash` where representable. The exact mapping must be locked with tests or comments near the parser so downstream agents can trace compatibility behavior.
- **D-07:** Preserve all three public size semantics for BA2 entries: `size` is unpacked output bytes, `packed_size` is compressed payload bytes or raw payload size, and `stored_size` is the on-disk byte range extraction validates and reads.
- **D-08:** Expose `entry_metadata::offset` as an archive-absolute payload offset after validation, continuing the Phase 5 TES3 pattern. Do not expose BA2 record-relative offsets directly to consumers.

### Compression Semantics
- **D-09:** Populate `entry_metadata::compression` with the resolved per-entry state (`raw`, `deflate`, or `lz4_block`) so metadata inspection matches extraction behavior.
- **D-10:** For Starfield v3 GNRL, route to raw LZ4-block extraction only when the entry is compressed and archive `compression_method == 3`. Raw entries remain raw even in method-3 archives.
- **D-11:** BA2 record size fields drive decompression framing. Do not require or auto-detect a BSA-style embedded uncompressed-size prefix for BA2 compressed payloads.
- **D-12:** Unsupported or inconsistent BA2 compression metadata must fail with structured `unsupported_format` or `malformed_archive` errors. Do not try fallback codecs and do not write partial bytes.

### Fixture Proof Shape
- **D-13:** Use generated deterministic BA2 fixture builders as the primary Phase 6 acceptance corpus. Do not depend on external real archives, BSArchPro execution, or committed binary BA2 blobs unless implementation discovers a concrete need.
- **D-14:** The generated fixture matrix must cover FO4 GNRL v1/v7/v8, Starfield GNRL v2, Starfield GNRL v3 raw/deflate/LZ4-block routes, file-table name parsing, and a `.dds`-named payload stored in GNRL as ordinary bytes.
- **D-15:** Keep fixture builders test-local first, likely in `tests/ba2_reader_tests.cpp` or a small test helper. Promote to shared helpers later only if writer phases reuse them.
- **D-16:** Explicit malformed coverage in Phase 6 must include core parser failures: truncated headers/records, impossible payload offsets, truncated name tables, mismatched file count/name count, and codec route confusion. Comprehensive fuzz-style hardening remains Phase 11 scope.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may choose exact helper names, parser file names, and test organization details as long as the decisions above, `06-SPEC.md`, and existing project patterns are satisfied.

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| BA2-01 | Consumer can open, list, inspect, and extract Fallout 4 BA2 GNRL archives across supported v1/v7/v8 variants. [VERIFIED: .planning/REQUIREMENTS.md] | Use BA2 `BTDX` + `GNRL` header parsing, 36-byte GNRL records, file-table names, `PackedSize != 0` compression detection, and existing deflate dispatcher. [VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: src/compression.cpp] |
| BA2-02 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v2 archives with the additional header fields preserved in metadata where relevant. [VERIFIED: .planning/REQUIREMENTS.md] | Starfield v2 adds two 32-bit words after the FO4 header; current public `archive_summary` has no dedicated unknown fields, and D-05 says not to add fields unless semantically required. [VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: .planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md] |
| BA2-03 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v3 archives with correct compression-method routing. [VERIFIED: .planning/REQUIREMENTS.md] | Starfield v3 adds `CompressionMethod`; existing routing maps Starfield BA2 `compression_method == 3` to `lz4_block` and otherwise supports deflate. [VERIFIED: src/detect.cpp; VERIFIED: src/compression.cpp] |
| BA2-04 | Consumer can parse BA2 file name tables located at `FileTableOffset` and associate length-prefixed names with entries. [VERIFIED: .planning/REQUIREMENTS.md] | TES5Edit seeks to `FileTableOffset` and reads one length-16 string per file; external BA2 format references describe the name table as uint16 length plus bytes. [VERIFIED: TES5Edit/Core/wbBSArchive.pas; CITED: https://miere.ru/posts/ba2-archive-format/] |
</phase_requirements>

## Summary

Phase 06 should implement a BA2-specific reader, not a generic archive abstraction. [VERIFIED: .planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md] The implementation can reuse the existing metadata-only `archive_view`, `archive_summary`, `entry_metadata`, `byte_source`, `byte_sink`, compression dispatcher, FO4 hash helpers, and BSA reader patterns. [VERIFIED: include/libbsa/archive_view.hpp; VERIFIED: include/libbsa/archive.hpp; VERIFIED: include/libbsa/io.hpp; VERIFIED: include/libbsa/compression.hpp; VERIFIED: src/hash.hpp; VERIFIED: src/bsa_reader.cpp]

The BA2 GNRL binary model is compact: `BTDX` magic and version precede a FO4-style header containing subtype, file count, and `FileTableOffset`; Starfield v2 appends two unknown 32-bit header words; Starfield v3 appends two unknown 32-bit words plus `CompressionMethod`; each GNRL entry record is 36 bytes and stores name hash, extension magic, directory hash, unknown word, absolute payload offset, packed size, unpacked size, and a tail word observed as `BAADF00D`. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 287-322 and 1136-1159; CITED: https://miere.ru/posts/ba2-archive-format/]

**Primary recommendation:** Add `include/libbsa/ba2.hpp`, `src/ba2_reader.cpp`, and `tests/ba2_reader_tests.cpp`; parse BA2 GNRL records into existing metadata semantics; extract through `resolve_payload_codec` + `decompress_payload`; and wire tests/CMake exactly like the BSA reader path. [VERIFIED: .planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md; VERIFIED: CMakeLists.txt; VERIFIED: src/bsa_reader.cpp]

## Project Constraints (from AGENTS.md)

- Implementation language is C++ and public APIs should be clear, portable C++ rather than Delphi/Pascal transliteration. [VERIFIED: AGENTS.md]
- `TES5Edit/` is read-only reference material: do not edit, format, stage, compile, vendor, or update the submodule. [VERIFIED: AGENTS.md]
- Preserve archive-format behavior discovered from BSArchPro/TES5Edit unless a documented reason exists to diverge. [VERIFIED: AGENTS.md]
- Use `libdeflate` for deflate and official `lz4` for LZ4; do not add speculative external dependencies. [VERIFIED: AGENTS.md]
- Public headers must not expose libdeflate, LZ4, DirectXTex, platform, Delphi, UI, or TES5Edit types. [VERIFIED: AGENTS.md; VERIFIED: include/libbsa/compression.hpp]
- Add comments for non-obvious compatibility constraints and Doxygen-compliant comments for public APIs and added/substantially rewritten methods. [VERIFIED: AGENTS.md]
- Add focused fixture-based tests for archive parsing, extraction, round-tripping, and compatibility behavior as surfaces are implemented. [VERIFIED: AGENTS.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| BA2 public read/extract API | C++ library public API | Tests | Consumers need `ba2.hpp`, `ba2_archive`, `open_ba2`, and `extract_ba2_entry` without private dependencies. [VERIFIED: 06-CONTEXT.md; VERIFIED: include/libbsa/bsa.hpp] |
| BA2 header and GNRL record parsing | C++ library parser implementation | Read-only TES5Edit reference | Binary layout and metadata validation belong in `src/`, with TES5Edit used only for compatibility tracing. [VERIFIED: AGENTS.md; VERIFIED: TES5Edit/Core/wbBSArchive.pas] |
| Path listing and lookup | Metadata view layer | Parser | Existing `archive_view` already owns copied metadata and normalized lookup keys. [VERIFIED: include/libbsa/archive_view.hpp; VERIFIED: .planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md] |
| Payload extraction | C++ library extraction implementation | Compression adapters | Extraction reads a validated byte range from caller-owned `byte_source`, resolves a codec, and writes to caller-owned `byte_sink`. [VERIFIED: include/libbsa/io.hpp; VERIFIED: include/libbsa/compression.hpp; VERIFIED: src/bsa_reader.cpp] |
| Compression routing | Compression dispatcher | BA2 parser metadata | BA2 parser supplies `entry_metadata::compression` and `summary().compression_method`; dispatcher prevents deflate/LZ4-frame/LZ4-block confusion. [VERIFIED: src/compression.cpp; VERIFIED: 06-CONTEXT.md] |
| Fixture validation | Catch2/CTest tests | CMake wiring | Generated fixtures are locked as the Phase 6 acceptance corpus and must be wired explicitly. [VERIFIED: 06-CONTEXT.md; VERIFIED: CMakeLists.txt] |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++ | C++20 | Public API and parser implementation | Project constraint and existing build target use `cxx_std_20`. [VERIFIED: AGENTS.md; VERIFIED: CMakeLists.txt] |
| CMake | Minimum 3.24; local tool 4.3.2 available | Source/header/test wiring | Existing project uses explicit `target_sources`, file sets, and CTest. [VERIFIED: CMakeLists.txt; VERIFIED: cmake --version] |
| vcpkg manifest | `vcpkg.json` baseline `12dcccadfe573d0eaa6c67a968413ded7805d256` | Dependency acquisition | Existing manifest lists libdeflate, lz4, DirectXTex, and Catch2. [VERIFIED: vcpkg.json] |
| libdeflate | vcpkg `1.25#0`, package last updated 2025-11-03 | BA2 deflate payload decompression/compression in fixtures | vcpkg package provides `compression` and `decompression` features; libdeflate is whole-buffer DEFLATE-focused, matching archive record payloads with known unpacked sizes. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://github.com/ebiggers/libdeflate/blob/master/README.md] |
| lz4 | vcpkg `1.10.0#0`, package last updated 2024-07-25 | Starfield v3 raw LZ4 block payload route | Official `lz4.h` states block APIs are different from frame APIs and require external compressed/uncompressed size metadata. [CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Catch2 | vcpkg `3.14.0#0`, package last updated 2026-04-06 | BA2 reader fixture/unit tests | Use for generated BA2 fixtures, malformed-input tests, and public-header smoke coverage. [CITED: https://vcpkg.io/en/package/catch2.html; VERIFIED: tests/bsa_reader_tests.cpp] |
| CTest | Bundled with CMake; local `ctest` 4.3.2 | Test orchestration | Use existing labels `unit`, `fixture`, `codec`, and `smoke`. [VERIFIED: ctest --version; VERIFIED: CMakeLists.txt] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Existing `archive_view` | New BA2-only lookup map | D-02 locks reuse of existing summary/metadata semantics, and `archive_view` already handles normalized deterministic lookup. [VERIFIED: 06-CONTEXT.md; VERIFIED: include/libbsa/archive_view.hpp] |
| Existing compression dispatcher | Direct libdeflate/LZ4 calls in BA2 reader | Direct calls would duplicate route policy and risk LZ4 frame/block confusion that Phase 03 already isolated. [VERIFIED: src/compression.cpp; VERIFIED: README.md] |
| Generated fixtures | Real committed BA2 blobs | D-13 locks generated deterministic fixtures as the primary Phase 6 corpus. [VERIFIED: 06-CONTEXT.md] |

**Installation:** no new dependency installation is required beyond the existing `vcpkg.json`; add only source/header/test files to CMake. [VERIFIED: vcpkg.json; VERIFIED: CMakeLists.txt]

**Version verification:** vcpkg package pages verified libdeflate `1.25#0`, lz4 `1.10.0#0`, and Catch2 `3.14.0#0`; local `vcpkg` CLI is not on PATH, so dependency resolution depends on `VCPKG_ROOT` during configure. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://vcpkg.io/en/package/catch2.html; VERIFIED: local environment probe]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer byte_source
  -> open_ba2(source)
      -> bounded BA2 header read: magic/version/subtype/file_count/FileTableOffset[/CompressionMethod]
      -> validate subtype == GNRL and supported version
      -> read file_count * 36-byte GNRL records
      -> seek FileTableOffset and read file_count length-prefixed names
      -> normalize paths + populate copied entry_metadata
      -> ba2_archive(archive_summary, entries) using archive_view

Consumer path + byte_source + byte_sink
  -> extract_ba2_entry(archive, source, path, sink)
      -> archive_view lookup by normalized path
      -> validate metadata.offset + metadata.stored_size within source.size()
      -> read payload bytes
      -> resolve_payload_codec(format, entry.compression, summary.compression_method)
         -> raw: no codec
         -> deflate: libdeflate wrapper
         -> Starfield v3 method 3 compressed: LZ4 raw block wrapper
      -> write complete output bytes to caller-owned sink
```

All steps above are derived from existing BSA reader architecture and BA2 context decisions. [VERIFIED: src/bsa_reader.cpp; VERIFIED: include/libbsa/archive_view.hpp; VERIFIED: 06-CONTEXT.md]

### Recommended Project Structure

```text
include/libbsa/
├── ba2.hpp                 # public BA2 archive API mirroring bsa.hpp
src/
├── ba2_reader.cpp          # BA2 GNRL parser/extractor implementation
tests/
├── ba2_reader_tests.cpp    # generated BA2 fixture builders and malformed cases
└── public_header_smoke.cpp # extended with ba2.hpp symbols
```

This structure follows the existing public/private layout and explicit CMake source-list pattern. [VERIFIED: AGENTS.md; VERIFIED: CMakeLists.txt]

### Pattern 1: Metadata-only archive wrapper
**What:** `ba2_archive` should be a thin wrapper over `archive_view` with Doxygen comments mirroring `bsa_archive`. [VERIFIED: include/libbsa/bsa.hpp; VERIFIED: include/libbsa/archive_view.hpp]  
**When to use:** Always for Phase 6 public BA2 metadata, because D-04 locks caller-owned source lifetimes. [VERIFIED: 06-CONTEXT.md]  
**Example:**
```cpp
/// Owns parsed metadata for a BA2-family archive.
///
/// The archive object is a metadata view only: payload bytes remain owned by the
/// caller-provided `byte_source` passed to `open_ba2` and extraction calls.
class ba2_archive {
public:
    ba2_archive(archive_summary summary, std::vector<entry_metadata> entries);
    [[nodiscard]] const archive_summary& summary() const noexcept;
    [[nodiscard]] std::vector<archive_path> paths() const;
    [[nodiscard]] bool contains(std::string path) const;
    [[nodiscard]] result<entry_metadata> entry(std::string path) const;
private:
    archive_view view_;
};
```
Source: existing `bsa_archive` pattern. [VERIFIED: include/libbsa/bsa.hpp]

### Pattern 2: BA2 GNRL entry parse semantics
**What:** For each GNRL record, parse `NameHash`, `Ext`, `DirHash`, `Unknown`, `Offset`, `PackedSize`, `Size`, and tail word; then attach the matching name-table path by record index. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 1147-1159 and 1195-1198]  
**When to use:** For FO4 v1/v7/v8 and Starfield v2/v3 GNRL only. [VERIFIED: src/detect.cpp; VERIFIED: 06-SPEC.md]  
**Example:**
```cpp
entry_metadata metadata{};
metadata.path = names[i];
metadata.name_hash = record.name_hash;
metadata.directory_hash = record.dir_hash;
metadata.offset = record.offset;          // BA2 records store archive-absolute payload offsets.
metadata.size = record.size;              // Unpacked output bytes.
metadata.packed_size = record.packed_size == 0 ? record.size : record.packed_size;
metadata.stored_size = metadata.packed_size;
metadata.compression = record.packed_size == 0 ? compression_state::raw
                     : (summary.compression_method == 3 ? compression_state::lz4_block
                                                        : compression_state::deflate);
```
Source: TES5Edit compressed predicate and extraction behavior. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 837-843 and 2157-2174]

### Pattern 3: No BSA embedded-size prefix for BA2
**What:** For BA2 compressed entries, use the record's `PackedSize` as compressed input length and `Size` as exact decompressed output length. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 2157-2167]  
**When to use:** Every BA2 GNRL compressed extraction route. [VERIFIED: 06-CONTEXT.md D-11]  
**Example:**
```cpp
auto algorithm = resolve_payload_codec({archive.summary().format,
                                        metadata.compression,
                                        archive.summary().compression_method});
auto unpacked = decompress_payload(algorithm.value(), packed_bytes, metadata.size);
```
Source: existing compression dispatcher contract. [VERIFIED: include/libbsa/compression.hpp]

### Anti-Patterns to Avoid
- **Adding BA2 APIs to `bsa.hpp`:** Contradicts D-01 and makes API family boundaries ambiguous. [VERIFIED: 06-CONTEXT.md]
- **Inferring BA2 codec from file extension:** Compression must come from `PackedSize`, archive format, and Starfield v3 `CompressionMethod`. [VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: src/compression.cpp]
- **Using LZ4 frame API for Starfield BA2 v3 raw blocks:** Official LZ4 docs state block and frame formats are different; existing dispatcher separates `lz4_block` from `lz4_frame`. [CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h; VERIFIED: src/compression.cpp]
- **Rejecting `.dds` files in GNRL:** SPEC and context explicitly include `.dds`-named payloads as ordinary GNRL bytes. [VERIFIED: 06-SPEC.md; VERIFIED: 06-CONTEXT.md]
- **Reading entire archives for metadata or extraction:** Project constraints require bounded random-access reads and streaming sink writes. [VERIFIED: AGENTS.md; VERIFIED: include/libbsa/io.hpp]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Deflate codec | Custom inflater or zlib fallback guessing | Existing `decompress_payload(deflate)` backed by libdeflate | libdeflate is already required and validates exact expected size through the project wrapper. [VERIFIED: src/compression.cpp; CITED: https://github.com/ebiggers/libdeflate/blob/master/README.md] |
| Starfield LZ4 block codec | LZ4 frame route or custom block decoder | Existing `decompress_payload(lz4_block)` | Official LZ4 block docs require external size metadata, which BA2 records provide. [CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h; VERIFIED: TES5Edit/Core/wbBSArchive.pas] |
| Path normalization | `std::filesystem::path` or host path normalization | `normalize_archive_path` and `archive_view` | Archive paths are virtual and existing code rejects absolute/traversal paths. [VERIFIED: include/libbsa/archive_path.hpp; VERIFIED: tests/path_hash_tests.cpp] |
| Lookup container | BA2-specific duplicate path map | `archive_view` | Existing view owns copied entries and deterministic sorted paths. [VERIFIED: include/libbsa/archive_view.hpp] |
| Test runner | New test framework | Catch2 + CTest | Existing project and CMake already use Catch2 and `catch_discover_tests`. [VERIFIED: CMakeLists.txt; CITED: /catchorg/catch2] |

**Key insight:** BA2 GNRL is not complex enough to justify new architecture; the risky parts are exact binary offsets, name-table indexing, and compression route metadata, all of which should be isolated behind existing parser, view, and codec boundaries. [VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: src/bsa_reader.cpp; VERIFIED: src/compression.cpp]

## Common Pitfalls

### Pitfall 1: Treating `PackedSize == 0` as an empty file
**What goes wrong:** Raw BA2 entries have `PackedSize == 0`, but their payload length is `Size`. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 2157-2174]  
**Why it happens:** `PackedSize` means compressed length only when the entry is compressed. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 837-843]  
**How to avoid:** Set `packed_size` and `stored_size` to `Size` for raw entries; set both to `PackedSize` for compressed entries. [VERIFIED: 06-CONTEXT.md D-07]  
**Warning signs:** Raw extraction reads zero bytes or metadata reports `stored_size == 0` for a non-empty raw fixture. [VERIFIED: 06-SPEC.md acceptance criteria]

### Pitfall 2: Adding a BSA-style uncompressed-size prefix to BA2 compressed payloads
**What goes wrong:** BA2 extraction reads compressed bytes directly and decompresses into `Size`; there is no BSA embedded 32-bit unpacked-size prefix in the BA2 GNRL extraction path. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 2157-2167]  
**Why it happens:** The existing BSA extractor does read a 32-bit size prefix for compressed TES4-family BSA payloads. [VERIFIED: src/bsa_reader.cpp]  
**How to avoid:** Keep BA2 extraction separate in `extract_ba2_entry` and pass `metadata.size` directly as `expected_size`. [VERIFIED: include/libbsa/compression.hpp; VERIFIED: 06-CONTEXT.md D-11]  
**Warning signs:** Deflate fixture only passes if four artificial bytes are inserted before compressed data. [VERIFIED: 06-SPEC.md]

### Pitfall 3: Starfield v3 raw entries accidentally using LZ4
**What goes wrong:** D-10 requires raw entries to remain raw even when archive `compression_method == 3`. [VERIFIED: 06-CONTEXT.md]  
**Why it happens:** `archive_default` with `compression_method == 3` routes to LZ4 block in the existing dispatcher. [VERIFIED: src/compression.cpp]  
**How to avoid:** Set per-entry `compression_state::raw` when `PackedSize == 0`; set `lz4_block` only for compressed entries in method-3 Starfield archives. [VERIFIED: 06-CONTEXT.md; VERIFIED: TES5Edit/Core/wbBSArchive.pas]  
**Warning signs:** Raw method-3 fixture fails unless bytes happen to be valid LZ4. [VERIFIED: 06-SPEC.md]

### Pitfall 4: Name table count mismatch not detected
**What goes wrong:** Records and names can become mis-associated or a truncated name table can pass silently. [VERIFIED: 06-CONTEXT.md D-16]  
**Why it happens:** BA2 stores names separately at `FileTableOffset`, one length-prefixed string per file. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 1195-1198]  
**How to avoid:** Read exactly `file_count` length-16 strings, fail on truncation, and test mismatched/truncated table cases. [VERIFIED: 06-CONTEXT.md D-16; CITED: https://miere.ru/posts/ba2-archive-format/]  
**Warning signs:** Parser accepts archives whose `FileTableOffset` points inside records or beyond `source.size()`. [VERIFIED: 06-SPEC.md acceptance criteria]

## Code Examples

Verified patterns from project sources:

### Public header pattern
```cpp
/// Opens a BA2-family archive and parses its GNRL metadata tables.
///
/// The source is read through bounded random-access calls; payload data remains
/// in the source until a caller extracts a selected entry.
[[nodiscard]] result<ba2_archive> open_ba2(const byte_source& source);

/// Extracts a single BA2 GNRL entry to a caller-owned sink.
[[nodiscard]] result<void> extract_ba2_entry(const ba2_archive& archive,
                                             const byte_source& source,
                                             std::string path,
                                             byte_sink& sink);
```
Source: BSA public API shape. [VERIFIED: include/libbsa/bsa.hpp]

### Compression route request
```cpp
payload_codec_request request{};
request.format = archive.summary().format;
request.entry_state = metadata.compression;
request.compression_method = archive.summary().compression_method;
auto algorithm = resolve_payload_codec(request);
```
Source: existing extractor and dispatcher contract. [VERIFIED: src/bsa_reader.cpp; VERIFIED: include/libbsa/compression.hpp]

### Catch2/CMake wiring
```cmake
add_executable(libbsa_ba2_reader_tests tests/ba2_reader_tests.cpp)
target_link_libraries(libbsa_ba2_reader_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain)

catch_discover_tests(libbsa_ba2_reader_tests TEST_PREFIX "libbsa_ba2_reader_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture;codec")
```
Source: existing CMake pattern and Catch2 docs. [VERIFIED: CMakeLists.txt; CITED: /catchorg/catch2]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Fallout 4 BA2 GNRL deflate only | Starfield v3 can require raw LZ4 block when `CompressionMethod == 3` | Starfield support added v2/v3 BA2 variants [VERIFIED: docs/PRD.md; VERIFIED: src/detect.cpp] | Parser must carry archive `compression_method` into extraction route. [VERIFIED: src/compression.cpp] |
| BSA compressed payloads include an unpacked-size prefix | BA2 GNRL record `Size` supplies the unpacked output length | BA2 design differs from TES4-family BSA [VERIFIED: TES5Edit/Core/wbBSArchive.pas] | BA2 extractor must not share BSA prefix skipping code. [VERIFIED: 06-CONTEXT.md D-11] |
| Real corpus / BSArchPro comparison as immediate gate | Generated deterministic fixtures for Phase 6 | Locked by Phase 6 context [VERIFIED: 06-CONTEXT.md] | Planner should prioritize source-reviewable fixture builders and defer broad corpus comparison to Phase 11. [VERIFIED: 06-SPEC.md] |

**Deprecated/outdated:**
- Treating all Starfield BA2 compressed payloads as deflate is outdated for v3 method-3 archives. [VERIFIED: docs/PRD.md; VERIFIED: src/compression.cpp]
- Using the LZ4 frame API for BA2 raw blocks is wrong because official LZ4 docs distinguish blocks from frames. [CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h]

## Assumptions Log

> List all claims tagged `[ASSUMED]` in this research. The planner and discuss-phase use this section to identify decisions that need user confirmation before execution.

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|

**If this table is empty:** All claims in this research were verified or cited — no user confirmation needed.

## Open Questions

1. **Should Starfield v2 `Unknown1` and `Unknown2` be exposed publicly?**
   - What we know: D-05 says not to add public fields for unknown/reserved header words unless research proves semantic need. [VERIFIED: 06-CONTEXT.md]
   - What's unclear: No semantic use was found in TES5Edit beyond reading/writing default values. [VERIFIED: TES5Edit/Core/wbBSArchive.pas lines 293-301 and 1672-1677]
   - Recommendation: Do not extend `archive_summary`; only account for header length internally and preserve available public summary fields. [VERIFIED: 06-CONTEXT.md D-05]

2. **Should parser validate the BA2 GNRL tail word equals `0xBAADF00D`?**
   - What we know: TES5Edit reads and skips the tail word with comment `BAADF00D`; external format notes also identify it as `0xBAADF00D`. [VERIFIED: TES5Edit/Core/wbBSArchive.pas line 1158; CITED: https://miere.ru/posts/ba2-archive-format/]
   - What's unclear: Phase context does not explicitly require rejecting non-`BAADF00D` tails. [VERIFIED: 06-CONTEXT.md]
   - Recommendation: Preserve it as a parser validation candidate only if generated malformed tests lock the behavior; otherwise skip/store internally without public exposure. [VERIFIED: 06-CONTEXT.md D-05 and D-16]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Configure/build | ✓ | 4.3.2 | Existing preset minimum is 3.24. [VERIFIED: cmake --version; VERIFIED: CMakePresets.json] |
| CTest | Test execution | ✓ | 4.3.2 | None needed. [VERIFIED: ctest --version] |
| Git | TES5Edit cleanliness and status checks | ✓ | 2.54.0.windows.1 | None needed. [VERIFIED: git --version] |
| vcpkg CLI | Dependency install/configure | ✗ on PATH | — | Existing preset uses `$env{VCPKG_ROOT}`; planner should include configure fallback/validation using established local build if available. [VERIFIED: environment probe; VERIFIED: CMakePresets.json] |
| Visual Studio generator | Configure/build | Not probed in this research | — | Prior project state records a local Visual Studio 2026 fallback build directory. [VERIFIED: .planning/STATE.md; VERIFIED: README.md] |

**Missing dependencies with no fallback:**
- `vcpkg` is not on PATH, but this is not blocking if `VCPKG_ROOT` is set for CMake manifest mode. [VERIFIED: environment probe; VERIFIED: CMakePresets.json]

**Missing dependencies with fallback:**
- Visual Studio 17 2022 preset may be unavailable locally; README documents `build/local-vs2026-vcpkg` fallback commands from prior phases. [VERIFIED: README.md; VERIFIED: .planning/STATE.md]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 `3.14.0#0` via vcpkg and CTest. [CITED: https://vcpkg.io/en/package/catch2.html; VERIFIED: CMakeLists.txt] |
| Config file | `CMakeLists.txt`, `CMakePresets.json`. [VERIFIED: CMakeLists.txt; VERIFIED: CMakePresets.json] |
| Quick run command | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` [VERIFIED: README.md pattern] |
| Full suite command | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` [VERIFIED: README.md pattern] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BA2-01 | FO4 v1/v7/v8 GNRL open/list/inspect/extract raw and deflate | fixture/unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` | ❌ Wave 0 [VERIFIED: no tests/ba2_reader_tests.cpp read or listed in CMakeLists.txt] |
| BA2-02 | Starfield v2 GNRL header length, summary metadata, names, extraction | fixture/unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` | ❌ Wave 0 [VERIFIED: CMakeLists.txt] |
| BA2-03 | Starfield v3 raw/deflate/LZ4-block route behavior | fixture/codec | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L codec` | ❌ Wave 0 [VERIFIED: CMakeLists.txt] |
| BA2-04 | `FileTableOffset` length-prefixed names, normalization, malformed table failure | fixture/unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` | ❌ Wave 0 [VERIFIED: CMakeLists.txt] |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` plus `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke`. [VERIFIED: README.md]
- **Per wave merge:** `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L "unit|fixture|codec|smoke"`. [VERIFIED: CMakeLists.txt labels]
- **Phase gate:** Full suite green, public-header grep for private dependencies, and `git status --short TES5Edit` empty. [VERIFIED: README.md; VERIFIED: AGENTS.md]

### Wave 0 Gaps
- [ ] `include/libbsa/ba2.hpp` — public API for BA2 requirement coverage. [VERIFIED: 06-CONTEXT.md]
- [ ] `src/ba2_reader.cpp` — BA2 parser/extractor implementation. [VERIFIED: 06-CONTEXT.md]
- [ ] `tests/ba2_reader_tests.cpp` — generated fixture builders and BA2 malformed cases. [VERIFIED: 06-CONTEXT.md]
- [ ] `tests/public_header_smoke.cpp` update — include `ba2.hpp`, name `ba2_archive`, and take `open_ba2` / `extract_ba2_entry` addresses. [VERIFIED: 06-CONTEXT.md D-03]
- [ ] `CMakeLists.txt` update — explicit new public header, source file, and BA2 reader test target. [VERIFIED: CMakeLists.txt]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No authentication surface in archive parser. [VERIFIED: 06-SPEC.md] |
| V3 Session Management | no | No session surface in archive parser. [VERIFIED: 06-SPEC.md] |
| V4 Access Control | no | Library does not implement disk extraction policy or authorization. [VERIFIED: 06-SPEC.md out-of-scope] |
| V5 Input Validation | yes | Bounded reads, overflow/range checks, structured `malformed_archive` / `unsupported_format` errors, generated malformed fixtures. [VERIFIED: src/bsa_reader.cpp; VERIFIED: 06-CONTEXT.md D-16] |
| V6 Cryptography | no | No cryptographic primitive is introduced; FO4 hashes are non-cryptographic compatibility hashes. [VERIFIED: src/hash.cpp; VERIFIED: tests/path_hash_tests.cpp] |

### Known Threat Patterns for BA2 parsing

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Truncated header/record/name table | Tampering | Exact bounded reads and `malformed_archive` failures. [VERIFIED: src/bsa_reader.cpp; VERIFIED: 06-CONTEXT.md D-16] |
| Offset + size overflow | Tampering/Denial of Service | Reuse checked-add/range-fit pattern before allocation or read. [VERIFIED: src/bsa_reader.cpp] |
| Unchecked allocation from huge `FileCount` or size fields | Denial of Service | Validate table ranges and `std::size_t` conversion before vector allocation/read. [VERIFIED: src/bsa_reader.cpp; VERIFIED: include/libbsa/io.hpp] |
| Codec confusion | Tampering | Resolve codec from format, per-entry state, and `compression_method`; never fallback between deflate/LZ4 modes. [VERIFIED: src/compression.cpp; VERIFIED: 06-CONTEXT.md D-12] |
| Path traversal in archive names | Tampering | Normalize via `normalize_archive_path`, which rejects absolute/traversal paths. [VERIFIED: include/libbsa/archive_path.hpp; VERIFIED: tests/path_hash_tests.cpp] |

## Sources

### Primary (HIGH confidence)
- `J:\libbsa-gsd\AGENTS.md` — project constraints, TES5Edit boundary, dependencies, comments, validation expectations. [VERIFIED: file read]
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md` — locked user decisions D-01 through D-16, scope, fixtures, and canonical references. [VERIFIED: file read]
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-SPEC.md` — Phase 6 requirements, boundaries, and acceptance criteria. [VERIFIED: file read]
- `.planning/REQUIREMENTS.md` — BA2-01 through BA2-04 requirement definitions and traceability. [VERIFIED: file read]
- `.planning/STATE.md` — prior phase decisions and local VS2026 fallback context. [VERIFIED: file read]
- `TES5Edit/Core/wbBSArchive.pas` — BA2 header/record structs, Starfield v2/v3 header handling, GNRL record parse, name table parse, FO4 hash, and extraction behavior. [VERIFIED: file read]
- `include/libbsa/bsa.hpp`, `archive.hpp`, `archive_view.hpp`, `io.hpp`, `compression.hpp` — reusable public API patterns and metadata contracts. [VERIFIED: file read]
- `src/detect.cpp`, `src/compression.cpp`, `src/bsa_reader.cpp`, `src/hash.cpp` — existing BA2 detection, compression routing, BSA parser/extractor patterns, and FO4 hash helpers. [VERIFIED: file read]
- `CMakeLists.txt`, `CMakePresets.json`, `README.md` — build/test wiring and commands. [VERIFIED: file read]
- Context7 `/catchorg/catch2` — `catch_discover_tests`, `ADD_TAGS_AS_LABELS`, and CTest integration. [CITED: /catchorg/catch2]
- Context7 `/kitware/cmake` — `target_sources(FILE_SET HEADERS)` and install/export patterns. [CITED: /kitware/cmake]
- vcpkg package pages for libdeflate, lz4, and Catch2. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://vcpkg.io/en/package/catch2.html]
- Official libdeflate README and LZ4 header documentation. [CITED: https://github.com/ebiggers/libdeflate/blob/master/README.md; CITED: https://raw.githubusercontent.com/lz4/lz4/dev/lib/lz4.h]

### Secondary (MEDIUM confidence)
- BA2 archive format article — cross-check for GNRL record size, packed/unpacked sizes, and uint16 length-prefixed name table. [CITED: https://miere.ru/posts/ba2-archive-format/]
- Bethesda Structs documentation — ecosystem confirmation that BTDX GNRL stores general compressed files and DX10 needs DDS reconstruction. [CITED: https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html]

### Tertiary (LOW confidence)
- None used for prescriptive recommendations.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — dependencies, versions, and build wiring were verified from project files, vcpkg pages, Context7, and official library docs. [VERIFIED: vcpkg.json; CITED: vcpkg pages; CITED: /catchorg/catch2; CITED: /kitware/cmake]
- Architecture: HIGH — API shape and metadata lifetimes are locked by CONTEXT and directly mirror existing BSA code. [VERIFIED: 06-CONTEXT.md; VERIFIED: include/libbsa/bsa.hpp; VERIFIED: src/bsa_reader.cpp]
- Pitfalls: HIGH — compression, name table, and record layout pitfalls are verified against TES5Edit and existing dispatcher behavior. [VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: src/compression.cpp]

**Research date:** 2026-05-05  
**Valid until:** 2026-06-04 for project-specific parser planning; re-check vcpkg package versions before dependency baseline changes. [VERIFIED: vcpkg package pages]
