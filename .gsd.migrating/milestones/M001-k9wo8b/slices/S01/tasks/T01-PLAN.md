---
estimated_steps: 13
estimated_files: 2
skills_used: []
---

# T01: Write the human-first coverage matrix

Expected executor skills: write-docs, verify-before-complete.

Why: S01's primary deliverable is the durable support-truth artifact, not new archive behavior. The existing proof is scattered across public headers, docs, generated fixtures, unit tests, and package-consumer checks; this task consolidates that evidence into one public matrix without turning optional local evidence into default proof.

Do:
1. Create `docs/coverage-audit-matrix.md` as a human-first document, not a machine-schema-first inventory.
2. Start with a short scope/evidence-policy section that defines `Proven`, `Partial`, `Missing`, `Deferred`, and `N/A`. State that `Proven` requires default legal/generated fixtures, always-on Catch2/CTest tests, package-consumer checks, or docs-policy tests.
3. Add a matrix for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10. Add subrows only where material: TES4 v103/v104/v105, Fallout 4 vs Starfield BA2 v2/v3, and deflate vs LZ4-frame/raw-LZ4 routes if proof/risk differs.
4. Use the required axes: reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof.
5. Cite concrete default evidence near each cell or row using repository-relative paths, including public headers, docs, unit tests, generated fixture metadata, and package-consumer smoke sources.
6. Explain that `tests/fixtures/generated/compatibility_matrix.json` is a malformed-hardening submatrix, not the full family/axis support matrix.
7. Add a ranked gap list with IDs such as `COV-GAP-001`, risk, affected family/axis, evidence source, and downstream route. If the audit confirms no high-risk executable-proof gaps, say that explicitly and rank any lower-risk/deferred gaps truthfully rather than fabricating failures.
8. Add an advisory evidence section for `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED`, making clear that absent local/copyrighted inputs do not block default green status and present local-only checks do not replace default proof.
9. Update `docs/compatibility-evidence.md` with a concise discoverability link to the new matrix. Keep public docs free of `.gsd` paths and milestone/slice IDs.

Done when: the new doc can be read independently by a future contributor, every family/axis claim has proof or a gap/deferral, and existing support docs point readers to it.

## Inputs

- `.gsd/REQUIREMENTS.md`
- `include/libbsa/archive.hpp`
- `include/libbsa/writer.hpp`
- `include/libbsa/validation.hpp`
- `docs/target-format-guide.md`
- `docs/compatibility-evidence.md`
- `docs/api-mainpage.md`
- `docs/integration-examples.md`
- `tests/fixtures/README.md`
- `tests/fixtures/generated/compatibility_matrix.json`
- `tests/unit/tes3_bsa_reader_tests.cpp`
- `tests/unit/tes3_bsa_writer_tests.cpp`
- `tests/unit/tes4_bsa_reader_tests.cpp`
- `tests/unit/tes4_bsa_writer_tests.cpp`
- `tests/unit/ba2_gnrl_reader_tests.cpp`
- `tests/unit/ba2_gnrl_writer_tests.cpp`
- `tests/unit/ba2_dx10_metadata_tests.cpp`
- `tests/unit/ba2_dx10_parser_tests.cpp`
- `tests/unit/ba2_dx10_extraction_tests.cpp`
- `tests/unit/ba2_dx10_writer_tests.cpp`
- `tests/unit/ba2_dx10_malformed_tests.cpp`
- `tests/unit/archive_reader_dispatch_tests.cpp`
- `tests/unit/bulk_extraction_tests.cpp`
- `tests/unit/compatibility_warning_tests.cpp`
- `tests/unit/validation_api_tests.cpp`
- `tests/unit/public_include_boundary_tests.cpp`
- `tests/unit/export_surface_policy_tests.cpp`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
- `tests/package-consumer/main.cpp`
- `tests/package-consumer/smoke.cmake`
- `tests/package-consumer/verify-runtime-dll-copy.cmake`

## Expected Output

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`

## Verification

test -s docs/coverage-audit-matrix.md
grep -q "TES3 BSA" docs/coverage-audit-matrix.md
grep -q "TES4-family BSA" docs/coverage-audit-matrix.md
grep -q "BA2 GNRL" docs/coverage-audit-matrix.md
grep -q "BA2 DX10" docs/coverage-audit-matrix.md
grep -q "Ranked gap list" docs/coverage-audit-matrix.md
grep -q "coverage-audit-matrix.md" docs/compatibility-evidence.md

## Observability Impact

No runtime signals added. The matrix itself improves future diagnostic visibility by putting evidence paths, support status, and gap IDs in one stable public document.
