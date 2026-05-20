# M001-k9wo8b: Audit and Stabilization Baseline

**Gathered:** 2026-05-19
**Status:** Ready for planning

## Project Description

libbsa is a relatively established Windows-only C++20 library for reading, extracting, validating, and writing Bethesda Game archive formats. Most of the implementation work appears already complete, but the project needs to close gaps, including testing gaps, fix bugs discovered by evidence, and create a proper public API surface because the current one feels pretty basic from prior interactions.

This milestone is not a greenfield rewrite and not an MVP reduction. It is an audit-and-stabilize pass over the existing library so later work can trust the project’s support claims.

## Why This Milestone

The user has no known external consumers yet and no known bug list. The only known truth source is what the current code, tests, generated fixtures, package-consumer checks, and docs already prove. Before deeper API polish, compatibility hardening, performance work, or release readiness, libbsa needs a truthful coverage/gap matrix and a risk-bounded first set of fixes.

## User-Visible Outcome

### When this milestone is complete, the user can:

- Inspect a durable matrix showing what libbsa currently proves for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 across read, write, extraction, round-trip, malformed, validation, compatibility, package-consumer, and docs claims.
- See which highest-risk gaps were fixed in M001 with durable tests or policy checks, and which remaining gaps were explicitly deferred with rationale and likely future owners.
- Trust that the public API story was audited rather than guessed: `archive_reader`, validation, bulk extraction, and family-specific writers are either proven as the stable core or flagged where small helpers/docs are needed.

### Entry point / environment

- Entry point: CMake/CTest development workflow, public headers under `include/libbsa/`, docs under `docs/`, package-consumer tests under `tests/package-consumer/`, and generated fixture tests under `tests/unit/` and `tests/fixtures/generated/`.
- Environment: Windows local dev and CI-style CMake/vcpkg/MSVC test environment.
- Live dependencies involved: local filesystem, CMake/vcpkg, Catch2/CTest, libdeflate, LZ4, DirectXTex, optional local game archives, optional BSArchPro-derived expected manifests. No network service is part of this milestone.

## Completion Class

- Contract complete means: the coverage/gap matrix exists, active requirements R001-R009 are mapped and re-checked, highest-risk selected gaps have tests/policy/docs proof, and remaining gaps are explicitly deferred.
- Integration complete means: default build/test/package-consumer verification proves the public package surface and representative generated-fixture behavior still work together.
- Operational complete means: default verification does not require copyrighted local fixtures; optional game/BSArchPro comparison paths remain documented and non-required. No daemon/service lifecycle is involved.

## Final Integrated Acceptance

To call this milestone complete, we must prove:

- A default CMake/CTest verification path passes after the audit-driven changes, including generated fixture and package-consumer checks available in the repo.
- The coverage/gap matrix truthfully records every current archive family and agreed capability axis, including fixed and deferred status after remediation.
- The installed/package-consumer public API remains usable through `<libbsa/libbsa.hpp>` and the exported target.
- The milestone cannot be considered complete if the matrix is docs-only with no remediation proof, or if fixed gaps lack durable tests/policy checks.

## Architectural Decisions

### Explicit Core API Plus Audit-Proven Helpers

**Decision:** Keep `archive_reader`, `validate_archive`, bulk extraction, and family-specific writer classes as the explicit public core. Add only small helpers or documentation improvements where the audit/package-consumer evidence proves real friction.

**Rationale:** `include/libbsa/archive.hpp` already exposes metadata, entry metadata, texture metadata, single and bulk extraction, and structured result/error behavior. `include/libbsa/writer.hpp` already exposes TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 writer classes and target options. The immediate risk is not that no API exists; it is whether the API tells a complete, pleasant, proper consumer story.

**Alternatives Considered:**
- New facade layer — may become valuable later, but too much design risk before real consumer or audit evidence.
- Document current API only — safer but insufficient because the user wants gaps closed and confidence improved.

### Matrix Plus Tests Audit Model

**Decision:** M001 should produce a durable coverage/gap matrix and convert important fixed gaps into tests, policy checks, fixture assertions, package-consumer examples, or docs policy checks.

**Rationale:** A matrix makes support claims and blind spots visible; tests keep fixed gaps from immediately going stale.

**Alternatives Considered:**
- Tests only — enforceable, but weak for planning and explaining what “support” means across all archive families.
- Docs first — useful for broad unknowns, but M001 should include concrete stabilization work too.

### Layered Proof

**Decision:** Use generated legal fixtures as the default CI proof, package-consumer tests for public API usability, and optional local game/BSArchPro-derived checks for higher-confidence compatibility evidence.

**Rationale:** This balances repeatability and legal constraints with a path to real-world compatibility confidence.

**Alternatives Considered:**
- CI fixtures only — simpler but weaker compatibility signal.
- Reference compatibility first — stronger signal but environment-dependent and not suitable as a default requirement.

### Risk-Bounded Remediation

**Decision:** Fix a coherent tranche of highest-risk gaps discovered by the audit; do not require every discovered gap to close in M001.

**Rationale:** The milestone should produce concrete stabilization progress without ballooning unpredictably if the audit finds deep issues.

**Alternatives Considered:**
- Close all gaps — attractive but likely to make the first milestone unbounded.
- Audit mostly — safer but would not deliver enough stabilization progress.

## Error Handling Strategy

Use sensible defaults based on the existing public result/error model:

- Keep `libbsa::result<T>` as the public failure channel.
- Keep public `error_code` categories stable and coarse enough for consumers to branch on.
- Treat diagnostic messages as human-readable text, not stable machine-readable contracts.
- Avoid exceptions for archive data, filesystem, compression, validation, or caller-input failures.
- Audit open/detect, lookup/extraction, bulk extraction, writers, validation, and compatibility warnings for inconsistent categories or vague diagnostics.
- Prefer contextual diagnostics that identify the archive family and operation, such as TES4 BSA extraction versus BA2 DX10 writer finalization.
- Do not add retries by default for local file operations; return structured failure with enough context.
- Compatibility concerns that do not make an archive unreadable should stay warnings rather than hard failures.

## Risks and Unknowns

- Current tests may prove less than support claims imply — this could invalidate assumptions about “most work is already complete.”
- Public API friction is not yet known — the API may already be richer than remembered, but package-consumer examples may expose missing ergonomic helpers or docs gaps.
- The highest-risk gaps are unknown until the audit — slice planning must allow evidence-driven remediation rather than preselecting fake bugs.
- Optional local game/BSArchPro evidence may be unavailable — default completion must rely on generated legal fixtures and documented opt-in paths.
- The PROJECT artifact renderer required a legacy `M001` checkbox line even though this active milestone uses directory `M001-k9wo8b` — downstream agents should treat `M001-k9wo8b` as the planned milestone ID for this run.

## Existing Codebase / Prior Art

- `include/libbsa/archive.hpp` — Public reader, metadata, entry metadata, texture metadata, payload sink, and bulk extraction surface.
- `include/libbsa/writer.hpp` — Public write-new API for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- `include/libbsa/validation.hpp` — Structured validation reports and compatibility warning categories.
- `src/formats/bsa/` — TES3 and TES4-family BSA parsing, reading, preparing, layout, serialization, and writers.
- `src/formats/ba2/` — BA2 GNRL and DX10 parsing, reading, texture chunk assembly, preparing, layout, serialization, and writers.
- `src/detail/` — Archive path, hash, binary I/O, compression routing, deflate/LZ4 codecs, host file/path, payload streaming, parallel work, and writer publication internals.
- `src/texture/` — DDS layout and DirectXTex analyzer internals.
- `tests/unit/` — Broad Catch2 test suite covering readers, writers, fixtures, validation, public include boundary, compatibility warnings, path handling, compression, and policies.
- `tests/package-consumer/main.cpp` — Compile-checked installed-package usage examples.
- `docs/api-mainpage.md` and `docs/integration-examples.md` — Current public API story and package-consumer examples.
- `docs/compatibility-evidence.md` — Mandatory generated/writer-output evidence and optional local game/BSArchPro comparison policy.
- `docs/target-format-guide.md` — Current target-format and compatibility warning claims.
- `TES5Edit/` — Read-only behavioral reference; never edit, format, stage, compile, vendor, or use as a mutable fixture workspace.

## Relevant Requirements

- R001 — M001 creates and maintains the truthful coverage matrix.
- R002 — M001 audits all current archive families.
- R003 — M001 audits the public API capability story.
- R004 — M001 fixes highest-risk audit gaps with proof.
- R005 — M001 preserves layered fixture and compatibility proof.
- R006 — M001 audits and stabilizes structured public error behavior.
- R007 — M001 proves the package-consumer API remains installable and usable.
- R008 — M001 preserves dependency-light C++20 public headers.
- R009 — M001 preserves the TES5Edit read-only boundary.

## Scope

### In Scope

- Audit the actual public API surface, especially whether it supports more than basic extraction and writing.
- Audit all currently implemented archive families: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Use current tests and fixtures as the starting truth source.
- Identify missing or weak coverage across parsing, writing, round-trip behavior, malformed archives, compatibility behavior, validation, package-consumer usage, and public API usability.
- Fix a risk-bounded tranche of highest-risk gaps found by the audit.
- Keep the library reusable and API-focused.

### Out of Scope / Non-Goals

- Supporting formats not already represented in the implementation, unless the audit discovers a partially implemented in-repo path that should be completed.
- Designing around existing user migrations; there are no known users yet.
- Doing a broad facade or major API redesign without audit evidence.
- Closing every possible audit-discovered gap in M001.
- Adding non-Bethesda archive formats.
- Building a GUI or CLI product.
- Adding in-place archive mutation for this milestone.
- Expanding platform support beyond Windows/MSVC/vcpkg.

## Technical Constraints

- C++20 public API; do not expose C++23-only types such as public `std::expected`.
- Public headers must remain dependency-light and hide libdeflate, LZ4, DirectXTex, DXGI, and private Windows implementation details.
- Use libdeflate, official LZ4, and DirectXTex via vcpkg; do not add dependencies speculatively.
- `TES5Edit/` is read-only reference only and must not be touched.
- Default verification must pass from committed/generated legal fixtures and policy tests without local copyrighted fixtures.
- Optional compatibility checks must stay opt-in and skipped when environment variables/local data are absent.

## Integration Points

- CMake/vcpkg package graph — libbsa must configure, build, install/export, and expose the package target correctly.
- Catch2/CTest — default tests and labels are the main automated proof mechanism.
- Generated fixture tools — legal always-on archives/manifests for format behavior proof.
- Package-consumer smoke tests — public installed API proof.
- Optional local fixture environment — `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` provide advisory compatibility evidence when available.
- Compression and texture libraries — libdeflate, LZ4, and DirectXTex stay internal implementation dependencies.

## Testing Requirements

- Default CTest path must pass without requiring local game archives or BSArchPro output.
- New or fixed gaps must have focused Catch2/policy/fixture/package-consumer/docs tests where applicable.
- Matrix entries should point to proof: test names, docs, generated fixtures, package-consumer examples, or deferred rationale.
- Error behavior changes must be verified through public `result<T>`, `validation_report`, or compatibility warning assertions.
- Public API changes must be proven through public headers and package-consumer usage, not only internal tests.
- Optional local compatibility tests must remain skipped when local inputs are unavailable.

## Acceptance Criteria

- S01 produces the coverage/gap matrix and ranks gaps by risk across all current archive families and capability axes.
- S02 audits the public API capability story and preserves dependency-light C++20 headers while identifying or implementing only evidence-backed small helpers/docs improvements.
- S03 closes the highest-risk fixture or round-trip coverage gaps selected from the matrix with durable proof.
- S04 stabilizes high-risk inconsistent error, validation, or compatibility-warning behavior discovered by the audit.
- S05 updates the matrix with fixed/deferred status and proves the integrated default build/test/package-consumer path.
- Remaining gaps are explicitly deferred with rationale and likely future owner.

## Open Questions

- Which exact gaps will rank highest — intentionally unknown until S01 audit evidence exists.
- Whether package-consumer examples already fully describe the desired API story or need small helpers/docs improvements — to be determined in S02.
- Whether optional local game/BSArchPro evidence is available in the execution environment — useful if present, but not required for M001 completion.
