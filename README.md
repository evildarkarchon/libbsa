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

## Reference Boundary

`TES5Edit/` is a read-only reference submodule for BSArchPro-compatible behavior. Do not edit, format, stage, compile, or copy generated outputs into that submodule.
