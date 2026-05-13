# Technology Stack

**Analysis Date:** 2026-05-12

## Languages

**Primary:**
- C++20 - core library implementation and public API in `CMakeLists.txt`, `include/libbsa/*.hpp`, and `src/**/*.cpp`

**Secondary:**
- CMake 4.0+ - build, packaging, docs target wiring, and test orchestration in `CMakeLists.txt`, `tests/CMakeLists.txt`, and `tests/package-consumer/CMakeLists.txt`
- YAML - CI workflow configuration in `.github/workflows/ci.yml`
- Python 3 - fixture-manifest validation in `tests/fixtures/generated/validate_fixture_manifests.py`
- Markdown - maintainer and consumer documentation in `README.md` and `docs/*.md`

## Runtime

**Environment:**
- Windows-only host runtime with MSVC-targeted builds; documented in `README.md` and enforced by Windows-specific code in `src/detail/atomic_file_ops.hpp`, `src/detail/writer_publish.cpp`, and `src/formats/ba2/ba2_dx10_prepare.cpp`

**Package Manager:**
- vcpkg manifest mode via `vcpkg.json`
- Registry baseline pinned in `vcpkg-configuration.json`
- Lockfile: missing

## Frameworks

**Core:**
- CMake package/export workflow - library build, install, and package config generation in `CMakeLists.txt`
- Standard C++ library - filesystem, spans, optional, jthread, and containers throughout `include/libbsa/*.hpp` and `src/**/*.cpp`

**Testing:**
- Catch2 - unit and policy tests in `tests/CMakeLists.txt` and `tests/unit/*.cpp`
- CTest - preset-driven execution and package-consumer smoke tests in `CMakeLists.txt`, `CMakePresets.json`, and `tests/CMakeLists.txt`

**Build/Dev:**
- Doxygen - optional API docs target in `CMakeLists.txt` and `docs/Doxyfile.in`
- GitHub Actions - Windows configure/build/test automation in `.github/workflows/ci.yml`
- clangd - lightweight editor compile flags in `.clangd`

## Key Dependencies

**Critical:**
- `libdeflate` - raw deflate compression/decompression used by archive codecs in `CMakeLists.txt` and `src/detail/deflate_codec.cpp`
- `lz4` - frame and raw-block compression/decompression used by archive codecs in `CMakeLists.txt`, `src/detail/lz4_frame_codec.cpp`, and `src/detail/lz4_block_codec.cpp`
- `directxtex` - DDS metadata and texture payload analysis kept behind an internal adapter in `CMakeLists.txt` and `src/texture/directxtex_analyzer.cpp`

**Infrastructure:**
- `Catch2` - test runner and assertions in `tests/CMakeLists.txt`
- `nlohmann-json` - JSON manifest parsing in fixture-driven tests such as `tests/unit/tes4_bsa_reader_tests.cpp` and `tests/unit/ba2_dx10_writer_tests.cpp`
- Windows `bcrypt` - system RNG for BA2 DX10 snapshot temp-directory naming in `CMakeLists.txt` and `src/formats/ba2/ba2_dx10_prepare.cpp`
- Windows file APIs (`MoveFileExW`, `GetFileAttributesW`) - atomic publish and reparse-point checks in `src/detail/atomic_file_ops.hpp` and `src/detail/writer_publish.cpp`

## Configuration

**Environment:**
- Build tooling requires `VCPKG_ROOT`; referenced in `README.md` and `CMakePresets.json`
- CI sets `VCPKG_ROOT` and `LIBBSA_CMAKE_VERSION` in `.github/workflows/ci.yml`
- Runtime env vars: Not detected
- `.env` files: Not detected during this scan

**Build:**
- Root build and install config: `CMakeLists.txt`
- Presets for static/shared debug builds: `CMakePresets.json`
- vcpkg manifest and registry baseline: `vcpkg.json`, `vcpkg-configuration.json`
- Test targets and fixture generators: `tests/CMakeLists.txt`
- Package consumer integration smoke test: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`
- API docs config: `docs/Doxyfile.in`
- CI workflow: `.github/workflows/ci.yml`

## Platform Requirements

**Development:**
- Windows development machine with CMake 4.0+ and vcpkg; see `README.md` and `CMakePresets.json`
- MSVC-oriented compile settings and warning flags in `CMakeLists.txt`
- Python 3 interpreter required for `validate_fixture_manifests` in `tests/CMakeLists.txt` and `tests/fixtures/generated/validate_fixture_manifests.py`
- Optional Doxygen installation enables `libbsa_docs` in `CMakeLists.txt`

**Production:**
- Windows library consumers using the installed CMake package target `libbsa::libbsa`; see `CMakeLists.txt` and `tests/package-consumer/CMakeLists.txt`
- Output publication relies on Windows filesystem semantics and APIs in `src/detail/atomic_file_ops.hpp` and `src/detail/writer_publish.cpp`

---

*Stack analysis: 2026-05-12*
