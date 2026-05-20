# S03: Fixture and Round-trip Gap Closure — Research

## Summary
The fixture/round-trip surface is already broad and well-proven across all implemented families. Generated fixtures exist for TES3, TES4-family, BA2 GNRL, and BA2 DX10; the writer tests, reader dispatch tests, and validation tests already cover the major reopen/extract/round-trip paths.

After reviewing the matrix and the current test surface, I did not find a separate high-risk S03 tranche that clearly needs implementation work right now. The only explicit partial-proof gap in the matrix is `COV-GAP-001`, which is validation-success granularity for specific variants/methods; that is a validation-evidence issue and belongs more naturally with S04 than with fixture/round-trip closure.

## Findings
- `tests/fixtures/generated/archives/` contains committed success and malformed fixtures for all four current families, plus manifests that encode default evidence.
- `tests/fixtures/README.md` documents the legal synthetic-fixture policy and the generator targets for TES3 and TES4-family archives.
- `tests/unit/archive_reader_dispatch_tests.cpp` exercises representative backends end-to-end across TES3, TES4, BA2 GNRL, and BA2 DX10, including lookup, extraction, `extract_bytes`, and bulk extraction behavior.
- `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp` already reopen writer-produced archives and verify extracted bytes and metadata.
- `tests/unit/validation_api_tests.cpp` validates writer-produced archives and generated fixtures, but the remaining gap there is variant-specific validation-success enumeration rather than round-trip failure.
- `docs/coverage-audit-matrix.md` marks round-trip/reopen as `Proven` for TES3, TES4-family, BA2 GNRL, and BA2 DX10; the remaining open item is not round-trip coverage.
- `tests/unit/compatibility_matrix_tests.cpp` proves the malformed matrix spans the family set and evidence types, which supports fixture provenance but does not reveal a missing round-trip tranche.

## Implementation Landscape
### Files and purpose
- `tests/fixtures/generated/archives/` — committed fixture outputs and manifests.
- `tests/fixtures/generated/source/` — synthetic source payloads used by texture and writer fixtures.
- `tests/fixtures/README.md` — fixture provenance and regeneration policy.
- `tests/unit/archive_reader_dispatch_tests.cpp` — representative multi-family open/list/extract/bulk-extract proof.
- `tests/unit/tes3_bsa_writer_tests.cpp` — TES3 writer reopen/extract proof.
- `tests/unit/tes4_bsa_writer_tests.cpp` — TES4-family writer reopen/extract proof for all target profiles.
- `tests/unit/ba2_gnrl_writer_tests.cpp` — BA2 GNRL writer reopen/extract proof across Fallout 4 and Starfield v2/v3 routes.
- `tests/unit/ba2_dx10_writer_tests.cpp` — BA2 DX10 writer reopen/extract proof across Fallout 4 and Starfield v3 routes.
- `tests/unit/validation_api_tests.cpp` — validation proof over generated fixtures and writer-produced archives.
- `tests/unit/compatibility_matrix_tests.cpp` — malformed matrix provenance and evidence-span checks.
- `tests/CMakeLists.txt` — generator targets and the test graph wiring.

### Natural seams
1. **Fixture-manifest seam**: if a planner wants one more proof row, add it through an existing generated manifest rather than inventing a new fixture format.
2. **Writer round-trip seam**: the writer tests are already the natural place for any missing family/route reopen proof.
3. **Reader dispatch seam**: `archive_reader_dispatch_tests.cpp` is the best place for one representative cross-family behavior check, but that surface already looks complete.
4. **Validation-success seam**: if the audit wants direct per-variant success rows, they should be added in `validation_api_tests.cpp`; that is not a round-trip gap.

### First proof
The fastest evidence that S03 does **not** currently have a major hole is:
- `tests/unit/tes4_bsa_writer_tests.cpp` — raw output reopens for every TES4 target profile.
- `tests/unit/ba2_gnrl_writer_tests.cpp` — raw Starfield v2/v3 defaults reopen through public metadata.
- `tests/unit/ba2_dx10_writer_tests.cpp` — Fallout 4 and Starfield v3 routes reopen and preserve texture payload bytes.
- `tests/unit/archive_reader_dispatch_tests.cpp` — representative backends cover lookup, extraction, and bulk extraction.
- `tests/unit/validation_api_tests.cpp` — writer-produced archives validate.

These already cover the expensive round-trip and fixture seams that S03 was supposed to find.

## Risks / Constraints
- TES5Edit/ remains read-only and must not be used as a fixture workspace.
- Committed fixture bytes must stay legal synthetic data; do not widen S03 using local copyrighted corpus checks.
- Windows-only support remains in force; no cross-platform detour is needed.
- The package-consumer runtime proof gap is real but low-risk and belongs to the release/package lane, not this slice.
- If the planner insists on closing something here, the remaining candidate is validation-success granularity (`COV-GAP-001`), which is better routed to S04 because it is an error/validation evidence problem rather than a round-trip defect.

## Verification
Recommended proof commands for any future S03 follow-up:
- `cmake --preset windows-msvc-debug-static`
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`
- `ctest --preset windows-msvc-debug-static -R "(roundtrip|fixture|archive_reader_dispatch|tes3_bsa_writer|tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|validation_api)" --output-on-failure`
- `cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures generate_tes4_bsa_fixtures generate_ba2_gnrl_fixtures generate_ba2_dx10_fixtures`

If a gap is still demanded after that evidence, it should likely be reclassified to S04 or S05 rather than forced into S03.

## Skills Discovered
Installed during this research pass:
- `cmake` — `mohitmishra786/low-level-dev-skills@cmake`
- `cpp-testing` — `affaan-m/everything-claude-code@cpp-testing`

## Recommendation
Do not allocate implementation capacity to S03 unless a new evidence gap emerges during S04/S05 integration. The current fixture and round-trip story is already strong enough that the remaining known gap is validation granularity, not round-trip closure.