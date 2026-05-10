# libbsa

libbsa is a reusable C++20 library for reading, writing, validating, and extracting Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield.

## Platform Support

libbsa is a Windows-only library. Development, review, CI, packaging, and dependency validation target Windows with MSVC and vcpkg. Do not file review findings or implementation tasks solely to preserve Linux, macOS, POSIX, or cross-platform portability.

## Build

Set `VCPKG_ROOT` to your vcpkg checkout, then use one of the supported Windows presets:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static --output-on-failure
```

The supported preset set is `windows-msvc-debug-static` and `windows-msvc-debug-shared`.

## Reference Boundary

`TES5Edit/` is a read-only reference submodule for BSArchPro-compatible behavior. Do not edit, format, stage, compile, or copy generated outputs into that submodule.
