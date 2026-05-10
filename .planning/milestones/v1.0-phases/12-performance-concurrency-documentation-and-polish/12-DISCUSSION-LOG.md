# Phase 12: performance-concurrency-documentation-and-polish - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-09T23:46:54-07:00
**Phase:** 12-performance-concurrency-documentation-and-polish
**Areas discussed:** Bulk Extraction Surface, Parallel Packing Controls, Bounded-Memory Proof, Benchmarks And Docs

---

## Bulk Extraction Surface

### Public Workflow Location

| Option | Description | Selected |
|--------|-------------|----------|
| Reader member | Add an `archive_reader::extract_entries`-style workflow with `worker_count` defaulting to `1`. | yes |
| Free function | Add a standalone `bulk_extract(reader, requests, options)` workflow. | |
| You decide | Planner chooses the narrowest API after research. | |

**User's choice:** Reader member
**Notes:** Bulk extraction should extend the existing reader surface and keep single-threaded behavior as the default.

### Extraction Destinations

| Option | Description | Selected |
|--------|-------------|----------|
| Per-entry sink factory | Caller provides a callback or factory that creates or returns a sink per entry. | yes |
| Filesystem output root | Library maps archive paths under a host directory and writes files itself. | |
| Caller-owned sink list | Caller passes one sink per requested entry. | |
| You decide | Planner chooses while preserving deterministic results and no partial-success ambiguity. | |

**User's choice:** Per-entry sink factory
**Notes:** This avoids shared mutable sink state in parallel extraction and keeps libbsa from becoming a filesystem extraction tool.

### Result Reporting

| Option | Description | Selected |
|--------|-------------|----------|
| Stable per-entry results | Return a vector in request/archive order with each entry path plus success or error. | yes |
| Fail fast | Stop on first extraction failure and return one `result<void>` error. | |
| Summary plus details | Return counts plus per-entry records. | |
| You decide | Planner picks the smallest deterministic shape. | |

**User's choice:** Stable per-entry results
**Notes:** Ordering is deterministic and does not depend on worker completion order.

### Failure Policy

| Option | Description | Selected |
|--------|-------------|----------|
| Continue siblings | Record that entry error, keep extracting independent entries, and return the full deterministic result vector. | yes |
| Cancel remaining | Record first error and skip or cancel later entries. | |
| Policy option | Default to continue with an option for fail-fast. | |
| You decide | Planner chooses while preserving typed sink failures. | |

**User's choice:** Continue siblings
**Notes:** Per-entry errors are recorded without turning one failed entry into a whole-batch failure.

---

## Parallel Packing Controls

### Worker Count Location

| Option | Description | Selected |
|--------|-------------|----------|
| Write-call options | Add `write_to(path, writer_execution_options{ worker_count = 1 })`. | yes |
| Existing writer options | Add `worker_count` to each archive compatibility options struct. | |
| Shared writer options object | Add common `packing_options` or `parallel_options` reused by every writer. | |
| You decide | Planner chooses after tracing writer internals. | |

**User's choice:** Write-call options
**Notes:** Threading is per-finalization execution policy, not archive compatibility metadata.

### Writer Coverage

| Option | Description | Selected |
|--------|-------------|----------|
| All public writers where meaningful | TES4-family, BA2 GNRL, BA2 DX10, and TES3 get the same public shape. | yes |
| Compression writers only | TES4-family, BA2 GNRL, and BA2 DX10 only. | |
| BA2 first | Add only to BA2 GNRL and DX10. | |
| You decide | Planner maps writer coverage to acceptance. | |

**User's choice:** All public writers where meaningful
**Notes:** TES3 may validate or effectively ignore parallelism, but the public shape should be uniform.

### Worker Count Semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit positive count only | `1` means serial, greater than `1` means opt-in parallel, `0` is invalid. | yes |
| Zero means auto | `0` chooses hardware concurrency. | |
| Auto enum plus count | Support `serial`, `auto`, or explicit count. | |
| You decide | Planner chooses the smallest API that avoids accidental parallelism. | |

**User's choice:** Explicit positive count only
**Notes:** Avoids accidental oversubscription and keeps tests deterministic.

### Output Determinism

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve byte-stable output where already required | Parallel mode must match existing byte-stable contracts; otherwise metadata/extraction equivalence is enough. | yes |
| Require byte-for-byte for every writer | Strongest guarantee. | |
| Semantic equivalence only | Reopen, extract, and validate output must match, but bytes may differ. | |
| You decide | Planner applies prior phase contracts writer by writer. | |

**User's choice:** Preserve byte-stable output where already required
**Notes:** Phase 12 should not weaken earlier compatibility guarantees or over-constrain formats beyond prior acceptance contracts.

---

## Bounded-Memory Proof

### Extraction Evidence

| Option | Description | Selected |
|--------|-------------|----------|
| Behavior + policy tests | Large synthetic raw extraction uses counting sinks to prove chunked writes; compressed tests prove selected entry/chunk allocation plus source-policy checks. | yes |
| Runtime memory measurement | Watch process memory or RSS during large extraction. | |
| Documentation only | Document structural bounds without enforcement. | |
| You decide | Planner chooses the evidence mix. | |

**User's choice:** Behavior plus policy tests
**Notes:** Structural proof is preferred over host-dependent memory measurements.

### Compressed Extraction Buffers

| Option | Description | Selected |
|--------|-------------|----------|
| Per-entry/chunk allowed and documented | Keep whole-block codec buffers, enforce selected-entry/chunk bounds and result errors. | yes |
| Force streaming decompression now | Rewrite codec adapters for streaming decompression. | |
| Leave as implementation detail | Avoid documenting allocation shape. | |
| You decide | Planner chooses after tracing codec APIs. | |

**User's choice:** Per-entry/chunk allowed and documented
**Notes:** Matches the locked SPEC boundary for libdeflate/LZ4 block style codecs.

### Disk-Backed Writer Evidence

| Option | Description | Selected |
|--------|-------------|----------|
| Refactor publish to streaming file writer + policy checks | Disk-backed finalization writes records and payloads incrementally and tests reject final whole-archive byte-vector publishing. | yes |
| Only cover large synthetic writer tests | Black-box large writer coverage. | |
| Memory/RSS gates | Host memory measurement checks. | |
| You decide | Planner picks the implementation proof. | |

**User's choice:** Refactor publish to streaming file writer plus policy checks
**Notes:** Large tests should be paired with source/policy checks so whole-archive vector publishing does not sneak back in.

### Memory Entry Semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Keep memory entries copied | Bounded-memory guarantee focuses on disk-backed large inputs; `add_bytes` remains copied and documented. | yes |
| Stream memory entries too | Avoid internal copies where possible. | |
| Disallow large memory entries | Add public size caps. | |
| You decide | Planner chooses if current behavior creates acceptance issues. | |

**User's choice:** Keep memory entries copied
**Notes:** Existing memory ownership semantics remain intentional and should be explicit in docs.

---

## Benchmarks And Docs

### Benchmark Deliverable

| Option | Description | Selected |
|--------|-------------|----------|
| CMake benchmark target + JSON/Markdown report | Generates legal synthetic data, compares worker `1` vs greater than `1`, verifies correctness, and writes a repeatable report. | yes |
| Standalone example program only | Simpler for users to run, weaker as a maintainer benchmark contract. | |
| CTest slow test only | Easy to wire, awkward benchmark reporting and CI runtime risk. | |
| You decide | Planner chooses while preserving correctness checks and no speedup threshold. | |

**User's choice:** CMake benchmark target plus JSON/Markdown report
**Notes:** Benchmark reports are evidence, not speed-threshold gates.

### Benchmark CI Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Buildable, not default-run | CI/policy verifies the target exists and correctness hooks are present; users run it explicitly. | yes |
| Run a tiny smoke benchmark in CI | More coverage but noisy default test time. | |
| Run full benchmark in CI | Strong evidence but machine-dependent and slow. | |
| You decide | Planner picks CI wiring while respecting no fixed speedup threshold. | |

**User's choice:** Buildable, not default-run
**Notes:** Keeps default CI fast and reliable while preserving a real maintainer benchmark path.

### API Documentation Stack

| Option | Description | Selected |
|--------|-------------|----------|
| Doxygen target only | Add repo-local Doxygen config/target for public headers, exclude private code and TES5Edit, and validate when Doxygen is available. | yes |
| Doxygen + Sphinx/Breathe | More polished but larger toolchain. | |
| Markdown-only API docs | Lower tooling but weaker navigation. | |
| You decide | Planner chooses if Doxygen availability creates friction. | |

**User's choice:** Doxygen target only
**Notes:** Fits existing Doxygen-style comments without adding a heavier documentation stack.

### Examples And Thread Safety

| Option | Description | Selected |
|--------|-------------|----------|
| Compile-checked examples + policy docs | Add consumer examples that compile through package-consumer or docs tests, plus machine-checked thread-safety notes for public APIs. | yes |
| Narrative docs only | Faster but prone to drift. | |
| Full sample CLI | Demonstrable but outside v1 scope. | |
| You decide | Planner picks exact docs/tests while machine-checking coverage. | |

**User's choice:** Compile-checked examples plus policy docs
**Notes:** No sample CLI for v1; examples should compile so docs do not drift.

---

## the agent's Discretion

- Exact public type names for bulk extraction requests/results, sink factory signatures, and writer execution/finalization options.
- Exact worker implementation strategy, task scheduling, benchmark target names, report schema, Doxygen target names, and policy-test mechanics.

## Deferred Ideas

None - discussion stayed within phase scope.
