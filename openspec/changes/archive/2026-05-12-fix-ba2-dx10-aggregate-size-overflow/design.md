## Context

`src/formats/ba2/ba2_dx10_parser.cpp` validates each DX10 chunk span before materializing public `entry_metadata`. The current materialization step then sums ordered chunk `raw_size` and `stored_size` values with unchecked `std::uint64_t` addition, and adds the reconstructed 148-byte DDS header size to the raw aggregate for the exposed entry size.

The existing BA2 DX10 on-disk fields make a real archive-level `std::uint64_t` aggregate overflow unreachable: each texture has at most 255 chunks because `ChunkCount` is UInt8, and each chunk's raw and stored size is represented as UInt32. The maximum public raw size is therefore `148 + 255 * UINT32_MAX`, far below `UINT64_MAX`. The parser should still use checked arithmetic at this boundary because downstream callers rely on public metadata as trustworthy sizes, and future internal widening should not expose wrapped values.

## Goals / Non-Goals

**Goals:**

- Use checked addition while summing BA2 DX10 chunk raw sizes.
- Use checked addition while summing BA2 DX10 chunk stored sizes.
- Use checked addition while adding the reconstructed DDS header size to the aggregate raw payload size.
- Return `libbsa::error_code::format_error` if an internal aggregate ever becomes unrepresentable before exposing `entry_metadata` values.
- Cover the checked-add helper directly and document the BA2 DX10 archive-schema bound that prevents a malformed archive fixture from triggering the overflow path today.

**Non-Goals:**

- Change public metadata types or API signatures.
- Clamp, saturate, or otherwise recover from impossible aggregate sizes.
- Change BA2 DX10 chunk ordering, DDS header reconstruction, compression routing, or extraction semantics.
- Add impossible malformed BA2 DX10 fixtures that require more chunks or wider chunk-size fields than the on-disk record format can encode.
- Refactor unrelated local checked-arithmetic helpers in writer/layout code.

## Decisions

- Use checked unsigned addition at every aggregate step rather than validating only after the loop.
  - Rationale: once unsigned overflow happens, the wrapped value no longer preserves enough information to diagnose the malformed record reliably.
  - Alternative considered: compare the final sum against the archive size. This misses raw decoded-size overflow because decoded sizes are not bounded by stored archive bytes in the same way compressed spans are.

- Do not create aggregate-overflow BA2 DX10 malformed archive fixtures for this change.
  - Rationale: the current record format cannot encode enough chunks or wide enough chunk sizes to overflow `std::uint64_t` for one entry. A fixture claiming to do so would need to violate the schema before reaching aggregate materialization.
  - Alternative considered: corrupting chunk sizes to `UINT32_MAX`. This can test other malformed layout checks, but even 255 such chunks plus the DDS header still fits `std::uint64_t`.

- Report aggregate overflow as `format_error` from BA2 DX10 open/materialization.
  - Rationale: the archive metadata is internally inconsistent with representable public entry metadata, matching existing parser behavior for invalid spans, invalid chunk sizes, and invalid texture chunk layouts.
  - Alternative considered: expose `uint64_t`-wrapped values and defer failure to extraction. This keeps malformed metadata visible through list/find APIs and preserves the bug.

- Keep validation in `materialize_entries` after chunk layout ordering.
  - Rationale: the public aggregate sizes are computed from `texture_chunk_metadata`, so the check should protect the exact values written into `entry_metadata`.
  - Alternative considered: reject earlier while reading raw chunk records. That would duplicate public-size aggregation before layout validation and make the check easier to diverge from exposed metadata.

- Prefer the existing parser arithmetic style and add only the smallest helper needed for `std::uint64_t` addition if no suitable helper already exists.
  - Rationale: parser code already uses shared primitives for archive-controlled arithmetic. A named checked-add helper keeps diagnostics and review focus clear without introducing a new dependency or public type.
  - Alternative considered: inline `max - lhs` checks in the loop. This is minimal but easier to repeat incorrectly around the DDS header addition.

## Risks / Trade-offs

- The defensive parser error branches are not currently reachable through a validly encoded BA2 DX10 archive because of UInt8/UInt32 record-field bounds. Direct helper tests and an explicit parser invariant test provide reachable coverage for the arithmetic rule.
- Adding a shared `uint64_t` checked-add helper may reveal duplicated local helpers elsewhere -> avoid broad refactors in this change unless required for the BA2 DX10 parser fix.
