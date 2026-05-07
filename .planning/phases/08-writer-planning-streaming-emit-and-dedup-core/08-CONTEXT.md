# Phase 08: writer-planning-streaming-emit-and-dedup-core - Context

**Gathered:** 2026-05-06T17:07:47.4910486-07:00
**Status:** Ready for planning

<domain>
## Phase Boundary

This phase delivers a reusable writer foundation that accepts normalized in-memory archive entries, creates deterministic and inspectable write plans, and finalizes planned archive bytes to caller-owned `byte_sink` implementations without whole-archive output buffering. It includes opt-in identical post-policy payload deduplication, compression-policy integration, structured writer failures, and generated test-only harness read-back. It does not deliver complete TES3/TES4-family BSA writers, complete BA2 GNRL/DDS writers, disk-path ingestion, DDS packing, real corpus compatibility validation, performance work, CLI/GUI behavior, in-place mutation, or any changes under `TES5Edit/`.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `08-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `08-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public or test-visible writer foundation types for in-memory entry payloads, writer options, target capabilities, write plans, layout previews, dedup references, and streaming finalization.
- Deterministic plan generation from normalized archive paths, in-memory payload bytes, compression policy, and dedup option.
- Full layout preview before emission, including table/index regions, payload/data regions, entry offsets, compression states, dedup references, and total planned size.
- Streaming finalization to caller-owned `byte_sink` implementations.
- Opt-in identical post-policy payload deduplication when the target writer capability allows shared data regions.
- Compression decision recording through existing archive-aware writer compression policy and codec dispatch.
- Generated harness archive tests proving plan-to-stream, read-after-write, round-trip extraction, metadata comparison, dedup behavior, and sink failure handling.
- Structured writer-input and finalization failure tests.

**Out of scope (from SPEC.md):**
- Complete TES3, TES4-family, FO3/FNV, or Skyrim BSA writer support - Phase 9 owns target-compatible BSA serialization, hashes, flags, embedded-name writing, and engine-specific layout.
- Complete Fallout 4 or Starfield BA2 GNRL/DDS writer support - Phase 10 owns BA2 version headers, file tables, DX10 texture chunking, and BA2 DDS packing.
- Disk-path ingestion or filesystem traversal APIs - Phase 8 acceptance uses in-memory payloads; disk source policy can be added by later writer/API phases.
- DDS input analysis, mip chunk planning, texture metadata copy paths, or DirectXTex-backed writer behavior - BA2 DDS writing belongs to Phase 10.
- Real game archive corpus comparison or BSArchPro/official-tool byte-for-byte writer validation - Phase 11 owns compatibility corpus validation after production writers exist.
- Multi-threaded packing, parallel compression, cancellation, progress reporting, and benchmarks - Phase 12 owns performance workflows after single-threaded correctness is stable.
- In-place mutation of existing archives - the project remains open-read-write-new only.
- CLI, GUI, safe disk extraction policy, overwrite handling, or directory cleanup - libbsa remains a reusable library and these are application/tooling concerns.
- Modifying, formatting, staging, compiling, or moving anything under `TES5Edit/` - the submodule remains read-only reference material.

</spec_lock>

<decisions>
## Implementation Decisions

### Writer API Shape
- **D-01:** Use a plan-then-finalize API shape. Consumers request a deterministic write plan first, then pass that plan to a finalization function with a caller-owned `byte_sink`.
- **D-02:** Prefer operation-style public functions over a stateful writer object for Phase 8, such as `plan_archive_write(...)` and `finalize_archive_write(plan, sink)`. Exact names are left to the planner, but the API must keep lifetimes explicit and align with existing `open_*` / `extract_*` operation patterns.
- **D-03:** Writer inputs should be public value entry structs containing a normalized archive path, in-memory payload bytes, and per-entry compression policy. Do not add source callbacks, disk traversal, or incremental add APIs in this phase.
- **D-04:** The public writer target should be an explicit libbsa-owned capability descriptor, not just `archive_format`. It should carry the target format identity plus the metadata needed for planning, including archive default compression behavior and whether compression and shared data regions are supported.
- **D-05:** Planning must reject invalid writer inputs before any finalization step can touch a sink. Duplicate normalized paths, invalid paths, unsupported target/options, unsupported compression/dedup combinations, and impossible size/offset arithmetic should return structured failures from planning.

### Plan Preview Model
- **D-06:** Expose write-plan preview as deterministic region and entry metadata, not as an opaque plan. The plan should include table/index regions, data/payload regions, per-entry layout records, total planned size, resolved compression states, stored sizes, dedup references, and offsets.
- **D-07:** Plan preview record order should be stable and testable through sorted vectors. Deterministic ordering is part of the consumer-visible preview contract and must not depend on caller input order.
- **D-08:** Planned offsets should be archive-absolute final stream offsets. This matches existing `entry_metadata::offset` semantics and makes read-after-write metadata comparison direct.
- **D-09:** The plan should own the post-policy packed payload bytes for each planned data region. Finalization should emit from the already-planned region bytes instead of recompressing or reprocessing payloads later, so preview metadata cannot drift from emitted bytes.
- **D-10:** Lookup conveniences over the plan are optional for later phases. Phase 8 should prioritize deterministic vectors and exact layout assertions over a larger ergonomic API surface.

### Harness Proof Shape
- **D-11:** Use a generated test-only generic harness archive format to prove Phase 8 writer-core behavior. Do not use a minimal BSA-like or BA2-like format that could be mistaken for partial production writer compatibility.
- **D-12:** Keep the harness reader/validator in tests or shared test helpers only. Do not expose a public fake archive format or public harness validation API.
- **D-13:** Harness read-after-write tests should compare both metadata and bytes: planned path order, table/data regions, data-region references, offsets, sizes, compression states, dedup references, total size, and extracted payload bytes.
- **D-14:** Harness bytes should be generated inside tests with source-reviewable builders. Do not commit binary golden harness archives unless a later phase discovers a concrete need.
- **D-15:** Harness malformed coverage should stay writer-focused: invalid writer inputs, unsupported target/options, emitted-layout validation, checked arithmetic, codec-route failures, dedup on/off behavior, and sink failure. Broad parser fuzzing or a full fake-format malformed matrix remains out of scope.
- **D-16:** The harness format should include enough structure to exercise real layout decisions: a small header, table/index region, data-region table or equivalent, and payload area. A flat path/payload list is too weak for Phase 8 layout-preview acceptance.
- **D-17:** Harness compression tests should use the real existing codec paths through `resolve_write_compression`, `compress_payload`, `resolve_payload_codec`, and `decompress_payload`. Do not use mock compression markers or raw-only coverage.
- **D-18:** Organize harness builders and assertions as reusable test helpers for Phase 9 and Phase 10 writer tests, while keeping them out of production public API.

### Dedup Semantics
- **D-19:** Dedup identity is byte-identical post-policy payload bytes. The user briefly selected original input bytes by mistake, then corrected the decision after the SPEC conflict was identified. Downstream agents must follow the SPEC-aligned post-policy rule.
- **D-20:** The plan preview must make dedup sharing explicit. Each entry should reference a deterministic data region, and shared entries should visibly point at the same data-region ID and archive-absolute offset.
- **D-21:** If dedup is requested but the target capability says shared data regions are unsupported, planning must return a structured failure. Do not silently downgrade to non-dedup output and do not add a warning-only path in this phase.
- **D-22:** Represent dedup region identity with stable deterministic numeric IDs, assigned in sorted plan order. Do not expose pointer/object references or public content hashes as the sharing identity.
- **D-23:** When dedup is disabled, duplicate payloads must produce distinct data regions and distinct offsets. This behavior should be asserted because it proves the option changes layout.
- **D-24:** Do not expose a public content hash in plan metadata for Phase 8. Hashing and collision handling should remain implementation-private; public preview exposes sharing through region IDs and offsets.
- **D-25:** Compression and stored-payload planning happen before dedup grouping. If compression policy resolution or compression fails for any entry, planning fails structurally and no partial dedup grouping is returned.
- **D-26:** Path normalization affects entry uniqueness, not dedup grouping. Duplicate normalized paths are invalid writer input; dedup grouping is based on post-policy payload bytes independent of path names.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may choose exact type names, function names, helper file names, and test organization details as long as the decisions above, `08-SPEC.md`, existing public API patterns, and project constraints are satisfied.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Locked Phase Scope
- `.planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-SPEC.md` - Locked Phase 8 requirements, boundaries, constraints, acceptance criteria, and interview decisions. MUST read before planning.
- `.planning/ROADMAP.md` - Phase ordering, Phase 8 goal, dependencies, success criteria, and writer requirement mapping.
- `.planning/REQUIREMENTS.md` - v1 requirement IDs and traceability, especially `WRT-05`, `WRT-06`, `WRT-07`, `CMP-05`, `BIO-02`, and public API boundary requirements.
- `.planning/PROJECT.md` - Project purpose, core value, public API constraints, streaming-first requirement, error/state model, and TES5Edit reference boundary.

### Prior Phase Decisions
- `.planning/phases/07-ba2-dds-read-and-dds-reconstruction/07-CONTEXT.md` - Carry-forward generated fixture style, DirectXTex/private-boundary lessons, no-partial-sink-write behavior, codec routing, and reusable test helper expectations.
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md` - Carry-forward BA2 API shape, metadata-only archive lifetimes, duplicate normalized name rejection before `archive_view`, BA2 compression semantics, and generated fixture/malformed coverage style.
- `.planning/phases/02-streaming-api-archive-model-detection-and-hashes/02-CONTEXT.md` - Carry-forward caller-owned source/sink lifetimes, copied archive views, normalized archive path API, strict detection, public API ownership, and result-returning lookup patterns.
- `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md` - Carry-forward local `libbsa::result`, public/private layout, explicit CMake source wiring, test labels, and TES5Edit boundary. Read if Phase 8 planning needs foundation details not summarized here.

### Product And Stack Context
- `docs/PRD.md` - Original writing capability expectations, including in-memory inputs, per-file compression override, deduplication, finalization after tables, and query/inspection goals.
- `.planning/research/STACK.md` - C++20/CMake/vcpkg guidance, `libbsa::result` instead of C++23 `std::expected`, public dependency-leakage constraints, libdeflate/LZ4 route separation, and writer-related stack patterns.
- `AGENTS.md` - Repository instructions, TES5Edit read-only boundary, dependency rules, comment/docstring policy, validation expectations, and GSD workflow constraints.

### Existing Code To Inspect
- `include/libbsa/io.hpp` - Existing `byte_source`, `byte_sink`, `memory_source`, and `memory_sink` caller-owned I/O contracts that writer finalization must reuse.
- `include/libbsa/compression.hpp` - Existing writer-facing `compression_policy`, `resolve_write_compression`, `compress_payload`, codec dispatcher, and no-fallback codec policy.
- `include/libbsa/archive.hpp` - Existing `archive_format`, `compression_state`, `archive_summary`, and `entry_metadata` semantics, including archive-absolute `offset` precedent.
- `include/libbsa/archive_path.hpp` - Existing normalized archive path API; writer input path validation should reuse this public boundary.
- `include/libbsa/archive_view.hpp` - Existing deterministic sorted path lookup pattern and copied metadata ownership. Useful precedent, but duplicate writer entries should be rejected before any view-like storage can overwrite them.
- `include/libbsa/bsa.hpp` - Existing operation-style BSA public API shape to mirror where appropriate.
- `include/libbsa/ba2.hpp` - Existing operation-style BA2 public API and texture metadata value-type patterns.
- `src/compression.cpp` - Existing compression/decompression dispatch path to reuse for planned packed payloads.
- `src/compression/deflate_codec.cpp` - Existing private libdeflate wrapper and exact-size validation behavior.
- `src/compression/lz4_frame_codec.cpp` - Existing private LZ4 frame wrapper for SSE-style BSA compression.
- `src/compression/lz4_block_codec.cpp` - Existing private raw LZ4 block wrapper for Starfield BA2 method-3 payloads.
- `src/bsa_reader.cpp` - Existing extraction through `byte_sink`, payload range validation, and BSA API implementation style.
- `src/ba2_reader.cpp` - Existing BA2 extraction, no-partial-write texture path, codec routing, duplicate-name rejection, and range validation style.
- `tests/public_header_smoke.cpp` - Consumer-style public header smoke coverage to extend with writer planning/finalization API and dependency leakage checks.
- `tests/compression_policy_tests.cpp` - Existing writer compression policy and codec-confusion coverage to extend for writer planning.
- `tests/ba2_reader_tests.cpp` - Generated BA2 fixture builder and malformed-input style.
- `tests/ba2_dds_reader_tests.cpp` and `tests/ba2_dds_fixture_helpers.cpp` - Generated DDS fixture helper organization and reusable test-helper precedent.
- `tests/io_tests.cpp` - `byte_sink` / `memory_sink` tests and possible custom failing sink pattern.
- `CMakeLists.txt` - Explicit source/header/test wiring; new writer headers, sources, and tests must be listed deliberately and must not include `TES5Edit/`.

### Read-Only Reference Areas
- `TES5Edit/BSArchPro.dpr` - Behavioral reference entry point for BSArchPro compatibility tracing; read-only.
- `TES5Edit/BSArch/` - Reference area for archive behavior; read-only.
- `TES5Edit/Core/wbBSArchive.pas` - Reference area for archive behavior; read-only.
- `TES5Edit/Core/wbBSA.pas` - Reference area for BSA/path/hash behavior; read-only.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `byte_sink` and `memory_sink` in `include/libbsa/io.hpp`: finalization should write through the same caller-owned sink contract used by extraction.
- `compression_policy`, `resolve_write_compression`, `compress_payload`, `resolve_payload_codec`, and `decompress_payload` in `include/libbsa/compression.hpp`: writer planning should route through these instead of adding extension-based or fallback codec inference.
- `archive_path` / `normalize_archive_path`: writer entry paths should be normalized and validated through the existing archive-virtual path API, not `std::filesystem::path`.
- `archive_format`, `compression_state`, `archive_summary`, and `entry_metadata`: existing metadata semantics provide precedent for libbsa-owned public values and archive-absolute offsets.
- `archive_view`: demonstrates deterministic sorted ordering and copied metadata ownership, but writer duplicate-path validation should happen before any map insertion can hide invalid input.
- `memory_source` and `memory_sink`: useful for generated harness read-back tests.
- Generated BA2 and BA2 DDS fixture helpers: reusable style for deterministic source-built archive bytes, positive matrices, and targeted malformed tests.

### Established Patterns
- Public APIs live under `include/libbsa/`, private implementation lives under `src/`, and public headers use Doxygen comments while exposing only libbsa-owned C++20 types.
- Operations use caller-owned source/sink lifetimes; archive objects and views own copied metadata rather than retaining byte sources or sinks.
- Parsers and extractors use bounded reads, explicit overflow/range checks, and structured `result<T>` failures instead of data-shaped exceptions.
- Codec routing is explicit by archive format, entry state, and Starfield compression method; unsupported combinations fail rather than trying fallbacks.
- Tests use Catch2, generated fixtures, CTest labels, public-header smoke coverage, and explicit CMake source/test lists.
- Existing extraction paths often build decoded output before calling the sink when no-partial-write behavior is required; Phase 8 finalization should return structured sink failures and never report partial success as success.

### Integration Points
- Add a writer public header under `include/libbsa/`, likely separate from `bsa.hpp` and `ba2.hpp`, for writer entry values, target capability descriptors, options, plan records, region records, and planning/finalization functions.
- Add private writer implementation under `src/`, with explicit `CMakeLists.txt` wiring.
- Extend `tests/public_header_smoke.cpp` so consumer-style code can build writer inputs, plan, inspect, and finalize without private dependency headers.
- Add focused writer-core tests, likely with shared test helpers for generated harness bytes and read-back validation.
- Extend compression policy/codec tests where needed so writer planning cannot confuse deflate, LZ4 frame, and raw LZ4 block routes.
- Keep harness readers/builders test-only and reusable for Phase 9/10 writer tests.
- Keep `TES5Edit/` read-only: reference tracing is allowed, but no edits, formatting, source-list inclusion, staging, commits, or submodule pointer changes.

</code_context>

<specifics>
## Specific Ideas

- The public API should feel like existing libbsa operation APIs: plan first, inspect value metadata, finalize to a caller-owned sink.
- The plan is intentionally more than a handle: downstream consumers should be able to inspect deterministic sorted entries, table/index regions, data regions, archive-absolute offsets, stored sizes, compression states, dedup sharing, and total size.
- Owning packed data regions in the plan is preferred because it makes finalization deterministic and prevents recompression drift between preview and emitted bytes.
- The generated harness should prove writer-core semantics without implying production BSA/BA2 compatibility. It should be structured enough to exercise header/table/data layout and real codec routes.
- Dedup should be visible as stable numeric data-region IDs and shared archive-absolute offsets, not as public content hashes or implementation pointers.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 08-writer-planning-streaming-emit-and-dedup-core*
*Context gathered: 2026-05-06T17:07:47.4910486-07:00*
