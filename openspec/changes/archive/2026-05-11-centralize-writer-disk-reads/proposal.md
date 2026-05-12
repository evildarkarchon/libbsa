## Why

Writer preparation currently has several whole-file disk read loops that grow `std::vector<std::byte>` one byte at a time before compression, deduplication, or DDS analysis. This creates avoidable memory pressure and slow packing hot paths for large payloads, especially where compressed BSA/BA2 entries still require whole-buffer codec boundaries.

## What Changes

- Add shared writer-side disk source helpers for bounded full-file reads, prefix reads, and chunked reads used for hashing or streaming comparisons.
- Replace duplicated byte-at-a-time read loops in TES4 BSA, BA2 GNRL, BA2 DX10, and compression-adjacent writer preparation paths with the shared helpers.
- Keep current whole-buffer compression and DirectXTex analysis boundaries explicit, but make the allocation and read behavior pre-sized and result-based.
- Preserve archive bytes, target-specific compression routing, disk-source mutation diagnostics, and public writer APIs.

## Capabilities

### New Capabilities
- `writer-bounded-source-reads`: Defines shared writer disk-source read behavior for bounded allocation, chunked transfer, and stable error translation across writer preparation paths.

### Modified Capabilities
- None.

## Impact

- Affected implementation areas: `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, and `src/detail/compression_router.cpp` or adjacent detail helpers.
- Public API impact: none expected.
- Dependency impact: none expected; use the standard library and existing libbsa result/error helpers.
- Test impact: add focused writer helper tests plus regression coverage for compressed disk entries, DDS source analysis, source mutation detection, and unchanged archive bytes.
