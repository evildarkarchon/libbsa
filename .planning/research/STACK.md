# Stack Research

**Domain:** Portable C++20 Bethesda BSA/BA2 archive-format library  
**Project:** libbsa  
**Researched:** 2026-05-05  
**Confidence:** HIGH for build/dependency stack, MEDIUM for cross-platform DirectXTex behavior

## Recommended Stack

### Core Technologies

| Technology | Version / Policy | Purpose | Why Recommended | Confidence |
|------------|------------------|---------|-----------------|------------|
| C++ | C++20 language mode; avoid C++23-only public API until the project intentionally raises the standard | Public API and implementation language | Matches project constraints while giving `std::span`, `std::endian`, `std::jthread`, concepts, and value-oriented interfaces. Do **not** expose `std::expected` yet in the public API because it is C++23, not C++20. Use a local `libbsa::result<T>` or explicit error-code API until a C++23 migration is deliberate. | HIGH |
| CMake | Minimum `3.24`; test current CMake `4.3.x` in CI/presets | Cross-platform build, install/export package, CTest integration | CMake is the standard for reusable C++ libraries and vcpkg integration. `3.24` is old enough for broad tool availability but modern enough for clean target/file-set based installs. Current docs show CMake `4.3.2`, so CI should keep a latest-CMake lane to catch policy changes early. | HIGH |
| vcpkg | Manifest mode with committed `vcpkg.json` and `vcpkg-configuration.json` / `builtin-baseline`; update baseline per milestone | Dependency acquisition and reproducible package graph | Microsoft docs recommend manifest mode for most projects; it isolates dependencies per project and is required for versioning. A committed baseline gives repeatable libdeflate/lz4/DirectXTex/Catch2 versions without vendoring dependencies. | HIGH |
| TES5Edit / BSArchPro | Read-only git submodule/reference only | Behavioral compatibility oracle | Required project constraint. Use `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas` to trace format behavior, but never compile, edit, format, stage, or vendor TES5Edit code. | HIGH |

### Required Runtime Libraries

| Library | Current vcpkg Version / Policy | Purpose | When to Use | Why Recommended | Confidence |
|---------|--------------------------------|---------|-------------|-----------------|------------|
| libdeflate | vcpkg `libdeflate` `1.25#0` (package last updated 2025-11-03); use features `compression` and `decompression`; add `zlib` only if compatibility tests prove Bethesda payloads require zlib-wrapped streams | Deflate compression/decompression for TES4/FO3/Skyrim LE BSA and FO4/SF BA2 deflate payloads | Every deflate read/write path | It is purpose-built for fast whole-buffer DEFLATE, maps directly to archive payload chunks, and is already a hard project requirement. Prefer whole-buffer wrappers that validate expected output size before returning data. | HIGH |
| lz4 | vcpkg `lz4` `1.10.0#0` (package last updated 2024-07-25); link official `lz4::lz4` | LZ4 frame and raw block compression/decompression | SSE BSA LZ4 frame paths; Starfield BA2 v3 raw LZ4 block paths | Official LZ4 provides both `LZ4F_*` frame APIs and `LZ4_*` block APIs. libbsa needs both, and the separate code paths reduce the risk of frame-vs-block corruption. | HIGH |
| DirectXTex | vcpkg `directxtex` `2026-03-31#0` (package last updated 2026-04-01); request core library only unless tools are intentionally needed | DDS metadata parsing, DXGI format interpretation, mip analysis, DDS header reconstruction support | BA2 DDS read/write phases only; keep behind an internal texture-analysis boundary | DirectXTex is maintained by Microsoft, available through vcpkg, and its March 2026 release made mip-level helpers public and improved permissive DDS handling. It is the right choice for Bethesda DDS metadata work, but its platform surface must be isolated because some auxiliary paths are Windows-oriented. | HIGH for Windows, MEDIUM for Linux/macOS path |

### Supporting Development Libraries and Tools

| Tool / Library | Version / Policy | Purpose | When to Use | Why Recommended | Confidence |
|----------------|------------------|---------|-------------|-----------------|------------|
| Catch2 | vcpkg `catch2` `3.14.0#0` (package last updated 2026-04-06) | Unit, fixture, round-trip, and compatibility tests | Start in Milestone 1 | Catch2 is C++-native, lightweight for library tests, integrates with CTest via `catch_discover_tests`, and has current thread-safe assertion support for later parallel phases. Prefer it over GoogleTest unless mocking becomes a real need. | HIGH |
| CTest | Bundled with CMake | Test orchestration | All test phases | Keeps test execution build-system-native and avoids a separate runner dependency. Use labels for `unit`, `fixture`, `roundtrip`, `compat`, and optional `slow`. | HIGH |
| CMakePresets.json | Schema compatible with minimum CMake; include `windows-msvc-vcpkg`, `linux-clang-vcpkg` later | Repeatable configure/build/test workflows | From project foundation | Presets encode toolchain file, triplet, build type, warnings, and sanitizer lanes without requiring developers to memorize configure flags. | HIGH |
| Doxygen | Acquire from system package, GitHub Actions, or vcpkg only when doc generation is added | Public API docs | Milestone 10 or earlier if API stabilizes | Project requires Doxygen-style comments. Generate docs in CI after public headers settle; do not make Doxygen a runtime or library dependency. | MEDIUM |
| Sanitizers | Compiler-provided ASan/UBSan on Clang/GCC; MSVC ASan where practical | Memory safety validation | Parser and decompressor hardening phases | Archive parsers handle untrusted binary data. Sanitizers should run against malformed fixtures and fuzz seeds, but should not affect release artifacts. | HIGH |

## Installation / Baseline Shape

Use vcpkg manifest mode, not global/classic installs. Recommended initial manifest shape:

```json
{
  "name": "libbsa",
  "version-semver": "0.1.0",
  "dependencies": [
    { "name": "libdeflate", "features": ["compression", "decompression"] },
    "lz4",
    "directxtex",
    "catch2"
  ],
  "builtin-baseline": "<commit from `vcpkg x-update-baseline --add-initial-baseline`>"
}
```

Recommended CMake dependency links:

```cmake
cmake_minimum_required(VERSION 3.24)
project(libbsa VERSION 0.1.0 LANGUAGES CXX)

add_library(libbsa)
target_compile_features(libbsa PUBLIC cxx_std_20)

find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)

target_link_libraries(libbsa
  PRIVATE
    $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
    lz4::lz4
    Microsoft::DirectXTex)
```

Configure with vcpkg through presets or explicitly:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## CMake Project Layout Recommendation

```text
include/libbsa/        Public headers only; no libdeflate/lz4/DirectXTex headers leak here
src/                  Format parsers, writers, compression adapters, texture adapter
tests/                Catch2 tests and fixture generators
cmake/                Package config helpers if needed
fixtures/             Small committed synthetic archives; no mutable TES5Edit fixtures
TES5Edit/             Read-only reference submodule, never built into libbsa
```

Public API should expose stable libbsa types (`archive_reader`, `archive_writer`, `archive_entry`, `archive_format`, `error_code`, `result<T>`), not dependency types (`libdeflate_*`, `LZ4F_*`, `DirectX::ScratchImage`, `HRESULT`). Keep compression and DDS logic behind private adapters:

- `compression/deflate_codec.*` wraps libdeflate allocation, exact-size decompression, and compression levels.
- `compression/lz4_frame_codec.*` wraps SSE frame payload handling.
- `compression/lz4_block_codec.*` wraps Starfield BA2 raw block handling.
- `texture/dds_analyzer.*` wraps DirectXTex metadata and mip/chunk calculations.

## Alternatives Considered

| Recommended | Alternative | Why Not / When Alternative Makes Sense | Confidence |
|-------------|-------------|----------------------------------------|------------|
| CMake + vcpkg manifest mode | Meson, Bazel, Premake, raw Visual Studio solutions | These can build C++, but CMake + vcpkg is the most common portable library distribution path and aligns with vcpkg package exports. Use another build system only if a downstream consumer requires it later. | HIGH |
| libdeflate | zlib/miniz/zlib-ng | zlib is slower and not required unless archive bytes prove zlib wrapper compatibility is needed. miniz is attractive as a single-file fallback but violates the no-speculative-dependency constraint. zlib-ng is unnecessary while libdeflate satisfies the archive payload requirement. | HIGH |
| official lz4 | bundled LZ4 source, game-specific reimplementation | Official lz4 has stable frame and block APIs and active maintenance. Reimplementing LZ4 risks silent corruption; vendoring adds update/security burden. | HIGH |
| DirectXTex | Hand-written DDS parser, DirectXTK-only utilities, texconv CLI invocation | BA2 DDS needs robust DXGI/mip metadata, not shelling out to tools. A small hand parser may be enough for read-only extraction but will become fragile for write/chunking. DirectXTex should still be isolated so a future non-Windows adapter can replace it if needed. | MEDIUM-HIGH |
| Catch2 | GoogleTest | GoogleTest is excellent for large ecosystems and mocks; libbsa mostly needs data-driven parser/round-trip tests. Catch2 keeps test code terse and dependency surface smaller. Switch only if mocks or existing org standards require GoogleTest. | MEDIUM |

## What NOT to Use

| Avoid | Why | Use Instead | Confidence |
|-------|-----|-------------|------------|
| Editing or compiling TES5Edit code | Violates the hard project boundary and risks Delphi/UI coupling in the library | Treat TES5Edit as read-only behavioral reference and write clean C++ outside `TES5Edit/` | HIGH |
| `std::expected` in a C++20 public API | `std::expected` is standardized in C++23; exposing it while claiming C++20 creates portability and ABI/API confusion | Local `libbsa::result<T>` or explicit `error_code` + output parameter; reconsider when moving to C++23 | HIGH |
| `std::filesystem::path` as the archive path type | Bethesda archive paths are normalized virtual paths, not host OS paths; platform path rules can corrupt separators/case expectations | Store archive paths as normalized UTF-8/byte strings with explicit normalization rules; use filesystem paths only for host I/O boundaries | HIGH |
| LZ4 frame API for Starfield BA2 v3 raw LZ4 blocks | Frame and block formats are different; using the wrong API can silently fail or corrupt output | Route by archive format/version/`CompressionMethod`: `LZ4F_*` for SSE frames, `LZ4_*` safe block APIs for BA2 raw blocks | HIGH |
| DirectXTex types in public headers | Leaks Windows/DXGI implementation details and makes future portability harder | Internal `dds_metadata` / `texture_layout` value types translated from DirectXTex internally | HIGH |
| External logging/formatting libraries by default (`spdlog`, `fmt`) | Not required for a reusable archive library and violates the minimal-dependency constraint | Return structured errors; let consumers log/format as they choose | HIGH |
| `Boost`, `libarchive`, ZIP/7z libraries | They do not implement Bethesda BSA/BA2 semantics and add large dependency/API surface | Purpose-built parsers/writers for BSA/BA2 | HIGH |
| Whole-archive memory loading as the primary design | Starfield archives can be very large; whole-file reads break performance and memory goals | Streaming file/sink abstractions and bounded scratch buffers | HIGH |
| In-place archive mutation in early milestones | Hard to make safe across shifting file tables, compression, and deduplication; not required until polish | Open-read-write-new archive flow; defer in-place updates | HIGH |

## Stack Patterns by Archive Variant

**TES3 BSA:**
- Use standard C++ binary I/O and hash/path utilities only.
- No compression or DirectXTex required.
- Keep offset math isolated because TES3 offsets are data-section-relative.

**TES4 / FO3 / Skyrim LE BSA:**
- Use libdeflate for deflate payloads.
- Keep embedded-name handling and hash-ordering in format-specific code.
- Do not add zlib unless compatibility fixtures demonstrate wrapped streams are required.

**Skyrim SE/AE BSA:**
- Use official lz4 frame APIs (`LZ4F_*`) only for compressed payloads.
- Keep this separate from Starfield raw block LZ4 helpers.

**FO4 / Starfield BA2 GNRL:**
- Use libdeflate for FO4 and Starfield v2 / non-LZ4 v3 payloads.
- Use official lz4 block APIs (`LZ4_*safe*`) when Starfield v3 `CompressionMethod == 3`.
- Encode compression method as explicit metadata; never infer solely from file extension.

**FO4 / Starfield BA2 DDS:**
- Use DirectXTex only through an internal DDS analyzer for dimensions, DXGI format, mip count, array/cubemap metadata, and mip chunk planning.
- Use libdeflate or lz4 block according to archive version and chunk metadata.
- Persist libbsa-native metadata, not DirectXTex objects.

## Version Compatibility

| Component | Compatible With | Notes | Confidence |
|-----------|-----------------|-------|------------|
| CMake `3.24+` | vcpkg toolchain, Catch2 3.x, DirectXTex current CMake package | DirectXTex's 2025 release notes mention CMake minimum `3.21`; choosing `3.24` leaves room for modern install/export patterns. Current CMake docs are `4.3.2`, so test latest too. | HIGH |
| vcpkg manifest mode | libdeflate `1.25`, lz4 `1.10.0`, DirectXTex `2026-03-31`, Catch2 `3.14.0` | Commit baseline rather than hand-pinning every dependency. Use `version>=` only for known minimums and `overrides` only to force a problematic port version. | HIGH |
| DirectXTex `2026-03-31` | Windows desktop / VS 2022 and VS 2026; vcpkg package supports Windows and Linux | March 2026 release retires VS 2019 projects and adds VS 2026 support. Keep CI focused on VS 2022/2026 for DirectXTex-heavy phases; treat Linux/macOS DDS support as a validation item. | MEDIUM |
| libdeflate `1.25` | All vcpkg triplets | vcpkg package supports all triplets and exposes CMake targets. Whole-buffer API fits archive chunk payloads. | HIGH |
| lz4 `1.10.0` | All vcpkg triplets | vcpkg package supports all triplets and exposes `lz4::lz4`. Use stable library APIs, not CLI behavior. | HIGH |
| Catch2 `3.14.0` | CMake/CTest | Use `Catch2::Catch2WithMain` for tests and `catch_discover_tests` once test executables exist. | HIGH |

## CI / Toolchain Recommendation

Start with a small CI matrix and expand only when portability work begins:

1. **Windows MSVC 2022 x64 + vcpkg**: required primary lane; exercises DirectXTex and Windows consumers.
2. **Windows ClangCL x64 + vcpkg**: catches non-MSVC assumptions while preserving Windows SDK/DirectXTex availability.
3. **Linux Clang/GCC + vcpkg**: add once the core parser compiles without Windows assumptions; mark DirectXTex-dependent tests separately until validated.
4. **Sanitizer lane**: Clang ASan/UBSan for parsers and decompression adapters with malformed fixtures.

Use static analysis and formatting conservatively: add `clang-format`/`clang-tidy` only with checked-in configs, and do not format `TES5Edit/`.

## Sources

- Context7 `/kitware/cmake` — verified target-based C++20 and install/export patterns.
- Context7 `/microsoft/vcpkg` and Microsoft Learn — verified manifest mode, CMake toolchain integration, `builtin-baseline`, `version>=`, and `overrides` behavior: https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode and https://learn.microsoft.com/en-us/vcpkg/users/versioning
- CMake official docs — current release documentation shows CMake `4.3.2`: https://cmake.org/cmake/help/latest/release/index.html
- vcpkg package page — `libdeflate` `1.25#0`, features, all-triplet support, MIT license, last updated 2025-11-03: https://vcpkg.io/en/package/libdeflate.html
- libdeflate GitHub releases — latest `v1.25`: https://github.com/ebiggers/libdeflate/releases
- vcpkg package page — `lz4` `1.10.0#0`, all-triplet support, BSD-2-Clause license: https://vcpkg.io/en/package/lz4.html
- LZ4 GitHub releases — `v1.10.0` official release, stable library notes, frame/block API context: https://github.com/lz4/lz4/releases
- Context7 `/microsoft/directxtex` and DirectXTex wiki — verified DDS load/metadata APIs (`LoadFromDDSMemory`, `LoadFromDDSFile`, `TexMetadata`, `ScratchImage`).
- DirectXTex GitHub releases — March 2026 release, VS 2026 support, public mip helpers, vcpkg availability: https://github.com/microsoft/DirectXTex/releases
- vcpkg package page — `directxtex` `2026-03-31#0`, features, Windows/Linux support, last updated 2026-04-01: https://vcpkg.io/en/package/directxtex.html
- vcpkg usage files — verified CMake targets for `libdeflate`, `lz4`, and `Microsoft::DirectXTex`.
- vcpkg/Catch2 package page and Catch2 GitHub releases — `catch2` `3.14.0#0`, current release fixes and CTest integration context: https://vcpkg.io/en/package/catch2.html and https://github.com/catchorg/Catch2/releases

---
*Stack research for: libbsa C++20 Bethesda archive library*  
*Researched: 2026-05-05*
