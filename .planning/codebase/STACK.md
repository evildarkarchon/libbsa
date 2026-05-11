# Technology Stack

**Analysis Date:** 2026-05-11

## Languages

**Primary:**
- C++20 - Public headers, library implementation, unit tests, fixture generators, package-consumer smoke tests, and benchmarks. Enforced through `target_compile_features(libbsa PUBLIC cxx_std_20)` in `CMakeLists.txt`, `target_compile_features(libbsa_tests PRIVATE cxx_std_20)` in `tests/CMakeLists.txt`, and `CMAKE_CXX_STANDARD=20` in `CMakePresets.json`.

**Secondary:**
- CMake 4.0+ - Build, install/export package generation, test orchestration, package-consumer smoke checks, and runtime-DLL copy helpers. Entry points are `CMakeLists.txt`, `tests/CMakeLists.txt`, `cmake/libbsaConfig.cmake.in`, `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`, and `tests/package-consumer/copy-runtime-dlls.cmake`.
- Python 3 - Test-time validation of generated fixture manifests only. `tests/CMakeLists.txt` requires `Python3 COMPONENTS Interpreter`, and `tests/fixtures/generated/validate_fixture_manifests.py` performs JSON manifest validation.
- PowerShell - Developer README commands and GitHub Actions provisioning scripts. `README.md` shows PowerShell preset usage, and `.github/workflows/ci.yml` uses `pwsh` to install and verify CMake.
- Markdown/Doxygen configuration - Project documentation lives under `docs/`; public API documentation is configured by `docs/Doxyfile.in`.

**Not detected:**
- No Node, Rust, Go, Python package, or .NET package manifests were detected at the repository root (`package.json`, `Cargo.toml`, `go.mod`, `pyproject.toml`, `requirements.txt`, and solution/project files are absent from the repo scan).

## Runtime

**Environment:**
- Windows-only C++ library target. `README.md` states that development, CI, packaging, and dependency validation target Windows with MSVC and vcpkg.
- GitHub CI uses `windows-latest` in `.github/workflows/ci.yml`.
- No long-running application server, web runtime, GUI runtime, or database runtime is part of the repo.

**Compiler/Toolchain:**
- MSVC is the supported developer/CI compiler path through the `windows-msvc-debug-static` and `windows-msvc-debug-shared` presets in `CMakePresets.json`.
- The library enables `/Zc:__cplusplus` publicly for MSVC and `/W4` privately in `CMakeLists.txt`.
- Non-MSVC warning flags exist in `CMakeLists.txt`, but `README.md` and `AGENTS.md` define Windows/MSVC as the supported platform boundary.
- `.clangd` adds `-std=c++20` and `-Iinclude` for editor tooling.

**Package Manager:**
- vcpkg manifest mode - `vcpkg.json` declares dependencies and `vcpkg-configuration.json` pins the default registry to `https://github.com/microsoft/vcpkg` at baseline `12dcccadfe573d0eaa6c67a968413ded7805d256`.
- Developer presets use `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` in `CMakePresets.json`.
- Lockfile: Not detected. No `vcpkg-lock.json` is present; dependency resolution is controlled by `vcpkg-configuration.json`.

## Frameworks

**Core:**
- CMake 4.0 - Project minimum and package/export generator. `CMakeLists.txt` begins with `cmake_minimum_required(VERSION 4.0)`, defines `project(libbsa VERSION 0.1.0 LANGUAGES CXX)`, creates `libbsa`, and exports `libbsa::libbsa`.
- vcpkg - Dependency acquisition for library, test, and development packages. Manifest files are `vcpkg.json` and `vcpkg-configuration.json`.
- C++ standard library - Public headers under `include/libbsa/` use standard C++ types such as `std::span`, `std::string_view`, `std::optional`, `std::vector`, `std::shared_ptr`, and a local C++20-compatible `libbsa::result<T>` in `include/libbsa/result.hpp`.

**Testing:**
- CTest - Enabled by `include(CTest)` in `CMakeLists.txt`; tests are registered in `tests/CMakeLists.txt`.
- Catch2 - `tests/CMakeLists.txt` requires `find_package(Catch2 CONFIG REQUIRED)`, links `Catch2::Catch2WithMain`, and registers tests through `catch_discover_tests`.
- nlohmann-json - Test-only JSON parsing dependency in `tests/CMakeLists.txt` and test files such as `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, and `tests/unit/validation_api_tests.cpp`.
- Python 3 - Used by CTest target `validate_fixture_manifests` in `tests/CMakeLists.txt`.

**Build/Dev:**
- GitHub Actions - `.github/workflows/ci.yml` runs the Windows MSVC matrix on push and pull request.
- Doxygen - Optional documentation generator. `CMakeLists.txt` uses `find_package(Doxygen QUIET)` and creates `libbsa_docs` only when Doxygen is available.
- OpenSpec CLI - Project-local workflow skills under `.codex/skills/*/SKILL.md` require the `openspec` CLI for change proposal/apply/archive/verify flows. This is workflow tooling, not a build dependency.
- Benchmark harness - `LIBBSA_BUILD_BENCHMARKS` in `CMakeLists.txt` controls `libbsa_benchmarks` and `libbsa_benchmark_report`; usage is documented in `benchmarks/README.md`.

## Key Dependencies

**Critical:**
- libdeflate - Private deflate compression/decompression implementation. Declared in `vcpkg.json`, discovered by `find_package(libdeflate CONFIG REQUIRED)` in `CMakeLists.txt`, linked privately through `libdeflate::libdeflate_static` or `libdeflate::libdeflate_shared`, and used in `src/detail/deflate_codec.cpp`.
- lz4 - Private LZ4 frame and raw block implementation. Declared in `vcpkg.json`, discovered by `find_package(lz4 CONFIG REQUIRED)` in `CMakeLists.txt`, target-selected as `LZ4::lz4_static`, `LZ4::lz4_shared`, or `lz4::lz4`, and used by `src/detail/lz4_frame_codec.cpp` and `src/detail/lz4_block_codec.cpp`.
- DirectXTex - Private DDS metadata and source analysis implementation. Declared in `vcpkg.json`, discovered by `find_package(directxtex CONFIG REQUIRED)` in `CMakeLists.txt`, linked as `Microsoft::DirectXTex`, and used by `src/texture/directxtex_analyzer.cpp`.

**Infrastructure:**
- Catch2 - Unit and fixture test runner dependency declared in `vcpkg.json` and linked by `tests/CMakeLists.txt`.
- nlohmann-json - Test manifest parsing dependency declared in `vcpkg.json` and linked by `tests/CMakeLists.txt`.
- Doxygen - Optional documentation tool configured by `docs/Doxyfile.in`; not declared in `vcpkg.json`.
- Python 3 - CTest-time manifest validator dependency discovered by `tests/CMakeLists.txt`; not declared in `vcpkg.json`.
- TES5Edit submodule - Read-only behavioral reference declared in `.gitmodules` at `TES5Edit/`. It is not compiled, linked, formatted, or used as a fixture output location by libbsa.

**Dependency Boundary Rules:**
- Keep `libdeflate`, `lz4`, `DirectXTex`, Windows headers, and codec-specific implementation details out of public headers under `include/libbsa/`.
- `tests/unit/public_include_boundary_tests.cpp` enforces the public include boundary by scanning for forbidden tokens such as `libdeflate`, `lz4`, `DirectXTex`, `Windows.h`, and thread primitives.
- `cmake/libbsaConfig.cmake.in` exports package dependencies for downstream CMake consumers with `find_dependency(libdeflate CONFIG)`, `find_dependency(lz4 CONFIG)`, and `find_dependency(directxtex CONFIG)`.

## Configuration

**Environment:**
- `VCPKG_ROOT` is required for developer preset configuration. `README.md` shows `$env:VCPKG_ROOT = 'C:\vcpkg'`, and `CMakePresets.json` uses `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`.
- `LIBBSA_CMAKE_VERSION` is a GitHub Actions environment variable set to `4.3.2` in `.github/workflows/ci.yml` and verified before configure/build/test.
- `LIBBSA_GAME_FIXTURES` is an optional test environment variable consumed by `tests/unit/local_game_fixture_tests.cpp` for local game archive smoke checks.
- `LIBBSA_BSARCHPRO_EXPECTED` is an optional test environment variable consumed by `tests/unit/local_game_fixture_tests.cpp` for BSArchPro-derived expected comparison manifests.
- `LIBBSA_SOURCE_DIR` is a test compile definition set in `tests/CMakeLists.txt` for tests that need repository-relative fixture/document paths.
- `.env*` files are not detected at the repository root.

**Build:**
- Main build configuration lives in `CMakeLists.txt`.
- Repeatable developer presets live in `CMakePresets.json` and cover `windows-msvc-debug-static` and `windows-msvc-debug-shared`.
- Dependency configuration lives in `vcpkg.json` and `vcpkg-configuration.json`.
- Package export configuration lives in `cmake/libbsaConfig.cmake.in`.
- Test build configuration lives in `tests/CMakeLists.txt`.
- Documentation configuration lives in `docs/Doxyfile.in`.

**CMake Options:**
- `LIBBSA_BUILD_TESTS` in `CMakeLists.txt` defaults to `BUILD_TESTING` and gates `add_subdirectory(tests)`.
- `LIBBSA_BUILD_BENCHMARKS` in `CMakeLists.txt` defaults to `ON` and gates the benchmark executable and report target.
- `BUILD_SHARED_LIBS` is set by `CMakePresets.json` to `OFF` for `windows-msvc-debug-static` and `ON` for `windows-msvc-debug-shared`.
- `BUILD_TESTING` comes from CTest and participates in the `BUILD_TESTING AND LIBBSA_BUILD_TESTS` gate in `CMakeLists.txt`.

**Package/Consumer Validation:**
- `CMakeLists.txt` installs `libbsa`, exports `libbsaTargets`, and generates `libbsaConfig.cmake` plus `libbsaConfigVersion.cmake`.
- `tests/package-consumer/smoke.cmake` installs the build tree into a temporary prefix, configures a downstream consumer with `CMAKE_PREFIX_PATH`, builds it, and runs its CTest suite.
- `tests/package-consumer/copy-runtime-dlls.cmake` handles empty `$<TARGET_RUNTIME_DLLS:...>` lists for static builds and copies runtime DLLs for shared builds.

## Platform Requirements

**Development:**
- Windows with MSVC.
- CMake 4.0 or newer. CI installs and verifies CMake `4.3.2` in `.github/workflows/ci.yml`.
- vcpkg checkout available through `VCPKG_ROOT`.
- Python 3 interpreter when building/running the full test suite from `tests/CMakeLists.txt`.
- Optional Doxygen for the `libbsa_docs` target.

**Production:**
- Reusable C++20 library consumed through the installed CMake target `libbsa::libbsa`.
- Static and shared library builds are both supported through `BUILD_SHARED_LIBS` presets in `CMakePresets.json`.
- Public headers under `include/libbsa/` expose standard C++ value types and `libbsa::result<T>` rather than dependency-specific, DirectX, or Windows implementation types.
- No application server, database, authentication provider, message queue, or network service is required by the library runtime.

**CI:**
- `.github/workflows/ci.yml` runs on `push` and `pull_request`.
- The CI matrix builds and tests `windows-msvc-debug-static` and `windows-msvc-debug-shared`.
- CI checks out submodules, installs a pinned CMake version, configures with presets, builds with presets, runs CTest, and verifies `TES5Edit/` stays unchanged with `git status --short TES5Edit`.

## Stack Usage Guidance

- Add new third-party C++ dependencies only through `vcpkg.json` and keep registry/baseline policy in `vcpkg-configuration.json`.
- Link implementation dependencies privately from `CMakeLists.txt` unless a public header genuinely requires the type.
- Keep new public APIs in `include/libbsa/` C++20-compatible; do not expose C++23-only types such as `std::expected`.
- Put library implementation in `src/` and tests in `tests/`; do not compile or mutate `TES5Edit/`.
- Use CTest/Catch2 for new validation and fixture coverage; use `tests/fixtures/generated/` for committed legal fixtures and `tests/fixtures/local/` plus environment variables for uncommitted local corpus checks.

---

*Stack analysis: 2026-05-11*
