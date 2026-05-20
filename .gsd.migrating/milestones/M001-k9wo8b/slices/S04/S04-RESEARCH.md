# S04 Research: Error and Validation Stabilization

## Summary

Depth: targeted research. The slice should close the concrete validation/error proof gap already routed here (`COV-GAP-001`) without redesigning the public error model.

Key finding: `validate_archive` already has the intended public behavior split: empty/invalid or unreadable host paths fail the outer `result` as `invalid_argument`/`io_error`, while readable unsupported or malformed archive bytes become inspectable `validation_report::errors`; compatible-but-risky conditions become `validation_report::warnings`. The actionable gap is not a missing API, but incomplete direct validation-success rows for variant-specific fixtures and Starfield compression-method routes.

Current direct `validation_api` generated fixture coverage is 4 of 9 success archives:

- Present: `tes3_success.bsa`, `tes4_v103.bsa`, `ba2_gnrl_fo4.ba2`, `ba2_dx10_fo4.ba2`.
- Missing from `tests/unit/validation_api_tests.cpp`: `tes4_v104.bsa`, `tes4_v105.bsa`, `ba2_gnrl_sfv2.ba2`, `ba2_gnrl_sfv3.ba2`, `ba2_dx10_sfv3.ba2`.
- Existing `tests/unit/host_path_correctness_boundary_tests.cpp` incidentally validates `tes4_v104`, `tes4_v105`, and `ba2_gnrl_sfv3` through `validate_archive`, but it is not the validation API proof matrix and misses `ba2_gnrl_sfv2` and `ba2_dx10_sfv3`.

Baseline proof run: `gsd_exec 08d47ca2-6d9a-4f50-902e-bcd97b8c6e0d` built `libbsa_tests` and passed `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure` (6/6 tests). Coverage scan: `gsd_exec 75712116-bafb-434d-9e86-9fd4088589eb` produced the exact present/missing fixture list above.

## Requirements owned or supported

- **R006 (primary owner: S04)** — Keep structured public error behavior consistent. This slice should stabilize behavior with tests around `libbsa::result<T>`, `error_code`, `validation_report`, and compatibility warnings. Do not change the public model unless the new tests expose a real semantic bug.
- **R009 (supporting)** — Keep `TES5Edit/` read-only. All proof should use committed/generated legal fixtures, writer-produced archives, and public docs/tests.
- **R001/R002/R004/R005 (already validated, S04 updates them as needed)** — The coverage matrix and proof catalog should be updated if S04 turns `COV-GAP-001` from `Partial` to fixed/proven evidence.
- **Not S04-owned**: `COV-GAP-003` package-consumer every-family runtime proof belongs to S05; `COV-GAP-004` warning taxonomy expansion remains deferred unless a concrete interoperability risk is found.

## Skills Discovered and Applied

- Prompt-listed skills relevant to approach: `api-design`, `design-an-interface`, `grill-me`, `observability`, `write-docs`, `cpp-testing`, and `cmake`.
- Skill discovery command `npx skills find "C++ Catch2 CTest validation error handling"` returned `affaan-m/everything-claude-code@cpp-testing` (4.3K installs), which is already installed as `cpp-testing`; no new skill was installed.
- Applied guidance:
  - API-design: stable machine-readable fields are `error_code` and `compatibility_warning_code`; diagnostic messages remain human-readable and should not become exact test contracts.
  - Design-an-interface: alternatives considered were a public error-model redesign, new warning taxonomy, or direct proof/tests. The least risky design is direct proof with no API shape change.
  - Grill-me: scope should answer the specific gap (`which variant success rows are directly proven?`) rather than inventing speculative warnings or compatibility campaigns.
  - Observability: developer/agent observability should come from structured validation reports, stable codes, and focused Catch2 `INFO` messages, not new runtime logging.
  - Write-docs: if the matrix changes from `Partial` to `Proven`, public docs must explain the evidence to a fresh reader and avoid internal planning IDs.

## Implementation Landscape

### Public API and implementation surfaces

- `include/libbsa/result.hpp`
  - Public error categories: `unsupported`, `invalid_argument`, `not_found`, `io_error`, `format_error`.
  - `result<T>`/`result<void>` already tests stable code branching and logic-error misuse in `tests/unit/result_tests.cpp`.
  - No public API change is needed for S04.

- `include/libbsa/validation.hpp`
  - Public validation shape: `validation_options`, `validation_report`, `validation_diagnostic`, `compatibility_warning`, and `compatibility_warning_code`.
  - Important contract: result-level failures are setup/caller path failures; archive-data issues should be collected in `validation_report::errors`; warnings remain compatible-but-noteworthy.
  - `expected_variant` exists but only has compile-boundary proof today; behavior is implemented in `src/validation.cpp` but not directly exercised as a runtime warning path.

- `src/validation.cpp`
  - `validate_archive` returns outer `error` for empty host path and for `archive_reader::open` failures with `io_error` or `invalid_argument`.
  - `unsupported`/`format_error` from readable archive bytes become report-level fatal diagnostics via `report_from_open_error`.
  - `append_target_family_warning` warns on either `expected_type` or `expected_variant` mismatch using `target_family_mismatch`.
  - `append_entry_warnings` warns for BSA embedded names and compressed sound-like payloads.
  - `validate_extractability` streams entries to a discard sink and converts extraction failures into report-level diagnostics.
  - Production behavior looks consistent with the documented model; start with tests/docs, then only patch production if the expanded test matrix fails.

### Current tests and gaps

- `tests/unit/validation_api_tests.cpp`
  - Best target for S04 runtime proof.
  - Current test `validation_api accepts generated fixture archives` has only 4 success cases and should become the compact variant matrix.
  - Current test `validation_api accepts writer-produced archives` validates representative writer outputs but only Fallout 4 BA2 targets; it does not cover Starfield BA2 v3 method `0`/`3` validation.
  - Existing malformed and extractability cap tests are good; keep them code/category-based.

- `tests/unit/compatibility_warning_tests.cpp`
  - Covers every current public warning code and an `expected_type` target mismatch.
  - If S04 adds expected-variant mismatch proof, it can go here or in `validation_api_tests.cpp`; prefer `validation_api_tests.cpp` if the goal is consolidated validation-options behavior.

- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
  - Best target for docs-policy guardrails after the matrix is updated.
  - Add a focused policy case requiring variant-specific validation evidence strings for `tes4_v104`, `tes4_v105`, `ba2_gnrl_sfv2`, `ba2_gnrl_sfv3`, `ba2_dx10_sfv3`, and Starfield compression methods `0`/`3` if the docs now claim `Proven`.

- `docs/coverage-audit-matrix.md`
  - Currently marks Validation API behavior as `Partial` for TES4-family BSA, BA2 GNRL, and BA2 DX10 and lists `COV-GAP-001` as medium risk.
  - After tests land, update those validation rows to `Proven` or explicitly state exactly what is now fixed and what remains partial. Avoid internal milestone/slice IDs.

- `docs/compatibility-evidence.md`
  - Currently says validation API tests cover representative fixtures and leaves per-variant validation granularity to the matrix.
  - After S04, mention the direct validation success matrix and writer-produced Starfield compression-method validation proof.

- `docs/public-api-reality-check.md`
  - Currently routes `COV-GAP-001` to validation/result behavior work.
  - If S04 closes the gap, update the downstream routing table to say fixed by direct validation matrix/proof; no facade/API helper needed.

### Generated fixture facts useful for tests

Generated success archives currently present under `tests/fixtures/generated/archives/`:

- `tes3_success.bsa`
- `tes4_v103.bsa`, `tes4_v104.bsa`, `tes4_v105.bsa`
- `ba2_gnrl_fo4.ba2`, `ba2_gnrl_sfv2.ba2`, `ba2_gnrl_sfv3.ba2`
- `ba2_dx10_fo4.ba2`, `ba2_dx10_sfv3.ba2`

Fixture-specific warnings/metadata watch-outs:

- `tes4_v104.bsa` and `tes4_v105.bsa` have embedded-name entries, so validation success may legitimately include `bsa_embedded_name_compatibility_risk` warnings. Do **not** assert `warnings.empty()` for all success rows. Instead assert `report.valid`, `errors.empty()`, metadata matches, and no `target_family_mismatch` when expected type/variant match.
- `ba2_gnrl_sfv2.ba2` is Starfield v2 and exposes `starfield_unknown1/2`, but public metadata does not expose a `compression_method` for v2.
- `ba2_gnrl_sfv3.ba2` exposes `compression_method == 3`.
- `ba2_dx10_sfv3.ba2` exposes `compression_method == 3` and default compression `lz4_block`.
- A generated BA2 DX10 Starfield method `0` archive does not exist; cover method `0` through a writer-produced archive using `ba2_dx10_writer_options::starfield_compression_method = 0`.

## Recommended implementation seams

### Seam 1 — Runtime validation stabilization proof

File: `tests/unit/validation_api_tests.cpp`.

Recommended changes:

1. Extend `validation_archive_case` with fields such as:
   - `file_name`
   - `expected_type`
   - `expected_variant`
   - `expected_version`
   - optional expected BA2 `compression_method`
2. Add helper(s):
   - `bool report_has_warning_code(const validation_report&, compatibility_warning_code)`.
   - A `require_valid_archive_case` helper that sets `validation_options::expected_type`, `expected_variant`, and `validate_entry_extractability = true`; validates `report.valid`, `report.is_valid()`, `errors.empty()`, metadata type/variant/version; and asserts no `target_family_mismatch` warning when expectations match.
3. Expand `validation_api accepts generated fixture archives` to all 9 generated success archives listed above.
4. Add direct Starfield writer compression-method validation proof:
   - BA2 GNRL v3 method `0` (deflate) and method `3` (raw LZ4 block) writer-produced archives, preferably with `archive_compression_policy::all_compressed` so the route is real.
   - BA2 DX10 Starfield v3 method `0` and method `3` writer-produced archives using a generated DDS source such as `tests/fixtures/generated/source/ba2_dx10_bc1_unorm.dds`.
   - Validate with extractability enabled and assert Starfield metadata/compression method.
5. Optional but useful: add an expected-variant mismatch test using `validation_options::expected_variant` to prove the currently untested public option warns with `target_family_mismatch` while leaving the archive valid.

Expected outcome: if production is already correct, this is mostly test expansion. If a route fails, the failure will identify a real validation/extraction path to stabilize.

### Seam 2 — Documentation and matrix update

Files:

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `docs/public-api-reality-check.md`

Recommended changes:

- Change validation API behavior rows for TES4-family BSA, BA2 GNRL, and BA2 DX10 from `Partial` to `Proven` only after the runtime tests pass.
- Update `COV-GAP-001` to a fixed/closed row or move it under a fixed-gap note. Preserve the gap ID for S05 traceability, but do not use internal slice/milestone identifiers in public docs.
- Keep optional local game/BSArchPro evidence explicitly advisory.
- Do not claim exhaustive real-game or BSArchPro compatibility; this slice only proves repository-reproducible validation-success granularity.

### Seam 3 — Docs-policy guardrail

File: `tests/unit/coverage_audit_matrix_docs_tests.cpp`.

Add a focused policy test that requires the public matrix/catalog to mention direct validation proof for:

- `tes4_v104.bsa` and `tes4_v105.bsa`
- `ba2_gnrl_sfv2.ba2` and `ba2_gnrl_sfv3.ba2`
- `ba2_dx10_sfv3.ba2`
- Starfield compression methods `0` and `3` where writer-produced validation proof is used
- `tests/unit/validation_api_tests.cpp`

This keeps the fixed gap from regressing back to prose-only claims.

## First proof to build

Start with the runtime test seam in `tests/unit/validation_api_tests.cpp`. It is the highest-value unblocker because it answers `COV-GAP-001` directly and may reveal real behavior failures before docs are updated.

Suggested first proof order:

1. Add/expand the validation API variant matrix.
2. Run:
   ```powershell
   cmake --build --preset windows-msvc-debug-static --target libbsa_tests
   ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure
   ```
3. Only after it passes, update docs and docs-policy tests.

## Verification plan

Focused verification after implementation:

```powershell
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target generate_tes4_bsa_fixtures generate_ba2_gnrl_fixtures generate_ba2_dx10_fixtures libbsa_tests
ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure
ctest --preset windows-msvc-debug-static -L compatibility_warning --output-on-failure
ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure
ctest --preset windows-msvc-debug-static -R validate_fixture_manifests --output-on-failure
```

If docs/public API wording changes are substantial, also run:

```powershell
ctest --preset windows-msvc-debug-static -L docs_policy --output-on-failure
ctest --preset windows-msvc-debug-static -R "public_include_boundary|export_surface_policy" --output-on-failure
```

Keep CTest label/pattern runs serialized in this build tree to avoid the known Catch2 `PRE_TEST` discovery race noted by S03.

## Risks and watch-outs

- Do not assert exact diagnostic message text. Use `error_code`/`compatibility_warning_code`; substrings are acceptable only where an existing test already needs a human diagnostic sanity check.
- Do not treat valid warnings as invalid archives. Embedded-name TES4 fixtures should remain `valid` with warning(s).
- Do not add new public warning codes for TES3 or BA2 DX10 unless a concrete compatibility risk and default proof exist; that is `COV-GAP-004`, currently deferred.
- Do not change `libbsa::result<T>`, `error_code`, or public validation structs for this slice unless tests expose an actual inconsistency.
- Do not touch `TES5Edit/`.
- Observed working tree status during research already had modified generated manifest files under `tests/fixtures/generated/...`; executors should inspect `git diff` before staging so pre-existing/generated changes are not accidentally attributed to S04.

## Sources inspected

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `docs/public-api-reality-check.md`
- `.gsd/REQUIREMENTS.md`
- `include/libbsa/result.hpp`
- `include/libbsa/archive.hpp`
- `include/libbsa/validation.hpp`
- `include/libbsa/writer.hpp`
- `src/validation.cpp`
- `src/archive.cpp`
- `src/formats/ba2/ba2_format_detector.cpp`
- `tests/unit/validation_api_tests.cpp`
- `tests/unit/compatibility_warning_tests.cpp`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/unit/host_path_correctness_boundary_tests.cpp`
- Generated success manifests under `tests/fixtures/generated/archives/`

## Research artifacts

- `gsd_exec ce8061b1-abb4-481d-a904-49bead052aa6` — result/error/validation reference scan.
- `gsd_exec 08d47ca2-6d9a-4f50-902e-bcd97b8c6e0d` — baseline `validation_api` CTest pass.
- `gsd_exec e0f18d57-cc65-47d6-9fff-0e5310389bbb` — CTest validation/compatibility/docs-policy test inventory.
- `gsd_exec 75712116-bafb-434d-9e86-9fd4088589eb` — direct validation fixture coverage comparison.
- `gsd_exec 61c9d9fb-f2f4-482c-8404-9fa1266b22f6` — skill discovery for C++/Catch2/CTest validation work.
