# Quick Task 260515-95h: Codex Review - Research

**Researched:** 2026-05-15  
**Domain:** TES4-family BSA parser payload-span validation and Windows host-path validation  
**Confidence:** HIGH

## Summary

TES4-family BSA parsing currently validates each payload span against archive bounds and rejects spans that intersect the metadata table, but it does not compare non-empty payload spans against previously accepted entries before publishing `entry_metadata`. [VERIFIED: src/formats/bsa/tes4_bsa_payload_descriptor.cpp; src/formats/bsa/tes4_bsa_parser.cpp] BA2 GNRL and BA2 DX10 already implement the desired policy: exact duplicate non-empty spans are accepted for dedupe, zero-length spans are ignored by overlap logic, and partial overlaps return `format_error` before metadata is exposed. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp; src/formats/ba2/ba2_dx10_parser.cpp]

`detail::resolve_host_file_path(std::string_view)` currently copies the caller text into a `std::string`, rejects empty and overlong inputs, then uses `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)` before constructing `std::filesystem::path`. [VERIFIED: src/detail/host_file_path.cpp] It does not reject embedded NUL bytes before conversion, while Windows file APIs take NUL-terminated path strings such as `CreateFileW`'s `LPCWSTR lpFileName`, creating a prefix-truncation risk if a caller supplies `"prefix\0suffix"`. [VERIFIED: src/detail/host_file_path.cpp] [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-createfilew] [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-getfullpathnamew]

**Primary recommendation:** Add TES4 span tracking inside `materialize_entries` using the BA2 exact-duplicate-vs-partial-overlap pattern, and add an early `host_path.find('\0')` invalid-argument guard in `resolve_host_file_path` before constructing or converting path text. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp; src/detail/host_file_path.cpp]

## Project Constraints (from AGENTS.md)

- Implementation belongs outside `TES5Edit/`; the submodule is read-only and must not be edited, formatted, staged, compiled, or used as vendored source. [VERIFIED: AGENTS.md]
- The project is Windows-only; do not add Linux/macOS/POSIX portability work for this task. [VERIFIED: AGENTS.md]
- Use C++20 and keep the reusable library independent of UI/tooling concerns. [VERIFIED: AGENTS.md]
- Do not introduce speculative external dependencies; prefer standard library for validation logic. [VERIFIED: AGENTS.md]
- Preserve accurate comments, add comments for non-obvious compatibility/security constraints, and add Doxygen comments only for public APIs or substantially rewritten methods. [VERIFIED: AGENTS.md]
- Add focused parser/path tests and do not keep production code solely for test compatibility. [VERIFIED: AGENTS.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| TES4 payload-span validation | Format parser | Public reader/validation surfaces | Parser owns rejecting malformed archive layout before `entry_metadata` is published; `archive_reader::open` and `validate_archive` reuse strict parser errors. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp; .planning/PROJECT.md] |
| Embedded-NUL host-path validation | Shared host-file boundary | Public reader/writer/validation call sites | `resolve_host_file_path` is the shared conversion seam for public UTF-8 host paths and should reject unsafe text before native path construction or filesystem I/O. [VERIFIED: src/detail/host_file_path.cpp; tests/unit/host_file_writer_name_tests.cpp] |

## Targeted Findings

### 1. TES4 overlap validation should match BA2 dedupe-safe semantics

- TES4 `materialize_entries` builds entries in folder/file order, checks duplicate canonical paths and file hash/name consistency, then calls `make_tes4_bsa_payload_descriptor` before pushing `entry_metadata`. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp]
- `make_tes4_bsa_payload_descriptor` already rejects payload spans outside the archive and non-empty spans intersecting the metadata prefix. [VERIFIED: src/formats/bsa/tes4_bsa_payload_descriptor.cpp]
- BA2 GNRL stores accepted spans as `{offset, size}`, skips `stored_size == 0`, accepts exact duplicate `{offset, size}`, and rejects any other overlap with message `BA2 GNRL entry payload spans partially overlap`. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp]
- BA2 DX10 applies the same exact-duplicate allowance to chunk spans and comments that writer dedupe can intentionally publish exact duplicate chunks while partial sharing makes extraction ambiguous. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp]
- TES3 sorts non-empty spans by start/end after materialization and rejects any overlap, but TES3 does not include the BA2/TES4 writer dedupe allowance for exact duplicate spans. [VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]

**Implementation shape:** Add a private `stored_payload_span` or equivalent near TES4 parser helpers, reserve `table.header.file_count` spans with `detail::reserve_metadata_vector`, and after descriptor creation but before `entries.push_back`, compare non-empty `{payload_offset, stored_size}` against accepted spans. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp; src/formats/ba2/ba2_gnrl_parser.cpp]

```cpp
// Pattern source: src/formats/ba2/ba2_gnrl_parser.cpp and ba2_dx10_parser.cpp
if (payload.value().stored_size != 0U)
{
  for (const auto &prior : accepted_payload_spans)
  {
    const auto exact_duplicate = prior.offset == payload.value().payload_offset &&
                                 prior.size == payload.value().stored_size;
    if (!exact_duplicate && spans_overlap_u64(prior.offset, prior.size,
                                              payload.value().payload_offset,
                                              payload.value().stored_size))
    {
      return error{error_code::format_error, "TES4 BSA entry payload spans partially overlap"};
    }
  }
  accepted_payload_spans.push_back({payload.value().payload_offset, payload.value().stored_size});
}
```

### 2. Host-path embedded NUL validation belongs before UTF-8 conversion

- `resolve_host_file_path` currently receives `std::string_view`, copies into `std::string`, treats an empty path as a default path, checks size against `INT_MAX`, then calls `MultiByteToWideChar` with an explicit byte count. [VERIFIED: src/detail/host_file_path.cpp]
- Existing tests cover UTF-8 decoding, strict `MB_ERR_INVALID_CHARS`, lack of diagnostics-only original UTF-8 state, and malformed UTF-8 rejection. [VERIFIED: tests/unit/host_file_path_tests.cpp]
- Add a direct embedded-NUL rejection test in `tests/unit/host_file_path_tests.cpp` beside malformed UTF-8, expecting `invalid_argument`. [VERIFIED: tests/unit/host_file_path_tests.cpp]
- The guard should operate on the original `std::string_view` or copied `std::string` before `MultiByteToWideChar`/`std::filesystem::path` so downstream Windows NUL-terminated APIs cannot observe a prefix. [VERIFIED: src/detail/host_file_path.cpp] [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-createfilew]

**Implementation shape:**

```cpp
if (host_path.find('\0') != std::string_view::npos)
{
  return error{error_code::invalid_argument, "archive path contains an embedded NUL byte"};
}
```

## Focused Test Plan

| Test Area | Recommended Test | Expected Result |
|-----------|------------------|-----------------|
| TES4 partial overlap | In `tests/unit/tes4_bsa_reader_tests.cpp`, mutate `tes4_v103.bsa` so two file records have non-empty, partially overlapping stored spans after metadata. [VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp] | `archive_reader::open` returns `format_error`; `validate_archive` returns invalid with one `format_error`, matching BA2 overlap tests. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; tests/unit/ba2_dx10_parser_tests.cpp] |
| TES4 exact duplicate | Mutate the same fixture so two non-empty records have identical offset and stored size. [VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp] | Open succeeds and entries retain duplicate payload offsets, matching BA2 duplicate-span tests. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; tests/unit/ba2_dx10_parser_tests.cpp] |
| Host path embedded NUL | Call `resolve_host_file_path(std::string_view{"prefix\0suffix", 13})`. [VERIFIED: tests/unit/host_file_path_tests.cpp] | Returns `invalid_argument` before filesystem I/O/conversion. [VERIFIED: src/detail/host_file_path.cpp] |

## Common Pitfalls

- **Rejecting exact duplicate TES4 spans:** Do not sort spans with a simple `start < previous.end` TES3-style check unless exact duplicates are excluded first; TES4 writer dedupe intentionally can publish identical stored spans. [VERIFIED: .planning/STATE.md; src/formats/ba2/ba2_gnrl_parser.cpp]
- **Tracking raw size instead of stored size:** Compare payload byte ranges using `payload_offset` plus `stored_size`, not uncompressed/raw size; compressed entries occupy `stored_size` bytes in the archive. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp; src/formats/bsa/tes4_bsa_payload_descriptor.cpp]
- **Checking overlaps before existing hash/path validation:** Preserve current error precedence where possible: TES4 currently validates canonical path uniqueness and record hash/name consistency before descriptor publication. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp]
- **NUL check after path construction:** Rejection must happen before native path construction/opening; otherwise later NUL-terminated Windows APIs may operate on a shorter prefix than the caller-provided string. [VERIFIED: src/detail/host_file_path.cpp] [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-createfilew]

## Validation Architecture

| Property | Value |
|----------|-------|
| Framework | Catch2 + CTest. [VERIFIED: .planning/PROJECT.md] |
| Focused command | `cmake --build --preset windows-msvc-debug --target libbsa_tests && ctest --preset windows-msvc-debug -R "tes4_bsa_malformed_open|host_file_path" --output-on-failure` [ASSUMED] |
| Broader regression | Existing Debug/ASan lanes are the project hardening baseline. [VERIFIED: .planning/STATE.md; .planning/PROJECT.md] |

## Security Domain

| Threat | STRIDE | Standard Mitigation |
|--------|--------|---------------------|
| Embedded NUL host path makes Windows file API use a prefix path | Tampering / Elevation of privilege | Reject embedded NUL bytes at the public UTF-8 host-file boundary before conversion/open. [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-createfilew] |
| Malformed archive uses partial span overlap to make one logical entry read bytes from another | Tampering | Reject non-empty partial payload overlaps before publishing `entry_metadata`; allow only exact duplicate dedupe spans. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp; src/formats/ba2/ba2_dx10_parser.cpp] |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The local preset name for focused test execution is `windows-msvc-debug`. | Validation Architecture | Planner/executor may need to adjust command to the actual configured preset. |

## Sources

### Primary (HIGH confidence)
- `AGENTS.md` — project constraints and TES5Edit read-only boundary. [VERIFIED: AGENTS.md]
- `.planning/STATE.md`, `.planning/PROJECT.md` — current state and relevant decisions. [VERIFIED: .planning/STATE.md; .planning/PROJECT.md]
- `src/formats/bsa/tes4_bsa_parser.cpp` — current TES4 materialization path. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp]
- `src/formats/bsa/tes4_bsa_payload_descriptor.cpp` — existing TES4 bounds/metadata overlap checks. [VERIFIED: src/formats/bsa/tes4_bsa_payload_descriptor.cpp]
- `src/formats/bsa/tes3_bsa_parser.cpp` — TES3 non-empty payload overlap pattern. [VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]
- `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp` — BA2 partial-overlap rejection and exact-duplicate allowance. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp; src/formats/ba2/ba2_dx10_parser.cpp]
- `src/detail/host_file_path.cpp`, `tests/unit/host_file_path_tests.cpp` — host-path conversion behavior and existing tests. [VERIFIED: src/detail/host_file_path.cpp; tests/unit/host_file_path_tests.cpp]
- Microsoft Learn `CreateFileW` and `GetFullPathNameW` docs — Windows path parameters use NUL-terminated wide strings/buffers. [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-createfilew] [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-getfullpathnamew]

## Metadata

**Confidence breakdown:**
- TES4 overlap pattern: HIGH — directly matched against existing BA2 and TES3 parser implementations. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp; src/formats/ba2/ba2_dx10_parser.cpp; src/formats/bsa/tes3_bsa_parser.cpp]
- Host path NUL rejection: HIGH — current seam and Windows API string behavior are directly verified. [VERIFIED: src/detail/host_file_path.cpp] [CITED: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-createfilew]
- Test command: LOW — preset name inferred from project convention, not inspected in this quick research. [ASSUMED]

**Research date:** 2026-05-15  
**Valid until:** 2026-06-14
