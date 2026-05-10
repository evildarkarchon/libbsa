# Phase 11: Compatibility Warnings, Validation API, and Hardening - Specification

**Created:** 2026-05-10
**Ambiguity score:** 0.15 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can validate existing archives and libbsa-produced archives through a public C++20 validation API that reports stable errors and typed compatibility warnings, while malformed inputs remain fail-closed under expanded malformed and sanitizer-backed test coverage.

## Background

Phases 1 through 10 established the reusable public API boundary, structured `result`/`error_code` flow, generated legal fixture policy, read/extract support for supported BSA and BA2 variants, and write-new support for BSA and BA2 outputs. The codebase already contains many malformed fixture tests for TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10, plus optional local game fixture plumbing through `LIBBSA_GAME_FIXTURES`. The public API does not yet expose a validation result model, typed compatibility warnings, target-family validation for writer outputs, or a traceable catalog connecting non-obvious Bethesda compatibility rules to fixture/reference evidence. CI currently covers Windows static/shared builds, but no sanitizer-oriented preset or CI path exists for malformed parser/decompressor hardening.

## Requirements

1. **Public validation API**: libbsa exposes a dependency-light public validation surface for archive files.
   - Current: Consumers can call `archive_reader::open` and receive success or a single structured `error`, but there is no public API for collecting validation diagnostics, compatibility warnings, or target-family policy results.
   - Target: Public headers define C++20-compatible validation types and functions that can validate an archive host path and return a result containing stable errors and warnings without exposing private parser, codec, DirectXTex, or platform types.
   - Acceptance: Public include-boundary tests compile against installed libbsa headers, call the validation API on at least one generated fixture path, and verify that the API surface uses only public libbsa-owned types.

2. **Archive and writer-output validation**: The validation API checks both existing archives and libbsa-produced writer output.
   - Current: Writer output is proven by unit tests that reopen and extract archives, but there is no consumer-facing validation API that can be run after packing or against an arbitrary archive file.
   - Target: The validation API can be used on existing fixture archives and on archives produced by TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10 writers to confirm structural validity, target-family metadata consistency, extractability where applicable, and compatibility policy checks.
   - Acceptance: Tests create representative writer-output archives for BSA and BA2 families, run the public validation API on them, and assert that validation succeeds without errors while preserving expected metadata and typed warnings.

3. **Typed compatibility warnings**: Compatibility diagnostics use stable warning codes or categories plus human-readable messages.
   - Current: Compatibility-sensitive behavior is documented in comments, specs, manifests, and tests, but consumers cannot programmatically distinguish known risky quirks from fatal format errors.
   - Target: The validation result exposes durable warning identifiers for compatible-but-risky Bethesda quirks, with messages intended for humans and codes intended for tests and downstream branching.
   - Acceptance: Tests exercise at least three warning-producing valid archive scenarios and assert stable warning identifiers without comparing full diagnostic text.

4. **Strict parsing remains the default**: Malformed archives are rejected rather than opened through a lenient reader mode.
   - Current: `archive_reader::open` generally fails closed for malformed generated fixtures; Phase 11 has not defined whether warnings can downgrade parser errors.
   - Target: `archive_reader::open` remains strict, and the validation API may report errors and warnings without producing an `archive_reader` for malformed archives.
   - Acceptance: Malformed fixtures continue to fail through `archive_reader::open` with stable `error_code` values, and validation reports fatal errors for those fixtures without exposing a partially usable reader.

5. **Compatibility evidence catalog**: Non-obvious compatibility rules are traceable to fixture coverage or reference evidence.
   - Current: Evidence is spread across generated manifests, phase specs, verification reports, comments, and tests; there is no single Phase 11 artifact or test-enforced catalog for compatibility rules.
   - Target: Phase 11 adds or updates documentation that maps each non-obvious warning or compatibility rule to at least one generated fixture, writer-output regression, BSArchPro/TES5Edit reference note, or optional local corpus check.
   - Acceptance: Tests or documentation checks verify that every public warning code introduced in Phase 11 appears in the compatibility evidence catalog with a rule description and evidence reference.

6. **Mandatory generated evidence with optional local corpus checks**: Legal generated fixtures remain required, while game or BSArchPro-derived comparisons are opt-in.
   - Current: Generated legal fixtures are committed and local game data is ignored/skipped by policy; the `compat` label exists but no Phase 11 corpus comparison contract is locked.
   - Target: Phase 11 acceptance relies on generated legal fixtures and writer-produced archives. Optional local corpus or BSArchPro-derived comparison tests may be added under `requires-game-fixture` and must skip cleanly when no local path is configured.
   - Acceptance: The full default CTest suite passes without local game archives, any local corpus checks are labeled `requires-game-fixture`, and fixture policy documentation explains how optional compatibility data is configured without committing copyrighted bytes.

7. **Expanded malformed hardening matrix**: Parser and decompressor hardening covers malformed, truncated, oversized, and internally inconsistent inputs across supported families.
   - Current: Many malformed tests exist, including TES3/TES4/BA2 cases, but coverage is uneven and some expectations are family-specific rather than consolidated for Phase 11.
   - Target: Phase 11 expands malformed coverage to a documented matrix spanning TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10, including truncated structures, duplicate canonical paths, invalid payload spans, unsupported compression routes, decompression size mismatches, oversized count/size arithmetic, and invalid texture chunk layouts.
   - Acceptance: Malformed fixture manifests and tests cover the documented matrix, reject unknown expected error-code strings in manifests, and assert stable public `error_code` categories for open-time and extraction-time failures.

8. **Sanitizer-oriented validation path**: Maintainers can run sanitizer-backed malformed parser/decompressor tests where the toolchain supports it.
   - Current: Windows MSVC static/shared CI presets exist, but there is no sanitizer preset or CI lane for malformed parser/decompressor behavior.
   - Target: The repository provides a sanitizer-oriented CMake preset or documented CI path that runs malformed, compression, parser, and validation tests with supported sanitizer flags without making unsupported local toolchains fail normal Windows CI.
   - Acceptance: A sanitizer-oriented configure/build/test path exists, is documented, and can select the malformed/validation test subset; default Windows static/shared CI continues to pass without requiring sanitizer support.

## Boundaries

**In scope:**
- Public validation result, error, and typed compatibility-warning surface in dependency-light C++20 headers.
- Validation of existing archive host paths and libbsa-produced writer-output archives.
- Stable warning codes or categories with human-readable diagnostic messages.
- Strict default parsing: malformed archives remain fatal for `archive_reader::open`.
- Validation reporting for malformed archives without producing a partially usable reader.
- Compatibility evidence documentation linking public warnings and non-obvious rules to fixtures, writer-output tests, TES5Edit/BSArchPro reference notes, or optional local corpus checks.
- Required generated legal fixture and writer-output evidence for Phase 11 acceptance.
- Optional local game or BSArchPro-derived comparison tests labeled `requires-game-fixture` and skipped by default.
- Expanded malformed matrix across TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, compression, and DDS layout behavior.
- Sanitizer-oriented preset or documented CI/test path for malformed parser/decompressor hardening where supported.
- Regression checks proving existing reader, writer, public include boundary, and `TES5Edit/` read-only guarantees remain intact.

**Out of scope:**
- Lenient archive recovery mode that opens malformed archives as usable readers - v2 tracks lenient recovery after strict validation is complete.
- Exhaustive detection of every known Bethesda quirk - Phase 11 introduces stable warning infrastructure and representative warnings, not a complete forever-catalog.
- Mandatory local game archive, BSArchPro output, or copyrighted fixture data for default acceptance - generated legal fixtures remain the required evidence.
- Public logging framework integration - consumers receive structured validation results and decide their own logging.
- Public exposure of DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, or private parser/codec types.
- Performance benchmarking, parallel extraction/packing, and concurrency guarantees - Phase 12 owns performance and concurrency.
- Doxygen site generation, full integration examples, and polished target-format guides - Phase 12 owns final documentation polish, though Phase 11 may add validation/evidence docs.
- In-place archive mutation or repair - v1 remains read/write-new plus strict validation.
- Editing, formatting, compiling into, staging, or using `TES5Edit/` as a fixture workspace - the submodule remains read-only reference material.

## Constraints

- Public validation APIs must remain C++20-compatible and must not expose `std::expected` or third-party implementation types.
- Warning identifiers and error categories are stable programmatic values; diagnostic strings are human-oriented and tests should not depend on exact wording.
- Strict parser behavior is preserved: malformed inputs produce fatal validation errors and fail `archive_reader::open`.
- Generated fixtures and writer-produced archives are the mandatory evidence path; optional local corpus checks must be skipped by default.
- Sanitizer support must be additive and toolchain-gated so unsupported developer machines and default Windows CI are not broken by Phase 11.
- Archive paths remain normalized archive virtual paths, not host `std::filesystem::path` values.
- Validation must not mutate archive files or write into `TES5Edit/`.
- `TES5Edit/` must remain untouched with no git status output after phase work.

## Acceptance Criteria

- [ ] Public include-boundary tests compile and use the validation API with only installed libbsa public headers.
- [ ] Validation succeeds for representative generated TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 success fixtures.
- [ ] Validation succeeds for representative TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives produced by libbsa writers.
- [ ] Validation returns stable fatal errors for malformed fixtures without producing a usable `archive_reader`.
- [ ] `archive_reader::open` continues to reject malformed archives with stable public `error_code` values.
- [ ] At least three valid compatibility-warning scenarios produce typed warning identifiers and human-readable messages.
- [ ] Tests assert warning identifiers or categories without comparing full diagnostic text.
- [ ] Every public warning code introduced by Phase 11 is documented in a compatibility evidence catalog with rule description and fixture/reference evidence.
- [ ] Malformed coverage spans TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, compression failures, oversized arithmetic, unsupported compression routes, and DDS chunk/layout inconsistencies.
- [ ] Manifest-driven malformed tests fail on unknown `expected_error` strings instead of silently mapping them to another category.
- [ ] Optional local game or BSArchPro-derived compatibility checks, if added, are labeled `requires-game-fixture`, skipped by default, and documented in fixture policy.
- [ ] Default full CTest runs pass without local game archives or copyrighted fixture data.
- [ ] A sanitizer-oriented configure/build/test path exists and can run malformed, parser, compression, and validation tests on supported toolchains.
- [ ] Existing reader, writer, fixture generation, public include boundary, and package-consumer tests remain green.
- [ ] `git -C TES5Edit status --short` produces no output after Phase 11 execution.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.92  | 0.75  | met    | Primary deliverable is a public validation API with stable errors and typed warnings. |
| Boundary Clarity    | 0.84  | 0.70  | met    | Strict parsing, optional local corpus, representative warnings, and no lenient recovery are locked. |
| Constraint Clarity  | 0.80  | 0.65  | met    | C++20 public boundary, generated evidence, sanitizer additivity, and TES5Edit limits are explicit. |
| Acceptance Criteria | 0.78  | 0.70  | met    | Pass/fail checks cover API, archives, writer output, warnings, malformed matrix, sanitizer path, and regressions. |
| **Ambiguity**       | 0.15  | <=0.20| met    | Gate passed after round 2. |

Status: met = meets minimum, below = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What should Phase 11's primary deliverable be? | Public validation API is the primary deliverable. |
| 1 | Researcher | What compatibility evidence is mandatory? | Generated legal fixtures are mandatory; local game or BSArchPro-derived corpus checks are optional and skipped by default. |
| 1 | Researcher | How strict should compatibility warnings be? | Warnings use typed stable identifiers plus messages, without requiring exhaustive detection of every quirk. |
| 2 | Researcher + Simplifier | What should the validation surface check? | It validates existing archives and libbsa-produced writer output for structure, warnings, and target-family policy checks. |
| 2 | Researcher + Simplifier | What hardening evidence should count? | Expanded malformed coverage plus sanitizer-oriented preset or CI path where supported. |
| 2 | Simplifier | Should warnings allow malformed archives to open? | No lenient recovery in Phase 11; `archive_reader::open` remains strict, while validation may report errors/warnings without producing a reader. |
| 2 | Gate | Ambiguity reached 0.15. Proceed with SPEC.md? | User selected `Yes - write SPEC.md`. |

---

*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Spec created: 2026-05-10*
*Next step: /gsd-discuss-phase 11 - implementation decisions only*
