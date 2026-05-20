# M001 Research — Audit and Stabilization Baseline

**Date:** 2026-05-20  
**Lane:** research  
**Scope:** research current libbsa code, tests, docs, requirements, and verification lanes so the roadmap planner can choose safe M001 slices.

## Executive summary

The main research finding is a state mismatch rather than a missing codebase map: the repository already contains completed M001-style public docs, tests, requirements validation notes, and a completed unique-ID GSD milestone, while the currently addressed `M001` row is queued and has an empty roadmap.

- `gsd_milestone_status("M001")` reports `queued`, `sliceCount: 0`.
- `gsd_milestone_status("M001-k9wo8b")` reports `complete`, with five complete slices and twelve complete tasks.
- `.gsd/milestones/M001/M001-ROADMAP.md` exists but has no slices.
- `.gsd/milestones/M001-k9wo8b/` contains completed roadmap, summaries, validation, and slice artifacts for the same milestone title.
- `.gsd/REQUIREMENTS.md` currently has **0 active requirements**, **R001-R009 validated**, **R010-R013 deferred**, and **R014-R017 out-of-scope**.
- Public M001 deliverables already exist in the repo: `docs/coverage-audit-matrix.md`, `docs/public-api-reality-check.md`, updated API/docs policy tests, and a package-consumer runtime smoke that creates/opens/validates/extracts all current families.

Fresh focused verification during this research passed:

- `ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure` — **16/16 passed**.
- `ctest --preset windows-msvc-debug-static -L docs_policy --output-on-failure` — **24/24 passed**.
- `ctest --preset windows-msvc-debug-static -L target_format_policy --output-on-failure` — **7/7 passed**, including `package_consumer_smoke` and runtime DLL copy.

**Planner implication:** before planning implementation work for `M001`, reconcile whether this is a replay/migration of already-completed `M001-k9wo8b` or a fresh duplicate milestone. If the current milestone must be planned anyway, the natural slices are already visible and should be reuse/verification-oriented, not a rewrite.

## Skills discovered

Requested skill guidance was relevant for API/story/documentation planning: API design, interface design, interrogation/stress testing, observability, and documentation authoring.

Technology-skill discovery results:

- Installed skills already present and relevant: `cmake`, `cpp-testing`, `api-design`, `design-an-interface`, `grill-me`, `observability`, `write-docs`.
- Ran `npx skills find` for missing core technologies: `vcpkg`, `Catch2`, `DirectXTex`, `libdeflate`, and `LZ4`.
- Found and installed `mohitmishra786/low-level-dev-skills@conan-vcpkg` globally as `conan-vcpkg` because vcpkg is central to libbsa's build/dependency contract.
- No directly relevant skills were found for Catch2, DirectXTex, libdeflate, or LZ4.

Evidence:

- Skill discovery: gsd_exec `6a424eda-ca5c-4bb1-8bd9-28b1e323b3cb`.
- Skill install: gsd_exec `d336ae10-aedd-4cd9-a4d2-ea27a8603712`.

## Current codebase shape

### Public API surface

The core public API is already substantial and matches the milestone context:

- `include/libbsa/archive.hpp`
  - `archive_reader::open`, `metadata`, `entries`, `find`, `contains`, `extract`, `extract_bytes`, `extract_entries`.
  - Dependency-light metadata: `archive_metadata`, `entry_metadata`, `texture_metadata`, `texture_chunk_metadata`.
  - Caller-owned `payload_sink` and `bulk_extract_sink_factory` boundaries.
- `include/libbsa/writer.hpp`
  - Family-specific write-new APIs: `tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, `ba2_dx10_writer`.
  - Target enums for TES4-family BSA, BA2 GNRL, and BA2 DX10 instead of raw archive flags.
  - Shared `write_execution_options` with positive `worker_count` semantics.
  - Explicit one-shot/snapshot lifecycle for `ba2_dx10_writer`.
- `include/libbsa/validation.hpp`
  - `validate_archive`, `validation_options`, `validation_report`, `validation_diagnostic`, and public compatibility warnings.
- `include/libbsa/result.hpp`
  - C++20-compatible `result<T>` / `result<void>` with stable `error_code` categories.
  - Diagnostic messages are explicitly not stable machine-readable contracts.
- `include/libbsa/libbsa.hpp`
  - Umbrella header includes reader, result, validation, version, and writer headers.

The public API direction should remain: `archive_reader`, validation, bulk extraction, `result<T>`, and family-specific writers are the stable core. No broad facade is justified by the current evidence.

### Internal implementation layout

The codebase is already split along useful format and responsibility seams:

- `src/formats/bsa/` — 31 C++ files for BSA detection, TES3/TES4 parsing, reading, preparation, layout, serialization, and writing.
- `src/formats/ba2/` — 35 C++ files for BA2 GNRL/DX10 detection, parsing, reading, DDS chunk assembly, preparation, layout, serialization, snapshot building, and writing.
- `src/detail/` — 28 C++ files for paths, hashing, binary I/O, compression routing, codecs, host file handling, bounded payload streaming, parallel work, and writer publication.
- `src/texture/` — DirectXTex-backed DDS analysis behind internal adapters.

Important dependency boundary: public headers mention observable concepts such as DXGI numeric format values and LZ4 compression enum values, but they do not expose DirectXTex, libdeflate, or LZ4 library types. `std::expected` is not used; public `libbsa::result<T>` preserves C++20.

### Build and dependency model

- `CMakeLists.txt` requires CMake 4.0 and C++20.
- vcpkg manifest dependencies: `libdeflate`, `lz4`, `directxtex`, `catch2`, and test-only `nlohmann-json`.
- CMake links libdeflate, LZ4, DirectXTex, and `bcrypt` privately.
- Public install/export target is `libbsa::libbsa`.
- CMake presets are Windows/MSVC-only: debug static/shared, release static/shared, and MSVC ASan static.
- CI runs Windows lanes only and checks `TES5Edit` status after build/test.

This is aligned with the project constraint: do not add Linux/macOS/POSIX portability work to M001.

## Existing proof and documentation artifacts

The support-claim truth artifacts are already present and policy-tested.

### `docs/coverage-audit-matrix.md`

This is the public support-truth matrix. It covers every current archive family and all key capability axes:

- TES3 BSA.
- TES4-family BSA v103/v104/v105.
- BA2 GNRL Fallout 4 / Starfield v2 / Starfield v3.
- BA2 DX10 Fallout 4 / Starfield v3 method 0 and method 3 routes.

Axes include reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, package-consumer proof, and docs/support-claim proof.

The matrix explicitly clarifies that `tests/fixtures/generated/compatibility_matrix.json` is only a malformed-hardening submatrix, not the whole support matrix.

Current ranked gaps:

- `COV-GAP-002` — full BSArchPro / real game-corpus comparisons are deferred.
- `COV-GAP-004` — TES3/BA2-DX10-specific compatibility-warning taxonomy is deferred until concrete interoperability risks exist.

Former gaps are documented as closed:

- `COV-GAP-001` — direct validation success evidence.
- `COV-GAP-003` — installed package-consumer runtime proof.

### `docs/public-api-reality-check.md`

This document maps consumer flows to the real public API and default proof. It concludes no broad public facade or new helper is currently warranted. Remaining friction is docs/examples clarity around existing APIs, not missing public capability:

- `find` vs `contains`.
- `extract` vs `extract_bytes`.
- Bulk extraction per-entry failure shape.
- Validation report vs result-level setup failures.
- BA2 DX10 one-shot writer lifecycle.

### `docs/compatibility-evidence.md`

This file maps public compatibility-warning codes to rule/evidence/gate:

- `compressed_sound_payload`.
- `bsa_embedded_name_compatibility_risk`.
- `target_family_mismatch`.

It also documents the default fixture/round-trip proof sweep and keeps optional local game/BSArchPro evidence advisory and non-blocking.

### Package-consumer proof

`tests/package-consumer/main.cpp` now does more than compile examples. Through only `<libbsa/libbsa.hpp>` and installed `libbsa::libbsa`, it:

- Creates writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives.
- Reopens them through `archive_reader`.
- Checks metadata, entries, `find`, `contains`, streaming extraction, `extract_bytes`, and bulk extraction.
- Validates archives through `validate_archive`.
- Generates a tiny legal BC1 DXT10 DDS inline for BA2 DX10 package-consumer runtime proof.
- Verifies missing archive errors branch as `io_error` through reader and validation paths.

This directly closes the package-consumer runtime proof gap without requiring source-tree fixtures, local copyrighted archives, or TES5Edit data.

## Requirements analysis

The current `.gsd/REQUIREMENTS.md` state is post-completion:

- **Active requirements:** none.
- **Validated requirements:** R001-R009.
- **Deferred requirements:** R010-R013.
- **Out of scope:** R014-R017.

If the roadmap planner treats the original M001 context as authoritative rather than the current requirement state, R001-R009 are table stakes:

- R001 coverage/gap matrix.
- R002 all current archive families audited.
- R003 public API story audited.
- R004 risk-bounded high-risk gaps fixed with proof.
- R005 layered generated fixture/package-consumer/optional compatibility proof preserved.
- R006 structured public error behavior stabilized.
- R007 installed/package-consumer API proven.
- R008 dependency-light C++20 public headers preserved.
- R009 TES5Edit read-only boundary preserved.

Do **not** pull these deferred/out-of-scope items into M001 unless the user explicitly changes scope:

- R010 broad ergonomic facade / major API redesign.
- R011 exhaustive BSArchPro/game-corpus campaign.
- R012 performance and large-archive stress gates.
- R013 publish/release readiness.
- R014 non-Bethesda archive formats.
- R015 GUI/CLI product surface.
- R016 in-place archive mutation.
- R017 cross-platform portability expansion.

Candidate requirement surfaced by this research:

- **Advisory only, not product scope:** reconcile the duplicate GSD milestone state (`M001` queued/empty vs `M001-k9wo8b` complete) before planning duplicate work. This is an automation/state-maintenance issue, not a libbsa product requirement.

## What should be proven first

1. **Milestone state truth before code work.** Determine whether `M001-k9wo8b` is the completed canonical milestone and whether `M001` should be closed, redirected, or replanned as a verification-only replay.
2. **Coverage matrix truth before remediation.** If any M001 work remains, keep `docs/coverage-audit-matrix.md` as the public truth source and update it in the same change as proof changes.
3. **Default legal proof before optional corpus confidence.** No default acceptance should depend on `LIBBSA_GAME_FIXTURES`, `LIBBSA_BSARCHPRO_EXPECTED`, or copyrighted archive bytes.
4. **Package-consumer runtime proof before API claims.** Compile-only examples are weaker than the current installed `package_consumer_smoke`; preserve runtime create/open/validate/extract proof for all families.
5. **Validation/result semantics before message wording.** Branchable `error_code` and warning codes matter; exact diagnostic messages remain human-readable and unstable.

## Boundary contracts that matter

- **Archive paths are virtual keys, not host filesystem paths.** Keep host path policy outside libbsa internals and examples.
- **Public headers stay dependency-light C++20.** No public DirectXTex/DXGI types beyond raw numeric format values, no public libdeflate/LZ4 types, and no C++23-only `std::expected`.
- **`result<T>` code is stable; message text is not.** Tests and docs should not compare exact messages as contracts.
- **Validation split:** setup/caller/host-path failures can be result-level failures; inspectable archive data problems belong in `validation_report::errors` when safely parseable.
- **Compatibility warnings are not speculative.** Add warning codes only for concrete interoperability risks with default generated/writer-output evidence.
- **Bulk extraction shape:** outer setup failures vs per-entry failures, duplicate request coalescing, distinct sink requirement, and positive worker count are all public behavior.
- **Writer finalization:** target profile enums own compatibility behavior; `write_to` publication is same-directory temp output with overwrite/reparse-point rules.
- **BA2 DX10 writer lifecycle:** DDS snapshots occur at `add_file`; `write_to` consumes the writer after ordinary attempts; cleanup is best-effort with abnormal termination residual risk.
- **Codec routing:** TES4 v105 uses LZ4 frame; Starfield BA2 v3 method `3` uses raw LZ4 block; method `0` routes through deflate. Do not conflate frame and raw-block APIs.
- **TES5Edit remains read-only reference.** It is not a fixture workspace, compiled input, or mutable source of committed outputs.

## Known failure modes that should shape ordering

- Treating `tests/fixtures/generated/compatibility_matrix.json` as the whole support matrix. It is only malformed-hardening evidence.
- Creating docs-only claims without tests, fixture manifests, package-consumer proof, or docs-policy guardrails.
- Marking optional game/BSArchPro local checks as required default proof.
- Adding a broad facade or public helper before evidence shows a real consumer flow cannot be documented or proven safely.
- Leaking internal dependencies or platform types into public headers while trying to improve proof.
- Testing exact diagnostic strings instead of stable `error_code` / warning code categories.
- Assuming package-consumer compile smoke proves runtime archive functionality. The current package-consumer smoke rightly exercises runtime create/open/validate/extract flows.
- Regressing BA2 DX10 snapshot cleanup or one-shot lifecycle wording.
- Planning duplicate M001 implementation work while completed `M001-k9wo8b` artifacts already exist.

## Natural slice boundaries

If the planner starts from the original M001 context and ignores the already-completed unique-ID milestone, the natural implementation slices are:

1. **Coverage Audit Matrix** — create/validate `docs/coverage-audit-matrix.md`, cover all families and axes, rank gaps, and add docs-policy tests.
2. **Public API Reality Check** — audit public headers, docs, and package-consumer usage; decide whether docs/examples/policy tests are enough or whether a narrow helper is justified.
3. **Fixture and Round-trip Gap Closure** — close the highest claim-risk generated-fixture/round-trip/default-proof gaps, update compatibility evidence, and avoid optional local corpus dependence.
4. **Error and Validation Stabilization** — close validation/report/error/warning inconsistencies, especially direct per-variant success proof and branchable warning behavior.
5. **Package-consumer and Final Matrix Closeout** — prove installed `libbsa::libbsa` runtime use across all families, finalize fixed/deferred statuses, run focused/full verification, and update requirements.

If the planner starts from the current repository state, a safer roadmap is shorter:

1. **State Reconciliation / Acceptance Slice** — decide whether to treat `M001-k9wo8b` as canonical, copy or link its conclusions into `M001`, and avoid duplicate implementation.
2. **Fresh Verification Slice** — run full default CTest and package-consumer verification on the current build environment, checkpoint the result, and verify TES5Edit stayed clean.
3. **Drift Remediation Slice, only if needed** — if fresh full verification or docs-policy checks fail, fix only the failing evidence path and update the matrix/requirements accordingly.

## Verification and evidence gathered in this research

Repository scans:

- Inventory and file map: gsd_exec `c12ccc55-68b5-401d-a9f4-881a270e2ba0`.
- Source module/dependency-boundary scan: gsd_exec `1bfce170-89b1-44ef-8683-1915c7ecd94f`.
- Test/tag/proof surface summary: gsd_exec `91ceb510-6480-44c3-83a7-c59f9c22e2aa`.
- Existing build/test availability check: gsd_exec `cd8bad27-e754-4f03-aa72-f41c5ba9cc12`.
- Workspace/TES5Edit/GSD artifact state check: gsd_exec `d0e28a61-6e55-48e1-954c-136f8aade6e3`.
- Completed `M001-k9wo8b` artifact summary: gsd_exec `e480798d-9a70-4413-ac55-4a5342e3035a`.

Fresh focused verification:

- gsd_exec `82d5bf20-033b-4993-a818-9122a0b18f7b` passed:
  - `coverage_audit_matrix`: 16/16.
  - `docs_policy`: 24/24.
  - `target_format_policy`: 7/7, including package-consumer runtime smoke and runtime DLL copy.

State observations:

- `git status --short -- TES5Edit` returned no status lines during research.
- The general workspace has many `.gsd.migrating` changes/untracked files. Treat those as GSD migration/state artifacts, not libbsa product changes, and do not stage them accidentally.

## Recommended planner decision

Do not plan a greenfield M001 implementation from the stale queued `M001` row without first reconciling the completed `M001-k9wo8b` state. The product work described by the M001 context appears already implemented and verified in public docs/tests, with remaining gaps explicitly deferred.

Best next step for the roadmap planner: create a tiny reconciliation/verification plan for `M001` or import/accept the completed `M001-k9wo8b` milestone, rather than duplicating the five-slice audit/remediation sequence.