# Roadmap: libbsa

## Overview

libbsa will be built as a compatibility-driven C++20 archive library: establish the reusable public boundary and safe binary services first, prove read/extract behavior for each archive family, then add write-new support on top of proven readers, followed by hardening, performance, concurrency, and final public documentation. Phases execute sequentially because format and writer correctness depends on validated lower-level behavior.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Foundation, API Boundary, and Test Harness** - Consumers and maintainers can build, include, test, and evolve libbsa without dependency or TES5Edit leakage.
- [x] **Phase 2: Binary I/O, Paths, Hashes, and Compression Services** - Parsers and writers share safe binary, virtual path, hash, streaming, and compression primitives.
- [x] **Phase 3: Format Detection and TES4-Family BSA Read/Extract** - Consumers can detect, inspect, query, and extract TES4/FO3/FNV/Skyrim LE/SSE BSA archives.
- [ ] **Phase 4: TES3 BSA Read/Extract** - Consumers can read and extract Morrowind BSA archives with TES3-specific offset semantics.
- [x] **Phase 5: BA2 GNRL Read/Extract** - Consumers can read and extract Fallout 4 and Starfield general BA2 archives.
- [x] **Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction** - Consumers can inspect and extract BA2 texture archives as valid DDS files.
- [ ] **Phase 7: TES4-Family BSA Write-New Support** - Consumers can create compatible TES4/FO3/FNV/Skyrim LE/SSE BSA archives.
- [ ] **Phase 8: BA2 GNRL Write-New Support** - Consumers can create compatible Fallout 4 and Starfield general BA2 archives.
- [ ] **Phase 9: BA2 DX10 Write-New Support** - Consumers can create compatible Fallout 4 and Starfield texture BA2 archives from DDS input.
- [ ] **Phase 10: TES3 Write Support and BSA Format Completeness** - Consumers can create Morrowind BSA archives and complete BSA read/write coverage.
- [ ] **Phase 11: Compatibility Warnings, Validation API, and Hardening** - Consumers receive structured validation results while malformed inputs are rejected safely.
- [ ] **Phase 12: Performance, Concurrency, Documentation, and Polish** - Consumers can use bounded-memory, parallel-capable workflows with documented APIs and examples.

## Phase Details

### Phase 1: Foundation, API Boundary, and Test Harness
**Goal**: Consumers and maintainers can build, include, test, and evolve libbsa as a reusable C++20 library without UI/tooling coupling, dependency leakage, or TES5Edit mutation.
**Depends on**: Nothing (first phase)
**Requirements**: FND-01, FND-02, FND-03, FND-04, FND-05, FND-06, FND-07, DOC-04
**Success Criteria** (what must be TRUE):
  1. Consumer can configure and build libbsa as either a static or shared C++20 library through CMake and vcpkg.
  2. Consumer can include public libbsa headers without transitively including libdeflate, lz4, DirectXTex, or TES5Edit headers.
  3. Consumer can call library APIs that return structured C++20-compatible result/error values without relying on mutable global state.
  4. Maintainer can run labeled Catch2/CTest suites and CI build/test validation without touching the read-only `TES5Edit/` submodule.
  5. Maintainer can add legal tiny archive fixtures in the project fixture layout without relying on copyrighted game archives in the repo.
**Plans**: 5 plans

Plans:

**Wave 1**
- [x] 01-01-PLAN.md — Create the CMake/vcpkg build and package foundation for static/shared C++20 libbsa builds.
- [x] 01-04-PLAN.md — Establish legal generated and local-only fixture layout, policy, and ignore boundaries.

**Wave 2** *(blocked on Wave 1 build foundation completion)*
- [x] 01-02-PLAN.md — Define and test the public result/error model and `archive_reader` open facade stub.

**Wave 3** *(blocked on Wave 2 public API completion)*
- [x] 01-03-PLAN.md — Wire Catch2/CTest labels and local fixture skip behavior around the public boundary tests.

**Wave 4** *(blocked on Wave 3 test harness and Wave 1 fixture policy completion)*
- [x] 01-05-PLAN.md — Add installed-package smoke validation and Windows/MSVC static/shared CI workflow.

Cross-cutting constraints:
- Public headers must not leak libdeflate, lz4, DirectXTex, Windows SDK, `std::expected`, or `TES5Edit/` references.
- `TES5Edit/` remains read-only: no editing, formatting, compiling, staging, or fixture workspace use.
- Phase 1 exposes read/open stubs only; writer shells, stream abstractions, archive parsing, binary I/O services, hashes, compression adapters, and DDS behavior remain out of scope.

### Phase 2: Binary I/O, Paths, Hashes, and Compression Services
**Goal**: Parsers and writers have reusable safe primitives for binary reads/writes, archive virtual paths, format hashes, streaming payloads, and exact-size compression routing.
**Depends on**: Phase 1
**Requirements**: BIN-01, BIN-02, BIN-03, BIN-04, BIN-05, BIN-06, BIN-07, BIN-08
**Success Criteria** (what must be TRUE):
  1. Parser can reject truncated, oversized, or malformed binary fields with structured errors instead of undefined behavior.
  2. Consumer can stream extracted payload data to caller-provided sinks without loading an entire archive into memory.
  3. Compression service can decompress deflate, LZ4 frame, and raw LZ4 block payloads only when the exact expected output size is produced.
  4. Writer support can compress deflate, LZ4 frame, and raw LZ4 block payloads by explicit target-format routing rather than extension guesses.
  5. Maintainer can verify TES3, TES4-family, and FO4/BA2 hash outputs against fixture-backed expected values.
**Plans**: 6 plans

Plans:

**Wave 1**
- [x] 02-01-PLAN.md — Build checked little-endian binary reader/writer primitives for BIN-01 and BIN-02.

**Wave 2** *(blocked on Wave 1 binary primitive foundation)*
- [x] 02-02-PLAN.md — Add internal archive path normalization and bounded synchronous payload streaming for BIN-03.

**Wave 3** *(blocked on Wave 2 internal primitive layout)*
- [x] 02-03-PLAN.md — Add private libdeflate raw deflate compression/decompression with exact-size validation.

**Wave 4** *(blocked on Wave 3 deflate adapter)*
- [x] 02-04-PLAN.md — Add private LZ4 frame/raw-block adapters and explicit compression routing.

**Wave 5** *(blocked on Wave 4 compression services)*
- [x] 02-05-PLAN.md — Add TES3, TES4-family, and FO4/BA2 hash functions plus final public-boundary verification.

Cross-cutting constraints:
- Binary I/O, path, streaming, hash, codec, and routing primitives remain internal under `libbsa::detail` and do not alter `archive_reader::open`.
- Compression adapters validate exact decompressed output size and keep libdeflate/lz4 types out of installed public headers.
- `TES5Edit/` remains read-only reference material and must have empty git status after execution.

### Phase 3: Format Detection and TES4-Family BSA Read/Extract
**Goal**: Consumers can open, detect, inspect, query, and extract TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE BSA archives with BSArchPro-compatible behavior.
**Depends on**: Phase 2
**Requirements**: FMT-01, FMT-02, FMT-03, FMT-04, FMT-05, FMT-06, BSA-01, BSA-02, BSA-03, BSA-05, BSA-06, BSA-07
**Success Criteria** (what must be TRUE):
  1. Consumer can open a TES4-family BSA and libbsa reports archive type, variant, version, flags, file count, paths, compression behavior, and per-entry metadata from archive bytes.
  2. Consumer can check archive virtual paths and locate entries using normalized path and compatible hash-based lookup semantics.
  3. Consumer can extract TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE entries whether stored raw, deflate-compressed, or LZ4-frame-compressed.
  4. Consumer can extract embedded-name entries while preserving payload bytes compatible with BSArchPro output.
  5. Maintainer can add a future archive version by extending detection/record handling without rewriting unrelated format families.
**Plans**: 6 plans

Plans:

**Wave 1**
- [x] 03-01-PLAN.md — Define public reader API contracts and test-only JSON dependency wiring.
- [x] 03-02-PLAN.md — Generate legal v103/v104/v105 success BSA fixtures and manifests.

**Wave 2** *(blocked on 03-02 success fixture generation)*
- [x] 03-03-PLAN.md — Generate malformed fixture set and manifest validation tests.

**Wave 3** *(blocked on Wave 1 contracts and Wave 2 malformed fixtures)*
- [x] 03-04-PLAN.md — Implement byte-driven detector/open state and parser skeleton.

**Wave 4** *(blocked on Wave 3 open-state skeleton; separated because it extends the same parser/open files)*
- [x] 03-05-PLAN.md — Implement TES4-family table parsing, entry metadata, deterministic listing, and normalized lookup.

**Wave 5** *(blocked on parser metadata and lookup semantics)*
- [x] 03-06-PLAN.md — Implement sink-first raw, deflate, LZ4-frame, embedded-name, bounded byte extraction, and final verification.

### Phase 4: TES3 BSA Read/Extract
**Goal**: Consumers can read, list, query, and extract TES3/Morrowind BSA archives while preserving TES3-specific data-section-relative offset behavior.
**Depends on**: Phase 3
**Requirements**: BSA-04, BSA-08
**Success Criteria** (what must be TRUE):
  1. Consumer can open a TES3/Morrowind BSA and list/query entries through the same library-owned path and metadata API shape used by later formats.
  2. Consumer can extract TES3 entries using data-section-relative offsets rather than TES4-family offset semantics.
  3. Maintainer can validate TES3 listing and extraction behavior with focused fixtures that isolate TES3 hash and offset rules.
**Plans**: 5 plans

Plans:

**Wave 1**
- [x] 04-01-PLAN.md — Create TES3 generated fixtures and RED public reader tests.

**Wave 2** *(blocked on Wave 1 fixture/test contracts)*
- [x] 04-02-PLAN.md — Rename hash metadata, document absolute offsets, and add TES3 byte detection/dispatch scaffolding.

**Wave 3** *(blocked on Wave 2 public model and detector dispatch)*
- [x] 04-03-PLAN.md — Implement TES3 parsing, strict hash/name validation, listing, and lookup.

**Wave 4** *(blocked on Wave 3 parser metadata and lookup)*
- [x] 04-04-PLAN.md — Implement TES3 raw extraction, malformed fail-closed validation, and final regression gates.

**Wave 5** *(gap closure; blocked on Wave 4 verification findings)*
- [x] 04-05-PLAN.md — Close TES3 hash-collision fixture isolation and sink-first bounded extraction gaps.

### Phase 5: BA2 GNRL Read/Extract
**Goal**: Consumers can read, inspect, and extract Fallout 4 and Starfield BA2 GNRL archives, including Starfield version-specific metadata and compression routing.
**Depends on**: Phase 4
**Requirements**: GNRL-01, GNRL-02, GNRL-03, GNRL-04, GNRL-05, GNRL-06, GNRL-07, GNRL-08
**Success Criteria** (what must be TRUE):
  1. Consumer can open Fallout 4 BA2 GNRL, Starfield BA2 v2 GNRL, and structurally valid Starfield BA2 v3 GNRL archives.
  2. Consumer can list BA2 names parsed from the length-prefixed filename table at `FileTableOffset`.
  3. Consumer can inspect raw/compressed status, sizes, offsets, compression method, and Starfield v2/v3 header fields in entry/archive metadata.
  4. Consumer can extract BA2 GNRL entries stored raw, deflate-compressed, or Starfield raw-LZ4-block-compressed when `CompressionMethod == 3`.
**Plans**: 5 plans

Plans:

**Wave 1**
- [x] 05-01-PLAN.md — Generate legal BA2 GNRL success and malformed fixtures with rich manifests.

**Wave 2** *(blocked on Wave 1 fixture generation)*
- [x] 05-02-PLAN.md — Add public BA2 metadata optionals plus byte-driven BA2 detector/open skeleton.

**Wave 3** *(blocked on Wave 2 detector and metadata contracts)*
- [x] 05-03-PLAN.md — Implement BA2 GNRL record/name parsing, deterministic listing, and normalized lookup.

**Wave 4** *(blocked on Wave 3 parsed metadata and lookup)*
- [x] 05-04-PLAN.md — Implement BA2 GNRL raw, deflate, and Starfield raw-LZ4-block extraction.

**Wave 5** *(blocked on Wave 4 extraction behavior)*
- [x] 05-05-PLAN.md — Close malformed/unsupported BA2 validation and final cross-format regression gates.

**Wave 6** *(gap closure; blocked on Wave 5 verification findings)*
- [x] 05-06-PLAN.md — Close bounded BA2 GNRL open/list filename-table read gap.

### Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction
**Goal**: Consumers can inspect BA2 DX10/DDS texture metadata and extract entries as valid DDS files while DirectXTex remains an internal validation/analyzer dependency.
**Depends on**: Phase 5
**Requirements**: DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07
**Success Criteria** (what must be TRUE):
  1. Consumer can open Fallout 4 and Starfield BA2 DX10/DDS texture archives and inspect dimensions, mip count, DXGI-derived format data, cubemap/array information, and chunk layout through libbsa-owned types.
  2. Consumer can extract BA2 DDS entries as valid DDS files with reconstructed headers and correct face/mip ordering for cubemaps.
  3. Consumer can extract BA2 DDS chunks compressed with deflate or raw LZ4 block according to archive version and chunk metadata.
  4. Maintainer can validate reconstructed DDS outputs through DirectXTex metadata loading without exposing DirectXTex types in public headers.
**Plans**: 8 plans

Plans:

**Wave 1**
- [x] 06-01-PLAN.md — Create generated BA2 DX10 fixtures, rich manifests, and CTest scaffolding for all DDS requirements.

**Wave 2** *(blocked on Wave 1 fixture/test scaffolding)*
- [x] 06-02-PLAN.md — Add dependency-light public texture metadata and the private DirectXTex analyzer boundary.

**Wave 3** *(blocked on Wave 2 metadata contract and Wave 1 fixture/test scaffolding)*
- [x] 06-03-PLAN.md — Add internal DDS DXT10 header construction and mip/array/cubemap layout validation.

**Wave 4** *(blocked on Wave 3 DDS layout validation and Wave 2 metadata contract)*
- [x] 06-04-PLAN.md — Implement BA2 DX10 open, metadata parsing, deterministic listing, and normalized lookup.

**Wave 5** *(blocked on Wave 4 parser metadata and lookup)*
- [x] 06-05-PLAN.md — Implement BA2 DX10 DDS reconstruction and raw/deflate/raw-LZ4 chunk extraction.

**Wave 6** *(blocked on Wave 5 extraction behavior)*
- [x] 06-06-PLAN.md — Close malformed DX10 hardening and final cross-format regression gates.

**Wave 7** *(gap closure; blocked on Wave 6 verification findings)*
- [x] 06-07-PLAN.md — Close bounded BA2 DX10 open/list filename parsing gap.
- [x] 06-08-PLAN.md — Close missing malformed duplicate-path and unsupported-compression coverage gaps.

### Phase 7: TES4-Family BSA Write-New Support
**Goal**: Consumers can create new TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE BSA archives from files or memory and prove them by reopening and extracting.
**Depends on**: Phase 6
**Requirements**: WBSA-01, WBSA-02, WBSA-03, WBSA-05, WBSA-06, WBSA-07, WBSA-08, WBSA-09, WBSA-10
**Success Criteria** (what must be TRUE):
  1. Consumer can create TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE BSA archives from disk files or memory buffers.
  2. Writer can generate folder/file indexes sorted by format-compatible hash order and derive archive/file flags from content.
  3. Consumer can choose archive defaults or per-file compression overrides while the writer avoids known embedded-name compatibility hazards.
  4. Consumer can optionally deduplicate identical payloads by content hash.
  5. Maintainer can pack, reopen, extract, and byte-compare TES4-family BSA writer output against source files.
**Plans**: 6 plans

Plans:

**Wave 1**
- [x] 07-01-PLAN.md — Define the dependency-light public TES4-family BSA writer contract and boundary tests.

**Wave 2** *(blocked on Wave 1 public writer contract)*
- [x] 07-02-PLAN.md — Implement writer-owned entry state, path validation, duplicate detection, and overwrite/source guards.

**Wave 3** *(blocked on Wave 2 validation foundation)*
- [x] 07-03-PLAN.md — Serialize raw v103/v104/v105 TES4-family BSA archives with hash-sorted tables and derived flags.

**Wave 4** *(blocked on Wave 3 raw serialization)*
- [x] 07-04-PLAN.md — Add target-routed deflate/LZ4-frame compression defaults and per-entry overrides.

**Wave 5** *(blocked on Wave 4 stored payload encoding)*
- [x] 07-05-PLAN.md — Add explicit target-compatible embedded-name prefix emission and round-trip proof.

**Wave 6** *(blocked on Wave 5 final stored payload shape)*
- [x] 07-06-PLAN.md — Add opt-in final-stored-byte deduplication and full Phase 7 regression gates.

### Phase 8: BA2 GNRL Write-New Support
**Goal**: Consumers can create Fallout 4 and Starfield BA2 GNRL archives with explicit target profile, version fields, filename tables, and compression policy.
**Depends on**: Phase 7
**Requirements**: WBA2-01, WBA2-02, WBA2-03, WBA2-04, WBA2-05
**Success Criteria** (what must be TRUE):
  1. Consumer can create Fallout 4 BA2 GNRL archives and Starfield BA2 GNRL archives from disk files or memory buffers.
  2. Consumer can select an explicit Starfield target version and compression method policy instead of relying on file extensions.
  3. Writer can serialize BA2 filename tables at the end of the archive and preserve or set version-specific header fields according to documented target profiles.
  4. Writer can compress BA2 GNRL entries with deflate or raw LZ4 block according to the selected format/version.
**Plans**: 6 plans

Plans:

**Wave 1**
- [x] 08-01-PLAN.md — Define the dependency-light public BA2 GNRL writer contract and boundary tests.
- [x] 08-02-PLAN.md — Update BA2 GNRL reader parsing to accept end-of-archive filename tables.

**Wave 2** *(blocked on Wave 1 public writer contract)*
- [x] 08-03-PLAN.md — Implement writer-owned entry state, validation, source ownership, and build wiring.

**Wave 3** *(blocked on Wave 1 end-table reader support and Wave 2 writer state)*
- [ ] 08-04-PLAN.md — Serialize raw FO4 v1, Starfield v2, and Starfield v3 BA2 GNRL archives with end filename tables.

**Wave 4** *(blocked on Wave 3 raw serialization)*
- [ ] 08-05-PLAN.md — Add target-routed deflate/raw-LZ4 compression defaults and per-entry overrides.

**Wave 5** *(blocked on Wave 4 compressed stored-payload shape)*
- [ ] 08-06-PLAN.md — Add opt-in final-stored-byte deduplication and full Phase 8 regression gates.

### Phase 9: BA2 DX10 Write-New Support
**Goal**: Consumers can create Fallout 4 and Starfield BA2 DX10/DDS texture archives from DDS files with valid mip/chunk metadata and extraction-preserving output.
**Depends on**: Phase 8
**Requirements**: WBA2-06, WBA2-07, WBA2-08, WBA2-09, WBA2-10, WBA2-11
**Success Criteria** (what must be TRUE):
  1. Consumer can create Fallout 4 and Starfield BA2 DX10/DDS texture archives from DDS inputs.
  2. Writer can analyze DDS input through DirectXTex internally and expose only library-owned texture metadata.
  3. Writer can split textures into compatible mip/chunk records with configurable chunk limits and per-chunk compression.
  4. Maintainer can pack, reopen, extract, and byte-compare or metadata-validate BA2 DDS writer output against source DDS files.
**Plans**: TBD

### Phase 10: TES3 Write Support and BSA Format Completeness
**Goal**: Consumers can create TES3/Morrowind BSA archives from files or memory, completing read/write support for all BSA families.
**Depends on**: Phase 9
**Requirements**: WBSA-04
**Success Criteria** (what must be TRUE):
  1. Consumer can create a TES3/Morrowind BSA archive from disk files or memory buffers.
  2. Writer can serialize TES3 file indexes and payload offsets using data-section-relative semantics.
  3. Maintainer can pack, reopen, extract, and byte-compare TES3 writer output against source files.
**Plans**: TBD

### Phase 11: Compatibility Warnings, Validation API, and Hardening
**Goal**: Consumers and maintainers can validate archives and compatibility quirks with structured warnings/errors while malformed inputs are rejected safely.
**Depends on**: Phase 10
**Requirements**: COMP-01, COMP-02, COMP-03, COMP-04, COMP-05, COMP-06
**Success Criteria** (what must be TRUE):
  1. Maintainer can compare extracted fixture output against BSArchPro-derived expected bytes or metadata.
  2. Maintainer can verify libbsa-produced archives load or validate as compatible with their target game/archive family.
  3. Consumer can receive structured compatibility warnings for known Bethesda quirks without adopting a logging framework.
  4. Parser can gracefully reject malformed, truncated, oversized, or internally inconsistent archives under sanitizer-backed tests.
  5. Maintainer can trace each non-obvious compatibility rule to reference evidence or fixture coverage.
**Plans**: TBD

### Phase 12: Performance, Concurrency, Documentation, and Polish
**Goal**: Consumers can use bounded-memory, documented, parallel-capable archive workflows, and maintainers can measure performance and publish integration guidance.
**Depends on**: Phase 11
**Requirements**: PERF-01, PERF-02, PERF-03, PERF-04, PERF-05, PERF-06, DOC-01, DOC-02, DOC-03
**Success Criteria** (what must be TRUE):
  1. Consumer can extract and pack large archives using streaming I/O and bounded scratch buffers.
  2. Consumer can opt into parallel compression during packing and parallel decompression during bulk extraction after single-threaded correctness is preserved.
  3. Maintainer can run benchmarks comparing single-threaded and multi-threaded packing/extraction for large archives.
  4. Consumer can understand documented thread-safety guarantees for readers, writers, entries, callbacks, and sinks.
  5. Consumer can read Doxygen API documentation, integration examples, and target-format guidance for supported variants, compression methods, and compatibility warnings.
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute sequentially in numeric order: 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10 → 11 → 12

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Foundation, API Boundary, and Test Harness | 5/5 | Complete | 2026-05-08 |
| 2. Binary I/O, Paths, Hashes, and Compression Services | 5/5 | Complete | 2026-05-08 |
| 3. Format Detection and TES4-Family BSA Read/Extract | 6/6 | Complete | 2026-05-08 |
| 4. TES3 BSA Read/Extract | 2/4 | In Progress|  |
| 5. BA2 GNRL Read/Extract | 5/6 | In Progress |  |
| 6. DDS Boundary and BA2 DX10 Read/Reconstruction | 8/8 | Complete | 2026-05-09 |
| 7. TES4-Family BSA Write-New Support | 0/TBD | Not started | - |
| 8. BA2 GNRL Write-New Support | 0/TBD | Not started | - |
| 9. BA2 DX10 Write-New Support | 0/TBD | Not started | - |
| 10. TES3 Write Support and BSA Format Completeness | 0/TBD | Not started | - |
| 11. Compatibility Warnings, Validation API, and Hardening | 0/TBD | Not started | - |
| 12. Performance, Concurrency, Documentation, and Polish | 0/TBD | Not started | - |
