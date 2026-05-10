# External Integrations

**Analysis Date:** 2026-05-10

## APIs & External Services

**Native Dependency APIs:**
- `libdeflate` - Private raw deflate codec API for archive payload compression/decompression.
  - SDK/Client: `libdeflate.h` included in `src/detail/deflate_codec.cpp`; dependency declared in `vcpkg.json` and linked in `CMakeLists.txt`.
  - Auth: Not applicable.
- `lz4` - Private LZ4 frame and raw block codec APIs.
  - SDK/Client: `lz4frame.h` in `src/detail/lz4_frame_codec.cpp` and `lz4.h` in `src/detail/lz4_block_codec.cpp`; dependency declared in `vcpkg.json` and linked in `CMakeLists.txt`.
  - Auth: Not applicable.
- `DirectXTex` - Private DDS metadata and source-image analyzer for BA2 DX10 archives.
  - SDK/Client: `DirectXTex.h` included in `src/texture/directxtex_analyzer.cpp`; dependency declared as `directxtex` in `vcpkg.json` and linked as `Microsoft::DirectXTex` in `CMakeLists.txt`.
  - Auth: Not applicable.

**Build and Package Services:**
- vcpkg builtin registry - Dependency acquisition and version baseline source for `libdeflate`, `lz4`, `directxtex`, `catch2`, and `nlohmann-json`.
  - SDK/Client: vcpkg manifest mode through `vcpkg.json`, `vcpkg-configuration.json`, and `CMakePresets.json`.
  - Auth: Not detected.
- GitHub Actions - CI service for Windows static/shared builds and tests.
  - SDK/Client: `.github/workflows/ci.yml` uses `actions/checkout@v4`, CMake presets, CTest presets, and a `git status --short TES5Edit` guard.
  - Auth: Repository-provided GitHub Actions token only; no repo secrets detected in `.github/workflows/ci.yml`.
- GitHub `TES5Edit` submodule - Read-only behavior reference for BSArchPro compatibility.
  - SDK/Client: `.gitmodules` points `TES5Edit/` at `https://github.com/TES5Edit/TES5Edit.git`; CI checks the submodule remains unchanged in `.github/workflows/ci.yml`.
  - Auth: Public HTTPS submodule URL; no credentials detected.

**Optional Local Compatibility Inputs:**
- BSArchPro-derived comparison manifests - Optional local compatibility evidence for game archives.
  - SDK/Client: `tests/unit/local_game_fixture_tests.cpp` reads `LIBBSA_BSARCHPRO_EXPECTED` or `bsarchpro_expected.json` under `LIBBSA_GAME_FIXTURES`; policy is documented in `tests/fixtures/README.md` and `docs/compatibility-evidence.md`.
  - Auth: Not applicable; files are local-only and must not be committed.
- Local game archive corpus - Optional local test data for smoke/compare coverage.
  - SDK/Client: `tests/unit/local_game_fixture_tests.cpp` and `tests/fixtures/README.md` use `LIBBSA_GAME_FIXTURES`; committed placeholder is `tests/fixtures/local/.gitkeep`.
  - Auth: Not applicable; local copyrighted data is ignored by `.gitignore`.

## Data Storage

**Databases:**
- Not detected.
  - Evidence: `src/`, `include/`, `tests/`, `benchmarks/`, `CMakeLists.txt`, `vcpkg.json`, and `.github/workflows/ci.yml` do not declare database clients or database connection configuration.
  - Connection: Not applicable.
  - Client: Not applicable.

**File Storage:**
- Local filesystem only.
  - Archive read/validate entry points accept host paths through `include/libbsa/archive.hpp` and `include/libbsa/validation.hpp`; implementation opens files in `src/archive.cpp`, `src/validation.cpp`, `src/formats/bsa/*_parser.cpp`, and `src/formats/ba2/*_parser.cpp`.
  - Writers publish new archives to caller-provided host paths through `include/libbsa/writer.hpp`; format writers live in `src/formats/bsa/*_writer.cpp` and `src/formats/ba2/*_writer.cpp`.
  - BA2 DX10 writer snapshots source DDS subresources in temp files under `src/formats/ba2/ba2_dx10_writer.cpp`.
  - Test fixtures are committed under `tests/fixtures/generated/`; local game fixtures are ignored under `tests/fixtures/local/`.
  - Benchmark reports are generated under `build/<preset>/benchmarks/` by `benchmarks/libbsa_benchmarks.cpp` and the `libbsa_benchmark_report` target in `CMakeLists.txt`.

**Caching:**
- Application/runtime cache: Not detected in `src/` or `include/`.
- Build/dependency cache: vcpkg install artifacts are local build output under `build/<preset>/vcpkg_installed/`, referenced by `tests/package-consumer/smoke.cmake` for consumer `CMAKE_PREFIX_PATH` setup.

## Authentication & Identity

**Auth Provider:**
- None.
  - Implementation: No authentication provider, OAuth client, token validation, user identity model, or credential flow is implemented under `src/`, `include/`, `tests/`, or `.github/workflows/ci.yml`.
  - Public APIs in `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and `include/libbsa/validation.hpp` operate on local paths and caller-owned sinks, not identities.

## Monitoring & Observability

**Error Tracking:**
- None.
  - Evidence: No Sentry, OpenTelemetry, external logging, or crash reporting dependency is declared in `vcpkg.json`, `CMakeLists.txt`, or `.github/workflows/ci.yml`.

**Logs:**
- Structured API errors use `libbsa::result<T>`, `libbsa::error`, and `libbsa::error_code` in `include/libbsa/result.hpp`.
- Validation diagnostics and compatibility warnings are returned through `include/libbsa/validation.hpp` and implemented in `src/validation.cpp`; callers own display/logging.
- Build and test diagnostics come from CMake and CTest in `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json`, and `.github/workflows/ci.yml`.
- Benchmark reports are explicit local files written by `benchmarks/libbsa_benchmarks.cpp`, documented in `benchmarks/README.md`.

## CI/CD & Deployment

**Hosting:**
- Not applicable for runtime service hosting.
- Distribution path is CMake install/export packaging from `CMakeLists.txt`, `cmake/libbsaConfig.cmake.in`, and `tests/package-consumer/smoke.cmake`.

**CI Pipeline:**
- GitHub Actions.
  - Workflow: `.github/workflows/ci.yml`.
  - Triggers: `push` and `pull_request` in `.github/workflows/ci.yml`.
  - Matrix: `windows-msvc-debug-static` and `windows-msvc-debug-shared` presets from `CMakePresets.json`.
  - Steps: checkout with submodules, `cmake --preset`, `cmake --build --preset`, `ctest --preset --output-on-failure`, and `TES5Edit/` cleanliness verification in `.github/workflows/ci.yml`.
  - Package smoke: `tests/package-consumer/smoke.cmake` installs the library, configures a separate consumer in `tests/package-consumer/`, builds it, and runs `libbsa_package_consumer_run`.

## Environment Configuration

**Required env vars:**
- `VCPKG_ROOT` - Required for preset-based configure because `CMakePresets.json` sets `CMAKE_TOOLCHAIN_FILE` to `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`; `.github/workflows/ci.yml` sets it for CI.

**Optional env vars and CMake variables:**
- `LIBBSA_GAME_FIXTURES` - Optional local fixture root for `tests/unit/local_game_fixture_tests.cpp`, documented in `tests/fixtures/README.md` and `docs/compatibility-evidence.md`.
- `LIBBSA_BSARCHPRO_EXPECTED` - Optional local BSArchPro-derived expected-manifest path for `tests/unit/local_game_fixture_tests.cpp`.
- `LIBBSA_BUILD_TESTS` - CMake option declared in `CMakeLists.txt` and set by `CMakePresets.json`.
- `LIBBSA_BUILD_BENCHMARKS` - CMake option declared in `CMakeLists.txt` for `benchmarks/libbsa_benchmarks.cpp` and `libbsa_benchmark_report`.
- `BUILD_SHARED_LIBS` - CMake option set by `CMakePresets.json` for static/shared library builds.
- `LIBBSA_SOURCE_DIR` - Test-only compile definition set in `tests/CMakeLists.txt`.
- `LIBBSA_BUILD_DIR`, `LIBBSA_INSTALL_PREFIX`, `CONSUMER_SOURCE_DIR`, `CONSUMER_BUILD_DIR`, and `CONFIG` - Script variables required by `tests/package-consumer/smoke.cmake`.

**Secrets location:**
- Not detected.
  - `.env` files are absent in the repo root scan and `*.env` is ignored by `.gitignore`.
  - No credential files were found in the non-build, non-`TES5Edit/` source scan.
  - GitHub workflow `.github/workflows/ci.yml` does not reference custom secrets.

## Webhooks & Callbacks

**Incoming:**
- None.
  - Evidence: No server, route, webhook, socket listener, or network callback code is implemented under `src/`, `include/`, `tests/`, or `benchmarks/`.

**Outgoing:**
- None at runtime.
  - Evidence: Public library APIs in `include/libbsa/` and implementations in `src/` operate on local host paths and caller-owned sinks.
- Build-time dependency and CI network access may occur through vcpkg package acquisition from `vcpkg.json` and GitHub Actions checkout/submodule operations in `.github/workflows/ci.yml`, but no runtime HTTP/API client is present in `src/` or `include/`.

---

*Integration audit: 2026-05-10*
