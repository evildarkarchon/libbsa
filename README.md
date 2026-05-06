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

## TES4-family BSA read and extract

Phase 04 supports opening, listing, lookup, metadata inspection, and extraction for Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 BSA archives. The API is exposed through `include/libbsa/bsa.hpp` and keeps I/O caller-owned by using `byte_source` for archive bytes and `byte_sink` for extracted output.

The reader implements compatibility-sensitive TES4-family behavior while keeping `TES5Edit/` read-only reference material:

- `ARCHIVE_COMPRESS` XOR `FILE_SIZE_COMPRESS` determines whether each entry is actually compressed.
- v103/v104 compressed payloads route through deflate.
- v105 compressed payloads route through LZ4 frame handling.
- v104/v105 embedded filename prefixes are skipped before bytes are written to the caller's sink.
- Public headers continue to avoid private codec, DirectXTex, and TES5Edit implementation details.

Local Phase 04 validation uses the Visual Studio 2026 fallback build directory:

```powershell
cmake --build build/local-vs2026-vcpkg --config Debug
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L fixture
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke
rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex|TES5Edit" include/libbsa
rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt
git status --short TES5Edit
```

## TES3/Morrowind BSA read and extract

Phase 05 extends the same `open_bsa` / `extract_bsa_entry` API to TES3/Morrowind BSA archives. Consumers can open generated or file-backed Morrowind BSA bytes, list normalized archive paths, inspect `entry_metadata`, look up entries with slash or backslash paths, and stream raw payload bytes to a caller-owned `byte_sink`.

TES3 file records store offsets relative to the data section; libbsa exposes absolute payload offsets in entry_metadata after validating the table range. TES3 payloads are raw and do not use TES4-family compression flags or embedded-name prefixes, so extraction reads the validated stored byte range directly through the existing sink contract.

`TES5Edit/` remains read-only reference material for compatibility behavior. It is not compiled, linked, vendored, formatted, staged, or modified by libbsa implementation work.

## BA2 GNRL read and extract

Phase 06 supports BA2 GNRL open, list, lookup, metadata inspection, and single-entry extraction for Fallout 4 versions 1, 7, and 8 plus Starfield versions 2 and 3. The public API lives in `include/libbsa/ba2.hpp` and exposes `ba2_archive`, `open_ba2`, and `extract_ba2_entry` without exposing private codec, DirectXTex, platform, or `TES5Edit/` implementation details.

`ba2_archive` objects are metadata-only views over copied archive summary and entry metadata. They do not retain a payload source, so callers pass a `byte_source` again to `extract_ba2_entry` when extracting bytes to their own `byte_sink`. BA2 GNRL `.dds` entries are treated as ordinary payloads; DX10 texture reconstruction is Phase 7. BA2 writer support, safe disk extraction policy, and bulk extraction orchestration are later/out-of-scope work rather than Phase 06 deliverables.

BA2 GNRL compression routing is record- and archive-version-aware:

- Entries with `PackedSize == 0` are raw payload bytes.
- Compressed Fallout 4 entries and default compressed Starfield entries use deflate.
- Compressed Starfield v3 entries use raw LZ4 block payloads when `CompressionMethod == 3`.

Generated deterministic BA2 fixtures in `tests/ba2_reader_tests.cpp` are the Phase 06 acceptance corpus. They cover the required Fallout 4 and Starfield version matrix, file-table names, raw/deflate/LZ4-block routes, GNRL `.dds` payload transparency, and malformed cases including truncated tables, impossible offsets, mismatched counts, and codec route confusion without relying on external archives or BSArchPro execution.

Local Phase 06 validation uses the Visual Studio 2026 fallback build directory:

```powershell
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L codec
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke
git status --short TES5Edit
```

## BA2 DDS read and extract

Phase 07 supports BA2 DDS / `DX10` open, list, lookup, texture metadata inspection, and single-entry extraction for Fallout 4 DX10 versions 1, 7, and 8 plus Starfield DX10 version 3. The public surface remains `include/libbsa/ba2.hpp`: callers use `open_ba2`, `ba2_archive::entry`, `ba2_archive::texture_metadata`, and `extract_ba2_entry` with caller-owned `byte_source` and `byte_sink` objects.

`texture_metadata` exposes libbsa-owned dimensions, raw DXGI format value, known-name lookup through `dxgi_format_name`, mip count, array/cubemap state, and logical chunk summaries. Public headers still do not expose DirectXTex, Windows SDK, codec headers, or `TES5Edit/` implementation types.

DX10 extraction reads each texture chunk, routes raw/deflate/Starfield method-3 LZ4-block payloads through existing private codec dispatch, reconstructs a complete DDS byte stream, validates it through a private DirectXTex boundary, and writes to the caller sink only after all prior steps succeed. Unsupported-but-readable texture layouts can still be inspected through `texture_metadata`; extraction returns a structured error without partial sink writes when reconstruction or validation is unsafe.

Generated fixture tests are the Phase 07 acceptance corpus. `tests/ba2_dds_fixture_helpers.*` builds deterministic source-reviewable BA2 DDS fixtures for positive and malformed cases, including one-mip, multi-mip, cubemap/array, raw, deflate, Starfield LZ4-block, truncated records, invalid chunks, duplicate normalized names, unsupported codecs, inconsistent mip mapping, and reconstruction failures. Real game archive corpus checks and BSArchPro byte-for-byte comparisons remain later validation work, not a Phase 07 claim.

Local Phase 07 validation uses the Visual Studio 2026 fallback build directory:

```powershell
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_dds_reader_tests|libbsa_ba2_reader_tests|libbsa_dds_reconstruction_tests"
ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke
rg -n "DirectXTex|DXGI_FORMAT|Windows\.h|libdeflate|lz4|TES5Edit" include/libbsa
git status --short TES5Edit
```

## TES5Edit/ reference boundary

`TES5Edit/` is a read-only reference submodule. It documents prior BSArchPro-compatible behavior, but it is not vendored source for libbsa.

Do not edit, format, stage, compile, link, or vendor files from `TES5Edit/`. Do not update the submodule pointer as part of library implementation work. All implementation belongs outside `TES5Edit/`.
