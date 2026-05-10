# Compatibility Evidence Catalog

This catalog maps public `libbsa::compatibility_warning_code` values to the
rule they represent, the evidence that proves the rule, and the default gate
that keeps the evidence reproducible.

Generated legal fixtures and writer-output archives are the mandatory evidence
path for Phase 11. Optional local game or BSArchPro-derived checks may add
smoke/compare confidence, but they are never required for the default suite.
The executable opt-in comparison harness is
`tests/unit/local_game_fixture_tests.cpp`; it consumes
`LIBBSA_BSARCHPRO_EXPECTED` or a local `bsarchpro_expected.json` manifest under
`LIBBSA_GAME_FIXTURES` and compares libbsa metadata plus optional extracted
bytes or FNV-1a payload hashes against BSArchPro-derived expectations.

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
- Evidence: `tests/unit/compatibility_warning_tests.cpp` test `compatibility_warning reports BA2 target family mismatch` creates a synthetic writer-output BA2 GNRL archive, validates it with `validation_options::expected_type = archive_type::bsa`, and asserts the risky target-family mismatch warning. Plan 11 validation tests also run `validate_archive` over generated BA2 fixtures and writer-output archives as the same parsed-metadata source of truth.
- Default gate: generated/writer-output; covered by default CTest through the `compatibility_warning` and `validation_api` test labels.

## Optional Local Corpus Checks

Local game archives or BSArchPro-derived comparison output may supplement this
catalog only as smoke/compare checks. The local comparison manifest is
exercised by the `BSArchPro-derived expected fixture comparisons are opt-in`
CTest case. Such checks must:

- Use the `[requires-game-fixture]` tag and remain skipped when `LIBBSA_GAME_FIXTURES` is unset.
- Read from ignored local data locations, not committed fixture directories.
- Avoid committing copyrighted archive bytes, extracted game payloads, or BSArchPro-generated corpus output.
- Treat `TES5Edit/` as read-only reference material, not a fixture workspace or output directory.

Default acceptance must continue to pass from committed generated fixtures,
writer-output archives, and policy tests alone.
