# S05 Research: Integrated Confidence Pass

**Depth:** Targeted research. The technologies and local patterns are established; the slice is a final integration/proof pass over package-consumer smoke, coverage-matrix truth, and docs-policy guardrails.

## Summary

S05's main executable unblocker is **COV-GAP-003**: the installed-package smoke currently compiles and links representative public APIs through `<libbsa/libbsa.hpp>`, then runs a missing-path error check, but it does **not** create/open an archive for every supported family from the installed package. S02/S03/S04 already established the public API story, generated fixture/round-trip proof, and validation/error proof. S05 should add the missing package-level runtime proof, then update the coverage matrix and policy tests so COV-GAP-003 is closed while COV-GAP-002 and COV-GAP-004 remain explicitly deferred.

Important surprise: `docs/coverage-audit-matrix.md` already marks each family’s `Public/package-consumer API proof` row as `Proven`, but the ranked gap list still keeps `COV-GAP-003` open for every-family installed-package runtime archive proof. `tests/unit/docs_policy_tests.cpp` also currently asserts the old text that this runtime proof is “intentionally not proven by default.” Closing S05 requires code + docs + docs-policy changes together; a package-consumer code-only change will leave policy tests stale.

## Active Requirements and Constraints

- **R007 (primary S05 owner, active):** prove installed/exported public API can be consumed through package target and umbrella header. This is the slice’s central closure target; package-consumer runtime proof should validate it.
- **R003 (supporting, active):** preserve the audited public API capability story. Do not add a broad facade; prove the existing reader/validation/writer flows are usable.
- **R008 (supporting, active):** public headers must remain dependency-light C++20. S05 should not need public header changes; package-consumer code may use standard C++ helpers internally.
- **R009 (supporting, active):** preserve TES5Edit read-only boundary. Package-consumer runtime proof must use writer-produced synthetic artifacts or standard-generated DDS bytes, not TES5Edit or local game corpora.
- **R001/R005/R006 (already validated, S05 supports):** keep matrix truth, layered default-vs-optional evidence, and validation/error proof accurate after closing COV-GAP-003.
- **R011/R012/R013 deferred:** do not turn S05 into a full BSArchPro/game corpus campaign, performance stress pass, or release-readiness declaration.

## Skills Discovered

- Relevant installed prompt skills: `cmake`, `cpp-testing`, and `write-docs` map to the build/test/docs nature of this work; no new skill needed for the implementation path.
- Ran `npx skills find "vcpkg"` in gsd_exec `90463850-3abe-4637-b1d1-4e8f2b127004`. It found `mohitmishra786/low-level-dev-skills@conan-vcpkg` with 131 installs. I did **not** install it because S05 does not change vcpkg dependency policy or package metadata; existing CMake/package-consumer smoke patterns are sufficient.
- No external library docs were needed; CMake/CTest/Catch2/vcpkg patterns are already encoded locally.

## Implementation Landscape

### Package-consumer smoke path

- `tests/package-consumer/main.cpp`
  - Includes only `<libbsa/libbsa.hpp>` plus standard headers.
  - Defines compile-checked examples for:
    - `example_open_list_extract`
    - `example_bulk_extract`
    - `example_create_tes3_bsa`
    - `example_create_tes4_bsa`
    - `example_create_ba2_gnrl`
    - `example_create_ba2_dx10`
    - `example_handle_result_errors`
    - `example_validate_archive`
  - Current `main()` only calls `example_link_representative_public_api()` and verifies `validate_archive("consumer-smoke.bsa")` returns `io_error` for a missing file. This is why COV-GAP-003 remains open.
  - Natural insertion point: add a self-contained runtime helper called from `main()` before/after the missing-path error check.

- `tests/package-consumer/smoke.cmake`
  - Installs libbsa into `${LIBBSA_INSTALL_PREFIX}`.
  - Configures `tests/package-consumer` as an external consumer with `find_package(libbsa CONFIG REQUIRED)` and `CMAKE_PREFIX_PATH` containing the libbsa install prefix plus vcpkg triplet prefixes.
  - Builds the consumer and runs its own CTest suite.
  - Copies runtime DLLs from install/vcpkg prefixes into the consumer runtime directory. This makes it the right place to catch installed-package/runtime dependency issues.
  - No change appears necessary unless the consumer executable needs extra command-line args, which it should not.

- `tests/package-consumer/CMakeLists.txt`
  - Links `libbsa_package_consumer` against `libbsa::libbsa` and adds `libbsa_package_consumer_run`.
  - Uses `copy-runtime-dlls.cmake` after build. No source-tree fixtures are passed in.
  - No change appears necessary.

- `tests/CMakeLists.txt`
  - Defines `package_consumer_smoke` and `package_consumer_runtime_dll_copy`, both labeled `package_consumer;target_format_policy`.
  - No dependency on generated fixtures; if S05 self-generates runtime archives inside the package consumer executable, this can remain unchanged.

### Public docs and policy tests that must move with COV-GAP-003

- `docs/coverage-audit-matrix.md`
  - Current source of truth for family/axis proof.
  - Needs COV-GAP-003 moved from ranked open gap to former/closed gap language.
  - Family package-consumer rows should say package-consumer smoke creates/opens/extracts writer-produced runtime archives for all four current families, while variant/compression depth remains unit-fixture proof.
  - Remaining ranked gaps should be COV-GAP-002 and COV-GAP-004 only.

- `docs/public-api-reality-check.md`
  - Currently routes COV-GAP-003 to S05 and says every-family installed-package runtime archive proof is not proven.
  - Needs flow table/routing updated to say COV-GAP-003 is closed by package-consumer runtime smoke. Keep “no facade” conclusion.

- `docs/compatibility-evidence.md`
  - Default proof sweep should include package-consumer runtime smoke and add `ctest --preset windows-msvc-debug-static -L package_consumer` to the focused command list.
  - Keep optional `LIBBSA_GAME_FIXTURES` / `LIBBSA_BSARCHPRO_EXPECTED` boundaries unchanged.

- `docs/integration-examples.md` and possibly `docs/api-mainpage.md`
  - Current wording says examples are compile-checked by `tests/package-consumer/main.cpp`.
  - Consider adding one sentence that the package-consumer smoke also runs a small writer-produced archive open/extract pass for all four current families through the installed target. Do not overclaim every variant or real-game compatibility.

- `tests/unit/docs_policy_tests.cpp`
  - Currently requires text saying COV-GAP-003 is intentionally not proven by default. This must change to closed-proof wording.
  - Preserve optional evidence/TES5Edit boundary assertions.

- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
  - Current COV-GAP guard only closes COV-GAP-001 and preserves COV-GAP-003.
  - Update/add a test that COV-GAP-003 is closed, is not present as a ranked open gap, and the matrix still preserves COV-GAP-002 and COV-GAP-004.
  - Add tokens for installed-package runtime proof if documenting it in `compatibility-evidence.md` and/or `public-api-reality-check.md`.

- `tests/unit/target_format_policy_tests.cpp`
  - Already checks package consumer uses umbrella header only, no direct public includes, no `.gsd/`, `.planning/`, `.audits/`, `TES5Edit/`, `LIBBSA_GAME_FIXTURES`, or `LIBBSA_BSARCHPRO_EXPECTED`.
  - Extend it to require package-consumer runtime smoke tokens for the four families and the documented installed-target gate.

## Recommended Implementation Approach

### 1. Close COV-GAP-003 in `tests/package-consumer/main.cpp`

Add a self-contained runtime smoke that uses only public libbsa APIs and standard C++:

1. Write a small source payload file in the consumer test working directory.
2. Write a minimal valid DDS DXT10 file in the consumer test working directory for BA2 DX10 writer input.
3. Call the existing documented writer examples to create one archive per current family:
   - TES3 BSA: `example_create_tes3_bsa(source, "consumer-runtime-tes3.bsa")`, path `book/readme.txt`.
   - TES4-family BSA: `example_create_tes4_bsa(source, "consumer-runtime-tes4.bsa")`, path `meshes/example/example.nif`; current example uses `tes4_bsa_target::skyrim_se` and target-default compression.
   - BA2 GNRL: `example_create_ba2_gnrl(source, "consumer-runtime-ba2-gnrl.ba2")`, path `scripts/example/example.pex`; current example uses Starfield v3 method 3/raw LZ4 for compressed entries.
   - BA2 DX10: `example_create_ba2_dx10(dds, "consumer-runtime-ba2-dx10.ba2")`, path `textures/example/example_d.dds`.
4. For each archive, open through `archive_reader::open`, call `metadata()`, `find()`, `contains()`, and `extract_bytes()` for the expected path.
5. For TES3/TES4/BA2 GNRL, compare extracted bytes to the source payload.
6. For BA2 DX10, avoid exact DDS byte equality unless confirmed; safer proof is open success, `entry_metadata::texture.has_value()`, non-empty `extract_bytes()`, and successful `validate_archive` with `validate_entry_extractability = true`.
7. Call `example_open_list_extract` for at least one archive (or all four) to keep the richer reader example executable, not just compile-checked.
8. Call `example_bulk_extract` on at least one generated archive to keep bulk extraction runtime-proven through the installed package.
9. Keep the existing missing-path `io_error` check so the previous representative error-path proof remains.

Recommended helper shape inside `namespace`:

- `std::vector<std::byte> consumer_payload()`
- `std::vector<std::byte> consumer_dds_r8g8b8a8_unorm_8x8()`
- `bool write_binary_file(std::string_view path, std::span<const std::byte> bytes)` or `libbsa::result<void> write_binary_file(...)`
- `libbsa::result<void> require_archive_roundtrip(std::string_view archive_path, std::string_view entry_path, std::span<const std::byte> expected, bool require_exact_bytes)`
- `libbsa::result<void> example_installed_package_runtime_archives()`

Minimal DDS guidance: borrow the existing generated-fixture logic, not the generated file. `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` builds valid synthetic DDS bytes with a 148-byte DXT10 header. For the package consumer, use the simplest known-supported case from that generator: `DXGI_FORMAT_R8G8B8A8_UNORM` (`28`), 8x8, 1 mip, 1 array slice, resource dimension TEXTURE2D (`3`), payload `8 * 8 * 4` bytes. This keeps the smoke self-contained and avoids a source-tree fixture dependency.

### 2. Update matrix/docs to close COV-GAP-003 without overclaiming

Recommended wording direction:

- Matrix: add a sentence near ranked gaps: “The former `COV-GAP-003` package-consumer runtime gap is no longer open: `tests/package-consumer/main.cpp` creates writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives through the installed package target, reopens them with `archive_reader`, validates/extracts representative entries, and remains independent of local game/TES5Edit fixtures.”
- Keep COV-GAP-002 “Full BSArchPro or real game-corpus comparisons” deferred.
- Keep COV-GAP-004 warning taxonomy deferred.
- Do **not** claim installed-package runtime proof covers every variant/compression route. That remains covered by unit fixture/writer/validation tests.

### 3. Update docs-policy tests after docs text changes

The policy tests are intentionally string-based guardrails. Update them in the same change as docs, or the suite will fail.

Good guardrail assertions:

- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
  - Requires former COV-GAP-003 closure text.
  - Requires package-consumer runtime proof mentions TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10.
  - Requires `COV-GAP-002` and `COV-GAP-004` still present as remaining/deferred gaps.
  - Requires no ranked open row for COV-GAP-003.

- `tests/unit/docs_policy_tests.cpp`
  - Replace “installed-package runtime archive creation/opening for every family is intentionally not proven by default” with closed-proof text.
  - Keep default proof/no local corpus/no TES5Edit boundary tokens.

- `tests/unit/target_format_policy_tests.cpp`
  - Require package consumer source has a named runtime proof helper plus runtime archive filenames/paths or family tokens.
  - Continue forbidding `.gsd/`, `.planning/`, `.audits/`, `TES5Edit/`, `LIBBSA_GAME_FIXTURES`, and `LIBBSA_BSARCHPRO_EXPECTED`.

## Natural Seams for Planning

1. **Executable package-consumer proof (highest risk / first proof)**
   - Files: `tests/package-consumer/main.cpp`.
   - Goal: installed consumer executable creates and reopens one archive per family using public APIs only.
   - Verification: `ctest --preset windows-msvc-debug-static -R package_consumer_smoke --output-on-failure`.

2. **Public evidence docs / matrix update**
   - Files: `docs/coverage-audit-matrix.md`, `docs/public-api-reality-check.md`, `docs/compatibility-evidence.md`, optionally `docs/integration-examples.md` and `docs/api-mainpage.md`.
   - Goal: close COV-GAP-003 truthfully, keep COV-GAP-002/COV-GAP-004 deferred, add package-consumer command to proof sweep.
   - Verification: docs-policy/coverage-matrix label tests after policy updates.

3. **Policy guardrail update**
   - Files: `tests/unit/docs_policy_tests.cpp`, `tests/unit/coverage_audit_matrix_docs_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`.
   - Goal: make stale COV-GAP-003-open language fail and make missing every-family package runtime proof fail.
   - Verification: `ctest --preset windows-msvc-debug-static -L "docs_policy|coverage_audit_matrix|target_format_policy|package_consumer" --output-on-failure`.

4. **Integrated confidence and requirements closeout**
   - Files/tools: GSD requirement updates for at least R007, possibly R003/R008/R009 if the planner chooses to validate all remaining active M001 requirements after proof.
   - Goal: final default build/test/package-consumer pass; no local corpus required; TES5Edit untouched.
   - Verification: full default CTest or agreed focused + full package-consumer proof.

## First Proof

Run package-consumer smoke immediately after the `main.cpp` change before touching docs-policy tests. This isolates COV-GAP-003’s real executable risk.

```powershell
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -R package_consumer_smoke --output-on-failure
```

If this fails, debug the installed consumer executable before updating the matrix. Likely failure points are DDS byte generation/DirectXTex acceptance, runtime DLL copy for shared installs, or a `result<T>::value()` call on an unchecked error.

## Final Verification Recommendation

After code/docs/policy updates:

```powershell
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L "package_consumer|docs_policy|coverage_audit_matrix|target_format_policy|validation_api|compatibility_warning" --output-on-failure
ctest --preset windows-msvc-debug-static --output-on-failure
```

Notes:

- Use serialized CTest runs. Memory MEM019 says concurrent CTest invocations can race with Catch2 `DISCOVERY_MODE PRE_TEST` and corrupt generated discovery files.
- Prior S04 verification reported non-fatal preexisting CMake/MSBuild warnings (`CMAKE_TOOLCHAIN_FILE` manually specified but unused, MSB8029 intermediate/output warnings). Treat failures as actionable, but do not spend S05 scope on unrelated warning cleanup unless they become fatal.
- Optional local corpus checks should remain skipped when environment variables are absent.

## Risks and Watch-outs

- **DDS generation is the highest implementation risk.** Use a minimal DXT10 DDS derived from the existing fixture generator’s known-valid `r8g8b8a8_unorm` case. Do not read `tests/fixtures/generated/source/*.dds` from the package-consumer executable, because that would turn installed-package proof into a source-tree fixture dependency.
- **BA2 DX10 writer is consuming after `write_to`.** Do not retry the same writer instance; create once, write once, then reopen the output.
- **Do not compare exact diagnostics.** Public contracts keep `error_code`/warning codes stable; messages are human-readable only.
- **Do not overclaim variants.** Package-consumer runtime proof should close “one installed runtime archive path per family.” Variant/compression-route depth remains in unit fixture/writer/validation tests.
- **Do not add new dependencies or public headers.** Standard C++ helpers inside the package-consumer test are enough.
- **Workspace is dirty from prior GSD/workflow/generated-fixture state.** gsd_exec `aa86db10-3c2f-4c22-857f-56084ea1d15d` showed many `.gsd.migrating/*` changes and generated fixture manifest changes already present. Executors should avoid reverting prior-slice artifacts and keep S05 changes targeted.

## Don't Hand-Roll

- Do not add a broad facade or convenience layer to close S05; S02 already decided the existing public core is sufficient and COV-GAP-003 is proof granularity, not missing API.
- Do not copy local game archives, BSArchPro output, or TES5Edit files into the test. Keep default proof legal and repository-reproducible.
- Do not create a new package-consumer CMake target unless `main.cpp` cannot stay readable. The existing smoke/install/run path is exactly the proof surface S05 needs.

## Evidence Sources Consulted

- Memory: MEM016 (COV-GAP-003 routed to S05), MEM012/MEM010 (default proof hierarchy and optional local corpus advisory only), MEM019 (CTest/Catch2 discovery race), MEM020 (docs-policy tests should assert public docs, not `.gsd`/local/TES5Edit paths).
- `docs/coverage-audit-matrix.md` — current family matrix and open COV-GAP-003 row.
- `docs/public-api-reality-check.md` — current COV-GAP routing and no-facade conclusion.
- `docs/compatibility-evidence.md` — default proof sweep and optional local corpus boundary.
- `tests/package-consumer/main.cpp` — current installed consumer examples and missing-path-only runtime smoke.
- `tests/package-consumer/smoke.cmake`, `tests/package-consumer/CMakeLists.txt`, `tests/CMakeLists.txt` — install/configure/build/run wiring and package-consumer labels.
- `tests/unit/docs_policy_tests.cpp`, `tests/unit/coverage_audit_matrix_docs_tests.cpp`, `tests/unit/target_format_policy_tests.cpp` — string-policy guardrails that must be updated with COV-GAP-003 closure.
- `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp` — public API contracts for result handling, reader extraction, validation, and writer flows.
- `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` — source of known-valid synthetic DDS construction pattern; use as guidance only, not as a package-consumer runtime dependency.
- gsd_exec `0c65c6f1-7d05-4ee3-bcdc-d972acfdb0a4` — summarized docs/package-consumer/policy file landscape.
- gsd_exec `d62fd33d-dcd0-4589-96e5-5f51b03fda39` — inventoried generated legal fixtures and package-consumer files.
- gsd_exec `a7cfc3f2-c7db-4910-b003-9d63a1f3102c` — confirmed CMake install/export/package-consumer wiring.
