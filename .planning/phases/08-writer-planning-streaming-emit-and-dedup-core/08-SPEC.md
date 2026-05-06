# Phase 8: Writer Planning, Streaming Emit, and Dedup Core - Specification

**Created:** 2026-05-06
**Ambiguity score:** 0.16 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can build deterministic archive write plans, inspect layout decisions, and finalize planned archive bytes to caller-owned streaming sinks with optional identical-payload deduplication, without delivering complete BSA or BA2 writers in this phase.

## Background

The codebase has reader-focused public APIs for archive detection, metadata inspection, path lookup, extraction, codec routing, and caller-owned I/O. `include/libbsa/io.hpp` exposes `byte_source`, `byte_sink`, `memory_source`, and `memory_sink`; extraction paths in `src/bsa_reader.cpp` and `src/ba2_reader.cpp` write to caller-owned sinks. `include/libbsa/compression.hpp` already exposes writer-facing `compression_policy` and `resolve_write_compression`, and tests cover basic policy routing. No writer API, write-plan model, layout preview, byte-emitting writer core, dedup model, or read-after-write writer verification seam exists today. The roadmap assigns complete TES3/TES4-family BSA writers to Phase 9 and complete BA2 GNRL/DDS writers to Phase 10, so Phase 8 must produce reusable foundations those phases consume rather than claim target-engine archive creation.

## Requirements

1. **Writer core surface**: Phase 8 exposes a libbsa-owned writer foundation for planning and finalizing archives from in-memory entry payloads.
   - Current: Public headers expose read/extract APIs and compression policy helpers, but no archive writer, write-plan, writer options, or finalize API exists.
   - Target: Consumers can create writer inputs for a target archive family, normalized archive paths, in-memory payload bytes, compression policy, and dedup option using public libbsa-owned C++20 types.
   - Acceptance: Public-header smoke coverage compiles consumer-style code that creates writer inputs, requests a plan, inspects the plan, and finalizes to a `byte_sink` without including private codec, DirectXTex, platform, Delphi, or TES5Edit headers.

2. **Deterministic write plan**: Phase 8 produces repeatable write plans for the same inputs and options.
   - Current: Existing readers return deterministic path lists through `archive_view`, but there is no writer ordering, record layout, payload region, table region, or total-size model.
   - Target: Planning the same in-memory entries and options twice produces byte-for-byte equivalent plan metadata, independent of caller input order where archive semantics require sorted output.
   - Acceptance: Tests build the same entry set in different input orders and assert identical planned path order, table/data region layout, compression states, dedup references, payload offsets, and final planned size.

3. **Layout preview**: Phase 8 lets consumers inspect layout choices before bytes are emitted.
   - Current: Consumers can inspect parsed archive metadata after opening existing archives, but cannot preview writer offsets, table placement, payload placement, compression choices, or dedup sharing.
   - Target: A write plan exposes each entry's normalized path, source size, planned stored size, compression state, payload/data-region reference, payload offset, table/index region information, and total planned archive size.
   - Acceptance: Tests assert exact preview fields for a generated multi-entry harness archive before any call to finalize writes to a sink.

4. **Streaming finalization**: Phase 8 finalizes planned bytes to a caller-owned `byte_sink` without requiring the whole archive image to be buffered as one output object.
   - Current: `byte_sink` supports extraction output, and `memory_sink` is only a convenience sink; no writer finalization path exists.
   - Target: Finalization writes the planned archive stream through the sink contract in deterministic order and propagates structured sink failures without exposing partially successful operations as success.
   - Acceptance: Tests finalize a generated harness archive to `memory_sink` and to a custom counting/failing sink; successful output matches the planned size, and an injected sink failure returns a structured error.

5. **Optional deduplication**: Phase 8 implements opt-in identical-payload sharing when the target writer capability allows shared data regions.
   - Current: There is no content deduplication model, and every existing extraction metadata entry points at archive-owned data parsed from an input archive.
   - Target: With dedup enabled, byte-identical post-policy payloads share one planned data region and entries reference that region; with dedup disabled, duplicate inputs receive distinct data regions.
   - Acceptance: Tests with duplicate in-memory payloads assert shared region IDs and identical payload offsets when dedup is enabled, distinct region IDs and offsets when disabled, and no sharing when a target capability marks dedup unsupported.

6. **Compression policy integration**: Phase 8 routes writer compression decisions through the existing archive-aware compression policy model.
   - Current: `resolve_write_compression` and `compress_payload` exist, but no writer plan records per-entry decisions or compressed payload sizes.
   - Target: Write plans record the resolved compression state and stored-size outcome for `archive_default`, `force_compressed`, and `force_raw` policies without codec inference from file extensions.
   - Acceptance: Tests cover raw, forced-compressed, and archive-default entries for supported harness targets, assert expected compression states and stored sizes, and assert unsupported routes return structured failures rather than falling back to another codec.

7. **Generated harness read-back**: Phase 8 proves the writer core through deterministic generated harness archives, not through full production BSA/BA2 writer claims.
   - Current: Reader phases use generated fixtures for format behavior, but there is no emitted writer output and no writer read-after-write seam.
   - Target: Tests use deterministic test-only or minimal harness archive descriptors to verify plan-to-stream output, dedup references, layout metadata, read-after-write parsing, round-trip extraction, and metadata comparison without claiming complete Phase 9/10 format writer support.
   - Acceptance: A focused writer-core test emits a generated harness archive, reopens it through the harness reader or validation seam, compares metadata to the plan, extracts all entries, and verifies payload bytes match the original in-memory inputs.

8. **Writer safety and boundaries**: Phase 8 rejects invalid writer inputs and preserves established project boundaries.
   - Current: Parser and extraction paths return structured errors for malformed archives, but writer-specific invalid inputs such as duplicate normalized paths, impossible planned sizes, unsupported target capabilities, or sink failure are not modeled.
   - Target: Planning and finalization return structured errors for duplicate normalized paths, empty or unsupported target descriptors, impossible offset/size arithmetic, unsupported compression/dedup combinations, and sink write failure; `TES5Edit/` remains read-only.
   - Acceptance: Negative tests assert structured failures for duplicate paths, unsupported target/dedup/compression combinations, checked arithmetic overflow fixtures, and sink failure; `git status --short TES5Edit` reports no changes after the phase.

## Boundaries

**In scope:**
- Public or test-visible writer foundation types for in-memory entry payloads, writer options, target capabilities, write plans, layout previews, dedup references, and streaming finalization.
- Deterministic plan generation from normalized archive paths, in-memory payload bytes, compression policy, and dedup option.
- Full layout preview before emission, including table/index regions, payload/data regions, entry offsets, compression states, dedup references, and total planned size.
- Streaming finalization to caller-owned `byte_sink` implementations.
- Opt-in identical post-policy payload deduplication when the target writer capability allows shared data regions.
- Compression decision recording through existing archive-aware writer compression policy and codec dispatch.
- Generated harness archive tests proving plan-to-stream, read-after-write, round-trip extraction, metadata comparison, dedup behavior, and sink failure handling.
- Structured writer-input and finalization failure tests.

**Out of scope:**
- Complete TES3, TES4-family, FO3/FNV, or Skyrim BSA writer support - Phase 9 owns target-compatible BSA serialization, hashes, flags, embedded-name writing, and engine-specific layout.
- Complete Fallout 4 or Starfield BA2 GNRL/DDS writer support - Phase 10 owns BA2 version headers, file tables, DX10 texture chunking, and BA2 DDS packing.
- Disk-path ingestion or filesystem traversal APIs - Phase 8 acceptance uses in-memory payloads; disk source policy can be added by later writer/API phases.
- DDS input analysis, mip chunk planning, texture metadata copy paths, or DirectXTex-backed writer behavior - BA2 DDS writing belongs to Phase 10.
- Real game archive corpus comparison or BSArchPro/official-tool byte-for-byte writer validation - Phase 11 owns compatibility corpus validation after production writers exist.
- Multi-threaded packing, parallel compression, cancellation, progress reporting, and benchmarks - Phase 12 owns performance workflows after single-threaded correctness is stable.
- In-place mutation of existing archives - the project remains open-read-write-new only.
- CLI, GUI, safe disk extraction policy, overwrite handling, or directory cleanup - libbsa remains a reusable library and these are application/tooling concerns.
- Modifying, formatting, staging, compiling, or moving anything under `TES5Edit/` - the submodule remains read-only reference material.

## Constraints

- Public headers must expose only libbsa-owned C++20 types and must not leak libdeflate, LZ4, DirectXTex, Windows SDK, platform, Delphi, UI, or TES5Edit implementation details.
- Phase 8 writer inputs are in-memory payloads; whole-archive output buffering is not required for finalization and must not be the only emission path.
- `byte_sink` remains caller-owned; writer finalization must not retain sink lifetimes after the operation returns.
- Plans must be deterministic for equivalent inputs and options.
- Deduplication is opt-in and applies to byte-identical post-policy payloads only when target capabilities allow shared data regions.
- Compression decisions must use archive format, writer policy, and explicit target metadata; no codec route may be inferred from file extension alone.
- Offset and size arithmetic must be checked and return structured failures on impossible layouts.
- Generated harness archives are sufficient acceptance proof for Phase 8; production BSA/BA2 writer compatibility claims are deferred.
- `TES5Edit/` is read-only and must remain unmodified.

## Acceptance Criteria

- [ ] Public-header smoke coverage compiles writer planning and finalization consumer code using only public libbsa headers and no private dependency or TES5Edit types.
- [ ] Planning the same entries in different caller orders produces identical normalized path order, layout preview, compression states, dedup references, payload offsets, and planned total size.
- [ ] Layout preview tests assert exact table/index regions, payload/data regions, entry offsets, stored sizes, compression states, dedup references, and final planned size before finalization.
- [ ] Finalization writes a generated harness archive to `memory_sink`, and output size equals the plan's total planned size.
- [ ] Finalization through a custom failing sink returns a structured error when the sink rejects a write.
- [ ] Dedup enabled causes duplicate post-policy payloads to share one data region where the target permits it; dedup disabled emits separate regions for the same inputs.
- [ ] Unsupported target capability combinations, including unsupported dedup or compression routes, return structured failures without fallback codec guessing.
- [ ] Generated harness read-after-write tests reopen emitted bytes, compare metadata to the plan, extract all entries, and verify round-trip payload bytes match the original in-memory inputs.
- [ ] Negative tests cover duplicate normalized paths, impossible offset/size arithmetic, unsupported target descriptors, unsupported compression/dedup combinations, and sink failure.
- [ ] Focused writer-core tests, public-header smoke tests, codec tests, full CTest, public-header dependency leakage grep, CMake source-list gate, and `git status --short TES5Edit` pass.

## Ambiguity Report

| Dimension           | Score | Min    | Status | Notes |
|---------------------|-------|--------|--------|-------|
| Goal Clarity        | 0.89  | 0.75   | PASS   | Writer core, layout preview, streaming emit, and opt-in dedup are locked. |
| Boundary Clarity    | 0.82  | 0.70   | PASS   | Complete BSA/BA2 writers, disk ingestion, DDS packing, corpus validation, and performance work are explicitly excluded. |
| Constraint Clarity  | 0.80  | 0.65   | PASS   | In-memory inputs, byte_sink finalization, public-header boundaries, deterministic plans, checked arithmetic, and dedup semantics are specified. |
| Acceptance Criteria | 0.82  | 0.70   | PASS   | Pass/fail checks cover plan preview, streaming emit, dedup on/off, generated harness read-back, failures, and boundary gates. |
| **Ambiguity**       | 0.16  | <=0.20 | PASS   | Gate passed after round 2. |

Status: PASS = met minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What should Phase 8 deliver when full BSA/BA2 writers are later phases? | Deliver writer core only: shared planning, streaming emission primitives, dedup decisions, and verification hooks; no complete BSA/BA2 writer. |
| 1 | Researcher | What must consumers inspect before bytes are emitted? | Expose full layout preview: offsets, table regions, payload regions, compression state, dedup groups/references, and total planned size. |
| 1 | Researcher | What proves acceptance before full format writers exist? | Use deterministic generated harness archives to prove plan-to-stream, dedup, and read-after-write seams without claiming production BSA/BA2 writer support. |
| 2 | Researcher + Simplifier | What is the irreducible core? | Plan plus emit plus dedup, backed by generated harness read-back tests. |
| 2 | Simplifier | Which input/output shape is required? | In-memory entry payloads emitted through arbitrary `byte_sink`; disk-path ingestion waits for later writer phases. |
| 2 | Researcher | What does opt-in dedup mean here? | Byte-identical post-policy payloads share one emitted data region when enabled; disabled mode emits separate regions. |
| 2 | Gate | Ambiguity gate reached; proceed to write SPEC.md? | User selected "Yes, write SPEC.md". |

---

*Phase: 08-writer-planning-streaming-emit-and-dedup-core*
*Spec created: 2026-05-06*
*Next step: /gsd-discuss-phase 8 - implementation decisions (how to build the writer core, layout planner, streaming emitter, and dedup seams)*
