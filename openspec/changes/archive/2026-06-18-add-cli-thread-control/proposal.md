## Why

The libbsa CLI already drives packing and extraction through the library's fully parallel engine (`run_indexed_work` + `std::jthread`), but it hardcodes single-threaded execution by always passing the default `worker_count` of `1`. On multi-core machines this leaves most CPU idle during the slowest operations — compressing large archives and extracting bulk requests — so the `bsa` tool is needlessly slow even though the parallelism it needs is built and tested in the library.

## What Changes

- Add a worker-count control (`--threads`/`-j`) to the `pack` and `unpack` subcommands so users can choose how many worker threads packing and extraction use.
- Define the value taxonomy for the control: a positive integer count, plus `auto` (and its equivalent shorthand `0`, matching the `zstd -T0` / `xz -T0` convention) which the CLI resolves to the host's available hardware concurrency. The library treats `0` as invalid and never as "auto", so the CLI resolves `auto`/`0` to a concrete count before calling the library.
- Apply the resolved worker count to the existing `libbsa::write_execution_options::worker_count` in all four pack paths (TES3, TES4 BSA, BA2 GNRL, BA2 DX10) and to `libbsa::bulk_extract_options::worker_count` in the unpack path.
- Reject invalid worker-count values (negative, non-numeric, empty, out of range) as usage errors before any archive work begins, consistent with existing CLI usage-error conventions.
- Make the control and its accepted values discoverable through `pack --help` and `unpack --help`.
- Keep all threading types out of the public headers (enforced by the existing public-include boundary test); no library API changes are required.

## Capabilities

### New Capabilities
- `cli-parallel-execution`: The CLI worker-count control shared by `pack` and `unpack` — option syntax, accepted value taxonomy (positive count, `auto`, and `0` as an alias for `auto`), resolution of `auto`/`0` to host hardware concurrency, validation and usage-error rejection of invalid values, mapping to the library's per-operation worker count, and discoverability through subcommand help.

### Modified Capabilities
<!-- No existing CLI requirement changes. Packing/extraction correctness requirements are unchanged; the worker-count control is an orthogonal capability layered on top via the library's existing options. -->

## Impact

- **Code**: `tools/cli/main.cpp` only — new option specs and parsing for `run_pack` and `run_unpack`, a shared worker-count parse/resolve/validate helper, and plumbing the resolved count into the four `write_to(..., write_execution_options)` calls and the `extract_entries(..., bulk_extract_options)` call. Help/usage text updates for both subcommands.
- **Library API**: None. Reuses existing `write_execution_options::worker_count` (`include/libbsa/writer.hpp`) and `bulk_extract_options::worker_count` (`include/libbsa/archive.hpp`). No public header changes, so the public-include boundary (no leaked `std::thread`/`std::mutex`) is preserved.
- **Dependencies**: None new. `auto` resolution uses `std::thread::hardware_concurrency()` from the standard library; the CLI link surface (`libbsa::libbsa` + standard/system libraries) is unchanged.
- **Tests**: New CLI tests covering value parsing, `auto` resolution, invalid-value usage errors, and that pack/unpack forward the resolved worker count. No changes to library threading internals, which remain covered by existing execution and bulk-extraction tests.
- **Docs**: CLI help text for `pack` and `unpack` gains the worker-count option and its accepted values.
