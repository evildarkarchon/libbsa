---
phase: 10-ba2-writers
reviewed: 2026-05-07T12:00:00Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - CMakeLists.txt
  - README.md
  - include/libbsa/ba2_writer.hpp
  - src/ba2_writer.cpp
  - src/texture/dds_analysis.hpp
  - src/texture/dds_analysis.cpp
  - src/texture/dds_reconstruction.cpp
  - tests/ba2_writer_tests.cpp
  - tests/public_header_smoke.cpp
findings:
  critical: 3
  warning: 1
  info: 0
  total: 4
status: issues_found
---

# Phase 10: Code Review Report

**Reviewed:** 2026-05-07T12:00:00Z
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Reviewed the BA2 writer API, implementation, DDS analysis/reconstruction paths, build wiring, README claims, and writer/smoke tests. The implementation still has correctness blockers that the current read-after-write tests miss because they mostly reopen through libbsa itself and do not assert several native BA2/DDS compatibility bytes or multi-array texture layouts.

## Critical Issues

### CR-01: BLOCKER - Native BA2 extension field is written with the dot

**File:** `src/ba2_writer.cpp:103-109`
**Issue:** `append_extension4` starts copying at the period returned by `find_last_of('.')`, so native records store values like `.nif`, `.dds`, and `.str`. BA2's four-byte extension field is the extension text without the separator, padded/truncated to four bytes. The libbsa reader ignores this field, so all current round-trip tests can pass while emitted archives remain incompatible with native/BSArchPro consumers that rely on the record extension.
**Fix:** Skip the dot and pad with NUL bytes. Add tests that assert the raw record bytes for GNRL and DX10 records, including a 3-character extension and a longer extension.

```cpp
void append_extension4(std::vector<std::byte>& bytes, std::string_view path)
{
    const auto dot = path.find_last_of('.');
    const auto extension = dot == std::string_view::npos
        ? std::string_view{}
        : path.substr(dot + 1U, std::min<std::size_t>(4U, path.size() - dot - 1U));
    for (std::size_t i = 0; i < 4U; ++i) {
        bytes.push_back(i < extension.size()
            ? static_cast<std::byte>(static_cast<unsigned char>(extension[i]))
            : std::byte{0});
    }
}
```

### CR-02: BLOCKER - Multi-mip array/cubemap DDS payloads are reordered incorrectly

**File:** `src/texture/dds_analysis.cpp:149-164`, `src/texture/dds_reconstruction.cpp:112`
**Issue:** `analyze_dds` serializes chunk payloads in mip-major order (`mip` outer, `array item` inner), and extraction reconstructs DDS files by appending chunk bytes directly. DDS texture arrays/cubemaps are item-major: all mips for array item/face 0, then all mips for item/face 1, etc. The current tests cover multi-mip single-item textures and single-mip arrays/cubemaps, but not multi-mip arrays/cubemaps, so this data corruption is invisible. A valid 2-item, 2-mip texture will be written/extracted with mip 0 for every item before mip 1, producing a DDS whose image payload no longer matches the source texture layout.
**Fix:** Either reject `array_size > 1 && mip_count > 1` during planning until safe reordering exists, or preserve enough per-item/per-mip layout metadata to reconstruct DDS payloads in the required item-major order. Add a regression test using a multi-mip array or cubemap DDS with distinct bytes per item/mip and compare extracted DDS payload order against the source.

### CR-03: BLOCKER - Cubemap arrays are silently collapsed to a single cube in BA2 records

**File:** `src/ba2_writer.cpp:692`, `src/texture/dds_analysis.cpp:106-147`
**Issue:** DDS analysis accepts any DirectXTex cubemap metadata, including cubemap arrays where `metadata.arraySize` is 12, 18, etc. The BA2 writer then serializes the DX10 record's array/cubemap field as `6` for every cubemap, discarding additional cubes. The plan preview still reports the original `array_size`, so callers see a successful plan for data that is not faithfully encoded in the archive table. This is silent metadata/data loss for valid DDS inputs.
**Fix:** If Phase 10 only supports one cubemap, reject cubemap inputs where `array_size != 6` with `unsupported_format`. If cubemap arrays are intended to be supported, encode the native field according to the BA2 format and update reader/reconstruction tests to prove multi-cube extraction preserves all faces.

```cpp
if (analyzed.is_cubemap && analyzed.array_size != 6U) {
    return failure<analyzed_dds_texture>({error_code::unsupported_format,
        "DDS analysis failed: cubemap arrays are not supported"});
}
```

## Warnings

### WR-01: WARNING - Finalization accepts a default-constructed invalid plan as success

**File:** `src/ba2_writer.cpp:736-748`, `tests/ba2_writer_tests.cpp:850-859`
**Issue:** `finalize_ba2_write` writes whatever bytes are present in the public `ba2_write_plan` and returns success for an empty/default plan. The test suite explicitly locks in this behavior. Since `ba2_write_plan` is public and mutable, callers can accidentally finalize an unplanned or corrupted plan and receive success with an empty/non-BA2 output stream, which is a robustness and data-loss risk.
**Fix:** Validate basic plan invariants before writing: non-empty `table_bytes`, `table_bytes.size() <= total_size`, `magic == BTDX`, subtype/target consistency, and data-region sizes matching stored payloads. Replace the current empty-plan success test with a failure expectation.

---

_Reviewed: 2026-05-07T12:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
