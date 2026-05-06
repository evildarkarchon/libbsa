# Phase 03 Research: Compression Services and Policy

**Status:** Complete
**Date:** 2026-05-05
**Question answered:** What does the planner need to know to plan Phase 03 well?

## Summary

Phase 03 should add codec wrappers and explicit routing before archive-family readers depend on them. The implementation should keep third-party headers out of public libbsa headers while giving later BSA/BA2 readers and writers one libbsa-owned compression surface.

Recommended slices:

1. Define public compression policy/routing contracts and a central dispatcher skeleton.
2. Implement exact-size raw deflate compression/decompression through libdeflate.
3. Implement LZ4 frame handling for Skyrim SE/AE BSA and raw LZ4 block handling for Starfield BA2 v3.
4. Expand smoke/docs/boundary gates so codec dependencies remain private and routing confusion stays tested.

## Existing Patterns to Reuse

- Public headers live under `include/libbsa/`; implementation files live under `src/`.
- `CMakeLists.txt` uses explicit `target_sources` and explicit test executable registration; do not introduce recursive globbing.
- Public failures use `libbsa::result<T>`, `libbsa::error`, and `libbsa::error_code` from `include/libbsa/result.hpp`.
- Public archive identity and entry compression state are in `include/libbsa/archive.hpp`.
- Catch2 tests live under `tests/` and are run through CTest labels.
- Verification on this machine uses `build/local-vs2026-vcpkg` with Visual Studio 18 2026 fallback; do not edit the committed Visual Studio 17 2022 preset for local tooling.

## Library Findings

### libdeflate

- Use the raw deflate API for Bethesda deflate payloads unless a later compatibility fixture proves zlib-wrapped streams are required.
- `libdeflate_deflate_compress_bound` gives an allocation upper bound for compressed output.
- `libdeflate_deflate_decompress` requires caller-provided input and output buffers and reports `LIBDEFLATE_SUCCESS`, `LIBDEFLATE_BAD_DATA`, or `LIBDEFLATE_INSUFFICIENT_SPACE`.
- libbsa wrappers must validate that the produced decompressed size exactly equals archive metadata's expected uncompressed size; a shorter successful decode is a malformed/decompression failure for archive payload purposes.

### LZ4 frame and raw block APIs

- Use `LZ4F_compressFrameBound` / `LZ4F_compressFrame` and LZ4 frame decompression APIs only for complete LZ4 frame payloads, including Skyrim SE/AE BSA compressed payloads.
- Use `LZ4_compressBound`, `LZ4_compress_default`, and `LZ4_decompress_safe` for raw block payloads, including Starfield BA2 v3 records when `CompressionMethod == 3`.
- Frame payloads start with LZ4 frame magic bytes (`04 22 4D 18`) in normal complete-frame output; raw blocks do not. Tests should prove frame and raw-block decoders reject each other's bytes.

## Recommended File Layout

| Purpose | Files |
|---------|-------|
| Public compression API and routing | `include/libbsa/compression.hpp`, `src/compression.cpp`, `tests/compression_policy_tests.cpp` |
| Raw deflate codec wrapper | `src/compression/deflate_codec.hpp`, `src/compression/deflate_codec.cpp`, `tests/deflate_codec_tests.cpp` |
| LZ4 frame/block wrappers | `src/compression/lz4_frame_codec.hpp`, `src/compression/lz4_frame_codec.cpp`, `src/compression/lz4_block_codec.hpp`, `src/compression/lz4_block_codec.cpp`, `tests/lz4_codec_tests.cpp` |
| Boundary docs/smoke | `tests/public_header_smoke.cpp`, `README.md`, `CMakeLists.txt` |

## Architecture Patterns

- Public API should use libbsa-owned enums and structs only; do not include `libdeflate.h`, `lz4.h`, or `lz4frame.h` from public headers.
- Route by explicit `archive_format`, `compression_state`, and Starfield BA2 v3 `compression_method`; never infer codec from file extension.
- Deflate and LZ4 wrappers should accept whole payload spans and expected output sizes because current archive metadata stores packed and unpacked sizes per payload/chunk.
- Writer policy should distinguish `archive_default`, `force_compressed`, and `force_raw` as policy inputs, then resolve to concrete `compression_state` only when target format capabilities are known.
- Return `error_code::decompression_failure` for codec failures and exact-size mismatches.

## Common Pitfalls

- Mixing LZ4 frame and raw block APIs can silently produce unreadable archives; tests must cross-feed frame bytes to raw block decode and raw block bytes to frame decode and require failure.
- Letting third-party codec types leak into public headers violates `BIO-05` and the project boundary.
- Treating successful decompression with the wrong output length as success would mask corrupt archive metadata.
- Adding zlib, miniz, or bundled LZ4 sources is not justified in Phase 03; existing vcpkg dependencies already cover required codecs.
- Do not edit, format, stage, or compile files under `TES5Edit/` while tracing compression behavior.

## Validation Architecture

- Test framework: Catch2 through CTest.
- Quick command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit`.
- Full command: `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`.
- Codec command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_(compression_policy|deflate_codec|lz4_codec)_tests"`.
- Boundary command: public-header forbidden token grep excluding comments/prose where appropriate, plus `git status --short TES5Edit` must be empty.

## Source Coverage Notes

- GOAL: correct compression/decompression behavior for each target archive variant — covered by routing contracts, deflate, LZ4 frame, raw LZ4 block, and final boundary docs.
- REQ: `CMP-01` through `CMP-05` — covered across the four planned slices.
- RESEARCH: codec API isolation, exact-size validation, and route-crossfeed tests — covered by each relevant plan.
- CONTEXT: no Phase 03 CONTEXT.md exists; planning uses requirements, roadmap, project constraints, and this research.

## Research Complete

Use these findings to create executable TDD-oriented plans for Phase 03.
