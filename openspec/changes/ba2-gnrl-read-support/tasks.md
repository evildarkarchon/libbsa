## 1. Public API Extensions

- [ ] 1.1 Add `ArchiveFormat::fo4` and `ArchiveFormat::starfield` to enum in `include/libbsa/archive.hpp`
- [ ] 1.2 Add `CompressionMethod::lz4_block` to enum in `include/libbsa/archive.hpp`

## 2. BA2 CRC32 Hash Algorithm

- [ ] 2.1 Create `src/ba2_hash.hpp` declaring `create_hash_fo4(std::string_view)` returning `uint32_t`
- [ ] 2.2 Implement `src/ba2_hash.cpp` with precomputed CRC32 table and per-char normalization (lowercase, `/` to `\`, skip bytes > 127)
- [ ] 2.3 Add unit tests verifying CRC32 hash output matches known BA2 fixture hashes

## 3. BA2 GNRL Archive Parser

- [ ] 3.1 Create `src/ba2_gnrl_archive.hpp` declaring `parse_ba2_gnrl_archive(path)` and `extract_ba2_gnrl_entry(...)` in `detail` namespace
- [ ] 3.2 Implement BTDX outer header parsing (magic + version) with version validation (1, 2, 7, 8)
- [ ] 3.3 Implement FO4 sub-header parsing (type magic `GNRL`, file count, file table offset)
- [ ] 3.4 Implement Starfield v2 extra header fields (`Unknown1`, `Unknown2`) and v3 `CompressionMethod` detection
- [ ] 3.5 Implement GNRL file record parsing loop (name hash, ext, dir hash, unknown, offset, packed size, size, BAADF00D sentinel)
- [ ] 3.6 Implement file-name table parsing at `FileTableOffset` (16-bit length-prefixed strings, normalize separators)
- [ ] 3.7 Build lookup map from parsed names using key format `"<dir_hash>:<name_hash>:<ext_magic>"`
- [ ] 3.8 Return `ParsedArchive` with metadata, entries, lookup map, and source path

## 4. BA2 GNRL Extraction

- [ ] 4.1 Implement `extract_ba2_gnrl_entry` — raw extraction when `PackedSize == 0`
- [ ] 4.2 Implement deflate decompression path for FO4 archives (libdeflate, `PackedSize != 0`)
- [ ] 4.3 Implement LZ4 block decompression path for Starfield archives (`CompressionMethod == 3`)
- [ ] 4.4 Add decompression size validation (return `decompression_failed` on mismatch)

## 5. Archive Reader Integration

- [ ] 5.1 Add `kMagicBtdx` constant to `src/archive_reader.cpp`
- [ ] 5.2 Route BTDX magic to `detail::parse_ba2_gnrl_archive(path)` in `parse_archive()`
- [ ] 5.3 Add BA2-specific lookup key logic in `lookup_key_for()` (split dir/name/ext, compute hash triplet)
- [ ] 5.4 Route BA2 format to `detail::extract_ba2_gnrl_entry()` in `ArchiveReader::extract()`

## 6. Tests

- [ ] 6.1 Create or acquire small FO4 GNRL BA2 fixture archive with known file contents
- [ ] 6.2 Create or acquire small Starfield GNRL BA2 fixture archive (LZ4 block compressed)
- [ ] 6.3 Add test: open FO4 GNRL BA2, verify metadata (format, version, file count)
- [ ] 6.4 Add test: list all entries from FO4 fixture, verify paths match expected
- [ ] 6.5 Add test: extract each file from FO4 fixture, verify byte-identical to expected content
- [ ] 6.6 Add test: open Starfield GNRL BA2, verify format is `starfield`
- [ ] 6.7 Add test: extract LZ4-block-compressed entry from Starfield fixture, verify bytes
- [ ] 6.8 Add test: path lookup with different case/slash style finds expected entry
- [ ] 6.9 Add test: lookup of non-existent path returns `missing_file` error
- [ ] 6.10 Add test: opening a DX10 BA2 returns `unsupported_format`

## 7. Build Integration

- [ ] 7.1 Add `ba2_hash.cpp` and `ba2_gnrl_archive.cpp` to CMakeLists.txt source list
- [ ] 7.2 Verify clean build with no warnings on MSVC
