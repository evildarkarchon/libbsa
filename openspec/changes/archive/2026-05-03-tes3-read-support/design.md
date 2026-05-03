## Context

The library currently supports reading TES4-family BSA archives (Oblivion, FO3/FNV, Skyrim LE/SE) through a single `ArchiveReader` class with a pimpl implementation that dispatches to `detail::parse_tes4_archive()`. The entire archive is loaded into a `vector<uint8_t>` and parsed via a `BinaryReader` cursor. File lookup uses a hash-based `unordered_map` keyed by `"folder_hash:file_hash"`.

Morrowind's TES3 BSA format is structurally different: flat file list (no folder hierarchy), different header layout, a unique two-part hash algorithm, no compression, and offsets relative to a data section rather than file start. Despite being simpler, it cannot reuse TES4 parsing logic.

The reference implementation is in `TES5Edit/Core/wbBSArchive.pas` (`OpenArchive` case `baTES3`).

## Goals / Non-Goals

**Goals:**
- Detect TES3 archives via their 4-byte magic (`0x00000100`) and dispatch to a dedicated parser.
- Parse the TES3 on-disk layout: size/offset table, name offset table, filename records, hash table, data section.
- Implement the TES3 hash algorithm with exact behavioral parity to `CreateHashTES3` in BSArchPro.
- Extract files by seeking to `data_offset + record_offset` and reading `record_size` raw bytes.
- Expose TES3 entries through the same `ArchiveReader` / `ArchiveEntry` public API -- callers should not need TES3-specific code.
- File lookup by path via hash comparison (matching BSArchPro's linear scan).

**Non-Goals:**
- TES3 write support (Milestone 8).
- Compression/decompression (TES3 has none).
- Folder-based grouping (TES3 has a flat namespace).
- Performance optimization of lookup (linear scan is fine for Morrowind's small file counts, typically <5000 entries).

## Decisions

### 1. Separate `tes3_archive.hpp/cpp` module (same pattern as TES4)

**Choice:** Create `src/tes3_archive.hpp` and `src/tes3_archive.cpp` with a `detail::parse_tes3_archive()` function that returns a `detail::ParsedArchive`.

**Rationale:** Mirrors the existing TES4 structure. Each format family gets its own parse module. The `ParsedArchive` struct is format-agnostic enough (path, metadata, entries, bytes, lookup map) to hold TES3 data without modification.

**Alternative considered:** Putting TES3 parsing inside the existing `tes4_archive.cpp`. Rejected because the formats share no structural code and mixing them would harm readability.

### 2. Separate `tes3_hash.hpp/cpp` module

**Choice:** Dedicated hash files following the `tes4_hash.hpp/cpp` pattern.

**Rationale:** The TES3 hash algorithm is entirely different from TES4 (split-string XOR/rotate vs. character-table lookup). Separate files keep each algorithm self-contained and independently testable.

### 3. Extend `ArchiveFormat` enum with `tes3` variant

**Choice:** Add `tes3` to the existing `ArchiveFormat` enum. Value `tes3 = 1` (before `tes4 = 2`) to maintain chronological ordering.

**Rationale:** The enum is the public discriminator. Adding a value is a source-compatible change; consumers who switch on it will get a compiler warning if they don't handle the new case.

### 4. Magic detection in `ArchiveReader::open()`

**Choice:** Read the first 4 bytes; if they match `0x00000100`, route to `parse_tes3_archive()`. Otherwise fall through to existing TES4 (`BSA\0`) detection.

**Rationale:** TES3 detection must precede TES4 because TES3's magic is a raw uint32 (`0x00000100`) that doesn't overlap with `BSA\0` (`0x00415342`). Order doesn't functionally matter but checking TES3 first and early-returning keeps the dispatch clean.

### 5. Lookup key format for TES3

**Choice:** Use the 64-bit hash formatted as a hex string (`"<hash_hex>"`) as the `unordered_map` key, since TES3 has no folder/file hash split.

**Rationale:** TES4 uses `"folder_hash:file_hash"`. TES3's single 64-bit hash has no folder component. Using just the hash value as key maintains the same map-based O(1) lookup. The path normalization (lowercase, forward slashes) happens before hashing to match BSArchPro behavior.

### 6. `ArchiveEntry` field mapping for TES3

**Choice:** Map TES3 fields to existing `ArchiveEntry` members:
- `path`: filename from the name table
- `folder_hash`: 0 (no folders in TES3)
- `file_hash`: the full 64-bit TES3 hash
- `data_offset`: `data_section_offset + record_offset` (absolute position in file)
- `stored_size` / `uncompressed_size`: both equal to `record_size` (no compression)
- `packed_size`: 0
- `compression`: `CompressionMethod::none`
- `compressed`: false

**Rationale:** Reuses the existing struct without modification. Fields that don't apply to TES3 get zero/default values. Consumers can check `metadata().format == ArchiveFormat::tes3` if they need format-specific branching.

### 7. Hash table read order (swapped dwords)

**Choice:** Read each hash entry as two uint32 values (high dword first, low dword second) and combine as `(uint64(high) << 32) | low`.

**Rationale:** BSArchPro writes/reads hashes in swapped-dword order, not native little-endian uint64. The implementation must match this to produce correct hash comparisons.

## Risks / Trade-offs

- **Hash collision risk**: TES3 uses linear scan on hash only (no path verification). If two files produce the same 64-bit hash, the first match wins. This matches BSArchPro behavior -- not a bug we need to fix, but worth documenting.
  → Mitigation: Document the behavior; Morrowind's file sets don't have known collisions.

- **Memory usage from full-file load**: TES3 archives are small (Morrowind.bsa is ~300MB), but loading entirely into memory follows the TES4 pattern. This is acceptable now; streaming I/O is deferred to Milestone 9.
  → Mitigation: The existing `ParsedArchive.bytes` pattern works fine for TES3's sizes.

- **Path normalization differences**: TES3 filenames use backslashes on disk. The normalization must convert to forward slashes and lowercase to match hash input expectations.
  → Mitigation: Apply the same `normalize_archive_path()` helper already used for TES4, but verify TES3's `LowerByte` only lowercases A-Z (not locale-aware). Implement a TES3-specific lowering if needed.
