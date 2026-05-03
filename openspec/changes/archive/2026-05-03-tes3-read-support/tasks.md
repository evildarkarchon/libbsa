## 1. Public API Extension

- [x] 1.1 Add `tes3` variant to `ArchiveFormat` enum in `include/libbsa/archive.hpp`
- [x] 1.2 Update `ArchiveReader::open()` dispatch to check for TES3 magic (`0x00000100`) before TES4 detection

## 2. TES3 Hash Algorithm

- [x] 2.1 Create `src/tes3_hash.hpp` with `hash_tes3(std::string_view filename) -> uint64_t` declaration
- [x] 2.2 Implement `hash_tes3` in `src/tes3_hash.cpp`: split string at `floor(len/2)`, XOR-accumulate first half into high dword, XOR-accumulate with rotation second half into low dword, ASCII-only lowercasing
- [x] 2.3 Add TES3 hash unit tests verifying output matches known Morrowind BSA hash table entries

## 3. TES3 Archive Parser

- [x] 3.1 Create `src/tes3_archive.hpp` declaring `parse_tes3_archive()` and `extract_tes3_entry()` in `libbsa::detail`
- [x] 3.2 Implement TES3 header parsing in `src/tes3_archive.cpp`: read magic, HashOffset, FileCount from 12-byte header
- [x] 3.3 Implement size/offset table parsing: read `FileCount` pairs of `(uint32 size, uint32 offset)`
- [x] 3.4 Implement name offset table skip: advance past `FileCount * 4` bytes
- [x] 3.5 Implement filename record parsing: read `FileCount` null-terminated strings
- [x] 3.6 Implement hash table parsing: read `FileCount` entries as two uint32 (high dword, low dword) combined into uint64
- [x] 3.7 Compute and store `data_section_offset` as the position immediately after the hash table
- [x] 3.8 Build `ArchiveEntry` vector and lookup map from parsed records (hash hex string as key)
- [x] 3.9 Return populated `ParsedArchive` with format `tes3`, appropriate metadata fields

## 4. TES3 Extraction

- [x] 4.1 Implement `extract_tes3_entry()`: seek to `data_section_offset + entry.offset`, read `entry.size` raw bytes
- [x] 4.2 Wire extraction into `archive_reader.cpp` for TES3 format entries (no decompression needed)

## 5. Integration and Dispatch

- [x] 5.1 Update `archive_reader.cpp` to call `parse_tes3_archive()` when TES3 magic is detected
- [x] 5.2 Add `src/tes3_archive.cpp` and `src/tes3_hash.cpp` to CMakeLists.txt

## 6. Test Fixtures and Validation

- [x] 6.1 Create or acquire a small TES3 BSA test fixture with known file contents
- [x] 6.2 Add integration test: open TES3 fixture, verify file count and entry paths match expected
- [x] 6.3 Add integration test: extract each file from TES3 fixture, verify byte-identical to expected content
- [x] 6.4 Add test: verify TES3 archive is not misidentified as TES4 and vice versa
- [x] 6.5 Add test: verify path lookup with different case and slash styles resolves correctly
