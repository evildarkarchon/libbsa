# Phase 2: Binary I/O, Paths, Hashes, and Compression Services - Specification

**Created:** 2026-05-08
**Ambiguity score:** 0.16 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

libbsa changes from a public API skeleton with an unsupported archive-open stub into a library with tested internal binary I/O, archive path, hash, streaming, and compression primitives that later archive parser and writer phases can reuse.

## Background

Phase 1 created the reusable C++20 build, vcpkg/CMake package foundation, public `libbsa::result` and `libbsa::error` boundary, and a minimal `archive_reader::open` facade that currently returns `error_code::unsupported` for non-empty paths. The codebase has no binary reader/writer helpers, no archive virtual path representation, no hash implementation, no streaming payload contracts, and no libdeflate or LZ4 adapters despite the dependencies already being listed in `vcpkg.json`. Phase 2 is triggered by that gap: Phase 3 and later format phases need safe, fixture-tested primitives before they can parse archives, route compression, look up hashed paths, or stream payloads without whole-archive memory loading.

## Requirements

1. **Checked binary I/O primitives**: Internal binary helpers read and write little-endian archive fields with checked offset, count, and size arithmetic.
   - Current: No binary reader or writer helpers exist; archive parsing is not implemented.
   - Target: Internal utilities can read and write fixed-width little-endian integer fields and byte ranges while rejecting truncation, overflow, negative-equivalent wraparound, and out-of-range offsets through structured `libbsa::error` results.
   - Acceptance: Unit tests prove successful little-endian reads/writes for 8/16/32/64-bit values and prove malformed buffers return structured errors instead of out-of-bounds access or undefined behavior.

2. **Archive virtual path service**: libbsa has a stable archive-internal path representation and normalization service independent of host filesystem path rules.
   - Current: Public `archive_reader::open` accepts a host path string, but no archive virtual path type or normalization behavior exists.
   - Target: Internal or public-as-needed path utilities normalize Bethesda archive keys using explicit archive semantics, including separator normalization and case handling needed by later hash and lookup code, without using `std::filesystem::path` for archive-internal names.
   - Acceptance: Unit tests prove equivalent slash/backslash and case variants normalize to the same archive key where required, invalid or empty archive paths are rejected consistently, and host filesystem path APIs are not required for archive-internal comparisons.

3. **Streaming payload contracts**: Later extraction and packing code can move payload bytes through bounded sources and sinks without loading an entire archive into memory.
   - Current: No source, sink, callback, or stream abstraction exists beyond host path text accepted by the Phase 1 facade.
   - Target: A minimal payload streaming contract exists for reading bounded chunks from archive data and writing chunks to caller-provided sinks, with failure reported through `libbsa::result` or `libbsa::error` values.
   - Acceptance: Unit tests stream a payload through the contract in multiple chunks, verify exact byte output, verify sink/source failure propagation, and verify the test path does not require whole-archive buffering.

4. **Deflate codec service**: libbsa can compress and decompress raw deflate payload chunks through libdeflate with exact expected output size validation.
   - Current: `libdeflate` is declared as a vcpkg dependency, but no code links to it and no compression helpers exist.
   - Target: An internal deflate adapter compresses buffers for writer phases and decompresses buffers for parser phases, returning an error unless decompression produces exactly the expected uncompressed size.
   - Acceptance: Unit tests round-trip known deflate vectors, reject truncated or corrupt compressed input, and reject successful decompression when the produced byte count differs from the caller-provided expected size.

5. **LZ4 frame codec service**: libbsa can compress and decompress Skyrim SE/AE BSA LZ4 frame payloads through the official LZ4 frame API with exact expected output size validation.
   - Current: `lz4` is declared as a vcpkg dependency, but no LZ4 wrapper exists.
   - Target: A dedicated LZ4 frame adapter uses `LZ4F_*` APIs for frame-formatted payloads and does not share implementation paths with raw LZ4 block data.
   - Acceptance: Unit tests round-trip known LZ4 frame vectors, reject malformed frame input, and reject output whose decompressed size differs from the expected archive metadata size.

6. **Raw LZ4 block codec service**: libbsa can compress and decompress Starfield BA2 v3 raw LZ4 block payloads through the official raw block API with exact expected output size validation.
   - Current: No raw LZ4 block wrapper exists, and there is no separation between LZ4 frame and raw block formats.
   - Target: A dedicated raw block adapter uses `LZ4_*safe*` APIs for Starfield raw-block payloads and never attempts to decode raw blocks through the frame API.
   - Acceptance: Unit tests round-trip known raw LZ4 block vectors, reject malformed block input, and prove raw block data is not routed through the frame codec.

7. **Explicit compression routing**: Compression and decompression calls are selected by explicit archive-format metadata rather than file extension or payload guesses.
   - Current: No compression routing model exists.
   - Target: A small internal routing enum or equivalent service distinguishes deflate, LZ4 frame, and raw LZ4 block for both read and future write paths, including Starfield BA2 v3 `CompressionMethod == 3` raw-block routing.
   - Acceptance: Unit tests prove each explicit route reaches the expected codec, unsupported routes return structured errors, and no routing test derives the codec solely from a filename extension.

8. **Bethesda hash service**: libbsa computes TES3, TES4-family, and FO4/BA2 archive hashes with fixture-backed expected values.
   - Current: No hash implementation exists; relevant reference behavior is only available in read-only TES5Edit code such as `TES5Edit/Core/wbBSArchive.pas`.
   - Target: A libbsa-owned hash service computes the required archive-family hash values from normalized archive paths and documents non-obvious compatibility constraints near the implementation.
   - Acceptance: Unit or generated-fixture tests verify known TES3, TES4-family folder/file, and FO4/BA2 hash outputs against expected constants, and tests do not mutate, stage, compile, or use `TES5Edit/` as a fixture workspace.

## Boundaries

**In scope:**
- Internal checked little-endian binary reader/writer helpers for fixed-width archive fields and bounded byte ranges.
- Archive virtual path normalization utilities needed by future lookup and hash behavior.
- Minimal payload source/sink or equivalent streaming contract for bounded chunk transfer.
- Private libdeflate adapter for raw deflate compression and exact-size decompression.
- Private LZ4 frame adapter for Skyrim SE/AE BSA frame payloads.
- Private raw LZ4 block adapter for Starfield BA2 v3 block payloads.
- Explicit compression routing model covering deflate, LZ4 frame, and raw LZ4 block.
- TES3, TES4-family, and FO4/BA2 hash functions with expected-value tests.
- Focused unit and generated-fixture tests for primitives, malformed inputs, exact-size validation, routing, paths, and hashes.

**Out of scope:**
- Real archive detection, listing, metadata inspection, or extraction through `archive_reader` - Phase 3 and later format phases own archive-level behavior.
- Parsing complete BSA or BA2 headers and records - this phase builds shared primitives, not format parsers.
- Creating complete BSA or BA2 archives - write-new phases own archive serialization and round-trip validation.
- Extracting from real Bethesda game archives - Phase 2 proof is unit vectors and generated fixtures, not game archive extraction.
- BA2 DDS metadata analysis or DDS reconstruction - DDS phases own DirectXTex-backed texture behavior.
- Multi-threaded compression, decompression, packing, or extraction - performance and concurrency are later scope after correctness is established.
- Public CLI, GUI, logging framework, or external app tooling - libbsa remains a reusable library.
- New external dependencies beyond the approved vcpkg stack of libdeflate, official lz4, DirectXTex, and Catch2 - no additional dependency need is established for this phase.
- Mutating, formatting, compiling, staging, vendoring, or using `TES5Edit/` as a fixture workspace - it remains read-only reference material.

## Constraints

- Public headers must remain C++20-compatible and must not expose C++23 `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, or TES5Edit types.
- Compression helpers must validate exact decompressed output sizes because archive metadata supplies the expected size and silent truncation or expansion is corruption.
- LZ4 frame and raw LZ4 block implementations must remain separate because their byte formats are incompatible.
- Compression routing must be explicit from archive family/version/method metadata and must not infer behavior solely from file extensions.
- Archive virtual paths are archive keys, not host filesystem paths; host path APIs are only for I/O boundaries.
- Tests must be focused and deterministic using unit vectors or legal generated fixtures; local game archives are not required for Phase 2 acceptance.
- `TES5Edit/` may be read for reference behavior but must not be modified, formatted, compiled into libbsa, staged, or used as mutable test data.

## Acceptance Criteria

- [ ] Unit tests prove checked little-endian reads and writes for fixed-width values and malformed-buffer failures.
- [ ] Unit tests prove overflow, truncation, and out-of-range binary operations return structured errors instead of out-of-bounds access.
- [ ] Path normalization tests prove archive virtual path equivalence and rejection behavior without relying on `std::filesystem::path` for archive-internal keys.
- [ ] Streaming tests move payload bytes through the source/sink contract in bounded chunks and propagate read/write failures.
- [ ] Deflate tests round-trip known vectors and reject corrupt input or mismatched expected output sizes.
- [ ] LZ4 frame tests round-trip known vectors through `LZ4F_*` behavior and reject malformed or size-mismatched data.
- [ ] Raw LZ4 block tests round-trip known vectors through `LZ4_*safe*` behavior and reject malformed or size-mismatched data.
- [ ] Compression routing tests prove explicit deflate, LZ4 frame, and raw LZ4 block selections and reject unsupported routes.
- [ ] Hash tests verify TES3, TES4-family, and FO4/BA2 expected hash constants from legal unit vectors or generated fixtures.
- [ ] Public include-boundary tests still pass without exposing libdeflate, lz4, DirectXTex, Windows SDK, or TES5Edit headers through public libbsa headers.
- [ ] `ctest -L unit --output-on-failure` passes for the Phase 2 primitive tests.
- [ ] `TES5Edit/` has no modified, staged, or submodule-pointer changes after Phase 2 work.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.90  | 0.75  | met    | Primary deliverable locked as a reusable internal primitive layer for future parser and writer phases. |
| Boundary Clarity    | 0.80  | 0.70  | met    | Real archive extraction, format parsing, archive writing, DDS behavior, and performance concurrency are excluded. |
| Constraint Clarity  | 0.80  | 0.65  | met    | Exact-size codecs, LZ4 frame/block separation, path semantics, dependency boundary, and TES5Edit boundary are explicit. |
| Acceptance Criteria | 0.84  | 0.70  | met    | Pass/fail unit and generated-fixture checks cover each primitive family. |
| **Ambiguity**       | 0.16  | <=0.20| met    | Gate passed after round 1. |

Status: met = meets minimum, below = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Phase 1 left only `archive_reader::open`, `libbsa::result`, build/test harness, and no primitive implementations. What is the primary Phase 2 deliverable? | Phase 2 produces an internal service layer: reusable primitives for later format phases, with public headers changed only where streaming or path/result contracts require it. |
| 1 | Researcher | What current gap most directly triggers Phase 2 before Phase 3 can start? | Later parsers lack safe checked binary I/O, bounded offset arithmetic, path normalization, hash algorithms, sinks, and codec wrappers. |
| 1 | Researcher | Which verification proof matters most for this phase? | Focused unit vectors and generated-fixture tests prove exact sizes, malformed input rejection, path normalization, hash outputs, and codec round trips. |
| 1 | Gate | Ambiguity reached 0.16; proceed to SPEC.md? | User chose to write SPEC.md. |

---

*Phase: 02-binary-i-o-paths-hashes-and-compression-services*
*Spec created: 2026-05-08*
*Next step: /gsd-discuss-phase 2 - implementation decisions (how to build what's specified above)*
