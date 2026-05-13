## Context

The BA2 DX10 writer currently reads a full DDS during `add_file`, uses DirectXTex to validate and copy every subresource, writes each subresource to an individual snapshot file, and stores snapshot paths in writer state. During `write_to`, chunk preparation plans BA2 texture chunks, finds the needed subresources, reopens each snapshot file, appends its bytes into a raw chunk vector, and then compresses that chunk.

The snapshot model is intentional: it gives the writer ownership of add-time texture bytes without holding long-lived full DDS buffers in memory, and it protects output from source-file mutation between `add_file` and `write_to`. The current finalization path still pays avoidable overhead from repeated per-subresource read setup, repeated vector growth, and benchmarks that only time the combined DX10 pack/extract path rather than making snapshot overhead visible.

## Goals / Non-Goals

**Goals:**

- Preserve `add_file` snapshot ownership semantics and best-effort temp cleanup.
- Assemble each planned BA2 DX10 chunk through a single batched helper that collects the required snapshot subresources, validates their aggregate size against `planned.raw_size`, and copies them into a pre-sized or pre-reserved raw chunk buffer.
- Reduce per-chunk allocation churn and avoid repeated ad hoc append loops while preserving chunk order, compression routing, archive bytes, and error categories.
- Add benchmark measurements that separate DX10 add-time snapshot staging from final `write_to` preparation/serialization enough to quantify snapshot overhead.

**Non-Goals:**

- Replacing snapshot-backed ownership with caller-file borrowing, long-lived DirectXTex objects, or mandatory in-memory DDS retention.
- Changing public writer APIs, public metadata types, archive ordering, chunk planning, compression method selection, or emitted BA2 bytes.
- Introducing new dependencies or changing the TES5Edit read-only reference boundary.
- Reworking all writer disk-source helpers or generalized compression streaming behavior.

## Decisions

1. Keep snapshot-backed writer state as the ownership boundary.

   Rationale: The existing `ba2_dx10_writer::state` lifetime and snapshot directory cleanup already establish the required behavior that callers may mutate or delete DDS source files after `add_file`. This change should optimize how those snapshots are consumed, not reopen the source ownership question.

   Alternatives considered: keep the full DDS bytes or DirectXTex `ScratchImage` in writer state. That would remove temp rereads but would increase long-lived memory pressure and leak third-party ownership decisions across the texture boundary.

2. Add a chunk-local snapshot batch assembly helper.

   Rationale: `ba2_dx10_prepare_chunk` has the planned chunk metadata and therefore knows the expected decoded chunk size before reading any snapshot bytes. A helper can gather the exact subresources for the planned array/face/mip range once, verify that the summed snapshot sizes match `planned.raw_size`, reserve or allocate the destination vector through `detail::byte_vector` helpers, and then copy snapshot bytes in BA2-required order. Keeping this helper near `append_subresource_bytes` limits the change to DX10 preparation internals.

   Alternatives considered: only call `reserve` before the existing loop. That fixes allocation churn but leaves size validation and snapshot read orchestration scattered. Replacing the snapshot files with a new per-entry bundle may be useful later if measurements show open/close costs dominate, but it is a larger temp-file layout change than this step needs.

3. Preserve bounded disk reads and result-based diagnostics.

   Rationale: Snapshot payload reads should continue using `detail::for_each_disk_source_chunk` or exact-size helpers with `ba2_dx10_snapshot_source_context` so I/O failures, missing snapshots, and unexpected snapshot size changes remain `result<T>` failures with BA2 DX10 wording. Allocation failures should continue to flow through `detail::reserve_byte_vector` or `detail::make_byte_vector` rather than escaping as exceptions.

   Alternatives considered: use ordinary `std::vector::reserve` and stream exceptions. That would regress the project-wide result/error model and duplicate prior writer-source hardening work.

4. Measure add-time snapshot staging and finalization separately in benchmarks.

   Rationale: The current `ba2_dx10_pack_extract` benchmark starts timing after `add_file`, so it does not show snapshot staging cost and mixes final write cost with reopen/extract validation. The benchmark should report at least one DX10 scenario that times add-time snapshot staging and `write_to` separately while preserving correctness checks.

   Alternatives considered: rely on unit tests or one combined elapsed value. Unit tests protect behavior but do not reveal the performance trade-off; a single combined value hides whether staging or finalization is the bottleneck.

## Risks / Trade-offs

- Snapshot files can still disappear or change before finalization -> keep exact size validation and existing snapshot I/O diagnostics so no archive is published with stale chunk metadata.
- Pre-sizing from `planned.raw_size` trusts planner metadata -> validate the summed snapshot sizes against the planned size before compression and return `format_error` on mismatch.
- Benchmark timing can be noisy -> keep correctness checks in the benchmark and compare relative add/write timings across the same synthetic DDS input and worker-count loop.
- A helper-level batch still opens existing per-subresource snapshot files -> treat this as the low-risk first optimization; use the new benchmark data before deciding whether to bundle snapshots into fewer temp files.

## Migration Plan

1. Add the DX10 snapshot batch assembly helper and route `ba2_dx10_prepare_chunk` through it.
2. Add or update focused tests for planned-size validation, missing snapshot handling, and unchanged DX10 writer round-trip output.
3. Extend `benchmarks/libbsa_benchmarks.cpp` to report separate DX10 add-time snapshot staging and finalization measurements.
4. Run BA2 DX10 writer, writer execution, and benchmark builds to confirm archive behavior remains stable.

Rollback is internal: restore the previous `append_subresource_bytes` path and benchmark scenario shape without changing public APIs or persisted data.

## Open Questions

- None. A later change can use the benchmark data to decide whether per-entry snapshot bundles are worth the additional temp-file layout work.
