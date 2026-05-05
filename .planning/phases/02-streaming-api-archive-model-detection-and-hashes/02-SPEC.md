# Phase 2: Streaming API, Archive Model, Detection, and Hashes - Specification

**Created:** 2026-05-05
**Ambiguity score:** 0.17 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

libbsa gains the public streaming, archive metadata, format detection, path normalization, and hash-vector foundation needed by later archive readers without implementing payload extraction or full family-specific table parsers yet.

## Background

Phase 1 created the CMake/vcpkg scaffold, `libbsa::result<T>`, `libbsa::error`, Catch2/CTest lanes, and public-header smoke coverage. The codebase still has no archive byte source abstraction, output sink abstraction, archive metadata model, format detector, path normalization API, Bethesda hash implementation, golden-vector tests, or archive parser. Phase 2 establishes these reusable contracts and compatibility primitives so later TES3, TES4-family BSA, BA2 GNRL, and BA2 DDS read phases can plug in concrete parsing and extraction behavior without redesigning the public surface.

## Requirements

1. **Random-access source contract**: Public API exposes a libbsa-owned random-access input abstraction for bounded reads at explicit offsets.
   - Current: No archive input abstraction exists; consumers cannot pass archive bytes through a libbsa-owned interface.
   - Target: Public headers define a source contract that can report size and read byte ranges without requiring whole-archive buffering.
   - Acceptance: Unit tests prove an in-memory source can read a requested byte range, reject out-of-range reads with `libbsa::error`, and avoid dependency, platform, or TES5Edit header exposure.

2. **Streaming sink contract**: Public API exposes a caller-provided output sink contract for future extraction paths, plus small in-memory helpers layered over the same contract.
   - Current: No sink abstraction exists and no future extraction API target is available.
   - Target: Public headers define a sink contract for writing byte spans and an in-memory helper suitable for small tests and convenience paths.
   - Acceptance: Unit tests prove a memory sink receives bytes through the sink contract; no archive payload extraction, decompression, file-system extraction, or source-to-sink copy utility is required in this phase.

3. **Archive metadata model**: Public API exposes libbsa-owned archive and entry metadata types that can represent supported family identity, version, subtype, flags, file counts, entry sizes, offsets, hashes, and compression state.
   - Current: No archive model exists beyond the generic result/error foundation.
   - Target: Consumers can construct and inspect metadata values without pulling in compression, DirectXTex, platform, Delphi, UI, or TES5Edit types.
   - Acceptance: Unit tests construct metadata for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DDS identities and verify version/subtype/count/offset/size/hash/compression fields are observable.

4. **Format detection from bounded header bytes**: Public API detects supported archive identities from magic bytes, version, subtype, and compression-method markers where those fields are available in the bounded header input.
   - Current: No archive detection function exists.
   - Target: Detection distinguishes TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DDS, and unsupported/malformed inputs using only small in-memory byte samples or bounded source reads.
   - Acceptance: Tests cover positive detection samples for each supported identity and negative samples for unknown magic, truncated headers, unsupported versions, and impossible marker combinations.

5. **Path normalization**: Public API exposes Bethesda archive path normalization that is independent from `std::filesystem::path` and produces stable normalized archive paths.
   - Current: No archive path type or normalization behavior exists.
   - Target: Consumers can normalize slash direction, repeated separators, and case according to libbsa's phase-locked archive-path rules while preserving archive-virtual path semantics.
   - Acceptance: Golden-vector tests prove representative TES3, TES4-family, and BA2 path strings normalize to expected archive paths, and public headers do not expose host filesystem path semantics as the archive path model.

6. **Bethesda-compatible hash primitives**: Public API or internal test-visible functions provide the path hash behavior needed for TES3, TES4-family BSA, and FO4/BA2 lookup compatibility.
   - Current: No hash implementation or vectors exist.
   - Target: Hash functions produce deterministic values for golden vectors traced from the read-only reference behavior.
   - Acceptance: Unit tests verify committed golden vectors for TES3, TES4-family folder/file path components, and FO4/BA2-relevant path hashing; vector provenance is documented without modifying `TES5Edit/`.

7. **Lookup-oriented archive view**: Consumers can list normalized paths, check existence, and retrieve entry metadata from a metadata-only archive view.
   - Current: No lookup container or metadata query API exists.
   - Target: A constructed archive view can list entries, perform normalized-path lookup, and return entry metadata without reading payload bytes.
   - Acceptance: Tests build a small in-memory metadata view with mixed path spellings, verify listing returns normalized archive paths, `contains` succeeds for equivalent input spellings, and missing paths return a structured failure or false result as appropriate.

8. **Boundary and validation hygiene**: Phase 2 extends the test foundation with focused unit tests and golden-vector tests while preserving the Phase 1 public-header and TES5Edit boundaries.
   - Current: Phase 1 has only result/error unit tests and public-header smoke coverage.
   - Target: New tests are labeled through CTest, public headers remain libbsa-owned, and compatibility notes for non-obvious hash/path behavior identify the reference source without editing or compiling the submodule.
   - Acceptance: Full CTest plus relevant labels pass; `git status --short TES5Edit` is empty; public headers contain no `libdeflate`, `LZ4`, `DirectX`, `Windows.h`, Delphi, UI, or TES5Edit includes; CMake still uses explicit source lists and no recursive globbing.

## Boundaries

**In scope:**
- Public random-access source contract for bounded reads.
- Public streaming sink contract and small in-memory helper layered over it.
- Public archive identity, summary, entry metadata, compression-state, and lookup-view types.
- Bounded header detection for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DDS identities.
- Archive-path normalization independent of host filesystem path rules.
- Bethesda-compatible hash primitives and committed golden-vector tests traced from reference behavior.
- Metadata-only listing, existence checks, and entry metadata lookup for constructed archive views.
- CTest unit/golden-vector coverage and documentation for public header and TES5Edit boundaries.

**Out of scope:**
- Full TES3, TES4-family BSA, BA2 GNRL, or BA2 DDS table parsing - later read phases own family-specific parsing.
- Payload extraction from real archives - this phase defines sink contracts only.
- Decompression through libdeflate, LZ4 frame, or raw LZ4 block APIs - Phase 3 owns codec routing.
- DDS header reconstruction, mip layout, or DirectXTex integration - BA2 DDS phases own texture behavior.
- Writer APIs, archive finalization, deduplication, or read-after-write tests - writer phases own these.
- Fixture archive corpus decisions - Phase 2 uses golden vectors and synthetic metadata tests, not full fixture archives.
- CLI, GUI, filesystem extraction policy, network access, or global singleton configuration - these remain outside the reusable library core.
- Editing, staging, formatting, compiling, or vendoring `TES5Edit/` - it remains read-only reference material.

## Constraints

- Public headers must expose libbsa-owned C++20 types only and must not leak dependency, platform, Delphi, UI, DirectXTex, or TES5Edit implementation types.
- Archive paths are virtual archive paths, not host paths; do not model them as `std::filesystem::path` in public APIs.
- The core path remains streaming-first; in-memory helpers are convenience layers for small payloads and tests.
- Detection must use bounded reads and reject truncated or malformed samples through `libbsa::result` errors rather than unchecked reads.
- Golden vectors must be committed as small text/source test data with documented provenance; do not use `TES5Edit/` as a mutable fixture.
- No new external dependencies are allowed for this phase.

## Acceptance Criteria

- [ ] A public random-access source abstraction supports size queries, bounded reads, in-memory test use, and structured out-of-range failures.
- [ ] A public sink abstraction plus small in-memory helper accepts byte writes without implementing archive extraction.
- [ ] Public metadata types represent archive identity, version, subtype, flags, file count, offsets, sizes, hashes, and compression state without dependency leakage.
- [ ] Format detection tests pass for TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DDS, unknown magic, truncated headers, unsupported versions, and invalid marker combinations.
- [ ] Path normalization golden-vector tests pass for representative TES3, TES4-family, and BA2 archive paths.
- [ ] Hash golden-vector tests pass for TES3, TES4-family BSA, and FO4/BA2-relevant path hash behavior with reference provenance documented.
- [ ] A metadata-only archive view can list normalized paths, check existence by equivalent path spelling, and return entry metadata without reading payload bytes.
- [ ] Full CTest and the relevant unit/golden-vector labels pass in the available local build environment.
- [ ] Public headers contain none of `std::filesystem::path` as the archive path model, `libdeflate`, `LZ4`, `DirectX`, `Windows.h`, Delphi, UI, or `TES5Edit` implementation includes.
- [ ] `git status --short TES5Edit` is empty and CMake still contains no recursive source globbing.

## Ambiguity Report

| Dimension          | Score | Min   | Status | Notes |
|--------------------|-------|-------|--------|-------|
| Goal Clarity       | 0.88  | 0.75  | met    | Phase delivers metadata/detection/hash foundations, not full readers. |
| Boundary Clarity   | 0.86  | 0.70  | met    | User locked metadata-only scope, golden vectors, and sink interfaces only. |
| Constraint Clarity | 0.78  | 0.65  | met    | Streaming-first, public-header isolation, no new dependencies, and TES5Edit boundary are explicit. |
| Acceptance Criteria| 0.82  | 0.70  | met    | Criteria are testable through unit/golden-vector tests and boundary gates. |
| **Ambiguity**      | 0.17  | <=0.20| met    | Gate passed after round 1. |

Status: met = dimension meets the workflow minimum.

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Should Phase 2 stop at a metadata foundation or include real table parsing for all supported families? | Metadata foundation only; parse enough headers/tables only where needed to prove lookup semantics without full extraction. |
| 1 | Researcher | What compatibility evidence should path normalization and hashes use? | Use small committed golden vectors traced from TES5Edit/BSArchPro behavior, not full fixture archives yet. |
| 1 | Researcher | Should streaming sinks be usable extraction behavior or interfaces only? | Define sink/result contracts and simple in-memory helpers; real payload extraction waits for later read phases. |

---

*Phase: 02-streaming-api-archive-model-detection-and-hashes*
*Spec created: 2026-05-05*
*Next step: /gsd-discuss-phase 2 - implementation decisions (how to build what's specified above)*
