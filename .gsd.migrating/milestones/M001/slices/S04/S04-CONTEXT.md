---
id: S04
milestone: M001
status: ready
---

# S04: Integrated Verification and Truth Closeout — Context

## Goal

Finalize active M001 by reconciling completed validation and integrated-confidence evidence, proving the current default-plus-package path, and leaving the public support matrix as the blunt source of truth.

## Why this Slice

S04 comes last because it consumes S01's evidence matrix, S02's public API and error-contract audit, and S03's bounded proof-locking/remediation decisions. The active M001 roadmap has four slices, while the already-completed `M001-k9wo8b` baseline split comparable work across S04 validation stabilization and S05 integrated confidence; this slice should reconcile those completed conclusions into active M001 rather than duplicate implementation. Order matters because final closeout is only credible after the matrix, API story, fixed/former gaps, package-consumer proof, optional-evidence boundaries, and TES5Edit boundary can be checked together.

## Scope

### In Scope

- Treat completed `M001-k9wo8b` S04 validation/error stabilization and S05 package-consumer/final-matrix closeout as the starting baseline, then perform focused drift checks against the current active M001 repository state.
- Keep `docs/coverage-audit-matrix.md` as the matrix-led public truth artifact: future readers should see what is Proven, Fixed, Partial, Missing, Deferred, closed/former, or advisory without reassuring overclaim language.
- Confirm that closed former gaps remain traceable: `COV-GAP-001` for direct validation success proof and `COV-GAP-003` for installed package-consumer runtime proof.
- Ensure remaining deferred gaps, especially `COV-GAP-002` and `COV-GAP-004`, state the rationale, why they are non-blocking for M001, the likely future owner/milestone, and what proof would be needed to close them.
- Run or require the default-plus-package proof gate: configure the default Windows MSVC debug-static preset, build the default test target, run full default CTest, run/package-consumer proof through the relevant label or test name, and run focused coverage/docs/target-format policy checks.
- Verify package-consumer behavior through the installed/exported public surface: `<libbsa/libbsa.hpp>` and the `libbsa::libbsa` target must still create, open, validate, and extract writer-produced archives across the current family set.
- Verify the TES5Edit read-only boundary during closeout and treat any mutation under `TES5Edit/` as a core blocker.
- Record optional local game-corpus, BSArchPro-derived, release package-lane, and ASan/hardening evidence as run, skipped, unavailable, or advisory without letting it turn a matrix cell green by itself.
- Repair core verification blockers or matrix/docs overclaims discovered during closeout, while routing non-core compatibility, warning-taxonomy, performance, release, or API-polish work to future milestones.
- Produce a final handoff that names mandatory proof commands/results, optional evidence status, closed gaps, deferred gaps, and future ownership.

### Out of Scope

- Re-running the full original M001 audit from scratch when completed `M001-k9wo8b` evidence and current focused drift checks are sufficient.
- Starting a new remediation tranche unless a core default-plus-package gate, package-consumer path, matrix truth claim, or TES5Edit boundary check fails.
- Requiring local copyrighted game archives, BSArchPro output, or optional local manifests for default completion.
- Requiring every supported Windows preset, release static/shared package lane, or MSVC ASan hardening lane as an absolute local completion gate.
- Treating optional local corpus or BSArchPro evidence as mandatory default proof or as a substitute for committed/generated fixtures, always-on tests, package-consumer proof, docs-policy checks, or focused verification.
- Broad public facade redesign, speculative helper APIs, richer diagnostic taxonomy, exact diagnostic-message contracts, performance/stress campaigns, release-readiness declarations, new archive formats, GUI/CLI work, in-place mutation, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- Evidence before completion claims: S04 cannot call the slice or milestone complete unless mandatory proof has fresh current evidence or any core blocker has been resolved and rechecked.
- The completion feel is matrix-led and blunt; public docs must not hide overclaim risk behind optimistic wording.
- Default completion must remain legally reproducible from committed/generated fixtures, CTest/Catch2 checks, policy tests, and package-consumer proof without local copyrighted archives.
- Core failures block completion: default configure/build/full CTest failure, installed/package-consumer runtime failure, matrix/docs-policy overclaim, package/export breakage, or TES5Edit mutation.
- Optional evidence absence is not a blocker; it must be documented as advisory or unavailable rather than silently assumed.
- Public headers must remain C++20 and dependency-light: no public `std::expected`, DirectXTex/DXGI object types, libdeflate/LZ4 library types, or private parser/writer types.
- Preserve the existing public result and validation split: branch on stable coarse `error_code` / warning-code categories, not exact diagnostic text.
- Preserve Windows/MSVC/vcpkg scope and do not add cross-platform requirements.
- Do not stage or commit `.gsd/` planning artifacts; GSD manages them externally.

## Integration Points

### Consumes

- `.gsd/milestones/M001/slices/S01/S01-CONTEXT.md` — Active evidence-matrix and gap-ranking decisions that define the support-truth vocabulary and advisory-evidence boundary.
- `.gsd/milestones/M001/slices/S02/S02-CONTEXT.md` — Active public API, package-consumer, result/error, validation, and dependency-light header boundaries.
- `.gsd/milestones/M001/slices/S03/S03-CONTEXT.md` — Active bounded-remediation and proof-locking decisions, especially the rule to avoid expanding adjacent work casually.
- `.gsd/milestones/M001-k9wo8b/slices/S04/S04-SUMMARY.md` — Completed validation/error stabilization baseline and direct validation proof for former `COV-GAP-001`.
- `.gsd/milestones/M001-k9wo8b/slices/S05/S05-SUMMARY.md` — Completed integrated confidence baseline, installed package-consumer runtime proof, final matrix updates, and deferred-gap ownership.
- `docs/coverage-audit-matrix.md` — Public matrix-led support-truth artifact to verify and update if drift is found.
- `docs/public-api-reality-check.md`, `docs/compatibility-evidence.md`, `docs/integration-examples.md`, `docs/api-mainpage.md`, and `docs/target-format-guide.md` — Public support/API/compatibility docs that must remain aligned with the final matrix.
- `tests/package-consumer/` and `tests/CMakeLists.txt` — Installed/exported package-consumer runtime proof and CTest registration.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`, `tests/unit/validation_api_tests.cpp`, and `tests/unit/compatibility_warning_tests.cpp` — Focused policy, validation, package-consumer, and warning proof surfaces.
- `tests/fixtures/generated/` and `tests/fixtures/generated/validate_fixture_manifests.py` — Generated legal fixtures and manifest validation proof for default completion.
- `CMakePresets.json`, `.github/workflows/ci.yml`, `README.md`, and package/export CMake wiring — Windows/MSVC/vcpkg verification lanes and public installed target behavior.
- `TES5Edit/` — Read-only behavioral reference boundary to check for cleanliness only; never mutable input or output.

### Produces

- Final active `S04-CONTEXT.md` for planning and execution.
- Later S04 plan/tasks that focus on reconciliation, default-plus-package verification, matrix truth closeout, and blocker-only remediation.
- A final verification record with commands, preset names, pass/fail counts, optional skipped/unavailable evidence, and TES5Edit boundary status.
- Any needed matrix/docs updates that keep public truth aligned with current proof.
- Final S04 summary handoff naming closed/former gaps, deferred gaps, owners, proof-to-close requirements, and requirements advanced/validated.

## Open Questions

- Does current default-plus-package verification still pass in this environment? — Current thinking: run it before completion; if it fails, treat it as a core blocker unless clearly outside the default proof contract and explicitly de-scoped.
- Are release static/shared package lanes and MSVC ASan practical to run locally during S04 execution? — Current thinking: run when cheap/available, but record unavailable or skipped lanes as advisory rather than blocking active M001.
- What exact TES5Edit boundary check should execution use? — Current thinking: prefer `git status --short -- TES5Edit`; if normal git status is unavailable or prohibited, document a non-git freshness check and keep any detected mutation blocking.
- Will S04 need code/docs edits or only verification and summary updates? — Current thinking: no duplicate implementation; edit only if focused drift checks uncover overclaim, stale docs, broken package proof, or a core verification failure.
