## Context

TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writers all follow the same high-level flow: validate/prep inputs, write a completed archive to a temporary host path, and publish that file to the requested output path. The publish phase has drifted across writer families. BSA writers use `detail::replace_file_atomically` for overwrite, while BA2 writers carry local backup reservation and rollback code in addition to their no-overwrite paths.

The change is internal to writer finalization. It must preserve the public `overwrite_existing` contract, keep diagnostics identifiable by writer family, avoid touching `TES5Edit/`, and avoid new dependencies.

## Goals / Non-Goals

**Goals:**
- Put temp directory reservation, temp cleanup, output existence/race checks, no-overwrite publication, overwrite replacement, backup/rollback handling, and publish error shaping behind one internal helper.
- Route TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writers through the shared helper with only a diagnostic prefix and writer callback varying by format.
- Preserve no-overwrite behavior: existing or raced destinations fail without replacing caller-owned bytes.
- Preserve overwrite behavior while removing BA2's larger move-aside crash window from writer-local code.
- Cover the helper and every writer family with focused regression tests.

**Non-Goals:**
- No public writer API changes.
- No changes to archive serialization, compression, hashing, DDS metadata, or entry ordering.
- No broad filesystem portability work beyond the existing Windows-first helper boundary.
- No changes under `TES5Edit/`.

## Decisions

1. Add a shared internal publish helper with a callback for writing the temporary archive.

The helper should live in `src/detail` because it is not BA2-specific and will be used by both BSA and BA2 writers. A callback shape such as `write_temp(temp_path) -> result<void>` lets the helper own temp path reservation and cleanup while leaving format-specific byte generation in the existing writer files. This is preferable to a helper that only renames an already-created temp file, because that would still leave duplicated temp directory reservation and cleanup in every writer.

2. Use the existing atomic file operations as the primitive layer.

The helper should continue to call `detail::publish_file_without_replace` for no-overwrite finalization and `detail::replace_file_atomically` for overwrite finalization where the destination is a regular file or absent. Keeping these primitives avoids reintroducing copy-based publish behavior and avoids BA2's writer-local move-aside window. If backup/rollback behavior remains necessary for a narrow edge case, it should be private to the shared helper rather than duplicated in BA2 writers.

3. Preserve writer-specific diagnostics through a prefix parameter.

Each writer should pass a stable prefix such as `TES3 BSA writer`, `TES4 BSA writer`, `BA2 GNRL writer`, or `BA2 DX10 writer`. The helper should compose publish failures from that prefix so existing tests and users can still tell which writer failed without each writer owning its own publish control flow.

4. Prove routing and behavior rather than only success-path round trips.

Tests should include helper-level behavior for no-overwrite, overwrite, cleanup, and rollback/error shaping, plus writer-level source/behavior checks that all four writer families delegate publish finalization through the shared helper. Existing archive round-trip tests are useful but not enough, because the regression risk is in failure and race behavior at the host path boundary.

## Risks / Trade-offs

- Helper callback hides control flow from writer files -> Keep the helper small, documented, and narrowly named around final publication rather than general write orchestration.
- Filesystem failure cases are hard to simulate deterministically -> Keep injectable operations or helper-level seams only in test-only/internal code paths, and use source-policy checks where OS-level fault injection would be brittle.
- Existing diagnostics may change slightly -> Parameterize the prefix and assert the important format-specific text in tests.
- Current BA2 rollback tests reference `formats/ba2/ba2_publish.hpp` -> Move or replace them with helper-level tests so rollback coverage follows the shared implementation.

## Migration Plan

1. Add the shared helper and tests while leaving existing writers untouched.
2. Migrate TES3 BSA and TES4 BSA writers to the helper, preserving existing no-overwrite and overwrite tests.
3. Migrate BA2 GNRL and BA2 DX10 writers to the helper, then remove BA2-only publish helper code that becomes obsolete.
4. Run focused writer publish tests, build `libbsa_tests`, run the relevant BSA/BA2 writer test labels, and finish with the full Windows Debug static CTest preset if available.

## Open Questions

- None.
