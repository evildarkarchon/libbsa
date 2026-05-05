# Phase 1: Build, Error, and Test Foundation - Context

**Gathered:** 2026-05-05
**Status:** Ready for planning

<domain>
## Phase Boundary

This phase delivers the implementation foundation only: a buildable C++20 libbsa skeleton, vcpkg dependency plumbing, Catch2/CTest execution, a local structured result/error API, public/private header separation, and visible safeguards that keep `TES5Edit/` read-only and out of the build.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**6 requirements are locked.** See `01-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `01-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Root CMake project and build presets or documented configure path for a C++20 library.
- `vcpkg.json` and any companion vcpkg configuration needed for dependency resolution.
- Initial `include/`, `src/`, and `tests/` layout for the library.
- Public foundational result/error API compatible with C++20.
- Catch2 and CTest integration with passing tests for linkability and result/error behavior.
- Documentation or comments that preserve the `TES5Edit/` read-only boundary.

**Out of scope (from SPEC.md):**
- Archive binary reader/writer primitives - Phase 2 owns streaming API, archive model, detection, and hash foundations.
- Archive format detection or parsing - Phase 2 and later read phases own real archive behavior.
- TES3, TES4-family, BA2 GNRL, or BA2 DDS extraction - later read phases own format-specific functionality.
- Compression adapter implementation - Phase 3 owns deflate, LZ4 frame, and LZ4 block behavior.
- DDS analysis or DirectXTex wrapper implementation - BA2 DDS phases own texture behavior.
- Writer planning or archive emission - writer phases own archive creation.
- Productized CLI or GUI tooling - the project scope is a reusable library.
- Any modification to `TES5Edit/` - it is read-only reference material.

</spec_lock>

<decisions>
## Implementation Decisions

### Build Shape
- **D-01:** Provide static/shared build options in Phase 1 rather than a single fixed library target. The planner should expose CMake options that let consumers choose static or shared output while keeping the foundation minimal.
- **D-02:** Add basic install support for the library and public headers, but do not require full generated CMake package config/version files unless the planner finds it trivial and low-risk.
- **D-03:** Add a primary Windows vcpkg CMake preset because Windows is the primary target. Linux/macOS presets can wait until portability validation becomes active.
- **D-04:** Root CMake should find all Phase 1 declared dependencies: `libdeflate`, `lz4`, `DirectXTex`, and `Catch2`. Runtime dependency implementation can remain private/minimal, but dependency resolution must be proven now.

### Result API
- **D-05:** Implement a local `libbsa::result<T>` class as the C++20-compatible result surface. Do not expose C++23 `std::expected` in public headers.
- **D-06:** Support `libbsa::result<void>` for operations that can fail but do not return a value.
- **D-07:** Phase 1 errors should carry an error category/code plus a short message. Rich parser context such as offsets, archive paths, nested causes, or source locations can wait until parser phases require it.
- **D-08:** Initial tests should lock result/error construction, `has_value`-style state checks, value access, error access, and representative error propagation/access behavior. Monadic helpers such as `and_then`/`transform` are not required in Phase 1.

### Test Layout
- **D-09:** Use one initial Catch2 test executable for foundation tests rather than many domain-specific executables before domains exist.
- **D-10:** Register Catch2 tests with CTest through Catch2's CMake discovery helper when available.
- **D-11:** Use `unit` and `smoke` labels in Phase 1. Reserve `fixture`, `roundtrip`, `compat`, and `slow` labels for later phases when those test types exist.
- **D-12:** Add a compile-only public-header isolation test that includes the foundational public header from a consumer translation unit without private dependency headers.

### Reference Boundary and Header Layout
- **D-13:** Document the `TES5Edit/` read-only boundary in `README.md` and add a CMake comment near explicit source lists so implementers see the rule where build sources are maintained.
- **D-14:** Use explicit CMake source lists. Do not use recursive source globbing that could accidentally pull in files from `TES5Edit/`.
- **D-15:** Do not create an empty compatibility-notes file in Phase 1. Add compatibility notes when a later phase actually traces non-obvious reference behavior.
- **D-16:** Keep public headers under `include/libbsa/` and private implementation headers under `src/`. Avoid `include/libbsa/detail` for Phase 1 internals unless a later public inline/template need forces it.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may still choose exact file names and CMake option names as long as the decisions above and SPEC acceptance criteria are satisfied.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Locked Phase Scope
- `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md` - Locked Phase 1 requirements, boundaries, constraints, and acceptance criteria.
- `.planning/ROADMAP.md` - Phase ordering, Phase 1 goal, and requirement mapping.
- `.planning/REQUIREMENTS.md` - v1 requirement IDs and traceability, especially `FND-01` through `FND-05` and `VAL-01`.
- `.planning/PROJECT.md` - Project purpose, core value, constraints, and key decisions.

### Research and Product Context
- `.planning/research/STACK.md` - C++20/CMake/vcpkg/Catch2 dependency guidance, target names, version policy, and what not to use.
- `.planning/research/SUMMARY.md` - Research synthesis and roadmap implications for Phase 1 foundations.
- `docs/PRD.md` - Original product requirements, supported archive scope, dependency policy, testing strategy, risks, and success metrics.
- `AGENTS.md` - Repository instructions, TES5Edit read-only boundary, dependency rules, and documentation/comment policy.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `.planning/research/STACK.md`: Provides the most concrete build/dependency guidance for CMake, vcpkg, Catch2, and public/private dependency boundaries.
- `.planning/phases/01-build-error-and-test-foundation/01-SPEC.md`: Provides the locked requirements and pass/fail checks for Phase 1.
- `AGENTS.md`: Provides implementation constraints that must be reflected in comments/docs and build boundaries.

### Established Patterns
- No implementation patterns exist yet. There is no `CMakeLists.txt`, `vcpkg.json`, `include/`, `src/`, `tests/`, or C++ source to preserve.
- Planning artifacts establish a correctness-first, dependency-minimal, C++20 library direction.
- `TES5Edit/` is a read-only submodule and must remain outside source lists, formatting, staging, and generated changes.

### Integration Points
- New implementation connects at repository root through `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json`, `include/libbsa/`, `src/`, `tests/`, and `README.md`.
- The initial public API should be limited to the foundational result/error surface and any minimal version/link smoke surface needed for tests.

</code_context>

<specifics>
## Specific Ideas

- Prefer CMake options for static/shared output in Phase 1.
- Basic install support is enough; full package config can be deferred unless effectively free.
- Windows/MSVC vcpkg preset is the primary preset for now.
- Public-header isolation should be verified through a compile-only consumer-style test.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 1-Build, Error, and Test Foundation*
*Context gathered: 2026-05-05*
