# libbsa

libbsa is a reusable C++20 library for reading and writing Bethesda Game Studios archive formats. The library is intended for modding tools, asset pipelines, and game utilities that need archive access without UI coupling or Delphi/BSArchPro implementation details in the public API.

## Build foundation

The primary Windows workflow uses CMake with vcpkg manifest mode:

```powershell
cmake --preset windows-msvc-vcpkg
cmake --build build/windows-msvc-vcpkg --config Debug
ctest --test-dir build/windows-msvc-vcpkg --output-on-failure
ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit
ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L smoke
```

Set `VCPKG_ROOT` before configuring so the preset can resolve `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`.

Public headers live under `include/libbsa/`. Private implementation files and implementation-only headers live under `src/`.

Phase 1 registers one Catch2 foundation executable with the `unit` label and one consumer-style public-header smoke executable with the `smoke` label. The `fixture`, `roundtrip`, `compat`, and `slow` labels are reserved until those test types exist in later phases.

Public headers must not expose libdeflate, LZ4, DirectXTex, Windows SDK, Delphi, UI/tooling, or TES5Edit types. Archive parsing, hash behavior, compression adapters, DDS wrappers, writer behavior, fixture archives, CLIs, GUIs, and global singleton configuration are outside the Phase 1 foundation.

## TES5Edit/ reference boundary

`TES5Edit/` is a read-only reference submodule. It documents prior BSArchPro-compatible behavior, but it is not vendored source for libbsa.

Do not edit, format, stage, compile, link, or vendor files from `TES5Edit/`. Do not update the submodule pointer as part of library implementation work. All implementation belongs outside `TES5Edit/`.
