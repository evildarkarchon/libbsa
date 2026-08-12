# Coverage Audit Matrix

This document is the human-readable support-truth matrix for libbsa's currently implemented archive families. It answers one question: for each family and capability axis, what is proven by default, what is only partially proven, and what is intentionally advisory or deferred?

It is not a promise that every real-world archive has been compared against BSArchPro or official game data. Default support claims below are based on committed legal fixtures, writer-produced archives, always-on Catch2/CTest coverage, and package-consumer smoke checks.

Documentation accuracy is not part of the default proof set. It was previously asserted by tests that substring-matched documentation prose, which could only detect a missing word rather than a wrong statement, and which broke on ordinary rewording. Those tests were removed; documentation is kept accurate by review.

## Scope and evidence policy

### Status vocabulary

- **Proven** — The claim has default evidence from committed legal/generated fixtures, writer-output archives exercised by always-on Catch2/CTest tests, or package-consumer checks. Optional local game/BSArchPro inputs are not required for this status.
- **Documented** — The claim is described in public documentation, and that is all. No automated check enforces it. Used for the documentation axis, which is a review responsibility rather than a test result.
- **Partial** — The implementation has meaningful default proof, but at least one important variant, compression route, or public-policy angle is indirect or representative rather than directly covered by a family/axis test.
- **Missing** — A support claim would currently lack default proof. Missing rows should become work items before the claim is repeated in user-facing docs.
- **Deferred** — The work is intentionally outside the current default proof contract, usually because it depends on local copyrighted inputs, a future compatibility campaign, performance/stress work, or a release-readiness pass.
- **N/A** — The axis does not apply to the family or to the current public support claim.

### Default evidence sources

Use these paths as the default evidence set when reading the matrix:

- Public API surface: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`.
- Public support docs: `docs/target-format-guide.md`, `docs/compatibility-evidence.md`, `docs/api-mainpage.md`, `docs/integration-examples.md`, `tests/fixtures/README.md`.
- Public/package-consumer proof: `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`, `tests/package-consumer/verify-runtime-dll-copy.cmake`, plus the `package_consumer_smoke` and `package_consumer_runtime_dll_copy` CTest gates referenced by `tests/unit/target_format_policy_tests.cpp`. The `package_consumer_smoke` gate installs libbsa, configures an external consumer against the installed `libbsa::libbsa` target, and then creates, opens, validates, and extracts writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives without source-tree or local-corpus fixtures.
- Fixture policy and provenance: `tests/fixtures/README.md` and the generated fixture manifests under `tests/fixtures/generated/archives/`.
- Malformed-hardening index: `tests/fixtures/generated/compatibility_matrix.json` plus the family malformed manifests it references.

Optional local corpora are advisory evidence only; see [Advisory local evidence](#advisory-local-evidence).

## Family/axis matrix

### TES3 BSA

Material subrow: **TES3/Morrowind BSA** only. TES3 has no compression route; writer output is raw/uncompressed.

| Axis | Status | Default evidence | Notes |
|---|---|---|---|
| Reader/open/list metadata | Proven | `include/libbsa/archive.hpp`; `tests/unit/tes3_bsa_reader_tests.cpp`; `tests/fixtures/generated/archives/tes3_success_manifest.json` | Reader tests prove detection, metadata, deterministic entries, canonical lookup, TES3 hash values, and data-section-relative offset handling exposed as archive-absolute `entry_metadata::payload_offset`. |
| Extraction | Proven | `tests/unit/tes3_bsa_reader_tests.cpp`; `tests/fixtures/generated/archives/tes3_success_manifest.json` | Tests cover streaming extraction and `extract_bytes` from generated TES3 payload offsets, including sink-write enforcement. |
| Writer | Proven | `include/libbsa/writer.hpp`; `tests/unit/tes3_bsa_writer_tests.cpp`; `tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json` | Public writer API is `tes3_bsa_writer`; tests cover byte-accurate raw tables, hash order, path validation, duplicate rejection, overwrite policy, and publication helpers. |
| Round-trip/reopen | Proven | `tests/unit/tes3_bsa_writer_tests.cpp`; `tests/unit/validation_api_tests.cpp` | Writer-output archives reopen through `archive_reader` lookup/extraction; validation API accepts writer-produced TES3 archives with extractability enabled. |
| Malformed handling | Proven | `tests/unit/tes3_bsa_reader_tests.cpp`; `tests/fixtures/generated/archives/tes3_malformed_manifest.json`; `tests/fixtures/generated/compatibility_matrix.json`; `tests/unit/validation_api_tests.cpp` | Default proof covers truncated structures, invalid spans, duplicates, count/offset inconsistencies, hash/order errors, and stable `format_error` reporting. |
| Validation API behavior | Proven | `include/libbsa/validation.hpp`; `tests/unit/validation_api_tests.cpp` | Generated TES3 success archives and writer-produced TES3 archives are accepted; malformed TES3 rows report structured validation diagnostics. |
| Compatibility warnings | N/A | `include/libbsa/validation.hpp`; `tests/unit/compatibility_warning_tests.cpp`; `docs/compatibility-evidence.md` | The current public warning catalog has no TES3-specific compatibility warning. This is not a TES3 support gap unless a known TES3 compatibility risk is added to the public warning contract. |
| Public/package-consumer API proof | Proven | `tests/package-consumer/main.cpp`; `docs/integration-examples.md`; `tests/unit/public_include_boundary_tests.cpp`; `tests/unit/export_surface_policy_tests.cpp` | The installed umbrella header and package-consumer smoke source compile/link representative reader, validation, `payload_sink`, and `tes3_bsa_writer` usage. The installed `package_consumer_smoke` runtime creates a writer-produced TES3 BSA, opens it through `archive_reader`, validates it through `validate_archive`, and extracts it through sink, `extract_bytes`, and bulk extraction paths. |
| Docs/support-claim proof | Documented | `docs/target-format-guide.md`; `docs/api-mainpage.md` | Public docs describe TES3 support, raw writer behavior, and dependency-light API shape. Accuracy is maintained by review, not by an automated check. |

### TES4-family BSA

Material subrows:

- **TES4-family BSA v103** — Oblivion-style target; raw and deflate extraction/writing are proven.
- **TES4-family BSA v104** — Fallout 3/Fallout New Vegas/Skyrim Legendary Edition style target; embedded-name stripping and raw/deflate routes are proven.
- **Skyrim SE/AE BSA v105** — LZ4-frame compressed payload route is proven.

| Axis | Status | Default evidence | Notes |
|---|---|---|---|
| Reader/open/list metadata | Proven | `include/libbsa/archive.hpp`; `tests/unit/tes4_bsa_reader_tests.cpp`; `tests/fixtures/generated/archives/tes4_v103_manifest.json`; `tests/fixtures/generated/archives/tes4_v104_manifest.json`; `tests/fixtures/generated/archives/tes4_v105_manifest.json` | Reader tests open byte-driven variants, expose archive-level metadata, materialize table paths, hashes, sizes, embedded-name metadata, and deterministic lookup behavior. |
| Extraction | Proven | `tests/unit/tes4_bsa_reader_tests.cpp`; `tests/fixtures/generated/archives/tes4_v103_manifest.json`; `tests/fixtures/generated/archives/tes4_v104_manifest.json`; `tests/fixtures/generated/archives/tes4_v105_manifest.json` | v103 raw/deflate, v104 embedded-name raw/deflate, and v105 LZ4-frame routes all stream fixture bytes through sinks and `extract_bytes`; corrupt compressed payloads and size mismatches fail. |
| Writer | Proven | `include/libbsa/writer.hpp`; `tests/unit/tes4_bsa_writer_tests.cpp` | Public writer target profiles are `tes4_bsa_target::oblivion`, `fallout3`, and `skyrim_se`; tests cover raw output reopening for every target, compression policy/overrides, embedded-name options, DDS target gating, dedupe policy, path validation, source I/O errors, and publish/overwrite behavior. |
| Round-trip/reopen | Proven | `tests/unit/tes4_bsa_writer_tests.cpp`; `tests/unit/archive_reader_dispatch_tests.cpp`; `tests/unit/validation_api_tests.cpp` | Writer raw output reopens for every target profile; target-default compression round-trips inherited entries; representative writer-produced TES4 archives validate with extractability enabled. |
| Malformed handling | Proven | `tests/unit/tes4_bsa_reader_tests.cpp`; `tests/fixtures/generated/archives/malformed_manifest.json`; `tests/fixtures/generated/compatibility_matrix.json`; `tests/unit/validation_api_tests.cpp` | Default proof covers unsupported future versions, malformed tables, oversized counts before allocation, inconsistent offsets, payload-span failures, hash mismatches, duplicate canonical paths, corrupt compressed payloads, and validation diagnostics. |
| Validation API behavior | Proven | `include/libbsa/validation.hpp`; `tests/unit/validation_api_tests.cpp`; `tests/unit/compatibility_warning_tests.cpp` | The validation API directly accepts generated success fixtures `tes4_v103.bsa`, `tes4_v104.bsa`, and `tes4_v105.bsa` with extractability enabled, validates writer-produced TES4 archives, reports malformed BSA diagnostics, and keeps compatibility-warning proof on the public report surface. |
| Compatibility warnings | Proven | `include/libbsa/validation.hpp`; `tests/unit/compatibility_warning_tests.cpp`; `docs/compatibility-evidence.md` | Current public BSA warning codes are proven: compressed sound payloads and embedded-name compatibility risk are generated/writer-output tests, not local corpus assumptions. |
| Public/package-consumer API proof | Proven | `tests/package-consumer/main.cpp`; `docs/integration-examples.md`; `tests/unit/public_include_boundary_tests.cpp`; `tests/unit/export_surface_policy_tests.cpp` | Package-consumer source compiles representative `tes4_bsa_writer`, `archive_reader`, validation, extraction, and error-handling usage through `<libbsa/libbsa.hpp>`. The installed `package_consumer_smoke` runtime creates a writer-produced TES4-family BSA, opens it through `archive_reader`, validates it through `validate_archive`, and extracts it through sink, `extract_bytes`, and bulk extraction paths. |
| Docs/support-claim proof | Documented | `docs/target-format-guide.md`; `docs/api-mainpage.md` | Public docs cover v103, v104, v105, deflate, LZ4-frame, writer target policies, and compatibility warnings. Accuracy is maintained by review, not by an automated check. |

### BA2 GNRL

Material subrows:

- **Fallout 4 BA2 GNRL v1** — Deflate-capable general BA2 route is proven.
- **Fallout 4 next-gen BA2 GNRL v7/v8** — Read-only support is proven; the fixed header, variant, and deflate routing match v1. There is no writer target for these versions. Retail GNRL archives open, list, and resolve path lookups end to end. Extracting a compressed retail entry is still blocked by zlib-wrapped payload framing (issue #42), and three vanilla archives are still refused by the record-identity cross-check (issue #43).
- **Starfield BA2 GNRL v2** — Starfield unknown header fields plus deflate routing are proven.
- **Starfield BA2 GNRL v3** — `CompressionMethod == 0` deflate and `CompressionMethod == 3` raw-LZ4 block routes are proven by reader/writer tests.

| Axis | Status | Default evidence | Notes |
|---|---|---|---|
| Reader/open/list metadata | Proven | `include/libbsa/archive.hpp`; `tests/unit/ba2_gnrl_reader_tests.cpp`; `tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json`; `tests/fixtures/generated/archives/ba2_gnrl_sfv2_manifest.json`; `tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json` | Reader tests prove Fallout 4 metadata without Starfield fields, Starfield v2 unknown fields, Starfield v3 compression method metadata, filename table behavior, sparse payload opening, hash lookup, and canonical path lookup. |
| Extraction | Proven | `tests/unit/ba2_gnrl_reader_tests.cpp`; BA2 GNRL generated manifests under `tests/fixtures/generated/archives/` | Tests stream manifest-backed bytes, exercise compressed fallback routes, enforce partial-sink errors, and keep sparse/large-payload behavior bounded. |
| Writer | Proven | `include/libbsa/writer.hpp`; `tests/unit/ba2_gnrl_writer_tests.cpp` | Public writer target profiles are `ba2_gnrl_target::fallout4`, `starfield_v2`, and `starfield_v3`; tests cover raw Fallout 4, raw Starfield v2/v3 defaults, all-compressed target routing, per-entry raw/compressed overrides, Starfield header overrides, record flags, dedupe, source I/O, path validation, and publication behavior. |
| Round-trip/reopen | Proven | `tests/unit/ba2_gnrl_writer_tests.cpp`; `tests/unit/archive_reader_dispatch_tests.cpp`; `tests/unit/validation_api_tests.cpp` | Writer-output BA2 GNRL archives reopen through public metadata and extraction; representative Fallout 4 writer output validates with extractability enabled. |
| Malformed handling | Proven | `tests/unit/ba2_gnrl_reader_tests.cpp`; `tests/fixtures/generated/archives/ba2_gnrl_malformed_manifest.json`; `tests/fixtures/generated/compatibility_matrix.json`; `tests/unit/validation_api_tests.cpp` | Default proof covers truncated name tables, invalid payload spans, unsupported Starfield v3 compression methods, exact-size mismatches, duplicate paths, hash mismatches, oversized tables, and validation diagnostics. |
| Validation API behavior | Proven | `include/libbsa/validation.hpp`; `tests/unit/validation_api_tests.cpp`; `tests/unit/compatibility_warning_tests.cpp` | The validation API directly accepts generated success fixtures `ba2_gnrl_fo4.ba2`, `ba2_gnrl_sfv2.ba2`, and `ba2_gnrl_sfv3.ba2` with extractability enabled, validates representative writer-produced BA2 GNRL archives, validates writer-produced Starfield BA2 v3 method 0 deflate and method 3 raw LZ4 block routes, and malformed matrix rows cover BA2 GNRL failures. |
| Compatibility warnings | Proven | `include/libbsa/validation.hpp`; `tests/unit/compatibility_warning_tests.cpp`; `docs/compatibility-evidence.md` | Current BA2 warning proof covers caller-supplied target-family mismatch through generated writer-output validation. No GNRL-specific warning beyond the current catalog is claimed. |
| Public/package-consumer API proof | Proven | `tests/package-consumer/main.cpp`; `docs/integration-examples.md`; `tests/unit/public_include_boundary_tests.cpp`; `tests/unit/export_surface_policy_tests.cpp` | Package-consumer source compiles representative `ba2_gnrl_writer`, target enum, compression policy, reader, validation, and error-handling usage through the installed public header. The installed `package_consumer_smoke` runtime creates a writer-produced BA2 GNRL archive, opens it through `archive_reader`, validates it through `validate_archive`, and extracts it through sink, `extract_bytes`, and bulk extraction paths. |
| Docs/support-claim proof | Documented | `docs/target-format-guide.md`; `docs/api-mainpage.md` | Public docs cover Fallout 4 GNRL, Starfield v2/v3 GNRL, deflate, raw LZ4 block, writer target policies, and compatibility warnings. Accuracy is maintained by review, not by an automated check. |

### BA2 DX10

Material subrows:

- **Fallout 4 BA2 DX10 v1** — Texture metadata, DDS reconstruction, writer DDS validation, and deflate chunk route are proven.
- **Fallout 4 next-gen BA2 DX10 v7/v8** — Read-only header support is proven; the fixed header, variant, and deflate routing match v1. There is no writer target for these versions. Whole-archive opening of retail v7/v8 texture archives now works: the reader honours `FileTableOffset` wherever it points instead of using it to size the record table (issue #36).
- **Starfield BA2 DX10 v2** — Starfield texture metadata and formats with fixed deflate chunk routing are proven.
- **Starfield BA2 DX10 v3, `CompressionMethod == 3`** — Raw-LZ4 block texture chunks are proven.
- **Starfield BA2 DX10 v3, `CompressionMethod == 0`** — Deflate compatibility route is proven.

| Axis | Status | Default evidence | Notes |
|---|---|---|---|
| Reader/open/list metadata | Proven | `include/libbsa/archive.hpp`; `tests/unit/ba2_archive_opening_tests.cpp`; `tests/unit/ba2_dx10_metadata_tests.cpp`; `tests/unit/ba2_dx10_reader_tests.cpp`; `tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json`; `tests/fixtures/generated/archives/ba2_dx10_sfv3_manifest.json` | Reader tests prove Fallout 4 and Starfield v2/v3 texture archive opening, dependency-light texture metadata, chunk metadata, canonical lowercase paths, original spelling, sparse opening, chunk order validation, and hash checks. Physical order is not assumed: the committed fixtures place the filename table before the payload area while writer-output archives place it after, and both open. |
| Extraction | Proven | `tests/unit/ba2_dx10_extraction_tests.cpp`; BA2 DX10 generated manifests under `tests/fixtures/generated/archives/` | Tests reconstruct DDS bytes through sinks and `extract_bytes`, validate reconstructed DDS metadata through DirectXTex-backed test paths, and exercise raw, deflate, and LZ4-block chunk routes. |
| Writer | Proven | `include/libbsa/writer.hpp`; `tests/unit/ba2_dx10_writer_tests.cpp`; `tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json` | Public writer target profiles are `ba2_dx10_target::fallout4`, `starfield_v2`, and `starfield_v3`; tests cover DDS source manifest policy, malformed/unsupported DDS rejection, source snapshot lifecycle, target format restrictions, Fallout 4 and Starfield v2 deflate reopening, Starfield v3 method 3 raw-LZ4 reopening, Starfield v3 method 0 deflate reopening, multi-mip arrays, cubemaps, dedupe, and publication behavior. |
| Round-trip/reopen | Proven | `tests/unit/ba2_dx10_writer_tests.cpp`; `tests/unit/archive_reader_dispatch_tests.cpp`; `tests/unit/validation_api_tests.cpp` | Writer-output archives reopen through `archive_reader`; Starfield v2/v3 routes preserve texture payload bytes through extraction; representative Fallout 4 writer output validates with extractability enabled. |
| Malformed handling | Proven | `tests/unit/ba2_dx10_reader_tests.cpp`; `tests/unit/ba2_dx10_malformed_tests.cpp`; `tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json`; `tests/fixtures/generated/compatibility_matrix.json`; `tests/unit/validation_api_tests.cpp` | Default proof covers truncated records, invalid chunk spans, decoded-size mismatches, duplicate mip/face layouts, unsupported compression, oversized filename offsets, aggregate chunk limits, hash mismatches, and validation diagnostics. |
| Validation API behavior | Proven | `include/libbsa/validation.hpp`; `tests/unit/validation_api_tests.cpp` | The validation API directly accepts generated success fixtures `ba2_dx10_fo4.ba2` and `ba2_dx10_sfv3.ba2` with extractability enabled, validates writer-produced Starfield BA2 v2 fixed-deflate output, validates Starfield BA2 v3 method 0 deflate and method 3 raw LZ4 block texture routes, and covers BA2 DX10 failures through malformed matrix rows. |
| Compatibility warnings | N/A | `include/libbsa/validation.hpp`; `tests/unit/compatibility_warning_tests.cpp`; `docs/compatibility-evidence.md` | The current public warning catalog has no BA2 DX10-specific warning. Texture format restrictions are enforced as writer/parser validation behavior rather than compatibility warnings. |
| Public/package-consumer API proof | Proven | `tests/package-consumer/main.cpp`; `docs/integration-examples.md`; `tests/unit/public_include_boundary_tests.cpp`; `tests/unit/export_surface_policy_tests.cpp` | Package-consumer source compiles `ba2_dx10_writer` v2 target/options usage and the documented example compiles through `<libbsa/libbsa.hpp>`. The installed `package_consumer_smoke` runtime generates a tiny legal BC1 DXT10 DDS input inline, creates writer-produced Starfield v2 and v3 DX10 archives, opens them through `archive_reader`, validates them through `validate_archive`, and extracts the reconstructed DDS through sink, `extract_bytes`, and bulk extraction paths. |
| Docs/support-claim proof | Documented | `docs/target-format-guide.md`; `docs/api-mainpage.md` | Public docs cover Fallout 4 DX10, Starfield v2/v3 DX10, raw LZ4 block, deflate routes, and writer target policies. Accuracy is maintained by review, not by an automated check. |

## What `compatibility_matrix.json` is and is not

`tests/fixtures/generated/compatibility_matrix.json` is a malformed-hardening submatrix. It currently indexes generated invalid or edge-case rows across TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 so validation and reader tests can prove stable errors for truncated structures, invalid spans, unsupported routes, decompression failures, duplicate records, and texture layout contradictions.

It is **not** the full support matrix. It does not replace the reader, extraction, writer, round-trip, package-consumer, docs-policy, or optional local-corpus evidence listed above. The family/axis tables in this document are the human support matrix; the JSON file is one default evidence input for malformed handling and validation diagnostics.

## Advisory local evidence

Two environment variables enable optional local compatibility checks:

- `LIBBSA_GAME_FIXTURES` points tests at local game-derived archives or a local `bsarchpro_expected.json` manifest.
- `LIBBSA_BSARCHPRO_EXPECTED` points directly at a BSArchPro-derived comparison manifest.

The executable harness is `tests/unit/local_game_fixture_tests.cpp`. It is tagged `[requires-game-fixture]` and skips by default when local inputs are absent. When present, it opens each listed archive through libbsa and compares public metadata plus optional extracted bytes or FNV-1a payload hashes against the BSArchPro-derived expectations.

Absent local/copyrighted inputs do **not** block default green status, because default proof must stay reproducible from committed legal/generated fixtures and writer-output archives. Present local-only checks also do **not** replace default proof: they are advisory smoke/compare evidence that may justify future hardening work, not a substitute for committed tests or docs-policy coverage.

## Ranked gap list

No high-risk executable-proof gap was found for the core open/list, extraction, writer, round-trip/reopen, malformed-handling, public API surface, or support-doc axes. The remaining gaps are lower-risk or intentionally deferred; they are listed so future work can improve confidence without overstating today's default evidence.

The former `COV-GAP-001` validation-success gap is no longer open: `tests/unit/validation_api_tests.cpp` directly validates TES4 v103/v104/v105, BA2 GNRL Fallout 4/Starfield v2/Starfield v3, BA2 DX10 Fallout 4/Starfield v3, and writer-produced Starfield BA2 v3 method 0 deflate and method 3 raw LZ4 block routes through `validate_archive` with extractability enabled.

The former `COV-GAP-003` package-consumer runtime gap is no longer open: `package_consumer_smoke` installs libbsa, configures an external consumer against the installed `libbsa::libbsa` target, and creates, opens, validates, and extracts writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives without local copyrighted fixtures or TES5Edit fixture dependencies.

| Rank | Gap ID | Risk | Affected family/axis | Evidence source | Downstream route |
|---:|---|---|---|---|---|
| 1 | `COV-GAP-002` | Low / Deferred | Full BSArchPro or real game-corpus compatibility comparisons for all families. | `tests/unit/local_game_fixture_tests.cpp`, `tests/fixtures/README.md`, and `docs/compatibility-evidence.md` document opt-in local checks, but default evidence intentionally avoids copyrighted archive bytes. | Future compatibility-hardening campaign using local manifests; keep default CI independent of local/copyrighted inputs. |
| 2 | `COV-GAP-004` | Low / Deferred | Family-specific compatibility warning taxonomy for TES3 BSA and BA2 DX10. | `include/libbsa/validation.hpp`, `tests/unit/compatibility_warning_tests.cpp`, and `docs/compatibility-evidence.md` prove every current public warning code; no TES3 or BA2 DX10-specific warning is currently claimed. | Add warning codes only when a known target interoperability risk is identified and can be proven with default generated or writer-output evidence. |

## Maintenance rule

When a support claim changes, update this matrix in the same change that adds or removes the default proof. If a row cannot cite committed fixtures, writer-output tests, package-consumer checks, or docs-policy tests, mark it `Partial`, `Missing`, or `Deferred` instead of upgrading it to `Proven`.
