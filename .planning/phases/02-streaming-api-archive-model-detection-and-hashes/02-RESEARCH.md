# Phase 02 Research: Streaming API, Archive Model, Detection, and Hashes

**Status:** Complete
**Date:** 2026-05-05
**Question answered:** What does the planner need to know to plan Phase 02 well?

## Summary

Phase 02 should establish libbsa-owned contracts and compatibility primitives without implementing real archive table parsers or payload extraction. The safest decomposition is:

1. Public bounded random-access source and streaming sink contracts, with memory helpers for tests and small payloads.
2. Public archive identity/metadata/detection types and bounded header detection.
3. Public archive-path normalization plus internal/test-visible Bethesda hash primitives with golden vectors.
4. Metadata-only archive view lookup built from copied metadata.
5. Boundary smoke/docs gates proving public headers remain dependency-free and `TES5Edit/` remains read-only.

## Existing Patterns to Reuse

- Public headers live under `include/libbsa/`; implementation files live under `src/`.
- `CMakeLists.txt` uses explicit `target_sources` and explicit test executable registration; do not introduce recursive source globbing.
- Public failures use `libbsa::result<T>`, `libbsa::error`, and `libbsa::error_code` from `include/libbsa/result.hpp`.
- Catch2 tests live under `tests/` and are run through CTest labels. Phase 02 should add focused `unit` tests and a `golden` label for path/hash vectors.
- Verification on this machine has used `build/local-vs2026-vcpkg` with Visual Studio 18 2026 because the committed Visual Studio 17 2022 preset is not available locally; do not change the committed preset only for this environment.

## Reference Findings

Reference source: `TES5Edit/Core/wbBSArchive.pas` (read-only; do not edit, format, stage, compile, or vendor).

### Archive Identity and Header Markers

- Archive type enum in the reference distinguishes `baTES3`, `baTES4`, `baFO3`, `baSSE`, `baFO4`, `baFO4dds`, `baSF`, and `baSFdds`.
- Magic constants:
  - TES3 BSA: bytes `00 01 00 00` (`MAGIC_TES3`).
  - TES4-family BSA: `BSA\0` (`MAGIC_BSA`).
  - BA2 container: `BTDX` (`MAGIC_BTDX`).
  - BA2 subtype: `GNRL` or `DX10` from explicit header subtype fields.
- Header versions:
  - TES4/Oblivion: `0x67`.
  - FO3/FNV/Skyrim LE: `0x68`.
  - Skyrim SE/AE: `0x69`.
  - FO4 BA2: `0x01`, `0x07`, `0x08`.
  - Starfield BA2: `0x02`, `0x03`.
- Starfield BA2 v3 has an explicit `CompressionMethod` field; value `3` maps to raw LZ4 block routing in later codec phases. Phase 02 should record this marker in metadata, not implement decompression.

### Header Field Shapes Useful for Detection Summaries

- TES3 header after magic contains `HashOffset` and `FileCount`.
- TES4-family BSA header after magic/version contains folder offset, flags, folder count, file count, folder names length, file names length, and file flags.
- BA2 header after magic/version contains subtype magic, file count, and `FileTableOffset`; Starfield v2/v3 append extra fields, and v3 appends `CompressionMethod`.
- Detection must be bounded and strict: a recognized magic with too few bytes for required fields is malformed, while recognized magic with an unsupported version is unsupported.

### Path and Hash Behavior

Reference source functions are `CreateHashTES3`, `CreateHashTES4`, `CreateHashFO4`, `SplitDirName`, `SplitNameExt`, `LowerByte`, and `String2Magic` in `TES5Edit/Core/wbBSArchive.pas`.

- `LowerByte` lowercases ASCII `A` through `Z` only.
- TES3 hash uses the whole archive filename string, split into two halves, with byte-lowercasing and bit shifts/rotations.
- TES4-family lookup splits directory and filename, hashes the directory with `CreateHashTES4(fdir, '')`, then splits the file name and extension and hashes `CreateHashTES4(name, ext)`.
- TES4 extension special cases set low-bit flags for `.kf`, `.nif`, `.dds`, and `.wav`.
- FO4/BA2 lookup splits directory/name/extension, hashes directory and basename separately using a CRC32 table and ASCII lowercasing, maps `/` to `\`, and stores the extension as a lowercased four-byte magic value without the dot.
- Phase 02 should keep exact hash functions internal or test-visible and lock behavior through committed tests with inline provenance comments near the vector groups.

## Recommended File Layout

| Purpose | Files |
|---------|-------|
| Streaming source/sink contracts | `include/libbsa/io.hpp`, `src/io.cpp`, `tests/io_tests.cpp` |
| Archive metadata and detection | `include/libbsa/archive.hpp`, `include/libbsa/detect.hpp`, `src/detect.cpp`, `tests/detection_tests.cpp` |
| Path normalization and hashes | `include/libbsa/archive_path.hpp`, `src/archive_path.cpp`, `src/hash.hpp`, `src/hash.cpp`, `tests/path_hash_tests.cpp` |
| Metadata-only lookup | `include/libbsa/archive_view.hpp`, `src/archive_view.cpp`, `tests/archive_view_tests.cpp` |
| Boundary docs/smoke | `tests/public_header_smoke.cpp`, `README.md`, `CMakeLists.txt` |

## Architecture Patterns

- Source and sink contracts should accept/return `std::span<std::byte>` where possible and report structured failures via `result`.
- Public archive paths should own normalized UTF-8 strings and should not be represented as `std::filesystem::path`.
- Archive views should own copied `entry_metadata` values and normalized path keys. They should not retain source/sink references.
- Detection should expose a summary object that reports safely read header fields; it must not claim full table parsing.
- Detection and lookup should never infer BA2 subtype from filenames or host path extensions.

## Common Pitfalls

- Treating a recognized magic with unsupported version as `unknown` loses important diagnostics; return `unsupported_format`.
- Returning partial identity for truncated headers violates the locked detection policy; return `malformed_archive`.
- Exposing hash functions publicly before the API is proven creates unnecessary compatibility surface; keep them internal/test-visible in this phase.
- Using `std::filesystem::path` for archive paths will import host path semantics into virtual archive paths.
- Adding a public file source would prematurely choose host I/O policy; Phase 02 should only add memory helpers.

## Validation Architecture

- Test framework: Catch2 through CTest.
- Quick command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit`.
- Full command: `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`.
- Golden vector command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L golden`.
- Boundary command: public-header forbidden token grep excluding comments/prose where appropriate, plus `git status --short TES5Edit` must be empty.

## Source Coverage Notes

- GOAL: public streaming, metadata, detection, path, hash, and lookup foundation — covered by the five recommended slices.
- REQ: `BIO-01` through `BIO-05` and `DPH-01` through `DPH-05` — covered by source/sink, metadata/detection, path/hash, view lookup, and boundary gates.
- CONTEXT: `D-01` through `D-16` — directly mapped to the same slices.
- Deferred ideas: none.

## Research Complete

Use the findings above to create executable, TDD-oriented plans for Phase 02.
