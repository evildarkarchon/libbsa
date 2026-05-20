# M001: Audit and Stabilization Baseline

**Gathered:** 2026-05-20
**Status:** Ready for planning

## Project Description

libbsa is a reusable Windows-only C++20 library for reading, extracting, validating, and writing Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. The implementation is already substantial: public reader/writer/validation headers exist, format-specific BSA/BA2 code is in place, generated legal fixtures and Catch2/CTest tests exist, package-consumer smoke checks exist, and docs already describe target formats and compatibility evidence.

M001 is an audit-and-stabilization baseline, not a rewrite and not a greenfield MVP. It should make libbsa's support claims truthful, make the public API story trustworthy, fix a risk-bounded tranche of the highest claim-risk gaps, and leave explicit future ownership for what remains.

## Why This Milestone

The current risk is not that libbsa has no implementation; it is that the project may claim more than the default proof actually demonstrates. Before deeper public API polish, compatibility hardening, performance work, or release readiness, the project needs a durable truth source: a coverage/gap matrix that states what is proven, what is partial or missing, what was fixed in M001, and what is explicitly deferred.

This is needed now because there are no known external consumers and no known bug list. The only reliable truth source is the repository itself: public headers, tests, generated fixtures, package-consumer checks, docs, optional local compatibility paths, and the read-only TES5Edit/BSArchPro reference.

## User-Visible Outcome

### When this milestone is complete, the user can:

- Inspect a public docs coverage/gap matrix showing what libbsa currently proves for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 across read, write, extraction, round-trip, malformed, validation, compatibility, package-consumer, and docs/API proof.
- See a ranked claim-risk gap list with clear fixed/deferred status, evidence references, rationale, and likely future ownership.
- Trust that the public API story was audited against real headers and package-consumer usage: `archive_reader`, `validate_archive`, bulk extraction, and family-specific writers are either proven as the stable core or improved with evidence-backed helpers/docs/tests.
- Run the default CMake/CTest development workflow and package-consumer checks without needing local copyrighted game archives or BSArchPro output.

### Entry point / environment

- Entry point: CMake/CTest development workflow, public headers under `include/libbsa/`, docs under `docs/`, package-consumer checks under `tests/package-consumer/`, generated fixtures under `tests/fixtures/generated/`, and unit/policy tests under `tests/unit/`.
- Environment: Windows local development and CI-style Windows/MSVC/vcpkg build/test environment.
- Live dependencies involved: local filesystem, CMake/vcpkg, Catch2/CTest, libdeflate, official LZ4, DirectXTex, optional local game archives, optional BSArchPro-derived expected manifests. No network service, daemon, GUI, or CLI product is part of this milestone.

## Completion Class

- Contract complete means: the coverage/gap matrix exists in public docs, active requirements R001-R009 are mapped and re-checked, every current archive family is represented, the public API story is audited, fixed gaps have durable proof, and remaining gaps are explicitly deferred with rationale.
- Integration complete means: the default build/test/package-consumer path proves the library still works as a package-consumable whole, including generated fixture behavior and installed/exported public API usage.
- Operational complete means: default verification does not require copyrighted local fixtures or BSArchPro-derived output; optional game/BSArchPro comparison paths remain documented, opt-in, and non-blocking. No long-running lifecycle or production service operation is involved.

## Final Integrated Acceptance

To call this milestone complete, we must prove:

- A default Windows MSVC CMake/CTest verification path passes after the audit-driven changes, including generated fixture, validation, compatibility, public API/policy, and package-consumer checks available in the repository.
- The coverage/gap matrix truthfully records every current archive family and agreed capability axis, with final Proven/Fixed/Partial/Missing/Deferred status, evidence references, and explicit rationale for remaining gaps.
- The installed/package-consumer public API remains usable through `<libbsa/libbsa.hpp>` and the exported `libbsa::libbsa` target.
- M001 cannot be considered complete if the matrix is docs-only with no remediation proof, if fixed gaps lack durable tests/policy/fixture/package-consumer evidence, or if default completion requires local copyrighted fixtures.

## Architectural Decisions

### Truth Plus Risk-Bounded Fixes

**Decision:** M001 should make support claims truthful, fix a bounded high-risk tranche, and explicitly defer the rest.

**Rationale:** The project already has broad implementation and tests. The immediate value is not rewriting the library, but turning current support claims into evidence-backed truth and closing the most misleading or risky proof gaps.

**Alternatives Considered:**
- Audit only — safer and predictable, but insufficient because the milestone should deliver concrete stabilization progress.
- Release confidence push — valuable later, but too broad for this baseline because compatibility, performance, and release readiness each need their own deeper pass.

### Public Docs Coverage Matrix

**Decision:** The durable coverage/gap matrix should live in public docs, with GSD summaries used for task-level audit detail and handoff.

**Rationale:** The matrix is a support-claim artifact, not only an internal planning note. Future maintainers and consumers should be able to see what is proven, fixed, partial, missing, or deferred without reading GSD internals.

**Alternatives Considered:**
- Keep the matrix only under `.gsd/` — useful for planning, but too hidden for a support-truth artifact.
- Machine-schema-first matrix — useful later if drift becomes painful, but M001 should optimize for a human-first matrix with evidence links.

### Explicit Core API With Evidence-Backed Helpers

**Decision:** Keep `archive_reader`, `validate_archive`, bulk extraction, and family-specific writer classes as the explicit public core. Polish aggressively where evidence proves consumer friction, but only add small public helpers when they can be documented and package-consumer tested immediately.

**Rationale:** The public API is richer than a minimal extraction surface, but it may still feel basic or hard to understand. M001 should improve the consumer story without prematurely committing to a broad facade or redesign.

**Alternatives Considered:**
- Broad facade layer — may become valuable in a future public API polish milestone, but it is too much design risk before real consumer evidence.
- No API changes — safe, but too conservative if the audit proves small helpers would materially improve correctness or usability.

### Layered Proof Model

**Decision:** Use generated legal fixtures, default Catch2/CTest tests, policy tests, and package-consumer checks as mandatory proof. Keep optional local game archives and BSArchPro-derived comparisons advisory and non-blocking.

**Rationale:** Default CI must be reproducible and legally safe, while optional local compatibility evidence still provides a path toward higher confidence against real-world archives and BSArchPro expectations.

**Alternatives Considered:**
- Require full game/BSArchPro corpus proof — stronger compatibility signal, but environment-dependent and legally unsuitable as default completion.
- Rely only on generated fixtures — repeatable, but weaker unless optional corpus paths stay documented and available for future compatibility hardening.

### Public Result and Validation Contract First

**Decision:** Preserve the existing public `libbsa::result<T>`, stable coarse `error_code` categories, focused `validation_report` behavior, and non-fatal compatibility warnings. Stabilize only high-risk inconsistencies surfaced by audit or gap closure.

**Rationale:** Archive consumers need branchable failure categories and human-readable context, but M001 should not redesign the error model. Focused stabilization protects consumers without expanding into broad taxonomy work.

**Alternatives Considered:**
- Richer diagnostic hierarchy — potentially useful later, but too broad for M001.
- Exact message contracts — brittle and unnecessary; messages should help humans, while codes and shapes remain stable.

## Error Handling Strategy

Use the existing public result/error model as the contract:

- Keep `libbsa::result<T>` as the public failure channel.
- Keep public `error_code` values coarse and branchable: `unsupported`, `invalid_argument`, `not_found`, `io_error`, and `format_error`.
- Treat diagnostic messages as human-readable text, not stable machine-readable strings.
- Avoid exceptions for archive data, filesystem, compression, validation, or caller-input failures; reserve thrown exceptions for programmer misuse of `result<T>` accessors.
- Preserve `validate_archive` semantics: setup/caller/host-path failures can be result-level failures, while readable malformed or unsupported archive bytes should be represented inside `validation_report::errors` where possible.
- Keep compatibility concerns that do not make an archive unreadable as warnings rather than hard failures.
- Prefer contextual diagnostics that identify the operation and archive family where evidence shows current messages are too vague.
- Do not add retries for local file operations by default; return structured failure with enough context.

## Risks and Unknowns

- Current tests may prove less than support claims imply — this could expose surprising gaps in apparently complete functionality.
- The highest-risk gaps are intentionally unknown until S01 audits the code, docs, tests, fixtures, package-consumer checks, and matrix evidence.
- API friction may be real even though public headers are already substantial — M001 should allow evidence-backed small helpers without becoming a facade redesign.
- Optional local game/BSArchPro evidence may be unavailable — default completion must rely on generated legal fixtures and documented opt-in paths.
- Verification capacity may vary by local environment — default plus package proof is the required gate, while heavier supported lanes can be documented as CI evidence or deferred when not practical locally.
- Public docs matrix placement may create maintenance pressure — this is acceptable because support claims should be visible and kept truthful.

## Existing Codebase / Prior Art

- `include/libbsa/archive.hpp` — Public reader, archive metadata, entry metadata, texture metadata, payload sink, single extraction, `extract_bytes`, and bulk extraction surface.
- `include/libbsa/writer.hpp` — Public write-new API for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 writer targets and options.
- `include/libbsa/validation.hpp` — Public validation reports, diagnostics, compatibility warning codes, severity, and options.
- `include/libbsa/result.hpp` — Public C++20 result/error model and stable error categories.
- `src/formats/bsa/` — TES3 and TES4-family BSA parsing, reading, preparation, layout, serialization, and writers.
- `src/formats/ba2/` — BA2 GNRL and BA2 DX10 parsing, reading, texture chunk assembly, preparation, layout, serialization, and writers.
- `src/detail/` — Archive path handling, hashing, binary I/O, compression routing, deflate/LZ4 codecs, host file/path handling, payload streaming, parallel work, and writer publication helpers.
- `src/texture/` — DDS layout and DirectXTex analyzer internals.
- `tests/unit/` — Catch2 tests covering readers, writers, fixtures, round-trips, validation, public API boundaries, compatibility warnings, path handling, compression, policy, and malformed cases.
- `tests/fixtures/generated/` — Legal generated fixtures, manifests, fixture generators, and existing malformed-hardening compatibility matrix.
- `tests/package-consumer/` — Installed/exported package-consumer smoke examples.
- `docs/api-mainpage.md` and `docs/integration-examples.md` — Current public API story and compile-checked examples.
- `docs/compatibility-evidence.md` — Compatibility warning evidence and optional local game/BSArchPro comparison policy.
- `docs/target-format-guide.md` — Supported target formats, compression routes, writer policies, and warnings.
- `README.md`, `CMakePresets.json`, `.github/workflows/ci.yml` — Supported Windows MSVC verification lanes, default workflow, package-proof lanes, and TES5Edit read-only CI check.
- `TES5Edit/` — Read-only behavioral reference and prior art; never edit, format, stage, compile, vendor, or use as mutable fixture data.

## Relevant Requirements

- R001 — M001 creates and maintains the truthful coverage/gap matrix.
- R002 — M001 audits all current archive families: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- R003 — M001 audits the public API capability story and identifies evidence-backed API friction or helper needs.
- R004 — M001 fixes a risk-bounded tranche of highest-risk audit gaps with durable proof.
- R005 — M001 preserves layered generated fixture, package-consumer, and optional compatibility proof.
- R006 — M001 audits and stabilizes high-risk structured public error behavior.
- R007 — M001 proves the package-consumer API remains installable and usable.
- R008 — M001 preserves dependency-light C++20 public headers.
- R009 — M001 preserves the TES5Edit read-only boundary.

## Scope

### In Scope

- Create a public docs coverage/gap matrix for all currently implemented archive families and agreed capability axes.
- Audit current public API capability and consumer story against headers, docs, package-consumer usage, and matrix findings.
- Identify and rank gaps by claim-risk, with data-integrity risk used as an important tie-breaker.
- Fix a coherent, risk-bounded tranche of highest-risk fixture, round-trip, API proof, validation, or error behavior gaps.
- Add or update focused Catch2 tests, generated fixtures, manifests, policy checks, package-consumer examples, or docs where needed for durable proof.
- Preserve generated legal fixtures as the default CI proof path.
- Keep optional local game/BSArchPro comparison paths documented, opt-in, and non-blocking.
- Finish by updating fixed/deferred status and running the agreed default plus package verification gate.

### Out of Scope / Non-Goals

- Closing every possible audit-discovered gap in M001.
- A broad public API facade or major redesign without direct audit evidence.
- Exhaustive BSArchPro/game-corpus compatibility campaign.
- Performance and large-archive stress gates beyond preserving existing policies and avoiding regressions.
- Publish/release readiness declaration.
- Adding new archive formats or non-Bethesda archive support.
- Building a GUI or CLI product.
- Adding in-place archive mutation.
- Expanding platform support beyond Windows/MSVC/vcpkg.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as a mutable fixture workspace.

## Technical Constraints

- Public API remains C++20; do not expose C++23-only public types such as `std::expected`.
- Public headers must stay dependency-light and must not expose libdeflate, LZ4 library types, DirectXTex, DXGI, private Windows implementation details, or internal parser/writer types.
- Use libdeflate, official LZ4, and DirectXTex via vcpkg; do not add speculative dependencies.
- Default verification must pass from committed/generated legal fixtures and policy/package-consumer tests without local copyrighted fixtures.
- Optional local compatibility checks must stay opt-in and skipped when local inputs are absent.
- `TES5Edit/` must remain read-only and untouched.
- Verification and docs should remain Windows/MSVC/vcpkg-oriented; do not add cross-platform requirements.

## Integration Points

- CMake/vcpkg package graph — Configure, build, install/export, and expose `libbsa::libbsa` correctly.
- Catch2/CTest — Primary automated proof path for unit, fixture, round-trip, malformed, validation, compatibility, public API, and policy tests.
- Generated fixture tools — Produce legal synthetic archives and manifests for default archive-family behavior proof.
- Package-consumer smoke tests — Prove installed/exported public API usage through `<libbsa/libbsa.hpp>` and `libbsa::libbsa`.
- Public documentation — `docs/coverage-matrix.md` or an equivalent public docs matrix becomes the durable support truth source; existing docs must not overclaim.
- Optional local fixture environment — `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` provide advisory compatibility evidence when available.
- Compression and texture libraries — libdeflate, LZ4, and DirectXTex remain internal implementation dependencies.
- TES5Edit reference — Used only as read-only prior art and behavioral reference.

## Testing Requirements

Default testing should include:

- CMake configure/build/test for the default Windows MSVC path.
- CTest/Catch2 unit and policy tests covering public result/error behavior, reader/writer behavior, validation, compatibility warnings, malformed cases, generated fixtures, compression routing, and path handling.
- Generated fixture manifest validation.
- Package-consumer smoke coverage for installed/exported target usage.
- Focused tests for every M001-fixed gap, preferably using generated fixtures, writer-output archives, policy checks, package-consumer examples, or validation assertions.
- Public API changes, if any, proven through public headers and package-consumer usage, not only internal tests.
- Error behavior changes verified through public `result<T>`, `validation_report`, `validation_diagnostic`, or `compatibility_warning` assertions.
- Optional local game/BSArchPro checks remain skipped when local inputs are unavailable and must not block default completion.

The final integrated gate is default plus package proof: default CMake/CTest verification plus package-consumer coverage. Release package-proof lanes should be run when practical; heavier lanes such as MSVC ASan may be documented as supported CI/hardening evidence rather than absolute local completion gates.

## Acceptance Criteria

- S01: A public docs coverage/gap matrix exists for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 across reader, writer, extraction, round-trip, malformed, validation, compatibility, public/package-consumer API, and docs proof, with blunt statuses and ranked claim-risk gaps.
- S02: The public API story is audited against real headers, docs, and package-consumer usage; dependency-light C++20 headers are preserved; evidence-backed docs/tests or small helpers are added only where the audit proves friction.
- S03: A coherent highest claim-risk fixture/round-trip tranche is fixed with behavioral round-trip proof: writer output reopens where relevant, metadata supports the claim, extracted bytes match expected payloads, and validation accepts or reports as expected.
- S04: High-risk public result/error/validation inconsistencies found by S01/S03/S02 are stabilized with focused tests, preserving `result<T>`, stable `error_code` categories, focused validation reports, and non-fatal compatibility warnings.
- S05: The matrix is updated with final Proven/Fixed/Partial/Missing/Deferred status, default plus package verification passes or documented blockers are resolved, optional compatibility paths remain advisory, and all remaining gaps are explicitly deferred with rationale and likely future owner.

## Open Questions

- Exact S01 matrix filename — Current thinking: use a public docs location such as `docs/coverage-matrix.md`, with GSD summaries holding task-level detail.
- Exact M001 remediation tranche — Current thinking: select after S01 ranks gaps by claim-risk, with data-integrity risk as a tie-breaker.
- Exact API helper candidates — Current thinking: allow only evidence-backed small helpers that can be documented and package-consumer tested immediately.
- Optional local compatibility evidence availability — Current thinking: useful if present, but absence must not block default completion.
- Exact local verification lane coverage — Current thinking: default plus package proof is required; release package-proof lanes should run when practical, while unsupported/unavailable heavier lanes are documented rather than silently assumed.
