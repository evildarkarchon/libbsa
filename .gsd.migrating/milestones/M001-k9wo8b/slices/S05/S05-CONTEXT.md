---
id: S05
milestone: M001-k9wo8b
status: ready
---

# S05: Integrated Confidence Pass — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Finalize M001 confidence by updating the coverage matrix with fixed/deferred status and proving the default build/test/package-consumer path without requiring local copyrighted fixtures.

## Why this Slice

S05 is last because it consumes the audit truth from S01, the public API story from S02, the fixture and round-trip fixes from S03, and the error/validation stabilization from S04. Its job is not to discover a new tranche of work, but to integrate the evidence, make remaining gaps explicit, and prove that the library still works as a package-consumable whole. This is the slice that turns the milestone from “several good fixes happened” into a coherent baseline future work can trust.

## Scope

### In Scope

- Consume all prior slice outputs: S01 coverage/gap matrix and ranked gaps, S02 API findings, S03 fixed-gap proof, and S04 error/validation stabilization proof.
- Update the coverage matrix as the source of truth, using blunt final statuses such as Proven, Fixed, Partial, Missing, and Deferred.
- Ensure every final non-green matrix cell has evidence, a gap ID, a rationale, and likely future ownership.
- Run or document the agreed integrated verification gate: the normal/default Windows MSVC CTest path plus package-consumer proof, with Release package-proof lanes used when practical for installed/exported target confidence.
- Preserve the existing supported-lane story: debug static as the quick default path, debug shared as inner-loop shared coverage, release static/shared as package-proof lanes, and MSVC ASan as a separate hardening lane.
- Treat heavier lanes that cannot be run in the current environment as explicitly documented verification evidence or deferrals, not silent assumptions.
- Verify that package-consumer coverage still proves `<libbsa/libbsa.hpp>` and the exported `libbsa::libbsa` target are usable.
- Verify that default checks remain independent of local copyrighted fixtures and BSArchPro-derived outputs.
- Keep optional local game/BSArchPro comparison evidence advisory and separate from default completion; if unavailable, document that unavailability without failing default acceptance.
- Produce a concise final handoff showing fixed gaps, deferred gaps, proof commands/tests, and any future milestone owners.

### Out of Scope

- Starting a new remediation tranche after S02/S03/S04 unless an integration blocker prevents the agreed default verification gate from passing.
- Requiring all supported presets, including the MSVC ASan hardening lane, as an absolute local completion gate for S05.
- Requiring local game archives, BSArchPro-derived manifests, or other optional corpus data to call M001 complete.
- Expanding from matrix finalization into a broad compatibility campaign, performance campaign, release-readiness campaign, or public API redesign.
- Treating optional advisory evidence as equivalent to default green proof in the matrix.
- Adding new archive formats, GUI/CLI surfaces, in-place mutation, broad performance gates, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- The matrix-led finish is the user-facing source of truth: fixed and deferred states must be visible directly in or next to the matrix.
- Default completion must remain runnable from committed/generated legal fixtures, policy tests, package-consumer checks, and CTest without local copyrighted archives.
- Package-consumer proof matters even if core unit tests pass; package-consumer failures are milestone blockers unless explicitly deferred with strong rationale.
- Do not overclaim support based on code inspection, optional local evidence, or stale docs; final green status needs default proof.
- Keep public headers dependency-light and C++20-compatible through the integrated pass.
- Preserve Windows/MSVC/vcpkg-only support boundaries; do not introduce portability requirements or cross-platform lanes.
- Keep `TES5Edit/` read-only and verify it remains untouched.
- Remaining gaps must be explicitly deferred with rationale and likely owner; “unknown” or “not checked” is not an acceptable final state for a claimed capability.

## Integration Points

### Consumes

- S01 coverage/gap matrix artifact — The primary matrix to update with final Proven/Fixed/Partial/Missing/Deferred status.
- S01 ranked gap list — Source of original risk-ranked gaps and any items that must be marked fixed or deferred.
- S02 public API reality-check findings — Public API, docs, package-consumer, and helper-gap conclusions to integrate into final status.
- S03 fixed-gap proof — Fixture, manifest, writer, round-trip, and validation evidence for selected closed gaps.
- S04 stabilized error/validation proof — Public result/error, validation, diagnostic, and compatibility-warning evidence for selected closed gaps.
- `CMakePresets.json` — Supported Windows MSVC configure/build/test lane definitions.
- `.github/workflows/ci.yml` — CI wiring for supported lanes and TES5Edit read-only verification.
- `README.md` — Public supported-verification lane story that must remain truthful.
- `tests/CMakeLists.txt` — CTest registration for unit tests, generated fixture manifest validation, package-consumer smoke, runtime DLL copy, and shared export-surface checks.
- `tests/package-consumer/` — Installed package smoke and runtime DLL proof.
- `tests/fixtures/generated/` — Generated legal fixture archives, manifests, fixture generators, malformed matrix, and manifest validation script.
- `tests/unit/` — Default Catch2/CTest proof across reader, writer, validation, compatibility, public API, policy, compression, path, and fixture behavior.
- `docs/compatibility-evidence.md`, `docs/target-format-guide.md`, `docs/api-mainpage.md`, and `docs/integration-examples.md` — Public documentation that must align with matrix results and final support claims.
- Optional `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` inputs — Advisory evidence only; skipped/unavailable status must be non-blocking for default completion.

### Produces

- Final updated coverage/gap matrix — Matrix-led source of truth with final fixed/deferred/proven status, evidence references, and future owners.
- Integrated verification record — Commands, presets, CTest/package-consumer results, and any environment limitations for lanes not run locally.
- Final deferred-gap list — Remaining gaps grouped by rationale and likely future owner or milestone direction.
- M001 completion evidence for requirements R001-R009 — Proof that active requirements were re-checked, satisfied, or explicitly deferred where allowed.
- S05 summary input — A concise handoff for future milestones: what is now trustworthy, what was fixed, what remains advisory, and what should be tackled next.
- Optional evidence note — A clear statement of whether local game/BSArchPro evidence was unavailable, skipped, or run as advisory confidence.

## Open Questions

- Exact verification commands — Current thinking: use the default Windows MSVC CTest path plus package-consumer proof, and run Release package-proof lanes when practical; document any heavier unrun lanes and rely on policy/CI wiring only when local execution is not available.
- Exact matrix path — Current thinking: follow S01’s chosen artifact path once S01 is implemented; S05 should update that artifact rather than create a competing final truth source.
- Optional evidence availability — Current thinking: treat local game/BSArchPro checks as advisory; absence should be recorded, not considered a failure.
