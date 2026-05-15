---
last_mapped: 2026-05-15
last_mapped_commit: ead68b6856b37c3ea3acbf007180951c3cc22698
---

# Technology Stack

**Analysis Date:** 2026-05-15

## Languages

**Primary:**
- C++20 - Library implementation and public API, configured by `target_compile_features(libbsa PUBLIC cxx_std_20)` in `CMakeLists.txt` and public headers under `include/libbsa/`.

**Secondary:**
- CMake language - Build, install/export package generation, CTest registration, fixture tool targets, and package-consumer smoke tests in `CMakeLists.txt`, `tests/CMakeLists.txt`, `cmake/libbsaConfig.cmake.in`, and `tests/package-consumer/CMakeLists.txt`.
- PowerShell - GitHub Actions setup and repository policy checks in `.github/workflows/ci.yml`.
- Python 3 - Fixture manifest validation uses `find_package(Python3 COMPONENTS Interpreter REQUIRED)` and runs `tests/fixtures/generated/validate_fixture_manifests.py` from `tests/CMakeLists.txt`.
- Markdown - Project documentation and API-guide inputs in `docs/api-mainpage.md`, `docs/thread-safety.md`, `docs/target-format-guide.md`, `docs/integration-examples.md`, and `docs/compatibility-evidence.md`.
- YAML/JSON - CI and tool metadata in `.github/workflows/ci.yml`, `vcpkg.json`, `vcpkg-configuration.json`, `CMakePresets.json`, and `openspec/config.yaml`.
- Delphi/Pascal - Read-only behavioral reference only under `TES5Edit/`; do not compile or modify it.

## Runtime

**Environment:**
- Windows-only C++ library using MSVC-oriented builds. CI runs on `windows-latest` in `.github/workflows/ci.yml` and presets are named `windows-msvc-*` in `CMakePresets.json`.
- Public library version is `0.1.0`, declared in `CMakeLists.txt`, `vcpkg.json`, and `include/libbsa/version.hpp`.
- The core API is synchronous and embeddable: consumers include `include/libbsa/libbsa.hpp` and link `libbsa::libbsa` via installed CMake package metadata from `cmake/libbsaConfig.cmake.in`.

**Package Manager:**
- vcpkg manifest mode through `vcpkg.json` and `vcpkg-configuration.json`.
- Registry baseline: `12dcccadfe573d0eaa6c67a968413ded7805d256` in `vcpkg-configuration.json`.
- Toolchain path is supplied by `CMAKE_TOOLCHAIN_FILE=$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` in every `CMakePresets.json` configure preset.
- Lockfile: Not detected (`vcpkg-lock.json` missing). Installed local package metadata exists under `vcpkg_installed/x64-windows/share/`, but the manifest and baseline are the committed dependency contract.

## Frameworks

**Core:**
- CMake 4.0 minimum - Top-level build requires `cmake_minimum_required(VERSION 4.0)` in `CMakeLists.txt`; `CMakePresets.json` also declares `cmakeMinimumRequired` 4.0.0.
- CMake install/export packaging - `install(TARGETS libbsa EXPORT libbsaTargets FILE_SET HEADERS)`, generated `libbsaConfig.cmake`, and `libbsaConfigVersion.cmake` are configured in `CMakeLists.txt`.
- CTest - Included by `include(CTest)` in `CMakeLists.txt`; tests are registered in `tests/CMakeLists.txt`.
- Doxygen - Optional API documentation target `libbsa_docs` is created only when `find_package(Doxygen QUIET)` succeeds; input is configured by `docs/Doxyfile.in`.

**Testing:**
- Catch2 3.14.0 - Declared in `vcpkg.json`, found with `find_package(Catch2 CONFIG REQUIRED)` in `tests/CMakeLists.txt`, linked as `Catch2::Catch2WithMain`, and discovered with `catch_discover_tests`.
- nlohmann-json 3.12.0#2 - Declared in `vcpkg.json`, found with `find_package(nlohmann_json CONFIG REQUIRED)`, linked into `libbsa_tests`, and used by manifest-based tests such as `tests/unit/archive_reader_tests.cpp` and `tests/unit/validation_api_tests.cpp`.
- CTest package-consumer tests - `package_consumer_smoke`, `package_consumer_runtime_dll_copy`, and `shared_export_surface` verify installed-package usability and Windows DLL exports in `tests/CMakeLists.txt`.
- Python 3 - Runs `validate_fixture_manifests` against generated fixture manifests in `tests/CMakeLists.txt`.

**Build/Dev:**
- MSVC compiler - Project compile options use `/Zc:__cplusplus` and `/W4` for MSVC in `CMakeLists.txt`; non-MSVC warning flags exist but supported project lanes are Windows/MSVC.
- MSVC AddressSanitizer - Optional `LIBBSA_ENABLE_MSVC_ASAN` enables `/fsanitize=address` and disables STL annotation ODR checks in `libbsa_enable_msvc_asan` in `CMakeLists.txt`.
- CMake Presets - Static/shared Debug/Release and ASan static lanes are encoded in `CMakePresets.json`.
- GitHub Actions - CI installs CMake 4.3.2, configures, builds, tests, and verifies `TES5Edit/` stayed unchanged in `.github/workflows/ci.yml`.
- OpenSpec project skills - `.claude/skills/openspec-*.*/SKILL.md` define change-management workflows; they are process tooling, not runtime dependencies.

## Key Dependencies

**Critical:**
- libdeflate 1.25 - Required dependency in `vcpkg.json`; `CMakeLists.txt` finds `libdeflate CONFIG REQUIRED` and links `libdeflate::libdeflate_static` or `libdeflate::libdeflate_shared` privately. `src/detail/deflate_codec.cpp` uses `<libdeflate.h>` for raw deflate compression/decompression and exact decoded-size checks.
- lz4 1.10.0 - Required dependency in `vcpkg.json`; `CMakeLists.txt` finds `lz4 CONFIG REQUIRED`, selects `LZ4::lz4_static`, `LZ4::lz4_shared`, or `lz4::lz4`, and links it privately. `src/detail/lz4_frame_codec.cpp` uses `<lz4frame.h>` for Skyrim SE/AE BSA LZ4 frame payloads, while `src/detail/lz4_block_codec.cpp` uses `<lz4.h>` for Starfield BA2 v3 raw LZ4 blocks.
- DirectXTex 2026-03-31 - Required dependency in `vcpkg.json`; `CMakeLists.txt` finds `directxtex CONFIG REQUIRED` and links `Microsoft::DirectXTex` privately. `src/texture/directxtex_analyzer.cpp` uses `<DirectXTex.h>` for DDS metadata and source image analysis, translating results into dependency-light `libbsa::texture_metadata` in public headers.
- Windows BCrypt - `CMakeLists.txt` links `bcrypt`; `src/formats/ba2/ba2_dx10_snapshot_builder.cpp` calls `BCryptGenRandom` to create random BA2 DX10 snapshot temporary directory suffixes.
- Windows API - `src/detail/host_file_path.cpp` uses `MultiByteToWideChar` for the public UTF-8 host path contract; `include/libbsa/export.hpp` uses `__declspec(dllexport/dllimport)` for DLL boundaries; `src/detail/writer_publish.cpp` and `src/detail/atomic_file_ops.hpp` include `<windows.h>` for Windows filesystem publication behavior.

**Infrastructure:**
- Catch2 - Unit, compatibility, malformed-input, policy, and package tests under `tests/unit/` and registered in `tests/CMakeLists.txt`.
- nlohmann-json - Test-only manifest parsing and fixture expectation support; linked only into `libbsa_tests`, not the library target.
- Python3 interpreter - Test-only manifest validation in `validate_fixture_manifests`.
- Doxygen - Optional docs generation from `include/libbsa/` and selected docs in `docs/Doxyfile.in`; `TES5Edit/`, `src/`, `tests/`, build outputs, and `vcpkg_installed/` are excluded.
- GitHub Actions checkout with submodules - `.github/workflows/ci.yml` uses `actions/checkout@v6` with `submodules: true` so `TES5Edit/` is available as a read-only reference.

## Configuration

**Environment:**
- `VCPKG_ROOT` must point to a vcpkg installation. Presets require `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` in `CMakePresets.json`, and CI sets `VCPKG_ROOT: C:\vcpkg` in `.github/workflows/ci.yml`.
- `LIBBSA_CMAKE_VERSION=4.3.2` is the CI-selected CMake version in `.github/workflows/ci.yml`; the project minimum remains CMake 4.0.
- Optional local compatibility fixture environment variables are `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED`, documented in `docs/compatibility-evidence.md` and consumed by `tests/unit/local_game_fixture_tests.cpp`. These variables point at local, ignored data; do not commit fixture archives or BSArchPro-derived corpus output.
- No `.env` files detected in the repository root or subdirectories during mapping.

**Build:**
- Main build graph: `CMakeLists.txt`.
- Test build graph: `tests/CMakeLists.txt`.
- Install package template: `cmake/libbsaConfig.cmake.in`.
- Presets: `CMakePresets.json`.
- vcpkg manifest and registry baseline: `vcpkg.json`, `vcpkg-configuration.json`.
- CI pipeline: `.github/workflows/ci.yml`.
- Documentation generator config: `docs/Doxyfile.in`.
- Package consumer proof: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`, `tests/package-consumer/copy-runtime-dlls.cmake`, and `tests/package-consumer/verify-runtime-dll-copy.cmake`.

## Platform Requirements

**Development:**
- Use Windows with MSVC and vcpkg. The supported local presets are:
  - `windows-msvc-debug-static`
  - `windows-msvc-debug-shared`
  - `windows-msvc-release-static`
  - `windows-msvc-release-shared`
  - `windows-msvc-asan-static`
- Typical commands:
  - `cmake --preset windows-msvc-debug-static`
  - `cmake --build --preset windows-msvc-debug-static`
  - `ctest --preset windows-msvc-debug-static --output-on-failure`
- Keep `TES5Edit/` read-only. CI explicitly fails if `git status --short TES5Edit` reports changes in `.github/workflows/ci.yml`.

**Production:**
- Deployment target is a reusable Windows C++ library delivered through CMake install/export package files under `${CMAKE_INSTALL_LIBDIR}/cmake/libbsa`.
- Both static and shared builds are supported by `BUILD_SHARED_LIBS` in `CMakePresets.json`. Static builds define `LIBBSA_STATIC_DEFINE`; shared builds use explicit `LIBBSA_API` annotations from `include/libbsa/export.hpp` and intentionally keep `WINDOWS_EXPORT_ALL_SYMBOLS` disabled.
- Public headers under `include/libbsa/` must remain dependency-light: they expose standard C++20 types and libbsa-owned enums/structs, not libdeflate, LZ4, DirectXTex, DXGI, BCrypt, or Windows implementation types.

## Archive/Codec Stack Routing

- TES3/Morrowind BSA is raw/uncompressed and implemented in `src/formats/bsa/tes3_bsa_*`.
- TES4-family BSA v103/v104 and Fallout 4/Starfield deflate BA2 paths use raw deflate through `src/detail/deflate_codec.cpp`.
- Skyrim SE/AE BSA v105 compressed payloads use LZ4 frame APIs through `src/detail/lz4_frame_codec.cpp`.
- Starfield BA2 v3 `CompressionMethod == 3` uses raw LZ4 block APIs through `src/detail/lz4_block_codec.cpp`.
- BA2 DX10 DDS metadata and source payload layout use DirectXTex only behind `src/texture/directxtex_analyzer.cpp`; public metadata remains numeric and dependency-light in `include/libbsa/archive.hpp`.

---

*Stack analysis: 2026-05-15*
