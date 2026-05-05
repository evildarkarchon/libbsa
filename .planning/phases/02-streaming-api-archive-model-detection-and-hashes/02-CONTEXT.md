# Phase 02: streaming-api-archive-model-detection-and-hashes - Context

**Gathered:** 2026-05-05
**Status:** Ready for planning

<domain>
## Phase Boundary

This phase delivers the reusable streaming, metadata, detection, path normalization, and hash-vector foundation for later archive readers. It defines the public contracts and executable compatibility primitives, but it does not implement real family-specific table parsing, payload extraction, decompression, DDS reconstruction, writers, CLI, GUI, or fixture archive corpus decisions.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `02-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `02-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public random-access source contract for bounded reads.
- Public streaming sink contract and small in-memory helper layered over it.
- Public archive identity, summary, entry metadata, compression-state, and lookup-view types.
- Bounded header detection for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DDS identities.
- Archive-path normalization independent of host filesystem path rules.
- Bethesda-compatible hash primitives and committed golden-vector tests traced from reference behavior.
- Metadata-only listing, existence checks, and entry metadata lookup for constructed archive views.
- CTest unit/golden-vector coverage and documentation for public header and TES5Edit boundaries.

**Out of scope (from SPEC.md):**
- Full TES3, TES4-family BSA, BA2 GNRL, or BA2 DDS table parsing - later read phases own family-specific parsing.
- Payload extraction from real archives - this phase defines sink contracts only.
- Decompression through libdeflate, LZ4 frame, or raw LZ4 block APIs - Phase 3 owns codec routing.
- DDS header reconstruction, mip layout, or DirectXTex integration - BA2 DDS phases own texture behavior.
- Writer APIs, archive finalization, deduplication, or read-after-write tests - writer phases own these.
- Fixture archive corpus decisions - Phase 2 uses golden vectors and synthetic metadata tests, not full fixture archives.
- CLI, GUI, filesystem extraction policy, network access, or global singleton configuration - these remain outside the reusable library core.
- Editing, staging, formatting, compiling, or vendoring `TES5Edit/` - it remains read-only reference material.

</spec_lock>

<decisions>
## Implementation Decisions

### Public API Shape
- **D-01:** Model the random-access byte source as a public abstract base class. It should expose bounded size/read-at behavior and allow consumers to adapt memory, files, or future mmap-like sources without libbsa taking ownership.
- **D-02:** Use caller-owned references for source and sink lifetime during operations. Metadata archive views should own copied metadata, not retain borrowed source/sink lifetimes.
- **D-03:** Expose public `memory_source` and `memory_sink` helpers for small payloads and tests. Do not add a public file source in Phase 2; host file I/O policy can wait.
- **D-04:** Provide both boolean existence checks and result-returning metadata lookup. `contains(path)`-style APIs may return bool, while `entry(path)`-style APIs should return `result<entry_metadata>` with structured failures for missing or invalid paths.

### Detection Policy
- **D-05:** If detection recognizes a supported magic value but finds an unsupported version, return a structured `unsupported_format` failure rather than reporting the input as unknown.
- **D-06:** Truncated headers must fail immediately with a structured malformed-archive result. Do not return best-effort family identity when required detection fields are missing.
- **D-07:** Successful detection should return a header summary: identity plus bounded header fields that are safely available, such as version, subtype, flags, counts, offsets, and compression markers when present. It must not imply full table parsing.
- **D-08:** BA2 GNRL vs DDS detection should be decided from explicit subtype/header fields only. Do not infer BA2 subtype from filenames or host path extensions.

### Path and Hash Surface
- **D-09:** Archive path normalization is public API. Consumers should be able to create or normalize archive paths before lookup.
- **D-10:** Represent public archive paths as libbsa-owned normalized UTF-8 string values, not `std::filesystem::path` and not lifetime-sensitive string views.
- **D-11:** Keep exact hash functions internal or test-visible in Phase 2. Public API should expose normalization and lookup behavior, while golden-vector tests lock compatibility hashes.
- **D-12:** Invalid archive path input should be reported through `result<archive_path>` from normalization. Do not silently normalize nonsensical paths and do not throw for data-shaped invalid path input.

### Golden Vectors
- **D-13:** Store Phase 2 golden vectors in dedicated test source/data under `tests/`, not only in planning documents.
- **D-14:** Document vector provenance with inline test comments near each vector group, citing the exact TES5Edit/BSArchPro reference files/functions or traced behavior. Do not create a separate compatibility-notes file unless research finds non-obvious behavior that needs more context.
- **D-15:** Use a representative minimal vector set: a few normal and edge-case cases per family/hash path, expanded later when real parser fixtures arrive.
- **D-16:** If reference tracing finds uncertain or contradictory hash behavior, stop and document the blocker instead of guessing or adding placeholder vectors.

### Claude's Discretion
No selected area was left to Claude's discretion. The planner may choose exact type/function/file names as long as the decisions above, `02-SPEC.md`, and project conventions are satisfied.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Locked Phase Scope
- `.planning/phases/02-streaming-api-archive-model-detection-and-hashes/02-SPEC.md` - Locked Phase 2 requirements, boundaries, constraints, acceptance criteria, and interview decisions.
- `.planning/ROADMAP.md` - Phase ordering, Phase 2 goal, dependencies, success criteria, and requirement mapping.
- `.planning/REQUIREMENTS.md` - v1 requirement IDs and traceability, especially `BIO-01` through `BIO-05` and `DPH-01` through `DPH-05`.
- `.planning/PROJECT.md` - Project purpose, core value, public API constraints, streaming-first requirement, and TES5Edit reference context.

### Prior Phase Context
- `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md` - Carry-forward decisions for `libbsa::result`, public/private layout, explicit CMake sources, test labels, and TES5Edit boundary.
- `.planning/phases/01-build-error-and-test-foundation/01-VERIFICATION.md` - Verified current foundation state, public-header boundary, and local VS 2026 fallback caveat.

### Research and Product Context
- `.planning/research/STACK.md` - C++20/CMake/vcpkg dependency guidance and public API dependency-leakage constraints.
- `.planning/research/SUMMARY.md` - Research synthesis and phase ordering implications.
- `docs/PRD.md` - Original product scope, supported archive families, compatibility expectations, testing strategy, and risks.
- `AGENTS.md` - Repository instructions, TES5Edit read-only boundary, dependency rules, comment policy, and validation expectations.

### Read-Only Reference Areas
- `TES5Edit/BSArchPro.dpr` - Behavioral reference entry point for BSArchPro compatibility tracing.
- `TES5Edit/BSArch/` - Reference area for archive behavior; read-only.
- `TES5Edit/Core/wbBSArchive.pas` - Reference area for archive behavior; read-only.
- `TES5Edit/Core/wbBSA.pas` - Reference area for BSA/path/hash behavior; read-only.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `include/libbsa/result.hpp`: Public C++20 result/error API to use for detection, normalization, source read, and lookup failures.
- `tests/foundation_tests.cpp`: Catch2 test style and `unit` label precedent for focused behavior tests.
- `tests/public_header_smoke.cpp`: Public-header smoke pattern to extend or mirror for new public headers.
- `CMakeLists.txt`: Explicit target/source/test wiring pattern; no recursive source globbing.
- `README.md`: Existing build/test/TES5Edit-boundary documentation to preserve and extend only as needed.

### Established Patterns
- Public headers live under `include/libbsa/`; private implementation belongs under `src/`.
- Public headers should include only standard/library-owned types and must not expose dependency, platform, Delphi, UI, DirectXTex, or TES5Edit implementation types.
- CTest currently has `unit` and `smoke` labels. Phase 2 can add a focused golden-vector label if useful, while preserving full/unit/smoke execution.
- The committed Windows preset targets Visual Studio 17 2022, but this machine has been verifying with a local Visual Studio 18 2026 fallback without changing the committed preset.

### Integration Points
- New public APIs should integrate through additional headers under `include/libbsa/` and explicit `target_sources(... FILE_SET public_headers ...)` entries.
- New source files should be listed explicitly under `src/` in `CMakeLists.txt`.
- New tests should be wired into existing `BUILD_TESTING`/Catch2/CTest setup.
- Golden vectors should live under `tests/` as committed source/data used by executable tests.

</code_context>

<specifics>
## Specific Ideas

- Random-access byte source should be a public abstract base class with caller-owned lifetime.
- Metadata archive views should own copied metadata rather than borrowing archive byte sources.
- Phase 2 should expose memory source/sink helpers but not a public file source.
- Detection should be strict for truncated required fields and informative for recognized-but-unsupported versions.
- BA2 subtype detection must use bytes/header subtype fields, not filename extensions.
- Public path API should expose normalized UTF-8 archive path values and result-returning normalization.
- Hashes can remain internal/test-visible as long as golden-vector tests lock behavior.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 02-streaming-api-archive-model-detection-and-hashes*
*Context gathered: 2026-05-05*
