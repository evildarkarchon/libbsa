## 1. Build scaffolding and deliverable

- [x] 1.1 Add `option(LIBBSA_BUILD_CLI "Build the libbsa command-line tool" ...)` to the root `CMakeLists.txt` with a documented default.
- [x] 1.2 Create the `tools/cli/` source tree and a new `add_executable` target (working name `bsa`) linking only `libbsa::libbsa`, using `cxx_std_20` and the project's MSVC warning flags.
- [x] 1.3 Guard the entire CLI target and its tests behind `LIBBSA_BUILD_CLI`; verify the project still configures and builds with the option `OFF` and that the CLI target is absent.
- [x] 1.4 Confirm the CLI target's resolved link libraries are only `libbsa::libbsa` plus system/standard libraries already required by libbsa (no new third-party dependency).

## 2. CLI foundation (`cli-foundation`)

- [x] 2.1 Implement a minimal argument parser supporting `--flag`, `--key value`, `--key=value`, `-h`/`-V`, repeated `--path`, and `--` end-of-options, with usage errors for unknown options and missing values.
- [x] 2.2 Implement subcommand dispatch for `pack`, `unpack`, `list`, `info`, `validate`; no/unknown subcommand prints top-level usage and returns the usage-error code.
- [x] 2.3 Implement `--help`/`-h` (top-level lists subcommands; per-subcommand lists options) and `--version`/`-V` (derived from the libbsa version), each exiting with the success code.
- [x] 2.4 Define the single `--format` token table mapping each token to its writer + target and supported options (per design table), and make it drive parsing, validation, and help text.
- [x] 2.5 Implement an `error`-to-diagnostic renderer that prints the `error_code` category and message to `stderr`.
- [x] 2.6 Define stable exit codes (`0` success, `2` usage error, `1` operational failure) and wrap subcommand execution in `main` so no library error or stray exception crashes the process.

## 3. Pack subcommand (`cli-archive-packing`)

- [x] 3.1 Parse `pack` arguments (input directory, output path, required `--format`, `--overwrite`, `--compress`), returning usage errors for missing input/output/`--format`.
- [x] 3.2 Map the selected `--format` token to the concrete writer + target and reject `--compress` values the selected writer cannot honor (raw-only TES3, compressed-only DX10) as usage errors.
- [x] 3.3 Walk the input directory recursively over regular files, compute each archive-internal path relative to the input root joined with `/` (excluding the input directory name and host-absolute prefixes), and add each via the writer's `add_file`.
- [x] 3.4 Implement the BA2 DX10 path adding inputs as DDS via `ba2_dx10_writer::add_file`, surfacing invalid-DDS rejections as operational failures.
- [x] 3.5 Finalize via `write_to`, honoring overwrite policy (refuse existing output unless `--overwrite`), and report a missing input directory as an operational failure.

## 4. Unpack subcommand (`cli-archive-extraction`)

- [x] 4.1 Parse `unpack` arguments (archive path, output directory, optional repeated `--path`, `--overwrite`), returning usage errors for missing archive/output.
- [x] 4.2 Implement a host-file `payload_sink` that writes accepted bytes to a `std::ofstream` and returns the full accepted count.
- [x] 4.3 Implement a `bulk_extract_sink_factory` that validates the destination, enforces overwrite policy, creates intermediate directories, and returns a file sink; record per-entry failures via the returned `error`.
- [x] 4.4 Implement output path-safety: normalize `output_root` and `output_root / entry_path` and refuse any entry that resolves outside the root (traversal, absolute, drive/UNC) before writing.
- [x] 4.5 Drive extraction through `archive_reader::extract_entries` for both full (requests from `entries()`) and selective (`--path`) modes; aggregate per-entry failures into the operational-failure exit code while still extracting siblings.

## 5. Inspection subcommands (`cli-archive-inspection`)

- [x] 5.1 Implement `list`: print entries in `archive_reader::entries` order, one per line, with a detail option adding raw size, stored size, and compression.
- [x] 5.2 Implement `info`: print archive metadata (type, variant, version, flags, file count, default compression) and BA2-specific fields that are present.
- [x] 5.3 Implement `validate`: run `validate_archive`, print diagnostics (with `error_code`) and warnings (code, severity, message, path), exit failure on invalid, success on valid; add optional `--strict` to escalate warnings to failure.

## 6. Tests (CTest integration)

- [x] 6.1 Add a CLI integration test target/registration under `tests/` gated by `LIBBSA_BUILD_CLI`, invoking the built CLI binary.
- [x] 6.2 Add pack → unpack round-trip tests for representative `--format` tokens, asserting extracted bytes match the inputs.
- [x] 6.3 Add `list`/`info`/`validate` output assertions against known fixtures, including a warning and an invalid-archive case.
- [x] 6.4 Add failure-path tests: unknown subcommand, unknown `--format`, missing required args, overwrite refusal (pack and unpack), traversal refusal, and missing selective `--path`, asserting the correct exit-code categories.
- [x] 6.5 Add DX10 pack/unpack coverage reusing existing DDS fixtures, gated like other fixture-dependent tests so the default lane stays green.

## 7. Documentation

- [x] 7.1 Add Doxygen-compliant comments for CLI helpers that warrant them and a short usage section to `README.md` documenting the subcommands, `--format` tokens, and exit-code categories.
- [x] 7.2 Verify the change builds and the CLI tests pass on a Windows MSVC preset (e.g., `windows-msvc-debug-static`).
