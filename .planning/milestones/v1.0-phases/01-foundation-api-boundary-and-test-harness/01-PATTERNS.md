# Phase 01 Pattern Map

**Phase:** 01 — Foundation, API Boundary, and Test Harness  
**Created:** 2026-05-07

## Existing Codebase State

- No root `CMakeLists.txt`, `CMakePresets.json`, `include/`, `src/`, `tests/`, fixture tree, or CI workflow exists yet.
- `vcpkg.json` already exists with `libdeflate`, `lz4`, `directxtex`, `catch2`, and a committed builtin baseline.
- `.clangd` already assumes C++20 and `include/`.
- `.gitignore` has Visual Studio/build ignores but does not yet ignore `tests/fixtures/local/`.

## Analog Patterns to Preserve

### `.clangd`

```yaml
CompileFlags:
  Add:
    - -std=c++20
    - -Iinclude
```

Implication: Phase 1 should create `include/libbsa/` and keep public headers conventional C++20 headers.

### `vcpkg.json`

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
  "builtin-baseline": "12dcccadfe573d0eaa6c67a968413ded7805d256"
}
```

Implication: do not replace the manifest; plans may add `vcpkg-configuration.json` but should preserve the direct dependencies per D-26.

### `.gitignore`

Existing comments are accurate and must not be removed. Append local fixture ignores rather than rewriting the file.

## Planned File Roles

| File | Role |
|------|------|
| `CMakeLists.txt` | Root CMake project, `libbsa` target, install/export, optional test enablement |
| `cmake/libbsaConfig.cmake.in` | Installed package config template |
| `CMakePresets.json` | Static/shared MSVC Debug configure/build/test presets |
| `vcpkg-configuration.json` | vcpkg registry baseline companion if needed |
| `include/libbsa/result.hpp` | Public C++20 result/error model |
| `include/libbsa/archive.hpp` | Public `archive_reader` facade stub |
| `include/libbsa/libbsa.hpp` | Umbrella public header |
| `include/libbsa/version.hpp` | Public version constants/macros |
| `src/archive.cpp` | Stub `archive_reader::open(std::string_view)` implementation |
| `src/libbsa.cpp` | Anchor translation unit if needed by library target |
| `tests/CMakeLists.txt` | Catch2/CTest integration and package consumer test wiring |
| `tests/unit/*.cpp` | Public boundary unit tests |
| `tests/package-consumer/*` | Installed-package `find_package(libbsa CONFIG REQUIRED)` smoke test |
| `tests/fixtures/README.md` | Fixture policy and label taxonomy |
| `.github/workflows/ci.yml` | Windows/MSVC static and shared configure/build/test workflow |

## Constraints for Executors

- Never modify files under `TES5Edit/`.
- Do not include dependency headers in `include/libbsa/*.hpp`.
- Do not introduce writer shells, stream abstractions, real archive parsing, compression adapters, hash/path services, DDS behavior, or C++20 modules.
- Add Doxygen comments for public APIs and any non-trivial methods.
