---
id: S02
milestone: M001
status: draft
---

# S02: Public API and Error Contract Audit — Context Draft

## Goal

Audit libbsa's explicit public API and error/validation contract against real headers, docs, package-consumer usage, and the S01 matrix, preserving an understandable dependency-light core while identifying evidence-backed gaps.

## Why this Slice

S02 follows S01 because the coverage matrix establishes which public/package-consumer/API claims are proven, weak, or deferred. The active M001 should reconcile the completed M001-k9wo8b S02 baseline and `docs/public-api-reality-check.md` rather than redesigning the API from scratch, while still drift-checking the current public story and package-consumer proof.

## Confirmed Human Decisions So Far

- Treat the existing completed S02 evidence and `docs/public-api-reality-check.md` as the starting baseline; reconcile it into active M001 and avoid duplicate redesign work.
- The public API should feel like an explicit, intentional core: `archive_reader`, `validate_archive`, bulk extraction, `result<T>`, and family-specific writers are the stable story.
- S02 proof floor should include installed-package compile/link plus cheap runtime smoke through `<libbsa/libbsa.hpp>` and `libbsa::libbsa` for representative core flows.
- If the API feels awkward but the existing surface works, tiny helpers are allowed only when audit evidence proves repeated misuse risk and the helper can be documented and package-consumer tested immediately.
- The error/validation contract should make the `result<T>` versus `validation_report` boundary most explicit: setup/caller/host-path failures can be result-level, while inspectable archive diagnostics belong in reports when safe.
- If S02 finds drift where implementation behavior is sound but docs/examples/policy/package-consumer proof is wrong, repair the proof and public story first.
- Behavior bugs or deeper validation/error inconsistencies should be routed to S03/S04 with gap IDs unless they are small proof/story repairs.

## Scope

### In Scope

- Audit the existing explicit public core rather than designing a broad facade.
- Compare public headers, docs, package-consumer usage, policy tests, and S01 matrix findings for drift, overclaims, unclear guidance, dependency leakage, and branchable error behavior.
- Cover the first serious consumer journey: open/list metadata, lookup, contains checks, streaming extraction, `extract_bytes`, bulk extraction, validation reports, compatibility warnings, stable error-code branching, and all writer families.
- Preserve the explicit core API while allowing a tiny helper only if evidence proves docs/examples cannot make the flow safe and the helper can be tested immediately through package-consumer usage.
- Keep `validate_archive` semantics clear: result-level failures for setup/caller/host-path issues; report-level diagnostics for safely inspectable malformed or unsupported archive bytes; warnings for non-fatal compatibility concerns.
- Use package-consumer smoke as the S02 proof floor for public API usability, supported by docs-policy and public-boundary tests.
- Repair docs, examples, policy tests, or package-consumer proof when implementation behavior is sound but the public story has drifted.
- Produce downstream gap routing for S03/S04 when S02 observes behavior, validation, or error contract gaps beyond docs/proof repair.

### Out of Scope

- Creating a broad ergonomic facade or major API redesign.
- Treating ordinary boilerplate alone as justification for new public API.
- Making public header changes by default; helpers require concrete misuse/friction evidence and immediate docs/package proof.
- Fixing reader, writer, round-trip, malformed, compression, validation, or compatibility behavior bugs that belong in S03/S04 unless they are unavoidable tiny proof repairs.
- Requiring copyrighted local archives or BSArchPro-derived outputs for package-consumer/API proof.
- Exposing libdeflate, LZ4 library types, DirectXTex, DXGI/private implementation types, C++23-only `std::expected`, or TES5Edit-derived implementation details in public headers.
- Adding new archive formats, GUI/CLI surfaces, in-place mutation, performance/stress gates, release-readiness declarations, or platform expansion.

## Constraints

- Public API remains C++20 and dependency-light.
- Keep `archive_reader`, `validate_archive`, bulk extraction, `result<T>`, and family-specific writers as the stable public core unless a tiny helper clears the evidence gate.
- Branchable `error_code` and `compatibility_warning_code` values are the machine-facing contract; diagnostic message text remains human-readable and non-stable.
- Package-consumer proof must use the installed/exported `libbsa::libbsa` target and `<libbsa/libbsa.hpp>` without source-tree fixture or local-corpus dependencies.
- Archive virtual paths are not host filesystem paths; examples must not imply unsafe automatic host-path mapping.
- Sharp edges should be documented plainly: `find` versus `contains`, streaming `extract` versus bounded `extract_bytes`, bulk per-entry failures, positive `worker_count`, writer finalization/publication rules, and BA2 DX10 one-shot lifecycle.
- TES5Edit remains read-only reference only and must not be edited, compiled, staged, vendored, or used as mutable fixture data.

## Integration Points

### Consumes

- `.gsd/milestones/M001/slices/S01/S01-CONTEXT.md` — Active S01 proof boundaries and gap-ranking decisions.
- `.gsd/milestones/M001-k9wo8b/slices/S02/S02-CONTEXT.md` — Completed baseline S02 scope and known public API sharp edges.
- `.gsd/milestones/M001-k9wo8b/slices/S02/S02-SUMMARY.md` — Completed S02 outcome, proof strategy, and no-broad-facade decision.
- `docs/public-api-reality-check.md` — Existing API audit baseline to reconcile and drift-check.
- `docs/coverage-audit-matrix.md` — Source of public/package-consumer/docs proof cells and COV-GAP routing.
- `include/libbsa/libbsa.hpp`, `archive.hpp`, `writer.hpp`, `validation.hpp`, and `result.hpp` — Public header surfaces to audit for clarity, stability, and dependency boundaries.
- `docs/api-mainpage.md`, `docs/integration-examples.md`, `docs/thread-safety.md`, `docs/target-format-guide.md`, and `docs/compatibility-evidence.md` — Public docs that carry API guidance, error semantics, lifecycle guidance, and optional-evidence boundaries.
- `tests/package-consumer/` — Installed-package compile/link/runtime smoke proof for public API usability.
- `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`, and related validation/error tests — Policy and behavior guardrails for public boundary and docs claims.

### Produces

- Active M001 S02 public API/error contract context and later plan inputs.
- Updated or accepted public API reality-check findings, with drift repairs if needed.
- Docs/example/policy/package-consumer proof updates when the public story is stale but behavior is sound.
- Evidence-backed helper/API candidate list, if any, with package-consumer proof requirements.
- Gap routing to S03/S04 for behavior, validation, result/error, or compatibility-warning issues discovered during the audit.

## Open Questions

- Whether any current drift exists between `docs/public-api-reality-check.md`, `docs/coverage-audit-matrix.md`, and package-consumer proof — current thinking: accept baseline unless focused checks show drift.
- Whether a tiny helper is actually justified — current thinking: possible but unlikely; require concrete misuse evidence and immediate package-consumer proof.
- Exact verification lane for S02 — current thinking: focused public-api/docs-policy/target-format/package-consumer checks when practical, with final default-plus-package proof left to closeout.
