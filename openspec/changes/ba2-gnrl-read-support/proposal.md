## Why

Milestones 1 and 2 delivered BSA read support for TES3 through SSE. Fallout 4 and Starfield ship a different archive family — BA2 with `BTDX` magic — that uses CRC32-based hashing, a trailing file-name table, and version-specific compression (deflate or LZ4 block). Without BA2 GNRL support the library cannot serve FO4/Starfield modding tools, which is the majority of active Bethesda modding today.

## What Changes

- Add `ArchiveFormat::fo4` and `ArchiveFormat::starfield` variants to the public enum.
- Add `CompressionMethod::lz4_block` to support Starfield's per-file LZ4 block compression.
- Implement the `BTDX`+`GNRL` header/record parser covering format versions 1, 2, 7, and 8.
- Implement `CreateHashFO4` (CRC32-based name/dir/extension hash).
- Parse the trailing length-prefixed file-name table located at `FileTableOffset`.
- Detect per-file compression from `PackedSize != 0` and decompress with deflate or LZ4 block as appropriate.
- Extend `ArchiveReader::open()` with BTDX magic detection and routing to the BA2 GNRL parser.
- Support Starfield v2's extra header fields (`Unknown1`, `Unknown2`) and `CompressionMethod = 3` (LZ4 block).

## Capabilities

### New Capabilities
- `ba2-gnrl-read`: Reading and extracting files from Fallout 4 and Starfield GNRL BA2 archives (BTDX+GNRL, versions 1/2/7/8), including format detection, index parsing, hash-based path lookup, file-name table parsing, and transparent decompression (deflate and LZ4 block).

### Modified Capabilities
- `tes4-family-bsa-read`: The archive detection logic must extend to recognize BTDX magic and route to the BA2 parser instead of falling through to the TES4 path.

## Impact

- **Public API**: New enum values in `ArchiveFormat` and `CompressionMethod`. `ArchiveEntry` gains CRC32-based hash fields. No breaking changes to existing consumers.
- **Source files**: New `src/ba2_gnrl_archive.cpp`, `src/ba2_gnrl_archive.hpp`, `src/ba2_hash.cpp`, `src/ba2_hash.hpp`. Modified `src/archive_reader.cpp` for routing.
- **Dependencies**: No new external dependencies. Uses existing `libdeflate` and `lz4` (block API instead of frame API used for SSE).
- **Tests**: New fixture-based tests against FO4 and Starfield GNRL BA2 archives. CRC32 hash unit tests.
