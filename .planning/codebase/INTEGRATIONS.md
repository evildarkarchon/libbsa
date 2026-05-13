# External Integrations

**Analysis Date:** 2026-05-12

## APIs & External Services

**Compression and binary-analysis libraries:**
- `libdeflate` - raw deflate codec used for BSA and BA2 payload compression/decompression
  - SDK/Client: vcpkg package declared in `vcpkg.json`, linked from `CMakeLists.txt`, wrapped in `src/detail/deflate_codec.cpp`
  - Auth: Not applicable
- `lz4` - LZ4 frame and raw block codec used for Skyrim SE/AE BSA and Starfield BA2 payloads
  - SDK/Client: vcpkg package declared in `vcpkg.json`, linked from `CMakeLists.txt`, wrapped in `src/detail/lz4_frame_codec.cpp` and `src/detail/lz4_block_codec.cpp`
  - Auth: Not applicable
- `DirectXTex` - DDS metadata loading and texture source analysis for BA2 DX10 workflows
  - SDK/Client: vcpkg package declared in `vcpkg.json`, linked from `CMakeLists.txt`, used privately in `src/texture/directxtex_analyzer.cpp`
  - Auth: Not applicable

**Reference/tooling repositories:**
- `TES5Edit` - read-only behavioral reference submodule for BSArchPro compatibility
  - SDK/Client: git submodule declared in `.gitmodules` and constrained by `AGENTS.md`
  - Auth: Not applicable
- `microsoft/vcpkg` registry - dependency registry source pinned by baseline
  - SDK/Client: git registry in `vcpkg-configuration.json`
  - Auth: Not applicable

## Data Storage

**Databases:**
- None
  - Connection: Not applicable
  - Client: Not applicable

**File Storage:**
- Local filesystem only
  - Archive reading/writing uses host paths through public APIs in `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and implementations in `src/archive.cpp` and `src/formats/**/*.cpp`
  - BA2 DX10 writer snapshots temporary DDS subresources under the system temp root in `src/formats/ba2/ba2_dx10_prepare.cpp`
  - Writer finalization publishes archives through same-directory temp staging in `src/detail/writer_publish.cpp` and `src/detail/atomic_file_ops.hpp`

**Caching:**
- None

## Authentication & Identity

**Auth Provider:**
- None
  - Implementation: Not applicable

## Monitoring & Observability

**Error Tracking:**
- None

**Logs:**
- No dedicated logging framework detected; the library returns structured `libbsa::result<T>` errors and `validation_report` diagnostics instead of emitting logs from core runtime paths in `include/libbsa/result.hpp`, `include/libbsa/validation.hpp`, and `src/**/*.cpp`

## CI/CD & Deployment

**Hosting:**
- Not detected; this repository builds a library package rather than a hosted service

**CI Pipeline:**
- GitHub Actions Windows matrix build in `.github/workflows/ci.yml`
  - Checks out submodules, installs CMake 4.3.2, configures both presets from `CMakePresets.json`, builds, runs `ctest`, and verifies `TES5Edit` remains unchanged
- CMake install/export packaging in `CMakeLists.txt`
- Package-consumer verification in `tests/CMakeLists.txt` and `tests/package-consumer/CMakeLists.txt`

## Environment Configuration

**Required env vars:**
- `VCPKG_ROOT` - required by `CMakePresets.json` and documented in `README.md`
- `LIBBSA_CMAKE_VERSION` - CI-only workflow variable in `.github/workflows/ci.yml`

**Secrets location:**
- Not detected

## Webhooks & Callbacks

**Incoming:**
- None

**Outgoing:**
- None from library runtime
- CI downloads the requested CMake archive from GitHub Releases in `.github/workflows/ci.yml`

## Tooling Integrations

**Documentation tooling:**
- Doxygen target `libbsa_docs` reads public headers and docs inputs from `docs/Doxyfile.in` via `CMakeLists.txt`

**Fixture and validation tooling:**
- Python 3 manifest validator runs through `validate_fixture_manifests` in `tests/CMakeLists.txt` using `tests/fixtures/generated/validate_fixture_manifests.py`
- Fixture generators are compiled C++ tools declared in `tests/CMakeLists.txt` and write into `tests/fixtures/generated/archives`

**Consumer integration tooling:**
- Installed-package smoke tests compile and run `tests/package-consumer/main.cpp` against `find_package(libbsa CONFIG REQUIRED)` in `tests/package-consumer/CMakeLists.txt`
- Public usage examples are mirrored in `docs/integration-examples.md`

**Project workflow skills:**
- Repository-local OpenSpec workflow skills are present under `.claude/skills/*/SKILL.md` and support proposal/design/task workflows rather than library runtime behavior
- Additional implementation rule files under `.claude/skills/*/rules/`: Not detected in this scan

---

*Integration audit: 2026-05-12*
