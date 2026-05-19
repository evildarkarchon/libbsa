# Inventory: CONCERNS.md Tech Debt

## Scope source

`CONCERNS.md` Tech Debt lists four items. This inventory treats the first item as the primary code refactor because it has concrete files and a safe extraction path; the remaining items are policy/API concerns that should stay evidence-gated unless new real-world fixtures justify changing public behavior.

## Current findings

### 1. Large-format parser/preparer translation units

Actionable now. The largest current hotspots are:

| File | Approx. lines | Responsibilities currently mixed |
| --- | ---: | --- |
| `src/formats/ba2/ba2_dx10_parser.cpp` | 869 | BA2 DX10 header parsing, record/chunk parsing, count caps, filename-table reading from memory and file-backed paths, payload-span validation, hash/extension checks, DDS texture public metadata materialization, file-backed parse orchestration. |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | 651 | BA2 GNRL header/record parsing, filename-table parsing, extension/hash checks, payload-span validation, public metadata materialization, file-backed parse orchestration. |
| `src/formats/bsa/tes3_bsa_parser.cpp` | 465 | TES3 fixed-table parsing, name/hash reading, payload-span validation, canonical path/hash validation, public metadata materialization. |
| `src/formats/bsa/tes4_bsa_prepare.cpp` | 645 | TES4 writer path splitting, file-flag policy, DDS texture validation, source byte loading, per-entry preparation, duplicate checking, folder grouping and sorting. |

Existing seam patterns to reuse:

- `src/formats/bsa/tes4_bsa_table.{hpp,cpp}` separates TES4 raw metadata table reading from public entry materialization.
- `src/formats/bsa/tes4_bsa_payload_descriptor.{hpp,cpp}` separates TES4 payload span/compression/embedded-name derivation.
- `src/formats/ba2/ba2_dx10_snapshot_builder.{hpp,cpp}` and `ba2_dx10_chunk_assembler.{hpp,cpp}` already split BA2 DX10 writer snapshot/chunk work.
- `tests/unit/parser_preparer_seam_policy_tests.cpp`, `tests/unit/tes4_bsa_parser_seam_tests.cpp`, and `tests/unit/ba2_dx10_preparer_seam_tests.cpp` provide policy/behavioral seam coverage.

Likely edit set for a safe first wave:

- Add `src/formats/ba2/ba2_dx10_records.{hpp,cpp}` for fixed-header and record/chunk table reading.
- Add `src/formats/ba2/ba2_dx10_names.{hpp,cpp}` for memory-backed and file-backed filename-table reading.
- Update `src/formats/ba2/ba2_dx10_parser.cpp` to orchestrate those helpers and retain public metadata materialization.
- Update `CMakeLists.txt` to compile the new internal sources.
- Extend seam tests in `tests/unit/parser_preparer_seam_policy_tests.cpp` and/or add focused helper assertions in `tests/unit/ba2_dx10_parser_tests.cpp`.

Dependencies/order: create helper headers and sources first, then move parser call sites, then update build/tests.

### 2. Strict parser policy may reject tolerated real-world archives

Not safe to relax in this refactor without compatibility evidence. Current strict checks are covered in generated malformed fixture tests across:

- `tests/unit/tes3_bsa_reader_tests.cpp`
- `tests/unit/tes4_bsa_reader_tests.cpp`
- `tests/unit/ba2_gnrl_reader_tests.cpp`
- `tests/unit/ba2_dx10_parser_tests.cpp`
- `tests/unit/compatibility_matrix_tests.cpp`

Relevant code sites include duplicate canonical path rejection, stored hash mismatch rejection, extension mismatch rejection, payload span overlap rejection, and TES3 hash-order rejection in the parser files above.

Dependency: any lenient mode would be public/API-affecting and requires documented fixtures in `docs/compatibility-evidence.md`; no such evidence is part of this workflow.

### 3. Validation warning coverage is intentionally narrow

Not safe to add warning codes without evidence. Current public warning surface is:

- `include/libbsa/validation.hpp`
- `src/validation.cpp`
- `tests/unit/compatibility_warning_tests.cpp`
- `tests/unit/validation_policy_tests.cpp`
- `docs/compatibility-evidence.md`

Adding codes changes public API and must include evidence docs plus policy/test coverage. No new evidenced warning condition is identified in this refactor inventory.

### 4. Metadata count limits are internal and high

Mostly already centralized:

- Constants live in `src/detail/parser_primitives.hpp`:
  - `metadata_entry_count_limit = 1,000,000`
  - `metadata_bsa_folder_count_limit = 65,536`
  - `metadata_dx10_chunk_count_limit = 1,000,000`
- Enforcement uses `detail::validate_metadata_count()` and reservation helpers.
- Tests exist in `tests/unit/parser_primitives_tests.cpp` and per-format malformed tests.

Small actionable cleanup: centralize repeated 64-bit span-overlap helpers into `parser_primitives` with overflow-safe arithmetic, then replace local copies in BA2 DX10, BA2 GNRL, and TES4 parsing. This reduces unsafe helper duplication and supports the broader parser-splitting debt.

## Estimated changed files

Initial safe migration wave is expected to touch 7-10 files:

- Source: `src/detail/parser_primitives.{hpp,cpp}`, `src/formats/ba2/ba2_dx10_parser.cpp`, two to four new BA2 DX10 helper files, `CMakeLists.txt`.
- Tests: `tests/unit/parser_primitives_tests.cpp`, `tests/unit/parser_preparer_seam_policy_tests.cpp`, possibly `tests/unit/ba2_dx10_parser_tests.cpp`.
- Docs/artifacts: this inventory, plan, final summary.

## Out of scope for this refactor

- Relaxing strict parser failures or adding lenient reader mode.
- Adding new public `compatibility_warning_code` values without compatibility evidence.
- Reworking TES3/GNRL/TES4 preparer modules beyond shared helper cleanup.
- Any edits under `TES5Edit/`.
