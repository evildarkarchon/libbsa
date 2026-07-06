## 1. Shared worker-count parsing and resolution

- [x] 1.1 Add a CLI-local maximum worker constant in `tools/cli/main.cpp` mirroring the library's `1024` cap, with a comment noting it mirrors the `run_indexed_work` limit.
- [x] 1.2 Implement a shared helper `resolve_worker_count(std::string_view value) -> libbsa::result<std::uint32_t>` that accepts a base-10 positive integer, the literal `auto`, or `0` as an alias for `auto`.
- [x] 1.3 In the helper, resolve both `auto` and `0` to `std::thread::hardware_concurrency()`, clamping to at least `1` when the standard library returns `0`.
- [x] 1.4 In the helper, reject empty, non-numeric, negative, and over-cap values by returning a usage error (so callers route through the existing usage-error exit path); `0` is accepted and resolved to auto, not rejected.

## 2. Wire the control into `pack`

- [x] 2.1 Add a `--threads` / `-j` value option to the `run_pack` `option_spec` array.
- [x] 2.2 In `run_pack`, read the option (defaulting to `1` when absent) and resolve it via `resolve_worker_count`, returning a usage error before collecting input files when resolution fails.
- [x] 2.3 Thread a `std::uint32_t worker_count` parameter into `pack_tes3`, `pack_tes4`, `pack_ba2_gnrl`, and `pack_ba2_dx10`.
- [x] 2.4 Pass `libbsa::write_execution_options{ worker_count }` to each helper's `write_to(...)` call.

## 3. Wire the control into `unpack`

- [x] 3.1 Add a `--threads` / `-j` value option to the `run_unpack` `option_spec` array.
- [x] 3.2 In `run_unpack`, read the option (defaulting to `1` when absent) and resolve it via `resolve_worker_count`, returning a usage error before opening the archive when resolution fails.
- [x] 3.3 Pass `libbsa::bulk_extract_options{ worker_count }` to the `extract_entries(...)` call.

## 4. Help and discoverability

- [x] 4.1 Update `pack --help` text to document `--threads`/`-j`, its `auto` value (with `0` as an alias), and the positive-integer range.
- [x] 4.2 Update `unpack --help` text to document `--threads`/`-j`, its `auto` value (with `0` as an alias), and the positive-integer range.

## 5. Tests

- [x] 5.1 Add CLI tests for `resolve_worker_count`: valid integer, `auto` and `0` both resolving to host concurrency (>= 1), and rejection of negative, non-numeric, empty, and over-cap values.
- [x] 5.2 Add a test asserting `pack` with an invalid `--threads` value (e.g., negative or non-numeric) exits with the usage-error code and creates no output archive.
- [x] 5.3 Add a test asserting `unpack` with an invalid `--threads` value (e.g., negative or non-numeric) exits with the usage-error code and extracts no files.
- [x] 5.4 Add tests asserting `pack` forwards the resolved worker count as `write_execution_options::worker_count` for each format family, and `unpack` forwards it as `bulk_extract_options::worker_count`.
- [x] 5.5 Add a test asserting that omitting `--threads` runs with a single worker (default unchanged).
- [x] 5.6 Add tests asserting `pack --help` and `unpack --help` mention the worker-count control and its accepted values.

## 6. Verification

- [x] 6.1 Confirm `tests/unit/public_include_boundary_tests.cpp` still passes (no threading types leaked into public headers).
- [x] 6.2 Build with `LIBBSA_BUILD_CLI` enabled and run the CLI test suite via CTest; confirm all new and existing tests pass.
- [x] 6.3 Run `openspec validate "add-cli-thread-control"` and confirm the change validates clean.
