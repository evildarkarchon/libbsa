# Compatibility Evidence Catalog

This catalog maps public `libbsa::compatibility_warning_code` values to the rule they represent, the evidence that proves the rule, and the default gate that keeps the evidence reproducible.

Generated legal fixtures and writer-output archives are the mandatory ordinary-CI evidence path for public compatibility warnings. Legacy local game or BSArchPro-derived manifest checks remain supplemental. [ADR-0005](adr/0005-releases-require-independent-archive-interoperability-evidence.md) separately requires full independent Archive Interoperability evidence for releases through [tests/compat](../tests/compat/README.md). That additional gate does not replace committed legal fixtures, writer-output archives, or package-consumer checks.

Use this catalog together with `docs/coverage-audit-matrix.md` and `docs/public-api-reality-check.md`:

- `docs/coverage-audit-matrix.md` tracks the broader archive-family support matrix across reader, extraction, writer, round-trip, validation, package-consumer, and documentation axes.
- `docs/public-api-reality-check.md` maps the current public API core to that proof and routes known public-story gaps without introducing a broader facade.
- This file is narrower: it explains the compatibility-warning taxonomy that validation reports expose today.

Default acceptance must continue to pass from repository-reproducible generated fixtures, writer-output archives, and policy tests alone. Absent local or copyrighted inputs do not block default green status. They do block the separate strict release gate when required oracle or retail coverage is missing; ordinary CI success alone does not satisfy ADR-0005.

## Default fixture and round-trip proof sweep

The default proof sweep uses committed legal generated fixtures, writer-output archives produced by the public writer APIs, the installed package-consumer runtime smoke, Catch2/CTest cases, and manifest validation only. It does not require local game archives, copied game payload bytes, or BSArchPro-derived comparison output. `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` enable the legacy focused checks; absent local inputs must not block the default suite. The thorough release command instead requires its enrolled retail baseline and pinned BSArch executable.

The current default sweep covers four archive families:

- TES3 BSA.
- TES4-family BSA.
- BA2 GNRL.
- BA2 DX10.

Use these public proof files together when auditing the default sweep:

- `docs/coverage-audit-matrix.md` is the family/axis support matrix and is the source of truth for proof granularity.
- `tests/fixtures/README.md` documents fixture provenance, generated-archive policy, optional local corpus rules, and test label vocabulary.
- `tests/unit/archive_reader_dispatch_tests.cpp` reopens representative generated archives through the public `archive_reader` dispatch surface and verifies metadata, lookup, extraction, and bulk extraction behavior.
- Family writer tests prove writer-output archive behavior: `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.
- `tests/unit/validation_api_tests.cpp` validates representative and direct generated success fixtures, writer-produced archives, and malformed matrix rows through the public validation API. Its direct validation success matrix includes `tes4_v103.bsa`, `tes4_v104.bsa`, `tes4_v105.bsa`, `ba2_gnrl_fo4.ba2`, `ba2_gnrl_sfv2.ba2`, `ba2_gnrl_sfv3.ba2`, `ba2_dx10_fo4.ba2`, and `ba2_dx10_sfv3.ba2`; it also validates writer-produced Starfield BA2 v3 method 0 deflate and method 3 raw LZ4 block routes for both BA2 GNRL and BA2 DX10.
- `tests/package-consumer/main.cpp` is exercised by the `package_consumer_smoke` CTest gate. The gate installs libbsa, configures an external project against the installed `libbsa::libbsa` target, and at runtime creates, opens, validates, and extracts writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives without local game archives or TES5Edit fixture dependencies.
- `tests/fixtures/generated/validate_fixture_manifests.py` keeps manifest shape, referenced malformed archives, and compatibility-matrix evidence references consistent with committed generated assets.

A focused default CMake/CTest sweep can be run with repository paths and presets only:

```powershell
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static -R validate_fixture_manifests
ctest --preset windows-msvc-debug-static -L fixture
ctest --preset windows-msvc-debug-static -L roundtrip
ctest --preset windows-msvc-debug-static -L validation_api
ctest --preset windows-msvc-debug-static -L docs_policy
ctest --preset windows-msvc-debug-static -L target_format_policy
ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure
```

For only the installed-package runtime archive proof, run the focused package-consumer CTest directly:

```powershell
ctest --preset windows-msvc-debug-static -R package_consumer_smoke --output-on-failure
```

For a family-writer-only round-trip pass, run the writer labels directly:

```powershell
ctest --preset windows-msvc-debug-static -L tes3_bsa_writer
ctest --preset windows-msvc-debug-static -L tes4_bsa_writer
ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer
ctest --preset windows-msvc-debug-static -L ba2_dx10_writer
```

These commands intentionally avoid optional local corpus inputs. They prove the public default fixture, round-trip, and direct validation success story from committed synthetic assets; `docs/coverage-audit-matrix.md` remains the source of truth for per-family granularity.

The legacy opt-in comparison harness is `tests/unit/local_game_fixture_tests.cpp`; it consumes `LIBBSA_BSARCHPRO_EXPECTED` or a local `bsarchpro_expected.json` manifest under `LIBBSA_GAME_FIXTURES` and compares libbsa metadata plus optional extracted bytes or FNV-1a payload hashes against BSArchPro-derived expectations. The independent release harness in `tests/compat` requires complete catalogs and SHA-256/content evidence, controlled cross-tool writer cases, and independently checked retail repacks. Its successful static/shared reports must pass the documented `verify-release` procedure.

### `compressed_sound_payload`

- Rule: BSA entries under `sound/`, or with `.wav`, `.xwm`, or `.fuz` names, are valid but should warn when their payload is compressed because sound compression is a known Bethesda compatibility risk.
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports compressed sound payloads` creates a synthetic writer-output TES4-family BSA with `sound/fx/alert.wav` compressed through the public writer, validates it through `validate_archive`, and asserts the public warning code and advisory severity. `docs/PRD.md` also tracks the known quirk as a sounds-in-compressed warning.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning` test label and this `validation_policy` catalog check.

### `bsa_embedded_name_compatibility_risk`

- Rule: TES4-family BSA entries with embedded file-name prefixes are valid but risky enough to surface as compatibility warnings, because target support differs and SSE-style embedded-name behavior has known crash-risk history.
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports BSA embedded name compatibility risk` creates a synthetic writer-output TES4-family BSA with `embed_file_names = true`, validates it through `validate_archive`, and asserts the risky warning with an archive path. `tests/unit/tes4_bsa_writer_tests.cpp` also proves writer-output embedded-name prefixes reopen and extract through `archive_reader` while v103 target compatibility keeps embedded names absent.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning`, `validation_api`, and TES4 writer test paths.

### `target_family_mismatch`

- Rule: A structurally valid archive should warn when the caller supplies an expected archive family or variant that does not match parsed archive metadata.
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports BA2 target family mismatch` creates a synthetic writer-output BA2 GNRL archive, validates it with `validation_options::expected_type = archive_type::bsa`, and asserts the risky target-family mismatch warning. Validation tests also run `validate_archive` over generated BA2 fixtures and writer-output archives as the same parsed-metadata source of truth.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning` and `validation_api` test labels.

### `ba2_record_identity_mismatch`

- Rule: A BA2 record whose stored `NameHash`, `DirHash`, or `Ext` disagrees with its own filename-table path is valid but unreachable by Bethesda-style hash lookup, so it warns instead of invalidating the archive. The reference never cross-checks these fields: `TwbBSArchive.LoadFromFile` reads them verbatim into the record array, and `FindFileRecordFO4` recomputes hashes from the *query* path before scanning for a match (`TES5Edit/Core/wbBSArchive.pas`). A disagreeing record is simply unaddressable by name and does not affect any sibling record.
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports BA2 record identity mismatch` writes a BA2 GNRL archive through the public writer, byte-patches record 0's `NameHash`, validates it through `validate_archive`, and asserts the risky warning with an archive path plus continued extraction by path. `tests/unit/ba2_gnrl_reader_tests.cpp` and `tests/unit/ba2_dx10_reader_tests.cpp` cover all three stored fields against generated fixtures and assert exactly one warned entry per mutation.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning`, `ba2_gnrl_hash_lookup`, and `ba2_dx10_hash_lookup` test labels.
- Open question (advisory, local corpus only): the three warned records in retail `Fallout4 - Voices.ba2` all carry non-ASCII file names. `hash_fo4` skips bytes above 127 exactly as reference `CreateHashFO4` does, so the reconstructed hash cannot depend on those bytes. Why Bethesda's packer stored a different value for them is unresolved; the warning states the disagreement, not a cause.

### `bsa_file_name_table_trailing_bytes`

- Rule: A TES4-family BSA whose `TotalFileNameLength` declares more bytes than its file names consume is valid, so it warns instead of being rejected. The reference reads names with exactly `FileCount` sequential `ReadStringTerm` calls and never compares the resulting stream position against the header total (`TES5Edit/Core/wbBSArchive.pas`, `TwbBSArchive.LoadFromFile`), so surplus table bytes are invisible to it. The surplus belongs to no entry and hides none: every entry stays listed, findable by hash, and extractable. The warning is archive-level and therefore carries no `archive_path`. Names that run *past* the declared table remain a hard error, because the archive then genuinely cannot be listed.
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports a file-name table with trailing bytes` writes a TES4-family BSA through the public writer, byte-patches 105 NUL bytes into the file-name table while rewriting `TotalFileNameLength` and every folder-record and file-record offset the growth shifts, validates it through `validate_archive`, and asserts the advisory warning plus continued listing and extraction. `tests/unit/tes4_bsa_reader_tests.cpp` covers the parse seam directly and pins the flag clear across every committed generated fixture.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning` and `tes4_bsa_metadata` test labels.
- The sibling `TotalFolderNameLength` total is demoted too; see the next entry.

### `bsa_folder_name_table_length_mismatch`

- Rule: A TES4-family BSA whose declared `TotalFolderNameLength` disagrees with the folder names it stores is valid, so it warns instead of being rejected. The field appears only on the reference's write path -- `wbBSArchive.pas:1393` zeroes it and `:1469` accumulates it -- and `TwbBSArchive.LoadFromFile` never reads it back, walking folder names sequentially from `FoldersOffset` instead. The warning is archive-level and carries no `archive_path`.
- Why libbsa can now match that: the parser used to size its whole metadata table from the field, which made a wrong value fatal twice over. The total cross-check rejected the archive outright, and because the derived table size also fixes the payload/metadata boundary, an inflated value would have rejected legal payloads as overlapping a phantom metadata region. `tes4_bsa_metadata_table_read_bound` now bounds the read by folder count (`tes4_bsa_max_folder_name_block_entry_size` per folder, clamped to the archive) and `read_tes4_bsa_raw_table` reports the table's true size measured by walking. With nothing sized from the field, the check is a pure cross-check and demoting it is free.
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports a declared folder-name length that disagrees` writes a TES4-family BSA through the public writer, overwrites `TotalFolderNameLength` with an inflated value -- a one-field patch that moves no byte and shifts no offset, which is itself the claim -- validates it through `validate_archive`, and asserts the advisory warning plus continued extraction of a payload sitting immediately after the true metadata table. `tests/unit/tes4_bsa_reader_tests.cpp` covers the inflated direction and the zero case at the parse seam, pins the flag clear across every committed generated fixture, and proves a committed fixture with `TotalFolderNameLength` set to `0xFFFFFFFF` still lists and extracts every entry. Zero is pinned specifically because it is the value the reference initializes the field to (`wbBSArchive.pas:1393`) before never reading it back, so it is a value BSArchPro loads without complaint; libbsa previously rejected it as "does not include usable entry names", which the two archive-flag checks on that line already answer properly. All three cases carry the `malformed` label so they run in the ASan lane.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning` and `tes4_bsa_metadata` test labels.
- Corpus note (local corpus only): every archive in the retail corpus satisfies the total once the bzstring length prefix is excluded from the count (commit `e97a0e2f`), so no retail archive currently raises this warning. It is kept because the parser no longer needs the field to be right, not because retail archives get it wrong.
- Corpus corroboration (issue #45, local corpus only): retail `Fallout - Voices1.bsa` declares a file-name table 105 bytes longer than its 105,517 names consume, and every surplus byte is NUL. Requiring exact consumption rejected that archive outright. libbsa does not verify that the surplus is NUL, and neither does the reference; the padding value is an observation about the corpus, not a parsed invariant. `tests/unit/local_game_fixture_tests.cpp` test `retail TES4-family BSA file-name table slack is a warning, not a rejection` asserts the implication over whatever archives a machine holds, and `every retail TES4-family BSA opens and lists its full entry count` proves the whole family opens.

## Local Corpus Checks and Required Release Evidence

The legacy local comparison manifest is exercised by the `BSArchPro-derived expected fixture comparisons are opt-in` CTest case. These focused supplemental checks must:

- Use the `[requires-game-fixture]` tag and remain skipped when neither an environment override nor the default local corpus provides the required input.
- Read from ignored local data locations, not committed fixture directories.
- Avoid committing copyrighted archive bytes, extracted game payloads, or BSArchPro-generated corpus output.
- Treat `TES5Edit/` as read-only reference material, not a fixture workspace or output directory.

Default acceptance must continue to pass from committed generated fixtures, writer-output archives, and policy tests alone.

Release preparation additionally requires [ADR-0005](adr/0005-releases-require-independent-archive-interoperability-evidence.md): run `libbsa_compatibility_check` in both MSVC Release static and shared lanes and verify the paired reports with `tests/compat/runner.py verify-release`. The [suite instructions](../tests/compat/README.md) document the pinned local oracle, the 101-archive baseline, 71 controlled cases, comparison rules, resource controls, and report identities. Every supplied retail entry participates; missing required coverage and unexplained differences fail the gate. This document records that procedure, not a successful full-corpus result.

BSA/GNRL decoded files require exact content equality. DX10 comparisons allow only independently validated equivalent DDS representations while requiring matching texture meaning and all surface bytes. Structural checks remain separate, and BSArch acceptance is not Game Acceptance. Detailed retail reports and oracle output stay local; publish only the content-free companion summary.
