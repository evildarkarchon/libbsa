## Context

The libbsa `bsa` CLI is a single-file custom-parser executable (`tools/cli/main.cpp`) built behind the `LIBBSA_BUILD_CLI` option. It dispatches `pack`, `unpack`, `list`, `info`, and `validate` subcommands and renders `libbsa::error` values as diagnostics with stable exit codes (`process_exit::{success=0, operational_failure=1, usage_error=2}`).

The library already implements the parallelism this change exposes:

- `src/detail/parallel_work.cpp` provides `run_indexed_work(task_count, worker_count, work)`: `worker_count == 1` runs serially; `worker_count > 1` spawns `std::jthread` workers with atomic work-stealing, first-error capture, and a hard cap of `1024` workers. `worker_count == 0` is rejected upstream as invalid.
- Packing honors `write_execution_options{ std::uint32_t worker_count{1U}; }` (`include/libbsa/writer.hpp`) on all four writers; the writers validate the count and feed `run_indexed_work` (e.g. `tes4_bsa_prepare.cpp`, `ba2_gnrl_prepare.cpp`, `ba2_dx10_chunk_assembler.cpp`).
- Extraction honors `bulk_extract_options{ std::uint32_t worker_count{1U}; }` (`include/libbsa/archive.hpp`), validated in `archive.cpp` and fed to `run_indexed_work`. The CLI's own `file_sink_factory` is already mutex-guarded for concurrent `create`/`finish`.

Today the CLI never sets these knobs: `run_pack` helpers call `write_to(path, write_execution_options{})` and `run_unpack` calls `extract_entries(requests, sink_factory, bulk_extract_options{})`, so `worker_count` is always the default `1`. This change wires a user-facing control into those existing options. No library code changes are required.

A guard test (`tests/unit/public_include_boundary_tests.cpp`) asserts the public headers never leak `std::thread`/`std::jthread`/`std::mutex`; this change stays entirely inside `tools/cli/main.cpp` and respects that boundary.

## Goals / Non-Goals

**Goals:**
- Let users control worker-thread count for `pack` and `unpack` via `--threads`/`-j`.
- Support an `auto` value (and its `0` shorthand) resolved by the CLI to host hardware concurrency.
- Validate values up front and reject invalid ones as usage errors before any archive work.
- Preserve current behavior when the option is omitted (single worker).
- Reuse the library's existing options structs with zero public-API change.

**Non-Goals:**
- No new threading engine, thread pool, or concurrency primitives — the library substrate is reused as-is.
- No parallelism for `list`, `info`, or `validate` (these are not bulk/CPU-bound operations in scope here).
- No change to the library's `worker_count` semantics, validation, or the `1024` cap.
- No `auto` semantics inside the library (`0` stays invalid there); resolution is a CLI concern only.
- No automatic parallelism by default; users opt in explicitly.

## Decisions

### Decision: Option spelling is `--threads` with `-j` alias
Use `--threads <value>` as the canonical long option and `-j <value>` as the short alias (familiar from `make -j`). Both `pack` and `unpack` get an identical `option_spec` entry of value kind.
- **Alternatives considered**: `--workers` (more precise but less conventional); `--jobs` only (no long form). `--threads` is the clearest user-facing term and `-j` matches widespread tool convention.

### Decision: Value taxonomy is "positive integer, `auto`, or `0`"
Accept a base-10 positive integer, the literal token `auto`, or `0` as a shorthand for `auto`. Parse via a single shared helper, e.g. `resolve_worker_count(std::string_view) -> libbsa::result<std::uint32_t>`, used by both subcommands.
- `auto` and `0` both resolve to `std::thread::hardware_concurrency()`, clamped to at least `1` when the standard library returns `0` (indeterminate).
- A parsed integer must be `0` (meaning auto) or in `[1, cap]`. The CLI mirrors the library's `1024` cap as its own validated maximum so it rejects oversized values as a usage error (exit code 2) rather than letting the library surface an operational error later.
- **Rationale for `0` = auto**: `0` is the established "use all cores" shorthand in adjacent compression tools (`zstd -T0`, `xz -T0`), so it matches user expectation for an archive tool. The mapping lives entirely in the CLI — it resolves `0`/`auto` to a concrete count before any library call — so the library's contract that `0` is invalid and "never means auto" is preserved; the library never receives `0`. The explicit `auto` token is kept alongside `0` as a self-documenting alternative.
- **Alternatives considered**: Rejecting `0` as a usage error — declined in favor of the `zstd`/`xz` convention. Dropping the `auto` token and using only `0` — declined; the word `auto` is more readable and costs nothing to keep. Allowing percentages or fractions — out of scope and unjustified complexity.

### Decision: Resolve and validate in the CLI before any library call
Parsing/resolution happens during argument handling in `run_pack`/`run_unpack`, before files are collected or any writer/reader is constructed. Failures return a usage error through the existing `make_usage_error`/`usage_failure` path so no archive is created or modified.
- **Rationale**: Matches the existing `--format` taxonomy pattern (validate-before-work) and the spec requirement that invalid values never start the operation.

### Decision: Default is a single worker when omitted
When `--threads`/`-j` is absent, the resolved count is `1`, preserving today's observable behavior and avoiding surprise multi-core resource use in scripts and CI.
- **Alternatives considered**: Defaulting to `auto`. Rejected for this change to keep behavior backward-compatible and predictable; switching the default can be a separate, deliberate decision later.

### Decision: Plumb the resolved count through existing options structs
- `pack`: pass `write_execution_options{ worker_count }` to all four `write_to(...)` calls (`pack_tes3`, `pack_tes4`, `pack_ba2_gnrl`, `pack_ba2_dx10`). The pack helpers gain a `std::uint32_t worker_count` parameter threaded from `run_pack`.
- `unpack`: pass `bulk_extract_options{ worker_count }` to `extract_entries(...)`.
- All threading types remain in `tools/cli/main.cpp` / standard library; public headers are untouched, honoring the public-include boundary test.

## Risks / Trade-offs

- **`std::thread::hardware_concurrency()` returns `0` on some hosts** → Clamp to `1` so `auto`/`0` always yields a valid, runnable worker count.
- **`0` silently means "use all cores" (a fat-finger footgun)** → Accepted as the cost of matching the `zstd`/`xz -T0` convention; the omitted-option default stays single-worker, so `0` only saturates cores when explicitly requested. The explicit `auto` token remains available for readability.
- **CLI cap drifts from the library's `1024` cap if the library changes** → Document the cap as mirroring the library limit near the CLI helper and add a test asserting the boundary; if they diverge, the library still rejects oversized counts, so the failure mode is a clear error, not corruption.
- **Higher worker counts increase peak memory (per-worker scratch/compression buffers)** → This is inherent to opting into parallelism; the default stays single-worker so users choose the trade-off. No mitigation needed beyond documenting it in help.
- **Sink/temp-file contention during parallel unpack** → Already mitigated: the CLI `file_sink_factory` is mutex-guarded and uses an atomic temp-id counter; this change adds no new shared mutable state.
- **TES3 packing ignores `worker_count` (no compression work)** → Accepted; passing the option is harmless and keeps the CLI uniform across formats.

## Migration Plan

Additive and backward-compatible. The option is optional and defaults to single-worker behavior, so existing invocations and scripts are unaffected. No data, archive-format, or public-API migration is required. Rollback is trivial: the change is confined to `tools/cli/main.cpp` and its tests.

## Open Questions

- Should a future change flip the default to `auto` for better out-of-the-box performance? Deferred; out of scope here to keep behavior backward-compatible.
- Should `list`/`info`/`validate` ever gain parallelism (e.g., parallel multi-archive validation)? Not in scope for this change.
