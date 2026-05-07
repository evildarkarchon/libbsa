# Phase 1: Foundation, API Boundary, and Test Harness - Specification

**Created:** 2026-05-07
**Ambiguity score:** 0.15 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

libbsa changes from a planning-only repository into a buildable C++20 library skeleton with a dependency-clean public API boundary, minimal callable facade stubs, a labeled Catch2/CTest harness, legal fixture layout, and CI build/test validation.

## Background

The repository currently contains planning documents, `docs/PRD.md`, project agent instructions, and the read-only `TES5Edit/` reference submodule. It does not yet contain `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json`, `include/`, `src/`, `tests/`, fixture directories, or CI workflow files. Phase 1 is triggered by that gap: every later parser, compression adapter, writer, and compatibility test needs a reusable library foundation that builds without leaking implementation dependencies or mutating `TES5Edit/`.

## Requirements

1. **Buildable C++20 library skeleton**: The repository provides a CMake/vcpkg project that builds libbsa as a reusable C++20 library target.
   - Current: No root `CMakeLists.txt`, vcpkg manifest, presets, `include/`, or `src/` tree exists.
   - Target: A consumer or maintainer can configure the project with CMake and vcpkg and build a `libbsa` library target in C++20 mode.
   - Acceptance: A clean configure and build using the project preset completes successfully and produces the `libbsa` target without compiling or linking any files from `TES5Edit/`.

2. **Static and shared build selection**: The foundation supports both static and shared library builds without requiring public header changes.
   - Current: There is no library target and no build option for linkage type.
   - Target: The build exposes a standard CMake-controlled way to build libbsa as static or shared while preserving the same installed public headers.
   - Acceptance: Static and shared configurations both configure and build successfully, and the public include set is identical between them.

3. **Dependency-clean public headers**: Public libbsa headers do not transitively include implementation dependency headers or TES5Edit files.
   - Current: No public headers exist, and dependency boundaries are only documented in planning files.
   - Target: Headers under `include/libbsa/` expose libbsa-owned types only and do not include `libdeflate`, `lz4`, `DirectXTex`, Windows SDK, or `TES5Edit/` headers.
   - Acceptance: A consumer translation unit that includes the public libbsa umbrella/header set compiles without those dependency include paths being required as public include directories.

4. **C++20-compatible result/error boundary**: Public APIs expose structured failure values for I/O and format failures without using C++23-only `std::expected`.
   - Current: There is no result or error type implementation.
   - Target: Public headers define a local result/error model usable in C++20, with explicit error categories or codes suitable for later I/O and format failures.
   - Acceptance: Tests can construct success and failure results, inspect error values, and compile in C++20 mode without referencing `std::expected` in public headers.

5. **Minimal callable facade stubs**: The public boundary includes tiny callable APIs that prove consumer usage, result returns, and absence of global mutable state before real archive parsing exists.
   - Current: There is no callable libbsa API.
   - Target: The library exposes minimal facade functions or objects that return structured results and deliberately do not implement real archive detection, parsing, extraction, or writing yet.
   - Acceptance: Tests call the facade from a consumer-style translation unit, verify deterministic success or explicit unsupported/not-implemented errors, and do not rely on singleton or process-wide mutable state.

6. **Catch2/CTest harness with labels**: Maintainers can run a labeled test suite through CTest.
   - Current: No `tests/` directory or test framework integration exists.
   - Target: Catch2 is integrated through vcpkg and CMake, with CTest labels prepared for `unit`, `fixture`, `roundtrip`, `compat`, `malformed`, `slow`, and `requires-game-fixture` test classes.
   - Acceptance: `ctest` runs at least one `unit` test for the public boundary, and `ctest -L unit` selects the expected tests.

7. **Legal fixture layout**: The repository defines where legal tiny fixtures and optional local game fixtures belong without mutating `TES5Edit/` or committing copyrighted archives.
   - Current: There is no fixture directory, fixture README, or fixture policy in the code tree.
   - Target: The test layout includes a committed fixture location and policy for small legal generated fixtures, plus a separate ignored/local location or label for game-derived fixtures that must not be committed.
   - Acceptance: Fixture documentation states that `TES5Edit/` is not a fixture workspace, copyrighted game archives are not committed, and tests requiring local game data are separable by label.

8. **CI build/test validation**: Maintainers have automated validation for the Phase 1 build and tests.
   - Current: No CI workflow exists for this repository.
   - Target: CI configures, builds, and runs the available tests for the C++20 library foundation using CMake, vcpkg, Catch2, and CTest.
   - Acceptance: The CI workflow file exists and runs configure, build, and test steps without staging, formatting, editing, or compiling `TES5Edit/` contents.

## Boundaries

**In scope:**
- CMake/vcpkg project foundation for a reusable C++20 `libbsa` library target.
- Static and shared build selection with stable public headers.
- `include/libbsa/` public header shell with libbsa-owned result/error and minimal facade types.
- Minimal callable facade stubs that prove structured result returns without real archive behavior.
- Internal source tree placeholder needed to build the library target.
- Catch2/CTest integration with at least one public-boundary unit test and the required label taxonomy.
- Legal fixture directory and policy for generated fixtures and local-only game fixture separation.
- CI workflow for configure, build, and test validation.

**Out of scope:**
- Real archive detection, parsing, listing, extraction, or writing - those begin in later format phases after the foundation exists.
- Binary I/O helpers, archive path normalization, hash services, and compression adapters - those are Phase 2 scope.
- DDS analysis or DirectXTex-backed texture behavior - BA2 DDS phases own that work.
- Compatibility comparisons against BSArchPro output - later compatibility and format phases require actual parser behavior first.
- GUI or CLI application surfaces - the product is a reusable library, not an app.
- Mutating, formatting, compiling, vendoring, staging, or using `TES5Edit/` as a fixture workspace - it is read-only reference material.
- Public ABI stability guarantees beyond source-compatible public headers - long-term ABI policy is deferred to v2.

## Constraints

- The public library surface must remain C++20-compatible and must not expose C++23-only library types such as `std::expected`.
- Public headers must not leak `libdeflate`, `lz4`, `DirectXTex`, platform SDK, or `TES5Edit/` headers.
- Runtime dependencies must be acquired through vcpkg manifest mode and linked privately when possible.
- CMake should use a modern minimum compatible with the project stack guidance (`3.24+`).
- `TES5Edit/` must remain read-only and must not be edited, formatted, compiled into libbsa, staged, or used as mutable test data.
- Tests must be runnable through CTest and organized so future fixture, compatibility, malformed, slow, and local-game-data tests can be selected independently.
- No additional external dependencies are introduced beyond the project-approved CMake, vcpkg, Catch2, libdeflate, lz4, and DirectXTex stack.

## Acceptance Criteria

- [ ] Root build configuration exists and can configure/build a C++20 `libbsa` target through CMake and vcpkg.
- [ ] Static and shared builds both succeed without changing or duplicating public headers.
- [ ] A consumer-style translation unit can include public libbsa headers without public include paths for `libdeflate`, `lz4`, `DirectXTex`, Windows SDK, or `TES5Edit/`.
- [ ] Public result/error tests compile in C++20 mode and do not expose `std::expected`.
- [ ] Minimal facade tests call public APIs and receive structured results without singleton or global mutable state requirements.
- [ ] `ctest` runs the initial test suite, and `ctest -L unit` selects the public-boundary unit tests.
- [ ] Fixture policy and directories distinguish committed legal fixtures from ignored/local game fixture data and prohibit using `TES5Edit/` as a fixture workspace.
- [ ] CI workflow configures, builds, and runs tests without modifying, compiling, or staging `TES5Edit/` contents.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.90  | 0.75  | met    | Primary deliverable locked as skeleton plus public boundary. |
| Boundary Clarity    | 0.82  | 0.70  | met    | Real archive behavior and Phase 2 primitives explicitly excluded. |
| Constraint Clarity  | 0.82  | 0.65  | met    | C++20, dependency leakage, vcpkg, CTest labels, and TES5Edit boundary are explicit. |
| Acceptance Criteria | 0.82  | 0.70  | met    | Eight pass/fail criteria cover the foundation requirements. |
| **Ambiguity**       | 0.15  | <=0.20| met    | Gate passed after round 1. |

Status: met = meets minimum, below = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Since no build or library skeleton exists, what counts as Phase 1's primary deliverable? | Buildable C++20 library skeleton plus public API boundary, dependency manifest, and test harness. |
| 1 | Researcher | What is the smallest callable public API proof before archive parsing exists? | Minimal callable facade stubs returning structured results, without real archive parsing. |
| 1 | Researcher | What should the initial test harness prove? | Build/API tests covering result/error behavior, fixture layout, and public dependency boundary. |
| 1 | Gate | Ambiguity reached 0.15; proceed to SPEC.md? | User chose to write SPEC.md. |

---

*Phase: 01-foundation-api-boundary-and-test-harness*
*Spec created: 2026-05-07*
*Next step: /gsd-discuss-phase 1 - implementation decisions (how to build what's specified above)*
