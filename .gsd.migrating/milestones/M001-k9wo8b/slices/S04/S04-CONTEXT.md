---
id: S04
milestone: M001-k9wo8b
status: ready
---

# S04: Error and Validation Stabilization — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Stabilize high-risk public result/error/validation inconsistencies discovered by the audit or gap-closure work, with focused proof and no public error-model redesign.

## Why this Slice

S04 follows S01 and S03 because error and validation behavior should be stabilized against actual evidence, not guessed in isolation. S01 identifies where support claims and proof are weak, while S03 may expose concrete fixture, extraction, writer, or malformed-input behavior that fails through inconsistent categories or unclear validation reports. Stabilizing the high-risk findings here keeps S05’s integrated confidence pass from inheriting ambiguous public failure behavior.

## Scope

### In Scope

- Consume S01 matrix findings, S03 fixed-gap evidence, and any S02 public API findings that involve public `result<T>`, `error_code`, `validation_report`, or compatibility warnings.
- Stabilize public contract boundaries first: caller/setup/host-path failures should remain result-level where the public API already defines them, while readable malformed or unsupported archive bytes should become inspectable `validation_report::errors` when using `validate_archive`.
- Preserve compatible-but-noteworthy conditions as `validation_report::warnings`, not fatal validation errors, unless the archive is actually unreadable or unsafe to process.
- Add focused tests for high-risk inconsistencies in open/detect, lookup/extraction, bulk extraction, writer finalization, validation setup, validation extractability, and compatibility-warning behavior when those inconsistencies are identified by S01/S03/S02 evidence.
- Keep validation reports focused and predictable: one clear fatal diagnostic per failing phase is acceptable when that is the strongest reliable evidence, and callers should be able to branch on stable `error_code` values.
- Improve diagnostic messages only where evidence shows the current message is too vague for humans to understand the operation, archive family, or failure phase.
- Verify behavior through public `result<T>`, `validation_report`, `validation_diagnostic`, and `compatibility_warning` assertions rather than private internals.
- Document or hand off any lower-risk taxonomy, wording, or exhaustive consistency findings that are outside the selected evidence-bounded tranche.

### Out of Scope

- Redesigning the public failure model or replacing `libbsa::result<T>` with exceptions, `std::expected`, or a richer public diagnostic hierarchy.
- Expanding the public `error_code` enum unless a high-risk M001 finding absolutely cannot be represented by the current stable categories.
- Making exact diagnostic message strings stable machine-readable contracts; messages remain human-readable and may improve over time.
- Performing an exhaustive wording-polish pass over every error message in the repository.
- Reclassifying warnings as hard failures merely because they represent compatibility risk; valid-but-risky conditions should remain warnings.
- Broad fixture, round-trip, or parser compatibility work that belongs to S03, unless needed to verify a selected S04 error/validation inconsistency.
- Public API facade or helper design; S02 owns API story and helper recommendations.
- Adding new archive formats, GUI/CLI surfaces, in-place mutation, broad performance gates, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- S04 is evidence-bounded: fix high-risk inconsistencies surfaced by S01, S03, or S02, and avoid expanding into broad taxonomy redesign.
- Keep stable public categories coarse and branchable: `unsupported`, `invalid_argument`, `not_found`, `io_error`, and `format_error`.
- Treat diagnostic messages as human-readable context, not exact stable API; tests should prefer codes and structural assertions, using message fragments only when needed to prevent vague diagnostics.
- Keep `validate_archive` semantics predictable: result-level failures are for call/setup failures, while inspectable archive data problems appear in `validation_report::errors`.
- Keep compatibility warnings non-fatal with stable public `compatibility_warning_code` and severity values.
- Default proof must run from committed/generated legal fixtures and default CTest/Catch2 paths without local game archives or BSArchPro-derived outputs.
- Public headers must remain dependency-light C++20 and must not expose private parser, codec, DirectXTex/DXGI, Windows implementation, or C++23-only types.

## Integration Points

### Consumes

- S01 coverage/gap matrix artifact — Identifies malformed, validation, compatibility, and public error-behavior cells that need stabilization or deferral.
- S01 ranked gap list — Provides risk ordering and evidence sources for error/validation gaps.
- S03 fixed-gap evidence and handoff findings — Supplies concrete fixture or round-trip failures where public errors, validation reports, or warnings were inconsistent.
- S02 public API findings — Supplies any consumer-facing result/error/validation friction discovered while auditing docs and package-consumer usage.
- `include/libbsa/result.hpp` — Public `result<T>`, `result<void>`, `error`, and stable `error_code` contract.
- `include/libbsa/validation.hpp` — Public validation report, diagnostic, warning, severity, and options contract.
- `src/validation.cpp` — `validate_archive` behavior, result-level versus report-level boundary, extractability validation, and warning construction.
- `include/libbsa/archive.hpp` and reader implementations — Public open, metadata, lookup, extraction, `extract_bytes`, and bulk extraction error behavior.
- `include/libbsa/writer.hpp` and writer implementations — Writer add/finalization/publication error behavior and diagnostic prefixes.
- `tests/unit/validation_api_tests.cpp` — Existing setup, generated fixture, writer-produced, malformed, and extractability validation proof.
- `tests/unit/compatibility_warning_tests.cpp` — Existing behavior proof for every public compatibility warning code.
- `tests/unit/result_tests.cpp` and public API/policy tests — Existing public result and boundary proof.
- `tests/fixtures/generated/compatibility_matrix.json` and malformed manifests — Existing malformed evidence used by validation tests and S01/S03 gap analysis.
- `docs/integration-examples.md`, `docs/target-format-guide.md`, and `docs/compatibility-evidence.md` — Consumer-facing error/warning and validation semantics that must stay truthful.

### Produces

- Stabilized public error/validation behavior — Focused code, tests, or docs changes for the selected high-risk inconsistencies.
- Durable proof references — Test names, fixture/manifest references, and assertions showing the public result/report/warning behavior is stable.
- S05 input — A summary of fixed error/validation gaps, deferred lower-risk inconsistencies, and verification commands or tests to include in the integrated confidence pass.
- Matrix update evidence — Fixed/deferred status references for S01 matrix cells related to malformed handling, validation, compatibility warnings, and public error behavior.
- Future-work notes — Any broad taxonomy, richer diagnostics, or wording polish that should wait for a later API/compatibility milestone.

## Open Questions

- Exact selected inconsistencies — Current thinking: wait for S01 and S03 outputs, then select only high-risk result/error/validation findings that affect public correctness or consumer confidence.
- Message-fragment assertions — Current thinking: prefer stable code/shape assertions; use message fragments sparingly when a test needs to prevent regression to vague or context-free diagnostics.
- Warning scope expansion — Current thinking: do not add new warning categories in S04 unless S01/S03 evidence shows a current valid-but-risky condition is already claimed or should be surfaced within M001.
