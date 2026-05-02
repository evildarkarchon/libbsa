## 1. Build Foundation

- [x] 1.1 Create `include/`, `src/`, and `tests/` directories with a libbsa namespace layout.
- [x] 1.2 Convert the root CMake target from interface-only to a C++20 static library with the `libbsa::libbsa` alias.
- [x] 1.3 Add Catch2 as a test-only vcpkg dependency and wire CTest-discoverable test targets.
- [x] 1.4 Keep libdeflate and lz4 linked privately through implementation sources, not public headers.
- [x] 1.5 Update README build and test instructions for configure, build, and test commands.

## 2. Public Read API

- [x] 2.1 Add public Doxygen-documented archive format, metadata, entry, error, and result types.
- [x] 2.2 Add an owning archive reader API for opening a path, reporting archive metadata, listing entries, and checking file existence.
- [x] 2.3 Add metadata lookup, extraction-to-memory, and extraction-to-caller-sink APIs.
- [x] 2.4 Ensure ordinary I/O, unsupported-format, malformed-archive, missing-file, and decompression failures return typed errors.
- [x] 2.5 Add a public-header compile test proving consumers do not include compression, Catch2, UI, or TES5Edit headers.

## 3. TES4-Family Parsing and Lookup

- [x] 3.1 Implement guarded little-endian binary read helpers and internal TES4-family record models.
- [x] 3.2 Implement magic/version detection for `BSA\0` versions `0x67`, `0x68`, and `0x69`, with typed rejection for unsupported formats.
- [x] 3.3 Parse TES4/FO3 folder records, names, file records, file names, archive flags, file flags, and data offsets.
- [x] 3.4 Parse SSE folder records with the extra unknown 32-bit field and 64-bit folder offsets.
- [x] 3.5 Implement BSArchPro-compatible `CreateHashTES4` folder and file hashing, including special extension bits.
- [x] 3.6 Implement archive-relative path normalization and hash-based file lookup for existence and metadata queries.

## 4. Extraction

- [x] 4.1 Implement TES4-family compression-state resolution using `ARCHIVE_COMPRESS` XOR `FILE_SIZE_COMPRESS`.
- [x] 4.2 Implement uncompressed entry extraction with exact stored-byte output.
- [x] 4.3 Implement TES4/FO3 deflate extraction using libdeflate and the stored uncompressed-size prefix.
- [x] 4.4 Implement SSE LZ4-frame extraction using the official lz4 library and the stored uncompressed-size prefix.
- [x] 4.5 Implement FO3/SSE `ARCHIVE_EMBEDNAME` prefix skipping before raw or compressed payload reads.
- [x] 4.6 Validate extraction bounds and malformed/truncated archive cases return typed errors.

## 5. Tests and Validation

- [x] 5.1 Add hash-vector tests for folder names, regular file names, and `.kf`, `.nif`, `.dds`, and `.wav` extension cases.
- [x] 5.2 Add minimal generated or checked-in TES4, FO3-family, and SSE fixture archives outside `TES5Edit/`.
- [x] 5.3 Add parser tests proving archive metadata, file listing, folder offsets, flags, and file records match fixture expectations.
- [x] 5.4 Add lookup tests for case differences, slash normalization, successful hits, and missing-file errors.
- [x] 5.5 Add extraction tests for raw, deflate-compressed, LZ4-frame-compressed, and embedded-name entries.
- [x] 5.6 Run the repo configure, build, and CTest validation commands and record any fixture limitations.
