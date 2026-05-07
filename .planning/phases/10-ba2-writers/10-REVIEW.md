---
phase: 10-ba2-writers
reviewed: 2026-05-07T11:19:54Z
depth: standard
files_reviewed: 8
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/ba2_writer.hpp
  - README.md
  - src/ba2_writer.cpp
  - src/texture/dds_analysis.cpp
  - src/texture/dds_analysis.hpp
  - tests/ba2_writer_tests.cpp
  - tests/public_header_smoke.cpp
findings:
  critical: 2
  warning: 0
  info: 0
  total: 2
status: issues_found
---

# Phase 10: Code Review Report

**Reviewed:** 2026-05-07T11:19:54Z
**Depth:** standard
**Files Reviewed:** 8
**Status:** issues_found

## Summary

Reviewed the BA2 writer public API, implementation, DDS analysis boundary, build wiring, README phase notes, and writer/smoke tests. The implementation has correctness blockers in DX10 compression edge handling and DDS format coverage that can produce unreadable output or reject valid BA2 texture inputs.

## Critical Issues

### CR-01: BLOCKER - Compressed DX10 chunks become silently corrupt when packed size equals unpacked size

**File:** `src/ba2_writer.cpp:619-637`
**Issue:** DX10 chunk records have no explicit compression flag in this implementation; the reader classifies `packed_size == size` as raw. The writer always records `chunk.packed_size = stored_size.value()` for both raw and compressed chunks. If deflate or Starfield LZ4-block output ever has exactly the same byte count as the unpacked chunk, `open_ba2` will mark that compressed chunk as raw and extraction will return compressed bytes as image data. This is a data-corruption bug for valid writer inputs.
**Fix:** After compression, handle the ambiguous equality case before emitting the chunk record. Either store the chunk raw, or retry/return a structured failure. For example:

```cpp
auto effective_compression = compression.value();
auto effective_stored = std::move(stored.value());

if (effective_compression != compression_state::raw &&
    effective_stored.size() == source_chunk.payload.size()) {
    // BA2 DX10 readers use PackedSize == Size as the raw marker, so equal-size
    // compressed bytes would be decoded as raw payload and corrupt extraction.
    effective_compression = compression_state::raw;
    effective_stored.assign(source_chunk.payload.begin(), source_chunk.payload.end());
}

auto unpacked_size = checked_u32(source_chunk.payload.size());
auto stored_size = checked_u32(effective_stored.size());
chunk.packed_size = stored_size.value();
chunk.compression = effective_compression;
```

Add a focused test that forces the equal-size path, e.g. by factoring/stubbing compression at the planning boundary or by adding a helper that exercises the DX10 chunk-size decision directly.

### CR-02: BLOCKER - DDS writer rejects valid non-BC1 DDS textures despite BA2 DX10 writer support

**File:** `src/texture/dds_analysis.cpp:67-68,121-123`
**Issue:** `analyze_dds` hard-codes support to `DXGI_FORMAT_BC1_UNORM` (`71`) and rejects every other valid DDS format. BA2 DX10 archives commonly contain other DDS formats, and the Phase 10 public API/README presents DDS/DX10 writer support rather than a BC1-only writer. This means valid DDS inputs such as BC3, BC5, BC7, or R8G8B8A8 textures cannot be planned or written.
**Fix:** Replace the single-format gate with a supported-format policy that covers the BA2 formats intended for this phase, and carry the real DirectXTex-derived `metadata.format` through the plan. If reconstruction/extraction currently only supports BC1, either extend that boundary in the same change or explicitly fail planning only for formats that cannot be round-tripped by libbsa yet with public documentation and tests for the declared limitation.

---

_Reviewed: 2026-05-07T11:19:54Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
