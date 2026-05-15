---
last_mapped: 2026-05-15
last_mapped_commit: ead68b6856b37c3ea3acbf007180951c3cc22698
---

# External Integrations

**Analysis Date:** 2026-05-15

## APIs & External Services

**Runtime Library Integrations:**
- libdeflate - Private codec backend for raw deflate payload compression/decompression.
  - SDK/Client: vcpkg package `libdeflate` from `vcpkg.json`; CMake target selected in `CMakeLists.txt`; header `<libdeflate.h>` used by `src/detail/deflate_codec.cpp`.
  - Auth: Not applicable.
  - Usage pattern: `compress_deflate` and `decompress_deflate_exact` allocate libdeflate compressor/decompressor objects and fail when decoded bytes do not match archive metadata.
- LZ4 frame API - Private codec backend for Skyrim SE/AE BSA v105 compressed payloads.
  - SDK/Client: vcpkg package `lz4`; target selected as `LZ4::lz4_static`, `LZ4::lz4_shared`, or `lz4::lz4` in `CMakeLists.txt`; header `<lz4frame.h>` used by `src/detail/lz4_frame_codec.cpp`.
  - Auth: Not applicable.
  - Usage pattern: `compress_lz4_frame`, `decompress_lz4_frame_exact`, and `decompress_lz4_frame_exact_to_sink` use `LZ4F_*` APIs for whole-buffer and streaming-to-sink decode paths.
- Raw LZ4 block API - Private codec backend for Starfield BA2 v3 `CompressionMethod == 3` payloads.
  - SDK/Client: vcpkg package `lz4`; header `<lz4.h>` used by `src/detail/lz4_block_codec.cpp`.
  - Auth: Not applicable.
  - Usage pattern: `compress_lz4_block` uses `LZ4_compress_default`; `decompress_lz4_block_exact` uses `LZ4_decompress_safe` and checks the exact expected decoded size.
- DirectXTex - Private DDS metadata and image analyzer for BA2 DX10 texture archives.
  - SDK/Client: vcpkg package `directxtex`; CMake target `Microsoft::DirectXTex` in `CMakeLists.txt`; header `<DirectXTex.h>` used by `src/texture/directxtex_analyzer.cpp`.
  - Auth: Not applicable.
  - Usage pattern: `DirectX::GetMetadataFromDDSMemory` reads metadata; `DirectX::LoadFromDDSMemory` loads DDS source subresources for BA2 DX10 writer snapshots. Public headers expose only libbsa-owned `texture_metadata` and raw numeric DXGI format values in `include/libbsa/archive.hpp`.
- Windows API - Host path conversion, DLL export semantics, and safe publication helpers.
  - SDK/Client: Windows SDK headers such as `<windows.h>` and `<bcrypt.h>` in `src/detail/host_file_path.cpp`, `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`, `src/detail/writer_publish.cpp`, and `src/detail/atomic_file_ops.hpp`; system library `bcrypt` linked in `CMakeLists.txt`.
  - Auth: Not applicable.
  - Usage pattern: `MultiByteToWideChar` preserves the public UTF-8 host path contract; `BCryptGenRandom` generates random BA2 DX10 snapshot directory suffixes; `__declspec` in `include/libbsa/export.hpp` controls shared-library imports/exports.

**Build, Test, and Documentation Services:**
- GitHub Actions - Windows CI service for build/test validation.
  - SDK/Client: Workflow file `.github/workflows/ci.yml`.
  - Auth: GitHub repository token provided by Actions runtime; no project secrets detected or required in workflow steps.
  - Usage pattern: Runs `actions/checkout@v6` with submodules, installs CMake 4.3.2 from Kitware GitHub releases, runs CMake configure/build/test presets, then verifies `TES5Edit/` stayed clean.
- vcpkg registry - Dependency acquisition through Microsoft vcpkg.
  - SDK/Client: `vcpkg.json`, `vcpkg-configuration.json`, and CMake toolchain path in `CMakePresets.json`.
  - Auth: Not applicable for the public registry.
  - Usage pattern: `vcpkg-configuration.json` points at `https://github.com/microsoft/vcpkg` with baseline `12dcccadfe573d0eaa6c67a968413ded7805d256`.
- Kitware CMake release downloads - CI obtains a pinned CMake binary archive.
  - SDK/Client: PowerShell `Invoke-WebRequest` in `.github/workflows/ci.yml` downloads `https://github.com/Kitware/CMake/releases/download/v$cmakeVersion/$cmakeArchive`.
  - Auth: Not applicable.
- Doxygen - Optional local documentation generator.
  - SDK/Client: `find_package(Doxygen QUIET)` in `CMakeLists.txt`; configuration in `docs/Doxyfile.in`.
  - Auth: Not applicable.
  - Usage pattern: Generates public API documentation from `include/libbsa/`, `docs/api-mainpage.md`, and `docs/thread-safety.md`; excludes `src/`, `TES5Edit/`, `tests/`, build outputs, and `vcpkg_installed/`.

**Reference and Compatibility Inputs:**
- TES5Edit / BSArchPro - Read-only behavioral reference submodule.
  - SDK/Client: Git submodule directory `TES5Edit/`; referenced by project policy in `AGENTS.md`, `docs/target-format-guide.md`, and CI cleanliness checks in `.github/workflows/ci.yml`.
  - Auth: Not applicable.
  - Usage pattern: May guide behavior, but must not be edited, formatted, staged, built into libbsa, used as writable fixtures, or treated as vendored source.
- Optional BSArchPro-derived comparison manifests - Local compatibility smoke inputs only.
  - SDK/Client: `tests/unit/local_game_fixture_tests.cpp`; documented in `docs/compatibility-evidence.md`.
  - Auth: Not applicable, but paths may point at local user-owned game/corpus data.
  - Environment: `LIBBSA_GAME_FIXTURES`, `LIBBSA_BSARCHPRO_EXPECTED`.
  - Usage pattern: Optional checks must remain skipped unless local fixture variables are set; do not commit copyrighted archive bytes, extracted payloads, or BSArchPro-generated corpus output.

## Data Storage

**Databases:**
- Not detected. There are no database clients, ORM dependencies, migrations, or connection strings in `vcpkg.json`, `CMakeLists.txt`, or source includes.

**File Storage:**
- Local filesystem only.
  - Archive reading/writing accepts host paths as UTF-8 strings and resolves them to Windows filesystem paths in `src/detail/host_file_path.cpp`.
  - Archive parsing and extraction use local file streams and caller-owned sinks in `include/libbsa/archive.hpp` and implementation files under `src/formats/` and `src/detail/`.
  - Writer output publication uses same-directory temporary output and Windows publication behavior described in `docs/target-format-guide.md` and implemented under `src/detail/writer_publish.cpp`.
  - BA2 DX10 writer source data is snapshotted into writer-owned temporary directories named `libbsa-dx10-snapshot-*` by `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`.
  - Generated test fixtures are committed/synthesized under `tests/fixtures/generated/` via targets in `tests/CMakeLists.txt`.

**Caching:**
- No external cache service detected.
- In-process buffers and writer-owned temporary snapshots are used instead of a cache server. Examples include bounded scratch buffers in `src/detail/lz4_frame_codec.cpp`, byte-vector helpers under `src/detail/byte_vector.hpp`, and BA2 DX10 snapshot lifecycle described in `docs/target-format-guide.md`.

## Authentication & Identity

**Auth Provider:**
- Not applicable. libbsa is a local C++ library with no user accounts, tokens, OAuth, SSO, or network authentication paths detected.

**Secrets:**
- Not detected. No `.env` files were found during mapping.
- CI workflow `.github/workflows/ci.yml` does not reference project-specific secrets.
- Optional local fixture paths are supplied by environment variables (`LIBBSA_GAME_FIXTURES`, `LIBBSA_BSARCHPRO_EXPECTED`) and should contain paths only, not secrets.

## Monitoring & Observability

**Error Tracking:**
- None. No Sentry, OpenTelemetry, Application Insights, logging SaaS, crash-reporting SDK, or telemetry dependency is present in `vcpkg.json`, `CMakeLists.txt`, or source includes.

**Logs:**
- Library diagnostics are returned as `libbsa::result<T>` errors, not emitted to a logging framework. The error model is declared in `include/libbsa/result.hpp` and exercised throughout codec and parser code such as `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, and `src/texture/directxtex_analyzer.cpp`.
- Build/test logs come from CMake, CTest, Catch2, and GitHub Actions. `ctest --output-on-failure` is used in `.github/workflows/ci.yml` and enabled in `CMakePresets.json` test presets.
- Benchmark reports are local files generated by `libbsa_benchmark_report` into `${CMAKE_BINARY_DIR}/benchmarks/libbsa-benchmark.json` and `${CMAKE_BINARY_DIR}/benchmarks/libbsa-benchmark.md` in `CMakeLists.txt`.

## CI/CD & Deployment

**Hosting:**
- No runtime hosting platform. libbsa is distributed as a Windows C++ library.
- Build artifact shape is a CMake install/export package: `CMakeLists.txt` installs `libbsaTargets`, public header file sets, `libbsaConfig.cmake`, and `libbsaConfigVersion.cmake` under `${CMAKE_INSTALL_LIBDIR}/cmake/libbsa`.

**CI Pipeline:**
- GitHub Actions via `.github/workflows/ci.yml`.
- Matrix lanes:
  - `windows-msvc-debug-static`
  - `windows-msvc-debug-shared`
  - `windows-msvc-release-static`
  - `windows-msvc-release-shared`
- Separate hardening lane:
  - `windows-msvc-asan-static`
- CI steps:
  1. Check out sources with submodules using `actions/checkout@v6`.
  2. Install and verify CMake 4.3.2.
  3. Configure with `cmake --preset <preset>`.
  4. Build with `cmake --build --preset <preset>`.
  5. Test with `ctest --preset <preset> --output-on-failure`.
  6. Run `git status --short TES5Edit` and fail if the submodule changed.

**Package Consumer Proof:**
- `tests/package-consumer/CMakeLists.txt` finds `libbsa CONFIG REQUIRED`, links `libbsa::libbsa`, and runs a smoke executable.
- `tests/package-consumer/smoke.cmake` installs libbsa into a temporary prefix and configures the consumer project against that install.
- Runtime DLL handling for shared installs is verified by `tests/package-consumer/copy-runtime-dlls.cmake` and `tests/package-consumer/verify-runtime-dll-copy.cmake`.
- Shared-library export shape is checked on Windows by `tests/export-surface/check-dll-exports.cmake` when `WIN32 AND BUILD_SHARED_LIBS` in `tests/CMakeLists.txt`.

## Environment Configuration

**Required env vars:**
- `VCPKG_ROOT` - Required for local CMake presets because `CMakePresets.json` points `CMAKE_TOOLCHAIN_FILE` at `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`.
- `LIBBSA_CMAKE_VERSION` - Set by CI to `4.3.2` in `.github/workflows/ci.yml`; not required for local builds if a compatible CMake is already installed.

**Optional env vars:**
- `LIBBSA_GAME_FIXTURES` - Enables optional local game fixture checks in `tests/unit/local_game_fixture_tests.cpp` and is documented in `docs/compatibility-evidence.md`.
- `LIBBSA_BSARCHPRO_EXPECTED` - Points at BSArchPro-derived expected metadata for optional comparison checks in `tests/unit/local_game_fixture_tests.cpp`.

**Secrets location:**
- Not applicable. No secret files or committed secret configuration were detected.
- Do not store game archives, extracted proprietary payloads, BSArchPro-generated corpus output, API keys, or credentials in committed repository paths. Optional local compatibility data belongs outside committed fixtures and must not use `TES5Edit/` as a writable workspace.

## Webhooks & Callbacks

**Incoming:**
- None. The library exposes local C++ APIs, not HTTP endpoints or webhook receivers.

**Outgoing:**
- None at runtime. libbsa does not make network calls during archive read/write/extract operations.
- CI makes outbound download requests to GitHub-hosted CMake release artifacts in `.github/workflows/ci.yml` and fetches vcpkg dependencies via the vcpkg toolchain/registry configuration.

## Public API Boundary for Integrations

- Consumers should include `include/libbsa/libbsa.hpp`, not private implementation headers under `src/`.
- Consumers link `libbsa::libbsa` from the installed CMake package; this is proven by `tests/package-consumer/CMakeLists.txt`.
- Public headers expose only standard C++20 and libbsa-owned types. Codec and platform backends remain private:
  - Do not expose `libdeflate_*` types from `src/detail/deflate_codec.cpp`.
  - Do not expose `LZ4F_*` or raw LZ4 API types from `src/detail/lz4_frame_codec.cpp` or `src/detail/lz4_block_codec.cpp`.
  - Do not expose DirectXTex or DirectX/DXGI types from `src/texture/directxtex_analyzer.cpp`; keep `texture_metadata::dxgi_format` numeric in `include/libbsa/archive.hpp`.
  - Do not expose Windows handles or BCrypt types; keep Windows-specific behavior behind implementation files and `include/libbsa/export.hpp` macros.

---

*Integration audit: 2026-05-15*
