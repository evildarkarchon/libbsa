# Phase 07: ba2-dds-read-and-dds-reconstruction - Research

**Researched:** 2026-05-05T21:23:25.3309455-07:00
**Domain:** C++20 BA2 DX10 texture parsing, DDS reconstruction, DirectXTex validation, and fixture-driven testing
**Confidence:** HIGH for project API/test patterns; MEDIUM for exact BA2 DX10 byte layout until executor traces TES5Edit reference code during implementation

## Summary

Phase 7 should extend the existing BA2 reader rather than introduce a new archive abstraction. The public surface stays in `include/libbsa/ba2.hpp`, with `ba2_archive::texture_metadata(path)` returning copied libbsa-owned metadata by value. Texture-specific concepts should not be added to generic `entry_metadata`.

DirectXTex is best used behind private implementation and test helpers. The implementation should reconstruct DDS/DDS-DX10 headers from parsed BA2 metadata, then validate reconstructed bytes with DirectXTex before writing to the caller sink where practical. DirectXTex documentation confirms `LoadFromDDSMemory` and `GetMetadataFromDDSMemory` can validate DDS buffers and expose `TexMetadata` fields for width, height, mip levels, array size, DXGI format, and cubemap state.

The highest-risk implementation points are DX10 texture record/chunk layout, chunk-to-mip association, codec routing per chunk, complete-buffer-before-sink-write behavior, and public-header dependency leakage. Generated source fixtures should be the proof vehicle.

## User Constraints

- Add `ba2_archive::texture_metadata(path)` and return `result<texture_metadata>` by value.
- Use public libbsa-owned `dxgi_format` preserving the raw DXGI numeric value plus a known-name helper.
- Preserve unknown DXGI values for inspection; fail extraction only when valid DDS reconstruction is unsupported.
- Expose logical chunk summaries with offsets, packed/unpacked sizes, and mip ranges when derivable.
- Validate/decompress/reconstruct a complete DDS byte vector before writing to the caller sink.
- Route DX10 chunk codecs from archive variant, chunk sizes, and Starfield `CompressionMethod`; never try fallback codecs.
- Reconstruct DDS headers in libbsa, validate through private DirectXTex boundaries, and keep helpers internally reusable for writer phases.
- Use a reusable private generated-fixture helper module; validate every positive extracted DDS fixture with DirectXTex.
- Prefer semantic DDS equivalence over universal byte-exact comparisons.

## Architecture Recommendations

| Area | Recommendation | Rationale |
|------|----------------|-----------|
| Public metadata | Add `texture_metadata`, `texture_chunk_metadata`, `dxgi_format`, and `dxgi_format_name` to `ba2.hpp` or a public BA2-adjacent header | Keeps texture metadata consumer-friendly while avoiding DirectXTex/Windows header leakage |
| Archive object | Extend `ba2_archive` to own copied texture metadata alongside generic `archive_view` metadata | Preserves the Phase 6 metadata-only lifetime model |
| Parser | Split BA2 parsing into subtype-specific helpers while sharing bounded read/range helpers | Prevents GNRL and DX10 behavior from tangling while preserving local parser style |
| Texture internals | Add private `src/texture/dds_reconstruction.*` and `src/texture/dds_validation.*` helpers | Reusable for Phase 10 writer work without exposing public DDS utilities |
| Fixtures | Add `tests/ba2_dds_fixture_helpers.*` and focused BA2 DDS tests | Keeps generated fixture corpus reviewable and reusable |
| Validation | Use DirectXTex `LoadFromDDSMemory` or `GetMetadataFromDDSMemory` in private test/validation adapter | Confirms DDS output loadability and metadata correctness |

## Pitfalls

- Do not reject unknown DXGI values during open if metadata is otherwise parseable; preserve the numeric value.
- Do not use Skyrim/SSE LZ4 frame helpers for Starfield BA2 v3 method-3 texture chunks; use raw LZ4 block routing.
- Do not write partial DDS bytes to caller sinks. Buffer, validate, then write once.
- Do not use `.dds` file names to infer archive subtype; `GNRL` `.dds` entries remain ordinary payloads from Phase 6.
- Do not expose DirectXTex, DXGI headers, Windows SDK, libdeflate, LZ4, or TES5Edit symbols from public headers.
- Do not mutate, format, compile, stage, or move anything under `TES5Edit/`.

## Validation Strategy

| Requirement | Proof |
|-------------|-------|
| BA2-05 | Generated FO4 DX10 v1/v7/v8 and Starfield DX10 v3 fixtures open, list paths, lookup entries, and return exact generic plus texture metadata |
| BA2-06 | Extracted DDS bytes validate through DirectXTex with expected dimensions, format, mip count, array size, and cubemap state |
| D-01 through D-08 | Public-header smoke covers `texture_metadata(path)`, `texture_metadata`, `texture_chunk_metadata`, `dxgi_format`, and known-name helper with no private dependency leakage |
| D-09 through D-13 | Parser and extraction tests assert logical chunk summaries, mip ranges, chunk routing, no fallback codecs, and full-buffer-before-sink-write failure behavior |
| D-14 through D-18 | Private DDS reconstruction tests and BA2 extraction tests assert validation failures are structured and no partial writes occur |
| D-19 through D-23 | Fixture helper tests cover variant/layout/codec matrix, targeted malformed classes, DirectXTex validation, and semantic equivalence |

## DirectXTex Notes

- `LoadFromDDSMemory(const void*, size_t, DDS_FLAGS, TexMetadata*, ScratchImage&)` loads DDS bytes from memory and returns metadata plus image data.
- `GetMetadataFromDDSMemory(const void*, size_t, DDS_FLAGS, TexMetadata&)` retrieves DDS metadata without retaining the buffer.
- `TexMetadata` includes width, height, depth, array size, mip levels, format, dimension, flags, and helpers such as `IsCubemap()`.
- Keep all DirectXTex includes in private implementation or tests; translate to libbsa-owned metadata before crossing the public boundary.

## Sources

- `.planning/phases/07-ba2-dds-read-and-dds-reconstruction/07-SPEC.md` - locked Phase 7 scope and acceptance criteria.
- `.planning/phases/07-ba2-dds-read-and-dds-reconstruction/07-CONTEXT.md` - user decisions D-01 through D-23.
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-CONTEXT.md` - BA2 API, metadata-only lifetime, compression route, and fixture precedents.
- `.planning/research/STACK.md` - dependency and public API leakage constraints for DirectXTex, libdeflate, LZ4, and Catch2.
- `docs/PRD.md` - BA2 DDS read deliverables and success criteria.
- `AGENTS.md` - project instructions, comment policy, dependency rules, and TES5Edit read-only boundary.
- `include/libbsa/ba2.hpp`, `include/libbsa/archive.hpp`, `src/ba2_reader.cpp`, `src/compression.cpp`, `tests/ba2_reader_tests.cpp`, `tests/public_header_smoke.cpp`, `CMakeLists.txt` - existing implementation/test/build patterns.
- Context7 `/microsoft/directxtex` - DirectXTex DDS memory load and metadata APIs.

## Assumptions Log

| # | Claim | Risk if Wrong | Mitigation |
|---|-------|---------------|------------|
| A-01 | Exact BA2 DX10 record/chunk field order can be traced from TES5Edit and encoded in generated fixtures | Parser plans may need small layout adjustment | Plan 03 requires executor to trace `TES5Edit/BSArch/` and `TES5Edit/Core/wbBSArchive.pas` before implementing record offsets |
| A-02 | DirectXTex can validate the generated DDS fixtures used by tests on the local vcpkg build | Test adapter may need platform/compiler guards | Keep DirectXTex private and add focused build/test validation before relying on it broadly |

