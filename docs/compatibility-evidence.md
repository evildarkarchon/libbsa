# Compatibility Evidence Catalog

This catalog maps public `libbsa::compatibility_warning_code` values to the rule they represent, the evidence that proves the rule, and the default gate that keeps the evidence reproducible.

Generated legal fixtures and writer-output archives are the mandatory evidence path for public compatibility warnings. Optional local game or BSArchPro-derived checks may add smoke/compare confidence, but they are never required for the default suite and never replace committed legal fixtures, writer-output archives, package-consumer checks, or documentation policy tests.

Use this catalog together with `docs/coverage-audit-matrix.md` and `docs/public-api-reality-check.md`:

- `docs/coverage-audit-matrix.md` tracks the broader archive-family support matrix across reader, extraction, writer, round-trip, validation, package-consumer, and documentation axes.
- `docs/public-api-reality-check.md` maps the current public API core to that proof and routes known public-story gaps without introducing a broader facade.
- This file is narrower: it explains the compatibility-warning taxonomy that validation reports expose today.

Default acceptance must continue to pass from repository-reproducible generated fixtures, writer-output archives, and policy tests alone. Optional local corpus checks are advisory evidence only; they may improve confidence in a developer workspace, but absent local or copyrighted inputs do not block default green status.

## Default fixture and round-trip proof sweep

The default proof sweep uses committed legal generated fixtures, writer-output archives produced by the public writer APIs, the installed package-consumer runtime smoke, Catch2/CTest cases, and manifest validation only. It does not require local game archives, copied game payload bytes, or BSArchPro-derived comparison output. `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` are optional advisory inputs for local smoke/compare confidence; unset variables must not block the default suite.

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

The executable opt-in comparison harness is `tests/unit/local_game_fixture_tests.cpp`; it consumes `LIBBSA_BSARCHPRO_EXPECTED` or a local `bsarchpro_expected.json` manifest under `LIBBSA_GAME_FIXTURES` and compares libbsa metadata plus optional extracted bytes or FNV-1a payload hashes against BSArchPro-derived expectations.

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

## Optional Local Corpus Checks

Local game archives or BSArchPro-derived comparison output may supplement this catalog only as smoke/compare checks. The local comparison manifest is exercised by the `BSArchPro-derived expected fixture comparisons are opt-in` CTest case. Such checks must:

- Use the `[requires-game-fixture]` tag and remain skipped when `LIBBSA_GAME_FIXTURES` is unset.
- Read from ignored local data locations, not committed fixture directories.
- Avoid committing copyrighted archive bytes, extracted game payloads, or BSArchPro-generated corpus output.
- Treat `TES5Edit/` as read-only reference material, not a fixture workspace or output directory.

Default acceptance must continue to pass from committed generated fixtures, writer-output archives, and policy tests alone.
