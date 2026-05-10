# Phase 02: Binary I/O, Paths, Hashes, and Compression Services - Context

**Gathered:** 2026-05-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 2 turns the Phase 1 public API skeleton into a tested internal primitive layer for later archive parser and writer phases. It delivers checked little-endian binary helpers, archive virtual path normalization, bounded payload streaming primitives, explicit compression routing, private libdeflate/LZ4 adapters, and Bethesda hash functions without implementing real archive detection, listing, extraction, writing, DDS behavior, or public tool surfaces.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `02-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `02-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Internal checked little-endian binary reader/writer helpers for fixed-width archive fields and bounded byte ranges.
- Archive virtual path normalization utilities needed by future lookup and hash behavior.
- Minimal payload source/sink or equivalent streaming contract for bounded chunk transfer.
- Private libdeflate adapter for raw deflate compression and exact-size decompression.
- Private LZ4 frame adapter for Skyrim SE/AE BSA frame payloads.
- Private raw LZ4 block adapter for Starfield BA2 v3 block payloads.
- Explicit compression routing model covering deflate, LZ4 frame, and raw LZ4 block.
- TES3, TES4-family, and FO4/BA2 hash functions with expected-value tests.
- Focused unit and generated-fixture tests for primitives, malformed inputs, exact-size validation, routing, paths, and hashes.

**Out of scope (from SPEC.md):**
- Real archive detection, listing, metadata inspection, or extraction through `archive_reader` - Phase 3 and later format phases own archive-level behavior.
- Parsing complete BSA or BA2 headers and records - this phase builds shared primitives, not format parsers.
- Creating complete BSA or BA2 archives - write-new phases own archive serialization and round-trip validation.
- Extracting from real Bethesda game archives - Phase 2 proof is unit vectors and generated fixtures, not game archive extraction.
- BA2 DDS metadata analysis or DDS reconstruction - DDS phases own DirectXTex-backed texture behavior.
- Multi-threaded compression, decompression, packing, or extraction - performance and concurrency are later scope after correctness is established.
- Public CLI, GUI, logging framework, or external app tooling - libbsa remains a reusable library.
- New external dependencies beyond the approved vcpkg stack of libdeflate, official lz4, DirectXTex, and Catch2 - no additional dependency need is established for this phase.
- Mutating, formatting, compiling, staging, vendoring, or using `TES5Edit/` as a fixture workspace - it remains read-only reference material.

</spec_lock>

<decisions>
## Implementation Decisions

### Public Surface Boundaries
- **D-01:** Keep Phase 2 public surface minimal. Binary I/O, hash, codec, routing, path, and streaming primitives should default to internal implementation details unless a later public parser API proves a concrete need.
- **D-02:** If Phase 2 needs an archive path type for implementation or tests, keep it in an internal/detail namespace for now. Do not commit to a public `path.hpp` or consumer-facing path type in Phase 2.
- **D-03:** Do not change `archive_reader::open` or other Phase 1 public API signatures in Phase 2. Preserve the minimal read/open facade until Phase 3 implements real reader behavior.
- **D-04:** Dependency-linked services must be private adapters only. Link and exercise libdeflate/lz4 internally without exposing dependency types or public codec utility functions.

### Path Normalization Rules
- **D-05:** Canonical archive path keys should fold to lowercase.
- **D-06:** Treat `/` and `\\` as equivalent input separators and store canonical keys with forward slashes for stable cross-platform tests and future lookup/hash behavior.
- **D-07:** Reject obvious invalid archive virtual paths in Phase 2: empty paths, absolute or drive-rooted paths, traversal/dot segments, and empty components. Defer rare character/encoding quirks to format-specific phases if reference behavior requires nuance.
- **D-08:** Phase 2 path utilities only need canonical keys. Original/display spelling should be preserved later by archive metadata records when real name tables are parsed.

### Streaming Contract Shape
- **D-09:** Keep the streaming contract internal in Phase 2. Public extraction/packing APIs should wait for format phases that know the real consumer workflow.
- **D-10:** Streaming tests should prioritize exact byte preservation and structured propagation of source/sink failures.
- **D-11:** Phase 2 streaming is synchronous only. Do not add async, threading, or cancellation models in this phase.
- **D-12:** Chunk size is an implementation detail. Tests should prove transfer happens through multiple bounded chunks, but should not lock a project-wide chunk-size constant unless needed by implementation.
- **D-13:** Prefer simple internal object interfaces for bounded sources and sinks over callback-only or span-only helpers.
- **D-14:** Treat partial sink acceptance of a requested chunk as a structured error rather than allowing ambiguous partial-success semantics.
- **D-15:** Sources for archive payload movement should know the expected bounded payload size so transfer stops exactly at archive metadata limits.
- **D-16:** Use memory-backed sources/sinks and test doubles in Phase 2. File-backed archive sources and real extraction sinks can arrive with parser/extraction phases.

### Proof And Reference Standard
- **D-17:** Hash algorithms should be proven with reference-traced constants. Researcher/planner should trace TES5Edit/BSArchPro hash behavior, then tests should assert known TES3, TES4-family, and FO4/BA2 constants.
- **D-18:** Compression tests should emphasize small legal round-trip vectors plus corrupt/truncated input rejection and exact-size mismatch rejection for each codec route.
- **D-19:** TES5Edit reference use must stay read-only, but non-obvious compatibility rules should be documented near implementation/tests with references to the relevant TES5Edit files/functions.
- **D-20:** Use unit vectors first for Phase 2 primitive proof. Add committed generated binary fixtures only when a primitive cannot be clearly proven with source literals or computed buffers.

### Agent Discretion
- Planner may choose exact internal file names, helper class names, namespace layout, and test file split if the decisions above, `02-SPEC.md`, and the public-header dependency boundary are satisfied.
- Researcher/planner should decide the exact additional `error_code` values only if existing Phase 1 codes are insufficient; tests should assert stable codes, not exact diagnostic strings.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-SPEC.md` — Locked Phase 2 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` — Phase 2 goal, mapped requirements BIN-01 through BIN-08, success criteria, and downstream phase dependencies.
- `.planning/REQUIREMENTS.md` — Shared binary/compression requirements and traceability for BIN-01 through BIN-08.

### Project Constraints
- `.planning/PROJECT.md` — Project purpose, dependency policy, public API constraints, error model, streaming goal, and TES5Edit boundary.
- `AGENTS.md` — Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` — Product goals, supported archive families, compression split, streaming goals, and reference compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/01-foundation-api-boundary-and-test-harness/01-CONTEXT.md` — Public facade/result/error decisions, fixture policy, dependency boundary, and test label taxonomy carried into Phase 2.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` — Read-only behavioral reference for archive/hash behavior; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` — Read-only behavioral reference for BSA behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` — Read-only BSArchPro application reference for behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` — Read-only reference directory for BSArchPro-related behavior; do not edit or use as fixture workspace.

### Current Codebase State
- `include/libbsa/result.hpp` — Existing public `libbsa::result`, `libbsa::error`, and `libbsa::error_code` boundary that Phase 2 should reuse.
- `include/libbsa/archive.hpp` — Existing minimal `archive_reader::open` public facade that Phase 2 should not revise unless a concrete blocker appears.
- `src/archive.cpp` — Current unsupported open stub; real archive behavior remains later scope.
- `CMakeLists.txt` — Library target, public header file set, install/export setup, and current source list.
- `tests/CMakeLists.txt` — Catch2/CTest test target and label discovery pattern to extend for Phase 2 unit tests.
- `tests/fixtures/README.md` — Fixture provenance and local-game-data policy for any generated fixture additions.
- `vcpkg.json` — Approved dependencies already include libdeflate, lz4, DirectXTex, and Catch2.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `libbsa::result<T>`, `libbsa::result<void>`, `libbsa::error`, and `libbsa::error_code` already exist and should carry Phase 2 primitive failures.
- `archive_reader::open(std::string_view)` exists as a minimal public facade and should remain unchanged in Phase 2.
- The CMake target already installs public headers via a file set and links tests against `libbsa::libbsa`.
- Catch2/CTest is already wired with `catch_discover_tests(... ADD_TAGS_AS_LABELS)` and a `libbsa_tests` executable.
- `vcpkg.json` already declares libdeflate with compression/decompression features, official `lz4`, DirectXTex, and Catch2.

### Established Patterns
- Public headers currently live under `include/libbsa/`; internal code lives under `src/`. Phase 2 should keep dependency-bearing adapters out of installed headers.
- Public API documentation uses Doxygen-style `///` comments and concise explanations of current/future contracts.
- Tests assert stable error codes rather than exact diagnostic strings.
- Fixture policy separates committed generated fixtures from ignored local game-derived data and forbids using `TES5Edit/` as a fixture workspace.
- CI/test policy expects `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` to remain a useful quick validation path.

### Integration Points
- Add new internal source files to `target_sources(libbsa ...)` without adding dependency headers to the public header file set.
- Add Phase 2 unit tests to `tests/CMakeLists.txt` and tag them with existing labels such as `unit`, `malformed`, and `fixture` only when applicable.
- If generated fixtures become necessary, place them under `tests/fixtures/generated` and document provenance/recipe per `tests/fixtures/README.md`.
- Compression adapters should link through the existing vcpkg dependencies from private implementation targets or private library linkage.

</code_context>

<specifics>
## Specific Ideas

- The user wants Phase 2 to stay conservative about public API commitments: internal primitives first, public extraction/path/stream surfaces later when archive phases prove the consumer shape.
- Canonical archive path keys should be lowercase and forward-slash separated. They are archive keys, not host filesystem paths.
- Streaming should be an internal, synchronous, bounded source/sink transfer model using memory/test doubles first.
- Hash work should be compatibility-led: trace TES5Edit/BSArchPro behavior, document non-obvious rules, and lock tests to known constants.
- Codec work should guard against silent corruption by proving exact-size validation, corrupt/truncated rejection, and separate LZ4 frame vs raw-block routing.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 02-Binary I/O, Paths, Hashes, and Compression Services*
*Context gathered: 2026-05-08*
