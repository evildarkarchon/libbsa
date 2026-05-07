# Phase 1: Foundation, API Boundary, and Test Harness - Context

**Gathered:** 2026-05-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 1 delivers the project foundation for libbsa: a reusable C++20 library package that can be built, included, tested, and evolved without UI/tooling coupling, dependency leakage, global state assumptions, or `TES5Edit/` mutation. It establishes the public boundary and validation harness only; archive parsing, compression implementation, and format-specific behavior start in later phases.

</domain>

<decisions>
## Implementation Decisions

### Build and Package Boundary
- **D-01:** Use a minimal reusable C++20 library scaffold as the Phase 1 target, not a CLI, GUI, or format parser milestone.
- **D-02:** Establish CMake and vcpkg manifest mode early, including static/shared library build options and CTest wiring, so downstream phases add implementation under stable targets.
- **D-03:** Keep installed public headers under `include/libbsa/` and implementation details under `src/`; do not shape the API around application-specific tooling.

### Public API and Error Model
- **D-04:** Introduce small libbsa-owned public API shells for result/error handling, archive source/sink abstractions, format enums, metadata value types, and future open/create entry points.
- **D-05:** Do not expose public `std::expected` while the project is C++20; prefer a local `libbsa::result<T>` or explicit error-code style.
- **D-06:** Reserve exceptions for programmer precondition violations; I/O, parse, format, and compatibility failures should flow through structured result/error values.
- **D-07:** Avoid global mutable state and singleton-based behavior from the first public API shape.

### Dependency and Reference Boundaries
- **D-08:** Keep libdeflate, official lz4, and DirectXTex behind internal adapters; Phase 1 may declare/package dependencies, but public headers must not expose their types.
- **D-09:** Treat `TES5Edit/` as read-only behavioral reference material only. Do not edit, format, stage, compile, vendor, or include TES5Edit code in libbsa targets.
- **D-10:** Preserve clear boundaries for future modules: binary I/O, archive virtual paths, hashes, compression adapters, DDS analysis, and format families should be separable from the public facade.

### Fixture and CI Test Harness
- **D-11:** Create Catch2/CTest infrastructure with labels for unit, fixture, round-trip, compatibility, malformed, slow, and game-fixture-dependent tests.
- **D-12:** Establish fixture directories and policy for tiny legal handcrafted/generated archives; do not use `TES5Edit/` as a mutable test fixture location.
- **D-13:** Add CI-ready build/test commands or presets in Phase 1 even if full CI provider configuration remains minimal.
- **D-14:** Make the test harness ready for downstream evidence: each later parser/writer phase should be able to add focused compatibility fixtures without reorganizing the project.

### Claude's Discretion
Claude may choose exact file names, target names, and CMake helper organization as long as the public/private boundaries, C++20 constraint, dependency isolation, and fixture policy above are preserved.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project Scope and Requirements
- `.planning/PROJECT.md` - Project context, core value, constraints, and key decisions.
- `.planning/REQUIREMENTS.md` - Phase 1 requirement IDs and traceability.
- `.planning/ROADMAP.md` - Phase 1 goal, dependencies, requirements, and success criteria.
- `.planning/STATE.md` - Current project state and phase focus.

### Research Guidance
- `.planning/research/SUMMARY.md` - Recommended stack, architecture approach, phase implications, pitfalls, and confidence gaps.
- `.planning/research/STACK.md` - Stack details, version policies, dependency guidance, and anti-recommendations.
- `.planning/research/ARCHITECTURE.md` - Public/internal boundary guidance and component layout.
- `.planning/research/PITFALLS.md` - Compatibility, binary parsing, path, compression, and fixture risks.

### Source PRD and Project Instructions
- `docs/PRD.md` - Original product requirements and archive-format milestone intent.
- `AGENTS.md` - Repository instructions, TES5Edit boundary, dependency policy, comments/documentation rules, and GSD workflow guidance.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- No implementation source files exist yet. Phase 1 should create the initial reusable library layout rather than integrate with existing code.

### Established Patterns
- Planning artifacts are established under `.planning/`; no code conventions beyond `AGENTS.md` are implemented yet.
- The repository already uses `TES5Edit/` as a read-only reference submodule and must keep implementation outside that tree.

### Integration Points
- New code should connect to the repository root through CMake/vcpkg/Catch2/CTest files and project directories such as `include/`, `src/`, and `tests/`.

</code_context>

<specifics>
## Specific Ideas

No user-added specifics during auto discussion. Use standard C++20 library foundation practices constrained by the PRD, research summary, and `AGENTS.md`.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 1-Foundation, API Boundary, and Test Harness*
*Context gathered: 2026-05-07*
