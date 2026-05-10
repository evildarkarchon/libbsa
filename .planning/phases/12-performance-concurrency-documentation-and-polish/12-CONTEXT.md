# Phase 12: performance-concurrency-documentation-and-polish - Context

**Gathered:** 2026-05-09T23:46:54-07:00
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 12 closes the v1 performance, concurrency, benchmark, API documentation, and consumer guidance work. Consumers get bounded-memory extraction and disk-backed packing behavior, public bulk extraction, opt-in parallel packing/extraction controls with single-threaded defaults, deterministic result reporting, and documented thread-safety rules. Maintainers get legal synthetic benchmarks, Doxygen public API generation, compile-checked examples, target-format guidance, and policy tests that keep those guarantees from drifting.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**9 requirements are locked.** See `12-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `12-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Bounded-memory extraction requirements for all supported archive families.
- Bounded-memory disk-backed packing requirements for TES3, TES4-family, BA2 GNRL, and BA2 DX10 writers.
- Public opt-in worker-count controls for bulk extraction and writer packing, with single-threaded defaults.
- Deterministic result ordering and compatibility-preserving output validation for parallel workflows.
- Synthetic benchmark target, command, or report path for single-threaded versus multi-threaded packing/extraction.
- Public thread-safety documentation for readers, writers, entries, callbacks, sinks, validation, and bulk/parallel APIs.
- Doxygen or equivalent public API documentation generation for public headers.
- Consumer integration examples and target-format/compression/warning guidance.

**Out of scope (from SPEC.md):**
- Fixed process-wide memory ceiling in MiB - this phase locks structural bounds and per-entry/per-chunk limits instead of promising a platform-independent RSS number.
- CI failure on a fixed parallel speedup threshold - benchmark performance varies by host, so correctness and reportability are mandatory while speedup is measured, not threshold-gated.
- Mandatory real game archives or BSArchPro local corpora - legal synthetic fixtures and optional local fixture hooks remain the required validation baseline.
- New archive families, new compression codecs, or expanded Starfield compatibility policy - Phase 12 polishes supported v1 formats rather than adding new format scope.
- DDS resizing, transcoding, mip generation, repair, or image-data transformation - BA2 DX10 writer semantics from Phase 9 remain unchanged.
- In-place archive mutation - v1 remains write-new/publish based.
- Stable long-term binary ABI policy - tracked as a v2 requirement.
- First-party CLI product - examples may compile or run, but libbsa remains a reusable library, not a CLI application.

</spec_lock>

<decisions>
## Implementation Decisions

### Public Bulk Extraction Surface
- **D-01:** Add the public bulk extraction workflow as an `archive_reader` member, not as a separate top-level service. Expected shape is an `archive_reader::extract_entries`-style API with a small dependency-light options object.
- **D-02:** Bulk extraction accepts an explicit `worker_count` option that defaults to `1`. Single-threaded behavior remains the default; `worker_count > 1` is opt-in parallel extraction.
- **D-03:** Callers provide a per-entry sink factory or callback. The API should create or return a distinct sink per requested entry so parallel workers do not share one mutable sink.
- **D-04:** Bulk extraction returns stable per-entry result records in request/archive order, not completion order.
- **D-05:** If one entry fails, record that entry's typed error and continue extracting independent sibling entries. Do not fail the entire bulk workflow on the first per-entry error.
- **D-06:** Sink partial writes remain non-ambiguous errors. No bulk result may report success for an entry whose sink accepted only part of a chunk.

### Parallel Packing Controls
- **D-07:** Packing `worker_count` is supplied at `write_to` time through a finalization/execution options object rather than being stored in archive compatibility options.
- **D-08:** All public writers get the same write-call execution-options shape where meaningful: TES4-family BSA, TES3 BSA, BA2 GNRL, and BA2 DX10. TES3 has no compression path, so it may validate or effectively ignore parallelism while preserving the uniform public call shape.
- **D-09:** `worker_count` is an explicit positive count. `1` means serial, values greater than `1` opt into parallel work, and `0` is invalid rather than "auto".
- **D-10:** Parallel packing must preserve byte-stable output wherever earlier writer phases already required byte-stable output. Where earlier phases required semantic compatibility rather than exact bytes, parallel output must reopen, validate, and extract to equivalent metadata/payloads.
- **D-11:** Parallel packing must preserve existing safe-publish and rollback semantics. Compression, missing-source, validation, or publish failures in parallel mode must return stable `result` errors without leaving partial or corrupted destination archives.

### Bounded-Memory Proof
- **D-12:** Extraction memory proof should use behavior tests plus policy/source checks, not fragile process RSS gates.
- **D-13:** Raw extraction tests should use large legal synthetic archives and counting sinks to prove raw TES3, TES4-family, BA2 GNRL, and BA2 DX10 paths write through bounded chunks rather than whole-entry or whole-archive buffers.
- **D-14:** Compressed extraction may allocate codec-required per-entry or per-texture-chunk stored/decoded buffers. Document and test that boundary explicitly; do not require streaming codec rewrites in Phase 12.
- **D-15:** Oversized archive-controlled stored, decoded, entry, or chunk sizes must continue to fail through `libbsa::result` errors before unsafe allocation.
- **D-16:** Disk-backed writer finalization should publish through streaming temp-file writers for records and payloads. Policy checks should reject reintroducing a final whole-archive byte vector for disk-backed TES3, TES4-family, BA2 GNRL, or BA2 DX10 flows.
- **D-17:** Large disk-backed writer regression tests should cover TES3, TES4-family, BA2 GNRL, and BA2 DX10. They should prove output by reopening and extracting/validating rather than relying only on implementation inspection.
- **D-18:** Memory entries keep their existing copied-at-add-time semantics. The bounded writer guarantee focuses on disk-backed large inputs; public docs should state that `add_bytes` intentionally copies caller memory.

### Benchmarks And Documentation
- **D-19:** Add a CMake benchmark target that generates legal synthetic data, runs equivalent worker-count `1` and `>1` packing/extraction scenarios, verifies output correctness, and emits a repeatable JSON and/or Markdown report.
- **D-20:** Benchmarks are buildable and policy-checked but not run by default CI or default CTest. CI may verify the target, wiring, and correctness hooks without gating on speed or long runtime.
- **D-21:** Add a Doxygen-only API documentation target/config for public headers. Exclude private implementation details and `TES5Edit/`; validation should run when Doxygen is available without adding a runtime dependency.
- **D-22:** Add compile-checked consumer examples for opening/listing/extracting archives, bulk extraction, creating each writer family, handling `result` errors, and validation. Prefer package-consumer/docs test wiring over narrative-only snippets.
- **D-23:** Add machine-checked thread-safety guidance for public readers, writers, entry metadata, callbacks, sinks, validation APIs, benchmark helpers, and new bulk/parallel types.
- **D-24:** Add target-format guidance that covers TES3 BSA, TES4-family BSA v103/v104/v105, Fallout 4 BA2 GNRL/DX10, Starfield BA2 v2/v3 GNRL, Starfield BA2 v3 DX10, deflate, LZ4 frame, raw LZ4 block, writer target policies, and public compatibility warning codes.

### Carry-Forward Decisions
- **D-25:** Preserve dependency-light C++20 public headers: no public `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, private parser, private writer, private hash, or private compression types.
- **D-26:** Preserve generated/legal fixture policy. Mandatory evidence uses synthetic in-repo data; optional local game or BSArchPro-derived checks stay skipped by default and must not gate acceptance.
- **D-27:** Preserve `TES5Edit/` as read-only reference material. Do not edit, format, compile into libbsa, stage, or use it as a fixture workspace.
- **D-28:** Preserve existing writer and validation public-error contracts. Tests should assert stable error codes and result shapes, not exact diagnostic text.

### the agent's Discretion
- Researcher/planner may choose exact public type names for bulk extraction requests/results, sink factory signatures, and writer execution options if the API remains C++20, dependency-light, deterministic, and close to the existing reader/writer style.
- Researcher/planner may choose the internal worker implementation strategy, scheduling order, and task decomposition, provided public ordering, typed errors, no-partial-sink behavior, and safe-publish semantics are preserved.
- Researcher/planner may choose exact benchmark executable names, target names, report schema, and generated synthetic corpus sizes if correctness is checked and no fixed speedup threshold is used.
- Researcher/planner may choose exact Doxygen target/config names and documentation policy-test mechanics if only public headers are documented and private/TES5Edit surfaces remain excluded.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/12-performance-concurrency-documentation-and-polish/12-SPEC.md` - Locked Phase 12 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` - Phase 12 goal, mapped PERF-01 through PERF-06 and DOC-01 through DOC-03 requirements, success criteria, and sequential phase context.
- `.planning/REQUIREMENTS.md` - Performance/concurrency and documentation requirements plus v2 exclusions.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, key decisions, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from Phases 8 through 11.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, performance expectations, documentation expectations, and BSArchPro compatibility direction.

### Prior Phase Decisions
- `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md` - Public validation API shape, compatibility-warning surface, evidence catalog, malformed matrix, sanitizer path, and Phase 12 docs/performance boundary.
- `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md` - TES3 writer API, raw-only writer behavior, generated legal writer evidence, reader-backed validation, and memory-copy disk/source timing decisions.
- `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md` - BA2 DX10 compressed-only writer rule, DirectXTex-private boundary, chunk/dedupe behavior, DDS fixture policy, and output validation strategy.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-CONTEXT.md` - BA2 GNRL writer targets, compression routing, safe publish, dedupe, and reader-backed writer validation patterns.
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md` - TES4-family writer API shape, compression/embedded-name/dedupe behavior, safe writer output, and public dependency boundary patterns.

### Public API State
- `include/libbsa/archive.hpp` - Existing `archive_reader`, `payload_sink`, metadata types, `extract`, and `extract_bytes` surfaces to extend for bulk extraction.
- `include/libbsa/writer.hpp` - Existing public writer classes and option structs to extend with write-call execution/finalization options.
- `include/libbsa/validation.hpp` - Validation report and compatibility-warning APIs that Phase 12 examples and target-format docs must cover.
- `include/libbsa/libbsa.hpp` - Umbrella include that should expose any new public Phase 12 APIs.
- `include/libbsa/result.hpp` - Stable C++20 result/error model for bulk extraction records, writer execution errors, and examples.

### Reader And Writer Implementation State
- `src/archive.cpp` - Current reader facade, single-entry extraction dispatch, and `extract_bytes` bounded single-entry convenience implementation.
- `src/detail/payload_stream.hpp` and `src/detail/payload_stream.cpp` - Existing internal bounded source/sink transfer helper and partial-write error policy.
- `src/formats/bsa/tes3_bsa_reader.cpp` - Raw TES3 extraction chunking behavior.
- `src/formats/bsa/tes4_bsa_reader.cpp` - TES4-family raw chunking and compressed per-entry decode behavior.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - BA2 GNRL raw chunking and compressed per-entry decode behavior.
- `src/formats/ba2/ba2_dx10_reader.cpp` - BA2 DX10 DDS reconstruction, raw chunk streaming, and compressed per-chunk decode behavior.
- `src/formats/bsa/tes3_bsa_writer.cpp` - TES3 writer source preparation, current payload vector behavior, and safe publish path to convert for disk-backed bounded publishing.
- `src/formats/bsa/tes4_bsa_writer.cpp` - TES4-family writer preparation, compression, dedupe, stored payload vectors, and publish path.
- `src/formats/ba2/ba2_gnrl_writer.cpp` - BA2 GNRL writer preparation, compression routing, dedupe, and safe publish path.
- `src/formats/ba2/ba2_dx10_writer.cpp` - BA2 DX10 writer add-time DDS snapshotting, chunk compression, dedupe, and safe publish path.
- `src/formats/ba2/ba2_publish.hpp` and `src/detail/atomic_file_ops.hpp` - Existing safe publish helpers and rollback behavior that parallel packing must preserve.

### Tests, Docs, And Build Hooks
- `tests/unit/public_include_boundary_tests.cpp` - Public dependency-boundary harness that must cover new bulk/parallel/documentation-facing types without leaking private dependencies.
- `tests/package-consumer/main.cpp` and `tests/package-consumer/smoke.cmake` - Package-consumer smoke path to extend with compile-checked examples.
- `tests/unit/validation_policy_tests.cpp` - Existing policy-test style for docs, fixture policy, CI, sanitizer path, and evidence checks; likely home or model for Phase 12 documentation/benchmark policy checks.
- `tests/unit/payload_stream_tests.cpp` - Existing bounded transfer tests to reuse or mirror for extraction/packing proof.
- `tests/CMakeLists.txt` - CTest labels, fixture generation targets, package-consumer smoke wiring, and future benchmark/docs validation hooks.
- `CMakeLists.txt` - Library source/header registration plus future Doxygen and benchmark target integration.
- `CMakePresets.json` - Current static/shared/sanitizer preset layout; Phase 12 benchmark/docs presets should remain additive.
- `.github/workflows/ci.yml` - Default Windows static/shared CI behavior that benchmark and Doxygen work should not make slow or environment-fragile.
- `tests/fixtures/README.md` - Fixture provenance, label taxonomy, local fixture policy, sanitizer path docs, and place to document benchmark fixture/data policy.
- `docs/compatibility-evidence.md` - Public compatibility warning catalog that target-format guidance and examples should reference.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for archive compatibility constraints; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for BSA parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for pack/write option behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit, format, stage, compile, or use as fixture workspace.

### External Process Guidance
- `https://www.modernescpp.com/index.php/c-core-guidelines-rules-for-concurrency-and-parallelism/` - C++ concurrency guideline summary; reinforces designing library code for multi-threaded use and avoiding data races.
- `https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/` - CMake/Doxygen target integration pattern; useful for public-header documentation generation.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `archive_reader::extract(path, payload_sink&)` already provides the single-entry extraction contract that bulk extraction should build on.
- `payload_sink` and `detail::transfer_payload` already encode the partial-write error rule and bounded chunk transfer pattern.
- Raw extraction paths already stream from host files in 64 KiB chunks for TES3, TES4-family, BA2 GNRL, and BA2 DX10 raw chunks.
- Compressed extraction paths already isolate whole-block allocation to selected entries or texture chunks, matching the SPEC's accepted per-entry/per-chunk boundary.
- Public writer classes already use stable `add_file`, `add_bytes`, and `write_to` lifecycles; Phase 12 should extend that shape rather than redesigning writer construction.
- Existing safe publish helpers and writer rollback paths are the contracts parallel packing must preserve.
- `validation_policy_tests.cpp` and package-consumer smoke tests are established machine-check hooks for documentation, policy, and public API drift.

### Established Patterns
- Public APIs live under `include/libbsa/` and must be exposed through `include/libbsa/libbsa.hpp`.
- Public methods and substantially rewritten methods need Doxygen-compliant comments.
- Public headers remain C++20 and dependency-light; private compression, DirectXTex, Windows, and TES5Edit types do not leak.
- Generated legal fixtures and writer-produced archives are the mandatory evidence path; optional local corpus checks are skipped by default.
- Tests assert stable enum/error/result identifiers and observable metadata/bytes, not exact diagnostic strings.
- Benchmarks, sanitizer paths, and other host-sensitive workflows should be additive and explicit, not default CI requirements.

### Integration Points
- Extend `include/libbsa/archive.hpp` with bulk extraction request/options/result types and an `archive_reader` member API.
- Extend `include/libbsa/writer.hpp` with shared write-call execution/finalization options and overloads for every public writer.
- Refactor writer internals so disk-backed large inputs and final publishing use streaming temp-file output without building one final archive byte vector.
- Add large synthetic raw/compressed extraction and packing tests across TES3, TES4-family, BA2 GNRL, and BA2 DX10.
- Add benchmark sources/targets, report generation, and policy checks without adding speed thresholds or default CI runtime.
- Add Doxygen config/target, thread-safety docs, target-format guide, and compile-checked integration examples.

</code_context>

<specifics>
## Specific Ideas

- Bulk extraction should feel like a natural sibling of `archive_reader::extract`, not a separate CLI-like extraction tool.
- Sink factories are the preferred boundary because caller-owned sinks/callbacks may not be thread-safe when shared across workers.
- `worker_count = 0` should be invalid, not auto, to avoid accidental oversubscription in applications that embed libbsa.
- Benchmark output should be useful to humans and automation: JSON for tools, Markdown for quick review.
- Doxygen is enough for v1 API docs; Sphinx/Breathe or a sample CLI would be future polish, not Phase 12 scope.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 12-performance-concurrency-documentation-and-polish*
*Context gathered: 2026-05-09T23:46:54-07:00*
