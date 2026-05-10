# Technology Stack

**Analysis Date:** 2026-05-10

## Languages

**Primary:**
- C++20 - Public API headers live in `include/libbsa/`, implementation lives in `src/`, tests live in `tests/unit/`, fixture generators live in `tests/fixtures/generated/`, and benchmark tooling lives in `benchmarks/libbsa_benchmarks.cpp`.
- C++20 is required by `CMakeLists.txt` through `target_compile_features(libbsa PUBLIC cxx_std_20)` and by `CMakePresets.json` through `CMAKE_CXX_STANDARD=20`.

**Secondary:**
- CMake 3.24+ - Build, install/export packaging, docs target, tests, fixture generators, and package-consumer smoke tests are defined in `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`, and `cmake/libbsaConfig.cmake.in`.
- Python 3 - Test-only manifest validation uses `tests/fixtures/generated/validate_fixture_manifests.py` and is wired with `find_package(Python3 COMPONENTS Interpreter REQUIRED)` in `tests/CMakeLists.txt`.
- Markdown - Public documentation and policy evidence live in `docs/api-mainpage.md`, `docs/thread-safety.md`, `docs/target-format-guide.md`, `docs/integration-examples.md`, `docs/compatibility-evidence.md`, `tests/fixtures/README.md`, and `benchmarks/README.md`.
- YAML - GitHub Actions CI is defined in `.github/workflows/ci.yml`.

## Runtime

**Environment:**
- Native C++ library, version `0.1.0`, declared in `CMakeLists.txt`, `vcpkg.json`, and `include/libbsa/version.hpp`.
- The built artifact is `libbsa` with CMake alias target `libbsa::libbsa`, defined in `CMakeLists.txt` and exported through `cmake/libbsaConfig.cmake.in`.
- Runtime I/O is local filesystem based through public APIs in `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and `include/libbsa/validation.hpp`; network or URL archive access is out of scope in `docs/PRD.md` and not implemented under `src/`.
- Optional parallel work uses C++20 `std::jthread` in `src/detail/parallel_work.cpp`; callers opt in with `worker_count` in `include/libbsa/archive.hpp` and `include/libbsa/writer.hpp`.

**Package Manager:**
- vcpkg manifest mode - Dependencies are declared in `vcpkg.json` and the builtin registry baseline is recorded in `vcpkg.json` plus `vcpkg-configuration.json`.
- vcpkg toolchain discovery uses `VCPKG_ROOT` through `CMakePresets.json`, and CI sets `VCPKG_ROOT` in `.github/workflows/ci.yml`.
- Lockfile: Not detected. Version resolution is baseline-pinned by `builtin-baseline` in `vcpkg.json` and `baseline` in `vcpkg-configuration.json`.

## Frameworks

**Core:**
- CMake 3.24+ - `CMakeLists.txt` owns project configuration, C++20 compile features, install/export packaging, optional docs, optional benchmarks, and test subdirectory wiring.
- vcpkg manifest mode - `vcpkg.json` owns dependency declaration for `libdeflate`, `lz4`, `directxtex`, `catch2`, and `nlohmann-json`.
- CMake package exports - `CMakeLists.txt` installs `libbsaTargets`, writes `libbsaConfigVersion.cmake`, and configures `cmake/libbsaConfig.cmake.in` with transitive dependency discovery for consumers.

**Testing:**
- Catch2 - `tests/CMakeLists.txt` builds `libbsa_tests` and links `Catch2::Catch2WithMain`; unit tests are in `tests/unit/*.cpp`.
- CTest - `include(CTest)` in `CMakeLists.txt`, `catch_discover_tests(libbsa_tests ...)` in `tests/CMakeLists.txt`, and presets in `CMakePresets.json` provide test orchestration.
- nlohmann-json - Test-only JSON parsing is declared in `vcpkg.json`, discovered with `find_package(nlohmann_json CONFIG REQUIRED)` in `tests/CMakeLists.txt`, and used in tests such as `tests/unit/local_game_fixture_tests.cpp`.
- Python3 - `tests/CMakeLists.txt` runs `tests/fixtures/generated/validate_fixture_manifests.py` as the `validate_fixture_manifests` CTest test.

**Build/Dev:**
- CMakePresets - `CMakePresets.json` defines `windows-msvc-debug-static`, `windows-msvc-debug-shared`, and `linux-clang-asan-ubsan`.
- GitHub Actions - `.github/workflows/ci.yml` runs the Windows MSVC static and shared presets, executes CTest, and checks that `TES5Edit/` remains unchanged.
- Doxygen - `find_package(Doxygen QUIET)` in `CMakeLists.txt` enables the optional `libbsa_docs` target; `docs/Doxyfile.in` scopes generated API docs to `include/libbsa/` and selected docs pages.
- clangd - `.clangd` sets `-std=c++20` and `-Iinclude` for editor indexing.
- Custom benchmark harness - `CMakeLists.txt` builds `libbsa_benchmarks` from `benchmarks/libbsa_benchmarks.cpp` and exposes the `libbsa_benchmark_report` target.

## Key Dependencies

**Critical:**
- `libdeflate` - Required private adapter for raw deflate compression/decompression, declared in `vcpkg.json`, discovered in `CMakeLists.txt`, linked privately by `libbsa`, exported as a consumer dependency in `cmake/libbsaConfig.cmake.in`, and implemented in `src/detail/deflate_codec.cpp`.
- `lz4` - Required private adapter for LZ4 frame and raw block compression/decompression, declared in `vcpkg.json`, discovered in `CMakeLists.txt`, linked as `lz4::lz4`, and implemented in `src/detail/lz4_frame_codec.cpp` plus `src/detail/lz4_block_codec.cpp`.
- `DirectXTex` - Required private texture metadata and DDS source analyzer for BA2 DX10 handling, declared as `directxtex` in `vcpkg.json`, discovered as `directxtex` in `CMakeLists.txt`, linked as `Microsoft::DirectXTex`, and wrapped by `src/texture/directxtex_analyzer.cpp`.
- C++ standard library - Public API and implementation use C++20 standard components such as `std::span`, `std::variant`, `std::optional`, `std::filesystem`, and `std::jthread` across `include/libbsa/*.hpp`, `src/detail/parallel_work.cpp`, and format writer/parser files under `src/formats/`.

**Infrastructure:**
- `catch2` - Test runner dependency declared in `vcpkg.json`, found in `tests/CMakeLists.txt`, and included by `tests/unit/*.cpp`.
- `nlohmann-json` - Test and fixture manifest parsing dependency declared in `vcpkg.json`, found in `tests/CMakeLists.txt`, and included by JSON-backed tests such as `tests/unit/compatibility_matrix_tests.cpp`.
- Python3 interpreter - CTest dependency for `tests/fixtures/generated/validate_fixture_manifests.py`, discovered in `tests/CMakeLists.txt`.
- Doxygen - Optional documentation tool discovered in `CMakeLists.txt`; `docs/Doxyfile.in` excludes `src/`, `TES5Edit/`, `tests/`, build output, and `vcpkg_installed/`.
- Git submodule `TES5Edit/` - Read-only reference material declared in `.gitmodules`; CI protects it with a status guard in `.github/workflows/ci.yml`.

## Configuration

**Environment:**
- `VCPKG_ROOT` must point at a vcpkg checkout for preset-based configure because `CMakePresets.json` uses `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`; `.github/workflows/ci.yml` sets it to `C:\vcpkg` in CI.
- `LIBBSA_GAME_FIXTURES` is optional and used only by opt-in local game fixture tests in `tests/unit/local_game_fixture_tests.cpp` and documented in `tests/fixtures/README.md`.
- `LIBBSA_BSARCHPRO_EXPECTED` is optional and points at a local BSArchPro-derived comparison manifest for `tests/unit/local_game_fixture_tests.cpp`.
- `LIBBSA_SOURCE_DIR` is a test-only compile definition set in `tests/CMakeLists.txt` so tests can locate committed fixtures and policy files.
- `.env` files are not present in the repo root scan, and `*.env` is ignored by `.gitignore`.

**Build:**
- `CMakeLists.txt` - Root project, library target, public header file set, private sources, dependencies, options, docs target, benchmark target, install/export package, and test subdirectory.
- `CMakePresets.json` - Windows static/shared debug presets and Linux Clang ASan/UBSan preset.
- `vcpkg.json` - Package manifest for `libdeflate`, `lz4`, `directxtex`, `catch2`, and `nlohmann-json`.
- `vcpkg-configuration.json` - vcpkg builtin registry baseline.
- `tests/CMakeLists.txt` - Unit test executable, fixture generation targets, Catch2 discovery, package-consumer smoke test, and Python manifest validator.
- `tests/package-consumer/smoke.cmake` - Installs `libbsa`, configures a separate consumer project, builds it, and runs its CTest suite.
- `cmake/libbsaConfig.cmake.in` - Installed package config with `find_dependency` entries for `libdeflate`, `lz4`, and `directxtex`.
- `docs/Doxyfile.in` - Optional public API documentation input and exclusion policy.
- `.github/workflows/ci.yml` - Windows CI for static/shared builds, tests, and `TES5Edit/` cleanliness.

## Platform Requirements

**Development:**
- CMake 3.24+ and a C++20 compiler are required by `CMakeLists.txt` and `CMakePresets.json`.
- Windows/MSVC is the primary configured CI path through `.github/workflows/ci.yml` and the `windows-msvc-debug-static` plus `windows-msvc-debug-shared` presets in `CMakePresets.json`.
- vcpkg must be available through `VCPKG_ROOT` for preset-based dependency resolution in `CMakePresets.json`.
- Python3 is required when building tests because `tests/CMakeLists.txt` adds `validate_fixture_manifests`.
- Doxygen is optional; missing Doxygen leaves `libbsa_docs` unavailable but does not block configure in `CMakeLists.txt`.
- Local game fixtures are optional and ignored under `tests/fixtures/local/`; default tests use committed generated fixtures in `tests/fixtures/generated/`.

**Production:**
- Production output is an installable static or shared CMake package with target `libbsa::libbsa`, configured by `CMakeLists.txt`, `cmake/libbsaConfig.cmake.in`, and `tests/package-consumer/smoke.cmake`.
- Runtime storage is caller-provided host filesystem paths for archives and payload sources through `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and `include/libbsa/validation.hpp`.
- Public headers intentionally avoid leaking `libdeflate`, `lz4`, `DirectXTex`, Windows, thread, and internal format types; this boundary is enforced by `tests/unit/public_include_boundary_tests.cpp`.

---

*Stack analysis: 2026-05-10*
