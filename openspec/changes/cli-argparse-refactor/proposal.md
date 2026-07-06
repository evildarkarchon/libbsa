## Why

The `bsa` CLI parses arguments with a ~370-line hand-rolled option engine (`option_spec`, `parse_options`, `has_flag`, `values_for`, `single_value_for`) in `tools/cli/main.cpp`. This bespoke parser duplicates well-solved logic (long/short options, `--name=value`, `--` terminator, repeatable options, value validation), is a recurring source of edge-case bugs, and must be re-tested by hand for every new subcommand or flag. Replacing it with the maintained, header-only `argparse` library removes that maintenance burden and standardizes parsing, usage, and help generation without adding a runtime/shared-library dependency.

## What Changes

- Add `argparse` (p-ranav, header-only) as a vcpkg dependency consumed only by the gated CLI target.
- Replace the hand-rolled parsing layer in `tools/cli/main.cpp` (`option_kind`, `option_spec`, `parsed_option`, `parsed_arguments`, `parse_options`, `find_option_spec`, `has_flag`, `values_for`, `single_value_for`) with an `argparse::ArgumentParser` configured with one subparser per command (`pack`, `unpack`, `list`, `info`, `validate`).
- Preserve the existing public CLI contract exactly: subcommand set, option names and spellings (`--format`, `--compress`, `--threads`/`-j`, `--overwrite`, `--path`, `--details`/`--detail`, `--strict`), positional argument counts, global `-h`/`--help` and `-V`/`--version`, exit-code categories (`0` success, `2` usage error, `1` operational failure), and the existing `libbsa::result`/`render_error` diagnostics for operational failures.
- Preserve the Windows UTF-8 argument-acquisition path (`command_line_arguments` via `GetCommandLineW`/`CommandLineToArgvW`); argparse is fed the existing UTF-8 argument vector, never the CRT `argv`.
- Add a translation boundary so `argparse` parse-time `std::exception`s become usage errors (exit code `2`) and never escape as an unhandled exception.
- **BREAKING (output text only):** the human-readable `--help`/usage text layout changes to argparse's format. The CLI command/option contract and exit codes are unchanged; only the formatting of help/usage strings differs.

## Capabilities

### New Capabilities
<!-- No new capabilities: behavior contract is preserved; this is an implementation + dependency change. -->

### Modified Capabilities
- `cli-foundation`: the "Gated, dependency-light build deliverable" requirement currently mandates that the CLI link **only** `libbsa::libbsa` and the C++ standard library and introduce **no new third-party runtime dependency**. It is updated to permit a single header-only command-line argument-parsing dependency (`argparse`) that adds no new runtime/shared-library dependency, while keeping the target gated and otherwise dependency-light.

## Impact

- **Code:** `tools/cli/main.cpp` — remove the hand-rolled parsing block (~lines 47-68, 315-440) and rewrite each `run_*` entry point to read values from `argparse` instead of `parsed_arguments`. Retain `command_line_arguments`, `wide_argument_to_utf8`, `process_exit`, `render_error`, `usage_failure`, `escaped_cli_text`, `format_table`/format validation, `resolve_worker_count`, `parse_compression`, and all libbsa I/O logic.
- **Build:** `CMakeLists.txt` — `find_package(argparse CONFIG REQUIRED)` and link `argparse::argparse` to the `bsa` target inside the existing `if(LIBBSA_BUILD_CLI)` block only.
- **Dependencies:** `vcpkg.json` — add `argparse`. Header-only, MIT-licensed; no new runtime/DLL dependency and no change to the library's own dependency surface (`libdeflate`, `lz4`, `directxtex`).
- **Tests:** any CLI test that asserts on exact `--help`/usage text must be updated to the argparse-formatted output; tests asserting on commands, options, exit codes, and operational diagnostics should continue to pass unchanged.
- **Spec:** `openspec/specs/cli-foundation/spec.md` — the dependency requirement is modified as described above.
