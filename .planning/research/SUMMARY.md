# Project Research Summary

**Project:** libbsa
**Domain:** Portable C++20 Bethesda BSA/BA2 archive-format library
**Researched:** 2026-05-05
**Confidence:** HIGH for foundations and read-path direction; MEDIUM for BA2 DDS and cross-platform texture details until fixture validation lands

## Executive Summary

libbsa is a reusable C++20 library for reading, inspecting, extracting, and eventually writing Bethesda archive formats from Morrowind BSA through Starfield BA2. Experts build this kind of product as a compatibility library first: small public API, strict format/version detection, byte-accurate parsing, explicit path/hash semantics, internal compression/DDS adapters, and fixture-heavy validation against known tools and reference behavior. The TES5Edit/BSArchPro submodule should remain a read-only behavioral oracle, not an implementation source.

The recommended approach is to establish CMake/vcpkg/Catch2 foundations, then build vertical read slices in dependency order: shared binary I/O and error handling, path/hash services, TES4-family BSA read/extract, TES3 read, BA2 GNRL read, and BA2 DDS read. Writer work should wait until read compatibility, compression routing, hashes, sorted layout, and DDS metadata behavior are proven. Public headers should expose libbsa-owned values and errors, never libdeflate, LZ4, DirectXTex, filesystem-path, or Pascal/reference types.

The highest risks are silent compatibility failures: version-specific record layouts, TES4 compression inversion, LZ4 frame-vs-raw-block confusion, BA2 DDS header/chunk reconstruction, Bethesda path encoding/hash behavior, and accidental whole-archive buffering. Mitigate these by modeling each target archive variant explicitly, using checked binary readers/writers, separating transform adapters, designing streaming APIs first, and requiring golden fixture tests for every compatibility seam before expanding scope.

## Key Findings

### Recommended Stack

Use a conventional portable C++ library stack: C++20, CMake 3.24+, vcpkg manifest mode, libdeflate, official LZ4, DirectXTex, Catch2, and CTest. Keep dependency usage private behind adapters, commit vcpkg baselines for reproducibility, and add CI lanes gradually around Windows MSVC first, then ClangCL/Linux and sanitizer lanes.

**Core technologies:**
- **C++20:** primary implementation/API language — gives modern standard-library tools without exposing C++23-only `std::expected`.
- **CMake 3.24+ + CTest:** build, install/export, test orchestration — standard reusable C++ library distribution path.
- **vcpkg manifest mode:** dependency acquisition — reproducible libdeflate/LZ4/DirectXTex/Catch2 graph without vendoring.
- **libdeflate:** deflate payloads — fast whole-buffer codec suited to archive chunks, with wrapper/raw choice fixture-validated.
- **official LZ4:** SSE frame and Starfield raw block payloads — separate `LZ4F_*` and `LZ4_*` paths prevent corruption.
- **DirectXTex:** internal DDS metadata/header validation — required for BA2 DDS correctness, but must not leak into public headers.
- **Catch2:** fixture, round-trip, malformed, and compatibility tests — lightweight C++ test stack integrated with CTest.

### Expected Features

The feature research strongly supports a correctness-first library release: reliable open/list/lookup/metadata/extract for common PC formats before write parity or performance polish. The full product promise eventually requires writer support, but v1 should prove read compatibility and streaming extraction first.

**Must have (table stakes):**
- Archive family auto-detection from magic/version/subtype/compression method.
- Full read support for TES3, TES4-family BSA, BA2 GNRL, and BA2 DDS.
- Transparent extraction with correct deflate, LZ4 frame, and raw LZ4 block routing.
- Random-access lookup by normalized archive path and complete metadata listing.
- Streaming read/extraction APIs with convenience byte-buffer wrappers layered above them.
- BA2 DDS header reconstruction for extractable, loadable `.dds` outputs.
- Correct hashes, hash-sorted tables, archive/file flags, compression metadata, and explicit malformed-archive errors.
- Archive creation/write support for supported formats, after read semantics are validated.

**Should have (competitive):**
- Unified archive facade plus format-specific expert APIs.
- Byte-first archive path model with explicit encoding/display helpers.
- Safety-hardened disk extraction helper.
- Bulk operations with progress, cancellation, and partial-failure reporting.
- Compatibility lint/verify reports and dry-run write planning.
- Archive diff/compare primitives, optional mmap backend, and small integration examples.

**Defer (v2+):**
- Parallel extraction/packing and benchmarks until single-threaded streaming correctness is stable.
- Preserve unchanged members during rebuild after writer layout is proven.
- Content deduplication and texture metadata copy paths after basic write support.
- Fuzz/property harness expansion after core parser seams exist.
- Console-only swizzled texture/GNF and proprietary XMem support unless separately researched and explicitly scoped.

### Architecture Approach

Use a layered architecture that separates the public API, format orchestration, binary layout parsing/serialization, payload transforms, DDS services, I/O abstractions, writer planning, and tests. This avoids reproducing BSArchPro's mega-class while preserving its observed compatibility behavior through named seams and golden tests.

**Major components:**
1. **Public API:** stable C++20 reader/writer/query surface with libbsa-owned value types and structured errors.
2. **Detection registry and format codecs:** strict dispatch by BSA/BTDX magic, version, subtype, target variant, and compression method.
3. **Binary reader/writer:** checked little-endian reads/writes, bounds checking, offset helpers, and safe serialization.
4. **Archive model:** normalized entries, hashes, offsets, sizes, compression state, flags, and raw/archive path identity.
5. **Path/hash service:** TES3/TES4/FO4 hashing, separator/case rules, byte-first normalization, and display conversion boundaries.
6. **Compression transforms:** separate libdeflate, LZ4 frame, and LZ4 block adapters selected by format semantics.
7. **DDS service:** DirectXTex-backed metadata and DDS header reconstruction hidden behind libbsa-native structs.
8. **I/O abstractions:** random-access sources and streaming sinks that avoid whole-archive loading.
9. **Writer planner:** deterministic plan-then-emit layout, sorted records, offset patching, compression policy, and optional dedup.
10. **Compatibility tests:** fixture, round-trip, malformed input, and BSArchPro/official-tool comparison baselines.

### Critical Pitfalls

1. **Treating versions as cosmetic** — avoid with strict target enums and format registry keyed by magic, version, subtype, and Starfield compression method.
2. **Misreading compression flags** — implement per-format `is_compressed` logic, especially TES4 archive-default inversion and BA2 `PackedSize != 0` semantics.
3. **Confusing deflate wrappers or LZ4 frame/block formats** — use explicit transform adapters and codec-specific fixtures; never auto-probe or choose by extension.
4. **Mishandling embedded filenames** — treat `ARCHIVE_EMBEDNAME` as a payload prefix and guard SSE writer flag derivation against known crash combinations.
5. **Reconstructing BA2 DDS incorrectly** — synthesize DDS/DX10 headers from metadata/chunks and validate loadability with DirectXTex.
6. **Assuming archive paths are Unicode filesystem paths** — store byte-first archive paths and define per-format hash/normalization/display behavior.
7. **Whole-buffer “streaming” APIs** — design around sinks/positioned reads first; make full-buffer helpers documented conveniences with limits.
8. **Trusting untrusted sizes/offsets/counts** — use checked arithmetic, allocation caps, structured errors, malformed fixtures, and later fuzzing.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Project Foundation, Error Model, and Binary I/O
**Rationale:** Every format needs safe primitive reads, bounded seeks, structured errors, test scaffolding, and dependency plumbing before archive-specific code.
**Delivers:** CMake/vcpkg/Catch2 setup, public error/result shape, random-access source and output sink abstractions, checked binary reader/writer, detection skeleton, initial malformed-header tests.
**Addresses:** Stable reusable C++ API, robust error reporting, streaming API shape, archive auto-detection foundation.
**Avoids:** Whole-archive memory assumptions, untrusted offset/count crashes, ambiguous unsupported-format failures.

### Phase 2: Path, Hash, Detection Registry, and Archive Model
**Rationale:** Lookup, sorting, writer correctness, and all format codecs depend on canonical archive identity before full extraction works.
**Delivers:** Byte-first path type/normalizer, TES3/TES4/FO4 hash functions, archive entry/model metadata, strict format/version/subtype detection, golden vectors traced from BSArchPro.
**Addresses:** Random-access lookup, metadata inspection, correct hash generation, archive family auto-detection.
**Avoids:** Filesystem path leakage, Unicode/codepage guessing, wrong hash sorting, treating versions as cosmetic.

### Phase 3: TES4-Family BSA Read and Streaming Extraction
**Rationale:** Best first vertical slice: high user value and exercises detection, folder/file records, flags, compression, embedded names, lookup, and streaming without DDS complexity.
**Delivers:** Oblivion/FO3/FNV/Skyrim LE/SSE BSA read/list/lookup/extract, deflate adapter, SSE LZ4 frame adapter, compression inversion tests, embedded-name tests, compatibility fixtures.
**Addresses:** MVP TES4-family read/extract, transparent decompression, metadata/list APIs, malformed BSA handling.
**Avoids:** TES4 compression flag mistakes, LZ4 frame/block confusion, embedded-name payload corruption, known zlib tolerance shape gaps.

### Phase 4: TES3 BSA Read Slice
**Rationale:** TES3 is simpler but has distinct data-section-relative offsets and hash/name table behavior; isolating it prevents polluting TES4/BA2 semantics.
**Delivers:** Morrowind BSA detection, table parsing, path/hash lookup, streaming extraction, data-relative offset fixtures.
**Addresses:** Full historical BSA read coverage and target-specific offset semantics.
**Avoids:** Generic offset serializers and cross-format record-model shortcuts.

### Phase 5: BA2 GNRL Read Slice
**Rationale:** Modern archives introduce BTDX detection, file table offsets, FO4 hashes, Starfield headers, and raw LZ4 block routing; this should land before DDS.
**Delivers:** FO4/Starfield-compatible BA2 GNRL read/list/lookup/extract, name table parsing at `FileTableOffset`, deflate/LZ4 block routing by version/method, BA2 metadata fixtures.
**Addresses:** BA2 general-file read/extract, Starfield compression methods, random access over modern archives.
**Avoids:** BA2 `PackedSize` mistakes, Starfield version/header flattening, file-table offset assumptions, LZ4 block/frame swap.

### Phase 6: BA2 DDS Read and DDS Reconstruction
**Rationale:** Texture archives are high-value but high-risk; they depend on BA2 basics, chunk decompression, and DirectXTex-isolated metadata validation.
**Delivers:** BA2 DX10 record/chunk parsing, per-chunk extraction, DDS/DX10 header reconstruction, DirectXTex validation, mip/cubemap/DXGI fixtures.
**Addresses:** BA2 DDS extraction and texture metadata inspection.
**Avoids:** Dumping chunks as DDS files, missing mip/cubemap flags, texture extraction tests that only check non-empty output.

### Phase 7: Writer Foundations and Dry-Run Planning
**Rationale:** Writers multiply risk through sorting, offsets, flags, compression choices, file tables, chunk records, and target-game policy; plan-then-emit must be testable before byte output.
**Delivers:** Archive builder, layout planner, write options, compression policy model, target presets, sorted record plans, offset patch plan tests, dry-run diagnostics.
**Addresses:** Archive creation groundwork, per-file compression policy, dry-run validation, future round-trip tests.
**Avoids:** Insertion-order writers, unknown-version writes, late offset failures, generic serialization across incompatible formats.

### Phase 8: Writers by Format Family
**Rationale:** After planning exists, emit deterministic writers one family at a time, starting with TES4-family and then BA2 GNRL, BA2 DDS, and TES3 as fixture confidence allows.
**Delivers:** Read-after-write and independent-reference compatibility tests for TES4 BSA, BA2 GNRL, BA2 DDS, and TES3; hash/order/flag/offset golden metadata; DDS pack/extract validation.
**Addresses:** Full read/write parity, BA2 DDS write, correct flags/hashes/sorted tables.
**Avoids:** Writer self-consistency only, BA2 DDS mip chunking bugs, SSE embedded-name crash combinations, target-game ambiguity.

### Phase 9: Safe Extraction, Verification, Rebuild, and Diagnostics
**Rationale:** Once broad metadata and write behavior exist, the library can expose higher-value helpers without guessing about compatibility semantics.
**Delivers:** Safe disk extraction policy, compatibility lint reports, archive diff/compare primitives, rebuild-with-preserved-members flow, warnings for known tolerances and engine hazards.
**Addresses:** Safety-hardened extraction, verification/lint, diff/compare, preserve unchanged archive members.
**Avoids:** Path traversal, silent tolerances, app-level duplication of archive compatibility checks, premature in-place mutation.

### Phase 10: Performance, Parallelism, Hardening, and Documentation
**Rationale:** Optimize only after deterministic correctness is established; parallelism must preserve layout and stream ownership semantics.
**Delivers:** Optional mmap/positioned read backend if justified, parallel bulk extraction/packing, allocation budgets/benchmarks, fuzz harnesses, large synthetic fixtures, thread-safety docs, Doxygen/API examples.
**Addresses:** Parallel operations, scale polish, fuzz/property testing, adoption examples, public documentation.
**Avoids:** Shared-stream races, lock-around-decompression bottlenecks, decompression bombs, malformed archive crashes, undocumented buffering behavior.

### Phase Ordering Rationale

- Foundations, path/hash, and detection come first because every reader and writer depends on exact target identity and safe binary access.
- Read slices precede writer slices because generated archive correctness depends on proving parsing, compression, path, hash, flags, and DDS metadata behavior against existing archives.
- TES4-family is the first vertical slice because it exercises the most MVP value without BA2 DDS complexity.
- TES3 is kept separate despite lower complexity because its offset base semantics differ materially from TES4.
- BA2 GNRL precedes BA2 DDS because texture chunks reuse BTDX/version/name-table/compression foundations.
- Performance, parallelism, rebuild, and linting are later because they require stable metadata, streaming, and writer contracts.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 3:** TES4-family edge cases require targeted BSArchPro tracing for embedded names, zlib tolerance, compression inversion, and SSE LZ4 frame behavior.
- **Phase 5:** Starfield BA2 v2/v3/v7/v8 behavior and `CompressionMethod` handling need fixture-backed validation beyond community notes.
- **Phase 6:** BA2 DDS reconstruction needs focused DirectXTex/DDS header, DXGI, mip, array, and cubemap validation.
- **Phase 8:** Each writer subphase should research target-game writer policies, official tool output, flag derivation, hash sorting, and BA2 DDS chunking before implementation.
- **Phase 10:** Parallel extraction/packing and fuzzing need design research around thread-safety, positioned I/O, allocation budgets, and sanitizer/fuzzer tooling.

Phases with standard patterns (skip research-phase unless scope changes):
- **Phase 1:** CMake/vcpkg/Catch2, checked binary reads, CTest, and structured C++ error surfaces are well-documented.
- **Phase 2:** Hash/path implementation should trace reference code and use tests, but does not need broad external research.
- **Phase 4:** TES3 read is format-specific but comparatively narrow once path/hash/binary foundations exist.
- **Phase 7:** Plan-then-emit writer architecture is clear from architecture research; phase planning should focus on test cases, not new technology research.
- **Phase 9:** Safe extraction, diagnostics, and diff helpers use established library patterns once archive metadata is complete.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | C++20/CMake/vcpkg/libdeflate/LZ4/Catch2 choices are backed by official docs and project constraints; DirectXTex cross-platform behavior is the main MEDIUM-confidence caveat. |
| Features | HIGH/MEDIUM | Core read/extract/write requirements are PRD-aligned and high confidence; ecosystem differentiators are medium confidence; console/GNF/XMem expectations are low-confidence and should stay out of core. |
| Architecture | HIGH/MEDIUM | Layering, codec boundaries, checked I/O, and adapter seams are high confidence; exact external-library integration and DDS portability need code validation. |
| Pitfalls | HIGH/MEDIUM | Compression, path/hash, version, DDS, streaming, and malformed-input risks are strongly supported by PRD/reference/community sources; some BA2 details rely on reverse-engineered community documentation. |

**Overall confidence:** HIGH for the roadmap direction; MEDIUM for BA2 DDS writing, Starfield variants, and legacy encoding details until fixtures prove behavior.

### Gaps to Address

- **DirectXTex portability:** Validate Linux/macOS build and DDS metadata paths before promising non-Windows BA2 DDS parity.
- **Deflate wrapper behavior:** Add known TES4/FO3/FO4 fixtures to decide raw-deflate vs zlib-wrapped handling and the Fallout vanilla buffer-error tolerance mode.
- **Starfield BA2 variants:** Confirm v2/v3/v7/v8 header fields, unknown preservation, and `CompressionMethod` behavior with real fixtures before writer support.
- **Legacy archive path encoding:** Define byte-first public semantics and explicit display conversion before claiming Unicode compatibility.
- **BA2 DDS writer chunking:** Research Archive2/BSArchPro output and create mip/cubemap fixtures before implementing write emission.
- **Reference compatibility corpus:** Decide what immutable fixtures can be committed and what must remain optional/external due to licensing or size.
- **Thread-safety contract:** Specify shareable vs cloneable reader state before implementing parallel extraction or packing.

## Sources

### Primary (HIGH confidence)
- `.planning/research/STACK.md` — C++20/CMake/vcpkg dependency stack, versions, CI/tooling recommendations.
- `.planning/research/FEATURES.md` — PRD-aligned feature priorities, MVP definition, anti-features, ecosystem analysis.
- `.planning/research/ARCHITECTURE.md` — layered architecture, component boundaries, data flow, compatibility seams, suggested build order.
- `.planning/research/PITFALLS.md` — compatibility pitfalls, phase mapping, security/performance traps, recovery strategies.
- `.planning/PROJECT.md`, `docs/PRD.md`, and `AGENTS.md` — project goals, supported formats, constraints, dependency policy, TES5Edit boundary.
- TES5Edit/BSArchPro read-only references — `TES5Edit/Core/wbBSArchive.pas`, `TES5Edit/Core/wbBSA.pas`, `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`.
- Official/project documentation for CMake, vcpkg, libdeflate, LZ4, DirectXTex, Catch2, and Microsoft DDS requirements.

### Secondary (MEDIUM confidence)
- UESP Skyrim archive-format documentation — BSA flags, compression inversion, embedded names, sorting notes.
- Nexus Bethesda archive compatibility table — user-facing archive variant compatibility risks.
- STEP Archive2 guide — BA2 texture/archive settings and streaming/chunking context.
- Ryan McKenzie's `bsa`, Rust `ba2`, `dream_archive`, and `dream_archivetool` docs — ecosystem API patterns, byte-path warnings, safety helpers, writer expectations.
- BSA Browser and B.A.E. docs/changelogs — real-world browse/extract/DDS correctness expectations.

### Tertiary (LOW confidence)
- Reverse-engineered BA2 notes and archived parser projects — useful for structure hints, but require fixture validation before writer commitments.
- Console-only PS4/GNF and XMem-related expectations — intentionally excluded from core scope unless future research justifies optional support.

---
*Research completed: 2026-05-05*
*Ready for roadmap: yes*
