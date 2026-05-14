# Phase 16: parser-and-preparer-seam-extraction - Context

**Gathered:** 2026-05-14
**Status:** Ready for planning

## Phase Boundary

Phase 16 is an internal, behavior-preserving maintainability refactor. It extracts smaller private seams from the targeted TES4-family BSA parser hotspot and the BA2 DX10 writer preparer/staging hotspot while keeping the public API, supported archive behavior, existing fixture semantics, Phase 13 host-file boundary, and Phase 15 reader facade/backend separation intact.

## Requirements (locked via SPEC.md)

**5 requirements are locked.** See `16-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `16-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Internal seam extraction for the targeted TES4-family BSA parser hotspot in `src/formats/bsa/tes4_bsa_parser.cpp` or adjacent private/internal format files.
- Internal seam extraction for the targeted BA2 DX10 writer preparer/staging hotspot in `src/formats/ba2/ba2_dx10_prepare.cpp` or adjacent private/internal format files.
- Focused runtime regression coverage for TES4 parser behavior that is already supported by current generated fixtures and malformed cases.
- Focused runtime regression coverage for BA2 DX10 preparation behavior, including snapshot-backed staging and planned chunk preparation.
- Source/policy guardrails or equivalent committed evidence that the targeted hotspot responsibilities remain split into smaller maintainable seams.
- Preservation of current public reader/writer APIs, current result/error-code behavior, and current fixture-backed parser/preparer behavior.

**Out of scope (from SPEC.md):**
- Public API redesign or new public reader/writer types - v1.1 is hardening-only and this phase changes internal seams, not consumer surface area.
- Full TES4 parser rewrite or broad generic parser framework - the phase targets smaller maintainable seams, not a new architecture for all parsers.
- Full BA2 writer redesign or replacement of the DX10 writer pipeline - the phase targets preparer/staging seams, not a writer architecture reset.
- TES4 or BA2 GNRL dedupe optimization - this belongs to Phase 17 writer hotspot hardening.
- BA2 DX10 temporary-data lifecycle cleanup proof or abnormal-termination risk documentation - this belongs to Phase 17 ship-gate scope, although Phase 16 may create seams that make that later work safer.
- New archive family support, new compression formats, or new DDS feature support - v1.1 does not expand product capability.
- Editing, formatting, compiling, staging, or committing files under `TES5Edit/` - the submodule remains read-only reference material.
- Cross-platform portability work - libbsa remains Windows-only.

## Implementation Decisions

### TES4 Parser Seam Shape
- **D-01:** Split the TES4 parser hotspot into adjacent private/internal format modules rather than only rearranging file-local helpers inside `tes4_bsa_parser.cpp`.
- **D-02:** The table seam should return a raw table bundle: checked header/table state, folder records or folder blocks, and file-name strings. It should not produce partially normalized entry candidates or final `entry_metadata` values.
- **D-03:** A dedicated payload descriptor helper should own embedded-name prefix sizing, raw-size calculation, compression interpretation, and payload-span validation from table records plus the payload reader.
- **D-04:** Focused TES4 seam tests may include narrow private/internal headers under `src/formats/bsa` so they can target table bundles and payload descriptors directly. This must not add or change public headers under `include/libbsa/`.

### BA2 DX10 Preparer Seams
- **D-05:** Extract a private snapshot-builder seam for DDS host-file load, DirectXTex-backed analysis handoff, target format validation, and subresource snapshot-file writing. `ba2_dx10_make_writer_entry` should become a narrower coordinator around that seam.
- **D-06:** Use a plan-then-assemble shape for chunk preparation: entry preparation plans chunks first, then a separate chunk assembler/compressor consumes snapshot handles and `texture::planned_texture_chunk` descriptors.
- **D-07:** Chunk assembly should preserve the current bounded-memory behavior by streaming staged snapshot bytes through host-file chunk helpers and size checks, not by loading every snapshot file fully into memory.
- **D-08:** Preserve current indexed parallel chunk work: `detail::run_indexed_work` remains the chunk worker mechanism, results are stored by planned-chunk index, and prepared entries are sorted by canonical archive path only after entry preparation.

### Regression Proof Focus
- **D-09:** Regression proof should be balanced across both locked hotspots. Phase 16 must not over-prove TES4 while treating BA2 DX10 as a smoke test, or vice versa.
- **D-10:** TES4 proof should directly cover the new table and payload seams: table sizing, folder/file-name offset handling, name/hash validation, duplicate canonical path rejection, embedded-name prefix sizing, raw-size calculation, and payload-span-over-metadata rejection for covered cases.
- **D-11:** BA2 DX10 proof should directly cover snapshot immutability, streamed snapshot-backed chunk assembly, multi-mip/array/cubemap chunk ordering, and Fallout 4 versus Starfield v3 compression routing for covered cases.
- **D-12:** Verification should include focused unit labels plus the affected existing public suites, and should use the Phase 14 MSVC AddressSanitizer hardening lane before closing this risky parser/writer refactor.

### Structural Guardrails
- **D-13:** Source/policy guardrails should be role-based. They should assert separated responsibilities and forbidden collapse patterns without freezing exact helper names or exact module filenames beyond the fact that dedicated private seams exist.
- **D-14:** Add a dedicated parser/preparer seam policy test under `tests/unit/` rather than expanding unrelated validation-policy tests.
- **D-15:** TES4 seam collapse means `parse_tes4_bsa_archive_impl` again directly owns table parsing, entry materialization, payload descriptor logic, duplicate canonical path rejection, and hash validation as one broad coordinator.
- **D-16:** BA2 DX10 seam collapse means `ba2_dx10_prepare.cpp` again directly owns snapshot creation, subresource assembly, chunk planning, compression routing, and entry sorting in one mixed flow without reviewable private seams.

### the agent's Discretion
None. The discussion locked seam shape, ownership split, proof balance, verification expectation, and structural guardrail style closely enough that downstream research and planning should not reopen them.

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope and Locked Requirements
- `.planning/PROJECT.md` - v1.1 hardening boundaries, Windows-only scope, dependency constraints, and no-public-API-expansion rule.
- `.planning/REQUIREMENTS.md` - `REFA-01` and `REFA-02` requirement traceability for Phase 16.
- `.planning/ROADMAP.md` - Phase 16 goal, dependency on Phase 15, and success criteria.
- `.planning/STATE.md` - current milestone position, accumulated decisions, and current parser/preparer hotspot focus.
- `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md` - locked requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.

### Carry-Forward Context
- `.planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md` - resolved host-file path boundary and diagnostics-only raw UTF-8 host-path rule that Phase 16 must preserve.
- `.planning/phases/14-verification-lane-truthfulness/14-CONTEXT.md` - supported verification matrix, ASan hardening lane role, and policy-test layering expectations.
- `.planning/phases/15-reader-backend-dispatch-cleanup/15-CONTEXT.md` - public reader facade/backend separation and method-scoped source-policy guard style to preserve.

### Codebase Maps
- `.planning/codebase/ARCHITECTURE.md` - layered public facade, format pipeline, shared detail, and texture adapter architecture; identifies parser/preparer monolith anti-patterns.
- `.planning/codebase/TESTING.md` - Catch2, fixture, malformed, writer-stage, and policy-test conventions to reuse.
- `.planning/codebase/CONCERNS.md` - monolithic parser/preparer concern, BA2 DX10 staging fragility, and safe-modification guidance.

### TES4 Parser Files
- `src/formats/bsa/tes4_bsa_parser.cpp` - current hotspot containing header/table parsing, table validation, entry materialization, embedded-name/raw-size logic, and payload-span checks.
- `src/formats/bsa/tes4_bsa_parser.hpp` - current internal parser entrypoints that must remain stable: `parse_tes4_bsa_archive`, `parse_tes4_bsa_archive_file`, and `parse_tes4_bsa_metadata`.
- `src/formats/bsa/tes4_bsa_constants.hpp` - version, record-size, and flag constants used by extracted TES4 parser seams.
- `tests/unit/tes4_bsa_reader_tests.cpp` - existing success, metadata, malformed, hash mismatch, duplicate canonical path, embedded-name, and extraction coverage to preserve and extend.

### BA2 DX10 Preparer Files
- `src/formats/ba2/ba2_dx10_prepare.cpp` - current hotspot containing DDS source loading, snapshot staging, target validation, chunk assembly, compression routing, and prepared-entry construction.
- `src/formats/ba2/ba2_dx10_prepare.hpp` - current internal preparer surface that must remain stable: `ba2_dx10_make_writer_entry`, `ba2_dx10_validate_entries`, `ba2_dx10_prepare_chunk`, and `ba2_dx10_prepare_entries`.
- `src/formats/ba2/ba2_dx10_writer.cpp` - top-level writer flow that owns state and calls the preparer pipeline.
- `src/formats/ba2/ba2_dx10_layout.cpp` and `src/formats/ba2/ba2_dx10_layout.hpp` - payload offset assignment and dedupe-related layout behavior that Phase 16 should not claim as Phase 17 work.
- `src/texture/dds_layout.cpp` and `src/texture/dds_layout.hpp` - current DDS layout and chunk planning helpers used by BA2 DX10 preparation.
- `src/texture/directxtex_analyzer.cpp` - DirectXTex-backed DDS analysis boundary that must stay internal.
- `tests/unit/writer_stage_tests.cpp` - existing direct internal stage coverage for BA2 DX10 chunk preparation, multi-mip ordering, truncated snapshot failure before publish, cubemap handling, and layout behavior.
- `tests/unit/ba2_dx10_writer_tests.cpp` - existing public writer/add-file/snapshot/round-trip/compression/structural coverage to preserve and extend.

### Shared Helpers and Policy Patterns
- `src/detail/host_file.hpp` and `src/detail/host_file.cpp` - shared host-file helpers that snapshot reading and parser file opens must continue to use where applicable.
- `src/detail/parallel_work.cpp` - indexed worker execution behavior that BA2 DX10 chunk preparation must preserve.
- `tests/unit/archive_reader_dispatch_policy_tests.cpp` - recent role-based source-policy pattern that forbids public reader dispatch collapse without freezing every implementation detail.
- `tests/unit/validation_policy_tests.cpp` - repo-reading policy-test utilities and matrix truthfulness examples; use as pattern input, but do not keep expanding this file for Phase 16.

### External Specs
- No external specs - requirements are fully captured in repository planning artifacts and codebase maps above.

## Existing Code Insights

### Reusable Assets
- `tes4_bsa_parser.cpp` already contains separable helpers such as `read_header`, `metadata_table_size`, `read_folder_records`, `read_folder_blocks`, `read_file_names`, `materialize_entries`, `embedded_prefix_size`, and `raw_size_for`; Phase 16 should turn the relevant responsibilities into private modules instead of inventing a generic parser framework.
- `ba2_dx10_prepare.cpp` already has candidate seams around `read_dds_file`, snapshot directory/file helpers, `collect_chunk_snapshots`, `ba2_dx10_make_writer_entry`, `ba2_dx10_prepare_chunk`, and `prepare_entry`; Phase 16 should make those seams explicit and testable.
- `tests/unit/writer_stage_tests.cpp` already includes direct internal BA2 DX10 preparation tests and can guide the new focused seam tests.
- `tests/unit/tes4_bsa_reader_tests.cpp` already has legal fixture and mutation patterns for TES4 table, hash, duplicate path, metadata, embedded-name, payload-span, and extraction cases.
- `tests/unit/archive_reader_dispatch_policy_tests.cpp` provides a recent source-reading policy-test style that is negative-invariant based rather than exact-helper-name based.

### Established Patterns
- Public APIs stay stable and C++20-compatible; internal helper headers under `src/formats/...` are acceptable for format-stage tests when they do not leak through `include/libbsa/`.
- Fallible parser and writer-preparer operations return `libbsa::result<T>` with stable `error_code` values; parser/preparer seam extraction must preserve current public result/error behavior.
- Fixture-backed tests should use committed generated archives and source DDS files; do not use `TES5Edit/` as a mutable fixture source.
- Source/policy tests should guard structural invariants and drift-prone repository facts, while runtime tests prove behavior.
- Risky parser/writer changes should use the Phase 14-supported ASan hardening lane before closure.

### Integration Points
- TES4 parser extraction should integrate under `src/formats/bsa/`, keep `tes4_bsa_parser.hpp` entrypoints intact, and update build/test registration for any new private modules.
- BA2 DX10 extraction should integrate under `src/formats/ba2/`, keep `ba2_dx10_prepare.hpp` function contracts intact, and keep DirectXTex details in the existing texture adapter layer.
- Tests should extend `tests/unit/` with focused runtime coverage and a dedicated parser/preparer seam policy suite.
- CMake/test registration must include any new source files and tests without changing package-consumer or public export surfaces.

## Specific Ideas

- Use the terms `raw table bundle`, `payload descriptor helper`, `snapshot builder`, and `plan then assemble` as conceptual anchors. Exact helper and module names remain implementation details unless the planner has a strong reason to lock them.
- Treat `ba2_dx10_prepare_chunk` as an internal contract worth preserving or narrowing, but do not replace the current indexed worker behavior with a new scheduler abstraction.
- Keep BA2 DX10 snapshot lifecycle cleanup and abnormal-termination risk documentation explicitly deferred to Phase 17, even if the new snapshot builder makes that later work easier.

## Deferred Ideas

None - discussion stayed within Phase 16 scope.

---

*Phase: 16-parser-and-preparer-seam-extraction*
*Context gathered: 2026-05-14*
