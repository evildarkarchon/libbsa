## Context

The shared writer publish helper writes completed archive bytes into a temporary directory next to the requested output path, then publishes the completed file with the platform file-move primitive. On Windows this uses `MoveFileExW` for both no-overwrite and overwrite publication, which gives the desired atomic destination-name behavior for ordinary same-volume local filesystem paths.

The helper already refuses existing non-regular overwrite targets, keeps non-overwrite publication atomic with respect to the destination name, and cleans only the writer-owned temporary directory. The remaining gap is that the contract is implicit: docs do not explain same-volume, network-share, or reparse-point expectations, and tests do not fully cover read-only destinations or reparse-point refusal.

## Goals / Non-Goals

**Goals:**
- Document writer publication constraints in consumer-facing docs and/or public writer API comments.
- Keep the temporary output directory next to the destination so normal saves use same-volume publish semantics.
- Add tests for directory destinations, read-only existing destinations, and reparse-point destinations when the CI host supports creating them.
- Refuse detectable reparse-point overwrite targets before publication so archive writers do not replace or follow surprising filesystem indirections.
- Preserve writer-specific diagnostics and the existing `overwrite_existing` API shape.

**Non-Goals:**
- No new public writer options or behavior switches.
- No attempt to guarantee atomic behavior for every network filesystem or unusual reparse-point provider.
- No archive serialization, compression, hashing, DDS metadata, or entry-order changes.
- No work under `TES5Edit/`.

## Decisions

1. Treat output publish constraints as part of the writer-safe-publish contract.

The behavior affects all writer families because TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 all delegate to the shared helper. Capturing this under `writer-safe-publish` avoids scattering the same expectation across format-specific capabilities.

Alternative considered: create a new documentation-only capability. That would obscure the fact that the docs describe concrete writer publish safety behavior and related test obligations.

2. Document supported and caveated filesystem shapes instead of promising universal atomicity.

The docs should say that libbsa creates the writer-owned temporary directory beside the destination to keep normal output publication on the same volume, that existing overwrite targets must be regular files, that reparse points may be rejected, and that network/reparse-provider semantics can still be platform-specific. This sets the caller expectation without overclaiming guarantees that Windows cannot provide for every filesystem provider.

Alternative considered: silently rely on implementation details. That leaves consumers unaware of path-shape constraints until a save fails in production.

3. Refuse detectable reparse-point overwrite destinations at the helper boundary.

`std::filesystem::is_regular_file` can be insufficient for Windows reparse-point policy because it may classify a symlink-to-file as regular. The helper should use a narrow Windows attribute check for `FILE_ATTRIBUTE_REPARSE_POINT` before allowing overwrite publication, then retain the existing regular-file check and `MoveFileExW` publish primitives. The check belongs in `src/detail/writer_publish.cpp` so all writer families inherit the same policy.

Alternative considered: leave reparse points to `MoveFileExW`. That can produce provider-specific outcomes and makes it harder to document a deterministic refusal policy.

4. Put most edge-case coverage at the shared helper, with writer-level coverage for delegation-sensitive cases.

Directory, read-only file, and reparse-point tests should primarily exercise `detail::publish_writer_output` because that is where the policy lives. Existing writer-family tests should be extended only where needed to prove public save paths surface the helper diagnostic and do not bypass the helper.

Alternative considered: duplicate every filesystem edge case across all four writer families. That would make tests slower and more brittle without adding much signal beyond the existing delegation-boundary test.

## Risks / Trade-offs

- Reparse-point creation may require privileges or developer-mode settings -> Make the reparse-point test conditional and explicit when the host cannot create the fixture.
- Read-only file attributes can leak after failed tests -> Test cleanup must restore writable attributes before removing files.
- Network filesystem behavior cannot be reliably simulated in CI -> Document the expectation and test the local helper behavior that libbsa controls.
- Windows attribute checks add platform-specific helper code -> Keep the check private to `src/detail` and do not expose Win32 types in public headers.

## Migration Plan

1. Add or update writer publish docs before changing code so the intended contract is visible.
2. Add helper tests for read-only existing destinations and conditional reparse-point refusal, plus fill any missing directory-target coverage.
3. Add the shared helper reparse-point check if existing behavior does not already refuse the tested case deterministically.
4. Extend writer-level tests only for gaps not covered by the helper and existing delegation-boundary assertions.
5. Run focused writer publish tests, relevant writer family tests, the full Windows Debug static CTest preset if available, and `openspec validate document-output-publish-safety --strict`.

## Open Questions

- None.
