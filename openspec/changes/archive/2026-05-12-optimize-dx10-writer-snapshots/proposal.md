## Why

BA2 DX10 writer finalization currently reopens each snapshot file per planned subresource, appending bytes into chunk buffers without reserving the known raw chunk size first. Snapshotting is still the right ownership boundary for mutable DDS inputs, but the finalization path needs less avoidable disk and allocation overhead before changing that ownership model.

## What Changes

- Preserve `add_file` snapshot ownership semantics so callers may still mutate or delete DDS source files after add-time validation.
- Batch BA2 DX10 snapshot subresource reads during chunk assembly so a planned texture chunk reads its required snapshot payloads through a single prepared flow instead of repeated ad hoc append work.
- Pre-size or reserve raw BA2 DX10 chunk buffers from planned chunk metadata before copying snapshot bytes.
- Add benchmark coverage that reports DX10 snapshot staging/finalization cost separately enough to measure the benefit and catch regressions.
- Keep archive bytes, compression routing, chunk planning, temp-directory cleanup, and public writer APIs unchanged.

## Capabilities

### New Capabilities
- `dx10-snapshot-read-efficiency`: BA2 DX10 writer snapshot-backed chunk assembly preserves snapshot semantics while reducing avoidable finalization reads, allocations, and unmeasured overhead.

### Modified Capabilities
- None.

## Impact

- Affected implementation files: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, and `src/texture/directxtex_analyzer.cpp` if ownership metadata needs small internal adjustments.
- Affected benchmark file: `benchmarks/libbsa_benchmarks.cpp`.
- Public APIs, archive format behavior, dependencies, and TES5Edit reference files are unchanged.
