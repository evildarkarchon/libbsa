## 1. Dependency and build wiring

- [x] 1.1 Add `"argparse"` to the `dependencies` array in `vcpkg.json`.
- [x] 1.2 In `CMakeLists.txt`, inside the existing `if(LIBBSA_BUILD_CLI)` block, add `find_package(argparse CONFIG REQUIRED)` and add `argparse::argparse` to the `bsa` target's `PRIVATE` link libraries.
- [x] 1.3 Configure with `-DLIBBSA_BUILD_CLI=ON` and confirm `argparse` resolves and the `bsa` target builds (clean configure picks up the new vcpkg dependency).
- [x] 1.4 Configure with `-DLIBBSA_BUILD_CLI=OFF` and confirm the library and tests still build without requiring `argparse`.

## 2. Parser construction and dispatch skeleton

- [x] 2.1 Add `#include <argparse/argparse.hpp>` to `tools/cli/main.cpp`.
- [x] 2.2 Build a single setup/dispatch scope that constructs the top-level `argparse::ArgumentParser program("bsa", <version>, argparse::default_arguments::none)` and one subparser local per command (`pack`, `unpack`, `list`, `info`, `validate`), registering each with `program.add_subparser(...)`. Keep all parser objects as named locals in one scope so they outlive `parse_args` (argparse parsers are non-copyable/non-movable since v3.0).
- [x] 2.3 Add explicit `-h/--help` flags to the top-level parser and every subparser, and `-V/--version` to the top-level parser only (do not use argparse's default `-v` version flag).
- [x] 2.4 Feed the existing UTF-8 argument vector from `command_line_arguments()` into `program.parse_args(const std::vector<std::string>&)` with element 0 set to the program name; do NOT use the `parse_args(argc, argv)` overload.
- [x] 2.5 Wrap `parse_args` in `try/catch (const std::exception&)`; on throw, print `usage error: <what()>` plus the relevant usage text and return `process_exit::usage_error` (exit code 2).
- [x] 2.6 Implement centralized help/version handling: `--version` prints the version and returns success; top-level `--help` with no subcommand prints top-level help and returns success; a used subcommand with its `--help` set prints that subparser's help and returns success.
- [x] 2.7 Preserve the unknown-subcommand behavior: when the first non-option token is not a known command and not a global option, print the existing `unknown subcommand` diagnostic plus top-level usage and return exit code 2.
- [x] 2.8 Route a used subcommand to its existing `run_*` handler (keep the name→handler dispatch mapping).

## 3. Per-subcommand argument definitions

- [x] 3.1 `pack`: define `--format` (string), `--compress` (string, default `default`), `-j/--threads` (string, default `auto`), `--overwrite` (flag), and two positionals (`<input-dir> <output-archive>`).
- [x] 3.2 `unpack`: define `--path` (`.append()`, repeatable), `-j/--threads` (string, default `auto`), `--overwrite` (flag), and two positionals (`<archive> <output-dir>`).
- [x] 3.3 `list`: define `--details`/`--detail` (flag, both spellings accepted) and one positional (`<archive>`).
- [x] 3.4 `info`: define one positional (`<archive>`), no other options.
- [x] 3.5 `validate`: define `--strict` (flag) and one positional (`<archive>`).
- [x] 3.6 Embed the substrings the integration test checks into argument help text: the `--threads` help must contain `-j <value>`, `1..1024`, `auto`, and `0 for auto`; the `pack` `--format` help or epilog must enumerate all nine format tokens (`bsa-tes3`, `bsa-oblivion`, `bsa-fo3`, `bsa-sse`, `ba2-gnrl-fo4`, `ba2-gnrl-sf-v2`, `ba2-gnrl-sf-v3`, `ba2-dx10-fo4`, `ba2-dx10-sf-v3`).

## 4. Port subcommand handlers (preserve semantics)

- [x] 4.1 Rewrite `run_pack` to read values from its subparser; keep manual validation: required `--format` (missing → `requires --format`, exit 2), `format_table` lookup (unknown → diagnostic listing `valid tokens`, exit 2), `parse_compression` (unsupported pairings still emit `does not support compressed` / `does not support raw`, exit 2), `resolve_worker_count` (invalid → `invalid --threads value`, exit 2), exact 2 positionals (missing → `pack requires`, exit 2). Preserve UTF-8 path decoding and all libbsa I/O.
- [x] 4.2 Rewrite `run_unpack` to read its subparser; preserve `--path` repeatable selection (`get<std::vector<std::string>>`, empty = all entries), `resolve_worker_count`, `--overwrite` refusal diagnostic (`--overwrite`, exit 1), atomic handle-based extraction, reparse/traversal guards, and exact 2 positionals.
- [x] 4.3 Rewrite `run_list` to read `--details`/`--detail` and one positional; keep `escaped_cli_text(entry.path)` rendering and `raw=`/`stored=`/`compression=` detail output.
- [x] 4.4 Rewrite `run_info` to read one positional; output unchanged.
- [x] 4.5 Rewrite `run_validate` to read `--strict` and one positional; preserve `valid:`/`warning:`/strict-failure behavior (exit 1 on strict warnings).
- [x] 4.6 Confirm operational errors still flow through `render_error` and return `process_exit::operational_failure` (exit 1), and that no libbsa error escapes as an exception.

## 5. Remove the hand-rolled parser

- [x] 5.1 Delete `option_kind`, `option_spec`, `parsed_option`, `parsed_arguments` (`main.cpp:47-68`).
- [x] 5.2 Delete `parse_options`, `find_option_spec`, `has_flag`, `values_for`, `single_value_for` (`main.cpp:315-440`) and any now-unused helpers/includes.
- [x] 5.3 Confirm `command_line_arguments`, `wide_argument_to_utf8`, `process_exit`, `render_error`, `usage_failure`, `escaped_cli_text`, `format_table`, `resolve_worker_count`, and `parse_compression` are retained.

## 6. Update affected tests

- [x] 6.1 In `tests/cli/cli_integration.cmake`, update line 249's source-text assertion `require_match_count(... "option_spec\{\"threads\", option_kind::value, 'j'\}" 2 ...)` to reflect the argparse-based implementation (assert the new threads-argument wiring or remove the obsolete white-box check) — do not retain the removed `option_spec` type to satisfy a test.
- [x] 6.2 Review the remaining source-text assertions (lines 244-313) and update only those that reference removed parser internals; keep the UTF-8, escaping, handle-based-destination, and permission-denied negative assertions intact (they should remain satisfied by the preserved code).
- [x] 6.3 Keep all behavioral assertions (exit codes, stdout/stderr substrings, round-trips, UTF-8 host/entry paths, threads, compression, selective extraction, traversal/reparse guards) unchanged and ensure they pass.
- [x] 6.4 Grep the rest of `tests/` for any other assertions on exact CLI usage/help text and update them to the argparse-formatted output where needed.

## 7. Build, run, and verify

- [x] 7.1 Build the `bsa` target and the test suite with `LIBBSA_BUILD_CLI=ON`.
- [x] 7.2 Run the full CTest suite (including the `cli-integration` test) and confirm it passes.
- [x] 7.3 Manually verify the contract: `bsa --help` (lists subcommands, exit 0), `bsa -V`/`--version` (contains `libbsa`, exit 0), `bsa <cmd> --help` (per-command options, exit 0), unknown subcommand (exit 2), missing/invalid `--format` (exit 2), invalid `--threads` including `-j -1` and `--threads=` (exit 2), and operational failures (exit 1).
- [x] 7.4 Inspect the `bsa` target's runtime/shared-library dependencies and confirm `argparse` adds no runtime/DLL dependency (header-only) and that no dependency beyond those already required by libbsa is introduced.

## 8. Documentation

- [x] 8.1 Add/refresh Doxygen-style doc comments on the rewritten dispatch and `run_*` functions, and add a short comment noting the argparse parser-lifetime constraint (non-copyable/non-movable; built and parsed in one scope) and the exception→usage-error translation boundary.
- [x] 8.2 Note in the commit message that help/usage text formatting changed (argparse) while the command/option contract and exit codes are unchanged, per the format-compatibility documentation rule.
