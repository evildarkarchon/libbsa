## Why

`archive_reader::extract_entries` currently processes each bulk request independently, so repeated archive paths can trigger duplicate lookup, sink creation, and payload extraction work when callers submit duplicate paths. This is especially wasteful in parallel extraction, where duplicate requests can compete across workers and amplify I/O, decompression, and user sink-factory cost.

## What Changes

- Add request de-duplication to bulk extraction so each distinct archive path is resolved and extracted at most once per `extract_entries` call.
- Preserve request-order result records for every input request, including duplicate requests.
- Keep per-entry failure behavior: missing paths, sink creation failures, and extraction failures are still reported on the corresponding request records without aborting independent paths.
- Avoid public API changes and keep `worker_count == 1` serial semantics observable through the same result contract.

## Capabilities

### New Capabilities

- `bulk-extraction-deduplication`: Defines how bulk extraction coalesces repeated archive paths while preserving request-order results and per-request failures.

### Modified Capabilities

None.

## Impact

- Affected code: `src/archive.cpp` bulk extraction orchestration, with possible reuse of `src/detail/parallel_work.cpp` for unique-path work scheduling.
- Affected tests: `tests/unit/bulk_extraction_tests.cpp` should add regression coverage for duplicate requests in serial and parallel modes.
- Public API: no type or signature changes are expected for `bulk_extract_request`, `bulk_extract_sink_factory`, `bulk_extract_entry_result`, or `archive_reader::extract_entries`.
- Dependencies: no new external dependencies.
