---
id: M001-k9wo8b
title: "Audit and Stabilization Baseline"
status: complete
completed_at: 2026-05-20T04:25:52.510Z
key_decisions:
  - Keep the public API core centered on `archive_reader`, `validate_archive`, extraction helpers, `result<T>`, and family-specific writers; do not add a broad facade or speculative helper API without concrete consumer evidence.
  - Use generated legal fixtures, always-on CTest policy checks, and package-consumer runtime tests as default proof; keep local game archives and BSArchPro-derived comparisons advisory and opt-in.
  - Stabilize validation/error behavior through public `validate_archive`, `result<T>`, `validation_report`, and compatibility-warning tests rather than changing private parser internals or redesigning the public error model.
  - Retain closed gap identifiers such as COV-GAP-001 and COV-GAP-003 as former-gap traceability instead of deleting them after remediation.
key_files:
  - docs/coverage-audit-matrix.md
  - docs/compatibility-evidence.md
  - docs/public-api-reality-check.md
  - docs/integration-examples.md
  - docs/api-mainpage.md
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
  - tests/unit/docs_policy_tests.cpp
  - tests/unit/target_format_policy_tests.cpp
  - tests/unit/validation_api_tests.cpp
  - tests/package-consumer/main.cpp
  - tests/fixtures/generated/validate_fixture_manifests.py
  - test.cmd
  - grep.cmd
  - .gsd/milestones/M001-k9wo8b/M001-k9wo8b-LEARNINGS.md
lessons_learned:
  - The existing public API was richer than remembered; documentation, proof, and package-consumer evidence were higher-value than adding new facade surface in M001.
  - COV-GAP-001 was an evidence gap rather than a production validation-code bug; the current public result/report/warning model already supported the contract once behavior-specific tests were added.
  - Package-consumer proof can remain fixture-free by creating representative archives at runtime through the umbrella header and exported target, then reopening, validating, and extracting them through public APIs.
  - Optional release static/shared lanes can fail before libbsa configuration because of local vcpkg/Visual Studio detection; record that separately from default milestone completion gates.
  - Docs-policy tests are useful support-claim guardrails when runtime behavior is already present but public evidence is scattered.
---

# M001-k9wo8b: Audit and Stabilization Baseline

**M001 made libbsa's current support claims truthful by publishing a tested coverage matrix, proving the public API/package-consumer path, closing selected fixture/validation gaps, and recording remaining compatibility, warning, performance, API-polish, and release work as explicit deferrals.**

## What Happened

M001 converted libbsa's support claims into a verified, actionable baseline. S01 created the public coverage audit matrix for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10, established the `COV-GAP-*` vocabulary, and guarded the matrix with always-on docs-policy tests. S02 audited the actual public API surface and found that the existing umbrella header, reader, validation, extraction, result/error, and family-specific writer APIs formed a coherent dependency-light C++20 core; it therefore improved docs, package-consumer compile smoke, and policy tests instead of adding a speculative facade. S03 made scattered generated-fixture and writer round-trip proof public and durable through the compatibility evidence catalog and docs-policy guardrails. S04 closed the high-risk validation evidence gap by proving public `validate_archive`, `result<T>`, `validation_report`, and compatibility-warning behavior across generated success fixtures, writer-produced Starfield BA2 v3 compression routes, and mismatch warnings without changing the public error model. S05 integrated the previous slices, expanded package-consumer proof to create/open/validate/extract representative runtime archives through `<libbsa/libbsa.hpp>` and `libbsa::libbsa`, finalized the matrix with COV-GAP-001/COV-GAP-003 closed and COV-GAP-002/COV-GAP-004 deferred, and ran the default debug static build/test/package-consumer gate.

Fresh closeout verification resolved the validation freshness concern. The validation artifact was newer than all slice summaries/UAT/assessments and contained requirement coverage, cross-slice integration, verification-class compliance, and artifact-presence evidence. A new default CTest run passed 438/438 tests with only opt-in local game/BSArchPro comparison tests skipped, and a fresh TES5Edit status check reported no changes.

Decision re-evaluation: D001/D009/D010 (avoid broad facade in M001) matched shipped behavior and should be revisited only if M002 finds repeated consumer misuse; D002/D006/D008 (coverage matrix plus docs-policy guardrails) matched shipped behavior and should remain project practice; D003/D007 (layered default/advisory proof) matched shipped behavior and should remain until legally commit-safe real-corpus proof exists; D004 (keep `result<T>` and stabilize diagnostics without redesign) matched shipped behavior and should be revisited only if later error taxonomy work finds an API-level blocker; D005 (risk-bounded tranche) matched shipped behavior with COV-GAP-002/COV-GAP-004/R010/R012/R013 deferred deliberately.

## Success Criteria Results

- [x] **Truthful coverage/gap matrix exists for all current archive families and agreed axes.** S01 created `docs/coverage-audit-matrix.md`; S05 finalized fixed/deferred status; fresh CTest evidence includes `coverage_audit_matrix` 16 tests passing.
- [x] **Public API capability story is audited against real headers, docs, and package-consumer usage.** S02 added `docs/public-api-reality-check.md` and improved docs/policy/package-consumer smoke; S05 proved installed runtime archive flows through the umbrella public API.
- [x] **A risk-bounded tranche of highest-risk audit gaps is fixed with durable proof.** S03 locked generated-fixture/round-trip evidence into public docs and policy tests; S04 closed COV-GAP-001 with public validation/error/compatibility-warning tests; S05 preserved the proof in the integrated default suite.
- [x] **Remaining gaps are explicitly deferred with rationale and future ownership.** S05 deferred COV-GAP-002/R011, COV-GAP-004, R010, R012, and R013 with owners and rationale.
- [x] **Default build/test/package-consumer verification passes without local copyrighted fixtures.** Fresh closeout CTest passed 438/438 default tests with only the two opt-in local game/BSArchPro comparison tests skipped; package-consumer label passed 4 tests in the same run.
- [x] **TES5Edit remains untouched and read-only.** Fresh closeout `git status --short -- TES5Edit` evidence reported zero TES5Edit status lines, and policy tests preserve the boundary.

## Definition of Done Results

- **Duplicate guard:** `gsd_milestone_status` returned milestone status `active`; all five slices were already `complete` with task counts S01 2/2, S02 3/3, S03 2/2, S04 2/2, S05 3/3.
- **Code-change verification:** Initial branch self-diff found `HEAD` equals `main`, so closeout inspected milestone-scoped commit evidence. `gsd_exec` `bf9e0c60-8b8b-4327-a507-9402ff04719c` found 9 candidate M001/GSD-task commits, 8 production commits, and 33 non-GSD file touches across docs, tests, fixtures, package-consumer, and policy files.
- **Validation freshness:** `gsd_exec` `ff2dd54c-ce70-40ad-a24f-9d044e0237e3` confirmed the pass validation artifact exists, contains the required requirement/integration/verification sections, has no missing slice SUMMARY/UAT/ASSESSMENT artifacts, and no slice artifact is newer than validation.
- **Integrated verification:** Fresh closeout CTest run `gsd_exec` `68f36251-ed97-4c66-ab85-ded4fa49a46b` passed the default debug static suite: 100% tests passed, 0 failed out of 438, with only the two opt-in local game/BSArchPro comparison tests skipped. Summary `8b2232c6-87d5-4462-8036-447a5e87e178` also confirmed `package_consumer` 4 tests, `coverage_audit_matrix` 16, `docs_policy` 24, `target_format_policy` 7, `validation_api` 8, and `compatibility_warning` 4 tests in the passing run.
- **TES5Edit boundary:** Fresh `gsd_exec` `f2693b8a-9851-4735-9851-80193fc4cbb1` reported `tes5edit_status_line_count=0`.
- **Horizontal checklist:** No horizontal checklist was present in the M001 roadmap.

## Requirement Outcomes

- **R001-R009:** Already validated by slice closeouts and re-supported by fresh milestone evidence. Fresh closeout CTest `68f36251-ed97-4c66-ab85-ded4fa49a46b` passed 438/438 default tests; focused labels in the run included coverage matrix, docs policy, target-format policy, package-consumer, validation API, and compatibility-warning coverage. The validation artifact maps each requirement to slice evidence.
- **R010:** Deferred to M002/API polish. Evidence: D010 and S02/S05 show no broad facade was justified during M001; future API additions require concrete consumer evidence.
- **R011:** Deferred to future compatibility hardening. Evidence: S05 keeps game/BSArchPro corpus comparisons optional/advisory and default proof legal/reproducible.
- **R012:** Deferred to future performance/operational work. Evidence: S05-UAT marks large-archive stress readiness as not proven by M001.
- **R013:** Deferred to release readiness. Evidence: release static/shared package-consumer attempts failed before project configuration in local vcpkg/Visual Studio detection and were recorded as advisory environment limitations.
- **R014-R017:** Remain out of scope or platform boundary requirements; no milestone work added non-Bethesda formats, GUI/CLI product surface, in-place mutation, or cross-platform portability expansion.
- **Closeout-time requirement updates:** No additional status transitions were needed during milestone closeout because `.gsd/REQUIREMENTS.md` already reflected validated/deferred/out-of-scope statuses with supporting evidence.

## Deviations

No milestone-scope deviations blocked completion. S02 added small `test.cmd` and `grep.cmd` verification shims to satisfy Windows `cmd.exe` gate usage; S03 serialized focused CTest runs to avoid a Catch2 `PRE_TEST` discovery race; S05 recorded release static/shared package-consumer lane failures as advisory environment limitations because they failed inside vcpkg Visual Studio detection before libbsa configuration or tests.

## Follow-ups

Future compatibility hardening should own COV-GAP-002/R011 real game and BSArchPro corpus comparison. Future warning/diagnostics work should own COV-GAP-004. M002/API polish should revisit facade or helper ideas only with concrete consumer evidence. Future performance/operational work should add large-archive stress gates for R012. Release readiness should rerun static/shared package-consumer lanes in a healthy vcpkg/Visual Studio environment and decide publish criteria for R013.
