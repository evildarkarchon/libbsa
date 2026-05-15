# Quick Task 260515-62d: BA2 GNRL Partial Payload Overlap - Research

**Researched:** 2026-05-15  
**Domain:** BA2 GNRL parser validation hardening  
**Confidence:** HIGH

## Summary

The BA2 GNRL parser currently validates each record's payload span independently against archive bounds, fixed metadata, and the filename table, then publishes all entries after sorting by canonical path. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:382-408] It does not track previously accepted non-empty payload ranges, so two records with different offsets and partially overlapping stored spans can both pass `open()`/`validate_archive()` if each individual span is outside metadata and inside the file. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:323-408]

The fix should live in `materialize_entries(...)`, immediately after the existing per-record span checks and before `entries.push_back(...)`, because that helper has archive size, record/name-table boundaries, canonicalized records, and the computed `stored_size` for both in-memory and host-file parser paths. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:323-408] Track accepted non-empty stored ranges as `{offset, stored_size}`; reject overlaps unless both ranges are exact duplicates, which preserves writer-created dedupe behavior. [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:888-917]

**Primary recommendation:** Add parser-side accepted-range tracking in `materialize_entries(...)`: skip zero-length entries, allow exact duplicate `{offset, stored_size}` spans, and reject any other overlap with `format_error` before publishing metadata. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:382-408]

## Project Constraints (from AGENTS.md)

- Implementation work must remain outside `TES5Edit/`; the submodule is read-only and must not be edited, formatted, staged, or compiled into libbsa. [VERIFIED: AGENTS.md:20-33]
- libbsa is Windows-only; do not add portability work for Linux/macOS/POSIX for this task. [VERIFIED: AGENTS.md:16-18]
- Use C++ and preserve BSArchPro-compatible behavior unless documenting a reason to diverge. [VERIFIED: AGENTS.md:35-41]
- Do not introduce speculative dependencies; prefer the standard library for this parser validation change. [VERIFIED: AGENTS.md:43-53]
- Add focused tests for archive parsing, writing, round-tripping, and compatibility behavior as surfaces are implemented; do not use `TES5Edit/` as mutable fixtures. [VERIFIED: AGENTS.md:63-68]
- Preserve accurate comments; add comments for non-obvious format compatibility constraints. [VERIFIED: AGENTS.md:55-61]

## Existing Validation Flow

| Step | Current behavior | Source |
|------|------------------|--------|
| Header/records | Reads fixed header, validates magic/subtype/version/file_count, validates record sentinel, and computes `records_end`. | [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:141-210,454-490,531-608] |
| Names | Parses exactly `file_count` length-prefixed names from `FileTableOffset`; host-file path avoids deriving table size from first payload because payloads can appear before final filename table. | [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:212-312,589-592] |
| Per-entry metadata | Normalizes paths, rejects duplicate canonical paths, validates name/directory hashes and extension fourcc. | [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:346-380] |
| Per-entry payload span | Computes `stored_size = packed_size != 0 ? packed_size : size`; validates span inside archive, not overlapping fixed header/record table, and not overlapping filename table. | [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:382-396] |
| Missing check | No collection of accepted payload ranges exists before `entries.push_back`, so inter-record partial overlap is not rejected. | [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:323-408] |

## Architecture Pattern

### Recommended Project Structure

```text
src/formats/ba2/ba2_gnrl_parser.cpp      # Add accepted stored-range validation in materialize_entries
tests/unit/ba2_gnrl_reader_tests.cpp     # Add synthetic malformed partial-overlap open() regression
tests/fixtures/generated/...             # Optional only if adding a manifest-backed generated case
```

### Pattern: Accepted Stored Range Set

**What:** Maintain a local vector of accepted non-empty stored spans in `materialize_entries(...)`; for each new span, first skip if `stored_size == 0`, then compare against prior spans. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:323-408]

**Required comparison semantics:**

```cpp
// Pseudocode only; implement with existing project style.
struct stored_span { std::uint64_t offset; std::uint64_t size; };

if (stored_size != 0U) {
  for (const auto& prior : accepted_payload_spans) {
    const bool exact_duplicate = prior.offset == record.offset && prior.size == stored_size;
    if (!exact_duplicate && spans_overlap_u64(prior.offset, prior.size, record.offset, stored_size)) {
      return error{error_code::format_error, "BA2 GNRL entry payload spans partially overlap"};
    }
  }
  accepted_payload_spans.push_back({record.offset, stored_size});
}
```

**Why here:** `materialize_entries(...)` runs for both `parse_ba2_gnrl_archive(...)` and `parse_ba2_gnrl_archive_file(...)`, so one change hardens memory-backed parsing, archive open, and validation. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:489-490,607-608]

## Compatibility Constraints

- Reject **partial** overlap between two non-empty stored spans, even when both spans are individually within the archive and outside metadata. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:382-396]
- Allow exact duplicate stored spans `{offset, stored_size}` because BA2 GNRL writer dedupe intentionally shares offsets for byte-identical stored payloads when `deduplicate_payloads` is enabled. [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:888-917]
- Continue allowing distinct offsets by default when dedupe is disabled; the new reader check must not require dedupe or coalesce entries. [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:861-885]
- Ignore empty payload records for overlap tracking. Existing `spans_overlap_u64` treats zero-length spans as non-overlapping, and the generated FO4 fixture includes an empty marker entry. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:56-64; tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp:317-327]
- Use stored-size semantics, not raw-size-only semantics: compressed records use `packed_size`; raw records use `size` when `packed_size == 0`. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:314-320,382]

## Test Findings

Existing malformed coverage is manifest-backed for truncation, invalid payload span, duplicate canonical path, corrupt compressed payload, size mismatch, and unsupported compression method. [VERIFIED: tests/fixtures/generated/archives/ba2_gnrl_malformed_manifest.json:9-19] The manifest-driven reader test expects open-phase cases to fail at `archive_reader::open(...)` and extraction-phase cases to fail during extraction. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp:900-940]

There are already helper functions in `ba2_gnrl_reader_tests.cpp` for building synthetic BA2 bytes (`append_u16_le`, `append_u32_le`, `append_u64_le`, `append_ascii`) and temp-file cleanup. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp:189-238] The fastest targeted regression is a new direct unit test in `ba2_gnrl_reader_tests.cpp` that creates two valid records with distinct names/hashes and payload ranges such as `[payload_base, payload_base + 8)` and `[payload_base + 4, payload_base + 12)`, writes enough payload bytes after the filename table, then requires `archive_reader::open(temp_path.string())` to fail with `format_error`. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp:505-547]

Add a second positive regression only if implementation risk warrants it: exact duplicate spans with two different canonical paths should still open and extract both entries successfully. Writer tests already verify dedupe-generated exact duplicate offsets, but a parser-level synthetic test would lock the malformed-overlap boundary directly. [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:888-917]

## Common Pitfalls

1. **Overflow in overlap math:** `spans_overlap_u64` computes `start + length` without its own overflow guard, so only call it after `span_fits_u64(offset, stored_size, archive_size)` has proven `offset + stored_size` fits within the archive. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:56-64,382-385]
2. **Rejecting exact dedupe:** Treating any overlap as invalid would reject archives produced by libbsa's own dedupe path. [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:888-917]
3. **Tracking empty spans:** Empty entries can share offsets harmlessly; pushing zero-length spans makes the logic harder and can create false positives if future comparison logic changes. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:56-64; tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp:317-327]
4. **Using raw size for compressed records:** Extraction reads the stored payload span first and then decompresses according to metadata, so overlap validation must use `packed_size` for compressed records. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:314-320,382]
5. **Only hardening host-file open:** Both memory-backed and host-file parser flows call `materialize_entries(...)`; avoid duplicating the check in only one caller. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:489-490,607-608]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Span arithmetic | New unchecked end calculations | Existing `span_fits_u64` then existing `spans_overlap_u64` | Current parser already centralizes archive-bound validation before overlap checks. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:382-396] |
| Persistent fixture corpus for one edge | New binary fixture checked in by hand | Synthetic temp-file test or generator-backed malformed case | Existing tests prefer generated/synthetic legal bytes and no game/TES5Edit bytes. [VERIFIED: tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp:464-486] |
| Dependency for interval checks | Interval tree/library | Local `std::vector` scan | File counts are already metadata-limited before materialization; no new dependency is justified. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:446-452; AGENTS.md:43-53] |

## Validation Architecture

| Behavior | Test Type | Suggested Command |
|----------|-----------|-------------------|
| Partial overlap rejected at open | Catch2 unit/malformed | `ctest --preset windows-msvc-debug -R ba2_gnrl_reader_tests --output-on-failure` [ASSUMED] |
| Exact duplicate dedupe still opens/extracts | Existing Catch2 writer unit | Existing BA2 GNRL writer tests cover shared offsets when dedupe is enabled. [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:888-917] |
| Manifest malformed cases unchanged | Existing Catch2 fixture unit | Existing manifest loop verifies stable error codes. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp:900-940] |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `ctest --preset windows-msvc-debug -R ba2_gnrl_reader_tests --output-on-failure` is the exact local focused command. | Validation Architecture | Command may need adjustment to the repository's current preset/test executable naming. |

## Sources

### Primary (HIGH confidence)
- `AGENTS.md` — project constraints, dependency rules, TES5Edit boundary. [VERIFIED: AGENTS.md:16-68]
- `.planning/PROJECT.md` — current project state and supported BA2 families. [VERIFIED: .planning/PROJECT.md:43-59,95-109]
- `.planning/STATE.md` — v1.1 state and BA2 GNRL dedupe decisions. [VERIFIED: .planning/STATE.md:60-63]
- `src/formats/ba2/ba2_gnrl_parser.cpp` — parser validation flow and exact insertion point. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:323-408,489-490,607-608]
- `tests/unit/ba2_gnrl_reader_tests.cpp` — synthetic malformed/open tests and manifest loop. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp:189-238,505-547,900-940]
- `tests/unit/ba2_gnrl_writer_tests.cpp` — dedupe exact duplicate offset behavior. [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:861-917]
- `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` and generated malformed manifest — fixture generation/malformed coverage. [VERIFIED: tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp:464-541]

## Metadata

**Confidence breakdown:**
- Existing parser behavior: HIGH — verified directly from target implementation. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp:323-408]
- Test strategy: HIGH — verified existing synthetic helper patterns and malformed assertions. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp:189-238,900-940]
- Exact validation command: LOW — inferred from repository CTest/Catch2 conventions, not re-verified in this quick research. [ASSUMED]

**Research date:** 2026-05-15  
**Valid until:** 2026-06-14
