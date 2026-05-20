---
id: S01
milestone: M001-k9wo8b
status: ready
---

# S01: Coverage Audit Matrix — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Produce a truthful, human-first coverage/gap matrix for current libbsa archive-family support, with default-verification evidence and ranked claim-risk gaps.

## Why this Slice

S01 runs first because later API, fixture, validation, and integrated-confidence work needs a shared truth source for what libbsa currently proves versus what it only appears to support. The current repository already has broad tests, docs, generated fixtures, package-consumer checks, and a malformed-hardening `compatibility_matrix.json`, but there is not yet a full M001 matrix that compares all current archive families against the agreed capability axes. A blunt audit now unblocks S02-S05 by turning vague support claims into evidence-backed cells, gap IDs, and explicit deferrals.

## Scope

### In Scope

- Audit all currently implemented archive families named by the roadmap: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Use a two-tier matrix: top-level family rows by default, with subrows only where target/version behavior materially changes proof or risk, such as TES4 v103/v104/v105, Fallout 4 versus Starfield BA2, deflate versus LZ4-frame/raw-LZ4 routes, or DX10 target differences.
- Cover the agreed capability axes: reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof.
- Use blunt statuses such as Proven, Partial, Missing, and Deferred. Every non-green cell should point to evidence, a gap ID, or a rationale so the matrix does not hide uncertainty.
- Treat a cell as green only when default verification can prove it through generated legal fixtures, always-on Catch2/CTest tests, package-consumer checks, or policy tests.
- Keep optional local game archives or BSArchPro-derived comparisons separate from default green status. They may add advisory confidence, but they must not be required for S01 or default completion.
- Rank gaps claim-risk first: the highest-priority gaps are places where current support claims are not backed by default executable proof.
- Produce a ranked gap list with identifiers, risk level, evidence source, affected family/axis, and likely downstream remediation or deferral path.
- Optimize the artifact for future humans and downstream slice planning rather than making a machine-schema-first inventory.

### Out of Scope

- Fixing production code, tests, or docs discovered by the audit, except for changes strictly necessary to write the S01 matrix artifact itself.
- Closing any S01-discovered gap in this slice; remediation belongs to S03, S04, or later slices based on the ranked list.
- Making the existing `tests/fixtures/generated/compatibility_matrix.json` the full M001 matrix; it is malformed-hardening-specific evidence, not the complete coverage audit.
- Building a full machine-checkable matrix schema as the primary deliverable unless task planning finds a cheap, low-risk way to add one without distracting from the human-first matrix.
- Treating optional local game or BSArchPro evidence as mandatory acceptance proof.
- Designing a new public API facade or making broad API changes; S02 owns the public API reality check.
- Adding new archive formats, GUI/CLI surfaces, in-place mutation, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- The matrix is human-first and should be easy for future contributors to scan, with evidence links and gap IDs visible near each claim.
- Default green proof requires committed/generated legal fixtures, always-on tests, package-consumer checks, or policy tests that can run without local copyrighted archives.
- Optional game-corpus or BSArchPro-derived checks may be documented as advisory evidence only and must remain skipped when local inputs are absent.
- S01 is audit-only by user decision; implementation fixes should be deferred into downstream slices with clear gap IDs.
- Keep TES5Edit strictly read-only and outside all generated outputs, mutable fixtures, formatting, staging, and build inputs.
- Preserve the existing C++20, dependency-light public API boundary while auditing proof; do not introduce public dependency leakage or speculative dependencies.
- Do not let a broad target-profile explosion obscure the main support picture; add subrows only where behavior differences change proof or risk.

## Integration Points

### Consumes

- `.gsd/REQUIREMENTS.md` — Maps R001/R002/R009 and related proof expectations into matrix scope and gap ranking.
- `include/libbsa/archive.hpp` — Source of public reader, metadata, lookup, extraction, and bulk extraction claims to audit.
- `include/libbsa/writer.hpp` — Source of family-specific writer and target-profile claims to audit.
- `include/libbsa/validation.hpp` — Source of validation report, options, and compatibility warning claims to audit.
- `docs/target-format-guide.md` — Existing support-claim surface for target variants, compression routes, writer policies, and compatibility warnings.
- `docs/compatibility-evidence.md` — Existing default-versus-optional evidence policy and public warning-code catalog.
- `docs/api-mainpage.md` and `docs/integration-examples.md` — Existing consumer-facing docs to compare against public API proof.
- `tests/unit/` — Always-on Catch2 evidence for readers, writers, round-trips, malformed handling, validation, policy checks, and docs policy checks.
- `tests/fixtures/generated/` — Legal generated fixtures, manifests, and the existing malformed-hardening compatibility matrix used as evidence sources.
- `tests/package-consumer/` — Installed/exported package-consumer smoke proof for public API usability.
- Optional `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` paths — Advisory compatibility evidence only when locally available; not default green proof.

### Produces

- Coverage/gap matrix artifact — Human-first matrix covering current archive families and capability axes, using Proven/Partial/Missing/Deferred statuses and evidence links.
- Ranked gap list — Claim-risk-prioritized gaps with IDs, affected family/axis, risk level, evidence source, and likely remediation or deferral path.
- S02 input — Public API and docs proof gaps that need consumer-story review.
- S03 input — Fixture, writer, extraction, and round-trip proof gaps selected as likely remediation candidates.
- S04 input — Malformed, validation, error/result, or compatibility-warning inconsistencies observed during the audit.
- S05 input — Final matrix update targets and deferred-gap rationale needed for integrated confidence reporting.

## Open Questions

- Exact matrix path — Current thinking: task planning should choose whether the durable matrix lives under `docs/` for repo-visible support truth or under the S01 planning artifacts for milestone coordination; the user has only decided the artifact should be human-first.
- Optional evidence presentation — Current thinking: optional game/BSArchPro evidence should appear in a separate advisory column or notes section and never turn a cell green without default proof.
