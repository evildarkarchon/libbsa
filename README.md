# libbsa

libbsa is a reusable C++20 library for reading, writing, validating, and extracting Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield.

## Platform Support

libbsa is a Windows-only library. Development, review, CI, packaging, and dependency validation target Windows with MSVC and vcpkg. Do not file review findings or implementation tasks solely to preserve Linux, macOS, POSIX, or cross-platform portability.

## Build

Set `VCPKG_ROOT` to your vcpkg checkout, then use the quick Windows MSVC inner-loop path:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static --output-on-failure
```

Supported Windows-only verification lanes are grouped by role:

- **Debug inner-loop lanes**
  - `windows-msvc-debug-static` — quick day-to-day path.
  - `windows-msvc-debug-shared` — shared-library inner-loop coverage.
- **Release package-proof lanes**
  - `windows-msvc-release-static`
  - `windows-msvc-release-shared`
  - Both Release lanes own the supported install/export proof plus downstream `package_consumer_smoke` coverage inside `ctest`. A package-consumer smoke failure is a Release-lane failure even if the core test binary passed.
- **MSVC AddressSanitizer hardening lane**
  - `windows-msvc-asan-static` — the supported MSVC AddressSanitizer hardening lane for risky parser, writer, compression, and validation changes before shipping.

The supported matrix stays Windows-only and runnable from checked-in presets plus repository-controlled automation. Tests tagged `requires-game-fixture` remain opt-in local-corpus checks and are skipped by default when no local fixture path is configured.

## Command-Line Tool

Set `LIBBSA_BUILD_CLI=ON` to build the first-party `bsa` executable. The option is enabled by default for first-party smoke coverage and can be disabled when only the library target is needed.

```powershell
cmake --preset windows-msvc-debug-static -DLIBBSA_BUILD_CLI=ON
cmake --build --preset windows-msvc-debug-static --target bsa
```

Subcommands:

- `bsa pack --format <token> [--compress default|raw|compressed] [--overwrite] <input-dir> <output-archive>` creates a new archive from a directory.
- `bsa unpack [--path <archive-path>]... [--overwrite] <archive> <output-dir>` extracts all entries or selected archive paths.
- `bsa list [--details] <archive>` prints entries in reader order.
- `bsa info <archive>` prints archive metadata.
- `bsa validate [--strict] <archive>` prints validation diagnostics and compatibility warnings.

Supported `--format` tokens are `bsa-tes3`, `bsa-oblivion`, `bsa-fo3`, `bsa-sse`, `ba2-gnrl-fo4`, `ba2-gnrl-sf-v2`, `ba2-gnrl-sf-v3`, `ba2-dx10-fo4`, and `ba2-dx10-sf-v3`.

Exit codes are stable categories: `0` for success, `1` for operational failures such as archive or host I/O errors, and `2` for usage errors.

## Reference Boundary

`TES5Edit/` is a read-only reference submodule for BSArchPro-compatible behavior. Do not edit, format, stage, compile, or copy generated outputs into that submodule.
