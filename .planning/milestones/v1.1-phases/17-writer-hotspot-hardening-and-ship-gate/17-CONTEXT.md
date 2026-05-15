# Phase 17: writer-hotspot-hardening-and-ship-gate - Context

**Gathered:** 2026-05-14
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 17 hardens the remaining writer hotspot risks without broad redesign: TES4-family BSA and BA2 GNRL dedupe get semantics-preserving candidate narrowing, BA2 DX10 snapshot temporary data gets explicit normal and ordinary-failure cleanup behavior, and v1.1 closes only after ship-ready verification evidence is recorded.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**6 requirements are locked.** See `17-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `17-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- TES4-family BSA writer dedupe candidate narrowing for `deduplicate_payloads = true`.
- BA2 GNRL writer dedupe staged identity or digest hardening for `deduplicate_payloads = true`.
- Preservation of exact final stored-byte equality as the authority for every shared dedupe offset.
- BA2 DX10 snapshot temp cleanup on successful `write_to` completion.
- BA2 DX10 snapshot temp cleanup on ordinary `result`-returning failure unwinding.
- Documentation or planning evidence for BA2 DX10 temp lifecycle guarantees and residual abnormal-termination risk.
- Focused runtime and/or source-policy tests that prove the writer-hotspot requirements and prevent regression to the known fragile patterns.
- v1.1 ship-gate evidence and consistent planning-state updates after successful execution.

**Out of scope (from SPEC.md):**
- Public writer API expansion or a new writer architecture - v1.1 is hardening-only and existing consumers must keep the same public surface.
- Changing opt-in dedupe defaults - dedupe remains disabled by default for TES4-family BSA, BA2 GNRL, and BA2 DX10 writers.
- Sharing payload offsets without exact stored-byte equality fallback - this would violate existing compatibility semantics.
- Full performance program or broad benchmarking suite - only the named dedupe hotspot narrowing is required.
- BA2 DX10 DDS feature expansion, new texture formats, new archive families, or new compression formats - Phase 17 hardens existing supported behavior only.
- Crash-proof or hard-termination temp cleanup guarantees - document residual risk unless the implementation and tests truly provide stronger behavior.
- Modifying, formatting, staging, or compiling `TES5Edit/` - the submodule remains read-only reference material.
- Cross-platform portability work - libbsa remains Windows-only.

</spec_lock>

<decisions>
## Implementation Decisions

### BA2 DX10 writer lifecycle
- **D-01:** BA2 DX10 `write_to` is terminal after any ordinary `write_to` attempt, whether it succeeds or returns a `result` error.
- **D-02:** After a BA2 DX10 writer is consumed, later `add_file` and `write_to` calls return `invalid_argument` through the existing `result` contract. They must not throw, silently no-op, or partially reuse stale staged entries.
- **D-03:** The consumed-writer rule is specific to BA2 DX10 because that writer owns snapshot temp data. Phase 17 must not standardize one-shot behavior across TES3, TES4-family BSA, or BA2 GNRL writers.
- **D-04:** Cleanup takes priority over retryability for BA2 DX10. Downstream planning should not preserve retry-after-failure behavior by retaining snapshot temp data or long-lived duplicate DDS byte storage.

### Dedupe proof and guardrails
- **D-05:** Dedupe narrowing proof should combine runtime regression tests with source/policy guardrails. Runtime tests prove archive behavior; policy tests prevent drift back to unkeyed or all-prior-payload scans.
- **D-06:** Performance evidence should be algorithmic and non-flaky: prove that exact comparisons are bounded behind keyed, identity, or digest candidate narrowing rather than adding timing-sensitive benchmarks.
- **D-07:** TES4 and BA2 GNRL dedupe narrowing should stay in format-local private helpers near their existing layout code unless planning finds a very small shared internal helper that does not become a generic dedupe framework.
- **D-08:** Hashes, digests, sizes, or staged identity values are candidate filters only. Exact final stored-byte equality remains mandatory before any shared payload offset is assigned.

### BA2 DX10 cleanup cases
- **D-09:** Cleanup tests must explicitly cover all named ordinary failure families: validation failure, missing or truncated snapshot data before publish, and output or publish failure.
- **D-10:** If cleanup itself fails during ordinary failure unwinding, the public result should preserve the primary validation, write, or publish error. Cleanup remains best-effort unless a later requirement changes the public contract.
- **D-11:** Phase 17 should also clean up BA2 DX10 temp directories created by failed `add_file` calls when `add_file` reserved a snapshot directory before failing.
- **D-12:** Cleanup tests should identify only new writer-owned `libbsa-dx10-snapshot-*` directories created during the test and assert those are removed. Tests must not require the entire system temp root to be free of unrelated directories.

### Ship gate evidence
- **D-13:** Phase 17 should close with a focused combined gate: focused Debug writer/runtime tests, focused MSVC AddressSanitizer hardening tests for the risky writer paths, and Release package proof for install/export and downstream consumer behavior.
- **D-14:** Optional local game-corpus or BSArchPro comparison tests remain advisory and are not required for the official v1.1 ship gate. The closure gate must be runnable from committed assets.
- **D-15:** BA2 DX10 temp lifecycle guarantees and residual abnormal-termination risk must be documented in public or maintainer-facing docs as well as Phase 17 planning/verification artifacts.
- **D-16:** If Phase 17 passes, updates to `.planning/REQUIREMENTS.md`, `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`, and verification evidence belong in the same closure plan so milestone status does not drift.

### the agent's Discretion
None. The user selected concrete decisions for writer lifecycle, dedupe proof style, cleanup coverage, documentation surface, and ship-gate evidence. Downstream agents should not reopen those choices.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and locked requirements
- `.planning/phases/17-writer-hotspot-hardening-and-ship-gate/17-SPEC.md` - locked Phase 17 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/PROJECT.md` - v1.1 hardening boundary, no-public-API-expansion rule, Windows-only scope, dependency constraints, and accumulated project decisions.
- `.planning/REQUIREMENTS.md` - `DEDU-01`, `DEDU-02`, `DX10-01`, and `DX10-02` traceability for Phase 17.
- `.planning/ROADMAP.md` - Phase 17 goal, dependency on Phase 16, success criteria, and v1.1 closeout position.
- `.planning/STATE.md` - current milestone state, pending Phase 17 blockers, and accumulated Phase 13-16 decisions.

### Carry-forward phase context
- `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md` - BA2 DX10 snapshot-builder and chunk-assembler seams, parser/preparer source-policy style, and explicit deferral of dedupe/temp-lifecycle work to Phase 17.
- `.planning/phases/15-reader-backend-dispatch-cleanup/15-CONTEXT.md` - negative-invariant source-policy pattern and public facade stability expectations to preserve.
- `.planning/phases/14-verification-lane-truthfulness/14-CONTEXT.md` - supported Debug, Release package-proof, and MSVC AddressSanitizer lane roles for the Phase 17 ship gate.

### Codebase maps
- `.planning/codebase/ARCHITECTURE.md` - writer pipeline shape, public API boundary, publish helper pattern, shared detail services, and writer state ownership model.
- `.planning/codebase/TESTING.md` - Catch2/CTest conventions, fixture policy, policy-test style, label taxonomy, and package-consumer smoke patterns.
- `.planning/codebase/CONCERNS.md` - TES4 dedupe quadratic concern, BA2 GNRL repeated disk-read concern, BA2 DX10 temp lifecycle risk, and recommended mitigation directions.

### Writer implementation files
- `include/libbsa/writer.hpp` - public writer API surface that must remain stable; BA2 DX10 lifecycle documentation may need updates here or in adjacent public docs.
- `src/formats/bsa/tes4_bsa_layout.cpp` - current TES4 dedupe linear candidate scan and `tes4_stored_payloads_equal` exact equality fallback.
- `src/formats/ba2/ba2_gnrl_layout.cpp` - current BA2 GNRL size/hash bucket narrowing, disk-backed equality fallback, and disk-source size-change rejection.
- `src/formats/ba2/ba2_dx10_writer.cpp` - BA2 DX10 writer state, snapshot directory ownership, current destructor cleanup, and top-level `write_to` pipeline.
- `src/formats/ba2/ba2_dx10_snapshot_builder.cpp` - snapshot directory reservation and per-subresource snapshot file creation.
- `src/formats/ba2/ba2_dx10_chunk_assembler.cpp` - snapshot-backed chunk assembly and pre-publish missing/truncated snapshot failure behavior.
- `src/formats/ba2/ba2_dx10_layout.cpp` - BA2 DX10 chunk payload dedupe behavior to preserve while Phase 17 focuses on temp lifecycle and TES4/BA2 GNRL dedupe.
- `src/detail/writer_publish.cpp` - shared writer temp-output publish and cleanup behavior that BA2 DX10 failure cleanup must interoperate with.

### Test and policy files
- `tests/unit/tes4_bsa_writer_tests.cpp` - existing TES4 dedupe, compression mismatch, embedded-name mismatch, disk-backed dedupe, output failure, and extraction regression coverage.
- `tests/unit/ba2_gnrl_writer_tests.cpp` - existing BA2 GNRL dedupe, raw-vs-compressed separation, disk-source size-change rejection, and output failure coverage.
- `tests/unit/ba2_dx10_writer_tests.cpp` - existing BA2 DX10 snapshot immutability, teardown cleanup, compression routing, dedupe, publish, and writer behavior tests to extend.
- `tests/unit/ba2_dx10_preparer_seam_tests.cpp` - Phase 16 focused seam tests for snapshot-backed chunk assembly and missing/truncated snapshot pre-publish failure.
- `tests/unit/bounded_memory_policy_tests.cpp` - existing policy coverage for BA2 DX10 hardened snapshot directory reservation and writer-source IO patterns.
- `tests/unit/parser_preparer_seam_policy_tests.cpp` - role-based source-policy style from Phase 16 that Phase 17 can mirror for writer hotspot guardrails.
- `tests/unit/validation_policy_tests.cpp` - verification-matrix and planning/docs policy-test pattern from Phase 14.
- `tests/CMakeLists.txt` - Catch2/CTest registration and package-consumer smoke wiring.

### External specs
- No external specs - requirements are fully captured in repository planning artifacts and codebase maps above.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `tes4_stored_payloads_equal` in `src/formats/bsa/tes4_bsa_layout.cpp`: keep this exact stored-byte equality authority and add narrowing before it rather than replacing it with hash-only sharing.
- `ba2_gnrl_payloads_equal` in `src/formats/ba2/ba2_gnrl_layout.cpp`: keep this exact fallback and disk-source growth/truncation rejection while improving candidate identity/digest selection.
- `ba2_dx10_ensure_snapshot_directory` and `ba2_dx10_build_writer_entry_snapshot` in `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`: current snapshot ownership seam to extend for add-time failure cleanup and consumed-state behavior.
- `ba2_dx10_writer::state` in `src/formats/ba2/ba2_dx10_writer.cpp`: natural home for consumed-state tracking and explicit snapshot cleanup before destructor safety-net cleanup.
- `detail::publish_writer_output` in `src/detail/writer_publish.cpp`: existing temp-output publish and cleanup helper that should remain the output publication boundary.
- Existing tests in `tes4_bsa_writer_tests.cpp`, `ba2_gnrl_writer_tests.cpp`, and `ba2_dx10_writer_tests.cpp`: reuse current output-offset and extraction assertions rather than creating a separate performance benchmark harness.

### Established Patterns
- Public writer APIs keep stable `result<T>` error reporting. New consumed-state behavior should return `invalid_argument` through `result`, not throw.
- Writers follow validate, prepare, layout, serialize-to-temp, publish. Phase 17 should insert cleanup and narrowing at the relevant internal stage without public API expansion.
- Source/policy tests are acceptable for structural invariants, but runtime tests must still prove archive bytes, offsets, extraction, and error behavior.
- Phase 14 established that risky writer changes should use the MSVC AddressSanitizer hardening lane before closure.
- Fixture and local-corpus policy remains committed-assets-first; `requires-game-fixture` checks stay opt-in and advisory.

### Integration Points
- TES4 dedupe changes integrate in or adjacent to `src/formats/bsa/tes4_bsa_layout.cpp` and must preserve `tes4_assign_offsets` behavior for existing writer tests.
- BA2 GNRL dedupe changes integrate in or adjacent to `src/formats/ba2/ba2_gnrl_layout.cpp` and must preserve raw/compressed routing and disk-source validation.
- BA2 DX10 lifecycle changes integrate across `ba2_dx10_writer.cpp`, `ba2_dx10_snapshot_builder.cpp`, and cleanup-aware tests in `ba2_dx10_writer_tests.cpp`.
- Public docs or maintainer docs must record temp lifecycle guarantees and abnormal-termination risk, with verification tying those docs to implementation behavior.
- Phase completion must update planning surfaces consistently when `DEDU-01`, `DEDU-02`, `DX10-01`, and `DX10-02` are verified.

</code_context>

<specifics>
## Specific Ideas

- Treat BA2 DX10 `write_to` as a consuming operation after any ordinary attempt; this is a deliberate lifecycle tradeoff to make cleanup predictable.
- Prefer source/policy guardrails that prove candidate narrowing exists and exact equality fallback remains mandatory without freezing every helper name.
- Prefer tests that track newly created writer-owned snapshot directories rather than asserting global temp-root cleanliness.
- The final verification gate should be role-based: focused Debug behavior, focused ASan hardening, and Release package proof each cover a distinct risk.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within Phase 17 scope.

</deferred>

---

*Phase: 17-writer-hotspot-hardening-and-ship-gate*
*Context gathered: 2026-05-14*
