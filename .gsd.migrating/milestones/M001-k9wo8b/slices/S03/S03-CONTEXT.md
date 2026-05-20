---
id: S03
milestone: M001-k9wo8b
status: ready
---

# S03: Fixture and Round-trip Gap Closure — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Close a risk-bounded tranche of the highest claim-risk generated-fixture or round-trip gaps from S01 with behavioral proof and minimal end-to-end fixes where needed.

## Why this Slice

S03 turns the S01 audit from a truthful matrix into concrete stabilization proof. The project already has substantial generated fixture infrastructure, writer-output tests, malformed manifests, and validation checks, but S01 may reveal places where current support claims are stronger than default executable proof. Closing a bounded tranche now keeps M001 from becoming a docs-only audit and gives S04/S05 real fixed evidence to validate and integrate.

## Scope

### In Scope

- Consume the S01 coverage/gap matrix and ranked gap list to select a coherent, risk-bounded tranche of fixture or round-trip gaps.
- Prioritize claim-risk first: choose gaps where current support claims would otherwise overstate read/write/round-trip behavior or where default proof is missing for claimed behavior.
- Use data-integrity risk as a tie-breaker inside the claim-risk tranche, especially wrong extracted bytes, bad compression routing, DDS reconstruction/chunk layout problems, or writer output that reopens but is semantically wrong.
- Add or repair committed/generated legal fixtures, manifests, Catch2 assertions, validation checks, or policy tests needed to prove selected gaps.
- Use behavioral round-trip proof as the default proof bar: writer output reopens, relevant metadata supports the claim, extracted bytes match expected payloads, and validation accepts or reports as expected.
- Use byte-for-byte or manifest-golden proof only where the compatibility claim specifically requires byte-level layout, ordering, metadata, or golden archive evidence.
- Make minimal production fixes when a selected gap exposes a real implementation bug, as long as the fix stays inside the selected tranche and has durable proof.
- Preserve generated-fixture provenance so every committed fixture remains legal synthetic data with generator/source documentation.
- Keep optional local game or BSArchPro-derived comparisons advisory; they may inform confidence but cannot be required for S03 closure.
- Produce clear downstream evidence: which gaps were fixed, which tests/manifests prove them, and which related findings should flow to S04 or S05.

### Out of Scope

- Closing every gap discovered by S01 or encountered during S03.
- Expanding from the selected risk-bounded tranche into a broad compatibility campaign.
- Requiring local copyrighted game archives, BSArchPro outputs, or optional corpus data for default proof.
- Treating byte-for-byte writer-output equality as the universal success criterion when behavioral archive compatibility is the actual claim.
- Redesigning the public API or adding public convenience helpers; S02 owns public API reality checking.
- Broad error/result/validation model stabilization outside what is necessary to prove a selected fixture or round-trip gap; S04 owns systemic diagnostic stabilization.
- Adding new archive families, GUI/CLI surfaces, in-place mutation, performance stress gates, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- S03 is bounded: once the selected tranche is complete with proof, newly discovered adjacent gaps should be documented and handed off rather than silently expanding scope.
- Default proof must be runnable from committed/generated legal fixtures and default CTest/Catch2 paths without local game data.
- Fixture changes must keep legal provenance explicit and must not copy game bytes, BSArchPro output, or data from `TES5Edit/`.
- Minimal production fixes are allowed only when a selected gap proves a real behavior bug and the fix can be covered durably in this slice.
- Prefer behavioral compatibility evidence over brittle archive-byte equality unless byte-level layout is the stated support claim.
- Fixed gaps should update or reference the S01 matrix so S05 can finalize fixed/deferred status without re-discovering the evidence.
- Public headers must remain dependency-light C++20; no speculative dependencies should be added for fixture closure.

## Integration Points

### Consumes

- S01 coverage/gap matrix artifact — Provides the family/axis cells and evidence status used to select S03 remediation targets.
- S01 ranked gap list — Defines the initial claim-risk ordering and likely remediation path for fixture and round-trip gaps.
- `tests/fixtures/generated/archives/` — Existing committed legal fixture archives and manifests for TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, and malformed cases.
- `tests/fixtures/generated/*.cpp` — Fixture generators for success, malformed, writer-output, BA2 GNRL, BA2 DX10, TES3, and TES4-family archives.
- `tests/fixtures/generated/validate_fixture_manifests.py` — Manifest and compatibility-matrix structural validation that should continue to pass after fixture changes.
- `tests/unit/*reader_tests.cpp` — Reader, metadata, lookup, extraction, malformed, and compression-routing tests that may need focused proof additions.
- `tests/unit/*writer_tests.cpp` and writer execution tests — Writer output, reopen, extraction, publication, compression, dedupe, and worker-count proof surfaces.
- `tests/unit/validation_api_tests.cpp` — Public validation proof for generated fixture archives and writer-produced archives.
- `tests/unit/compatibility_matrix_tests.cpp` and `tests/fixtures/generated/compatibility_matrix.json` — Existing malformed-hardening evidence that may inform but does not replace the full S01 matrix.
- `tests/fixtures/README.md` and `docs/compatibility-evidence.md` — Fixture legality, default/optional proof boundaries, provenance requirements, and local corpus policy.
- Optional `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` inputs — Advisory local compatibility evidence only when present; not mandatory for default proof.

### Produces

- Fixed generated-fixture or round-trip gaps — The selected S01 gaps closed through tests, manifests, fixture updates, or minimal implementation fixes.
- Durable proof references — Test names, manifest paths, fixture generator references, policy checks, and validation evidence for every fixed gap.
- Updated S01 matrix/gap evidence, if task planning chooses that handoff path — Fixed/deferred status references that S05 can integrate.
- S04 input — Any high-risk result/error/validation/compatibility-warning inconsistencies discovered while closing selected gaps but not fully stabilized in S03.
- S05 input — A concise list of fixed gaps, commands/tests used as proof, remaining deferred gaps, and any optional compatibility evidence notes.

## Open Questions

- Exact selected tranche — Current thinking: wait for S01 matrix output, then choose the top claim-risk fixture/round-trip gaps that can be closed coherently within S03.
- Byte-level proof exceptions — Current thinking: require golden byte/layout proof only for claims that explicitly depend on byte-level compatibility, ordering, offsets, or exact metadata; otherwise prefer reopen/metadata/extraction/validation behavior.
- Matrix update ownership — Current thinking: S03 should at minimum produce fixed-gap evidence that S05 can consume; task planning can decide whether S03 also edits the matrix immediately or leaves final status consolidation to S05.
