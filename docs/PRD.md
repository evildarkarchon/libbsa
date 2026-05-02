# Product Requirements Document: libbsa

## Overview

**libbsa** is a reusable C++ library for reading and writing Bethesda Game Studios archive formats (BSA and BA2). It targets all known format versions spanning Morrowind through Starfield, providing a clean, portable API suitable for embedding in modding tools, asset pipelines, and game utilities.

The behavioral reference is the BSArchPro codebase within TES5Edit. This library reimplements that behavior as idiomatic C++20 with no UI coupling.

---

## Goals

1. **Format coverage** -- Support every BSA/BA2 variant shipped by Bethesda games from Morrowind (2002) through Starfield (2023+).
2. **Correctness** -- Byte-level compatibility with archives produced by official tools (Archive.exe, Archive2.exe) and BSArchPro.
3. **Reusability** -- Static or dynamic library consumption; no global state; no mandatory dynamic allocation strategy.
4. **Performance** -- Multi-threaded packing/extraction; zero-copy where possible; streaming I/O for large archives.
5. **Portability** -- Windows primary target; minimize platform-specific code to enable future Linux/macOS support.

---

## Non-Goals

- GUI or CLI tool (consumers build their own).
- Network/URL-based archive access.
- In-place modification of existing archives (open-read-modify-write is acceptable).
- Support for non-Bethesda archive formats (ZIP, 7z, etc.).

---

## Supported Archive Formats

| Format ID | Game(s) | Extension | Magic | Compression |
|-----------|---------|-----------|-------|-------------|
| TES3 | Morrowind | `.bsa` | `\x00\x01\x00\x00` | None |
| TES4 | Oblivion | `.bsa` | `BSA\0` v103 | Deflate |
| FO3 | Fallout 3, FNV, Skyrim LE | `.bsa` | `BSA\0` v104 | Deflate |
| SSE | Skyrim SE/AE | `.bsa` | `BSA\0` v105 | LZ4 Frame |
| FO4 GNRL | Fallout 4 | `.ba2` | `BTDX`+`GNRL` v1/7/8 | Deflate |
| FO4 DDS | Fallout 4 | `.ba2` | `BTDX`+`DX10` v1/7/8 | Deflate |
| SF GNRL | Starfield | `.ba2` | `BTDX`+`GNRL` v2 | Deflate or LZ4 Block |
| SF DDS | Starfield | `.ba2` | `BTDX`+`DX10` v3 | LZ4 Block |

---

## Core Capabilities

### Reading
- Auto-detect archive format from magic bytes and version.
- Parse and expose the folder/file index.
- Random-access extraction of individual files by path or hash.
- Streaming extraction (caller-provided output sink).
- Bulk iteration with caller-supplied callback.
- Transparent decompression (deflate, LZ4 frame, LZ4 block).
- DDS header reconstruction for BA2 DDS archives.

### Writing
- Create new archives of any supported format.
- Add files from disk paths or in-memory buffers.
- Per-file compression override (compress / leave raw / use archive default).
- Data deduplication (identical files share one data region via content hash).
- Automatic file-flag and archive-flag derivation from content.
- DDS mipmap chunking for BA2 DDS archives.
- Finalize (write header + file table) after all data is appended.

### Query / Inspection
- List all file paths in an archive.
- Check file existence by path.
- Retrieve metadata (compressed size, raw size, compression method, hash).
- Report archive type, version, flags, file count.

---

## Dependencies

| Library | Purpose | Acquisition |
|---------|---------|-------------|
| libdeflate | Deflate compress/decompress | vcpkg |
| lz4 | LZ4 frame and block compress/decompress | vcpkg |
| DirectXTex | DDS header parsing, DXGI format detection, mipmap analysis | vcpkg |

No other external dependencies without documented justification.

---

## Milestones

### Milestone 1: Foundation and TES4-family Read Support

**Goal:** Parse and extract files from Oblivion, Fallout 3/NV, Skyrim LE, and Skyrim SE/AE BSA archives.

**Deliverables:**
- Project structure: `include/`, `src/`, `tests/` directories; CMake build that can produce either a static or dynamic library.
- Binary header/record structures for TES4/FO3/SSE formats.
- Archive type auto-detection (magic + version).
- Read and parse folder/file index tables.
- TES4-family hash algorithm implementation (`CreateHashTES4`).
- File lookup by path (hash-based).
- File extraction with transparent deflate (TES4/FO3) and LZ4 frame (SSE) decompression.
- Embedded file-name handling (`ARCHIVE_EMBEDNAME` flag).
- Unit tests proving extraction against known archives.

**Exit criteria:** Can open any TES4/FO3/SSE BSA archive and extract any file, producing byte-identical output to BSArchPro.

---

### Milestone 2: TES3 (Morrowind) Read Support

**Goal:** Extend read support to Morrowind's simpler BSA format.

**Deliverables:**
- TES3 header and record structures.
- TES3 hash algorithm implementation (`CreateHashTES3`).
- Morrowind-specific offset calculation (relative to data section).
- File listing and extraction.
- Tests against Morrowind BSA fixtures.

**Exit criteria:** Can open and extract all files from Morrowind BSA archives.

---

### Milestone 3: BA2 General Read Support (Fallout 4 / Starfield)

**Goal:** Parse and extract files from Fallout 4 and Starfield GNRL BA2 archives.

**Deliverables:**
- BTDX+GNRL header and record structures (versions 1, 2, 7, 8).
- FO4 CRC32-based hash algorithm (`CreateHashFO4`).
- Per-file compression detection (`PackedSize != 0`).
- Deflate decompression for FO4/SF v2.
- LZ4 block decompression for SF v2 with `CompressionMethod = 3`.
- File name table parsing (length-prefixed, located at `FileTableOffset`).
- Tests against FO4 and Starfield GNRL archive fixtures.

**Exit criteria:** Can open and extract all files from FO4/SF GNRL BA2 archives across all known versions.

---

### Milestone 4: BA2 DDS Read Support

**Goal:** Parse and extract DDS texture archives with proper header reconstruction.

**Deliverables:**
- BTDX+DX10 header and record structures.
- Texture chunk parsing (variable-length records).
- DDS header reconstruction from stored metadata (DXGI format, dimensions, mips, cubemap flags).
- Per-chunk decompression (deflate or LZ4 block depending on version).
- Correct cubemap handling.
- Tests proving reconstructed DDS files match original source textures.

**Exit criteria:** Can extract any texture from FO4/SF DDS BA2 archives with a valid, loadable DDS file as output.

---

### Milestone 5: TES4-family Write Support

**Goal:** Create new BSA archives (TES4/FO3/SSE) from a set of input files.

**Deliverables:**
- Archive creation API (specify format, add files, finalize).
- Sorted folder/file index generation (hash-ordered).
- Hash computation for folder and file names.
- Compression (deflate for TES4/FO3, LZ4 frame for SSE).
- Per-file compression override.
- Automatic archive-flag and file-flag derivation.
- Embedded file-name writing when appropriate.
- Data deduplication (optional, via content hash).
- Round-trip test: pack files into a BSA, then extract and compare to originals.

**Exit criteria:** Produces BSA archives that are loadable by the corresponding game engines and whose extracted content is byte-identical to the source files.

---

### Milestone 6: BA2 General Write Support

**Goal:** Create new FO4/SF GNRL BA2 archives.

**Deliverables:**
- BA2 GNRL creation API.
- File table generation at end of archive.
- Per-file compression (deflate or LZ4 block depending on target version).
- Data deduplication.
- Version-specific header differences (v1, v2, v7, v8).
- Round-trip tests against FO4 and Starfield GNRL archives.

**Exit criteria:** Produces GNRL BA2 archives compatible with Fallout 4 and Starfield.

---

### Milestone 7: BA2 DDS Write Support

**Goal:** Create DDS texture archives with proper mipmap chunking.

**Deliverables:**
- DDS file analysis via DirectXTex (format detection, mip count, dimensions).
- Mipmap chunking algorithm (configurable max chunk count).
- Per-chunk compression.
- DX10 record generation from DDS metadata.
- Cubemap support.
- Round-trip test: pack DDS textures into a BA2, extract, and compare.

**Exit criteria:** Produces DDS BA2 archives that load correctly in FO4/Starfield with no texture corruption.

---

### Milestone 8: TES3 Write Support and Morrowind Round-trip

**Goal:** Complete Morrowind archive write capability.

**Deliverables:**
- TES3 archive creation API.
- Hash-sorted file index generation.
- Offset calculation (data-section-relative).
- Round-trip tests.

**Exit criteria:** Can produce Morrowind-loadable BSA archives.

---

### Milestone 9: Multi-threading and Performance

**Goal:** Add parallelism to compression/decompression and I/O for large archives.

**Deliverables:**
- Thread pool or `std::jthread`-based parallel compression during packing.
- Parallel decompression during bulk extraction.
- Streaming I/O (avoid loading entire archive into memory).
- Benchmark suite comparing single-threaded vs. multi-threaded performance.
- Thread-safety documentation and guarantees.

**Exit criteria:** Multi-threaded packing/extraction of a 10+ GB archive is measurably faster than single-threaded, with no data corruption.

---

### Milestone 10: Polish, Compatibility, and Hardening

**Goal:** Handle edge cases, improve error reporting, and ensure rock-solid compatibility.

**Deliverables:**
- Graceful handling of malformed/truncated archives.
- Known Bethesda quirks (sounds-in-compressed warning, SSE `EMBEDNAME` crash bug avoidance, Fallout vanilla zlib bug tolerance).
- Comprehensive error codes or exception hierarchy.
- API documentation (Doxygen).
- Integration examples demonstrating library usage.
- CI pipeline (build + test on MSVC, optionally Clang/GCC).

**Exit criteria:** Library handles all known edge cases from the reference implementation; API documentation is complete; CI is green.

---

## API Design Principles

1. **Value semantics by default** -- Archives and file records are movable; no raw `new`/`delete` in the public API.
2. **Non-throwing fast path** -- Use `std::expected` or error codes for operations that can fail due to I/O or format issues. Reserve exceptions for programmer errors (precondition violations).
3. **Minimal headers** -- Public headers should avoid pulling in platform or compression library headers.
4. **Extensible format registry** -- Adding a hypothetical new archive version should require minimal changes (new record struct + hash function + entry in the type-detection table).
5. **No implicit global state** -- No singletons, no static mutable data. Thread safety comes from isolation, not locking.

---

## Testing Strategy

- **Unit tests** for hash algorithms, compression round-trips, header parsing, and record serialization.
- **Integration tests** using fixture archives (small hand-crafted BSA/BA2 files committed to the repo or generated by a helper script).
- **Round-trip tests** that pack a known file set, extract, and diff.
- **Compatibility tests** that compare extraction output against BSArchPro's output for the same archive.
- **Fuzz targets** (optional, milestone 10) for the header-parsing code paths.

Test framework: to be selected in Milestone 1 (likely Catch2 or GoogleTest via vcpkg).

---

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Undocumented format quirks | Incorrect output breaks game loading | Byte-compare against BSArchPro output; test with real game archives |
| BA2 version drift (future game updates) | Library becomes outdated | Extensible format registry; version field drives behavior branches |
| DirectXTex Windows-only concern | Blocks future cross-platform | Isolate DDS logic behind an interface; DDS write is only required for BA2 DDS archives |
| Large archive performance | Unusable for 50+ GB Starfield archives | Streaming I/O from milestone 1; defer full parallelism to milestone 9 |
| LZ4 frame vs. block confusion | Silent data corruption | Dedicated code paths per format with explicit version checks; extensive test coverage |

---

## Success Metrics

- All Bethesda archive formats (8 variants) readable and writable.
- Byte-identical extraction compared to BSArchPro for a corpus of test archives.
- Archives produced by libbsa load correctly in their target game engines.
- Clean, documented public API with no BSArchPro/Delphi idioms leaking through.
- Build time under 30 seconds for a clean build (excluding vcpkg install).
