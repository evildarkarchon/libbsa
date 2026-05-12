## Context

`src/formats/ba2/ba2_gnrl_parser.cpp` has separate in-memory and host-file parsing paths. The in-memory path converts `FileTableOffset` to `std::size_t` and uses `detail::add_fits` when computing the parsed filename table end. The host-file path supports large archives by reading exactly `file_count` length-prefixed names from `FileTableOffset`, but it currently computes `name_table_end` with unchecked `std::uint64_t` addition before passing the span to `materialize_entries` for payload overlap validation.

Each individual host-file name read is already bounded against `archive_size`, but the aggregate end value is still archive-derived metadata. It should use the same checked-arithmetic pattern as other parser table and span calculations before the end value becomes input to overlap checks.

## Goals / Non-Goals

**Goals:**

- Use checked `std::uint64_t` addition for the BA2 GNRL host-file filename table end calculation.
- Return `libbsa::error_code::format_error` before materializing entries if `FileTableOffset + name_table_consumed` is not representable.
- Keep the existing in-memory parsing behavior intact, including its `std::size_t` `detail::add_fits` guard.
- Add focused malformed coverage around the high-offset aggregate filename table end case.

**Non-Goals:**

- Change public archive metadata types, reader APIs, or validation APIs.
- Change BA2 GNRL filename table parsing, name normalization, hash validation, extension validation, compression routing, or extraction behavior.
- Refactor unrelated local checked-arithmetic helpers in writer/layout code.
- Add new dependencies or compression/texture behavior.

## Decisions

- Reuse `detail::add_fits_u64` in `ba2_gnrl_parser.cpp` instead of introducing a BA2-local helper.
  - Rationale: parser primitives already expose a documented `std::uint64_t` checked-add helper for archive-derived byte counts, and BA2 DX10 parser code already follows that pattern.
  - Alternative considered: inline `max - lhs` at the call site. This would be smaller locally but duplicates a common overflow pattern and makes future parser audits harder.

- Validate the aggregate end immediately after `read_names_from_file` returns `name_table_consumed`.
  - Rationale: this is the first point where both operands are available, and it ensures `materialize_entries` receives only a valid `[FileTableOffset, name_table_end)` range.
  - Alternative considered: move the end calculation into `materialize_entries`. That would blur parsing and metadata materialization responsibilities and force callers that already have a validated span to repeat validation.

- Preserve the existing `format_error` classification for overflow.
  - Rationale: the failure is malformed archive metadata, matching existing parser behavior for oversized record tables, out-of-range spans, and invalid filename table data.
  - Alternative considered: report `io_error` because the host-file path reads from disk. The disk reads already succeeded; the invalid condition is the archive's declared and parsed layout.

- Keep the successful high-offset sparse archive behavior unchanged.
  - Rationale: the current bounded-open behavior allows payload bytes before an end filename table and avoids reading sparse payload data. The fix should reject only unrepresentable aggregate ranges.
  - Alternative considered: derive the filename table span from the first payload offset. That would regress archives where payloads legally appear before the filename table.

## Risks / Trade-offs

- The exact overflow trigger may be difficult to encode as a practical filesystem fixture on Windows if the required sparse file size approaches platform limits. Mitigation: prefer a focused sparse-file malformed test when feasible and add direct checked-add coverage if filesystem limits prevent a full fixture.
- Reusing `add_fits_u64` requires adding the corresponding `using detail::add_fits_u64;` import in the parser. Mitigation: keep the edit localized to `ba2_gnrl_parser.cpp` and avoid broad checked-arithmetic refactors.
- The new guard changes malformed archive behavior from wrapped overlap validation to early failure. Mitigation: assert only `format_error` and keep diagnostics family-specific but not overly coupled to exact wording unless existing tests already require it.
