## Context

The BSA and BA2 parsers already validate many archive-derived byte spans with shared arithmetic helpers and use byte-vector/string helpers to translate some allocation failures into `format_error`. They still trust archive-declared metadata counts long enough to reserve typed vectors, grow hash sets, iterate duplicate checks, and sort entry metadata.

The affected paths are all open/list metadata paths. They should fail before extraction or decompression if a header attempts to force unreasonable metadata work. The project is Windows-only C++20, avoids speculative dependencies, and keeps public headers minimal, so this design uses internal parser policy rather than a new public options object.

## Goals / Non-Goals

**Goals:**

- Reject excessive TES3 file counts, TES4 folder/file counts, BA2 GNRL file counts, BA2 DX10 file counts, and aggregate BA2 DX10 texture chunk counts with `format_error`.
- Apply count checks before typed metadata container reservation, duplicate-detection hash-set growth, sort inputs, and DX10 public chunk materialization.
- Translate metadata allocation failures from typed vectors, strings, and unordered sets into `format_error` on public parser paths.
- Keep archive-format compatibility behavior intact for ordinary archives and existing malformed structural checks.

**Non-Goals:**

- Add caller-configurable validation limits or a new public `archive_reader::open` overload.
- Change payload extraction size limits, compression/decompression behavior, writer limits, or DirectXTex integration.
- Tune limits from telemetry. The first implementation should choose conservative internal defaults and document the rationale in code.

## Decisions

### Use Internal Metadata Limit Constants

Define internal parser metadata limits in `src/detail/parser_primitives.hpp` or a small adjacent internal header. The limits should cover at least archive entry counts, BSA folder counts, and aggregate DX10 chunk counts.

Rationale: the immediate risk is default parser exposure to untrusted archive counts. Internal constants close that risk without committing libbsa to a public configuration API before real callers need one.

Alternatives considered: caller-configurable validation options would be more flexible, but `archive_reader::open` currently has no options object and adding one would expand the public API for a mitigation that can be safely enforced by default.

### Validate Counts at Parser Boundaries

Run count checks immediately after reading a header or record count field and before any `reserve`, record-table read sized by that count, duplicate-detection set growth, or entry materialization. For TES4, validate both the header `folder_count`/`file_count` and each folder record's `file_count` against the aggregate header count and policy limit. For BA2 DX10, maintain a running total of parsed chunk counts and fail as soon as the aggregate would exceed the policy.

Rationale: early rejection prevents both memory pressure and CPU work. Keeping checks in parser modules preserves archive-family context in diagnostics.

Alternatives considered: relying only on byte-span arithmetic catches integer overflow and truncation, but it still allows large count values that are structurally valid enough to allocate or iterate over.

### Add Result-Based Metadata Container Helpers

Add small internal helpers for typed metadata allocation, such as result-returning vector reserve and unordered-set reserve wrappers, and use them in affected parser paths. Public parser entry points should also retain a narrow allocation exception boundary around metadata materialization so unexpected metadata-container growth still becomes `format_error`.

Rationale: `byte_vector.hpp` covers byte buffers only. Parser metadata uses `std::vector<record>`, `std::vector<entry_metadata>`, `std::vector<std::string>`, `std::unordered_set<std::string>`, and `std::unordered_set<std::uint64_t>`, all of which can throw independently from byte-buffer reads.

Alternatives considered: wrapping every parser call in a broad `catch (...)` would hide non-allocation bugs. Only `std::bad_alloc` and `std::length_error` should be translated, preserving other programmer errors.

### Preserve Existing Error Categories

Use `libbsa::error_code::format_error` for count-limit failures and archive-controlled metadata allocation failures. Keep `io_error`, `unsupported`, and existing specific malformed-data errors unchanged.

Rationale: excessive archive-controlled metadata is a malformed-input condition from the parser's perspective. This matches current byte-vector allocation translation and validation report behavior.

## Risks / Trade-offs

- Real archive exceeds a new internal limit -> choose generous defaults, make the constants easy to adjust internally, and add tests that assert the boundary behavior rather than hard-coding user-facing messages.
- Limit checks duplicate existing arithmetic validation -> keep arithmetic checks because they prove byte-span correctness, and add count checks only for memory/CPU policy.
- Allocation failures still occur in an unexpected container path -> combine local result-based reserves with a narrow allocation exception boundary at parser entry/materialization boundaries.
- BA2 DX10 file parsing may allocate metadata bytes before aggregate chunk count is known -> validate file counts and file-table offset bounds before reading metadata, then reject aggregate chunks during record parsing before public chunk vectors or entries are materialized.

## Migration Plan

No data migration is required. Existing callers continue using `archive_reader::open` and `validate_archive`; archives that exceed the internal safety policy fail with `format_error` instead of driving unbounded metadata work.

## Open Questions

- None for the initial change. If downstream callers later need larger limits for verified local archives, propose a separate public options change with compatibility documentation.
