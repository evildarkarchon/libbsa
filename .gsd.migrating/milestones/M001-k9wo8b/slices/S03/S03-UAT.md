# S03: Fixture and Round-trip Gap Closure — UAT

**Milestone:** M001-k9wo8b
**Written:** 2026-05-20T02:47:17.357Z

# UAT: S03 Fixture and Round-trip Gap Closure

## UAT Type

Automated evidence review and repository verification. This slice has no human-facing UI; acceptance is proven through public documentation, docs-policy Catch2 tests, fixture generation, and focused CTest labels.

## Preconditions

- Work from `J:/libbsa` on the Windows MSVC/vcpkg development environment.
- Do not provide `LIBBSA_GAME_FIXTURES` or `LIBBSA_BSARCHPRO_EXPECTED`; those inputs must remain optional advisory evidence.
- Treat `TES5Edit/` as read-only reference material and do not use it as mutable fixture data.
- Use serialized CTest invocations for final evidence if using the existing Catch2 `PRE_TEST` discovery mode.

## Steps and Expected Outcomes

1. Open `docs/compatibility-evidence.md` and locate `Default fixture and round-trip proof sweep`.
   - Expected: the section is present, public-facing, and describes default proof from generated legal fixtures, writer-output archives, Catch2/CTest, and manifest validation.
2. Confirm the catalog references the proof surfaces needed by S03.
   - Expected: it names `tests/unit/archive_reader_dispatch_tests.cpp`, `tests/unit/validation_api_tests.cpp`, writer proof surfaces, fixture manifests, and `Optional local corpus checks` without making local game/BSArchPro data mandatory.
3. Configure the default debug static preset.
   - Command: `cmake --preset windows-msvc-debug-static`
   - Expected: configure succeeds without local copyrighted fixtures.
4. Regenerate/build the focused fixture and test targets.
   - Command: `cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures generate_tes3_bsa_writer_fixtures generate_tes4_bsa_fixtures generate_ba2_gnrl_fixtures generate_ba2_dx10_fixtures libbsa_tests`
   - Expected: all requested targets build successfully from committed synthetic fixture sources.
5. Run the focused docs, reader, writer, validation, and manifest proof labels.
   - Commands: `ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure`, `ctest --preset windows-msvc-debug-static -L reader_backend_dispatch --output-on-failure`, `ctest --preset windows-msvc-debug-static -L tes3_bsa_writer --output-on-failure`, `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure`, `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure`, `ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure`, `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure`, and `ctest --preset windows-msvc-debug-static -R validate_fixture_manifests --output-on-failure`.
   - Expected: every command passes; round-trip/reopen evidence remains proven for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
6. Review `tests/unit/coverage_audit_matrix_docs_tests.cpp` failures if any policy check breaks.
   - Expected: failures identify stale or missing public proof wording rather than requiring inspection of `.gsd`, local corpus paths, or `TES5Edit/`.

## Edge Cases

- Default proof must still pass when optional local game archives and BSArchPro comparison directories are absent.
- CTest label runs should be serialized if the build tree is using Catch2 `DISCOVERY_MODE PRE_TEST`, avoiding known generated-discovery-file races.
- Public docs must not imply that optional local corpus checks are required for CI/default support claims.
- The proof sweep must remain bounded to existing archive families and must not silently expand into validation/error-model redesign.

## Not Proven By This UAT

- Byte-for-byte equality against BSArchPro or official game archives.
- Optional local corpus compatibility results from `LIBBSA_GAME_FIXTURES` or `LIBBSA_BSARCHPRO_EXPECTED`.
- Public package-consumer runtime proof for every family; S05 owns the integrated confidence pass.
- COV-GAP-001 validation-success granularity and broader public error behavior; S04 owns that stabilization.
- New archive families, GUI/CLI behavior, in-place mutation, or performance stress guarantees.
