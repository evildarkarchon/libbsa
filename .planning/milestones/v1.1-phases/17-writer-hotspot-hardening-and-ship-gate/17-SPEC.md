# Phase 17: Writer Hotspot Hardening and Ship Gate - Specification

**Created:** 2026-05-14
**Ambiguity score:** 0.19 (gate: <= 0.20)
**Requirements:** 6 locked

## Goal

The remaining writer hotspot risks change from expensive or teardown-dependent staging paths into semantics-preserving, test-backed hardening outcomes, and v1.1 closes with explicit ship-ready evidence.

## Background

Phase 17 is the final v1.1 hardening phase after Phase 16 split the targeted TES4 parser and BA2 DX10 preparer hotspots into private seams. The current milestone state marks Phase 17 as ready to plan and lists two active blocker concerns: dedupe optimizations must preserve exact stored-byte equality semantics, and BA2 DX10 temp-data cleanup must be honest about any residual abnormal-termination risk.

Current TES4-family BSA dedupe behavior is implemented in `src/formats/bsa/tes4_bsa_layout.cpp`. When `tes4_bsa_writer_options::deduplicate_payloads` is true, layout assignment currently walks previously assigned payloads and calls `tes4_stored_payloads_equal` to prove exact final stored-byte equality. Existing tests in `tests/unit/tes4_bsa_writer_tests.cpp` prove that opt-in identical stored bytes share offsets, dedupe is disabled by default, disk-backed raw payloads can dedupe, and entries with different compression encoding or embedded-name prefixes do not dedupe.

Current BA2 GNRL dedupe behavior is implemented in `src/formats/ba2/ba2_gnrl_layout.cpp`. It groups candidates by stored size and `payload_hash`, then falls back to exact stored-payload equality across memory and disk-backed entries. Existing tests in `tests/unit/ba2_gnrl_writer_tests.cpp` prove default distinct offsets, opt-in shared offsets, raw-vs-compressed separation, and disk-source size-change rejection.

Current BA2 DX10 writer snapshot staging is created during `ba2_dx10_writer::add_file` through `ba2_dx10_ensure_snapshot_directory` and `ba2_dx10_build_writer_entry_snapshot`. Snapshot files live under a random `libbsa-dx10-snapshot-*` temp directory and are removed best-effort by `ba2_dx10_writer::state::~state`. Existing tests prove add-time source immutability and teardown cleanup, but the current cleanup boundary waits for writer teardown rather than proving cleanup after normal write completion and ordinary failure unwinding.

## Requirements

1. **TES4 dedupe narrowing**: TES4-family BSA dedupe uses faster candidate narrowing before exact stored-byte comparison while preserving opt-in final stored-byte equality semantics.
   - Current: `tes4_assign_offsets` keeps a linear list of prior assigned payloads and can compare each new dedupe-enabled entry against every previous owned payload until an exact stored-byte match is found or all candidates are rejected.
   - Target: With `deduplicate_payloads = true`, TES4 layout considers only plausible candidates selected by cheap final-stored-payload identity facts before invoking exact equality; byte-identical final stored payloads still share offsets, and any differing stored bytes still receive distinct offsets.
   - Acceptance: Focused tests or policy evidence fail if TES4 dedupe falls back to an unkeyed all-prior-payload scan, while existing TES4 writer tests still prove shared offsets for identical stored bytes, distinct offsets for compression/embedded-name mismatches, disk-backed dedupe behavior, and byte-correct extraction.

2. **BA2 GNRL dedupe hardening**: BA2 GNRL dedupe uses stronger staged identity or digest narrowing while preserving exact stored-byte equality fallback behavior.
   - Current: `ba2_gnrl_assign_payload_offsets` groups candidates by stored size and `payload_hash`, then calls `ba2_gnrl_payloads_equal`; disk-backed fallback comparisons are bespoke to this layout path and must continue rejecting growth or truncation before publishing malformed offsets.
   - Target: With `deduplicate_payloads = true`, BA2 GNRL candidate selection is backed by explicit staged identity or digest evidence for the final stored bytes, and every shared offset is still justified by exact equality across memory and disk-backed payload sources.
   - Acceptance: Focused tests or policy evidence fail if BA2 GNRL dedupe can share offsets from identity/digest alone without exact equality fallback, if disk-backed source growth/truncation is no longer rejected, or if raw and compressed source twins share offsets despite different final stored bytes.

3. **BA2 DX10 normal cleanup**: BA2 DX10 snapshot temporary data is cleaned up as part of normal successful write completion, without requiring the caller to destroy the writer object first.
   - Current: `ba2_dx10_writer::state::~state` removes `state_->snapshot_dir` best-effort, and `tests/unit/ba2_dx10_writer_tests.cpp` only proves the teardown-owned cleanup path after the writer object leaves scope.
   - Target: After a successful `ba2_dx10_writer::write_to` call returns, snapshot temp directories and files created for the staged DDS data no longer remain under the system temp location as live `libbsa-dx10-snapshot-*` artifacts attributable to that writer.
   - Acceptance: A committed test records snapshot temp directories before add/write, writes a BA2 DX10 archive successfully, verifies the output can be opened and extracted, and verifies that no new writer-owned `libbsa-dx10-snapshot-*` directory remains after `write_to` returns while the writer object is still alive.

4. **BA2 DX10 failure cleanup**: BA2 DX10 snapshot temporary data is cleaned up during ordinary failure unwinding for expected `result`-returning write failures.
   - Current: Missing or truncated snapshot files can fail before publish-visible output changes, and writer-publish temporary output directories have their own cleanup helper, but Phase 16 did not prove snapshot temp cleanup on ordinary BA2 DX10 write failure paths.
   - Target: For expected write failures that return through `result<void>` instead of process termination, BA2 DX10 cleanup removes writer-owned snapshot temp data and preserves the primary error result.
   - Acceptance: Committed tests cover at least one pre-publish BA2 DX10 failure and one publish/output-path failure or validation failure, verify the failure is reported through the existing `result` error contract, verify any preexisting destination bytes remain unchanged when applicable, and verify no new writer-owned snapshot temp directory remains after the failing `write_to` call returns.

5. **DX10 lifecycle documentation**: The remaining BA2 DX10 temporary-data lifecycle behavior is documented truthfully, including residual abnormal-termination risk.
   - Current: Planning state names abnormal-termination honesty as a blocker, and the code comment in `ba2_dx10_writer::state::~state` documents best-effort teardown cleanup but there is no phase-level lifecycle statement that distinguishes normal completion, ordinary failure unwinding, destructor safety-net cleanup, and process-abnormal termination.
   - Target: Phase 17 artifacts and the relevant maintainer-facing documentation identify which BA2 DX10 temp cleanup paths are guaranteed by tests, which cleanup is best-effort, and whether hard process termination, crash, OS shutdown, or external temp-directory deletion can still leave residual temp artifacts.
   - Acceptance: A verifier can point to committed documentation or planning artifacts that explicitly list the BA2 DX10 temp lifecycle guarantees and residual risks, and no artifact claims crash-proof or hard-termination cleanup unless that behavior is actually implemented and tested.

6. **Milestone ship gate**: Phase 17 closes v1.1 only with evidence that all writer-hotspot requirements are satisfied and earlier v1.1 constraints remain intact.
   - Current: `.planning/REQUIREMENTS.md` maps `DEDU-01`, `DEDU-02`, `DX10-01`, and `DX10-02` to Phase 17 as pending, while Phases 13-16 are marked complete and verified.
   - Target: Phase 17 verification records pass/fail evidence for all four pending Phase 17 requirements, confirms no public API expansion or `TES5Edit/` modification occurred, and leaves the v1.1 milestone in a ship-ready state with no unresolved active requirements.
   - Acceptance: Final Phase 17 verification evidence names the exact tests or checks run for TES4 dedupe, BA2 GNRL dedupe, BA2 DX10 cleanup, lifecycle documentation, public API stability, and the supported Windows verification lane used for the ship gate; `.planning/REQUIREMENTS.md`, `.planning/PROJECT.md`, `.planning/ROADMAP.md`, and `.planning/STATE.md` are updated consistently if execution completes the milestone.

## Boundaries

**In scope:**
- TES4-family BSA writer dedupe candidate narrowing for `deduplicate_payloads = true`.
- BA2 GNRL writer dedupe staged identity or digest hardening for `deduplicate_payloads = true`.
- Preservation of exact final stored-byte equality as the authority for every shared dedupe offset.
- BA2 DX10 snapshot temp cleanup on successful `write_to` completion.
- BA2 DX10 snapshot temp cleanup on ordinary `result`-returning failure unwinding.
- Documentation or planning evidence for BA2 DX10 temp lifecycle guarantees and residual abnormal-termination risk.
- Focused runtime and/or source-policy tests that prove the writer-hotspot requirements and prevent regression to the known fragile patterns.
- v1.1 ship-gate evidence and consistent planning-state updates after successful execution.

**Out of scope:**
- Public writer API expansion or a new writer architecture - v1.1 is hardening-only and existing consumers must keep the same public surface.
- Changing opt-in dedupe defaults - dedupe remains disabled by default for TES4-family BSA, BA2 GNRL, and BA2 DX10 writers.
- Sharing payload offsets without exact stored-byte equality fallback - this would violate existing compatibility semantics.
- Full performance program or broad benchmarking suite - only the named dedupe hotspot narrowing is required.
- BA2 DX10 DDS feature expansion, new texture formats, new archive families, or new compression formats - Phase 17 hardens existing supported behavior only.
- Crash-proof or hard-termination temp cleanup guarantees - document residual risk unless the implementation and tests truly provide stronger behavior.
- Modifying, formatting, staging, or compiling `TES5Edit/` - the submodule remains read-only reference material.
- Cross-platform portability work - libbsa remains Windows-only.

## Constraints

- Preserve the public C++20 API in `include/libbsa/`; do not expose new public dedupe, temp-staging, compression, or DirectXTex types.
- Preserve all existing archive compatibility behavior for TES4-family BSA, BA2 GNRL, and BA2 DX10 writers, including target-specific compression routing and archive metadata.
- Preserve exact final stored-byte equality as the final dedupe authority; identity, hashes, digests, sizes, or staged keys are candidate filters only.
- Preserve Phase 13 Windows host-file boundary behavior and do not reintroduce raw narrow-string host-path opens in migrated writer paths.
- Preserve bounded-memory intent for disk-backed and DX10 staging paths; do not solve dedupe performance by whole-archive or broad whole-corpus memory accumulation.
- Do not add external dependencies beyond the approved project stack.
- Keep all implementation work outside `TES5Edit/`.
- Treat BA2 DX10 snapshot cleanup as best-effort for OS-level deletion failures unless implementation returns a stronger tested error contract without hiding the primary write result.

## Acceptance Criteria

- [ ] TES4-family BSA dedupe uses keyed or otherwise bounded candidate narrowing before exact stored-byte equality checks when `deduplicate_payloads = true`.
- [ ] TES4 writer regression coverage still proves identical final stored bytes share offsets and compression/embedded-name mismatches do not share offsets.
- [ ] BA2 GNRL dedupe uses explicit staged identity or digest narrowing and still requires exact stored-byte equality before sharing offsets.
- [ ] BA2 GNRL regression coverage still proves disk-source growth/truncation rejection and raw-vs-compressed offset separation.
- [ ] BA2 DX10 successful `write_to` completion leaves no new writer-owned `libbsa-dx10-snapshot-*` temp directory while preserving readable/extractable output.
- [ ] BA2 DX10 ordinary `result`-returning write failure leaves no new writer-owned `libbsa-dx10-snapshot-*` temp directory and preserves the primary error result.
- [ ] BA2 DX10 cleanup documentation distinguishes successful completion, ordinary failure unwinding, destructor safety-net cleanup, and residual abnormal-termination risk.
- [ ] `include/libbsa/` public writer signatures remain unchanged unless a later explicit requirement update approves an API change.
- [ ] No files under `TES5Edit/` are modified, staged, or committed.
- [ ] No new external dependencies are introduced.
- [ ] Final verification evidence covers `DEDU-01`, `DEDU-02`, `DX10-01`, and `DX10-02` and records the supported Windows verification lane used for the v1.1 ship gate.
- [ ] Planning state is updated consistently if Phase 17 completes and v1.1 becomes ship-ready.

## Ambiguity Report

| Dimension           | Score | Min    | Status | Notes |
|---------------------|-------|--------|--------|-------|
| Goal Clarity        | 0.88  | 0.75   | met    | Roadmap locks the final writer hotspot and ship-gate outcome. |
| Boundary Clarity    | 0.78  | 0.70   | met    | In-scope and out-of-scope lists separate dedupe, DX10 temp cleanup, and ship evidence from redesign or expansion. |
| Constraint Clarity  | 0.74  | 0.65   | met    | Public API stability, exact equality, host-path, bounded-memory, dependency, and TES5Edit constraints are explicit. |
| Acceptance Criteria | 0.82  | 0.70   | met    | Pass/fail checks cover all four pending Phase 17 requirements plus milestone closure evidence. |
| **Ambiguity**       | 0.19  | <=0.20 | met    | Gate passed from roadmap, requirements, state, and codebase scout in auto mode. |

Status: met = met minimum, below = below minimum (planner treats below-minimum dimensions as assumptions)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 0 | Auto current-state assessment | Initial ambiguity was scored from `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/PROJECT.md`, and `.planning/STATE.md`. | Gate passed at 0.19, so auto mode skipped interactive questions and generated SPEC.md from existing locked context. |
| 0 | Researcher scout | What exists today for Phase 17 writer hotspots? | TES4 dedupe lives in `tes4_bsa_layout.cpp`, BA2 GNRL dedupe lives in `ba2_gnrl_layout.cpp`, and BA2 DX10 snapshots are created by `ba2_dx10_snapshot_builder.cpp` then cleaned best-effort by writer teardown. |
| 0 | Boundary Keeper | Which adjacent work must stay out of Phase 17? | Public API redesign, dedupe default changes, crash-proof cleanup claims, broad benchmarking, new archive/DDS features, new dependencies, `TES5Edit/` edits, and portability work are out of scope. |
| 0 | Failure Analyst | What would cause verification to reject the phase? | Regressing exact stored-byte equality, leaking snapshot temp dirs on normal/failure paths, claiming unsupported abnormal-termination cleanup, changing public writer signatures, modifying `TES5Edit/`, or leaving Phase 17 requirements unverified. |

---

*Phase: 17-writer-hotspot-hardening-and-ship-gate*
*Spec created: 2026-05-14*
*Next step: /gsd-discuss-phase 17 - implementation decisions (how to build the locked writer hotspot hardening above)*
