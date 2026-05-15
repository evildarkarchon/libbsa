# Quick Research: Reject Partially Overlapping BA2 DX10 Chunk Payload Spans

**Researched:** 2026-05-15  
**Task:** Codex Review: Reject partially overlapping BA2 DX10 chunk payload spans while allowing exact duplicate spans for writer dedupe.  
**Confidence:** HIGH

## Project Constraints

- Do not modify, format, stage, or compile anything under `TES5Edit/`; it is read-only behavioral reference material only. [VERIFIED: AGENTS.md]
- Keep implementation in C++20 and inside libbsa source, with no speculative external dependencies. [VERIFIED: AGENTS.md]
- Add focused parser/writer/compatibility tests for archive behavior changes; do not add product-code test shims. [VERIFIED: AGENTS.md]
- Existing project state says v1.1 hardening is complete and the immediately previous quick task rejected partially overlapping BA2 GNRL payloads. [VERIFIED: .planning/STATE.md]

## Findings

### 1. Current DX10 parser validates bounds but not cross-chunk overlap

`ba2_dx10_parser.cpp` validates each DX10 chunk independently in `first_payload_offset_for`: it computes `stored_size` as `packed_size != 0 ? packed_size : raw_size`, rejects zero/inconsistent chunk sizes, checks `span_fits_u64(chunk.offset, stored_size, archive_size)`, and tracks the minimum payload offset. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp]

That path does not keep a list of accepted payload spans, so two different chunks can partially overlap as long as each individual span fits inside the archive. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp]

`materialize_entries` then turns ordered chunks into public `texture_chunk_metadata` and aggregates raw/stored sizes without re-checking whether physical chunk byte ranges overlap. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp]

### 2. GNRL now has the desired accepted-span pattern

`ba2_gnrl_parser.cpp` defines a local `stored_payload_span { offset, size }` and a `spans_overlap_u64` helper that treats zero-length spans as non-overlapping. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp]

During GNRL materialization, non-empty stored spans are compared against previously accepted spans; an exact duplicate `(same offset, same size)` is allowed, but any non-exact overlap returns `format_error` with message `BA2 GNRL entry payload spans partially overlap`. [VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp]

The GNRL tests already prove both sides of the policy: partial overlap rejects during `archive_reader::open` and `validate_archive`, while exact duplicate non-empty spans open successfully and extract identical bytes from both paths. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp]

### 3. DX10 writer already depends on exact duplicate spans being valid

`ba2_dx10_writer_tests.cpp` contains `BA2 DX10 writer shares duplicate DDS chunk offsets when deduplicate_payloads = true`, which opens the written archive, compares duplicate texture chunk offsets, and extracts both entries. [VERIFIED: tests/unit/ba2_dx10_writer_tests.cpp]

Therefore the parser fix must reject partial physical sharing without rejecting exact duplicate chunk spans produced by writer dedupe. [VERIFIED: tests/unit/ba2_dx10_writer_tests.cpp; src/formats/ba2/ba2_dx10_parser.cpp]

## Implementation Guidance

1. Add a local DX10 span type near `dx10_chunk_record`, e.g. `stored_chunk_span { std::uint64_t offset; std::uint64_t size; }`. [VERIFIED: codebase pattern in ba2_gnrl_parser.cpp]
2. Add or copy the GNRL `spans_overlap_u64` helper into `ba2_dx10_parser.cpp`; keep it local unless a follow-up refactor intentionally shares BA2 parser primitives. [VERIFIED: codebase pattern in ba2_gnrl_parser.cpp]
3. In `first_payload_offset_for`, reserve `accepted_payload_spans` using `detail::reserve_metadata_vector(..., detail::metadata_dx10_chunk_count_limit or aggregate expected count if convenient)` before scanning, or accumulate without reservation if avoiding extra pre-pass is preferred. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp]
4. For every non-empty DX10 chunk after existing `span_fits_u64` succeeds, compare with accepted spans:
   - `exact_duplicate = prior.offset == chunk.offset && prior.size == stored_size`
   - if `!exact_duplicate && spans_overlap_u64(prior.offset, prior.size, chunk.offset, stored_size)`, return `format_error`, e.g. `BA2 DX10 chunk payload spans partially overlap`
   - always push the current span after checks so later chunks can compare against it. [VERIFIED: codebase pattern in ba2_gnrl_parser.cpp]
5. Keep the check at the raw record level (`first_payload_offset_for`) rather than after `public_chunks_for`, so malformed archives fail before public metadata materialization and before extraction. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp]
6. Add a short comment matching the GNRL rationale: exact duplicate chunk spans are allowed for writer dedupe, but partial sharing makes texture chunk extraction ambiguous. [VERIFIED: AGENTS.md comment policy; codebase pattern in ba2_gnrl_parser.cpp]

## Focused Tests

### Add parser-level DX10 overlap tests

Best location: `tests/unit/ba2_dx10_parser_tests.cpp`, near the existing malformed parser tests and helper functions. [VERIFIED: tests/unit/ba2_dx10_parser_tests.cpp]

Use the existing synthetic helper style in that file: `append_*` helpers, temporary path, `archive_reader::open`, and `validate_archive` checks. [VERIFIED: tests/unit/ba2_dx10_parser_tests.cpp]

Recommended cases:

1. **Reject partial overlap**
   - Build a minimal two-entry or two-chunk DX10 archive where both chunks have valid hashes/names, valid `chunk_header_size == 24`, valid mip ranges for their records, and physical spans such as `[payload_base, payload_base + 8)` and `[payload_base + 4, payload_base + 12)`. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp pattern; src/formats/ba2/ba2_dx10_parser.cpp]
   - Assert `archive_reader::open(temp_path.string())` fails with `libbsa::error_code::format_error`. [VERIFIED: existing BA2 malformed tests]
   - Assert `validate_archive(temp_path.string())` succeeds as a report, `is_valid() == false`, and first error code is `format_error`. [VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp]

2. **Allow exact duplicate span**
   - Build two valid DX10 entries/chunks with the same physical `(offset, stored_size)` and distinct canonical archive paths. [VERIFIED: BA2 writer dedupe tests require exact duplicate DX10 chunk offsets to remain readable]
   - Assert open succeeds, both entries expose the same `texture->chunks.front().payload_offset` and same `stored_size`, and both `extract_bytes(path)` calls succeed with identical reconstructed DDS bytes. [VERIFIED: tests/unit/ba2_dx10_writer_tests.cpp; tests/unit/ba2_gnrl_reader_tests.cpp]

### Alternative lower-effort test path

If constructing a fully valid synthetic DX10 archive is verbose, mutate an existing generated fixture by changing one chunk offset to partially overlap another while preserving all other metadata. [VERIFIED: tests/unit/ba2_dx10_parser_tests.cpp already has `read_binary_file`, `write_binary_file`, and `overwrite_u32_le` helpers]

Risk: mutating an existing fixture requires accurate byte offsets for the target chunk fields; the synthetic builder is more explicit and less brittle once written. [ASSUMED]

## Risks and Edge Cases

- Do not reject exact duplicate non-empty spans; BA2 DX10 writer dedupe tests depend on duplicate chunk offsets being readable. [VERIFIED: tests/unit/ba2_dx10_writer_tests.cpp]
- Do not treat zero-length spans as overlapping; existing parser already rejects DX10 chunks where stored or raw size is zero, but matching GNRL helper semantics keeps the overlap primitive safe if reused later. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp; src/formats/ba2/ba2_gnrl_parser.cpp]
- Use UInt64 arithmetic and existing `span_fits_u64` before overlap comparison so `start + length` cannot wrap for accepted spans. [VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp; src/formats/ba2/ba2_gnrl_parser.cpp]
- Avoid moving this into shared parser primitives during the quick fix unless both GNRL and DX10 are updated together; a local duplicate matches the current file-local style and minimizes regression surface. [ASSUMED]

## Sources

- `AGENTS.md` — project constraints, TES5Edit boundary, test expectations. [VERIFIED]
- `.planning/STATE.md` and `.planning/PROJECT.md` — current milestone state and BA2 support context. [VERIFIED]
- `src/formats/ba2/ba2_dx10_parser.cpp` — target parser flow and missing accepted-span tracking. [VERIFIED]
- `src/formats/ba2/ba2_gnrl_parser.cpp` — accepted-span exact-duplicate/partial-overlap policy. [VERIFIED]
- `tests/unit/ba2_gnrl_reader_tests.cpp` — focused overlap reject/duplicate accept test pattern. [VERIFIED]
- `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp` — DX10 parser, malformed, and dedupe test locations. [VERIFIED]

## Assumptions Log

| # | Claim | Risk if Wrong |
|---|-------|---------------|
| A1 | Synthetic DX10 fixture construction may be less brittle than mutating generated fixture offsets. | Implementer might choose a slower path, but correctness target is unchanged. |
| A2 | Keeping overlap helpers local is lower risk than a quick shared-helper refactor. | Some duplication remains until a later cleanup. |
