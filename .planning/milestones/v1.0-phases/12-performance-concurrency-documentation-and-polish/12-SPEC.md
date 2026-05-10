# Phase 12: Performance, Concurrency, Documentation, and Polish - Specification

**Created:** 2026-05-10
**Ambiguity score:** 0.11 (gate: <= 0.20)
**Requirements:** 9 locked

## Goal

Consumers can run bounded-memory, opt-in parallel read/write workflows for every supported archive family, and maintainers can publish benchmark evidence plus complete public API and target-format guidance.

## Background

Phase 11 completed strict validation, compatibility warnings, malformed fixture coverage, and sanitizer-oriented policy documentation. All v1 format read/write correctness requirements are now complete, so Phase 12 closes the remaining performance and documentation requirements.

The current public reader exposes single-entry extraction through `payload_sink`, and raw extraction paths stream through bounded scratch buffers. Compressed extraction still decodes a stored payload or texture chunk into a per-entry/per-chunk buffer before writing to the sink because the current codecs operate on whole compressed blocks. Writer APIs already accept disk files and memory buffers, but disk-backed writer finalization still prepares full source/stored/archive byte vectors before publishing. There is no public bulk extraction or bulk packing workflow with a worker-count option, no benchmark target/report comparing single-threaded and multi-threaded workflows, and no complete generated API documentation or integration examples.

## Requirements

1. **Bounded extraction memory**: Extraction for all supported archive families must avoid whole-archive buffering and keep scratch storage structurally bounded.
   - Current: Raw TES3, TES4-family, BA2 GNRL, and BA2 DX10 extraction streams stored bytes in fixed chunks, while compressed entries and DX10 chunks allocate decoded per-entry/per-chunk buffers before sink writes.
   - Target: All extraction paths document and enforce that no whole archive is buffered; raw paths use fixed-size scratch buffers, and compressed paths allocate only the stored/decoded entry or texture chunk buffers required by the codec route.
   - Acceptance: Automated tests or policy checks prove raw extraction transfers through bounded chunks, compressed extraction allocations are limited to the selected entry/chunk rather than archive size, and oversized archive-controlled sizes still fail through `result` errors.

2. **Bounded packing memory**: Disk-backed writer flows for all supported writer families must avoid whole-archive source buffering and use bounded scratch storage for host-file payload transfer.
   - Current: TES3, TES4-family, BA2 GNRL, and BA2 DX10 writer finalization prepares source payloads, stored payloads, or final archive bytes in owned vectors before writing the destination archive.
   - Target: Disk-backed writer paths stream source file data and publish output without holding an entire source archive or final archive image in memory; compressed payloads may use bounded per-entry or per-texture-chunk buffers when libdeflate, LZ4, or DirectXTex analysis requires whole-block data.
   - Acceptance: Writer regression tests cover disk-backed large synthetic inputs for TES3, TES4-family, BA2 GNRL, and BA2 DX10, and code/policy checks reject reintroducing final whole-archive byte-vector publishing for those disk-backed flows.

3. **Public bulk extraction workflow**: Consumers must be able to extract multiple entries through a public workflow with deterministic reporting and opt-in worker count.
   - Current: Consumers call `archive_reader::extract` or `extract_bytes` one entry at a time and must build their own iteration, ordering, error collection, and concurrency.
   - Target: libbsa exposes a dependency-light bulk extraction workflow or options object that accepts a worker count, defaults to single-threaded execution, streams entry data to caller-selected sinks, and returns deterministic per-entry results in archive/list order.
   - Acceptance: Public-header tests prove the bulk extraction API is C++20 and dependency-light; fixture tests prove worker count `1` and worker count `>1` produce identical bytes/results and stable result ordering.

4. **Public parallel packing workflow**: Consumers must be able to opt into parallel compression during packing while single-threaded defaults and writer output compatibility remain intact.
   - Current: Writer APIs expose archive targets and compatibility options, but no worker-count control or public parallel packing contract exists.
   - Target: Supported writer finalization exposes a dependency-light worker-count option or equivalent public control, defaults to `1`, and uses parallelism only when the caller opts in.
   - Acceptance: Writer tests for TES4-family BSA, BA2 GNRL, and BA2 DX10 compare worker count `1` with worker count `>1`; reopened archives expose identical metadata and extracted payloads, and deterministic writer bytes are preserved wherever earlier writer phases already required byte-stable output.

5. **Parallel correctness preservation**: Parallel workflows must not change validation, error, cancellation, or publishing semantics compared with single-threaded execution.
   - Current: Safe publish, overwrite rollback, validation, and typed errors are covered for existing single-threaded writer and reader paths.
   - Target: Opt-in parallel modes preserve the same `result` error categories, no-partial-publish guarantees, no partial-success sink behavior, and validation compatibility as single-threaded mode.
   - Acceptance: Failure-path tests cover at least one missing-source or compression failure during parallel packing and one sink/extract failure during parallel extraction; outputs and overwrite targets remain absent or restored according to the existing writer publish contracts.

6. **Benchmark suite**: Maintainers must be able to run benchmarks that compare single-threaded and multi-threaded packing/extraction using legal synthetic data.
   - Current: There is no benchmark target, benchmark preset, or generated report path.
   - Target: The build exposes a benchmark target or documented command that generates synthetic large-enough inputs, runs equivalent worker count `1` and worker count `>1` packing/extraction scenarios, verifies output correctness, and emits a repeatable report.
   - Acceptance: A committed benchmark README or generated-report schema documents how to run the benchmark; automated tests verify the benchmark target exists and its correctness checks do not require copyrighted game archives or a fixed speedup threshold.

7. **Thread-safety guarantees**: Public documentation must state thread-safety rules for readers, writers, entries, callbacks, sinks, validation, and benchmark helpers.
   - Current: The project constraints require isolated objects and no global mutable state, but public documentation does not define which objects may be shared or called concurrently.
   - Target: Thread-safety documentation explicitly states supported concurrent usage, caller-owned sink/callback requirements, writer mutation/finalization rules, and any non-thread-safe object ownership boundaries.
   - Acceptance: Documentation policy tests confirm every public reader, writer, validation, sink, callback, and bulk/parallel type has a thread-safety note or points to the canonical thread-safety section.

8. **Generated API documentation**: Consumers must be able to generate and read public API documentation for libbsa headers.
   - Current: Public headers have Doxygen-style comments, but no Doxygen configuration, generated-doc command, or documentation completeness policy exists.
   - Target: The repo includes a Doxygen configuration or equivalent build/documentation command that covers public headers and excludes private implementation details and `TES5Edit/`.
   - Acceptance: A documentation validation command succeeds locally without touching `TES5Edit/`, and policy tests or CI checks prove public headers are included while private dependency headers are not exposed as public API docs.

9. **Integration and target-format guidance**: Consumers must be able to follow examples for core library usage and understand supported archive variants, compression methods, and compatibility warnings.
   - Current: `docs/compatibility-evidence.md`, `docs/PRD.md`, and fixture docs describe project policy and warning evidence, but there is no consolidated consumer integration guide.
   - Target: Documentation includes examples for opening archives, listing entries, single-entry extraction, bulk extraction, creating each supported writer family, handling `result` errors, validating archives, and interpreting target-format/compression/warning guidance.
   - Acceptance: Documentation tests or package-consumer examples compile and exercise representative snippets, and a target-format guide covers TES3 BSA, TES4-family BSA v103/v104/v105, Fallout 4 BA2 GNRL/DX10, Starfield BA2 v2/v3 GNRL, Starfield BA2 v3 DX10, deflate, LZ4 frame, raw LZ4 block, and public compatibility warning codes.

## Boundaries

**In scope:**
- Bounded-memory extraction requirements for all supported archive families.
- Bounded-memory disk-backed packing requirements for TES3, TES4-family, BA2 GNRL, and BA2 DX10 writers.
- Public opt-in worker-count controls for bulk extraction and writer packing, with single-threaded defaults.
- Deterministic result ordering and compatibility-preserving output validation for parallel workflows.
- Synthetic benchmark target, command, or report path for single-threaded versus multi-threaded packing/extraction.
- Public thread-safety documentation for readers, writers, entries, callbacks, sinks, validation, and bulk/parallel APIs.
- Doxygen or equivalent public API documentation generation for public headers.
- Consumer integration examples and target-format/compression/warning guidance.

**Out of scope:**
- Fixed process-wide memory ceiling in MiB — this phase locks structural bounds and per-entry/per-chunk limits instead of promising a platform-independent RSS number.
- CI failure on a fixed parallel speedup threshold — benchmark performance varies by host, so correctness and reportability are mandatory while speedup is measured, not threshold-gated.
- Mandatory real game archives or BSArchPro local corpora — legal synthetic fixtures and optional local fixture hooks remain the required validation baseline.
- New archive families, new compression codecs, or expanded Starfield compatibility policy — Phase 12 polishes supported v1 formats rather than adding new format scope.
- DDS resizing, transcoding, mip generation, repair, or image-data transformation — BA2 DX10 writer semantics from Phase 9 remain unchanged.
- In-place archive mutation — v1 remains write-new/publish based.
- Stable long-term binary ABI policy — tracked as a v2 requirement.
- First-party CLI product — examples may compile or run, but libbsa remains a reusable library, not a CLI application.

## Constraints

- Public APIs remain C++20 and must not expose C++23 `std::expected`, DirectXTex, libdeflate, lz4, Windows, or TES5Edit types.
- `TES5Edit/` remains read-only and must not be edited, formatted, compiled into libbsa, staged, or used as a mutable fixture location.
- No new external runtime dependencies may be added without documented justification; standard C++ concurrency primitives are preferred.
- Single-threaded behavior is the default for every new workflow, and opt-in parallelism must preserve existing result/error, validation, and safe-publish contracts.
- Parallel writer output must remain deterministic for metadata, extraction behavior, validation reports, and byte-level output wherever earlier phases already required byte-stable archives.
- Compressed payload paths may allocate codec-required per-entry or per-chunk buffers, but they must not allocate based on whole archive size or unchecked archive-controlled counts.
- Benchmarks use legal synthetic data by default and must not require copyrighted game archives, local BSArchPro exports, or machine-specific paths.

## Acceptance Criteria

- [ ] Raw extraction paths for TES3, TES4-family, BA2 GNRL, and BA2 DX10 are verified to stream through bounded chunks without whole-archive buffering.
- [ ] Compressed extraction paths are verified to allocate at most the selected entry or texture chunk stored/decoded buffers required by the selected codec route.
- [ ] Disk-backed TES3, TES4-family, BA2 GNRL, and BA2 DX10 writer flows are verified not to publish by constructing one final whole-archive byte vector.
- [ ] Public bulk extraction exposes worker-count control, defaults to `1`, keeps public headers dependency-light, and returns deterministic per-entry results.
- [ ] Public writer packing exposes worker-count control or equivalent options, defaults to `1`, and preserves single-threaded output compatibility.
- [ ] Parallel extraction and packing tests compare worker count `1` with worker count `>1` and prove identical extracted bytes or metadata-valid equivalent output.
- [ ] Parallel failure tests prove no partial publish and stable `result` errors for representative writer, extraction, sink, or compression failures.
- [ ] A benchmark target or documented command runs legal synthetic packing/extraction comparisons for worker count `1` and worker count `>1`, verifies correctness, and emits a report without fixed CI speedup gating.
- [ ] Public thread-safety guidance covers readers, writers, entries, callbacks, sinks, validation, benchmarks, and new bulk/parallel types.
- [ ] Doxygen or equivalent API documentation generation covers public headers, excludes private implementation and `TES5Edit/`, and is validated by an automated command or policy test.
- [ ] Consumer examples compile or are otherwise machine-checked for opening/listing/extracting archives, bulk extraction, creating each writer family, handling `result`, and validating archives.
- [ ] Target-format guidance documents supported variants, compression methods, writer target policies, and public compatibility warning codes.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.90  | 0.75  | met    | Full Phase 12 v1 closeout across performance, concurrency, benchmarks, and docs is locked. |
| Boundary Clarity    | 0.90  | 0.70  | met    | Scope covers all supported public workflows and excludes speedup thresholds, new formats, in-place mutation, CLI product, and binary ABI policy. |
| Constraint Clarity  | 0.88  | 0.65  | met    | Bounded memory is structural, parallelism is opt-in, benchmark speed is report-only, and compressed codec buffers are per-entry/per-chunk. |
| Acceptance Criteria | 0.86  | 0.70  | met    | Acceptance is tied to public APIs, policy tests, benchmark command/report, docs generation, examples, and deterministic correctness checks. |
| **Ambiguity**       | 0.11  | <=0.20| met    | Gate passed after round 2. |

Status: met = meets minimum, below = planner treats as assumption

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Should bounded-memory scope cover all public workflows or only obvious gaps? | Cover all supported families and public reader/writer/validation/example workflows. |
| 1 | Researcher | What counts as the v1 parallel-capable deliverable? | Add opt-in public parallel bulk extraction and packing controls while preserving single-threaded defaults and deterministic output. |
| 1 | Researcher | What benchmark/docs evidence is mandatory? | Synthetic large fixture benchmarks plus Doxygen/API docs, examples, and target-format guidance are required. |
| 2 | Researcher + Simplifier | What does bounded memory mean for this phase? | No whole-archive buffering; disk-backed flows stream file data and use fixed scratch buffers, with compressed payloads limited to codec-required per-entry/per-chunk buffers. |
| 2 | Researcher + Simplifier | What public parallel workflow shape is locked? | Bulk extract/pack APIs or options accept worker count, default to `1`, and guarantee deterministic archive output and result ordering. |
| 2 | Researcher + Simplifier | How should benchmark speedup be accepted? | Benchmarks compare worker count `1` versus `>1` and assert correctness, but CI does not fail on a fixed speedup threshold. |

---

*Phase: 12-performance-concurrency-documentation-and-polish*
*Spec created: 2026-05-10*
*Next step: $gsd-discuss-phase 12 - implementation decisions (how to build what's specified above)*
