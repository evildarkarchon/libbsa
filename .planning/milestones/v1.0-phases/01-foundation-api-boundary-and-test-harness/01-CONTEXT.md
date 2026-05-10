# Phase 01: Foundation, API Boundary, and Test Harness - Context

**Gathered:** 2026-05-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 1 turns libbsa from a planning-only repository into a buildable C++20 library foundation. It must establish the public API boundary, build/test harness, fixture policy, and CI validation without implementing real archive detection, parsing, extraction, writing, binary I/O services, hashing, compression adapters, DDS handling, or any mutable use of `TES5Edit/`.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `01-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `01-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- CMake/vcpkg project foundation for a reusable C++20 `libbsa` library target.
- Static and shared build selection with stable public headers.
- `include/libbsa/` public header shell with libbsa-owned result/error and minimal facade types.
- Minimal callable facade stubs that prove structured result returns without real archive behavior.
- Internal source tree placeholder needed to build the library target.
- Catch2/CTest integration with at least one public-boundary unit test and the required label taxonomy.
- Legal fixture directory and policy for generated fixtures and local-only game fixture separation.
- CI workflow for configure, build, and test validation.

**Out of scope (from SPEC.md):**
- Real archive detection, parsing, listing, extraction, or writing - those begin in later format phases after the foundation exists.
- Binary I/O helpers, archive path normalization, hash services, and compression adapters - those are Phase 2 scope.
- DDS analysis or DirectXTex-backed texture behavior - BA2 DDS phases own that work.
- Compatibility comparisons against BSArchPro output - later compatibility and format phases require actual parser behavior first.
- GUI or CLI application surfaces - the product is a reusable library, not an app.
- Mutating, formatting, compiling, vendoring, staging, or using `TES5Edit/` as a fixture workspace - it is read-only reference material.
- Public ABI stability guarantees beyond source-compatible public headers - long-term ABI policy is deferred to v2.

</spec_lock>

<decisions>
## Implementation Decisions

### Public Facade Shape
- **D-01:** Expose a tiny future-shaped read/open facade centered on `libbsa::archive_reader`.
- **D-02:** Phase 1 should include read/open only. Do not add writer shells until write phases need them.
- **D-03:** The initial open stub should accept host path text via a string-view-style API and return an explicit unsupported/not-implemented result. Do not design stream abstractions in Phase 1.
- **D-04:** Provide an umbrella header such as `libbsa/libbsa.hpp` plus focused headers such as `archive.hpp` and `result.hpp`.
- **D-05:** Opening should be explicit, with an open/factory call returning `result<archive_reader>`. Constructors should not perform fallible work.
- **D-06:** Do not invent fake archive metadata in Phase 1. If open is unsupported, return a structured error rather than placeholder type/version/count values.
- **D-07:** Public API doc comments should document the Phase 1 stub contract and future intent, not full parser behavior.
- **D-08:** Use `libbsa` for public declarations and `libbsa::detail` for private implementation helpers when needed. Do not introduce a versioned namespace in Phase 1.
- **D-09:** Include install/export support now and prove the reusable boundary with a minimal installed-package consumer smoke test.
- **D-10:** Treat public API naming as a soft lock until Phase 2: preserve the chosen shape unless Phase 2 reveals a concrete issue.

### Result/Error Ergonomics
- **D-11:** Public result/error types should remain libbsa-owned. Downstream code should plan around `libbsa::result<T>`, `libbsa::result<void>`, `libbsa::error`, and `libbsa::error_code`.
- **D-12:** `libbsa::error` should contain a stable error code plus diagnostic-only human-readable message text. Tests should assert codes, not exact messages.
- **D-13:** Start with a small future-safe `error_code` enum: include at least unsupported/not-implemented, invalid argument, I/O error, and format error categories. Do not define a full parser/compression taxonomy yet.
- **D-14:** The public `result<T>` surface should be expected-like: `has_value`, `operator bool`, `value`, and `error` are expected. Rich monadic helpers are not required in Phase 1.
- **D-15:** `value()` on an error result may throw a programmer-error exception. I/O and format failures still travel through result values.
- **D-16:** Mark result-returning public APIs `noexcept` where practical, but do not overpromise when allocation or diagnostic string construction can throw.
- **D-17:** Boost.Outcome may be investigated as a way to avoid homegrown result bugs, despite the normal no-new-dependency policy. It is viable only if it builds cleanly through vcpkg and can be hidden behind libbsa-owned public result/error types. If Boost would leak into public headers or API, fall back to a local `libbsa::result<T>` implementation.
- **D-18:** Use a plain `libbsa::error_code` enum first. Do not integrate with `std::error_code` in Phase 1.

### Build/Test Presets
- **D-19:** The primary local developer preset should target MSVC Debug static builds.
- **D-20:** Define explicit static and shared configure presets rather than relying on manual `BUILD_SHARED_LIBS` flags.
- **D-21:** Use descriptive preset names such as `windows-msvc-debug-static` and `windows-msvc-debug-shared`.
- **D-22:** Enable high compiler warnings, but do not make warnings errors in Phase 1.
- **D-23:** CI should validate Windows/MSVC static and shared configure/build/test flows.
- **D-24:** Add a minimal install/package consumer smoke test to prove exported headers and CMake package consumption.
- **D-25:** Use Catch2 tags as CTest labels via `catch_discover_tests(... ADD_TAGS_AS_LABELS)` so `ctest -L unit` and future labels work naturally.
- **D-26:** For Phase 1, list vcpkg dependencies directly rather than introducing manifest features for tests. If Boost.Outcome clears D-17, the planner must document the dependency-policy exception.

### Fixture Layout Policy
- **D-27:** Committed legal fixtures should live under `tests/fixtures/generated`.
- **D-28:** Reserve separate generated fixture locations for source inputs and archive outputs, e.g. `tests/fixtures/generated/source` and `tests/fixtures/generated/archives`.
- **D-29:** Local-only game-derived fixtures should live under ignored `tests/fixtures/local`, with optional discovery through `LIBBSA_GAME_FIXTURES` for data stored outside the repo.
- **D-30:** Phase 1 should add fixture README/policy documentation, not a detailed manifest format yet.
- **D-31:** Tests requiring local game archives should be discovered but skipped by default when data is absent. Skip messages should include setup hints for `tests/fixtures/local` and/or `LIBBSA_GAME_FIXTURES`.
- **D-32:** The fixture README should require each committed generated fixture to document its generator/source recipe, legal provenance, and behavior it proves.
- **D-33:** Fixture docs must explicitly prohibit using `TES5Edit/` as a fixture workspace or source of committed fixture files.
- **D-34:** Phase 1 should establish README placeholders/taxonomy for future labels: `unit`, `fixture`, `roundtrip`, `compat`, `malformed`, `slow`, and `requires-game-fixture`.
- **D-35:** Tiny legal generated binary fixtures may be committed later when documented by provenance/recipe.

### Agent Discretion
- Planner may choose exact file names and CMake target organization if the choices satisfy the decisions above and the locked SPEC.
- Researcher/planner should investigate the smallest clean Boost.Outcome integration only for D-17; if it complicates the public boundary, prefer local result implementation.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/01-foundation-api-boundary-and-test-harness/01-SPEC.md` — Locked Phase 1 requirements, boundaries, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` — Phase 1 goal, dependencies, mapped requirements, and success criteria.
- `.planning/REQUIREMENTS.md` — Foundation requirements FND-01 through FND-07 and DOC-04 traceability.

### Project Constraints
- `.planning/PROJECT.md` — Project purpose, constraints, key decisions, dependency policy, public API direction, and TES5Edit boundary.
- `AGENTS.md` — Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` — Product goals, supported archive families, dependency list, and milestone context.

### Current Codebase State
- `.clangd` — Existing C++20 compile flag and `include` path assumption.
- `.gitignore` — Existing Visual Studio/build-output ignore baseline; planner should add ignored local fixture paths if needed.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- No existing CMake, library, source, test, fixture, or CI assets exist yet. Phase 1 is creating the foundation from scratch.

### Established Patterns
- `.clangd` already assumes C++20 and `include/`, matching the intended public header layout.
- The repo uses `.planning/` workflow artifacts and a read-only `TES5Edit/` submodule boundary.
- `.gitignore` already contains broad Visual Studio/build-output ignores, but does not yet define local fixture ignores.

### Integration Points
- New foundation files should be created outside `TES5Edit/`, primarily under root CMake/vcpkg/preset files, `include/libbsa/`, `src/`, `tests/`, `tests/fixtures/`, and `.github/workflows/`.

</code_context>

<specifics>
## Specific Ideas

- User explicitly challenged the no-new-dependency constraint for Boost.Outcome to avoid homegrown result bugs, then resolved the public API boundary as Boost-free. Treat Boost.Outcome as a conditional research/implementation option, not a public API decision.
- User wants local game fixture tests discovered but skipped by default, rather than fully excluded from CTest discovery.
- User wants C++20 modules deferred explicitly rather than implemented in Phase 1.

</specifics>

<deferred>
## Deferred Ideas

- C++20 modules are explicitly deferred; Phase 1 should use conventional public headers.

</deferred>

---

*Phase: 01-Foundation, API Boundary, and Test Harness*
*Context gathered: 2026-05-07*
