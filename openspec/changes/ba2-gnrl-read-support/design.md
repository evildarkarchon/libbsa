## Context

libbsa currently supports TES3 and TES4-family BSA archives. These use a `BSA\0` magic with hash-based folder/file indexing. Fallout 4 and Starfield introduced a different archive family — BA2 — identified by `BTDX` magic. BA2 archives have a fundamentally different structure:

- A fixed header followed by a type-specific sub-header (`GNRL` or `DX10`).
- Fixed-size per-file records with CRC32-based hash triplets (dir hash, name hash, extension as 4-byte literal).
- A trailing file-name table (at `FileTableOffset`) with length-prefixed strings.
- Per-file compression indicated by `PackedSize != 0` (no archive-level XOR inversion).
- Version-specific behavior: Starfield v2 adds extra header fields; Starfield v3 adds a `CompressionMethod` field that selects LZ4 block instead of deflate.

The existing `ArchiveReader` routes on magic bytes. Adding BA2 GNRL support requires a new parser module that plugs into the same routing and produces the same `ParsedArchive` result type used by TES3/TES4 parsers.

## Goals / Non-Goals

**Goals:**
- Parse and extract all files from BTDX+GNRL BA2 archives at versions 1, 2, 7, and 8.
- Maintain a single `ArchiveReader::open()` entry point that auto-detects BA2.
- Support deflate and LZ4 block decompression depending on archive version/flags.
- Hash-based file lookup using the CRC32 triplet (dir hash + name hash + extension).
- Parse the trailing file-name table for human-readable path listing.

**Non-Goals:**
- BA2 DDS (DX10) support — that is Milestone 4.
- Write support — that is Milestone 6.
- Multi-threaded extraction — that is Milestone 9.

## Decisions

### 1. New source files: `ba2_gnrl_archive.{hpp,cpp}` and `ba2_hash.{hpp,cpp}`

**Rationale:** Keeps BA2 parsing separate from TES4-family code. Same module pattern as `tes3_archive` and `tes4_archive`. The hash algorithm is CRC32-based and unrelated to TES4 hashing, so it gets its own unit.

**Alternatives considered:**
- Single `ba2_archive.{hpp,cpp}` covering both GNRL and DDS → Rejected: DX10 parsing is complex enough to warrant its own module in Milestone 4. GNRL is self-contained.

### 2. BTDX magic detection in `archive_reader.cpp`

**Rationale:** The existing `read_archive_magic` function already reads 4 bytes. Add a `kMagicBtdx` constant and route to `detail::parse_ba2_gnrl_archive(path)`. The BA2 parser will internally read the version and sub-header magic (`GNRL`/`DX10`) to determine the specific format variant.

**Alternatives considered:**
- Reading both magic and version in the router → Rejected: version semantics differ between BSA and BA2 (BSA versions are 0x67-0x69; BA2 versions are 1-8). The format-specific parser handles this internally.

### 3. CRC32 hash using a precomputed lookup table

**Rationale:** The reference implementation uses a hardcoded CRC32 table (standard CRC32 polynomial). The hash is computed per-character with path normalization (lowercase, `/ → \`), skipping bytes > 127. This is a trivial implementation with no external dependency needed.

**Alternatives considered:**
- Using a system CRC32 intrinsic → Rejected: the algorithm applies per-character normalization before hashing; a bulk CRC32 wouldn't match.

### 4. Lookup key format: `"<dir_hash>:<name_hash>:<ext_magic>"`

**Rationale:** BA2 file lookup uses a triplet (directory CRC32, file-name CRC32, 4-byte extension literal). Combining these into a single string key for the `ParsedArchive::lookup` map maintains the same design used by TES3/TES4 parsers. The key is derived from the normalized path at lookup time.

**Alternatives considered:**
- Numeric composite key (e.g., `uint64_t` packing two CRC32s) → Rejected: doesn't encode the 4-char extension cleanly without a custom hash map.

### 5. Version routing within the BA2 GNRL parser

The parser handles all GNRL versions (1, 2, 7, 8) in one function with version-conditional header reads:
- v1/v7/v8: Standard FO4 header (`TwbBSHeaderFO4`).
- v2: SF header with `Unknown1`+`Unknown2` extra fields.
- v3: SF header with `Unknown1`+`Unknown2`+`CompressionMethod` (but v3 implies DX10, so the GNRL parser only encounters v2 for Starfield).

The `CompressionMethod == 3` check sets the decompression strategy to LZ4 block for Starfield.

**Alternatives considered:**
- Separate parser functions per version → Rejected: the file record layout is identical across all GNRL versions; only the header size varies.

### 6. New enum values `ArchiveFormat::fo4` and `ArchiveFormat::starfield`

**Rationale:** Differentiating FO4 from Starfield at the format level matters because the compression method differs. Consumers need this distinction to know which decompressor was used.

### 7. Using LZ4 block API (not frame)

**Rationale:** Starfield BA2 stores raw LZ4-block-compressed data without framing. The lz4 library's `LZ4_decompress_safe()` handles this directly. SSE archives use LZ4 frame (handled by existing LZ4F code). These are different APIs in the lz4 library.

## Risks / Trade-offs

- **[Risk] Unknown BA2 versions from future game patches** → Mitigation: The parser checks version explicitly and returns `unsupported_format` for unknown versions. Adding new versions is a small code change.
- **[Risk] BAADF00D sentinel not validated** → Mitigation: The reference skips this field during read. We'll read and discard it, optionally logging a warning in debug builds if it doesn't match. Not validating keeps compatibility with modified archives.
- **[Risk] File-name table may be absent or corrupt** → Mitigation: If `FileTableOffset` points beyond EOF or the name count doesn't match file count, return `malformed_archive` error. Files remain accessible by index even without names.
- **[Trade-off] Linear scan fallback for lookup** → For now, the lookup map is built at parse time from the name table. If the name table is missing, path-based lookup is unavailable but index-based extraction still works.
