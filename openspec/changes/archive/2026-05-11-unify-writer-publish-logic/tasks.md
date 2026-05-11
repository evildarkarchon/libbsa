## 1. Shared Publish Helper

- [x] 1.1 Add focused helper tests for temp-directory reservation, no-overwrite publish races, overwrite replacement, diagnostic prefixes, and best-effort cleanup.
- [x] 1.2 Implement a shared internal writer publish helper under `src/detail` that accepts `output_path`, `overwrite_existing`, a writer diagnostic prefix, and a temporary-file writer callback.
- [x] 1.3 Route helper overwrite publication through the existing atomic replacement primitive and no-overwrite publication through the existing no-replace primitive.
- [x] 1.4 Move or replace BA2 rollback helper coverage so rollback/error-shaping tests exercise the shared helper instead of BA2-only publish code.

## 2. Writer Migration

- [x] 2.1 Migrate `tes3_bsa_writer` to the shared helper while preserving existing serialization and `overwrite_existing` behavior.
- [x] 2.2 Migrate `tes4_bsa_writer` to the shared helper while preserving existing serialization and `overwrite_existing` behavior.
- [x] 2.3 Migrate `ba2_gnrl_writer` to the shared helper and remove BA2-local backup/rollback code made obsolete by the shared helper.
- [x] 2.4 Migrate `ba2_dx10_writer` to the shared helper and remove BA2-local backup/rollback code made obsolete by the shared helper.
- [x] 2.5 Remove duplicated writer-local temp-directory reservation, cleanup, final publish, backup, and rollback helpers that are no longer used.

## 3. Writer Regression Coverage

- [x] 3.1 Add or update writer-level tests proving TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 preserve existing destinations when overwrite is disabled.
- [x] 3.2 Add or update writer-level tests proving TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 overwrite existing regular archives only when overwrite is enabled.
- [x] 3.3 Add source-policy or behavior tests proving all four writer families route final publication through the shared helper and retain format-specific diagnostic prefixes.

## 4. Validation

- [x] 4.1 Build the test target with `cmake --build --preset windows-msvc-debug-static --target libbsa_tests --config Debug`.
- [x] 4.2 Run focused BSA and BA2 writer publish tests with the Windows Debug static CTest configuration.
- [x] 4.3 Run the full Windows Debug static CTest preset with output on failure.
- [x] 4.4 Run `openspec validate "unify-writer-publish-logic" --strict`.
