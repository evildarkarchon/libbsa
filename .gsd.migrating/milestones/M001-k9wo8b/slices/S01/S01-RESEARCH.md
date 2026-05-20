# S01 Coverage Audit Matrix Research

## Summary

- libbsa already has broad synthetic-fixture proof across the four current archive families: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- I did **not** find a missing current family or a docs-vs-tests contradiction in the audited surface.
- The main deliverable gap for S01 is therefore **synthesis**: a single human-first coverage matrix that cross-links the existing proof instead of implying that support is less proven than it really is.
- The current `tests/fixtures/generated/compatibility_matrix.json` is a **narrow malformed-hardening submatrix** (`matrix_kind: phase11_malformed_hardening`), not the full family/axis support matrix that S01 needs.
- Default proof remains Windows/MSVC/vcpkg-only and must continue to avoid local copyrighted fixtures; `TES5Edit/` stays read-only.

## Active requirements this slice owns or supports

- **Owns:** R001, R002
- **Supports:** R005, R006, R007, R008, R009

## What already exists

### Strong family coverage

- **TES3 BSA**
  - Reader proof: `tests/unit/tes3_bsa_reader_tests.cpp`
  - Writer proof: `tests/unit/tes3_bsa_writer_tests.cpp`
  - Round-trip proof: writer output reopens through reader lookup/extraction; `tests/unit/validation_api_tests.cpp` also accepts writer-produced archives
  - Malformed proof: generated malformed fixtures + `tes3_bsa_malformed` cases
  - Validation proof: `tests/unit/validation_api_tests.cpp`
  - API/docs proof: `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `docs/target-format-guide.md`, `docs/integration-examples.md`
  - Compatibility: no public warning code surface, which matches the docs’ raw/uncompressed TES3 story

- **TES4-family BSA**
  - Reader proof: `tests/unit/tes4_bsa_reader_tests.cpp`
  - Writer proof: `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/bsa_writer_execution_tests.cpp`
  - Round-trip proof: raw output reopens; target-default compression round-trips; compressed entries remain stable
  - Malformed proof: generated malformed fixtures + `tes4_bsa_malformed_open` cases + compatibility matrix rows
  - Validation proof: `tests/unit/validation_api_tests.cpp`
  - Compatibility proof: `tests/unit/compatibility_warning_tests.cpp` covers `compressed_sound_payload` and `bsa_embedded_name_compatibility_risk`
  - API/docs proof: `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`

- **BA2 GNRL**
  - Reader proof: `tests/unit/ba2_gnrl_reader_tests.cpp`
  - Writer proof: `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`
  - Round-trip proof: Fallout 4 raw output reopens; Starfield v2/v3 defaults reopen; compressed entries stay byte-stable
  - Malformed proof: generated malformed fixtures + compatibility matrix rows
  - Validation proof: `tests/unit/validation_api_tests.cpp`
  - Compatibility proof: `tests/unit/compatibility_warning_tests.cpp` covers `target_family_mismatch`
  - API/docs proof: `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`

- **BA2 DX10**
  - Reader/metadata proof: `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`
  - Writer proof: `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`
  - Round-trip proof: FO4 deflate and Starfield v3 raw LZ4 archives reopen through `archive_reader`; extraction reconstructs DDS bytes
  - Malformed proof: `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, generated malformed fixtures, compatibility matrix rows
  - Validation proof: `tests/unit/validation_api_tests.cpp`
  - Compatibility proof: only the family-mismatch warning surface is public; no DX10-specific warning code exists, which is consistent with current docs/tests
  - API/docs proof: `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`

### Cross-cutting proof assets

- `tests/unit/archive_reader_dispatch_tests.cpp` proves representative reader operations across TES3, TES4, BA2 GNRL (FO4 and Starfield v3), and BA2 DX10.
- `tests/unit/archive_reader_dispatch_policy_tests.cpp` verifies the public reader methods do not leak backend identity or repeat family dispatch internally.
- `tests/unit/bulk_extraction_tests.cpp` covers serial and parallel bulk extraction semantics, duplicate-request coalescing, per-entry failures, partial sink writes, and bounded chunking across TES3, TES4, BA2 GNRL, and BA2 DX10 raw payloads.
- `tests/unit/compatibility_warning_tests.cpp` covers every public warning code.
- `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, and `tests/unit/target_format_policy_tests.cpp` keep the public docs aligned with the API story.
- `tests/unit/public_include_boundary_tests.cpp` and `tests/unit/export_surface_policy_tests.cpp` prove the public headers stay dependency-light and do not leak private codec/DirectXTex/Windows details.
- `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`, and `tests/package-consumer/verify-runtime-dll-copy.cmake` provide installed-package proof for open/list/extract, bulk extract, all four writers, validation, and error handling.
- `tests/fixtures/README.md` and `docs/compatibility-evidence.md` define the mandatory default evidence path: committed generated fixtures and writer-output archives, with local game/BSArchPro checks explicitly opt-in.

## Existing malformed-hardening submatrix

`tests/fixtures/generated/compatibility_matrix.json` is already a durable, machine-checked malformed matrix:

- `matrix_kind`: `phase11_malformed_hardening`
- rows: **17**
- families: `tes3_bsa:4`, `tes4_bsa:4`, `ba2_gnrl:4`, `ba2_dx10:5`
- categories: `truncated_structure:4`, `invalid_payload_span:4`, `duplicate_canonical_path:2`, `decompression_failure:3`, `oversized_arithmetic:1`, `unsupported_route:2`, `dds_chunk_layout:1`
- evidence types: `manifest`, `test`
- phases: `open`, `extraction`

That submatrix is excellent proof for malformed input hardening, but it is **not** yet the broader family/axis support matrix S01 needs.

## Family coverage snapshot

| Family | Reader | Writer | Round-trip | Malformed | Validation | Compatibility | API | Docs | Main evidence |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| TES3 BSA | ✅ | ✅ | ✅ | ✅ | ✅ | n/a | ✅ | ✅ | `tes3_bsa_reader_tests.cpp`, `tes3_bsa_writer_tests.cpp`, `validation_api_tests.cpp`, `bulk_extraction_tests.cpp` |
| TES4-family BSA | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ (`compressed_sound_payload`, `bsa_embedded_name_compatibility_risk`) | ✅ | ✅ | `tes4_bsa_reader_tests.cpp`, `tes4_bsa_writer_tests.cpp`, `compatibility_warning_tests.cpp`, `validation_api_tests.cpp`, `target_format_policy_tests.cpp` |
| BA2 GNRL | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ (`target_family_mismatch`) | ✅ | ✅ | `ba2_gnrl_reader_tests.cpp`, `ba2_gnrl_writer_tests.cpp`, `ba2_writer_execution_tests.cpp`, `validation_api_tests.cpp`, `compatibility_warning_tests.cpp` |
| BA2 DX10 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ (`target_family_mismatch`) | ✅ | ✅ | `ba2_dx10_metadata_tests.cpp`, `ba2_dx10_parser_tests.cpp`, `ba2_dx10_extraction_tests.cpp`, `ba2_dx10_writer_tests.cpp`, `ba2_dx10_malformed_tests.cpp`, `validation_api_tests.cpp` |

### Notes on the snapshot

- All four families have direct reader/writer proof plus malformed and validation coverage.
- The strongest family-specific risk concentration is **BA2 DX10** because the proof spans DirectXTex, DDS layout, chunk assembly, compression routing, and a consuming writer lifecycle.
- TES4-family compatibility warnings are intentionally visible because they are valid-but-risky rather than hard failures.
- BA2 GNRL and BA2 DX10 only expose the family-mismatch compatibility warning; that matches the current public warning taxonomy.

## Ranked claim-risk gaps

1. **Missing consolidated matrix artifact** — the highest-priority gap is not a code defect, but the absence of a single human-first matrix that ties all of the above proof together with explicit evidence grading.
2. **BA2 DX10 claim sensitivity** — not unproven, but highest-risk because the surface is the most complex and most implementation-dependent; the matrix should cross-link the DX10 tests, docs, and package-consumer proof prominently.
3. **TES4 compatibility policy visibility** — the warning surface is covered, but it is policy-sensitive and should be kept explicit in the matrix so support claims don’t collapse into “it works” without caveats.
4. **Bulk extraction is representative, not per-family exhaustive** — request ordering, duplicate coalescing, and per-entry failure behavior are well covered, but much of that semantic proof is centered on one synthetic bulk reader; the matrix should say that clearly.
5. **Optional local corpus checks remain deferred by design** — `tests/unit/local_game_fixture_tests.cpp` is opt-in and skipped when the environment is absent. That is a deliberate gap, not a defect, but the matrix should mark it as such.

## Likely follow-up seams for the planner

- `tests/fixtures/generated/compatibility_matrix.json` — existing malformed submatrix that should remain a source of truth for malformed hardening rows.
- `tests/unit/compatibility_matrix_tests.cpp` — likely place to extend assertions once the broader matrix exists.
- `tests/unit/validation_api_tests.cpp` and `tests/unit/compatibility_warning_tests.cpp` — best places to anchor validation and compatibility rows.
- `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp` — API/docs proof seams.
- `tests/unit/tes3_bsa_*`, `tests/unit/tes4_bsa_*`, `tests/unit/ba2_gnrl_*`, `tests/unit/ba2_dx10_*` — family-specific proof seams.
- `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`, `tests/CMakeLists.txt` — installed package-consumer proof seams.
- `docs/api-mainpage.md`, `docs/integration-examples.md`, `docs/target-format-guide.md`, `docs/compatibility-evidence.md`, `docs/thread-safety.md` — docs seams that should be cross-linked directly from the matrix.

## Recommended verification for the follow-up work

- `ctest --preset windows-msvc-debug-static --output-on-failure -R "compatibility_matrix|validation_api|compatibility_warning|public_include_boundary|target_format_policy|docs_policy|thread_safety_policy|export_surface"`
- `ctest --preset windows-msvc-debug-static --output-on-failure -R "tes3|tes4|ba2_gnrl|ba2_dx10|bulk_extraction"`
- `ctest --preset windows-msvc-release-static --output-on-failure -R "package_consumer"`
- `ctest --preset windows-msvc-asan-static --output-on-failure` for the highest-risk writer/validation/DX10 surfaces if the matrix work touches implementation details.

## Bottom line

S01 does **not** need to discover a new supported family. It needs to turn already-strong, scattered proof into a durable coverage matrix that makes the support story honest, searchable, and easy for S02–S05 to consume.