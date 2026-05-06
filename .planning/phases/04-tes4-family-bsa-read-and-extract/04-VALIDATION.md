---
phase: 04
phase_slug: tes4-family-bsa-read-and-extract
created: 2026-05-06
---

# Validation Strategy: TES4-Family BSA Read and Extract

## Required Validation Dimensions

1. v103/v104/v105 table parsing exposes correct normalized paths and metadata.
2. Per-entry compression state uses archive-default XOR file-size flag behavior.
3. Extraction writes raw, deflate, and LZ4-frame payloads to caller-owned `byte_sink`.
4. Embedded filename prefixes in v104/v105 are skipped before payload handling.
5. Malformed/truncated offsets, sizes, and names return structured failures.
6. Public headers remain dependency-clean and `TES5Edit/` remains untouched.

## Required Verification Commands

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L fixture`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke`
- `rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex|TES5Edit" include/libbsa`
- `rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt`
- `git status --short TES5Edit`
