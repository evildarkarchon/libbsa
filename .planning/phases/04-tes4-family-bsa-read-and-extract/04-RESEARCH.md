# Phase 04 Research: TES4-Family BSA Read and Extract

**Phase:** 04 — TES4-Family BSA Read and Extract  
**Requirements:** BSA-01, BSA-02, BSA-03, BSA-05  
**Status:** Complete

## Research Question

What does the planner need to know to create executable prompts for opening, listing, inspecting, looking up, and extracting Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 BSA archives using the existing libbsa C++20 API foundation?

## Source Artifacts Consulted

- `TES5Edit/Core/wbBSArchive.pas` lines 242-284 — TES3/TES4 record structures.
- `TES5Edit/Core/wbBSArchive.pas` lines 802-819 — TES4 compression flag inversion behavior.
- `TES5Edit/Core/wbBSArchive.pas` lines 1201-1238 — TES4-family folder/file/name table read order.
- `TES5Edit/Core/wbBSArchive.pas` lines 1790-1814 — codec routing and Fallout vanilla zlib buffer-error compatibility note.
- `TES5Edit/Core/wbBSArchive.pas` lines 2094-2149 — TES4-family extraction, embedded-name skipping, compressed payload prefix handling.
- Existing libbsa surfaces: `include/libbsa/io.hpp`, `archive.hpp`, `archive_view.hpp`, `compression.hpp`, `detect.hpp`, `result.hpp`.
- Existing tests: `tests/detection_tests.cpp`, `tests/archive_view_tests.cpp`, `tests/compression_policy_tests.cpp`, `tests/public_header_smoke.cpp`.

## Standard Stack

- C++20 public API with `libbsa::result<T>`; do not expose `std::expected`.
- Existing `byte_source` / `byte_sink` abstractions are the I/O boundary.
- Existing `archive_summary`, `entry_metadata`, `archive_view`, and compression dispatcher should be reused.
- New implementation belongs under `include/libbsa/`, `src/`, and `tests/`; `TES5Edit/` remains read-only.

## Architecture Patterns

1. **Public header boundary:** Add a narrow `include/libbsa/bsa.hpp` surface for TES4-family BSA open/list/lookup/extract operations. It should expose libbsa-owned types only.
2. **Internal parser boundary:** Put binary table parsing helpers in `src/bsa_reader.*` or equivalent private source files, keeping constants and table-record structs private.
3. **Metadata view reuse:** Back `bsa_archive::paths()`, `contains()`, and `entry()` with existing `archive_view` semantics so path normalization and deterministic sorted paths remain consistent.
4. **Explicit compression route:** Use `resolve_payload_codec` and `decompress_payload`; do not call libdeflate or LZ4 implementation wrappers directly from BSA extraction logic.
5. **Fixture-backed tests:** Use small synthetic in-memory BSA byte builders for v103/v104/v105, compression flag inversion, embedded names, malformed offsets, and extraction payload behavior. This keeps tests committed and independent of mutable TES5Edit fixtures.

## Format Constraints to Preserve

- Magic is `BSA\0` as little-endian `0x00415342`; versions are `0x67` (Oblivion/TES4), `0x68` (FO3/FNV/Skyrim LE), and `0x69` (Skyrim SE/AE).
- TES4-family header after magic/version contains: `FoldersOffset`, `Flags`, `FolderCount`, `FileCount`, `FolderNamesLength`, `FileNamesLength`, `FileFlags`.
- Folder records use `hash:u64`, `file_count:u32`, and offset. v105/SSE includes an extra `unk32:u32` and stores offset as `u64`; v103/v104 store offset as `u32`.
- Table parsing order in BSArchPro reads all folder records from `FoldersOffset`, then reads each folder name followed by that folder's file records, then reads the global null-terminated file-name table.
- Archive compression flag `ARCHIVE_COMPRESS = 0x0004` is inverted by per-file `FILE_SIZE_COMPRESS = 0x40000000`; actual compressed status is `(archive_flags & 0x0004) XOR (record_size & 0x40000000)`.
- Logical record size is `record_size & ~0x40000000`.
- For FO3/FNV/Skyrim LE v104 and SSE v105 only, when `ARCHIVE_EMBEDNAME = 0x0100`, payload bytes begin with a one-byte length followed by the embedded filename. Extraction must skip that prefix before handling uncompressed-size prefix or raw payload.
- Compressed TES4-family payloads store a little-endian `u32` uncompressed size before the compressed byte stream after any embedded-name prefix.
- v103/v104 compressed payloads route to deflate; v105 routes to LZ4 frame.

## Do Not Hand-Roll / Avoid

- Do not introduce a filesystem source, disk extractor, CLI, or safe-disk extraction policy in this phase; those are separate concerns.
- Do not use `std::filesystem::path` for archive paths.
- Do not expose `libdeflate`, `lz4`, `DirectXTex`, Windows, or TES5Edit types in public headers.
- Do not modify, format, stage, or compile anything under `TES5Edit/`.
- Do not infer compression from file extension.

## Common Pitfalls

- Treating `FILE_SIZE_COMPRESS` as direct compression state instead of XORing it with the archive default compression flag.
- Skipping embedded names for v103; BSArchPro only skips embedded names for FO3/SSE family variants even if Oblivion writer defaults include the flag.
- Forgetting to subtract embedded-name and uncompressed-size prefix bytes from packed payload length before decompression.
- Using folder-record offsets inconsistently with the actual table traversal used by the reference; tests should lock both table layout and malformed-offset handling.
- Accepting impossible offsets/sizes that run past `byte_source::size()`.

## Validation Architecture

### Dimensions

1. **Header/table parsing:** v103, v104, and v105 synthetic archives open successfully with correct format, flags, folder count, file count, normalized paths, hashes, offsets, packed sizes, and compression states.
2. **Compression resolution:** Deflate and LZ4 frame extraction are selected by archive version and actual entry compression state; raw entries bypass codecs with exact-size validation.
3. **Embedded filenames:** v104/v105 embedded-name entries skip the length-prefixed name and preserve payload bytes after the prefix.
4. **Malformed input safety:** Unsupported versions, missing names, truncated folder records, impossible offsets, and payload ranges past source size return structured `malformed_archive` / `unsupported_format` failures without crashes.
5. **Boundary gates:** Public headers remain dependency-clean; CMake uses explicit source lists; `git status --short TES5Edit` remains empty.

### Required Commands

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L fixture`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke`
- `rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex|TES5Edit" include/libbsa`
- `rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt`
- `git status --short TES5Edit`

## Architectural Responsibility Map

| Tier | Responsibilities |
|------|------------------|
| Public API | `include/libbsa/bsa.hpp` owns consumer-facing BSA open/list/lookup/extract declarations and Doxygen comments. |
| Parser implementation | `src/bsa_reader.cpp` owns little-endian parsing, bounds checks, table layout, compression-state derivation, and extraction. |
| Existing services | `archive_path`, `archive_view`, `byte_source`, `byte_sink`, and `compression` are reused, not reimplemented. |
| Tests | `tests/bsa_reader_tests.cpp` owns synthetic fixture builders, TDD behavior cases, malformed archives, and extraction verification. |
| Documentation | `README.md` describes Phase 04 BSA read/extract behavior and validation commands after implementation. |

## RESEARCH COMPLETE
