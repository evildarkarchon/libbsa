# Requirements

This file is the explicit capability and coverage contract for the project.

## Active

### R003 — Public API capability story audited: M001 must verify what the public API actually supports beyond basic extraction and writing, and identify API friction or missing ergonomic helpers.
- Class: core-capability
- Status: active
- Description: Public API capability story audited: M001 must verify what the public API actually supports beyond basic extraction and writing, and identify API friction or missing ergonomic helpers.
- Why it matters: The current API may already expose more than remembered, but it needs a clearer, proper consumer-facing story.
- Source: user
- Primary owning slice: M001-k9wo8b/S02
- Supporting slices: M001-k9wo8b/S05
- Validation: mapped
- Notes: Keep archive_reader and family-specific writers as the core; add helpers only when audit evidence justifies them.

### R006 — Structured public error behavior remains consistent: M001 must audit and stabilize public result/error behavior where high-risk inconsistencies are found.
- Class: failure-visibility
- Status: active
- Description: Structured public error behavior remains consistent: M001 must audit and stabilize public result/error behavior where high-risk inconsistencies are found.
- Why it matters: Archive libraries need reliable diagnostic behavior for malformed data, I/O failures, and compatibility issues.
- Source: inferred
- Primary owning slice: M001-k9wo8b/S04
- Supporting slices: M001-k9wo8b/S01, M001-k9wo8b/S03
- Validation: mapped
- Notes: Keep libbsa::result<T>, stable error_code categories, and human-readable diagnostic messages as the public failure model.

### R007 — Package-consumer API remains installable and usable: M001 must prove the installed/exported public API can be consumed through the package target and umbrella header.
- Class: integration
- Status: active
- Description: Package-consumer API remains installable and usable: M001 must prove the installed/exported public API can be consumed through the package target and umbrella header.
- Why it matters: A reusable library is not stable if it only works internally and cannot be consumed as installed.
- Source: inferred
- Primary owning slice: M001-k9wo8b/S05
- Supporting slices: M001-k9wo8b/S02
- Validation: mapped
- Notes: Package-consumer proof should cover representative read, write, validation, bulk extraction, and error handling examples where feasible.

### R008 — Public headers remain dependency-light C++20: M001 changes must not expose private codec, DirectXTex, platform, or C++23-only types in public headers.
- Class: constraint
- Status: active
- Description: Public headers remain dependency-light C++20: M001 changes must not expose private codec, DirectXTex, platform, or C++23-only types in public headers.
- Why it matters: Consumers need a stable C++20 API without inheriting implementation dependencies or newer language requirements.
- Source: inferred
- Primary owning slice: M001-k9wo8b/S02
- Supporting slices: M001-k9wo8b/S05
- Validation: mapped
- Notes: Public headers should continue to use libbsa-owned metadata/result types and avoid leaking libdeflate, LZ4, DirectXTex, DXGI, or std::expected.

### R009 — TES5Edit remains read-only reference only: M001 must not edit, format, stage, compile, vendor, or use TES5Edit as mutable fixture data.
- Class: constraint
- Status: active
- Description: TES5Edit remains read-only reference only: M001 must not edit, format, stage, compile, vendor, or use TES5Edit as mutable fixture data.
- Why it matters: TES5Edit is behavioral reference/prior art only; modifying it violates the project boundary.
- Source: user
- Primary owning slice: M001-k9wo8b/S01
- Supporting slices: M001-k9wo8b/S02, M001-k9wo8b/S03, M001-k9wo8b/S04, M001-k9wo8b/S05
- Validation: mapped
- Notes: All implementation, tests, docs, and generated artifacts must live outside TES5Edit/.

## Validated

### R001 — Truthful coverage matrix: M001 must produce a durable coverage/gap matrix that states what libbsa currently proves and where gaps remain.
- Class: failure-visibility
- Status: validated
- Description: Truthful coverage matrix: M001 must produce a durable coverage/gap matrix that states what libbsa currently proves and where gaps remain.
- Why it matters: The project cannot safely stabilize or improve API claims unless current coverage is visible and truthful.
- Source: user
- Primary owning slice: M001-k9wo8b/S01
- Supporting slices: M001-k9wo8b/S02, M001-k9wo8b/S03, M001-k9wo8b/S04, M001-k9wo8b/S05
- Validation: S01 produced `docs/coverage-audit-matrix.md`, a durable human-first support-truth matrix with default evidence sources, family/axis statuses, advisory local-evidence separation, and ranked gaps `COV-GAP-001` through `COV-GAP-004`. Fresh S01 closeout verification passed docs smoke checks plus `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure` (7/7 tests).
- Notes: Validated by M001-k9wo8b/S01 closeout; future slices may update the matrix as gaps are fixed or deferred.

### R002 — All current archive families audited: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 must all be included in the audit scope.
- Class: core-capability
- Status: validated
- Description: All current archive families audited: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 must all be included in the audit scope.
- Why it matters: The user explicitly wants all currently implemented archive families considered, not a subset.
- Source: user
- Primary owning slice: M001-k9wo8b/S01
- Supporting slices: M001-k9wo8b/S03, M001-k9wo8b/S04
- Validation: `docs/coverage-audit-matrix.md` includes TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 rows with required capability axes. Fresh S01 closeout verification passed required family greps and the `coverage_audit_matrix lists every current archive family` Catch2/CTest policy case.
- Notes: Validated by M001-k9wo8b/S01 closeout; all required current archive families are represented in the public matrix and protected by docs-policy tests.

### R004 — Highest-risk audit gaps fixed with proof: M001 must fix a risk-bounded tranche of the most important gaps discovered by the audit and prove each fix durably.
- Class: quality-attribute
- Status: validated
- Description: Highest-risk audit gaps fixed with proof: M001 must fix a risk-bounded tranche of the most important gaps discovered by the audit and prove each fix durably.
- Why it matters: The milestone should produce concrete stabilization progress without ballooning into every possible remediation.
- Source: user
- Primary owning slice: M001-k9wo8b/S03
- Supporting slices: M001-k9wo8b/S04
- Validation: S03 closed the selected high claim-risk fixture/round-trip tranche by documenting the default legal generated-fixture proof sweep in `docs/compatibility-evidence.md`, adding docs-policy Catch2 guardrails in `tests/unit/coverage_audit_matrix_docs_tests.cpp`, regenerating the default synthetic fixture targets, and passing the focused closeout verification sweep in gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` (15/15 checks including coverage_audit_matrix, reader_backend_dispatch, TES3/TES4/BA2 GNRL/BA2 DX10 writer labels, validation_api, and validate_fixture_manifests).
- Notes: COV-GAP-001 remains intentionally routed to S04 for validation-success granularity; S03 did not broaden scope into unrelated implementation or public API redesign work.

### R005 — Layered fixture and compatibility proof preserved: M001 must preserve generated fixture CI proof, package-consumer proof, and documented opt-in game/BSArchPro comparison paths.
- Class: quality-attribute
- Status: validated
- Description: Layered fixture and compatibility proof preserved: M001 must preserve generated fixture CI proof, package-consumer proof, and documented opt-in game/BSArchPro comparison paths.
- Why it matters: libbsa needs repeatable proof while still keeping a path to higher-confidence real-world compatibility evidence.
- Source: user
- Primary owning slice: M001-k9wo8b/S03
- Supporting slices: M001-k9wo8b/S05
- Validation: S03 preserved layered fixture and compatibility proof by adding the public `Default fixture and round-trip proof sweep` section to `docs/compatibility-evidence.md`, enforcing coverage-matrix/default-proof wording through Catch2 docs-policy tests, and passing closeout verification gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` across generated fixture targets, writer round-trip/reopen labels for all four current archive families, reader dispatch, validation API, and manifest validation.
- Notes: Optional `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` paths remain advisory only and are explicitly separate from default proof.

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
| R001 | failure-visibility | validated | M001-k9wo8b/S01 | M001-k9wo8b/S02, M001-k9wo8b/S03, M001-k9wo8b/S04, M001-k9wo8b/S05 | S01 produced `docs/coverage-audit-matrix.md`, a durable human-first support-truth matrix with default evidence sources, family/axis statuses, advisory local-evidence separation, and ranked gaps `COV-GAP-001` through `COV-GAP-004`. Fresh S01 closeout verification passed docs smoke checks plus `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure` (7/7 tests). |
| R002 | core-capability | validated | M001-k9wo8b/S01 | M001-k9wo8b/S03, M001-k9wo8b/S04 | `docs/coverage-audit-matrix.md` includes TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 rows with required capability axes. Fresh S01 closeout verification passed required family greps and the `coverage_audit_matrix lists every current archive family` Catch2/CTest policy case. |
| R003 | core-capability | active | M001-k9wo8b/S02 | M001-k9wo8b/S05 | mapped |
| R004 | quality-attribute | validated | M001-k9wo8b/S03 | M001-k9wo8b/S04 | S03 closed the selected high claim-risk fixture/round-trip tranche by documenting the default legal generated-fixture proof sweep in `docs/compatibility-evidence.md`, adding docs-policy Catch2 guardrails in `tests/unit/coverage_audit_matrix_docs_tests.cpp`, regenerating the default synthetic fixture targets, and passing the focused closeout verification sweep in gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` (15/15 checks including coverage_audit_matrix, reader_backend_dispatch, TES3/TES4/BA2 GNRL/BA2 DX10 writer labels, validation_api, and validate_fixture_manifests). |
| R005 | quality-attribute | validated | M001-k9wo8b/S03 | M001-k9wo8b/S05 | S03 preserved layered fixture and compatibility proof by adding the public `Default fixture and round-trip proof sweep` section to `docs/compatibility-evidence.md`, enforcing coverage-matrix/default-proof wording through Catch2 docs-policy tests, and passing closeout verification gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` across generated fixture targets, writer round-trip/reopen labels for all four current archive families, reader dispatch, validation API, and manifest validation. |
| R006 | failure-visibility | active | M001-k9wo8b/S04 | M001-k9wo8b/S01, M001-k9wo8b/S03 | mapped |
| R007 | integration | active | M001-k9wo8b/S05 | M001-k9wo8b/S02 | mapped |
| R008 | constraint | active | M001-k9wo8b/S02 | M001-k9wo8b/S05 | mapped |
| R009 | constraint | active | M001-k9wo8b/S01 | M001-k9wo8b/S02, M001-k9wo8b/S03, M001-k9wo8b/S04, M001-k9wo8b/S05 | mapped |
| R010 | differentiator | deferred | M002 provisional | none | unmapped |
| R011 | quality-attribute | deferred | M003 provisional | none | unmapped |
| R012 | quality-attribute | deferred | M004 provisional | none | unmapped |
| R013 | launchability | deferred | M005 provisional | none | unmapped |
| R014 | anti-feature | out-of-scope | none | none | n/a |
| R015 | anti-feature | out-of-scope | none | none | n/a |
| R016 | anti-feature | out-of-scope | none | none | n/a |
| R017 | constraint | out-of-scope | none | none | n/a |

## Coverage Summary

- Active requirements: 5
- Mapped to slices: 5
- Validated: 4 (R001, R002, R004, R005)
- Unmapped active requirements: 0
