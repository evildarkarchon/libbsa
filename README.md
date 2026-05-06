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

## Compression services

Phase 03 adds libbsa-owned compression routing and payload dispatcher APIs in `include/libbsa/compression.hpp`. The public API exposes archive-aware `compression_algorithm`, `compression_policy`, and resolver functions without exposing native codec headers to consumers.

Routing is explicit and format-aware:

- Deflate is used for compressed TES4/FO3/FNV-family BSA payloads, Fallout 4 BA2 payloads, and Starfield BA2 payloads that use the deflate/default method.
- LZ4 frame is used for Skyrim SE/AE BSA payloads.
- Raw LZ4 block is used for Starfield BA2 v3 payloads when `CompressionMethod == 3`.
- Writer policy supports `archive_default`, `force_compressed`, and `force_raw` where the target archive format supports that state.

The `libdeflate` and LZ4 implementation headers are private implementation details. Public headers under `include/libbsa/` must not include or mention implementation headers such as `libdeflate.h`, `lz4.h`, `lz4frame.h`, `LZ4` APIs, or `DirectXTex` types outside documentation that describes the boundary.

Local Phase 03 validation used the Visual Studio 2026 fallback build directory:

```powershell
cmake --build build/local-vs2026-vcpkg --config Debug
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L codec
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke
rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex" include/libbsa
rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt
git status --short TES5Edit
```

## TES5Edit/ reference boundary

`TES5Edit/` is a read-only reference submodule. It documents prior BSArchPro-compatible behavior, but it is not vendored source for libbsa.

Do not edit, format, stage, compile, link, or vendor files from `TES5Edit/`. Do not update the submodule pointer as part of library implementation work. All implementation belongs outside `TES5Edit/`.
