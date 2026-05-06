# Requirements: libbsa

**Defined:** 2026-05-05
**Core Value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Foundation

- [x] **FND-01**: Consumer can build libbsa as a reusable C++20 library with CMake and vcpkg-managed dependencies.
- [x] **FND-02**: Consumer can run a focused test suite through CTest for parser, codec, fixture, and round-trip behavior.
- [x] **FND-03**: Consumer can receive structured errors for invalid magic, unsupported versions, truncated records, impossible offsets, decompression failures, and malformed archives.
- [x] **FND-04**: Consumer can use archive operations without global mutable state or application-specific UI/tooling dependencies.
- [x] **FND-05**: Maintainer can trace non-obvious compatibility behavior to BSArchPro/TES5Edit reference code without modifying the `TES5Edit/` submodule.

### Binary I/O and API

- [x] **BIO-01**: Consumer can read archive data through a random-access source abstraction without loading the whole archive into memory.
- [x] **BIO-02**: Consumer can extract archive payloads to a caller-provided streaming output sink.
- [x] **BIO-03**: Consumer can use in-memory convenience helpers layered over the streaming APIs for small payloads.
- [x] **BIO-04**: Consumer can inspect archive type, version, flags, file count, entry sizes, offsets, hashes, and compression state without extracting file data.
- [x] **BIO-05**: Public headers expose libbsa-owned types and avoid leaking libdeflate, LZ4, DirectXTex, platform, Delphi, or UI implementation details.

### Detection, Paths, and Hashes

- [x] **DPH-01**: Consumer can auto-detect TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DDS archives from magic bytes, version, subtype, and compression method where applicable.
- [x] **DPH-02**: Consumer can list normalized archive paths for every entry in a supported archive.
- [x] **DPH-03**: Consumer can check whether a file exists by archive path and retrieve its metadata.
- [x] **DPH-04**: Consumer can perform random-access lookup using Bethesda-compatible path normalization and hash behavior.
- [x] **DPH-05**: Maintainer can validate TES3, TES4-family, and FO4/BA2 hash implementations against golden vectors traced from the reference behavior.

### Compression

- [x] **CMP-01**: Consumer can transparently extract deflate-compressed TES4, FO3/FNV, Fallout 4 BA2, and Starfield deflate payloads.
- [x] **CMP-02**: Consumer can transparently extract Skyrim SE/AE BSA payloads that use LZ4 frame compression.
- [x] **CMP-03**: Consumer can transparently extract Starfield BA2 v3 payloads that use raw LZ4 block compression when `CompressionMethod == 3`.
- [x] **CMP-04**: Maintainer can test compression routing so deflate, LZ4 frame, and LZ4 block paths cannot be accidentally interchanged.
- [x] **CMP-05**: Writer APIs can select archive default compression, force compressed, or force raw behavior where the target format supports it.

### BSA Reading

- [x] **BSA-01**: Consumer can open, list, inspect, and extract TES4 v103 BSA archives for Oblivion.
- [x] **BSA-02**: Consumer can open, list, inspect, and extract FO3/FNV/Skyrim LE v104 BSA archives.
- [x] **BSA-03**: Consumer can open, list, inspect, and extract Skyrim SE/AE v105 BSA archives.
- [x] **BSA-04**: Consumer can open, list, inspect, and extract TES3 Morrowind BSA archives with correct data-section-relative offsets.
- [x] **BSA-05**: Consumer can extract BSA entries with embedded filenames while preserving payload bytes after the embedded-name prefix is handled.

### BA2 Reading

- [ ] **BA2-01**: Consumer can open, list, inspect, and extract Fallout 4 BA2 GNRL archives across supported v1/v7/v8 variants.
- [ ] **BA2-02**: Consumer can open, list, inspect, and extract Starfield BA2 GNRL v2 archives with the additional header fields preserved in metadata where relevant.
- [ ] **BA2-03**: Consumer can open, list, inspect, and extract Starfield BA2 GNRL v3 archives with correct compression-method routing.
- [x] **BA2-04**: Consumer can parse BA2 file name tables located at `FileTableOffset` and associate length-prefixed names with entries.
- [ ] **BA2-05**: Consumer can open, list, inspect, and extract Fallout 4 and Starfield BA2 DDS archives.
- [ ] **BA2-06**: Consumer can extract BA2 DDS texture entries as valid, loadable DDS files with reconstructed headers, dimensions, DXGI formats, mip levels, and cubemap metadata.

### Writing

- [ ] **WRT-01**: Consumer can create TES4-family BSA archives for Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE from disk paths or in-memory buffers.
- [ ] **WRT-02**: Consumer can create Fallout 4 and Starfield BA2 GNRL archives with version-specific headers, file tables, offsets, and compression metadata.
- [ ] **WRT-03**: Consumer can create Fallout 4 and Starfield BA2 DDS archives from DDS inputs with proper metadata analysis and mipmap chunking.
- [ ] **WRT-04**: Consumer can create TES3 Morrowind BSA archives with correct hash sorting and data-section-relative offsets.
- [ ] **WRT-05**: Consumer can finalize new archives through streaming output without requiring the entire archive image in memory.
- [ ] **WRT-06**: Consumer can opt into content deduplication so identical files share a data region when the target format allows it.
- [ ] **WRT-07**: Maintainer can verify archives written by libbsa through read-after-write, round-trip extraction, and metadata comparison tests.

### Compatibility and Validation

- [x] **VAL-01**: Maintainer can run unit tests for hash algorithms, compression round trips, header parsing, and record serialization.
- [ ] **VAL-02**: Maintainer can run fixture-based integration tests using committed or generated small archives for each supported family.
- [ ] **VAL-03**: Maintainer can compare extraction output against BSArchPro output for the same archive corpus.
- [ ] **VAL-04**: Maintainer can verify malformed or truncated archive handling without crashes or unchecked allocations.
- [ ] **VAL-05**: Consumer can receive warnings or explicit behavior for known Bethesda quirks, including compressed sounds, SSE `EMBEDNAME` hazards, and Fallout vanilla zlib tolerance.

### Performance and Documentation

- [ ] **PERF-01**: Consumer can run bulk extraction without whole-archive buffering and with clear partial-failure reporting.
- [ ] **PERF-02**: Consumer can use multi-threaded packing or extraction for large archives after correctness paths are established.
- [ ] **PERF-03**: Maintainer can run benchmarks comparing single-threaded and multi-threaded behavior on large archive workloads.
- [ ] **DOC-01**: Consumer can read Doxygen API documentation covering core types, error semantics, threading/lifetime guarantees, and supported formats.
- [ ] **DOC-02**: Consumer can follow integration examples for open/list/extract, streaming extraction, archive creation, and error handling.

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Diagnostics and Tooling Helpers

- **DIAG-01**: Consumer can generate compatibility lint reports for engine hazards, path hazards, compression/layout concerns, and suspicious metadata.
- **DIAG-02**: Consumer can compare two archives through metadata and optional content diff primitives.
- **DIAG-03**: Consumer can perform safe disk extraction with policy controls for traversal, absolute paths, overwrites, and partial failures.
- **DIAG-04**: Consumer can dry-run archive builds to preview output layout, compression choices, warnings, and estimated sizes.

### Rebuild Workflows

- **RBLD-01**: Consumer can rebuild an archive while preserving unchanged members from an existing archive without true in-place mutation.
- **RBLD-02**: Consumer can preserve unchanged BA2 DDS texture metadata/chunks when rebuilding texture archives.

### Optional Scale Features

- **SCAL-01**: Consumer can choose an optional memory-mapped input backend where it measurably improves random-access workloads.
- **SCAL-02**: Maintainer can run fuzz or property-test harnesses for header and record parsing paths.

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| GUI archive browser or extractor | The product is a reusable library; UI belongs in consuming tools. |
| Productized CLI application | CLI behavior can dominate API design and is not part of the PRD scope. |
| Network or URL archive access | Transport, caching, authentication, and retries are application concerns. |
| ZIP, 7z, or other non-Bethesda archive formats | The compatibility target is Bethesda BSA/BA2 only. |
| True in-place mutation of existing archives | Offset tables, compression sizes, and chunking make safe mutation fragile; rebuild workflows are acceptable instead. |
| Directly compiling, wrapping, editing, or staging TES5Edit source | TES5Edit is read-only prior art and compatibility reference material. |
| Texture transcoding or optimization | libbsa should parse/analyze DDS enough for BA2 packaging, not become a texture conversion tool. |
| Console-only swizzled texture or proprietary XMem support | These need separate platform-specific research and conflict with the portable PC-focused scope. |
| Global singleton configuration | Conflicts with the no-global-state and thread-safety goals. |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| FND-01 | Phase 1 | Complete |
| FND-02 | Phase 1 | Complete |
| FND-03 | Phase 1 | Complete |
| FND-04 | Phase 1 | Complete |
| FND-05 | Phase 1 | Complete |
| BIO-01 | Phase 2 | Complete |
| BIO-02 | Phase 2 | Complete |
| BIO-03 | Phase 2 | Complete |
| BIO-04 | Phase 2 | Complete |
| BIO-05 | Phase 2 | Complete |
| DPH-01 | Phase 2 | Complete |
| DPH-02 | Phase 2 | Complete |
| DPH-03 | Phase 2 | Complete |
| DPH-04 | Phase 2 | Complete |
| DPH-05 | Phase 2 | Complete |
| CMP-01 | Phase 3 | Complete |
| CMP-02 | Phase 3 | Complete |
| CMP-03 | Phase 3 | Complete |
| CMP-04 | Phase 3 | Complete |
| CMP-05 | Phase 3 | Complete |
| BSA-01 | Phase 4 | Complete |
| BSA-02 | Phase 4 | Complete |
| BSA-03 | Phase 4 | Complete |
| BSA-04 | Phase 5 | Complete |
| BSA-05 | Phase 4 | Complete |
| BA2-01 | Phase 6 | Pending |
| BA2-02 | Phase 6 | Pending |
| BA2-03 | Phase 6 | Pending |
| BA2-04 | Phase 6 | Complete |
| BA2-05 | Phase 7 | Pending |
| BA2-06 | Phase 7 | Pending |
| WRT-01 | Phase 9 | Pending |
| WRT-02 | Phase 10 | Pending |
| WRT-03 | Phase 10 | Pending |
| WRT-04 | Phase 9 | Pending |
| WRT-05 | Phase 8 | Pending |
| WRT-06 | Phase 8 | Pending |
| WRT-07 | Phase 8 | Pending |
| VAL-01 | Phase 1 | Complete |
| VAL-02 | Phase 11 | Pending |
| VAL-03 | Phase 11 | Pending |
| VAL-04 | Phase 11 | Pending |
| VAL-05 | Phase 11 | Pending |
| PERF-01 | Phase 12 | Pending |
| PERF-02 | Phase 12 | Pending |
| PERF-03 | Phase 12 | Pending |
| DOC-01 | Phase 12 | Pending |
| DOC-02 | Phase 12 | Pending |

**Coverage:**
- v1 requirements: 48 total
- Mapped to phases: 48
- Unmapped: 0

---
*Requirements defined: 2026-05-05*
*Last updated: 2026-05-05 after roadmap traceability mapping*
