---
id: S02
milestone: M001
status: ready
---

# S02: Public API and Error Contract Audit — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Audit libbsa's explicit public API and error/validation contract against real headers, docs, package-consumer usage, and the S01 matrix, preserving an understandable dependency-light core while identifying evidence-backed gaps.

## Why this Slice

S02 follows S01 because the coverage matrix establishes which public/package-consumer/API claims are proven, weak, or deferred, and downstream remediation needs a trustworthy consumer-facing contract before selecting fixes. The active `M001` state is duplicated against completed `M001-k9wo8b` artifacts, so this slice should reconcile the existing S02 baseline and `docs/public-api-reality-check.md`, drift-check the current public API story and package-consumer proof, and avoid redesigning the API from scratch unless audit evidence proves a tiny helper is necessary.

## Scope

### In Scope

- Reconcile the existing completed S02 evidence and `docs/public-api-reality-check.md` into active `M001` rather than repeating a greenfield API audit.
- Audit the existing explicit public core: `archive_reader`, `validate_archive`, bulk extraction, `result<T>`, and family-specific writers for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Compare public headers, docs, package-consumer usage, policy tests, and S01 matrix findings for drift, overclaims, unclear guidance, dependency leakage, and branchable error behavior.
- Cover the first serious consumer journey: open/list metadata, lookup, contains checks, streaming extraction, `extract_bytes`, bulk extraction, validation reports, compatibility warnings, stable error-code branching, and all writer families.
- Make the public API feel like an explicit, intentional core rather than a hidden or accidental set of primitives.
- Use installed-package compile/link plus cheap runtime smoke through `<libbsa/libbsa.hpp>` and `libbsa::libbsa` as the S02 proof floor for representative core flows.
- Preserve the explicit core API while allowing a tiny helper only if audit evidence proves repeated misuse risk, docs/examples cannot make the flow safe, the helper stays dependency-light C++20, and package-consumer proof lands with it immediately.
- Make `validate_archive` semantics clear: result-level failures for setup/caller/host-path issues; report-level diagnostics for safely inspectable malformed or unsupported archive bytes; warnings for non-fatal compatibility concerns.
- Repair docs, examples, policy tests, or package-consumer proof when implementation behavior is sound but the public story has drifted.
- Route behavior bugs or deeper validation/error inconsistencies to S03/S04 with gap IDs unless they are small proof/story repairs within S02's scope.

### Out of Scope

- Creating a broad ergonomic facade or major public API redesign.
- Treating ordinary boilerplate alone as justification for new public API.
- Making public header changes by default; helpers require concrete misuse/friction evidence and immediate docs/package-consumer proof.
- Fixing reader, writer, round-trip, malformed, compression, validation, or compatibility behavior bugs that belong in S03/S04 unless they are unavoidable tiny proof repairs.
- Requiring copyrighted local archives or BSArchPro-derived outputs for package-consumer/API proof.
- Exposing libdeflate, LZ4 library types, DirectXTex, DXGI/private implementation types, C++23-only `std::expected`, or TES5Edit-derived implementation details in public headers.
- Adding new archive formats, GUI/CLI surfaces, in-place mutation, performance/stress gates, release-readiness declarations, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- Public API remains C++20 and dependency-light.
- Keep `archive_reader`, `validate_archive`, bulk extraction, `result<T>`, and family-specific writers as the stable public core unless a tiny helper clears the evidence gate.
- Branchable `error_code` and `compatibility_warning_code` values are the machine-facing contract; diagnostic message text remains human-readable and non-stable.
- The most important error/validation boundary for S02 is result-level setup/caller/host-path failures versus inspectable archive diagnostics inside `validation_report`.
- Package-consumer proof must use the installed/exported `libbsa::libbsa` target and `<libbsa/libbsa.hpp>` without source-tree fixture or local-corpus dependencies.
- Archive virtual paths are not host filesystem paths; examples must not imply unsafe automatic host-path mapping.
- Sharp edges should be documented plainly: `find` versus `contains`, streaming `extract` versus bounded `extract_bytes`, bulk per-entry failures, positive `worker_count`, writer finalization/publication rules, and BA2 DX10 one-shot lifecycle.
- Optional game-corpus and BSArchPro-derived evidence remains advisory and non-blocking, not part of the S02 proof floor.
- Keep `TES5Edit/` strictly read-only and outside generated outputs, mutable fixtures, formatting, staging, and build inputs.

## Integration Points

### Consumes

- `.gsd/milestones/M001/slices/S01/S01-CONTEXT.md` — Active S01 proof boundaries and gap-ranking decisions that determine which public/package-consumer claims need S02 scrutiny.
- `.gsd/milestones/M001-k9wo8b/slices/S02/S02-CONTEXT.md` — Completed baseline S02 scope and known public API sharp edges.
- `.gsd/milestones/M001-k9wo8b/slices/S02/S02-SUMMARY.md` — Completed S02 outcome, proof strategy, no-broad-facade decision, and known follow-ups.
- `docs/public-api-reality-check.md` — Existing API audit baseline to reconcile and drift-check.
- `docs/coverage-audit-matrix.md` — Source of public/package-consumer/docs proof cells and COV-GAP routing.
- `include/libbsa/libbsa.hpp` — Umbrella include that should remain the primary consumer entry point.
- `include/libbsa/archive.hpp` — Public reader, metadata, lookup, streaming extraction, `extract_bytes`, and bulk extraction surface.
- `include/libbsa/writer.hpp` — Public TES3, TES4-family, BA2 GNRL, and BA2 DX10 writer targets, options, lifecycle, and finalization surface.
- `include/libbsa/validation.hpp` — Public validation report, diagnostics, options, compatibility warnings, and warning-code contract.
- `include/libbsa/result.hpp` — Public C++20 result/error model and stable error-code categories.
- `docs/api-mainpage.md`, `docs/integration-examples.md`, `docs/thread-safety.md`, `docs/target-format-guide.md`, and `docs/compatibility-evidence.md` — Public docs that carry API guidance, error semantics, lifecycle guidance, concurrency expectations, target/compression routes, and optional-evidence boundaries.
- `tests/package-consumer/` — Installed-package compile/link/runtime smoke proof for public API usability.
- `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`, and related validation/error tests — Policy and behavior guardrails for public boundary, docs claims, package-consumer proof, and error/validation semantics.

### Produces

- Active M001 public API/error contract audit findings — A concise determination of whether the existing explicit core remains trustworthy and understandable.
- Updated or accepted `docs/public-api-reality-check.md` findings — The public API story either accepted as current or repaired for drift.
- Docs/example/policy/package-consumer proof updates, if needed — Repairs when implementation behavior is sound but the public story or proof floor is stale.
- Evidence-backed tiny-helper recommendation or implementation, if justified — Only when the misuse/friction and package-consumer proof gates are met.
- S03 input — Behavior, fixture, writer, extraction, or round-trip gaps discovered while auditing public API flows.
- S04 input — Validation, result/error, diagnostic-shape, or compatibility-warning contract gaps discovered during the audit.
- S04/final closeout input — Confirmation that installed/package-consumer public API proof and dependency-light header boundaries are still valid, plus any deferred API/story gaps.

## Open Questions

- Whether any current drift exists between `docs/public-api-reality-check.md`, `docs/coverage-audit-matrix.md`, and package-consumer proof — current thinking: accept the completed baseline unless focused checks show drift.
- Whether a tiny helper is actually justified — current thinking: possible but unlikely; require concrete repeated-misuse evidence, dependency-light C++20 shape, documentation, and immediate package-consumer proof.
- Exact verification lane for S02 — current thinking: focused public-api/docs-policy/target-format/package-consumer checks when practical, with final default-plus-package proof left to closeout.
