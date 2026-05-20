---
id: S05
parent: M001-k9wo8b
milestone: M001-k9wo8b
provides:
  - A final M001 support-truth matrix with fixed/deferred status.
  - Fresh default debug static build/test/package-consumer proof that does not require local copyrighted fixtures.
  - Validated package-consumer runtime proof for all current archive families through the installed public API.
  - Explicit future ownership for remaining advisory/compatibility/warning/performance/release gaps.
requires:
  - slice: S01
    provides: Initial coverage/gap matrix schema, family/axis rows, and ranked gap IDs.
  - slice: S02
    provides: Public API audit conclusions and package-consumer evidence requirements.
  - slice: S03
    provides: Generated fixture, writer, round-trip, and compatibility evidence for selected fixed gaps.
  - slice: S04
    provides: Stabilized public result/error/validation and compatibility-warning proof.
affects:
  - Future compatibility hardening for COV-GAP-002/R011.
  - Future warning taxonomy work for COV-GAP-004.
  - Future performance and release-readiness milestones R012/R013.
  - Future API polish R010/M002.
key_files:
  - tests/package-consumer/main.cpp
  - docs/coverage-audit-matrix.md
  - docs/public-api-reality-check.md
  - docs/compatibility-evidence.md
  - docs/integration-examples.md
  - docs/api-mainpage.md
  - tests/unit/docs_policy_tests.cpp
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
  - tests/unit/target_format_policy_tests.cpp
  - .gsd/REQUIREMENTS.md
key_decisions:
  - Kept package-consumer proof fixture-free and umbrella-header-only while still creating and validating representative runtime archives for every current archive family.
  - Preserved COV-GAP-003 as a closed historical/former gap instead of deleting the identifier, so future readers can trace why it no longer appears as an open gap.
  - Kept real game/BSArchPro corpus evidence advisory rather than a default completion gate.
  - Treated release static/shared package-consumer failures as advisory environment limitations because they failed in vcpkg before libbsa configuration or test execution.
patterns_established:
  - Installed package-consumer runtime proof should create writer-produced archives at runtime through `<libbsa/libbsa.hpp>` and `libbsa::libbsa`, not consume internal fixtures.
  - Coverage-matrix docs should retain closed gap IDs in former-gap language for traceability and use policy tests to reject stale open-gap wording.
  - Docs-policy and target-format policy tests now act as support-claim guardrails for default-vs-optional evidence, dependency-light headers, package-consumer coverage, and TES5Edit boundaries.
observability_surfaces:
  - Full CTest failures and label-specific CTest failures identify regressions by capability area.
  - `package_consumer_smoke` failures expose installed-target runtime regressions.
  - Docs-policy and coverage-matrix tests expose stale or overclaimed support documentation.
  - The final coverage matrix exposes remaining non-green capability cells through gap IDs and future ownership.
drill_down_paths:
  - .gsd/milestones/M001-k9wo8b/slices/S05/tasks/T01-SUMMARY.md
  - .gsd/milestones/M001-k9wo8b/slices/S05/tasks/T02-SUMMARY.md
  - .gsd/milestones/M001-k9wo8b/slices/S05/tasks/T03-SUMMARY.md
  - .gsd/exec/dd048d7e-b313-49fb-a513-91437f8ef731.stdout
  - .gsd/exec/5feeb285-70a3-4a07-8f84-5187c2e7037a.stdout
  - .gsd/exec/dab7acc3-c02a-4244-a622-df2df2944624.stdout
duration: ""
verification_result: passed
completed_at: 2026-05-20T04:02:23.505Z
blocker_discovered: false
---

# S05: S05

**Finalized M001's integrated confidence pass by closing the installed package-consumer runtime proof gap, locking the final coverage-matrix status, and re-verifying the default build/test/package-consumer gate.**

## What Happened

S05 consumed the audit matrix from S01, the public API reality check from S02, the generated-fixture and round-trip proof from S03, and the error/validation stabilization from S04. T01 expanded `tests/package-consumer/main.cpp` from a compile/link smoke into a fixture-free installed-package runtime proof: the consumer includes only `<libbsa/libbsa.hpp>`, links the exported `libbsa::libbsa` target, creates writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives at runtime, reopens them through `archive_reader`, validates them through `validate_archive`, and extracts payloads through public extraction/sink paths. T02 finalized the public evidence story in `docs/coverage-audit-matrix.md` and companion docs: COV-GAP-001 and COV-GAP-003 are closed/former gaps, while only COV-GAP-002 (real game/BSArchPro corpus comparison) and COV-GAP-004 (full warning taxonomy) remain explicitly deferred with rationale and likely future ownership. T02 also updated docs-policy, coverage-matrix, and target-format policy tests so stale support claims, hidden local fixture dependencies, TES5Edit coupling, or package-consumer regressions fail visibly. T03 ran the integrated default gate and attempted the optional release package-proof lanes; the mandatory debug static gate passed, while release static/shared stopped before libbsa configuration due a local vcpkg Visual Studio detection internal error and were recorded as advisory environment limitations. During closeout, active requirements R003, R007, R008, and R009 were marked validated, and R001/R005 validation notes were refreshed to reflect the final matrix/package-consumer proof.

## Verification

Fresh closeout verification used gsd_exec, not direct shell verification. `dd048d7e-b313-49fb-a513-91437f8ef731` passed `cmake --preset windows-msvc-debug-static`, `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`, full default `ctest --preset windows-msvc-debug-static --output-on-failure` (`438/438` tests passed, with only the two opt-in local game/BSArchPro comparison tests skipped), and `ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure` (`4/4` tests passed, including `package_consumer_smoke` and runtime DLL copy). `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed the focused policy labels: coverage_audit_matrix (`16/16`), docs_policy (`24/24`), and target_format_policy (`7/7`). Prior T03 evidence `c93e7807-2f93-480e-956b-a72a386372ad` recorded `git status --short -- TES5Edit` exit 0 with no TES5Edit status lines; because this closeout lane was instructed not to run git commands, closeout refreshed the boundary with non-git gsd_exec `dab7acc3-c02a-4244-a622-df2df2944624`, which found `changed_files_after_prior_git_status=0` under `TES5Edit`. Optional release static/shared package-consumer attempts were already recorded in `e87bb418-9197-4fe4-bab4-fd125465e5e7` and `4c7a60f7-8c46-417e-b8f0-8c2c301c6f99`; both failed before project configuration during vcpkg install with `visualstudio.cpp(90): Value was null`, so they remain advisory environment-limit evidence rather than default completion blockers.

## Requirements Advanced

- R001 — Finalized the matrix with closed/deferred gap status and policy tests.
- R003 — Finalized the public API story with package-consumer runtime evidence and no major facade redesign.
- R005 — Added installed package-consumer runtime proof to the layered default/advisory evidence model.
- R007 — Implemented and verified installed/exported target runtime proof across all current archive families.
- R008 — Kept dependency-light public header and C++20 boundaries covered by docs/static-boundary tests.
- R009 — Preserved the TES5Edit read-only boundary and refreshed evidence without mutating the submodule.

## Requirements Validated

- R001 — Focused coverage_audit_matrix label passed 16/16 and final docs close COV-GAP-001/COV-GAP-003 while deferring only COV-GAP-002/COV-GAP-004.
- R002 — Full default CTest passed 438/438 and package_consumer_smoke covers TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 runtime archive paths.
- R003 — Public API reality-check docs and policy labels passed; S05 closes package-consumer proof without API redesign.
- R004 — Full default CTest passed after the S03/S04 remediation tranche, preserving fixed gap proof.
- R005 — Full default CTest passed with only opt-in local corpus comparisons skipped; package_consumer label passed 4/4 and docs-policy preserves advisory boundaries.
- R006 — Full default CTest passed after S04 validation/error stabilization; validation and compatibility warning policy coverage remains in the suite.
- R007 — Package-consumer label passed 4/4, including installed runtime archive creation/open/validate/extract smoke and runtime DLL copy.
- R008 — Full CTest plus docs_policy and target_format_policy labels passed, including public API/static-boundary checks.
- R009 — Prior T03 git-status evidence showed no TES5Edit status lines, and closeout non-git check found zero files changed after that status evidence.

## New Requirements Surfaced

- None.

## Requirements Invalidated or Re-scoped

None.

## Operational Readiness

None.

## Deviations

Optional release static and release shared package-consumer lanes were attempted but failed before libbsa project configuration during vcpkg manifest install with `visualstudio.cpp(90): Value was null`; per the slice plan, these are recorded as advisory environment limitations rather than default completion blockers. The closer did not rerun `git status` because the closeout instruction prohibited git commands; it reused T03 git-status evidence and added a non-git freshness check for TES5Edit modifications.

## Known Limitations

COV-GAP-002 real game/BSArchPro corpus comparisons remain deferred to a future compatibility hardening milestone. COV-GAP-004 full compatibility warning taxonomy remains deferred. Optional release package-proof lanes need a healthy vcpkg/Visual Studio detection environment or CI to complete. No production logging/metrics were added; diagnostics are through CTest, package-consumer failures, docs-policy failures, target-format policy failures, and explicit matrix gap IDs.

## Follow-ups

Future compatibility hardening should own COV-GAP-002 and R011. A future warning/diagnostics slice should own COV-GAP-004. A future performance/operational milestone should own large-archive stress gates (R012). A release-readiness milestone should rerun release static/shared package-proof lanes in a healthy environment and decide publish criteria (R013). M002/API polish may revisit ergonomic facade ideas only with concrete consumer evidence (R010).

## Files Created/Modified

- `tests/package-consumer/main.cpp` — Expanded installed package smoke to create, reopen, validate, and extract representative archives for all current families through the umbrella public API.
- `docs/coverage-audit-matrix.md` — Finalized M001 support-truth statuses: COV-GAP-001/COV-GAP-003 closed and COV-GAP-002/COV-GAP-004 deferred.
- `docs/public-api-reality-check.md` — Updated public API audit conclusion to include closed installed-package runtime proof and no major redesign.
- `docs/compatibility-evidence.md` — Aligned default package-consumer proof with optional real-game/BSArchPro advisory evidence.
- `docs/integration-examples.md` — Documented the package-consumer journey and runtime proof expectations.
- `docs/api-mainpage.md` — Linked the audited API proof and consumer-facing support story.
- `tests/unit/docs_policy_tests.cpp` — Added/updated policy checks for docs evidence boundaries and package-consumer proof.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — Added/updated policy checks for final matrix statuses, closed/former gaps, and installed package-consumer proof.
- `tests/unit/target_format_policy_tests.cpp` — Added/updated target-format policy checks for package-consumer, optional evidence, and TES5Edit boundaries.
- `.gsd/REQUIREMENTS.md` — Regenerated by requirement updates marking R003/R007/R008/R009 validated and refreshing R001/R005 validation notes.
