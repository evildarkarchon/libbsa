# Roadmap: libbsa

## Overview

libbsa v1 is delivered as a correctness-first C++20 compatibility library: establish reusable build/API foundations, prove safe streaming binary access and exact format identity, land read/extract support for each archive family, then build deterministic writers, compatibility validation, performance paths, and public documentation. TES5Edit/BSArchPro remains a read-only behavioral reference throughout; all implementation lives outside `TES5Edit/` and exposes libbsa-owned public types.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Build, Error, and Test Foundation** - Consumers and maintainers get a reusable C++20 library skeleton, structured failures, and executable tests.
- [x] **Phase 2: Streaming API, Archive Model, Detection, and Hashes** - Consumers can identify archives, inspect entries, and perform path/hash lookup through streaming-safe public APIs.
- [x] **Phase 3: Compression Services and Policy** - Archive payloads and writers use explicit deflate, LZ4 frame, and raw LZ4 block routing without codec confusion.
- [x] **Phase 4: TES4-Family BSA Read and Extract** - Consumers can open, inspect, and extract Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE BSA archives.
- [x] **Phase 5: TES3 BSA Read and Extract** - Consumers can open, inspect, and extract Morrowind BSA archives with TES3-specific offset semantics.
- [x] **Phase 6: BA2 GNRL Read and Extract** - Consumers can open, inspect, and extract Fallout 4 and Starfield general BA2 archives.
- [x] **Phase 7: BA2 DDS Read and DDS Reconstruction** - Consumers can extract Fallout 4 and Starfield texture BA2 entries as valid DDS files.
- [ ] **Phase 8: Writer Planning, Streaming Emit, and Dedup Core** - Consumers can finalize archives through deterministic streaming writer foundations with optional deduplication.
- [ ] **Phase 9: BSA Writers** - Consumers can create TES4-family and TES3 BSA archives compatible with target engines.
- [ ] **Phase 10: BA2 Writers** - Consumers can create Fallout 4 and Starfield BA2 GNRL and DDS archives.
- [ ] **Phase 11: Compatibility Validation and Hardening** - Maintainers can prove compatibility and malformed-input safety across supported archive families.
- [ ] **Phase 12: Performance and Documentation** - Consumers get bulk/parallel workflows, benchmarks, and complete API documentation/examples.

## Phase Details

### Phase 1: Build, Error, and Test Foundation
**Goal**: Consumers can build libbsa as a reusable C++20 library and maintainers can run focused tests against structured failure behavior.
**Depends on**: Nothing (first phase)
**Requirements**: FND-01, FND-02, FND-03, FND-04, FND-05, VAL-01
**Success Criteria** (what must be TRUE):
  1. Consumer can configure and build libbsa through CMake with vcpkg-managed `libdeflate`, `lz4`, `DirectXTex`, and test dependencies.
  2. Consumer can link against public libbsa headers without pulling in UI, Delphi, platform, compression, or texture-library implementation headers.
  3. Consumer receives structured errors for invalid magic, unsupported versions, truncation, impossible offsets, decompression failures, and malformed archive inputs.
  4. Maintainer can run CTest for hash, compression, header parsing, record serialization, fixture, and round-trip test targets as they are added.
  5. Maintainer can record BSArchPro/TES5Edit compatibility notes while keeping the `TES5Edit/` submodule unmodified and uncompiled.
**Plans**: 3 plans
Plans:
- [x] 01-01-PLAN.md — Create the CMake/vcpkg library scaffold and document the TES5Edit source boundary.
- [x] 01-02-PLAN.md — Implement and unit-test the C++20 result/error API.
- [x] 01-03-PLAN.md — Add public-header smoke coverage and finalize CTest label/boundary gates.

### Phase 2: Streaming API, Archive Model, Detection, and Hashes
**Goal**: Consumers can identify supported archive families, inspect archive contents, and perform path/hash lookup without whole-archive buffering.
**Depends on**: Phase 1
**Requirements**: BIO-01, BIO-02, BIO-03, BIO-04, BIO-05, DPH-01, DPH-02, DPH-03, DPH-04, DPH-05
**Success Criteria** (what must be TRUE):
  1. Consumer can open archive bytes through a random-access source abstraction and extract payloads to caller-provided streaming sinks.
  2. Consumer can use in-memory helpers for small payloads while the documented core path remains streaming-first.
  3. Consumer can auto-detect TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DDS archives from magic, version, subtype, and compression markers.
  4. Consumer can list normalized archive paths, check file existence, retrieve metadata, and inspect archive summaries without extracting data.
  5. Maintainer can validate TES3, TES4-family, and FO4/BA2 path normalization and hash behavior against golden vectors traced from the reference.
**Plans**: 5 plans
Plans:
**Wave 1**
- [x] 02-01-PLAN.md — Define public streaming source/sink contracts and memory helpers.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 02-02-PLAN.md — Add archive metadata types and strict bounded header detection.

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 02-03-PLAN.md — Implement archive-path normalization and hash golden-vector coverage.

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 02-04-PLAN.md — Build metadata-only archive view listing and lookup APIs.

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 02-05-PLAN.md — Finalize public-header smoke, documentation, and boundary gates.

Cross-cutting constraints:
- Public headers expose only libbsa-owned C++20 types and avoid dependency/platform/TES5Edit leakage.
- Archive paths use normalized libbsa-owned UTF-8 values, not host filesystem path semantics.
- TES5Edit remains read-only reference material throughout implementation and verification.

### Phase 3: Compression Services and Policy
**Goal**: Consumers and writers get correct compression/decompression behavior for each target archive variant.
**Depends on**: Phase 2
**Requirements**: CMP-01, CMP-02, CMP-03, CMP-04, CMP-05
**Success Criteria** (what must be TRUE):
  1. Consumer can transparently extract deflate-compressed TES4, FO3/FNV, Fallout 4 BA2, and Starfield deflate payloads.
  2. Consumer can transparently extract Skyrim SE/AE BSA payloads through LZ4 frame decompression.
  3. Consumer can transparently extract Starfield BA2 v3 payloads through raw LZ4 block decompression when `CompressionMethod == 3`.
  4. Maintainer can run tests proving deflate, LZ4 frame, and LZ4 block routes cannot be accidentally interchanged.
  5. Consumer can select archive-default, force-compressed, or force-raw write policy where the target format supports it.
**Plans**: 4 plans
Plans:
**Wave 1**
- [x] 03-01-PLAN.md — Define compression routing and writer policy contracts.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 03-02-PLAN.md — Implement exact-size libdeflate payload compression and decompression.

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 03-03-PLAN.md — Implement separate LZ4 frame and raw LZ4 block payload codecs.

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 03-04-PLAN.md — Finalize public smoke coverage, documentation, and boundary gates.

### Phase 4: TES4-Family BSA Read and Extract
**Goal**: Consumers can open, inspect, list, look up, and extract every TES4-family BSA variant in scope.
**Depends on**: Phase 3
**Requirements**: BSA-01, BSA-02, BSA-03, BSA-05
**Success Criteria** (what must be TRUE):
  1. Consumer can open, list, inspect, and extract Oblivion TES4 v103 BSA archives.
  2. Consumer can open, list, inspect, and extract FO3/FNV/Skyrim LE v104 BSA archives.
  3. Consumer can open, list, inspect, and extract Skyrim SE/AE v105 BSA archives.
  4. Consumer can extract entries with embedded filenames while preserving the payload bytes after the embedded-name prefix is handled.
  5. Maintainer can compare TES4-family extraction, compression flag, and embedded-name behavior against compatibility fixtures and BSArchPro output.
**Plans**: 5 plans
Plans:
**Wave 1**
- [x] 04-01-PLAN.md — Define public/private TES4-family BSA contracts and fixture builders.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 04-02-PLAN.md — Implement bounded v103/v104/v105 BSA table parsing and metadata lookup.

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 04-03-PLAN.md — Implement raw, deflate, LZ4-frame, and embedded-name extraction.

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 04-04-PLAN.md — Harden malformed input and lock compatibility edge cases.

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 04-05-PLAN.md — Finalize public smoke coverage, documentation, and boundary gates.

Cross-cutting constraints:
- TES4-family BSA support covers Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105.
- Embedded-name handling preserves payload bytes after the prefix is skipped for v104/v105 archives.
- TES5Edit remains read-only reference material throughout implementation and verification.

### Phase 5: TES3 BSA Read and Extract
**Goal**: Consumers can open, inspect, list, look up, and extract Morrowind BSA archives with TES3-specific layout behavior.
**Depends on**: Phase 4
**Requirements**: BSA-04
**Success Criteria** (what must be TRUE):
  1. Consumer can open and identify TES3 Morrowind BSA archives as distinct from TES4-family BSA files.
  2. Consumer can list and inspect TES3 entries with correct names, hashes, sizes, and data-section-relative offsets.
  3. Consumer can extract TES3 entries through the streaming sink API without corrupting payload bytes.
  4. Maintainer can verify TES3 offset and hash behavior against dedicated golden fixtures.
**Plans**: 4 plans
Plans:
**Wave 1**
- [x] 05-01-PLAN.md — Parse TES3 BSA metadata with generated fixtures and data-section-relative offset checks.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 05-02-PLAN.md — Prove TES3 raw extraction and normalized lookup through the streaming sink API.

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 05-03-PLAN.md — Harden TES3 malformed table, name/hash, and payload range handling.

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 05-04-PLAN.md — Finalize TES3 documentation, public smoke coverage, and validation gates.

Cross-cutting constraints:
- TES3 file records store offsets relative to the data section; libbsa exposes absolute payload offsets after validation.
- TES3 payloads are raw and do not use TES4-family compression flags or embedded-name prefixes.
- TES5Edit remains read-only reference material throughout implementation and verification.

### Phase 6: BA2 GNRL Read and Extract
**Goal**: Consumers can open, inspect, list, look up, and extract Fallout 4 and Starfield general BA2 archives.
**Depends on**: Phase 5
**Requirements**: BA2-01, BA2-02, BA2-03, BA2-04
**Success Criteria** (what must be TRUE):
  1. Consumer can open, list, inspect, and extract Fallout 4 BA2 GNRL archives across supported v1/v7/v8 variants.
  2. Consumer can open, list, inspect, and extract Starfield BA2 GNRL v2 archives while preserving additional header fields in metadata where relevant.
  3. Consumer can open, list, inspect, and extract Starfield BA2 GNRL v3 archives with version-specific compression-method routing.
  4. Consumer can resolve BA2 entry names from length-prefixed file name tables located at `FileTableOffset`.
  5. Maintainer can validate BA2 general metadata, hash, file-table, deflate, and raw LZ4 behavior against fixtures.
**Plans**: 7 plans
Plans:
**Wave 1**
- [x] 06-01-PLAN.md — Add the BA2 public API surface, metadata-only wrapper, CMake wiring, and public smoke coverage.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 06-02-PLAN.md — TDD the BA2 GNRL metadata parser and FileTableOffset name-table association for FO4 and Starfield v2.

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 06-03-PLAN.md — TDD raw and deflate BA2 GNRL single-entry extraction through caller-owned sinks.

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 06-04-PLAN.md — TDD Starfield v3 CompressionMethod routing, including raw LZ4-block extraction and codec-confusion failure.

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 06-05-PLAN.md — TDD malformed BA2 header, record, name-table, offset, path, and codec safety checks.

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 06-06-PLAN.md — Finalize README documentation, public smoke confirmation, and Phase 6 validation gates.

**Wave 7** *(gap closure; blocked on Wave 6 completion)*
- [x] 06-07-PLAN.md — Close BA2 malformed verification gaps for duplicate normalized names and zero-entry FileTableOffset validation.

### Phase 7: BA2 DDS Read and DDS Reconstruction
**Goal**: Consumers can extract Fallout 4 and Starfield BA2 texture entries as valid, loadable DDS files.
**Depends on**: Phase 6
**Requirements**: BA2-05, BA2-06
**Success Criteria** (what must be TRUE):
  1. Consumer can open, list, inspect, and extract Fallout 4 and Starfield BA2 DDS archives.
  2. Consumer can extract BA2 DDS texture entries with reconstructed DDS/DX10 headers, dimensions, DXGI formats, mip levels, and cubemap metadata.
  3. Consumer receives texture metadata through libbsa-owned public types without DirectXTex leaking into public headers.
  4. Maintainer can validate reconstructed DDS output as loadable and metadata-correct with DirectXTex-backed fixtures.
**Plans**: 6 plans
Plans:
**Wave 1**
- [x] 07-01-PLAN.md - Add the Phase 7 public texture metadata surface.
- [x] 07-02-PLAN.md - Add generated BA2 DDS fixture helpers and reader test target.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 07-03-PLAN.md - Parse Fallout 4 and Starfield DX10 texture metadata and chunk routing.

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 07-04-PLAN.md - Reconstruct DDS/DX10 headers and validate through the private DirectXTex helper.

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 07-05-PLAN.md - Extract BA2 DX10 texture chunks as valid DDS output.

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 07-06-PLAN.md - Harden malformed BA2 DDS inputs and finalize documentation and validation gates.

### Phase 8: Writer Planning, Streaming Emit, and Dedup Core
**Goal**: Consumers can build deterministic archive write plans and finalize archive bytes without whole-archive output buffering.
**Depends on**: Phase 7
**Requirements**: WRT-05, WRT-06, WRT-07
**Success Criteria** (what must be TRUE):
  1. Consumer can finalize new archives through a streaming output sink without requiring the entire archive image in memory.
  2. Consumer can opt into content deduplication so identical files share a data region when the target format allows it.
  3. Consumer can preview deterministic writer layout choices for offsets, tables, compression state, and data regions before bytes are emitted.
  4. Maintainer can verify written archives through read-after-write, round-trip extraction, and metadata comparison tests.
**Plans**: 6 plans
Plans:
**Wave 1**
- [x] 08-01-PLAN.md — Add the writer contract skeleton, source wiring, and writer test target.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 08-02-PLAN.md — TDD deterministic writer planning, validation, compression-policy resolution, and checked layout preview.

**Wave 3** *(blocked on Wave 2 completion)*
- [ ] 08-03-PLAN.md — TDD opt-in post-policy payload deduplication and unsupported-target failure behavior.

**Wave 4** *(blocked on Wave 3 completion)*
- [ ] 08-04-PLAN.md — TDD streaming finalization through caller-owned `byte_sink` implementations.

**Wave 5** *(blocked on Wave 4 completion)*
- [ ] 08-05-PLAN.md — TDD generated test-only harness read-after-write, metadata comparison, and round-trip extraction.

**Wave 6** *(blocked on Wave 5 completion)*
- [ ] 08-06-PLAN.md — Finalize public smoke coverage, documentation, and Phase 8 validation gates.

### Phase 9: BSA Writers
**Goal**: Consumers can create TES4-family and TES3 BSA archives compatible with their target game engines.
**Depends on**: Phase 8
**Requirements**: WRT-01, WRT-04
**Success Criteria** (what must be TRUE):
  1. Consumer can create TES4-family BSA archives for Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE from disk paths or in-memory buffers.
  2. Consumer can create TES3 Morrowind BSA archives with correct hash sorting and data-section-relative offsets.
  3. Consumer can read back libbsa-written BSA archives and retrieve matching paths, metadata, and payload bytes.
  4. Maintainer can compare BSA writer sorting, flags, hashes, offsets, compression, and embedded-name decisions against target compatibility fixtures.
**Plans**: TBD

### Phase 10: BA2 Writers
**Goal**: Consumers can create Fallout 4 and Starfield BA2 GNRL and DDS archives with correct version-specific layout and texture chunking.
**Depends on**: Phase 9
**Requirements**: WRT-02, WRT-03
**Success Criteria** (what must be TRUE):
  1. Consumer can create Fallout 4 and Starfield BA2 GNRL archives with version-specific headers, file tables, offsets, hashes, and compression metadata.
  2. Consumer can create Fallout 4 and Starfield BA2 DDS archives from DDS inputs with proper metadata analysis and mipmap chunking.
  3. Consumer can read back libbsa-written BA2 archives and retrieve matching paths, metadata, and payload bytes.
  4. Maintainer can validate BA2 DDS pack/extract behavior with mip, cubemap, DXGI format, chunking, and compression fixtures.
**Plans**: TBD

### Phase 11: Compatibility Validation and Hardening
**Goal**: Maintainers can prove supported formats match reference behavior and malformed archives fail safely.
**Depends on**: Phase 10
**Requirements**: VAL-02, VAL-03, VAL-04, VAL-05
**Success Criteria** (what must be TRUE):
  1. Maintainer can run fixture-based integration tests using committed or generated small archives for every supported archive family.
  2. Maintainer can compare extraction output against BSArchPro output for the same archive corpus without modifying the `TES5Edit/` submodule.
  3. Maintainer can verify malformed or truncated archive handling completes without crashes, unchecked allocations, or unstructured failures.
  4. Consumer receives warnings or explicit behavior for known Bethesda quirks, including compressed sounds, SSE `EMBEDNAME` hazards, and Fallout vanilla zlib tolerance.
**Plans**: TBD

### Phase 12: Performance and Documentation
**Goal**: Consumers can adopt libbsa confidently for large archive workloads with documented API, threading, and integration behavior.
**Depends on**: Phase 11
**Requirements**: PERF-01, PERF-02, PERF-03, DOC-01, DOC-02
**Success Criteria** (what must be TRUE):
  1. Consumer can run bulk extraction without whole-archive buffering and with clear partial-failure reporting.
  2. Consumer can use multi-threaded packing or extraction for large archives after correctness paths are established.
  3. Maintainer can run benchmarks comparing single-threaded and multi-threaded behavior on large archive workloads.
  4. Consumer can read Doxygen API documentation covering core types, error semantics, threading/lifetime guarantees, and supported formats.
  5. Consumer can follow integration examples for open/list/extract, streaming extraction, archive creation, and error handling.
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10 → 11 → 12

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Build, Error, and Test Foundation | 3/3 | Complete | 2026-05-05 |
| 2. Streaming API, Archive Model, Detection, and Hashes | 5/5 | Complete | 2026-05-05 |
| 3. Compression Services and Policy | 4/4 | Complete | 2026-05-06 |
| 4. TES4-Family BSA Read and Extract | 5/5 | Complete | 2026-05-06 |
| 5. TES3 BSA Read and Extract | 4/4 | Complete | 2026-05-06 |
| 6. BA2 GNRL Read and Extract | 7/7 | Complete | 2026-05-06 |
| 7. BA2 DDS Read and DDS Reconstruction | 6/6 | Complete | 2026-05-06 |
| 8. Writer Planning, Streaming Emit, and Dedup Core | 1/6 | In Progress | - |
| 9. BSA Writers | 0/TBD | Not started | - |
| 10. BA2 Writers | 0/TBD | Not started | - |
| 11. Compatibility Validation and Hardening | 0/TBD | Not started | - |
| 12. Performance and Documentation | 0/TBD | Not started | - |
