# Phase 16: parser-and-preparer-seam-extraction - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-14
**Phase:** 16-parser-and-preparer-seam-extraction
**Areas discussed:** TES4 seam shape, BA2 DX10 seams, Regression proof focus, Structural guardrails

---

## TES4 Seam Shape

### Where should the TES4 parser seam live?

| Option | Description | Selected |
|--------|-------------|----------|
| New private modules | Split table parsing and entry materialization into adjacent internal format modules so `tes4_bsa_parser.cpp` becomes a thin coordinator. | Yes |
| File-local helpers | Keep helpers in `tes4_bsa_parser.cpp`, but the file remains the main hotspot and direct seam tests are weaker. | |
| Hybrid split | Move only table parsing into a module and leave entry materialization in the parser file for a smaller first cut. | |
| You decide | Planner can choose the smallest approach that satisfies `16-SPEC.md` acceptance criteria. | |

**User's choice:** New private modules
**Notes:** The new seams should be private/internal and must not change public headers.

### What should the TES4 table seam return to the entry-materialization seam?

| Option | Description | Selected |
|--------|-------------|----------|
| Raw table bundle | Header, folder records/blocks, and file-name strings only; entry materialization still owns canonical paths, hashes, compression, and payload metadata. | Yes |
| Entry candidates | Return partially normalized path/hash/record candidates; reduces materializer work but blurs table parsing with semantic validation. | |
| Final entries | Table seam produces `entry_metadata` directly; simplest call chain but does not really separate table parsing from materialization. | |
| You decide | Planner can pick the cleanest split after reading the current parser types. | |

**User's choice:** Raw table bundle
**Notes:** The table seam should remain a checked table-reading/data extraction boundary.

### Who should own TES4 embedded-name and raw-size payload metadata?

| Option | Description | Selected |
|--------|-------------|----------|
| Payload descriptor helper | A dedicated internal helper computes prefix size, raw size, compression, and payload-span checks from table records plus payload reader. | Yes |
| Entry materializer | Keep payload-prefix and raw-size logic inside entry materialization; less API surface but leaves a dense compatibility hotspot. | |
| Reader/extractor | Defer prefix/raw-size interpretation to extraction helpers; risky because metadata currently exposes these values at open/list time. | |
| You decide | Planner can keep this as an implementation detail if tests prove the required invariants. | |

**User's choice:** Payload descriptor helper
**Notes:** The helper should preserve existing open-time metadata semantics.

### How directly should TES4 seam tests reach the extracted parser helpers?

| Option | Description | Selected |
|--------|-------------|----------|
| Private-header tests | Expose narrow internal helpers/structs under `src/formats/bsa` so tests can target table bundles and payload descriptors without public API changes. | Yes |
| Black-box only | Test only `archive_reader::open`, listing, and validation results; preserves encapsulation but proves seam behavior less directly. | |
| Policy only | Use source/policy checks for helper presence and rely on existing runtime tests for behavior; low runtime churn but weaker behavior proof. | |
| You decide | Planner can balance direct helper testing against minimal header churn. | |

**User's choice:** Private-header tests
**Notes:** Direct tests are allowed only against internal surfaces.

---

## BA2 DX10 Seams

### What should be the main BA2 DX10 staging seam?

| Option | Description | Selected |
|--------|-------------|----------|
| Snapshot builder | Extract DDS load/analyze/subresource snapshot writing into a private snapshot-builder module used by `ba2_dx10_make_writer_entry`. | Yes |
| Entry helpers only | Keep one module but split `ba2_dx10_make_writer_entry` into smaller helpers; lower churn but weaker hotspot reduction. | |
| Chunk-first only | Focus mostly on `prepare_entry` and `ba2_dx10_prepare_chunk`; leaves add-time snapshot staging mixed with entry creation. | |
| You decide | Planner can choose the smallest split that satisfies the SPEC.md BA2 DX10 seam requirements. | |

**User's choice:** Snapshot builder
**Notes:** Snapshot staging should become an explicit private seam.

### Where should BA2 DX10 planned-chunk preparation be separated?

| Option | Description | Selected |
|--------|-------------|----------|
| Plan then assemble | Make entry preparation plan chunks first, then call a separate chunk assembler/compressor that consumes snapshot handles and planned chunk descriptors. | Yes |
| Per-chunk only | Only keep `ba2_dx10_prepare_chunk` as the main seam; `prepare_entry` remains a broad coordinator. | |
| Move to texture layer | Push more BA2 chunk planning into `src/texture`; risky because BA2 record/compression rules belong in the format layer. | |
| You decide | Planner can decide after reading `texture::plan_dx10_chunks` and current `prepare_entry` flow. | |

**User's choice:** Plan then assemble
**Notes:** BA2-specific record and compression decisions remain in the format layer.

### How should BA2 DX10 chunk assembly read staged snapshot data?

| Option | Description | Selected |
|--------|-------------|----------|
| Stream snapshots | Preserve current bounded host-file chunk reads and size checks while moving them behind the new assembler seam. | Yes |
| Load snapshots fully | Simpler assembler logic, but weakens bounded-memory behavior for large DDS sources. | |
| Keep inline | Leave snapshot file reads inside `ba2_dx10_prepare_chunk`; least churn but keeps I/O mixed with compression and metadata construction. | |
| You decide | Planner can choose as long as source mutation isolation and chunk sizing behavior stay locked. | |

**User's choice:** Stream snapshots
**Notes:** Bounded-memory behavior remains part of the seam contract.

### How should BA2 DX10 parallel chunk preparation behave after seam extraction?

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve indexed work | Keep `detail::run_indexed_work` over planned chunks, store results by index, and sort only prepared entries by canonical path afterward. | Yes |
| Serial first | Temporarily make chunk preparation serial during refactor; simpler but risks changing worker-count behavior and losing existing coverage value. | |
| New scheduler seam | Add a broader scheduler abstraction; likely too much design for a semantics-preserving hardening phase. | |
| You decide | Planner can keep the current behavior if tests catch deterministic ordering and worker failure mapping. | |

**User's choice:** Preserve indexed work
**Notes:** The refactor should not become a scheduler redesign.

---

## Regression Proof Focus

### How should Phase 16 balance regression proof between the two hotspots?

| Option | Description | Selected |
|--------|-------------|----------|
| Balanced proof | Require focused TES4 and BA2 DX10 evidence in the same phase, because both `REFA-01` and `REFA-02` are locked requirements. | Yes |
| TES4-heavy | Prioritize parser invariants more deeply and keep BA2 DX10 to smoke/round-trip coverage; risks under-proving `REFA-02`. | |
| BA2-heavy | Prioritize snapshot/chunk behavior more deeply and keep TES4 to existing parser fixtures; risks under-proving `REFA-01`. | |
| You decide | Planner can balance based on implementation risk discovered during planning. | |

**User's choice:** Balanced proof
**Notes:** Both locked requirements need focused evidence.

### Which TES4 parser behavior should the new focused proof lock most directly?

| Option | Description | Selected |
|--------|-------------|----------|
| Tables plus payloads | Test table bundle sizing/offset/name/hash behavior and payload descriptor prefix/raw-size/span behavior, matching the chosen seams. | Yes |
| Metadata success | Focus on successful metadata materialization across v103/v104/v105 fixtures; useful but weaker against malformed boundary drift. | |
| Malformed only | Focus on count, offset, duplicate path, hash, and payload-span failures; strong hardening but less proof of normal metadata preservation. | |
| You decide | Planner can pick cases from `tes4_bsa_reader_tests.cpp` and `malformed_manifest.json`. | |

**User's choice:** Tables plus payloads
**Notes:** TES4 proof should include both normal metadata and compatibility-sensitive boundary behavior.

### Which BA2 DX10 preparer behavior should the new focused proof lock most directly?

| Option | Description | Selected |
|--------|-------------|----------|
| Snapshot plus chunks | Prove snapshot immutability, streamed snapshot assembly, mip/array/cubemap chunk ordering, and target compression routing. | Yes |
| Failure safety | Focus on missing/truncated snapshot failure before publish changes output; important, but less complete for the seam extraction goal. | |
| Round-trip only | Rely mostly on public writer round-trip extraction; good consumer proof but less direct evidence for the extracted seams. | |
| You decide | Planner can choose based on which seam changes most during implementation. | |

**User's choice:** Snapshot plus chunks
**Notes:** Failure safety remains important but is not the only BA2 DX10 proof target.

### What verification expectation should downstream planning attach to this refactor?

| Option | Description | Selected |
|--------|-------------|----------|
| Focused plus ASan | Run focused unit labels and relevant public suites, then use the Phase 14 MSVC ASan hardening lane before closing this risky parser/writer refactor. | Yes |
| Focused only | Run only the new focused tests and existing affected suites; faster, but less aligned with Phase 14 guidance for risky parser/writer changes. | |
| Full default CTest | Run the default debug CTest suite only; broad but may not exercise the ASan hardening lane now available. | |
| You decide | Planner can decide exact commands from README, `CMakePresets.json`, and Phase 14 context. | |

**User's choice:** Focused plus ASan
**Notes:** The planner should derive exact commands from the checked-in supported matrix.

---

## Structural Guardrails

### How strict should Phase 16 source/policy guardrails be?

| Option | Description | Selected |
|--------|-------------|----------|
| Role-based guards | Assert separated responsibilities and forbidden collapse patterns, but do not require exact helper or module names. | Yes |
| Exact structure | Require specific file/helper names; strongest enforcement but brittle for future refactors. | |
| Behavior tests only | Skip source/policy guardrails and rely on runtime tests; conflicts with SPEC.md's structural-evidence acceptance criterion. | |
| You decide | Planner can decide the least brittle source checks that still catch seam collapse. | |

**User's choice:** Role-based guards
**Notes:** Guard the invariant, not every implementation name.

### Where should the Phase 16 structural policy checks live?

| Option | Description | Selected |
|--------|-------------|----------|
| Dedicated policy file | Add a focused `tests/unit` parser/preparer seam policy suite instead of expanding `validation_policy_tests.cpp`. | Yes |
| Existing policy file | Put checks in `validation_policy_tests.cpp`; fewer files but makes that suite less focused. | |
| Split by hotspot | Add separate TES4 and BA2 DX10 policy files; clear, but may be more boilerplate than needed. | |
| You decide | Planner can fit the checks into the existing test organization. | |

**User's choice:** Dedicated policy file
**Notes:** Keep the policy contract discoverable.

### What should count as TES4 seam collapse for policy tests?

| Option | Description | Selected |
|--------|-------------|----------|
| Coordinator bloat | Fail if `parse_tes4_bsa_archive_impl` again owns table parsing, entry materialization, payload descriptor logic, and duplicate-path/hash checks directly. | Yes |
| Missing files only | Fail only if expected adjacent private modules disappear; easier to implement but closer to brittle file-name enforcement. | |
| No TES4 policy | Use only runtime/internal tests for TES4; weaker against the SPEC.md structural guardrail. | |
| You decide | Planner can define a clear negative invariant after choosing module boundaries. | |

**User's choice:** Coordinator bloat
**Notes:** Catch responsibility collapse rather than exact file-name changes.

### What should count as BA2 DX10 preparer seam collapse for policy tests?

| Option | Description | Selected |
|--------|-------------|----------|
| Staging plus chunk bloat | Fail if `ba2_dx10_prepare.cpp` again owns snapshot creation, subresource assembly, chunk planning, compression routing, and entry sorting in one mixed flow. | Yes |
| Missing files only | Fail only if expected snapshot/chunk module files disappear; straightforward but too tied to names. | |
| No BA2 policy | Use runtime writer-stage tests only; weaker against the SPEC.md structural guardrail. | |
| You decide | Planner can define the least brittle BA2 DX10 negative invariant after implementation. | |

**User's choice:** Staging plus chunk bloat
**Notes:** The BA2 policy guard should also avoid exact helper-name brittleness.

---

## the agent's Discretion

None.

## Deferred Ideas

None.
