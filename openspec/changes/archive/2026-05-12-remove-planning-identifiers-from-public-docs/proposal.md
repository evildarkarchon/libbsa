## Why

Public headers and published documentation still contain planning-era labels such as `Phase 3`, `Phase 11`, `Phase 12`, `D-23`, and `D-09`. These labels make consumer-facing library documentation read like internal planning notes and can imply stale milestone scope even though libbsa now supports broader archive behavior.

## What Changes

- Rewrite public API comments and published docs to describe durable behavior names instead of milestone, phase, or decision identifiers.
- Preserve accurate compatibility rationale, thread-safety guarantees, benchmark policy, fixture policy, and format-scope explanations while removing internal planning labels from public surfaces.
- Keep planning identifiers in planning artifacts only; do not move them into public headers, shipped docs, or fixture documentation.
- Add targeted validation so future public documentation changes cannot reintroduce planning-era identifiers.

## Capabilities

### New Capabilities
- `public-documentation-stability`: Defines the public documentation contract that consumer-facing headers, docs, and fixture guidance use stable behavior wording rather than internal planning identifiers.

### Modified Capabilities

## Impact

- Affected public headers: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`.
- Affected public docs and fixture guidance: `docs/thread-safety.md`, `docs/compatibility-evidence.md`, `tests/fixtures/README.md`.
- Affected tests: documentation policy coverage that scans or otherwise validates public documentation surfaces.
- No public API signature, archive format, dependency, build-system, or runtime behavior changes are intended.
