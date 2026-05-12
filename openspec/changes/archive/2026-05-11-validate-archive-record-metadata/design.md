## Context

The BA2 GNRL and DX10 parsers validate name and directory hashes against filename-table paths, but their record readers currently treat the 4-byte extension field as disposable padding. The BA2 writers serialize that extension field as lookup metadata, so parsed entries should not be materialized when the stored record extension disagrees with the filename table.

TES4-family BSA folder records store offsets used by game/tool readers to locate folder blocks. The parser consumes folder blocks sequentially, so it must also verify each stored offset matches the expected folder-block location instead of accepting any in-range value.

## Goals / Non-Goals

**Goals:**
- Preserve BA2 record extension bytes in the parsed internal record structs.
- Reject BA2 GNRL entries whose record extension FourCC does not match the filename-table file extension.
- Reject BA2 DX10 entries whose record extension FourCC does not match the parsed texture filename extension.
- Reject TES4-family BSA folder records whose stored folder-block offset does not match the parsed table layout.
- Cover each mismatch with focused malformed archive tests that fail during open/validation with `format_error`.

**Non-Goals:**
- Do not change public archive metadata, writer APIs, or extraction APIs.
- Do not introduce new dependencies or modify the read-only `TES5Edit/` reference submodule.
- Do not relax existing hash, payload span, sentinel, or texture chunk validation.

## Decisions

- Store BA2 extension fields as `std::array<std::byte, 4>` in `gnrl_record` and `dx10_record` rather than converting them to strings in the record reader. This keeps the on-disk bytes intact, including NUL padding, and avoids accepting malformed display text before the filename table is parsed.
- Derive the expected BA2 extension FourCC during entry materialization, after filename-table paths have been separator-normalized and validated. GNRL should derive the extension from the filename-table file name using the same 1-4 printable ASCII plus NUL padding rule used by the GNRL writer; DX10 should derive it from the parsed texture extension used for the stem/name hash check.
- Return `libbsa::error_code::format_error` for extension or folder-offset mismatches. These are archive consistency failures discovered while parsing externally supplied bytes, not caller argument errors.
- Keep the extension encoder local to the BA2 parser files unless a later change needs the same helper in multiple modules. The smallest correct implementation avoids a new shared parser primitive for two format-specific checks.
- Validate TES4 stored folder offsets against the parser's expected folder-block position in both metadata validation and materialization paths. The compatibility rule is that the stored offset points to the current folder-block position adjusted by the file-name table placement used by the TES4 layout.

## Risks / Trade-offs

- Existing third-party archives with inconsistent BA2 extension fields will become rejected at open time. This is intentional because the record metadata is part of lookup behavior and inconsistent archives can resolve differently in BA2 consumers.
- Case handling can be format-sensitive. The implementation should follow each writer/parser path's existing normalized-vs-preserved filename behavior rather than adding broad case-insensitive comparison.
- TES4 offset validation must preserve the existing version-specific folder record widths and file-name-table offset adjustment. Tests should mutate only the stored offset so failures prove the new consistency check rather than a table-size side effect.
