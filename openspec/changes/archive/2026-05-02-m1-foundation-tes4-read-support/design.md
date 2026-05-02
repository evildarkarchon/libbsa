## Context

libbsa currently has a minimal CMake target and vcpkg manifest, but no public headers, implementation sources, test suite, or archive API. Milestone 1 must turn the repository into a real C++20 static library and implement read/extract support for TES4-family BSA archives: Oblivion (`BSA\0` version `0x67`), Fallout 3/New Vegas/Skyrim LE (`0x68`), and Skyrim SE/AE (`0x69`).

The behavioral reference is `TES5Edit/Core/wbBSArchive.pas` and `TES5Edit/Core/wbBSA.pas`. Those files are read-only reference material; implementation and tests must live outside `TES5Edit/`.

## Goals / Non-Goals

**Goals:**

- Build `libbsa` as a reusable C++20 static library with public headers under `include/`, implementation under `src/`, and tests under `tests/`.
- Provide a small public read API for opening an archive, reporting format metadata, listing entries, checking existence, retrieving entry metadata, extracting to memory, and extracting to a caller-provided byte sink or stream.
- Parse TES4-family BSA headers and index tables into immutable in-memory metadata suitable for random-access extraction.
- Match BSArchPro-compatible TES4-family hashing, compression-flag interpretation, embedded-name handling, and byte output.
- Add focused automated tests and fixture-based compatibility tests for the milestone surface.

**Non-Goals:**

- TES3, BA2, write support, DDS reconstruction, data deduplication, parallel extraction, and performance benchmarking.
- A GUI or CLI wrapper.
- Compiling or modifying any source under `TES5Edit/`.
- Exposing libdeflate, lz4, or test-framework headers through the public libbsa API.

## Decisions

### Static Library Layout

Build a compiled static library target rather than continuing with an interface-only target.

- Rationale: milestone 1 includes binary parsing, decompression, and file I/O implementation that belongs in `.cpp` files and should not leak dependency headers to consumers.
- Alternative considered: keep the library header-only. This would make compression and parsing internals part of the public build surface and would work against the PRD's minimal-header goal.

### Public API Shape

Expose a narrow `libbsa` namespace with value-type metadata records and an owning archive reader object opened from a filesystem path.

- Rationale: callers need reusable library primitives, not BSArchPro object structure. The reader can own the file path, parsed index, and any lightweight configuration while entries remain cheap value records.
- Error handling should use a C++20-compatible result/error surface instead of `std::expected`, because `std::expected` is not standard until C++23. A small `libbsa::Result<T>` plus `libbsa::Error` enum keeps failure modes explicit without requiring a new dependency.
- Alternative considered: throw exceptions for all parse and I/O failures. That is simpler but conflicts with the PRD's non-throwing fast path for ordinary archive and filesystem errors.

### Internal Parsing Model

Use explicit little-endian read helpers and internal record structs instead of exposing packed structs in public headers.

- Rationale: BSA fields are little-endian binary records, but public ABI should not depend on compiler packing, padding, or platform alignment. Guarded reads also give better malformed/truncated archive errors.
- Alternative considered: map binary records directly onto packed C++ structs. This is shorter but more fragile and harder to validate safely.

### TES4-Family Detection

Detection is based on magic bytes plus version:

- `BSA\0` + `0x67` -> Oblivion/TES4 BSA
- `BSA\0` + `0x68` -> Fallout 3/New Vegas/Skyrim LE BSA
- `BSA\0` + `0x69` -> Skyrim SE/AE BSA

Unknown magic or unsupported versions return a typed open error. BA2 and TES3 signatures are recognized only enough to reject them as unsupported by this milestone.

### Index and Lookup

Parse the TES4 header, folder records, folder names, file records, and file name table into internal folder and entry arrays. SSE uses the version-specific folder record shape with an extra `uint32` and 64-bit offset; TES4/FO3 use 32-bit folder offsets.

Lookup is hash-based using the BSArchPro-compatible `CreateHashTES4` algorithm for folder names and file name/extension pairs. Public lookup should normalize incoming paths to backslashes and case-insensitive ASCII before hashing so callers can use portable path spelling while the archive data still remains byte-compatible.

### Extraction and Compression

Interpret compression through the TES4-family XOR rule:

- Archive flag `ARCHIVE_COMPRESS` (`0x0004`) defines the archive default.
- File size high bit `FILE_SIZE_COMPRESS` (`0x40000000`) inverts that default for a file.
- Compressed TES4/FO3 entries use deflate through libdeflate.
- Compressed SSE entries use LZ4 frame decompression through the official lz4 library.
- Compressed entries store the uncompressed size before the compressed payload.

The implementation should keep compression adapters internal so public headers do not include libdeflate or lz4 headers.

### Embedded Names

For FO3/SSE archives with `ARCHIVE_EMBEDNAME` (`0x0100`), extraction must skip the length-prefixed embedded name before reading the uncompressed-size prefix or raw payload. The reference only applies this skip to FO3/SSE, so the milestone should preserve that behavior rather than applying it blindly to every version.

### Tests and Fixtures

Use Catch2 as the milestone test framework via vcpkg and CTest.

- Rationale: Catch2 is small, CMake-friendly, and well suited to binary parser and fixture assertions.
- Alternative considered: GoogleTest. It is also viable, but Catch2 needs less scaffolding for this initial library.

Tests should include:

- Unit tests for TES4-family hash vectors and path normalization.
- Binary parser tests for minimal hand-crafted TES4, FO3, and SSE archives.
- Extraction tests for raw, deflate-compressed, LZ4-frame-compressed, and embedded-name entries.
- Fixture compatibility tests that compare extracted bytes to known expected payloads or checked-in BSArchPro output.

Fixtures must be checked in or generated by test helpers outside `TES5Edit/`; the submodule is never a mutable fixture location.

## Risks / Trade-offs

- [Risk] Hash-only lookup can collide in theory. -> Mitigation: preserve hash-based lookup for compatibility but keep stored path metadata available for diagnostics and future collision handling.
- [Risk] Deflate payload flavor may differ between official archives and library defaults. -> Mitigation: verify decompression against fixture archives and keep the libdeflate adapter isolated so zlib-wrapper handling can be adjusted without API churn.
- [Risk] LZ4 frame and LZ4 block can be confused. -> Mitigation: milestone 1 only uses LZ4 frame for SSE BSA `0x69`; BA2 LZ4 block support remains out of scope.
- [Risk] Public API design may need expansion for write support. -> Mitigation: keep read metadata value-based and avoid exposing parser internals so later writer types can be added alongside the reader.
- [Risk] Real game archives are large or redistributable only under game licenses. -> Mitigation: prefer tiny synthetic fixtures plus optional local compatibility tests that can run against user-provided archives.

## Migration Plan

1. Convert the existing CMake target from interface-only to a static library and add install/export-friendly include layout.
2. Add internal parsing, hashing, decompression, and reader modules behind public API headers.
3. Add Catch2/CTest wiring and milestone fixture tests.
4. Keep README build instructions current for vcpkg configure, build, and test commands.

Rollback is straightforward while this is the first implementation milestone: remove the new source/test directories and restore the prior interface-only CMake target if the approach proves wrong before archive APIs are consumed.

## Open Questions

- Exact checked-in fixture corpus is still to be chosen during implementation. The tests should start with generated minimal archives and add real compatibility fixtures only when licensing and size are acceptable.
- The first public result/error type should stay minimal, but implementation may reveal whether filesystem and parse errors need separate categories before milestone 1 exits.
