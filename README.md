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

## Phase 02 streaming, detection, path, and metadata APIs

Phase 02 adds public reusable contracts for later archive readers without adding full archive parsing or extraction behavior:

- `include/libbsa/io.hpp` exposes `byte_source`, `byte_sink`, `memory_source`, and `memory_sink` for bounded random-access reads and caller-provided writes.
- `include/libbsa/archive.hpp` exposes `archive_summary`, `entry_metadata`, archive identity, and compression-state metadata values.
- `include/libbsa/detect.hpp` exposes `detect_archive` for bounded header detection of TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DDS identities.
- `include/libbsa/archive_path.hpp` exposes `archive_path` and `normalize_archive_path` for owned archive-virtual paths independent of host filesystem rules.
- `include/libbsa/archive_view.hpp` exposes `archive_view` for metadata-only path listing, `contains`, and result-returning entry lookup.

The exact TES3, TES4-family, and FO4/BA2 hash functions remain internal/test-visible in `src/hash.hpp` and are locked by golden-vector tests. The vector groups in `tests/path_hash_tests.cpp` include inline provenance comments pointing to the read-only `TES5Edit/Core/wbBSArchive.pas` reference routines.

Local Phase 02 verification used the Visual Studio 2026 fallback build directory established during Phase 1 verification:

```powershell
cmake --build build/local-vs2026-vcpkg --config Debug
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L golden
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L smoke
```

Phase 02 does not implement full table parsing, payload extraction, decompression through libdeflate or LZ4, DDS reconstruction, writers, CLI, GUI, or a public file source. Those behaviors are intentionally left to later format-specific and writer phases.

## TES5Edit/ reference boundary

`TES5Edit/` is a read-only reference submodule. It documents prior BSArchPro-compatible behavior, but it is not vendored source for libbsa.

Do not edit, format, stage, compile, link, or vendor files from `TES5Edit/`. Do not update the submodule pointer as part of library implementation work. All implementation belongs outside `TES5Edit/`.
