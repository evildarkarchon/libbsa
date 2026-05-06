# Phase 05: TES3 BSA Read and Extract — Research

**Status:** Complete
**Phase:** 05 — TES3 BSA Read and Extract
**Requirement:** BSA-04

## Research Summary

Phase 05 should extend the existing `bsa_archive` surface rather than introduce a separate public archive type. Detection already recognizes TES3 BSA by the first four bytes `00 01 00 00` (`0x00000100`) and reports `archive_format::tes3_bsa`, but `open_bsa` currently assumes the TES4-family `BSA\0` magic and rejects TES3 bytes.

The implementation should add a TES3 branch inside `open_bsa` and reuse `bsa_archive`, `archive_view`, `entry_metadata`, `memory_source`, `memory_sink`, and `extract_bsa_entry`. TES3 payloads are uncompressed and have no folder table; stored offsets are relative to the data section, not absolute archive offsets.

## Reference Findings

Primary reference: `TES5Edit/Core/wbBSArchive.pas` (read-only).

Relevant reference behavior:

- Lines 242-252 define `TwbBSHeaderTES3` and `TwbBSFileTES3`:
  - Header after magic: `HashOffset: Cardinal`, `FileCount: Cardinal`.
  - File record fields: `Hash: UInt64`, `Size: Cardinal`, `Offset: Cardinal`, `Name: string`.
- Lines 1079-1084 identify TES3 archives by `MAGIC_TES3` instead of the TES4 `BSA\0` magic.
- Lines 1112-1130 parse Morrowind BSA layout:
  1. Read TES3 header.
  2. Read `FileCount` pairs of `{Size, Offset}`.
  3. Skip `4 * FileCount` name-offset entries.
  4. Read `FileCount` null-terminated names.
  5. Read `FileCount` UInt64 hashes.
  6. Store `fDataOffset := fStream.Position`.
- Lines 2108-2119 extract TES3 entries by seeking to `fDataOffset + FileTES3.Offset` and reading `FileTES3.Size` bytes.
- Lines 705-731 define `CreateHashTES3`; libbsa already implements this as `detail::hash_tes3_path` and has golden tests.

## Implementation Direction

Use the existing reader files:

- `include/libbsa/bsa.hpp` — keep the public API stable; update doc comments from "TES4-family" to BSA-family where needed.
- `src/bsa_reader.hpp` — add TES3 constants such as `magic_tes3`, `tes3_header_size`, and record-size helpers.
- `src/bsa_reader.cpp` — route `open_bsa` by first magic value:
  - TES3 magic (`0x00000100`) → parse TES3 tables.
  - TES4-family magic (`0x00415342`) → preserve existing v103/v104/v105 parsing.
  - Otherwise → `unsupported_format`, `unsupported BSA magic`.
- `tests/bsa_reader_tests.cpp` — add generated TES3 fixture bytes, open/list/metadata tests, extraction tests, and malformed offset/table tests.

## TES3 Layout Contract

TES3 BSA bytes should be interpreted as:

```text
u32 magic/version = 0x00000100
u32 hashOffset
u32 fileCount
repeat fileCount:
  u32 size
  u32 dataSectionRelativeOffset
repeat fileCount:
  u32 nameOffset
repeat fileCount:
  null-terminated name bytes
repeat fileCount:
  u64 hash
data section begins immediately after hash table
payload absolute offset = dataSectionOffset + dataSectionRelativeOffset
```

Compatibility constraints:

- `hashOffset` is the offset from byte 12 to the hash table in TES5Edit writer logic (`fHeaderTES3.HashOffset := fDataOffset - 12`) and can be used as a bounded consistency check for the hash-table start.
- Name offsets should be parsed and validated as offsets into the contiguous name block, not ignored, so malformed name tables cannot silently associate records with the wrong names.
- Store `entry_metadata.offset` as the absolute payload offset because extractors validate against the source size using absolute offsets. Preserve the relative offset semantics in tests and comments.
- Store `entry_metadata.stored_size`, `size`, and `packed_size` all equal to the TES3 uncompressed size. Store `compression_state::raw`.
- Store `name_hash` from the hash table. Tests should also prove `detail::hash_tes3_path(metadata.path)` matches the stored hash for generated fixtures.
- `directory_hash` should be `0` because TES3 has a flat file table, not TES4 folder records.

## Validation Architecture

Automated validation should use existing CTest/Catch2 infrastructure:

- Targeted command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`
- Hash command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_path_hash_tests`
- Smoke command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke`
- Full command: `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`

Required test coverage:

1. `open_bsa` identifies TES3 archives as `archive_format::tes3_bsa`.
2. TES3 listing returns normalized paths while preserving TES3 hashes, sizes, and absolute data offsets derived from data-section-relative records.
3. `extract_bsa_entry` writes exact raw TES3 payload bytes through `memory_sink`.
4. TES3 malformed inputs fail with structured `malformed_archive` errors for truncated records, impossible data-relative offsets, and invalid name/hash table ranges.
5. Existing TES4-family tests remain green.

## Security Notes

Trust boundary is untrusted archive bytes crossing into table parsing and extraction. Mitigations are bounded reads, checked offset arithmetic, source-size range checks before allocation/read, structured failures, and no writes to `TES5Edit/`.

## Source Audit

| Source | Item | Coverage |
|--------|------|----------|
| GOAL | Open, inspect, list, look up, and extract Morrowind BSA archives | Covered by 05-01 through 05-04 |
| REQ BSA-04 | Open/list/inspect/extract TES3 with data-section-relative offsets | Covered by every plan |
| RESEARCH | TES3 header/record parsing and name/hash tables | Covered by 05-01 and 05-03 |
| RESEARCH | Data-section-relative offset extraction | Covered by 05-02 |
| RESEARCH | Fixture-backed malformed input validation | Covered by 05-03 |
| CONTEXT | No CONTEXT.md exists | No locked user decisions to implement |

## Out of Scope

- TES3 writing belongs to Phase 9 (`WRT-04`).
- BA2 formats belong to Phases 6 and 7.
- Bulk/parallel extraction belongs to Phase 12.
