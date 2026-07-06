## Context

The `bsa` CLI lives entirely in `tools/cli/main.cpp` (~2,110 lines). Argument parsing today is a hand-rolled engine:

- Types `option_kind`, `option_spec`, `parsed_option`, `parsed_arguments` (`main.cpp:47-68`).
- `parse_options()` (`main.cpp:315-413`) tokenizes long/short options, `--name=value`, the `--` terminator, repeatable options, and value/flag validation, returning `libbsa::result<parsed_arguments>`.
- Accessors `has_flag`, `values_for`, `single_value_for` (`main.cpp:415-440`).
- Each `run_*` subcommand (`run_pack`, `run_unpack`, `run_list`, `run_info`, `run_validate`) parses with a per-command `option_spec` span, then performs semantic validation and libbsa I/O.

Surrounding infrastructure that is **not** parsing and must be preserved:

- `command_line_arguments()` / `wide_argument_to_utf8()` (`main.cpp:612-674`): on Windows, ignores the CRT `argv` and re-parses the authoritative UTF-16 command line via `GetCommandLineW`/`CommandLineToArgvW`, converting to UTF-8 to honor libbsa's UTF-8 host-path contract.
- `process_exit` (`0` success, `1` operational failure, `2` usage error), `render_error`, `usage_failure`, `escaped_cli_text`.
- `format_table` + format-token validation, `resolve_worker_count` (`auto`/`0`/`1..1024`), `parse_compression`.
- The library `result`/`error` model and the rule that no libbsa error may surface as an unhandled exception.

`argparse` (p-ranav) is a single-header, MIT-licensed, C++17 library available in vcpkg as `argparse` (CMake target `argparse::argparse`, an INTERFACE/header-only target). It supports subcommands via `add_subparser`, repeatable options via `.append()`, name aliases, and per-parser help, and it throws `std::exception` subclasses on parse errors.

## Goals / Non-Goals

**Goals:**
- Delete the hand-rolled parsing engine and express the grammar declaratively with `argparse`.
- Preserve the externally observable CLI contract: subcommand set, option spellings, positional counts, global `-h/--help` and `-V/--version`, exit-code categories, and operational-error diagnostics.
- Keep the Windows UTF-8 argument-acquisition path and feed `argparse` the existing UTF-8 vector — never the CRT `argv`.
- Keep all parse-time and semantic failures on the CLI's centralized exit path (no `std::exit`, no uncaught exceptions).
- Add only a header-only dependency; introduce no new runtime/shared-library dependency.

**Non-Goals:**
- No change to libbsa library APIs, archive formats, or compression behavior.
- No new subcommands, options, or output data fields (help/usage **text layout** may change; this is expected).
- No change to the library's own dependency surface (`libdeflate`, `lz4`, `directxtex`).
- No move to C++23 or `std::expected`.

## Decisions

### Decision 1: One top-level `ArgumentParser` with one subparser per command

Build a top-level parser `bsa` and register `pack`, `unpack`, `list`, `info`, `validate` via `add_subparser`. Dispatch uses `program.is_subcommand_used("<name>")` and retrieves the chosen subparser to read values. Rationale: matches the existing subcommand model directly and yields per-subcommand `--help` for free.

*Alternative considered:* keep manual first-token dispatch and use a fresh `ArgumentParser` per subcommand. Rejected — `add_subparser` already gives unified usage and per-command help without bespoke routing.

### Decision 2: `argparse` does syntax; the CLI keeps semantic validation

`argparse` handles tokenization, long/short options, `--name=value`, `--`, repeatable options, and unknown-option rejection. The CLI retains its existing semantic checks in each `run_*` handler: required `--format` presence and `format_table` lookup, exact positional counts (2 for `pack`/`unpack`, 1 for `list`/`info`/`validate`), `resolve_worker_count` range/`auto` handling, and `parse_compression`.

Crucially, arguments are **not** marked `argparse`-`.required()`. Required-ness is validated manually after parse. Rationale: an argparse `.required()` argument throws *before* we can honor a `--help` request (e.g. `bsa pack --help` with no `--format`). Keeping presence checks manual preserves both the existing usage diagnostics and exit code `2`, and lets help short-circuit cleanly. This also matches the current code, which already validates positional counts and option constraints by hand.

*Alternative considered:* push everything into argparse (`.required()`, `.choices()`, positional `nargs`). Rejected — it conflicts with help short-circuiting and would change diagnostic wording and exit codes.

### Decision 3: Centralized help/version handling, no `std::exit`

Construct all parsers with `argparse::default_arguments::none` and add explicit flags: `-h/--help` on the top level and every subparser, `-V/--version` on the top level only. After parsing, a single code path decides:
- `--version` set → print version (existing `print_version`), return `success`.
- top-level `--help` with no subcommand → print top-level help, return `success`.
- subcommand used with its `--help` set → print that subparser's help, return `success`.
- otherwise dispatch to the `run_*` handler.

Rationale: avoids `argparse`'s default help/version action, which calls `std::exit(0)` and skips destructors, and which binds version to `-v` (lowercase) rather than the contract's `-V`. Manual handling keeps a single exit path through `process_exit` and preserves `-V/--version`. Help text is emitted with `parser.help().str()` to `std::cout`.

*Alternative considered:* `default_arguments::help` with `exit_on_default_arguments = true` for automatic help. Rejected — `std::exit` bypasses RAII/cleanup and the project's centralized exit discipline, and it would still require a custom `-V` because the default version flag is `-v`.

### Decision 4: Translate `argparse` exceptions to usage errors at the parse boundary

Wrap `program.parse_args(args)` in a `try/catch (const std::exception&)`. On throw, print `usage error: <what()>` plus the relevant usage text and return `process_exit::usage_error` (`2`). Rationale: `argparse` reports parse failures by throwing; the CLI contract forbids unhandled exceptions and requires usage errors to exit with code `2`. The existing top-level `catch (...)` in `main` remains as a last-resort safety net.

### Decision 5: Feed `argparse` the UTF-8 vector, not the CRT `argv`

Keep `command_line_arguments()` and call the `parse_args(const std::vector<std::string>&)` overload, where element `0` is the program name (`"bsa"`). Rationale: preserves the Windows UTF-16→UTF-8 contract; using `parse_args(argc, argv)` would reintroduce non-UTF-8 CRT arguments.

### Decision 6: Option-to-argparse mapping

| CLI option | argparse configuration | Post-parse handling |
|---|---|---|
| `--format <token>` (pack) | `add_argument("--format")`, default unset | manual `format_table` lookup; missing/unknown → usage error |
| `--compress <default\|raw\|compressed>` | `add_argument("--compress").default_value(std::string("default"))` | existing `parse_compression`; keep diagnostic wording |
| `--threads`, `-j <value>` | `add_argument("-j","--threads").default_value(std::string("auto"))` | existing `resolve_worker_count` (`auto`/`0`/`1..1024`) |
| `--overwrite`, `--strict` | `.flag()` | boolean read |
| `--path <p>` (unpack, repeatable) | `add_argument("--path").append()` | `get<std::vector<std::string>>`, empty = all entries |
| `--details` / `--detail` (list) | `add_argument("--details","--detail").flag()` | boolean read; both spellings accepted |
| positionals | `.remaining()` / `nargs`, gathered as `std::vector<std::string>` | manual exact-count check (2 or 1) |

Rationale: keeps `--threads` accepting `auto` (argparse `.scan` cannot), preserves both `--details`/`--detail` spellings, and preserves existing compression/format diagnostics.

### Decision 7: Build wiring scoped to the gated CLI target

In `CMakeLists.txt`, inside the existing `if(LIBBSA_BUILD_CLI)` block, add `find_package(argparse CONFIG REQUIRED)` and `target_link_libraries(bsa PRIVATE argparse::argparse)`. Add `"argparse"` to `vcpkg.json`. Rationale: the dependency is needed only when the CLI is built; the library and tests are unaffected.

## Risks / Trade-offs

- **`argparse::ArgumentParser` is non-copyable/non-movable since v3.0** → All subparsers must be constructed as named locals and registered with `add_subparser` within a single scope that also performs the parse and dispatch. The parser objects must outlive `parse_args`. Mitigation: build, parse, and dispatch inside one function (or hold parsers in a container of stable references); do not return parsers by value.

- **Help/usage text format changes** → Any test or doc asserting on exact usage/help strings will break. Mitigation: grep tests for usage assertions during implementation and update golden text; the spec scenarios assert on *content* (lists subcommands, enumerates format tokens), not exact layout.

- **Unknown-subcommand diagnostic wording** → `argparse`'s error for an unrecognized first token may not name the subcommand the way the current CLI does (spec: "prints a diagnostic naming the unknown subcommand"). Mitigation: keep a lightweight pre-dispatch check that, when the first non-option token is not a known command and not a global option, emits the existing "unknown subcommand" diagnostic and top-level usage with exit code `2`.

- **Accidental exposure of argparse's default `-v`** → Would violate the `-V` contract. Mitigation: use `default_arguments::none` and define `-V/--version` explicitly; assert in tests that `bsa -V`/`--version` works.

- **Format-token discoverability in help** → argparse won't auto-list `format_table` tokens. Mitigation: enumerate tokens in the `--format` argument `help(...)` text or the `pack` subparser epilog so `bsa pack --help` still lists them (spec requirement).

- **vcpkg version drift** → Mitigation: rely on the committed builtin baseline; pin via `version>=`/`overrides` only if a concrete incompatibility appears. argparse is header-only, lowering ABI risk.

## Migration Plan

1. Add `argparse` to `vcpkg.json`; add `find_package` + link in the gated CLI block; confirm a clean configure/build with `LIBBSA_BUILD_CLI=ON` and that `LIBBSA_BUILD_CLI=OFF` still builds without argparse.
2. Introduce the top-level parser + subparsers and the centralized help/version/dispatch path, feeding the existing UTF-8 argument vector.
3. Port each `run_*` handler to read from its subparser while keeping its semantic validation and libbsa I/O unchanged.
4. Delete `option_kind`/`option_spec`/`parsed_option`/`parsed_arguments`/`parse_options`/`find_option_spec`/`has_flag`/`values_for`/`single_value_for`.
5. Update any usage-text tests; verify exit codes for success/usage/operational paths.

Rollback: revert the `vcpkg.json`, `CMakeLists.txt`, and `tools/cli/main.cpp` changes; no persisted state or format change is involved.

## Open Questions

- Are there existing tests (unit or script-based) that assert on exact `--help`/usage output? (Resolve by grepping `tests/` during implementation; reflected as a task.)
- Should positional-count validation use argparse `nargs` or remain manual? (Leaning manual for diagnostic/exit-code parity; revisit if argparse messages prove acceptable.)
