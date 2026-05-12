## Context

`archive_reader::extract_entries` currently schedules work across the full request span. Each request performs lookup, calls `bulk_extract_sink_factory::create`, and extracts the payload independently. With duplicate `bulk_extract_request::path` values, this repeats archive lookup and payload I/O/decompression work and can also call user sink-factory code multiple times for the same archive path.

The public result shape is request-ordered and per-request failures are recorded in `bulk_extract_entry_result`. The implementation must keep that shape, keep setup failures as outer `result` failures, and continue using bounded extraction readers for the actual payload copy/decode work.

## Goals / Non-Goals

**Goals:**

- Coalesce exact duplicate request path strings before dispatching extraction work.
- Resolve and extract each distinct requested path at most once per `extract_entries` call.
- Preserve one result record per input request in input order.
- Mirror the first occurrence outcome to later duplicate result records.
- Keep missing-path, sink-factory, and extraction failures isolated to that path group rather than aborting unrelated unique paths.
- Preserve existing `worker_count` validation and worker scheduling limits.

**Non-Goals:**

- No public API signature changes.
- No attempt to coalesce different request strings that normalize to the same archive entry.
- No buffering of extracted payloads solely to write duplicate outputs multiple times.
- No changes to format-specific extraction readers or `detail::run_indexed_work` scheduling semantics unless implementation exposes a narrow helper need.

## Decisions

1. Coalesce by exact `bulk_extract_request::path` string before worker dispatch.

   Rationale: exact string grouping removes the caller-reported duplicate workload without requiring format-specific path normalization outside existing lookup code. It also avoids surprising merges for distinct caller inputs whose canonicalization rules may differ by archive family.

   Alternative considered: coalesce by `entry_metadata::path` after lookup. This would require lookup for every unique raw string first and could merge paths with different caller-visible validation behavior. It is deferred until a concrete need appears.

2. Let the first request occurrence own lookup, sink creation, and extraction.

   Rationale: this is the only approach that eliminates all duplicated work named in the issue: `find`, sink creation, and extraction. Later duplicates receive request-order result records copied from the first occurrence outcome, with their own `path` field preserved from the original request.

   Alternative considered: fan out a single extraction stream into one sink per duplicate request. This would preserve duplicate sink side effects but still repeats sink creation, complicates per-sink failures, and risks turning a bounded streaming path into a multi-sink orchestration problem.

3. Build a compact unique-work list and run `detail::run_indexed_work` over unique paths.

   Rationale: keeping worker scheduling at the unique-path level preserves the existing serial/parallel control path while reducing contention and duplicate archive reads. The final results vector can still be pre-sized to `requests.size()` and populated from group outcomes.

   Alternative considered: keep the current request-index work and use a shared concurrent cache. This adds synchronization around in-flight duplicate paths and is more complex than pre-grouping the immutable request span.

4. Update public comments/tests to make duplicate handling explicit.

   Rationale: current comments describe creating one sink per requested entry. After this change, duplicate request paths intentionally share the first occurrence extraction result and do not create additional sinks. The public header and thread-safety guidance should match the observable contract.

## Risks / Trade-offs

- Duplicate requests no longer trigger duplicate sink-factory side effects -> Mitigation: document that only the first occurrence creates a sink and add tests that lock in request-order mirrored results.
- Exact-string grouping misses equivalent paths with different casing or separators -> Mitigation: keep scope narrow to avoid cross-format normalization bugs; callers still receive existing lookup behavior for each distinct string.
- Copying result metadata from the first occurrence could accidentally overwrite duplicate request paths -> Mitigation: explicitly set each duplicate result record's `path` from the corresponding original request after copying shared outcome fields.
- Parallel workers could race while writing group outcomes -> Mitigation: assign each unique group to exactly one worker and write only that group's owned result indices.

## Migration Plan

1. Add regression tests covering duplicate request paths in serial and parallel extraction.
2. Implement pre-grouping in `archive_reader::extract_entries` and dispatch unique groups through existing worker scheduling.
3. Update public comments or documentation that describe sink creation frequency for bulk extraction.
4. Run the focused bulk extraction tests, then the broader unit test target if available.

Rollback is straightforward: revert the grouping change and associated duplicate-specific tests/docs. No persisted archive data or public ABI migration is involved.

## Open Questions

None.
