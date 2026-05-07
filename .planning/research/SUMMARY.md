# Project Research Summary

**Project:** libbsa  
**Domain:** reusable C++20 Bethesda BSA/BA2 archive reader/writer library  
**Researched:** 2026-05-07  
**Confidence:** HIGH overall; MEDIUM for undocumented Bethesda/Starfield edge cases until fixtures validate them

## Executive Summary

libbsa is an embeddable C++20 library for reading, extracting, creating, and eventually optimizing Bethesda Game Studios archive formats from Morrowind BSA through Starfield BA2. Experts build this kind of product as a compatibility-driven binary format library: small public facade, strict dependency boundaries, format-specific internal modules, explicit binary readers, streaming payload access, and a serious fixture/compatibility test matrix. It should not be a GUI, CLI-first tool, generic archive framework, or transliteration of TES5Edit/BSArchPro.

The recommended approach is read-first and boundary-first. Establish the CMake/vcpkg/Catch2 foundation, local `libbsa::result<T>` error model, archive virtual path model, checked little-endian I/O, compression adapters, and format registry before the first full parser. Then implement readers by format family, use those readers to prove compatibility, and only then add write-new builders/finalizers. Direct dependencies (`libdeflate`, official `lz4`, DirectXTex) should remain internal adapters; public headers expose libbsa-owned metadata and options only.

The main risks are compatibility corruption hidden behind superficially successful parsing: mixing BSA/BA2 variants into one generic model, confusing LZ4 frame and raw block APIs, accepting non-exact decompression, reconstructing BA2 DDS textures incorrectly, and treating archive paths like host filesystem paths. Mitigate these by keeping format modules explicit, routing behavior from parsed format/version/record metadata, validating exact sizes and offsets, using DirectXTex behind a DDS boundary, and requiring fixtures/BSArchPro comparison evidence for every non-obvious format rule.

## Key Findings

### Recommended Stack

Use C++20 with CMake and vcpkg manifest mode. C++20 satisfies the project constraint and gives modern value-oriented APIs without exposing C++23-only `std::expected`; use a project-owned result/error type instead. CMake 3.24+ with latest-CMake CI, committed vcpkg baseline, Catch2/CTest tests, and sanitizer lanes provides the right reusable-library foundation.

Runtime dependencies are justified and should be narrowly wrapped: `libdeflate` for DEFLATE payloads, official `lz4` for both SSE frame LZ4 and Starfield raw block LZ4, and DirectXTex for BA2 DDS metadata/header reconstruction. All three should link privately where possible and never leak through installed public headers.

**Core technologies:**
- **C++20:** implementation and public API — aligns with requirements while avoiding C++23 public API drift.
- **CMake 3.24+:** build, install/export, CTest orchestration — standard C++ library distribution path.
- **vcpkg manifest mode:** dependency acquisition — reproducible libdeflate/lz4/DirectXTex/Catch2 graph without vendoring.
- **TES5Edit/BSArchPro:** read-only behavioral oracle — compatibility reference only; never edit, stage, compile, or vendor it.
- **libdeflate:** DEFLATE compression/decompression — exact-size whole-buffer payload/chunk operations.
- **official lz4:** LZ4 frame and raw block handling — separate wrappers prevent SSE-vs-Starfield corruption.
- **DirectXTex:** DDS metadata and validation — internal texture boundary for BA2 DX10/DDS support.
- **Catch2/CTest + sanitizers:** fixture, round-trip, malformed, and compatibility validation.

### Expected Features

The MVP should prove the library shape and the first high-value read slice: embeddable C++20 API, byte-driven auto-detection, TES4-family BSA reading/extraction including DEFLATE and SSE LZ4 frame payloads, listing/metadata/query APIs, streaming extraction, and focused compatibility tests. Format-complete v1 then expands through TES3, BA2 GNRL, BA2 DDS read/reconstruction, and write-new support for every supported variant.

**Must have (table stakes):**
- Embeddable library-only API with structured C++20-compatible errors.
- Byte-driven archive auto-detection by magic/version/type, not extension.
- Full read support for TES3, TES4/FO3/FNV/Skyrim LE/SSE BSA, FO4 BA2, and Starfield BA2 variants.
- Listing, metadata queries, existence checks, and random-access extraction by path/entry handle.
- Streaming extraction to caller sinks with bounded memory.
- Transparent DEFLATE, LZ4 frame, and raw LZ4 block decompression.
- BA2 DDS extraction that reconstructs valid DDS files, including mip/chunk/cubemap metadata.
- Write-new archive creation, add from memory/filesystem, finalization, correct hashes/sorting, flags, and per-file compression controls.
- Byte-level and metadata-level compatibility tests against fixtures and BSArchPro-derived expectations.

**Should have (competitive):**
- Unified all-generations API with typed variant metadata.
- Compatibility mode profiles per target game/archive family.
- Structured compatibility warnings as data.
- Rich inspection/validation APIs without extraction.
- Opt-in deduplication/shared payload support.
- Streaming writer with bounded memory.
- Deterministic output mode after writers stabilize.
- Incremental progress/cancellation hooks without logging framework coupling.

**Defer (v2+):**
- Parallel packing/extraction — wait until single-threaded correctness is proven.
- Benchmark suite and advanced performance knobs — tune after stable algorithms exist.
- In-place archive mutation — requires transactional safety and offset/free-space design.
- Lenient recovery mode/fuzzing as public harness — add after strict parsers stabilize.
- CLI/demo tool — useful later, but must not shape the core API.

### Architecture Approach

Use a public facade plus internal format strategies. Public types cover readers, writers, entries, metadata, options, sources/sinks, and `result<T>`. Internals own detection, parsing, serialization, binary I/O, normalized paths, hashes, compression, DDS analysis, payload descriptors, and diagnostics. Format quirks must live in explicit modules (`tes3`, `bsa`, `ba2/gnrl`, `ba2/dx10`) rather than a generic archive model with scattered version switches.

**Major components:**
1. **Public facade** — open/create archives, list/query entries, extract to sinks, finalize writers.
2. **Format registry/detector** — reads bytes and selects concrete archive format/version.
3. **Format readers/writers** — parse and serialize each archive family with local quirks preserved.
4. **Binary I/O core** — checked little-endian reads/writes, seek validation, bounded buffers, offset arithmetic.
5. **Path/hash services** — archive virtual path normalization and format-specific hash/order rules.
6. **Compression adapters** — exact-size DEFLATE, LZ4 frame, and LZ4 raw block wrappers.
7. **Texture/DDS boundary** — DirectXTex-backed analysis, DDS header reconstruction, BA2 mip/chunk planning.
8. **Compatibility oracle process** — trace TES5Edit/BSArchPro behavior into tests and documented constraints, not source reuse.

### Critical Pitfalls

1. **Treating BSA/BA2 as one format** — avoid with concrete format modules, byte-driven detection, and per-branch fixtures.
2. **Confusing LZ4 frame with raw LZ4 blocks** — keep `lz4_frame_codec` for SSE BSA and `lz4_block_codec` for Starfield BA2 v3 method 3.
3. **Non-exact DEFLATE/LZ4 validation** — require expected output sizes, distinguish corrupt/short/insufficient outputs, and compare decompressed bytes/metadata rather than compressed blobs.
4. **Incorrect BA2 DDS reconstruction** — isolate DirectXTex metadata, rebuild headers from archive metadata, and validate extracted DDS files by loading metadata.
5. **Host filesystem path semantics for archive names** — use archive virtual paths internally and sanitize only at host I/O boundaries.
6. **Packed struct parsing and unchecked offsets** — use explicit little-endian readers and checked arithmetic for all untrusted binary content.
7. **Whole-archive memory design** — parse metadata eagerly but seek/stream payloads on demand.
8. **Writers before reader proof** — gate each writer on reader fixtures and external/BSArchPro compatibility checks.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Foundation, Public API Boundary, and Test Harness
**Rationale:** Everything depends on the C++20 API shape, dependency isolation, result/error model, archive path model, and validation harness.  
**Delivers:** CMake/vcpkg/Catch2 skeleton, public header shell, `result<T>`, error taxonomy, source/sink abstractions, format enums, fixture layout, CI labels.  
**Addresses:** embeddable API, structured errors, minimal dependency surface, compatibility fixture tests.  
**Avoids:** public dependency leakage, `std::expected` in C++20 API, TES5Edit boundary violations, weak fixtures.

### Phase 2: Binary I/O, Paths, Hashes, and Compression Adapters
**Rationale:** Safe parsing, lookup, extraction, and writer ordering all rely on these shared primitives.  
**Delivers:** checked little-endian reader/writer, offset/count guards, archive virtual path normalization, traversal rejection rules, TES3/TES4/BA2 hash units, `deflate_codec`, `lz4_frame_codec`, `lz4_block_codec`.  
**Addresses:** path existence/metadata queries, transparent decompression, hash generation/sorting prerequisites.  
**Avoids:** packed-struct parsing, path normalization corruption, LZ4 frame/block confusion, non-exact decompression.

### Phase 3: Format Detection and TES4-Family BSA Read/Extract MVP
**Rationale:** TES4-family BSA covers a broad high-value slice and exercises detection, paths/hashes, DEFLATE, SSE LZ4 frame, embedded names, metadata listing, and streaming extraction.  
**Delivers:** byte-driven detector, BSA v103-v105 parser, listing/query APIs, random-access and streaming extraction, compressed/uncompressed fixtures, embedded-name tests.  
**Addresses:** MVP read/extract capability, auto-detection, metadata queries, streaming extraction.  
**Avoids:** extension-driven behavior, whole-archive loading, incomplete exact-byte extraction tests.

### Phase 4: TES3 BSA Read Support
**Rationale:** TES3 is simpler on compression but has unique data-section-relative offsets and hash/path behavior that should be isolated early.  
**Delivers:** Morrowind BSA parser, offset-base tests, listing/extraction integration fixtures.  
**Addresses:** full BSA reader family coverage.  
**Avoids:** applying TES4 offset semantics to TES3.

### Phase 5: BA2 GNRL Read Support
**Rationale:** BA2 GNRL introduces BTDX headers, file tables, FO4/Starfield variants, `PackedSize == 0`, Starfield unknown fields, and Starfield v3 raw LZ4 routing.  
**Delivers:** BA2 common parser, GNRL records, file name table parsing, FO4/SF metadata, DEFLATE/raw LZ4 extraction fixtures.  
**Addresses:** modern general archive read support.  
**Avoids:** dropping unknown fields, treating `PackedSize == 0` as an error, misrouting Starfield compression.

### Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction
**Rationale:** Texture archives are not generic files; DDS reconstruction needs BA2 common parsing and a stable DirectXTex adapter.  
**Delivers:** `dds_analyzer`, texture metadata model, DDS header builder, mip/chunk descriptor handling, BA2 DX10 extraction with DirectXTex validation.  
**Addresses:** BA2 DDS extraction with valid DDS reconstruction and texture inspection metadata.  
**Avoids:** concatenated chunk output, hard-coded public DXGI/DirectXTex types, weak `DDS ` magic-only tests.

### Phase 7: TES4-Family BSA Write-New Support
**Rationale:** First writer should target the best-proven reader slice so round-trip and compatibility failures are diagnosable.  
**Delivers:** BSA builder/finalizer, archive/file flag derivation, per-entry compression policy, hash-sorted tables, embedded-name behavior, re-open/extract/compare tests.  
**Addresses:** new archive creation, add files, finalization, compression controls for BSA.  
**Avoids:** writer-before-reader proof, global compress-everything defaults, extension-only target inference.

### Phase 8: BA2 GNRL Write-New Support
**Rationale:** Builds on BA2 GNRL read metadata and compression adapters before DDS chunking complexity.  
**Delivers:** BA2 GNRL writer for FO4/SF profiles, file table serialization, unknown-field policy, Starfield compression method selection, optional dedup planning hook.  
**Addresses:** FO4/Starfield general archive packaging.  
**Avoids:** zeroing unknown fields, wrong compression method routing, non-deterministic table output.

### Phase 9: BA2 DX10 Write-New Support
**Rationale:** DDS writer needs the texture boundary, BA2 serializer, and read-side DDS validation to be stable.  
**Delivers:** DDS input analysis, mip/chunk planner, BA2 DX10 serializer, chunk compression, cubemap/mip fixtures, DirectXTex validation.  
**Addresses:** texture BA2 packaging workflows.  
**Avoids:** invalid DDS metadata, wrong chunk `StartMip`/`EndMip`, public DirectXTex leakage.

### Phase 10: TES3 Write Support and Format Completeness
**Rationale:** TES3 write is lower dependency than DDS but unique enough to implement after writer framework patterns are proven.  
**Delivers:** TES3 builder/finalizer, data-section-relative offset serialization, hash/order fixtures, round-trip extraction.  
**Addresses:** complete read/write coverage across target BSA/BA2 families.  
**Avoids:** reusing TES4 writer assumptions for Morrowind archives.

### Phase 11: Compatibility Warnings, Validation API, and Hardening
**Rationale:** Once formats exist, the library can expose warnings and validation over real compatibility knowledge instead of guesses.  
**Delivers:** structured warnings, validation-only APIs, malformed/truncated fixtures, sanitizer/fuzz seed expansion, optional BSArchPro comparison harness documentation.  
**Addresses:** compatibility warnings, strict malformed archive handling, fixture matrix gaps.  
**Avoids:** silent best-effort corruption, generic errors, demo-only test coverage.

### Phase 12: Performance, Determinism, Parallelism, and Polish
**Rationale:** Optimize only after single-threaded correctness and streaming invariants are proven.  
**Delivers:** deterministic output mode, benchmark suite, bounded-memory measurements, parallel extraction/packing with documented thread-safety, optional examples/docs.  
**Addresses:** competitive parallel workflows and reproducible builds.  
**Avoids:** nondeterministic writer output, race-amplified compression bugs, premature performance-driven API distortion.

### Phase Ordering Rationale

- Foundation and shared primitives come first because every parser/writer depends on error, path, hash, I/O, and compression semantics.
- Readers precede writers so every writer can be validated by re-opening, extracting, and comparing against known behavior.
- BA2 DDS read precedes BA2 DDS write because header reconstruction and mip/chunk metadata must be proven before planning chunks.
- Hardening and performance are late because they depend on complete feature surfaces, but malformed smoke tests and streaming constraints must begin in Phase 1.
- Phase groupings align with architectural boundaries: foundation/services, read families, write families, validation/hardening, optimization.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 3:** TES4/SSE BSA embedded-name behavior, exact LZ4 frame fixtures, and BSArchPro comparison expectations.
- **Phase 5:** Starfield BA2 v2/v3 unknown fields and `CompressionMethod` matrix need fixture confirmation.
- **Phase 6:** BA2 DX10 DDS reconstruction edge cases, cubemaps, mip chunk ordering, and DirectXTex portability need focused research.
- **Phase 7-10:** Writer policy for flags, sorting, compression defaults, unknown fields, and official/BSArchPro compatibility needs per-format validation.
- **Phase 11:** Warning taxonomy and malformed fixture matrix should be informed by issues found in earlier phases.
- **Phase 12:** Parallelism and deterministic compression/output need design research after baseline measurements.

Phases with standard patterns (skip research-phase unless requirements change):
- **Phase 1:** CMake/vcpkg/Catch2 skeleton and public dependency boundary are well-documented.
- **Phase 2:** Binary I/O guards, local result/error modeling, and private dependency adapters are standard C++ library patterns, though archive-specific hash tests still need implementation research.
- **Phase 4:** TES3 read is format-specific but simpler; focused reference tracing may suffice without broad external research.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Based on project constraints, official/vcpkg/package docs, Context7 lookups, and clear dependency requirements. DirectXTex non-Windows behavior remains a validation item. |
| Features | HIGH for required capabilities; MEDIUM for ecosystem differentiators | Project PRD and BSArchPro expectations strongly establish table stakes. Competitor/library expectations rely partly on public pages and package docs. |
| Architecture | HIGH for boundaries/build order; MEDIUM for Starfield details | Component split and read-first sequence are strongly supported. Starfield unknown fields and compression edge cases need fixtures. |
| Pitfalls | HIGH for compression/path/I/O/API pitfalls; MEDIUM for undocumented Bethesda quirks | Many risks come from official library APIs and binary parsing fundamentals. Game/tool-specific quirks require empirical validation. |

**Overall confidence:** HIGH for roadmap direction and phase order; MEDIUM-HIGH for exact per-format compatibility details until fixture coverage is built.

### Gaps to Address

- **Legal fixture corpus:** Need tiny generated fixtures plus optional local real-archive validation instructions; do not rely on mutable TES5Edit or copyrighted game archives in repo.
- **Starfield BA2 fields:** `Unknown1`, `Unknown2`, version differences, and v3 compression method behavior need fixture-backed policy before writers.
- **Deflate stream flavor:** Confirm raw/zlib expectations per archive family before exposing stable writer behavior.
- **BA2 DDS edge cases:** Cubemaps, arrays, small textures, mip thresholds, and chunk-count limits require focused fixtures and DirectXTex validation.
- **Path encoding policy:** Decide and document byte/UTF-8 behavior for extended-byte archive names and host extraction sanitization.
- **Compatibility warnings:** Define machine-readable warning taxonomy after enough writer/read quirks are observed.
- **Public ABI policy:** Decide whether to use pimpl/internal handles for long-term binary compatibility before publishing installed packages.

## Sources

### Primary (HIGH confidence)
- `.planning/research/STACK.md` — stack, dependency versions, CMake/vcpkg/Catch2 guidance, dependency boundaries.
- `.planning/research/FEATURES.md` — table stakes, differentiators, anti-features, MVP/v1/v2 priorities.
- `.planning/research/ARCHITECTURE.md` — component boundaries, data flows, build order, integration boundaries.
- `.planning/research/PITFALLS.md` — critical pitfalls, phase mapping, robustness/security concerns.
- Project context: `.planning/PROJECT.md`, `docs/PRD.md`, `AGENTS.md` — product constraints, TES5Edit read-only boundary, dependency requirements.
- Official/package documentation cited by stack research: CMake, vcpkg, libdeflate, lz4, DirectXTex, Catch2.

### Secondary (MEDIUM confidence)
- BSArch/BSArchPro public feature/changelog pages and DeepWiki overview — ecosystem expectations and compatibility behavior hints.
- DirectXTex documentation and release notes — DDS metadata/header reconstruction and platform notes.
- GECK, STEP Archive2, Bethesda Structs, and BA2 format notes — archive behavior and game/tool workflow context.

### Tertiary (LOW-MEDIUM confidence)
- Existing C++/Rust library/package pages (`bsa`, `ba2`, `dream_archive`) — useful ecosystem comparison and warnings, not authoritative for libbsa requirements.
- Community format notes — hypotheses to validate with fixtures, official tools, and BSArchPro behavior.

---
*Research completed: 2026-05-07*  
*Ready for roadmap: yes*
