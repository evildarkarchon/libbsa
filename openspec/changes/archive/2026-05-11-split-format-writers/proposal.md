## Why

The four format writer translation units (`tes3_bsa_writer.cpp`, `tes4_bsa_writer.cpp`, `ba2_gnrl_writer.cpp`, `ba2_dx10_writer.cpp`) each fuse five distinct responsibilities — public writer class, source preparation, payload assignment with deduplication, archive serialization, and orchestration — into a single dense file ranging from 380 to 1030 lines, with several similarly-named helpers (`prepare_entries`, `assign_*_offsets`, `write_archive_bytes`, `validate_entries`) repeating that shape across files. Small changes to compression routing, deduplication, offset assignment, or serialization currently force edits inside large cross-coupled units, raising the chance that a fix lands in one writer but silently drifts in another.

## What Changes

- Split each writer family's `*_writer.cpp` along its existing responsibility boundaries into dedicated translation units for source preparation, payload assignment + deduplication + offset planning, archive serialization, and a thin orchestrator that wires the stages together and delegates publish to the existing `writer-safe-publish` helper.
- Keep the public writer class (`tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, `ba2_dx10_writer`) and its `add_file` / `add_bytes` / `write_to` surface in its current header and translation unit; only the internal pipeline below `write_to` is reorganized.
- Move behavior, not semantics: prepared-entry layouts, hash and offset math, deduplication equality, compression routing, DDS chunk planning, table layout, and emitted byte order remain identical. No archive-format byte changes are intended.
- Migrate existing writer tests in lockstep so each responsibility unit gains direct fixture coverage where today only the orchestrator path exercises it; preserve all existing round-trip and compatibility tests as the regression net.
- Mark internal headers/symbols `private` to the libbsa target (no new public API surface). The shared `writer-safe-publish` helper and `shared-parser-primitives` remain the only cross-format writer collaborators.

## Capabilities

### New Capabilities
- `archive-writer-layering`: Internal architectural contract requiring each archive writer family to organize its implementation into separate translation units for source preparation, payload assignment / deduplication / offset planning, archive serialization, and orchestration, with the orchestrator delegating final host-path publication to `writer-safe-publish`.

### Modified Capabilities
- None.

## Impact

- Affected implementation files: `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, plus new internal headers and translation units under `src/formats/bsa/` and `src/formats/ba2/` for the per-stage units. The public writer headers (`*_writer.hpp`) are unchanged.
- Affected build files: CMake target source lists for the libbsa library to register the new translation units.
- Affected tests: existing TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writer unit, round-trip, and compatibility tests; new focused unit tests at the stage boundary (preparation, dedup/offsets, serialization).
- Public API impact: none. No header in `include/libbsa/` changes shape, signature, or exported symbol.
- Behavior impact: none intended. Archive bytes, error messages, and `write_to` semantics remain identical.
- Dependency impact: none. No new vcpkg packages or CMake dependencies; `writer-safe-publish` and `shared-parser-primitives` continue to provide their current contracts.
- TES5Edit submodule impact: none. The submodule remains read-only and is not modified.
