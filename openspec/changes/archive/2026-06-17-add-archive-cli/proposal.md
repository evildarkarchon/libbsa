## Why

libbsa exposes a complete read/write/validate/extract C++ API, but there is no first-party way to exercise those formats from a terminal. Modders, asset-pipeline authors, and the project's own contributors currently have to write throwaway C++ harnesses to pack a folder into a BSA/BA2 or to unpack one for inspection. A small, dependency-light command-line front-end makes the supported formats usable directly, gives the library a runnable end-to-end smoke surface, and demonstrates the public API as the only integration seam.

## What Changes

- Add a new command-line executable (working name `bsa`) built as a CMake target that links only `libbsa::libbsa` and the C++ standard library.
- Add a `pack` subcommand that creates a new archive from a host directory, selecting the writer and target profile through an explicit `--format` flag (no silent inference).
- Add an `unpack` subcommand that extracts an archive to a host directory, supporting full extraction and selective extraction by archive path, with path-traversal-safe output writing.
- Add `list`, `info`, and `validate` subcommands for read-only inspection built on `archive_reader` and `validate_archive`.
- Define stable CLI conventions: subcommand dispatch, `--help`/`--version`, the `--format` target taxonomy, structured human-readable diagnostics mapped from `libbsa::error_code`, and process exit-code categories.
- Gate the tool behind a CMake option (working name `LIBBSA_BUILD_CLI`) and add CTest-driven CLI integration tests; do not add any new third-party runtime dependency.

## Capabilities

### New Capabilities
- `cli-foundation`: Executable entry point, subcommand dispatch, global options (`--help`, `--version`), the `--format` target taxonomy, mapping of `libbsa` errors to human-readable diagnostics, exit-code conventions, and the gated CMake/CTest deliverable shape.
- `cli-archive-packing`: The `pack` subcommand — directory walking, archive-internal path mapping, explicit format/target selection across the TES3, TES4-family, BA2 GNRL, and BA2 DX10 writers, and exposed compression/overwrite options.
- `cli-archive-extraction`: The `unpack` subcommand — full and selective extraction to a host directory, output path safety, and per-entry result reporting.
- `cli-archive-inspection`: The `list`, `info`, and `validate` subcommands — deterministic entry listing, archive-level metadata summary, and structured validation diagnostics and compatibility warnings.

### Modified Capabilities
<!-- None. The CLI consumes existing public libbsa APIs (archive_reader, the writer family, validate_archive) without changing their requirements. -->

## Impact

- **New code**: A CLI source tree (working location `tools/cli/`) and a new executable target in the root `CMakeLists.txt`, gated by `LIBBSA_BUILD_CLI`.
- **Public API consumed (unchanged)**: `libbsa::archive_reader` (`open`, `metadata`, `entries`, `find`, `extract`, `extract_entries`), the writer family (`tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, `ba2_dx10_writer`), and `libbsa::validate_archive`. If the CLI needs behavior the public API cannot express, that is a signal to file a separate library change rather than reaching into internals.
- **Dependencies**: None added. Argument parsing and console I/O use the C++ standard library only, consistent with the project's minimal-dependency policy.
- **Build/test**: New CTest integration tests for the CLI under `tests/`, runnable from the existing Windows MSVC presets; no change to the supported preset matrix.
- **Platform**: Windows-only, consistent with the rest of libbsa.
