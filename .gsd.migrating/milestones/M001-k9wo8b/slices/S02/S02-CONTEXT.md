---
id: S02
milestone: M001-k9wo8b
status: ready
---

# S02: Public API Reality Check — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Audit whether libbsa's explicit public API core is trustworthy, understandable, dependency-light, and package-consumable, with only evidence-backed docs/test updates or tiny helper recommendations.

## Why this Slice

S02 follows S01 because the coverage matrix and ranked gaps provide the truth source for which public API claims need consumer-facing proof, clarification, or deferral. The library already exposes a meaningful public core through `archive_reader`, `validate_archive`, bulk extraction, and family-specific writers; the user-visible risk is not that no API exists, but that the consumer story may feel basic, under-proven, or easy to misuse. This slice turns the S01 matrix into a public API reality check before later fixture, validation, and integrated-confidence slices depend on the public surface.

## Scope

### In Scope

- Audit the existing explicit public core rather than designing a broad new facade.
- Verify that a first consumer can understand and trust the full core journey at the docs/example level: open/list metadata, lookup, contains checks, single-entry extraction, `extract_bytes`, bulk extraction, validation reports, compatibility warnings, stable error-code branching, and all writer families.
- Use a compile-plus-smoke package-consumer standard for S02: installed/exported target wiring, `<libbsa/libbsa.hpp>`, compile/link coverage of representative examples, and a minimal runtime smoke path.
- Compare public headers, package-consumer examples, docs, policy tests, and S01 matrix findings to identify unclear, unproven, or misuse-prone consumer flows.
- Treat a S02 API gap as real when a core consumer flow is unclear, unproven, or likely to cause wrong usage. Ordinary boilerplate alone is not enough to require new API.
- Prefer docs, examples, package-consumer smoke coverage, and policy tests as the default S02 changes.
- Identify public helper/API gaps for later work when the audit proves consumer friction but the fix is more than a tiny, low-risk adjustment.
- Name sharp edges directly with practical guidance, including BA2 DX10 one-shot writer lifecycle, `worker_count` rules, `result<T>` error contracts, host filesystem path versus archive virtual path separation, and optional corpus/BSArchPro proof boundaries.
- Preserve the dependency-light C++20 public boundary while auditing both usability and support claims.

### Out of Scope

- Creating a broad ergonomic facade or redesigning the public API around a new abstraction layer.
- Treating every repeated snippet or bit of boilerplate as an automatic API gap.
- Making public header changes as the default S02 outcome; header additions should be deferred unless the audit proves a tiny fix is unavoidable and can be documented/tested immediately.
- Requiring a full generated-fixture walkthrough from package-consumer context in S02; stronger behavioral fixture integration can be deferred to S05 unless evidence shows compile-plus-smoke is insufficient.
- Fixing reader, writer, round-trip, malformed, compression, validation, or compatibility bugs that belong to S03/S04.
- Expanding supported archive formats, adding GUI/CLI surfaces, adding in-place mutation, or expanding platform support.
- Exposing private dependencies or implementation details such as libdeflate, LZ4 library types, DirectXTex, DXGI, private Windows internals, or C++23-only public types.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- The public API story should feel trustworthy through explicit examples and honest caveats, not through overclaiming or hiding unsupported behavior.
- Keep `archive_reader`, `validate_archive`, bulk extraction, and the family-specific writer classes as the stable public core unless audit evidence proves a very small helper is immediately justified.
- Public errors remain `libbsa::result<T>` with stable `error_code` categories; diagnostic messages are human-readable and not stable machine-readable contracts.
- Package-consumer proof for S02 should stay cheap, repeatable, and default-runnable without local copyrighted fixtures or BSArchPro-derived outputs.
- Public headers must remain C++20-compatible and dependency-light.
- Sharp edges should be documented plainly enough that consumers know what to do, especially around writer lifecycle, concurrency/worker-count expectations, and path boundaries.
- Any API/doc improvement must be tied to S01 matrix evidence, package-consumer friction, policy-test findings, or a concrete public claim that needs proof.

## Integration Points

### Consumes

- S01 coverage/gap matrix artifact — Identifies public/package-consumer/docs proof cells, weak support claims, and API-related gaps to audit.
- S01 ranked gap list — Provides claim-risk-prioritized API, docs, validation, and package-consumer findings that may need S02 treatment or deferral.
- `include/libbsa/libbsa.hpp` — Umbrella include that must remain the primary consumer entry point.
- `include/libbsa/archive.hpp` — Public reader, metadata, payload sink, `extract_bytes`, and bulk extraction surface to audit for clarity and proof.
- `include/libbsa/writer.hpp` — Public TES3, TES4-family, BA2 GNRL, and BA2 DX10 writer targets, options, lifecycle, and finalization surface to audit.
- `include/libbsa/validation.hpp` — Public validation reports, diagnostics, warning codes, and validation options to audit.
- `include/libbsa/result.hpp` — Public result/error model and stable error categories to audit for consumer guidance.
- `docs/api-mainpage.md` — Current high-level public API story.
- `docs/integration-examples.md` — Current compile-checked consumer examples.
- `docs/target-format-guide.md` — Target profile, compression route, writer policy, publication safety, and compatibility-warning claims.
- `docs/thread-safety.md` — Public concurrency and caller-owned sink/factory guidance.
- `docs/compatibility-evidence.md` — Default versus optional compatibility evidence policy.
- `tests/package-consumer/` — Installed-package compile/link and runtime smoke proof.
- `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, and `tests/unit/target_format_policy_tests.cpp` — Existing policy tests that enforce public boundary and docs claims.

### Produces

- Public API reality-check findings — A concise audit of which core consumer flows are proven, unclear, overclaimed, or deferred.
- Package-consumer proof updates, if needed — Compile-plus-smoke coverage that keeps installed target and umbrella header usage credible.
- Docs/example updates, if needed — Clarifications that make the full core journey and sharp edges understandable without a facade redesign.
- Evidence-backed helper/API gap list — Deferred or tiny immediately justified helper opportunities, with rationale and proof requirements.
- S05 input — Confirmation that the installed/package-consumer public API remains usable and dependency-light, plus any deferred API story gaps.
- S03/S04 input — Any fixture, validation, result/error, or compatibility-warning behavior gaps discovered while auditing public API flows.

## Open Questions

- Whether any S01 matrix API cells will be weak enough to justify more than docs/test updates — Current thinking: default to docs/test audit and defer public header changes unless a tiny evidence-backed fix is unavoidable.
- Whether compile-plus-smoke package-consumer proof is enough after S01 findings are known — Current thinking: yes for S02, while a stronger generated-fixture walkthrough can be considered in S05 if integrated confidence needs it.
- Exact report location — Current thinking: task planning should choose whether S02 findings live in docs, the S02 summary, or an appendix to the S01 matrix, as long as downstream slices can consume the gap IDs and proof status.
