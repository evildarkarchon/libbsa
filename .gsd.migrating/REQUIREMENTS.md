# Requirements

This file is the explicit capability and coverage contract for the project.

## Active

## Validated

### R001 — Truthful coverage matrix: M001 must produce a durable coverage/gap matrix that states what libbsa currently proves and where gaps remain.
- Class: failure-visibility
- Status: validated
- Description: Truthful coverage matrix: M001 must produce a durable coverage/gap matrix that states what libbsa currently proves and where gaps remain.
- Why it matters: The project cannot safely stabilize or improve API claims unless current coverage is visible and truthful.
- Source: user
- Primary owning slice: S01
- Supporting slices: S03,S04
- Validation: S01 produced `docs/coverage-audit-matrix.md`; S05 finalized it with fixed/deferred status and package-consumer runtime evidence. Fresh S05 closeout verification gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed `ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure` (`16/16`), `docs_policy` (`24/24`), and `target_format_policy` (`7/7`), including tests that keep COV-GAP-001/COV-GAP-003 closed and COV-GAP-002/COV-GAP-004 deferred.
- Notes: Mapped in M001 roadmap: S01 creates the public coverage and gap matrix; S03 and S04 update fixed, deferred, and final statuses after remediation and verification.

### R002 — All current archive families audited: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 must all be included in the audit scope.
- Class: core-capability
- Status: validated
- Description: All current archive families audited: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 must all be included in the audit scope.
- Why it matters: The user explicitly wants all currently implemented archive families considered, not a subset.
- Source: user
- Primary owning slice: S01
- Supporting slices: S04
- Validation: `docs/coverage-audit-matrix.md` includes TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 rows with required capability axes. Fresh S01 closeout verification passed required family greps and the `coverage_audit_matrix lists every current archive family` Catch2/CTest policy case.
- Notes: Mapped in M001 roadmap: S01 audits TES3 BSA, TES4 family BSA, BA2 GNRL, and BA2 DX10 across all matrix axes; S04 rechecks final evidence.

### R003 — Public API capability story audited: M001 must verify what the public API actually supports beyond basic extraction and writing, and identify API friction or missing ergonomic helpers.
- Class: core-capability
- Status: validated
- Description: Public API capability story audited: M001 must verify what the public API actually supports beyond basic extraction and writing, and identify API friction or missing ergonomic helpers.
- Why it matters: The current API may already expose more than remembered, but it needs a clearer, proper consumer-facing story.
- Source: user
- Primary owning slice: S02
- Supporting slices: S03,S04
- Validation: S02 audited the public API capability story and S05 finalized it in `docs/public-api-reality-check.md`, `docs/api-mainpage.md`, and policy tests. Fresh S05 closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed configure/build/full CTest/package-consumer (`438/438` default tests and `4/4` package-consumer tests), and gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed focused `coverage_audit_matrix`, `docs_policy`, and `target_format_policy` labels.
- Notes: Mapped in M001 roadmap: S02 audits the public API capability story and identifies evidence backed friction or helper needs; S03 and S04 prove any accepted changes.

### R004 — Highest-risk audit gaps fixed with proof: M001 must fix a risk-bounded tranche of the most important gaps discovered by the audit and prove each fix durably.
- Class: quality-attribute
- Status: validated
- Description: Highest-risk audit gaps fixed with proof: M001 must fix a risk-bounded tranche of the most important gaps discovered by the audit and prove each fix durably.
- Why it matters: The milestone should produce concrete stabilization progress without ballooning into every possible remediation.
- Source: user
- Primary owning slice: S03
- Supporting slices: S01,S02,S04
- Validation: S03 closed the selected high claim-risk fixture/round-trip tranche by documenting the default legal generated-fixture proof sweep in `docs/compatibility-evidence.md`, adding docs-policy Catch2 guardrails in `tests/unit/coverage_audit_matrix_docs_tests.cpp`, regenerating the default synthetic fixture targets, and passing the focused closeout verification sweep in gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` (15/15 checks including coverage_audit_matrix, reader_backend_dispatch, TES3/TES4/BA2 GNRL/BA2 DX10 writer labels, validation_api, and validate_fixture_manifests).
- Notes: Mapped in M001 roadmap: S03 selects and fixes a risk bounded tranche of the highest claim risk gaps from S01 and S02 with durable proof.

### R005 — Layered fixture and compatibility proof preserved: M001 must preserve generated fixture CI proof, package-consumer proof, and documented opt-in game/BSArchPro comparison paths.
- Class: quality-attribute
- Status: validated
- Description: Layered fixture and compatibility proof preserved: M001 must preserve generated fixture CI proof, package-consumer proof, and documented opt-in game/BSArchPro comparison paths.
- Why it matters: libbsa needs repeatable proof while still keeping a path to higher-confidence real-world compatibility evidence.
- Source: user
- Primary owning slice: S04
- Supporting slices: S01,S03
- Validation: S03 preserved the layered generated-fixture and compatibility-proof model; S05 added installed-package runtime archive proof and kept optional local evidence boundaries documented. Fresh S05 closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed full default CTest (`438/438`, with only the two opt-in local fixture/BSArchPro comparison tests skipped) and package-consumer label (`4/4`). Focused docs-policy verification gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed coverage, docs, and target-format policy labels that preserve the default-versus-optional evidence story.
- Notes: Mapped in M001 roadmap: S04 proves layered generated fixture, package consumer, and optional compatibility paths remain legally safe and non blocking; S01 and S03 maintain evidence references.

### R006 — Structured public error behavior remains consistent: M001 must audit and stabilize public result/error behavior where high-risk inconsistencies are found.
- Class: failure-visibility
- Status: validated
- Description: Structured public error behavior remains consistent: M001 must audit and stabilize public result/error behavior where high-risk inconsistencies are found.
- Why it matters: Archive libraries need reliable diagnostic behavior for malformed data, I/O failures, and compatibility issues.
- Source: inferred
- Primary owning slice: S02
- Supporting slices: S03
- Validation: S04 stabilized high-risk public result/error/validation behavior through public API and docs-policy proof. Fresh closeout verification gsd_exec `b97534ad-8a07-4c3b-86fc-6b1a2fba4577` passed `cmake --preset windows-msvc-debug-static`, `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`, `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure` (11/11), `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure` (8/8), and `ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure` (5/5).
- Notes: Mapped in M001 roadmap: S02 audits structured public result, error, and validation behavior; S03 stabilizes high risk inconsistencies found by that audit.

### R007 — Package-consumer API remains installable and usable: M001 must prove the installed/exported public API can be consumed through the package target and umbrella header.
- Class: integration
- Status: validated
- Description: Package-consumer API remains installable and usable: M001 must prove the installed/exported public API can be consumed through the package target and umbrella header.
- Why it matters: A reusable library is not stable if it only works internally and cannot be consumed as installed.
- Source: inferred
- Primary owning slice: S04
- Supporting slices: S02
- Validation: S05 extended `tests/package-consumer/main.cpp` so an installed consumer including only `<libbsa/libbsa.hpp>` and linking `libbsa::libbsa` creates writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives, reopens them through `archive_reader`, validates them through `validate_archive`, and extracts payloads. Fresh closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed `ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure` (`4/4` tests including `package_consumer_smoke` and runtime DLL copy).
- Notes: Mapped in M001 roadmap: S04 verifies installed and exported package consumer usage; S02 audits the public API story that package consumers exercise.

### R008 — Public headers remain dependency-light C++20: M001 changes must not expose private codec, DirectXTex, platform, or C++23-only types in public headers.
- Class: constraint
- Status: validated
- Description: Public headers remain dependency-light C++20: M001 changes must not expose private codec, DirectXTex, platform, or C++23-only types in public headers.
- Why it matters: Consumers need a stable C++20 API without inheriting implementation dependencies or newer language requirements.
- Source: inferred
- Primary owning slice: S02
- Supporting slices: S04
- Validation: S05 preserved the dependency-light public header story in docs and policy tests. Fresh closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed full default CTest (`438/438`), including public API/static-boundary tests, and gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed focused `docs_policy` (`24/24`) and `target_format_policy` (`7/7`) labels.
- Notes: Mapped in M001 roadmap: S02 audits dependency light C++20 public headers and API boundaries; S04 confirms final package consumer and policy proof.

### R009 — TES5Edit remains read-only reference only: M001 must not edit, format, stage, compile, vendor, or use TES5Edit as mutable fixture data.
- Class: constraint
- Status: validated
- Description: TES5Edit remains read-only reference only: M001 must not edit, format, stage, compile, vendor, or use TES5Edit as mutable fixture data.
- Why it matters: TES5Edit is behavioral reference/prior art only; modifying it violates the project boundary.
- Source: user
- Primary owning slice: S04
- Supporting slices: S01,S02,S03
- Validation: T03 recorded `git status --short -- TES5Edit` exit 0 with no TES5Edit status lines in gsd_exec `c93e7807-2f93-480e-956b-a72a386372ad`. The S05 closer did not run git commands; instead it reused that prior evidence and ran non-git freshness check gsd_exec `dab7acc3-c02a-4244-a622-df2df2944624`, which found `changed_files_after_prior_git_status=0` under `TES5Edit`.
- Notes: Mapped in M001 roadmap: S04 verifies TES5Edit remains read only, with every slice constrained to avoid editing, formatting, staging, compiling, or using it as mutable fixture data.

## Deferred

### R010 — Full ergonomic facade or major API redesign is deferred beyond M001 unless audit evidence proves a small helper is immediately needed.
- Class: differentiator
- Status: deferred
- Description: Full ergonomic facade or major API redesign is deferred beyond M001 unless audit evidence proves a small helper is immediately needed.
- Why it matters: A full facade may improve ergonomics later, but M001 should avoid redesigning without consumer evidence.
- Source: inferred
- Primary owning slice: M002 provisional
- Supporting slices: none
- Validation: unmapped
- Notes: Provisional future owner: M002 public API polish.

### R011 — Exhaustive BSArchPro/game-corpus compatibility campaign is deferred beyond M001.
- Class: quality-attribute
- Status: deferred
- Description: Exhaustive BSArchPro/game-corpus compatibility campaign is deferred beyond M001.
- Why it matters: Full compatibility campaigning is valuable but too environment-dependent for the first audit-and-stabilize milestone.
- Source: inferred
- Primary owning slice: M003 provisional
- Supporting slices: none
- Validation: unmapped
- Notes: Provisional future owner: M003 compatibility hardening. M001 keeps the optional path documented and may use it if available, but default completion cannot depend on local copyrighted fixtures.

### R012 — Performance and large-archive stress gates are deferred beyond M001.
- Class: quality-attribute
- Status: deferred
- Description: Performance and large-archive stress gates are deferred beyond M001.
- Why it matters: Large-archive throughput and stress proof are important later, but M001 is about coverage truth and targeted stabilization.
- Source: inferred
- Primary owning slice: M004 provisional
- Supporting slices: none
- Validation: unmapped
- Notes: Provisional future owner: M004 performance and operational polish. M001 should not regress existing benchmark/build hooks but does not need full stress criteria.

### R013 — Publish/release readiness contract is deferred beyond M001.
- Class: launchability
- Status: deferred
- Description: Publish/release readiness contract is deferred beyond M001.
- Why it matters: Release readiness depends on later API polish, compatibility hardening, and performance/operational proof.
- Source: inferred
- Primary owning slice: M005 provisional
- Supporting slices: none
- Validation: unmapped
- Notes: Provisional future owner: M005 release readiness. M001 may improve docs and package-consumer proof but does not need to declare the library ready for release.

## Out of Scope

### R014 — New non-Bethesda archive formats are out of scope.
- Class: anti-feature
- Status: out-of-scope
- Description: New non-Bethesda archive formats are out of scope.
- Why it matters: The project purpose is Bethesda archive compatibility, and non-Bethesda formats would dilute scope.
- Source: user
- Primary owning slice: none
- Supporting slices: none
- Validation: n/a
- Notes: Do not add ZIP, 7z, libarchive, or other non-Bethesda archive behavior as part of this project/milestone.

### R015 — GUI or CLI application surface is out of scope.
- Class: anti-feature
- Status: out-of-scope
- Description: GUI or CLI application surface is out of scope.
- Why it matters: libbsa is a reusable library, not a GUI or CLI application.
- Source: user
- Primary owning slice: none
- Supporting slices: none
- Validation: n/a
- Notes: Consumers can build tools on top of libbsa; M001 should not create an app-specific UI or command-line product.

### R016 — In-place archive mutation is out of scope for M001.
- Class: anti-feature
- Status: out-of-scope
- Description: In-place archive mutation is out of scope for M001.
- Why it matters: In-place mutation is high-risk and unrelated to the immediate coverage/API stabilization goal.
- Source: inferred
- Primary owning slice: none
- Supporting slices: none
- Validation: n/a
- Notes: Open/read/write-new flows remain acceptable. In-place update should not be planned in the audit-and-stabilize milestone.

### R017 — Cross-platform portability expansion is out of scope.
- Class: constraint
- Status: out-of-scope
- Description: Cross-platform portability expansion is out of scope.
- Why it matters: The project is explicitly Windows-only with MSVC and vcpkg as the target development environment.
- Source: project constraint
- Primary owning slice: none
- Supporting slices: none
- Validation: n/a
- Notes: Do not add Linux, macOS, POSIX, or general cross-platform requirements unless the user explicitly reopens platform support.

## Traceability

| ID | Class | Status | Primary owner | Supporting | Proof |
|---|---|---|---|---|---|
| R001 | failure-visibility | validated | S01 | S03,S04 | S01 produced `docs/coverage-audit-matrix.md`; S05 finalized it with fixed/deferred status and package-consumer runtime evidence. Fresh S05 closeout verification gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed `ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure` (`16/16`), `docs_policy` (`24/24`), and `target_format_policy` (`7/7`), including tests that keep COV-GAP-001/COV-GAP-003 closed and COV-GAP-002/COV-GAP-004 deferred. |
| R002 | core-capability | validated | S01 | S04 | `docs/coverage-audit-matrix.md` includes TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 rows with required capability axes. Fresh S01 closeout verification passed required family greps and the `coverage_audit_matrix lists every current archive family` Catch2/CTest policy case. |
| R003 | core-capability | validated | S02 | S03,S04 | S02 audited the public API capability story and S05 finalized it in `docs/public-api-reality-check.md`, `docs/api-mainpage.md`, and policy tests. Fresh S05 closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed configure/build/full CTest/package-consumer (`438/438` default tests and `4/4` package-consumer tests), and gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed focused `coverage_audit_matrix`, `docs_policy`, and `target_format_policy` labels. |
| R004 | quality-attribute | validated | S03 | S01,S02,S04 | S03 closed the selected high claim-risk fixture/round-trip tranche by documenting the default legal generated-fixture proof sweep in `docs/compatibility-evidence.md`, adding docs-policy Catch2 guardrails in `tests/unit/coverage_audit_matrix_docs_tests.cpp`, regenerating the default synthetic fixture targets, and passing the focused closeout verification sweep in gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` (15/15 checks including coverage_audit_matrix, reader_backend_dispatch, TES3/TES4/BA2 GNRL/BA2 DX10 writer labels, validation_api, and validate_fixture_manifests). |
| R005 | quality-attribute | validated | S04 | S01,S03 | S03 preserved the layered generated-fixture and compatibility-proof model; S05 added installed-package runtime archive proof and kept optional local evidence boundaries documented. Fresh S05 closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed full default CTest (`438/438`, with only the two opt-in local fixture/BSArchPro comparison tests skipped) and package-consumer label (`4/4`). Focused docs-policy verification gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed coverage, docs, and target-format policy labels that preserve the default-versus-optional evidence story. |
| R006 | failure-visibility | validated | S02 | S03 | S04 stabilized high-risk public result/error/validation behavior through public API and docs-policy proof. Fresh closeout verification gsd_exec `b97534ad-8a07-4c3b-86fc-6b1a2fba4577` passed `cmake --preset windows-msvc-debug-static`, `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`, `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure` (11/11), `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure` (8/8), and `ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure` (5/5). |
| R007 | integration | validated | S04 | S02 | S05 extended `tests/package-consumer/main.cpp` so an installed consumer including only `<libbsa/libbsa.hpp>` and linking `libbsa::libbsa` creates writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives, reopens them through `archive_reader`, validates them through `validate_archive`, and extracts payloads. Fresh closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed `ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure` (`4/4` tests including `package_consumer_smoke` and runtime DLL copy). |
| R008 | constraint | validated | S02 | S04 | S05 preserved the dependency-light public header story in docs and policy tests. Fresh closeout verification gsd_exec `dd048d7e-b313-49fb-a513-91437f8ef731` passed full default CTest (`438/438`), including public API/static-boundary tests, and gsd_exec `5feeb285-70a3-4a07-8f84-5187c2e7037a` passed focused `docs_policy` (`24/24`) and `target_format_policy` (`7/7`) labels. |
| R009 | constraint | validated | S04 | S01,S02,S03 | T03 recorded `git status --short -- TES5Edit` exit 0 with no TES5Edit status lines in gsd_exec `c93e7807-2f93-480e-956b-a72a386372ad`. The S05 closer did not run git commands; instead it reused that prior evidence and ran non-git freshness check gsd_exec `dab7acc3-c02a-4244-a622-df2df2944624`, which found `changed_files_after_prior_git_status=0` under `TES5Edit`. |
| R010 | differentiator | deferred | M002 provisional | none | unmapped |
| R011 | quality-attribute | deferred | M003 provisional | none | unmapped |
| R012 | quality-attribute | deferred | M004 provisional | none | unmapped |
| R013 | launchability | deferred | M005 provisional | none | unmapped |
| R014 | anti-feature | out-of-scope | none | none | n/a |
| R015 | anti-feature | out-of-scope | none | none | n/a |
| R016 | anti-feature | out-of-scope | none | none | n/a |
| R017 | constraint | out-of-scope | none | none | n/a |

## Coverage Summary

- Active requirements: 0
- Mapped to slices: 0
- Validated: 9 (R001, R002, R003, R004, R005, R006, R007, R008, R009)
- Unmapped active requirements: 0
