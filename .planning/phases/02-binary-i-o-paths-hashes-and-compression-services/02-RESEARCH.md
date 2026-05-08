# Phase 02 Research: Binary I/O, Paths, Hashes, and Compression Services

**Status:** Complete  
**Phase requirement IDs:** BIN-01, BIN-02, BIN-03, BIN-04, BIN-05, BIN-06, BIN-07, BIN-08

## Research Complete

Phase 2 should add an internal primitive layer under `src/detail/` and keep installed public headers unchanged except for any unavoidable `error_code` additions. This honors D-01 through D-04: binary I/O, path, streaming, hash, and codec services remain private implementation details, and `archive_reader::open(std::string_view)` stays unchanged.

## Standard Stack

- C++20 and existing `libbsa::result<T>` / `libbsa::error` for all fallible primitives.
- Catch2/CTest with existing `[unit]` and `[malformed]` tags.
- `libdeflate` via private target linkage for raw deflate chunks.
- Official `lz4` via private target linkage for both `LZ4F_*` frame APIs and `LZ4_*safe*` block APIs.
- No new dependencies and no public dependency-bearing headers.

## Architecture Patterns

- Internal files should live under `src/detail/`, e.g. `binary_io.hpp/.cpp`, `archive_path.hpp/.cpp`, `payload_stream.hpp/.cpp`, `deflate_codec.hpp/.cpp`, `lz4_frame_codec.hpp/.cpp`, `lz4_block_codec.hpp/.cpp`, `compression_router.hpp/.cpp`, and `bethesda_hash.hpp/.cpp`.
- Tests should live under `tests/unit/` and be registered in the existing `libbsa_tests` executable in `tests/CMakeLists.txt`.
- Public header boundary tests should continue grepping installed/public headers only. Private sources may include `libdeflate.h`, `lz4.h`, and `lz4frame.h`.
- Use `libbsa::error_code::format_error` for malformed archive bytes/compressed data and `invalid_argument` for invalid caller inputs such as rejected archive virtual paths.

## Library Findings

### libdeflate

Use raw deflate APIs, not zlib/gzip wrappers. The C API shape maps cleanly to exact-size archive chunks: allocate a decompressor, call `libdeflate_deflate_decompress(...)` into a caller-sized output buffer, check the return code, and then require `actual_out == expected_size`. Compression should use a compressor and a bound-sized output buffer, trimming to the returned byte count.

### LZ4 frame

Skyrim SE/AE BSA LZ4 payloads must use frame APIs. `LZ4F_compressFrameBound` and `LZ4F_compressFrame` support one-shot frame compression for unit vectors. `LZ4F_decompress` returns frame-progress state and must be checked with `LZ4F_isError`; the adapter should fail unless the produced output count is exactly the expected archive metadata size and the frame is fully consumed.

### Raw LZ4 block

Starfield BA2 v3 raw LZ4 block payloads must use raw block APIs, not frame APIs. `LZ4_compressBound`, `LZ4_compress_default`, and `LZ4_decompress_safe` provide the required one-shot block path. `LZ4_decompress_safe` returns a negative value for malformed data and a byte count for success; the adapter must require that count equals the expected size.

## TES5Edit Hash Trace

`TES5Edit/Core/wbBSArchive.pas` provides the behavior to port without modifying the submodule:

- `LowerByte` (lines 646-654) folds only ASCII `A` through `Z` to lowercase.
- `CreateHashTES3` (lines 705-731) splits the byte string in half, XOR/rotates lower bytes, and combines two 32-bit halves into a 64-bit hash.
- `CreateHashTES4` (lines 733-784) combines filename length/edge bytes, extension special bits (`.kf`, `.nif`, `.dds`, `.wav`), and sdbm-like high-half accumulation for name interior plus extension.
- `CreateHashFO4` (lines 786-800) uses a CRC32 table with initial value `0`, ASCII lowercase folding, skips bytes over 127, and treats `/` as `\\` before hashing.

Reference constants for tests:

| Input | TES3 | TES4 | FO4/BA2 |
|-------|------|------|---------|
| `meshes/foo/bar.nif` | `0x0E5C1667258ACAD8` | `0x8A690D8F6D0EE172` | `0x89046777` |
| `textures/actors/armor.dds` | `0x737E762E920BB2D6` | `0xC9B25E337415EFF2` | `0xC047BD9D` |
| `sound/fx/test.wav` | `0x16134017C1A51758` | `0xBA13EDF9F30D7374` | `0x6819A08B` |
| `bookart/sample.txt` | `0x441B1D707AD1ED71` | `0xF68736EA620E6C65` | `0x9E143B2B` |

Additional TES4 split constants: folder `meshes/foo` => `0xA0E6BEDF6D0A6F6F`; file `bar.nif` => `0x92CD45FD6203E172`.

## Common Pitfalls

- Do not use `std::filesystem::path` for archive virtual paths; archive keys are lowercased forward-slash byte strings.
- Do not route compression by file extension.
- Do not decode raw LZ4 blocks with `LZ4F_*` or frames with `LZ4_decompress_safe`.
- Do not compare diagnostic strings exactly in tests; assert stable error codes.
- Do not add public codec/path/hash headers in Phase 2.
- Do not use `TES5Edit/` as a fixture workspace or commit anything under it.

## Validation Architecture

Phase 2 validation is entirely automated through unit and malformed tests. Each plan must add failing tests before implementation because TDD mode is active. The quick command is:

`cmake --build --preset windows-msvc-debug-static --config Debug && ctest --preset windows-msvc-debug-static -L unit --output-on-failure`

Plan-specific test filters should use `ctest --preset windows-msvc-debug-static -R "(binary_io|archive_path|payload_stream|deflate|lz4|compression_router|bethesda_hash|public_include_boundary)" --output-on-failure` after the build step.

## Source Audit Coverage

- GOAL: covered by plans 01-05.
- REQ: BIN-01/02 in plan 01, BIN-03 in plan 02, BIN-04/07 in plan 03, BIN-05/06/07 in plan 04, BIN-08 in plan 05.
- RESEARCH: library API separation, exact-size validation, and TES5Edit hash constants are covered by plans 03-05.
- CONTEXT: D-01 through D-20 are distributed across plans 01-05 and cited in task actions.
