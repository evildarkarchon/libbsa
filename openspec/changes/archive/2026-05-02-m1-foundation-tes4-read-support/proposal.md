## Why

Milestone 1 establishes libbsa as a buildable, testable C++20 library and delivers the first usable archive-reading surface. TES4-family BSA support is the right foundation because it covers Oblivion through Skyrim SE/AE and exercises detection, indexing, hashing, path lookup, and compression without the BA2 and TES3-specific branches.

## What Changes

- Add the repository's initial reusable C++ library structure with public headers, implementation sources, tests, and a CMake static-library target.
- Define the first public read API for opening archives, inspecting metadata, listing entries, resolving paths, and extracting file bytes.
- Implement TES4-family BSA detection for `BSA\0` versions 103, 104, and 105.
- Parse TES4/FO3/SSE folder and file index tables, including archive flags, file flags, folder records, file records, and name tables.
- Implement `CreateHashTES4`-compatible folder and file hashing for hash-based lookup.
- Extract raw and compressed entries, using libdeflate for TES4/FO3 deflate data and LZ4 frame decompression for SSE data.
- Handle embedded file names controlled by the `ARCHIVE_EMBEDNAME` flag.
- Add focused unit and fixture-based integration tests proving parsing, lookup, hash behavior, decompression, and byte-identical extraction against known TES4-family archives.
- Keep `TES5Edit/` read-only as a behavioral reference; no source is compiled from or written into the submodule.

## Capabilities

### New Capabilities

- `library-build-foundation`: Build, package, and test libbsa as a reusable C++20 static library with repo-native dependencies.
- `tes4-family-bsa-read`: Detect, inspect, look up, and extract files from TES4-family BSA archives.

### Modified Capabilities

- None.

## Impact

- Affects root CMake configuration, vcpkg manifest usage, and new `include/`, `src/`, and `tests/` project directories.
- Introduces public C++ API headers for archive opening, metadata inspection, file listing, lookup, and extraction.
- Introduces internal binary parsing, hashing, decompression, path normalization, and error-reporting code.
- Adds test fixtures or fixture-generation helpers outside `TES5Edit/`, plus focused automated tests for milestone 1 behavior.
- Uses existing required dependencies `libdeflate` and `lz4`; selects and documents a C++ test framework for milestone 1.
