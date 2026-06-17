## Context

libbsa already exposes everything a command-line tool needs through stable public headers: `archive_reader` for open/list/inspect/extract, the writer family (`tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, `ba2_dx10_writer`) for create-new packing, and `validate_archive` for diagnostics. There is currently no executable that drives these, so contributors and downstream users write ad-hoc harnesses. This design adds a thin CLI that is a pure consumer of the public API — it must not reach into `src/` internals or the `TES5Edit/` reference.

Constraints carried from the project:
- Windows-only, C++20, MSVC + vcpkg, built from the existing CMake presets.
- Minimal dependencies: standard library only for the CLI itself; no new third-party runtime dependency.
- Public-API-only seam: if the CLI needs behavior the public API cannot express, that is a signal to file a separate library change, not to bypass the headers.

## Goals / Non-Goals

**Goals:**
- A single executable (working name `bsa`) with `pack`, `unpack`, `list`, `info`, and `validate` subcommands.
- Explicit, discoverable `--format` selection that maps one token to exactly one writer + target profile.
- Path-traversal-safe extraction, conservative overwrite defaults, and stable exit codes.
- Faithful rendering of `libbsa::error` and validation diagnostics; no uncaught exceptions reaching the OS.
- CTest-driven integration tests (including pack → unpack round-trips) runnable from existing presets.

**Non-Goals:**
- In-place archive editing, append, or deletion (the library is open/read/write-new).
- Auto-detecting the pack target from extension or content (explicitly rejected; `--format` is required).
- A general scripting/JSON output contract or a stable machine-readable schema (human-readable output for this change; structured output can be a later change).
- Parallel pack/extract tuning beyond defaults; the CLI may pass `worker_count = 1` and defer parallelism to a later change.
- Cross-platform packaging or a separate installable distribution.

## Decisions

### Deliverable shape: gated executable in this build

Add `tools/cli/` with a small set of translation units and a new `add_executable` target in the root `CMakeLists.txt`, gated behind `option(LIBBSA_BUILD_CLI ...)` and linking only `libbsa::libbsa` (which transitively carries the system libs libbsa already needs). The target uses `cxx_std_20` and the same warning flags as the library.

- **Why:** Keeps the tool versioned and tested with the library, and the only integration seam is the public target `libbsa::libbsa`.
- **Alternative considered:** A separate repository/project — rejected as overhead for a first-party utility that should track the library version exactly.
- **Default for the option:** Leaning `ON` for inner-loop convenience, but final default is decided in tasks; either way it must be toggleable and excluded when `OFF`.

### Argument parsing: hand-rolled, no dependency

Implement a minimal argument parser over `int argc, char** argv`: dispatch on the first positional (subcommand), then a small option loop per subcommand supporting `--flag`, `--key value`, `--key=value`, `-h`, repeated `--path` values, and `--` to end option parsing. Unknown options and missing required values are usage errors.

- **Why:** The option surface is small and the project policy prefers the standard library until a real need justifies a dependency. Avoids pulling CLI11/argparse into a minimal-dependency library repo.
- **Alternative considered:** A third-party arg parser (CLI11) — better ergonomics but violates the minimal-dependency constraint for marginal benefit at this surface size.

### `--format` token taxonomy

One token selects one writer + target. Tokens are prefixed by writer family for discoverability:

| `--format` token   | Writer              | Target / mode                         |
|--------------------|---------------------|---------------------------------------|
| `bsa-tes3`         | `tes3_bsa_writer`   | Morrowind, raw/uncompressed           |
| `bsa-oblivion`     | `tes4_bsa_writer`   | `tes4_bsa_target::oblivion`           |
| `bsa-fo3`          | `tes4_bsa_writer`   | `tes4_bsa_target::fallout3`           |
| `bsa-sse`          | `tes4_bsa_writer`   | `tes4_bsa_target::skyrim_se`          |
| `ba2-gnrl-fo4`     | `ba2_gnrl_writer`   | `ba2_gnrl_target::fallout4`           |
| `ba2-gnrl-sf-v2`   | `ba2_gnrl_writer`   | `ba2_gnrl_target::starfield_v2`       |
| `ba2-gnrl-sf-v3`   | `ba2_gnrl_writer`   | `ba2_gnrl_target::starfield_v3`       |
| `ba2-dx10-fo4`     | `ba2_dx10_writer`   | `ba2_dx10_target::fallout4`           |
| `ba2-dx10-sf-v3`   | `ba2_dx10_writer`   | `ba2_dx10_target::starfield_v3`       |

The token list, its writer mapping, and the writer's supported options are defined in one table so help text, validation, and dispatch stay in sync. An unknown token prints the valid set and exits with the usage-error code.

- **Why:** Explicit tokens remove the Oblivion-vs-FO3-vs-Skyrim and FO4-vs-Starfield ambiguity that extension/content inference cannot resolve, satisfying the packing spec's "no inference" requirement.

### Compression / overwrite option mapping

The CLI maps generic flags onto the per-writer options structs:
- `--overwrite` → each writer options' `overwrite_existing`.
- `--compress {default|raw|compressed}` → `archive_compression_policy` for `tes4_bsa_writer` (`compression_policy`) and `ba2_gnrl_writer` (`compression`).
- `bsa-tes3` is raw-only and `ba2-dx10-*` is compressed-only; passing `--compress` with a value the selected writer cannot honor is a **usage error** (per the packing spec), not a silent no-op.
- Starfield GNRL/DX10 header compatibility fields (`starfield_unknown1/2`, `starfield_compression_method`) keep their library defaults in this change; exposing them is deferred.

- **Why:** Surfaces the meaningful knobs while keeping the option grammar uniform across writers and refusing combinations that would mislead the user.

### Extraction architecture: sink + sink factory

Implement a `payload_sink` that writes accepted bytes to a host `std::ofstream` and returns the full accepted count (partial acceptance is treated as `io_error` by the library). Drive extraction through `archive_reader::extract_entries` with a `bulk_extract_sink_factory` that, in `create(path, entry)`:
1. Computes the destination from the output root + entry path.
2. Performs the path-safety check (below); returns an `error` to record a per-entry failure on traversal.
3. Enforces the overwrite policy (returns an error when the destination exists and `--overwrite` is off).
4. Creates intermediate directories, then returns a file-writing sink.

Full extraction builds the request list from `entries()`; selective extraction builds it from user `--path` values. `extract_entries` records per-entry failures without aborting siblings, which directly matches the extraction spec; the CLI aggregates failures into the exit code. `worker_count` stays `1` for this change.

- **Why:** A single code path serves both full and selective extraction and gives spec-aligned per-entry failure semantics for free.
- **Alternative considered:** Looping `extract`/`extract_bytes` per entry — simpler but duplicates failure aggregation and loses the library's coalescing/parallel-ready structure.

### Path-traversal safety

Destinations are validated by normalizing `output_root` to an absolute, lexically-normal path and resolving `output_root / entry_path` the same way, then verifying the result is lexically within `output_root`. Entries containing `..` segments that escape, absolute components, or (on Windows) drive/UNC roots are rejected before any file is created.

- **Why:** Archive-internal paths are untrusted input; extraction must never write outside the chosen directory. Using lexical normalization avoids resolving symlinks on a path that should not exist yet.

### Packing path mapping

Pack walks the input directory with `std::filesystem::recursive_directory_iterator` over regular files, computes each file's path relative to the input root, and converts separators to `/` to form the archive-internal path (the input directory's own name is not included). Entries are added via `writer.add_file(archive_path, host_path, ...)`; DX10 uses `add_file(archive_path, dds_host_path)`. Finalization calls `write_to(output_path)`.

- **Why:** Produces stable, host-independent archive keys consistent with how libbsa treats archive paths as normalized keys rather than filesystem paths.

### Error rendering and exit codes

Centralize a `render(error)` helper that prints `error_code` category + message to `stderr`, and define exit codes: `0` success, `2` usage error, `1` operational failure. `main` wraps subcommand execution so any stray exception (e.g., a `result` misuse) is caught and converted to an operational failure rather than crashing.

- **Why:** Satisfies the foundation spec's stable exit-code and "never crash on a library error" requirements and gives scripts a dependable contract.

### Testing approach

Add CTest integration tests under `tests/` that invoke the built CLI binary (via the target's path) against small fixtures:
- Round-trip: `pack` a fixture directory with each applicable `--format`, then `unpack` and compare bytes.
- `list`/`info`/`validate` output assertions on known fixtures.
- Failure paths: unknown subcommand, unknown `--format`, missing args, overwrite refusal, traversal refusal, missing selective path.
DX10 round-trips use small DDS fixtures already exercised by the library tests where available; otherwise they are gated like other fixture-dependent tests.

- **Why:** Proves the CLI end-to-end against the real library and locks the exit-code and output contracts the specs define.

## Risks / Trade-offs

- **Hand-rolled parser drift / inconsistency** → Keep one option-spec table per subcommand that drives both parsing and `--help`; cover the grammar (`--k v`, `--k=v`, repeated `--path`, `--`) with unit tests.
- **Path-safety gaps on Windows (drive-relative, UNC, reserved names, long paths)** → Centralize validation in one helper, test traversal/absolute/drive cases explicitly, and reject anything that does not resolve strictly within the output root.
- **`--format` ↔ writer-option mismatch** (e.g., `--compress` on a raw-only target) → Encode each token's supported options in the format table and return usage errors for unsupported combinations instead of ignoring them.
- **DX10 fixtures may be scarce** → Reuse existing library DDS fixtures; gate DX10 round-trip tests behind the same opt-in mechanism as other fixture-dependent tests so the default lane stays green.
- **CLI exit codes / output become a de facto contract** → Treat exit-code categories and the diagnostic format as the spec'd surface; keep richer machine-readable output for a future change rather than overcommitting now.
- **Default value of `LIBBSA_BUILD_CLI` affecting CI time** → Make it a single option, decide the default in tasks, and ensure the disabled path is tested so the matrix is unaffected if turned off.

## Open Questions

- Final executable name (`bsa` vs `libbsa-cli`) and default value of `LIBBSA_BUILD_CLI` — resolved during implementation; neither affects the spec'd behavior.
- Whether `validate --strict` (warnings-as-failures) ships in this change or is deferred — the inspection spec includes it as optional; implement if low-cost, otherwise split out.
- Whether to expose Starfield header-compatibility fields and `worker_count` now — deferred to a follow-up unless a test fixture requires them.
