## Why

Milestone 1 delivered TES4-family (Oblivion/FO3/SSE) BSA read support. Morrowind's BSA format (TES3) is the oldest and simplest variant but uses a completely different header layout, hash algorithm, and offset scheme. Adding TES3 read support completes BSA coverage for all pre-BA2 Bethesda titles and exercises the extensibility of the archive reader architecture.

## What Changes

- New TES3 binary structures (header, file records) and parsing logic.
- New TES3-specific hash algorithm (`CreateHashTES3`) with its two-half split-string approach.
- TES3-specific offset calculation (offsets relative to the data section, not the file start).
- Extend `ArchiveFormat` enum and `ArchiveReader::open()` to detect and dispatch TES3 archives.
- No compression support needed -- TES3 stores files as raw bytes.
- Tests against Morrowind BSA fixture archives.

## Capabilities

### New Capabilities
- `tes3-bsa-read`: Reading, parsing, and extracting files from Morrowind TES3 BSA archives, including the TES3 hash algorithm implementation and the TES3-specific on-disk layout (hash table, name table, size/offset table, data section).

### Modified Capabilities
- `tes4-family-bsa-read`: The archive type detection logic (magic-based dispatch in `ArchiveReader::open()`) needs to be extended to recognize the TES3 magic (`0x00000100`) and route to the new TES3 parser. No behavioral change to existing TES4/FO3/SSE reading.

## Impact

- **Public API**: `ArchiveFormat` enum gains a `tes3` variant. `CompressionMethod::none` already exists. No new methods needed -- the existing `ArchiveReader` surface handles TES3 transparently.
- **Internal**: New files `tes3_archive.hpp/cpp` and `tes3_hash.hpp/cpp` in `src/`. The `archive_reader.cpp` dispatch logic grows a TES3 branch.
- **Dependencies**: None -- TES3 has no compression, so no new library deps.
- **Tests**: New test fixtures with small hand-crafted or extracted Morrowind BSA archives.
