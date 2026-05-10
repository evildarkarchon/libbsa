# Requirements: libbsa

**Defined:** 2026-05-07
**Core Value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

## v1 Requirements

Requirements for the initial complete library scope. Each maps to roadmap phases.

### Foundation

- [x] **FND-01**: Consumer can build libbsa as a reusable C++20 library with CMake and vcpkg.
- [x] **FND-02**: Consumer can choose static or shared library builds without changing public headers.
- [x] **FND-03**: Consumer can include public libbsa headers without transitively depending on libdeflate, lz4, DirectXTex, or TES5Edit headers.
- [x] **FND-04**: Consumer can receive structured C++20-compatible result/error values for I/O and format failures.
- [x] **FND-05**: Consumer can use libbsa objects without process-wide mutable global state or singleton behavior.
- [x] **FND-06**: Maintainer can run Catch2/CTest test suites for unit, fixture, round-trip, compatibility, and slow tests.
- [x] **FND-07**: Maintainer can add small legal archive fixtures without mutating the `TES5Edit/` submodule or relying on copyrighted game archives in the repo.

### Format Detection and Metadata

- [x] **FMT-01**: Consumer can open an archive and have libbsa detect its format from magic bytes, archive type fields, and version fields rather than file extension.
- [x] **FMT-02**: Consumer can inspect archive type, variant, version, flags, file count, and supported compression behavior.
- [x] **FMT-03**: Consumer can list archive file paths in a stable library-owned path representation.
- [x] **FMT-04**: Consumer can check whether a path exists in an archive using normalized archive virtual path semantics.
- [x] **FMT-05**: Consumer can retrieve per-entry metadata including raw size, compressed size, offset, compression method, hash values, and format-specific record data where applicable.
- [x] **FMT-06**: Maintainer can add a future archive version by extending detection/record handling without rewriting unrelated format families.

### Shared Binary and Compression Services

- [x] **BIN-01**: Parser can read little-endian integer fields with checked offset, count, and size arithmetic.
- [x] **BIN-02**: Parser rejects truncated or malformed archives with structured errors instead of out-of-bounds reads or undefined behavior.
- [x] **BIN-03**: Extraction can stream payloads to caller-provided sinks without loading whole archives into memory.
- [x] **BIN-04**: Compression service can decompress deflate payloads with exact expected output size validation.
- [x] **BIN-05**: Compression service can decompress Skyrim SE/AE BSA LZ4 frame payloads through the LZ4 frame API.
- [x] **BIN-06**: Compression service can decompress Starfield BA2 v3 raw LZ4 block payloads through the LZ4 block API.
- [x] **BIN-07**: Compression service can compress deflate, LZ4 frame, and raw LZ4 block payloads for writer phases using explicit target-format routing.
- [x] **BIN-08**: Hash service can compute TES3, TES4-family, and FO4/BA2 hashes with fixture-backed expected values.

### BSA Read Support

- [x] **BSA-01**: Consumer can read and extract files from TES4/Oblivion BSA v103 archives.
- [x] **BSA-02**: Consumer can read and extract files from FO3/FNV/Skyrim LE BSA v104 archives.
- [x] **BSA-03**: Consumer can read and extract files from Skyrim SE/AE BSA v105 archives.
- [x] **BSA-04**: Consumer can read and extract files from TES3/Morrowind BSA archives.
- [x] **BSA-05**: Consumer can locate TES4-family BSA entries by path using hash-based lookup behavior compatible with BSArchPro.
- [x] **BSA-06**: Consumer can extract embedded-name BSA entries while preserving payload bytes compatible with BSArchPro output.
- [x] **BSA-07**: Consumer can extract BSA entries that are stored raw, deflate-compressed, or LZ4-frame-compressed according to archive version and flags.
- [x] **BSA-08**: Consumer can extract TES3 entries using data-section-relative offset semantics.

### BA2 General Read Support

- [x] **GNRL-01**: Consumer can read and extract files from Fallout 4 BA2 GNRL archives.
- [x] **GNRL-02**: Consumer can read and extract files from Starfield BA2 v2 GNRL archives.
- [x] **GNRL-03**: Consumer can read and extract structurally valid Starfield BA2 v3 GNRL archives.
- [x] **GNRL-04**: Consumer can parse BA2 filename tables located at `FileTableOffset` with length-prefixed names.
- [x] **GNRL-05**: Consumer can distinguish raw BA2 entries from compressed entries using `PackedSize` and format metadata.
- [x] **GNRL-06**: Consumer can extract BA2 GNRL entries compressed with deflate.
- [x] **GNRL-07**: Consumer can extract Starfield BA2 v3 GNRL entries compressed with raw LZ4 block when `CompressionMethod == 3`.
- [x] **GNRL-08**: Consumer can inspect and preserve Starfield BA2 v2/v3 version-specific header fields in metadata.

### BA2 DDS Read Support

- [x] **DDS-01**: Consumer can read Fallout 4 BA2 DX10/DDS texture archives.
- [x] **DDS-02**: Consumer can read Starfield BA2 v3 DX10/DDS texture archives.
- [x] **DDS-03**: Consumer can inspect texture metadata including dimensions, mip count, DXGI format, cubemap/array information, and chunk layout.
- [x] **DDS-04**: Consumer can extract BA2 DDS entries as valid DDS files with reconstructed headers.
- [x] **DDS-05**: Consumer can extract BA2 DDS chunks compressed with deflate or raw LZ4 block according to archive version and chunk metadata.
- [x] **DDS-06**: Maintainer can validate reconstructed DDS outputs through DirectXTex metadata loading without exposing DirectXTex types publicly.
- [x] **DDS-07**: Consumer can extract cubemap textures with correct DDS metadata and face/mip ordering.

### BSA Write Support

- [x] **WBSA-01**: Consumer can create new TES4/Oblivion BSA v103 archives from disk files or memory buffers.
- [x] **WBSA-02**: Consumer can create new FO3/FNV/Skyrim LE BSA v104 archives from disk files or memory buffers.
- [x] **WBSA-03**: Consumer can create new Skyrim SE/AE BSA v105 archives from disk files or memory buffers.
- [x] **WBSA-04**: Consumer can create new TES3/Morrowind BSA archives from disk files or memory buffers.
- [x] **WBSA-05**: Writer can generate folder and file indexes sorted by format-compatible hash order.
- [x] **WBSA-06**: Writer can derive archive flags and file flags from content using compatible behavior.
- [x] **WBSA-07**: Writer can apply per-file compression overrides while respecting target archive defaults.
- [x] **WBSA-08**: Writer can write embedded file names where appropriate without triggering known compatibility hazards.
- [x] **WBSA-09**: Writer can optionally deduplicate identical file payloads by content hash.
- [x] **WBSA-10**: Maintainer can round-trip BSA writer output by packing, reopening, extracting, and byte-comparing source files.

### BA2 Write Support

- [x] **WBA2-01**: Consumer can create new Fallout 4 BA2 GNRL archives from disk files or memory buffers.
- [x] **WBA2-02**: Consumer can create new Starfield BA2 GNRL archives with explicit target version and compression method policy.
- [x] **WBA2-03**: Writer can serialize BA2 filename tables at the end of the archive.
- [x] **WBA2-04**: Writer can compress BA2 GNRL entries with deflate or raw LZ4 block according to target format/version.
- [x] **WBA2-05**: Writer can preserve or set version-specific BA2 header fields according to documented target profiles.
- [x] **WBA2-06**: Consumer can create new Fallout 4 BA2 DX10/DDS texture archives from DDS files.
- [x] **WBA2-07**: Consumer can create new Starfield BA2 v3 DX10/DDS texture archives from DDS files.
- [x] **WBA2-08**: Writer can analyze DDS input through DirectXTex and generate BA2 texture records from library-owned metadata.
- [x] **WBA2-09**: Writer can split DDS textures into compatible mip/chunk records with configurable chunk limits.
- [x] **WBA2-10**: Writer can apply per-chunk compression and serialize chunk metadata so extracted DDS output remains valid.
- [x] **WBA2-11**: Maintainer can round-trip BA2 writer output by packing, reopening, extracting, and byte-comparing or metadata-validating source files.

### Compatibility and Validation

- [ ] **COMP-01**: Maintainer can compare extracted fixture output against BSArchPro-derived expected bytes or metadata.
- [ ] **COMP-02**: Maintainer can verify archives produced by libbsa load or validate as compatible with their target game/archive family.
- [x] **COMP-03**: Consumer can receive structured compatibility warnings for known Bethesda quirks without requiring a logging framework.
- [x] **COMP-04**: Parser can gracefully reject malformed, truncated, oversized, or internally inconsistent archives.
- [ ] **COMP-05**: Maintainer can run sanitizer-backed malformed-input tests for parser and decompressor hardening.
- [ ] **COMP-06**: Maintainer can document each non-obvious compatibility rule with reference evidence or fixture coverage.

### Performance and Concurrency

- [ ] **PERF-01**: Consumer can extract large archives using streaming I/O and bounded scratch buffers.
- [ ] **PERF-02**: Consumer can pack large archives using streaming writer flows and bounded scratch buffers.
- [ ] **PERF-03**: Consumer can opt into parallel compression during packing after single-threaded correctness is established.
- [ ] **PERF-04**: Consumer can opt into parallel decompression during bulk extraction after single-threaded correctness is established.
- [ ] **PERF-05**: Maintainer can run benchmarks comparing single-threaded and multi-threaded packing/extraction for large archives.
- [ ] **PERF-06**: Consumer can understand documented thread-safety guarantees for readers, writers, entries, callbacks, and sinks.

### Documentation and Examples

- [ ] **DOC-01**: Consumer can read Doxygen-generated public API documentation for supported archive operations.
- [ ] **DOC-02**: Consumer can follow integration examples for opening archives, listing files, extracting files, creating archives, and handling errors.
- [ ] **DOC-03**: Consumer can read target-format guidance that explains supported variants, compression methods, and known compatibility warnings.
- [x] **DOC-04**: Maintainer can run CI for build and test validation on MSVC and optionally Clang/GCC.

## v2 Requirements

Deferred to future releases. Tracked but not in current roadmap.

### Tooling

- **TOOL-01**: Consumer can use an optional sample CLI that demonstrates library APIs without becoming the primary product.
- **TOOL-02**: Consumer can use optional fuzzing harnesses as public developer tooling after strict parsers stabilize.

### Advanced Compatibility

- **ADV-01**: Consumer can request lenient recovery mode for partially corrupt archives after strict validation is complete.
- **ADV-02**: Consumer can use a stable long-term ABI policy if libbsa is published as a binary package.

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| GUI application | The product is a reusable library; consumers build their own UI. |
| First-party CLI as v1 scope | A CLI can be a later example, but it must not shape the core library API. |
| Network or URL archive access | v1 focuses on local/stream archive I/O. |
| ZIP, 7z, libarchive, or non-Bethesda formats | libbsa's value is Bethesda-specific behavior and compatibility. |
| Editing, formatting, compiling, or staging `TES5Edit/` | TES5Edit is a read-only behavioral reference submodule. |
| In-place mutation of existing archives in v1 | Write-new flows are safer until all format writers, offsets, and transactional rules are proven. |
| Public DirectXTex, libdeflate, or lz4 types | Public headers should remain portable and dependency-light. |
| External logging or formatting libraries | Consumers should decide their own logging and formatting stack. |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| FND-01 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |
| FND-02 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |
| FND-03 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |
| FND-04 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |
| FND-05 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |
| FND-06 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |
| FND-07 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |
| FMT-01 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| FMT-02 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| FMT-03 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| FMT-04 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| FMT-05 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| FMT-06 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| BIN-01 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BIN-02 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BIN-03 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BIN-04 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BIN-05 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BIN-06 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BIN-07 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BIN-08 | Phase 2: Binary I/O, Paths, Hashes, and Compression Services | Complete |
| BSA-01 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| BSA-02 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| BSA-03 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| BSA-04 | Phase 4: TES3 BSA Read/Extract | Complete |
| BSA-05 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| BSA-06 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| BSA-07 | Phase 3: Format Detection and TES4-Family BSA Read/Extract | Complete |
| BSA-08 | Phase 4: TES3 BSA Read/Extract | Complete |
| GNRL-01 | Phase 5: BA2 GNRL Read/Extract | Complete |
| GNRL-02 | Phase 5: BA2 GNRL Read/Extract | Complete |
| GNRL-03 | Phase 5: BA2 GNRL Read/Extract | Complete |
| GNRL-04 | Phase 5: BA2 GNRL Read/Extract | Complete |
| GNRL-05 | Phase 5: BA2 GNRL Read/Extract | Complete |
| GNRL-06 | Phase 5: BA2 GNRL Read/Extract | Complete |
| GNRL-07 | Phase 5: BA2 GNRL Read/Extract | Complete |
| GNRL-08 | Phase 5: BA2 GNRL Read/Extract | Complete |
| DDS-01 | Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction | Complete |
| DDS-02 | Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction | Complete |
| DDS-03 | Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction | Complete |
| DDS-04 | Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction | Complete |
| DDS-05 | Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction | Complete |
| DDS-06 | Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction | Complete |
| DDS-07 | Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction | Complete |
| WBSA-01 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-02 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-03 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-04 | Phase 10: TES3 Write Support and BSA Format Completeness | Complete |
| WBSA-05 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-06 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-07 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-08 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-09 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBSA-10 | Phase 7: TES4-Family BSA Write-New Support | Complete |
| WBA2-01 | Phase 8: BA2 GNRL Write-New Support | Complete |
| WBA2-02 | Phase 8: BA2 GNRL Write-New Support | Complete |
| WBA2-03 | Phase 8: BA2 GNRL Write-New Support | Complete |
| WBA2-04 | Phase 8: BA2 GNRL Write-New Support | Complete |
| WBA2-05 | Phase 8: BA2 GNRL Write-New Support | Complete |
| WBA2-06 | Phase 9: BA2 DX10 Write-New Support | Complete |
| WBA2-07 | Phase 9: BA2 DX10 Write-New Support | Complete |
| WBA2-08 | Phase 9: BA2 DX10 Write-New Support | Complete |
| WBA2-09 | Phase 9: BA2 DX10 Write-New Support | Complete |
| WBA2-10 | Phase 9: BA2 DX10 Write-New Support | Complete |
| WBA2-11 | Phase 9: BA2 DX10 Write-New Support | Complete |
| COMP-01 | Phase 11: Compatibility Warnings, Validation API, and Hardening | Pending |
| COMP-02 | Phase 11: Compatibility Warnings, Validation API, and Hardening | Pending |
| COMP-03 | Phase 11: Compatibility Warnings, Validation API, and Hardening | Complete |
| COMP-04 | Phase 11: Compatibility Warnings, Validation API, and Hardening | Complete |
| COMP-05 | Phase 11: Compatibility Warnings, Validation API, and Hardening | Pending |
| COMP-06 | Phase 11: Compatibility Warnings, Validation API, and Hardening | Pending |
| PERF-01 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| PERF-02 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| PERF-03 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| PERF-04 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| PERF-05 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| PERF-06 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| DOC-01 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| DOC-02 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| DOC-03 | Phase 12: Performance, Concurrency, Documentation, and Polish | Pending |
| DOC-04 | Phase 1: Foundation, API Boundary, and Test Harness | Complete |

**Coverage:**
- v1 requirements: 81 total
- Mapped to phases: 81
- Unmapped: 0

---
*Requirements defined: 2026-05-07*
*Last updated: 2026-05-07 after roadmap creation*
