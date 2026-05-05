# Phase 1: Build, Error, and Test Foundation - Specification

**Created:** 2026-05-05
**Ambiguity score:** 0.10 (gate: <= 0.20)
**Requirements:** 6 locked

## Goal

libbsa changes from an empty planning-only repository into a buildable C++20 library skeleton with vcpkg-managed dependencies, CTest/Catch2 test execution, and a C++20-compatible structured result/error API verified by tests.

## Background

The repository currently contains planning artifacts, `docs/PRD.md`, an empty `README.md`, and the read-only `TES5Edit/` reference submodule. No implementation scaffold exists yet: there is no `CMakeLists.txt`, `vcpkg.json`, `include/`, `src/`, `tests/`, install/export setup, public library target, or C++ source code. Phase 1 is triggered by the need to establish the foundation that every later archive parser, codec, writer, and compatibility test will depend on.

## Requirements

1. **Buildable C++20 library target**: The repository provides a CMake-configurable `libbsa` library target using C++20.
   - Current: No CMake project or C++ target exists.
   - Target: `cmake -S . -B <build-dir>` configures a project with a `libbsa` library target and public headers under `include/libbsa/`.
   - Acceptance: A clean configure and build succeeds with the configured preset or documented CMake invocation.

2. **vcpkg manifest dependencies**: The repository declares required dependency acquisition through vcpkg manifest mode.
   - Current: No vcpkg manifest exists.
   - Target: `vcpkg.json` declares `libdeflate`, `lz4`, `directxtex`, and `catch2` as project dependencies, with reproducibility metadata such as a baseline or configuration file where appropriate.
   - Acceptance: CMake can find the required packages through the vcpkg toolchain and the build does not require vendored dependency source.

3. **Public header isolation**: Public libbsa headers do not expose implementation dependency, platform, UI, Delphi, or TES5Edit types.
   - Current: No public headers exist.
   - Target: Public headers expose only libbsa-owned C++20 types for the foundational API.
   - Acceptance: A consumer translation unit can include the public foundation header without including libdeflate, LZ4, DirectXTex, Windows platform, Delphi, or TES5Edit headers.

4. **Structured result/error API**: The library provides a C++20-compatible local result/error surface for operations that can fail.
   - Current: No error model exists in code.
   - Target: Public API includes a libbsa-owned result/error type or equivalent local abstraction that supports success values and categorized errors without exposing C++23-only `std::expected`.
   - Acceptance: Tests prove successful value access, error propagation, and representative error categories for unsupported format, malformed archive, I/O failure, and decompression failure.

5. **CTest/Catch2 test foundation**: Maintainers can run the initial test suite through CTest.
   - Current: No test target or test runner exists.
   - Target: CMake builds a Catch2-based test executable and registers it with CTest.
   - Acceptance: `ctest --test-dir <build-dir> --output-on-failure` runs and passes tests that verify the library target links and the result/error behavior is correct.

6. **TES5Edit reference boundary documented in foundation**: The foundation makes the read-only reference boundary visible to future implementers.
   - Current: The boundary is documented in planning files, but no implementation-facing project scaffold exists.
   - Target: Repository docs or source comments in the new scaffold state that `TES5Edit/` is read-only reference material and is not compiled, formatted, staged, or linked into libbsa.
   - Acceptance: The build does not add files under `TES5Edit/`, does not include `TES5Edit/` in library sources, and documentation visible from the foundation reiterates the boundary.

## Boundaries

**In scope:**
- Root CMake project and build presets or documented configure path for a C++20 library.
- `vcpkg.json` and any companion vcpkg configuration needed for dependency resolution.
- Initial `include/`, `src/`, and `tests/` layout for the library.
- Public foundational result/error API compatible with C++20.
- Catch2 and CTest integration with passing tests for linkability and result/error behavior.
- Documentation or comments that preserve the `TES5Edit/` read-only boundary.

**Out of scope:**
- Archive binary reader/writer primitives - Phase 2 owns streaming API, archive model, detection, and hash foundations.
- Archive format detection or parsing - Phase 2 and later read phases own real archive behavior.
- TES3, TES4-family, BA2 GNRL, or BA2 DDS extraction - later read phases own format-specific functionality.
- Compression adapter implementation - Phase 3 owns deflate, LZ4 frame, and LZ4 block behavior.
- DDS analysis or DirectXTex wrapper implementation - BA2 DDS phases own texture behavior.
- Writer planning or archive emission - writer phases own archive creation.
- Productized CLI or GUI tooling - the project scope is a reusable library.
- Any modification to `TES5Edit/` - it is read-only reference material.

## Constraints

- The public API must remain C++20-compatible; do not expose C++23 `std::expected`.
- Dependency acquisition must use vcpkg manifest mode with `libdeflate`, official `lz4`, `DirectXTex`, and `Catch2`.
- Public headers must avoid leaking dependency types such as `libdeflate_*`, `LZ4F_*`, `DirectX::ScratchImage`, `HRESULT`, Windows SDK types, or TES5Edit/Delphi names.
- The implementation must not compile, edit, format, stage, or otherwise mutate anything under `TES5Edit/`.
- Initial tests must be deterministic and not depend on mutable real game archives.

## Acceptance Criteria

- [ ] `cmake -S . -B <build-dir>` configures libbsa as a C++20 project using vcpkg-managed dependencies.
- [ ] `cmake --build <build-dir>` builds the `libbsa` library target and the initial test executable.
- [ ] `ctest --test-dir <build-dir> --output-on-failure` passes.
- [ ] `vcpkg.json` declares `libdeflate`, `lz4`, `directxtex`, and `catch2` without vendoring their sources.
- [ ] A consumer translation unit can include the foundational public header without including compression, texture, platform, Delphi, or TES5Edit headers.
- [ ] Tests verify success and error paths for the local result/error API, including representative unsupported-format, malformed-archive, I/O, and decompression error categories.
- [ ] No build source list, install rule, or test target includes files from `TES5Edit/`.
- [ ] Foundation documentation or source comments explicitly state that `TES5Edit/` is read-only reference material.

## Ambiguity Report

| Dimension          | Score | Min   | Status | Notes |
|--------------------|-------|-------|--------|-------|
| Goal Clarity       | 0.94  | 0.75  | OK     | Goal narrowed to buildable skeleton plus local result/error API. |
| Boundary Clarity   | 0.92  | 0.70  | OK     | Archive parsing, binary I/O, compression adapters, DDS, and writers are explicitly excluded. |
| Constraint Clarity | 0.86  | 0.65  | OK     | C++20, vcpkg dependency policy, public header isolation, and TES5Edit boundary are locked. |
| Acceptance Criteria| 0.86  | 0.70  | OK     | Build, CTest, dependency, header-isolation, error-API, and boundary checks are pass/fail. |
| **Ambiguity**      | 0.10  | <=0.20| OK     | Gate passed after round 2. |

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Minimum successful Phase 1 deliverable | Buildable skeleton plus structured error/result API. |
| 1 | Researcher | Dependency proof needed in Phase 1 | Resolve all required dependencies through vcpkg. |
| 1 | Researcher | Error surface shape | Use a local C++20-compatible result/error type; do not expose `std::expected`. |
| 2 | Simplifier / Boundary Keeper | Adjacent work to exclude | No archive parsing, binary reader/writer, detection, hash/path handling, compression adapters, or real archive fixtures. |
| 2 | Simplifier | First tests to verify | CTest/Catch2 proves the library links and result/error behavior works. |

---

*Phase: 01-build-error-and-test-foundation*
*Spec created: 2026-05-05*
*Next step: /gsd-discuss-phase 1 - implementation decisions (how to build what's specified above)*
