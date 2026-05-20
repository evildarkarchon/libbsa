---
phase: M001-k9wo8b
phase_name: Audit and Stabilization Baseline
project: libbsa
generated: 2026-05-20T04:22:18Z
counts:
  decisions: 4
  lessons: 5
  patterns: 6
  surprises: 4
missing_artifacts: []
---

# M001-k9wo8b Learnings

### Decisions

- Keep the public API core centered on `archive_reader`, `validate_archive`, extraction helpers, `result<T>`, and family-specific writers; do not add a broad facade or speculative helper API without concrete consumer misuse evidence.
  Source: S02-SUMMARY.md/What Happened

- Treat generated legal fixtures, always-on CTest policy checks, and package-consumer tests as default proof; keep local game archives and BSArchPro-derived comparisons advisory and opt-in.
  Source: S01-SUMMARY.md/What Happened

- Stabilize validation and error behavior through public `validate_archive`, `result<T>`, `validation_report`, and compatibility-warning tests rather than changing private parser internals or redesigning the public error model.
  Source: S04-SUMMARY.md/What Happened

- Retain closed gap identifiers such as COV-GAP-001 and COV-GAP-003 as former-gap traceability instead of deleting them once fixed.
  Source: S05-SUMMARY.md/What Happened

### Lessons

- The existing public API surface was more capable than remembered; M001's highest-value API work was documenting and proving the core consumer journey rather than adding new public surface area.
  Source: S02-SUMMARY.md/What Happened

- COV-GAP-001 was primarily a validation-evidence gap, not a production validation-code bug; the existing result/report/warning model already supported the desired public contract once behavior-specific tests were added.
  Source: S04-SUMMARY.md/What Happened

- Installed package-consumer proof can stay fixture-free by generating representative archives at runtime through the public umbrella header and exported target, then reopening, validating, and extracting them through the same public API.
  Source: S05-SUMMARY.md/What Happened

- Optional release static/shared package-consumer lanes may fail before libbsa configuration because of local vcpkg/Visual Studio detection issues; such failures should be recorded as advisory environment limitations unless they reach project configuration or test execution.
  Source: S05-SUMMARY.md/Deviations

- In the shared build tree used during S03 closeout, separate CTest label or pattern runs should be serialized to avoid Catch2 `PRE_TEST` discovery races.
  Source: S03-SUMMARY.md/Deviations

### Patterns

- Maintain a human-first coverage matrix for support truth, requiring each family/capability cell to cite default evidence or explicitly expose uncertainty through `Partial`, `Missing`, `Deferred`, `N/A`, or `COV-GAP-*` language.
  Source: S01-SUMMARY.md/patterns_established

- Use lightweight Catch2 docs-policy tests to guard public documentation contracts and support claims without introducing runtime format behavior changes.
  Source: S01-SUMMARY.md/patterns_established

- Keep `compatibility_matrix.json` scoped as malformed-hardening evidence only; do not let it substitute for the full support matrix across reader, writer, validation, package-consumer, and docs axes.
  Source: S01-SUMMARY.md/patterns_established

- When runtime proof already exists but is scattered, stabilize support claims by documenting the proof catalog and adding executable docs-policy guardrails that read tracked public docs.
  Source: S03-SUMMARY.md/patterns_established

- Validation/error stabilization tests should assert stable public codes and report/warning shape, using message fragments only as narrow vague-diagnostic guards.
  Source: S04-SUMMARY.md/patterns_established

- Installed package-consumer runtime proof should create writer-produced archives at runtime through `<libbsa/libbsa.hpp>` and `libbsa::libbsa`, not depend on internal fixtures, `.gsd`, planning files, TES5Edit, or local corpora.
  Source: S05-SUMMARY.md/patterns_established

### Surprises

- A broad public facade was not immediately justified; the audit found proof, documentation, and consumer-example gaps before it found an unavoidable ergonomic API gap.
  Source: S02-SUMMARY.md/What Happened

- The high-risk validation stabilization slice required no production validation-code or public enum changes because the existing public error model already fit the contract.
  Source: S04-SUMMARY.md/What Happened

- Release static/shared package-consumer attempts failed inside vcpkg Visual Studio detection before libbsa configuration, making them environment-limit evidence rather than milestone blockers.
  Source: S05-SUMMARY.md/Deviations

- Validation closeout needed a traceability clarification: S01/S03 supplied validation/error gap inputs and proof context, while S04 produced the behavior-specific public validation and compatibility-warning tests that closed COV-GAP-001.
  Source: S05-ASSESSMENT.md/Assessment
