# Technology Stack

**Analysis Date:** 2026-05-11

## Languages

**Primary:**
- C++20 - Library implementation, public headers, fixture generators, and benchmark tooling. C++20 is enforced by `target_compile_features(libbsa PUBLIC cxx_std_20)` in `CMakeLists.txt` and by `CMAKE_CXX_STANDARD: "20"` in `CMakePresets.json`.

**Secondary:**
- CMake 4.0+ - Build, install/export package generation, tests, fixture generator targets, and package-consumer smoke tests in `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/package-consumer/CMakeLists.txt`, and `cmake/libbsaConfig.cmake.in`.
- Python 3 - Test-time manifest validation through `tests/fixtures/generated/validate_fixture_manifests.py`, discovered as `Python3::Interpreter` in `tests/CMakeLists.txt`.
- PowerShell - GitHub Actions setup script and documented local build shell in `.github/workflows/ci.yml` and `README.md`.
- YAML - CI and OpenSpec workflow metadata in `.github/workflows/ci.yml` and `openspec/config.yaml`.
- JSON - vcpkg manifests, CMake presets, generated fixture manifests, and compatibility matrices in `vcpkg.json`, `vcpkg-configuration.json`, `CMakePresets.json`, and `tests/fixtures/generated/**/*.json`.

## Runtime

**Environment:**
- Windows-only library and validation target. The project states the support boundary in `README.md` and uses Windows/MSVC presets in `CMakePresets.json`.
- MSVC is the primary compiler environment. `CMakeLists.txt` adds MSVC-specific `/Zc:__cplusplus` and `/W4`; CI runs `windows-latest` in `.github/workflows/ci.yml`.
- C++ standard library runtime plus Windows API for atomic file publication. `src/detail/atomic_file_ops.hpp` uses `MoveFileExW` behind `_WIN32`.

**Package Manager:**
- vcpkg manifest mode - dependencies are declared in `vcpkg.json` and pinned to the Microsoft vcpkg registry baseline in `vcpkg-configuration.json`.
- Lock/state files present: `vcpkg_installed/vcpkg/vcpkg-lock.json` and `vcpkg_installed/vcpkg/status` exist in the current workspace.
- CMake presets expect `VCPKG_ROOT` and set `CMAKE_TOOLCHAIN_FILE` to `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` in `CMakePresets.json`.

## Frameworks

**Core:**
- CMake 4.0 minimum - Root project, target definition, package config generation, and install/export rules live in `CMakeLists.txt`.
- libbsa CMake package export - Installed consumers use `find_package(libbsa CONFIG REQUIRED)` and `libbsa::libbsa`, as verified by `tests/package-consumer/CMakeLists.txt`.
- OpenSpec - Project change/spec workflow metadata and skills exist under `openspec/` and `.claude/skills/openspec-*`. Use this for structured changes; do not treat OpenSpec files as library runtime inputs.

**Testing:**
- Catch2 3.14.0 - Unit and fixture tests link `Catch2::Catch2WithMain` in `tests/CMakeLists.txt`; tests include `<catch2/catch_test_macros.hpp>` throughout `tests/unit/*.cpp`.
- CTest - Enabled with `include(CTest)` in `CMakeLists.txt`, `catch_discover_tests(libbsa_tests ...)` in `tests/CMakeLists.txt`, and `ctest --preset ...` in `.github/workflows/ci.yml` and `README.md`.
- nlohmann-json 3.12.0 - Test-only JSON parsing for fixture manifests and compatibility matrices; linked as `nlohmann_json::nlohmann_json` in `tests/CMakeLists.txt`.
- Python 3 - Test-only fixture manifest validator invoked by `validate_fixture_manifests` in `tests/CMakeLists.txt`.

**Build/Dev:**
- Doxygen - Optional documentation target `libbsa_docs` is created when `find_package(Doxygen QUIET)` succeeds in `CMakeLists.txt`; configuration lives in `docs/Doxyfile.in`.
- CMakePresets - Supported local/CI presets are `windows-msvc-debug-static` and `windows-msvc-debug-shared` in `CMakePresets.json`.
- GitHub Actions - Windows CI config in `.github/workflows/ci.yml` installs CMake 4.3.2, configures, builds, tests, and verifies `TES5Edit/` remains unchanged.
- Synthetic benchmark harness - `LIBBSA_BUILD_BENCHMARKS` defaults ON in `CMakeLists.txt`; `benchmarks/libbsa_benchmarks.cpp` generates legal synthetic data and `benchmarks/README.md` documents `libbsa_benchmark_report`.

## Key Dependencies

**Critical:**
- libdeflate 1.25 - Required compression/decompression dependency declared with `compression` and `decompression` features in `vcpkg.json`; linked privately in `CMakeLists.txt`; used by `src/detail/deflate_codec.cpp` for raw deflate exact-size compression/decompression.
- lz4 1.10.0 - Required LZ4 dependency declared in `vcpkg.json`; linked privately via `LZ4::lz4_static`, `LZ4::lz4_shared`, or `lz4::lz4` in `CMakeLists.txt`; used by `src/detail/lz4_frame_codec.cpp` for LZ4 frames and `src/detail/lz4_block_codec.cpp` for raw LZ4 blocks.
- DirectXTex 2026-03-31 - Required texture dependency declared in `vcpkg.json`; linked privately as `Microsoft::DirectXTex` in `CMakeLists.txt`; used only internally by `src/texture/directxtex_analyzer.cpp` for DDS metadata and source analysis.

**Infrastructure:**
- Catch2 3.14.0 - Test runner and assertions for `tests/unit/*.cpp`, declared in `vcpkg.json` and linked in `tests/CMakeLists.txt`.
- nlohmann-json 3.12.0 - Test-only fixture/manifest JSON support, declared in `vcpkg.json` and linked in `tests/CMakeLists.txt`.
- DirectXMath 2026-03-12 - Transitive vcpkg dependency installed with DirectXTex; present in `vcpkg_installed/vcpkg/status` but not included directly by libbsa source.
- CMake package helper modules - `GNUInstallDirs`, `CMakePackageConfigHelpers`, and `CTest` are included in `CMakeLists.txt`.

## Configuration

**Environment:**
- Required developer environment variable: `VCPKG_ROOT`, used by `CMakePresets.json` and documented in `README.md`.
- CI environment: `VCPKG_ROOT: C:\vcpkg` and `LIBBSA_CMAKE_VERSION: 4.3.2` in `.github/workflows/ci.yml`.
- Build options: `LIBBSA_BUILD_TESTS`, `LIBBSA_BUILD_BENCHMARKS`, and `BUILD_SHARED_LIBS` in `CMakeLists.txt` and `CMakePresets.json`.
- Public DLL decoration is controlled by `LIBBSA_BUILDING_LIBRARY` and `LIBBSA_STATIC_DEFINE` in `CMakeLists.txt` and `include/libbsa/export.hpp`.
- No `.env` files detected at the repository root during the scan.

**Build:**
- Root build configuration: `CMakeLists.txt`.
- Test build configuration: `tests/CMakeLists.txt`.
- Consumer package smoke configuration: `tests/package-consumer/CMakeLists.txt` and `tests/package-consumer/smoke.cmake`.
- Presets: `CMakePresets.json`.
- vcpkg manifest/baseline: `vcpkg.json` and `vcpkg-configuration.json`.
- Package config template: `cmake/libbsaConfig.cmake.in`; installed package propagates dependencies with `find_dependency(libdeflate CONFIG)`, `find_dependency(lz4 CONFIG)`, and `find_dependency(directxtex CONFIG)`.
- Documentation config: `docs/Doxyfile.in`.
- CI config: `.github/workflows/ci.yml`.

## Platform Requirements

**Development:**
- Use Windows with MSVC and vcpkg. Supported commands are documented in `README.md`:
  - `cmake --preset windows-msvc-debug-static`
  - `cmake --build --preset windows-msvc-debug-static`
  - `ctest --preset windows-msvc-debug-static --output-on-failure`
- Use `windows-msvc-debug-static` for static library validation and `windows-msvc-debug-shared` for DLL/export validation, both defined in `CMakePresets.json`.
- Keep `TES5Edit/` read-only. CI enforces this with `git status --short TES5Edit` in `.github/workflows/ci.yml`; Doxygen excludes `TES5Edit/` in `docs/Doxyfile.in`.

**Production:**
- Deployment target is an embeddable C++ library installed as a CMake package exporting `libbsa::libbsa` from `CMakeLists.txt`.
- Public consumers include `<libbsa/libbsa.hpp>` and standard C++ headers only, as documented in `docs/integration-examples.md` and compile-checked by `tests/package-consumer/main.cpp`.
- Shared-library builds rely on explicit `LIBBSA_API` annotations in public headers; `WINDOWS_EXPORT_ALL_SYMBOLS` is forbidden in `CMakeLists.txt`.
- Public headers must not expose private dependency types from libdeflate, LZ4, DirectXTex, or nlohmann-json; dependency use stays in `src/detail/*`, `src/texture/*`, and tests.

---

*Stack analysis: 2026-05-11*
