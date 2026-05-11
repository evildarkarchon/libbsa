## Why

Archive writers currently duplicate host-file publish behavior across BSA and BA2 families, and the duplicated branches do not all use the same replacement strategy. Unifying this logic reduces the chance that a future writer exposes a partial archive, loses an existing archive during overwrite, or leaves temp/backup artifacts behind after a failure.

## What Changes

- Add one shared writer publish helper that owns temporary output directory reservation, no-overwrite publication, overwrite replacement, rollback/error shaping, and best-effort cleanup.
- Update TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writers to delegate final host-path publication to that helper after each writer finishes producing the temporary archive bytes.
- Preserve each writer's format-specific diagnostic prefix while making the underlying no-overwrite and overwrite semantics uniform.
- Add regression coverage that proves all writer families route through the shared helper and keep the expected no-overwrite/overwrite failure behavior.

## Capabilities

### New Capabilities
- `writer-safe-publish`: Defines the shared final-publication contract for archive writers, including no-overwrite behavior, overwrite replacement, rollback expectations, cleanup, and diagnostics.

### Modified Capabilities
- None.

## Impact

- Affected source files: `src/detail/atomic_file_ops.hpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, and `src/formats/ba2/ba2_publish.hpp`.
- Affected tests: focused unit/regression tests for writer publish behavior across BSA and BA2 writer families.
- Public API impact: none expected; this is an internal writer finalization refactor with preserved `overwrite_existing` behavior.
- Dependency impact: none.
