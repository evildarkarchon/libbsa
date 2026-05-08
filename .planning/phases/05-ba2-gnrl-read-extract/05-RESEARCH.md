# Phase 05: BA2 GNRL Read/Extract - Research

**Researched:** 2026-05-08
**Status:** Complete

## Research Summary

Phase 5 should implement BA2 GNRL support as a new private `src/formats/ba2/` reader slice while preserving the public `archive_reader` facade established in Phases 3-4. TES5Edit `Core/wbBSArchive.pas` is the primary reference: `BTDX` identifies BA2, the following version selects Fallout 4 or Starfield, the fixed BA2 header stores subtype magic (`GNRL`/`DX10`), `FileCount`, and `FileTableOffset`, and Starfield v2/v3 append raw header fields after the common header.

## Reference Findings

- TES5Edit defines `TwbBSHeaderFO4` as `Magic`, `FileCount`, `FileTableOffset` and `TwbBSHeaderSFv2` as `Unknown1`, `Unknown2`; `TwbBSHeaderSFv3` adds `CompressionMethod`.
- TES5Edit reads GNRL records as `NameHash`, `Ext`, `DirHash`, `Unknown`, `Offset`, `PackedSize`, `Size`, and a trailing `BAADF00D` sentinel.
- TES5Edit reads names by seeking to `FileTableOffset` and reading one UInt16 length-prefixed filename per record. The existing libbsa path policy should convert separators to `/` and canonicalize through `detail::normalize_archive_path`.
- BA2 raw-vs-compressed classification is record-driven: `PackedSize == 0` means raw; non-zero `PackedSize` is compressed and the decompressed output must exactly match `Size`.
- Starfield v3 `CompressionMethod == 3` selects raw LZ4 block. The default non-LZ4 path remains deflate only for evidence-bounded method values used in generated fixtures; unsupported values fail `archive_reader::open` with `error_code::unsupported`.
- `DX10` is a valid BA2 subtype but belongs to Phase 6; Phase 5 should inspect only enough header state to return `error_code::unsupported` without parsing texture records.

## Recommended Architecture

- Add public `ba2_archive_metadata` as a small typed optional nested field on `archive_metadata`, with version-gated `std::optional<std::uint32_t>` values named `starfield_unknown1`, `starfield_unknown2`, and `compression_method` per D-01 through D-03.
- Add `src/formats/ba2/ba2_format_detector.*`, `ba2_gnrl_parser.*`, and `ba2_gnrl_reader.*`. Keep all parser structs private and expose only `archive_metadata` / `entry_metadata` values.
- Extend `archive_reader::state` with a private format discriminator or use `archive_metadata.type` to dispatch BA2 listing/lookup/extraction without changing public caller flow.
- Reuse `detail::hash_fo4`, `detail::normalize_archive_path`, and `detail::decompress_payload_exact`. Do not add dependencies.
- Add `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` with legal synthetic FO4, Starfield v2, Starfield v3, DX10 rejection, malformed, corrupted-compression, and unsupported-v3-method fixtures.

## Validation Architecture

- Test framework: Catch2 via existing `libbsa_tests`; generated fixtures through CMake custom targets.
- Quick command: `ctest --preset windows-msvc-debug-static -L ba2_gnrl --output-on-failure`.
- Full command: `ctest --preset windows-msvc-debug-static --output-on-failure` plus `git -C TES5Edit status --short` clean gate.
- Required labels: `ba2_gnrl_fixtures`, `ba2_gnrl_detector`, `ba2_gnrl_metadata`, `ba2_gnrl_lookup`, `ba2_gnrl_extract`, `ba2_gnrl_malformed`, `public_include_boundary`, `tes3_bsa`, `tes4_bsa`.

## Pitfalls to Avoid

- Do not parse DX10 chunk records in Phase 5.
- Do not infer compression from extensions, record extension fields, or archive path names.
- Do not expose DirectXTex, lz4, libdeflate, TES5Edit, or private parser types from public headers.
- Do not treat absent Starfield optionals as zero.
- Do not assert diagnostic message text in malformed tests; assert stable `error_code` values only.
