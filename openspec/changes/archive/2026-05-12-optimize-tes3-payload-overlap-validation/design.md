## Context

`src/formats/bsa/tes3_bsa_parser.cpp` materializes TES3 entries in stored hash-table order. During that pass it validates hash ordering, duplicate stored hashes, hash/name agreement, canonical path uniqueness, data-section-relative payload offsets, archive bounds, and finally payload overlap. The overlap step currently keeps `payload_spans` in insertion order and scans every prior span for each entry, which is O(n^2) for large valid archives.

The TES5Edit reference confirms the important compatibility constraint: TES3 file record offsets are relative to the computed data section. `TES5Edit/Core/wbBSArchive.pas` records `fDataOffset` after the hash table, writes TES3 offsets as `Offset - fDataOffset`, and reads payloads from `fDataOffset + FileTES3.Offset`. The optimization must not reinterpret offsets or weaken the existing pre-overlap metadata validation.

## Goals / Non-Goals

**Goals:**

- Keep TES3 hash-order validation, duplicate-hash detection, hash/name validation, canonical path validation, data-section-relative offset handling, and archive-bound checks in their current order before overlap checking.
- Detect overlapping non-empty TES3 payload spans without scanning all previously materialized spans for every entry.
- Preserve public metadata, public APIs, dependencies, and existing error-code behavior.
- Add tests that prove adjacent sorted-span overlap rejection and large non-overlapping validation coverage.

**Non-Goals:**

- No change to TES3 hash computation, path normalization, extraction, writer ordering, or data-section offset interpretation.
- No new interval-tree dependency or shared parser abstraction unless another format needs it later.
- No attempt to validate or mutate bytes in `TES5Edit/`; it remains a read-only reference.

## Decisions

- Collect payload spans during materialization, then sort once by offset and validate adjacent spans. Sorting changes overlap validation from O(n^2) scans to O(n log n) sorting plus O(n) adjacency checks while keeping the implementation local to the TES3 parser.
- Validate hashes, names, canonical paths, offset arithmetic, and archive bounds before inserting a span. This preserves the current failure precedence for malformed archives where a hash-order or name/hash problem appears before a payload overlap problem.
- Treat only non-empty payloads as occupying archive bytes for overlap checks. Zero-byte TES3 entries are already present in repository fixtures and should not create artificial interval conflicts because they have no payload extent.
- Keep span storage as a simple local `std::vector` with existing metadata allocation handling. An ordered interval tree would add complexity without improving the dominant cost for a one-shot parse.

## Risks / Trade-offs

- Sorting spans means overlap errors are found after the materialization pass rather than at the first overlapping entry in hash order -> mitigate by preserving the same `format_error` code and stable diagnostic message for overlap failures.
- Zero-length span handling can expose accidental reliance on the old insertion-order overlap asymmetry -> mitigate with explicit tests for legal empty TES3 entries and overlapping non-empty entries.
- Synthetic performance coverage can become timing-sensitive -> prefer deterministic structural coverage, such as large non-overlapping generated metadata that would be expensive under quadratic scans, and avoid wall-clock assertions.

## Migration Plan

- Update TES3 parser overlap validation locally, keeping existing public metadata output unchanged.
- Extend TES3 reader tests with repository-owned synthetic bytes for overlapping non-empty spans and a larger non-overlapping archive shape.
- Run the focused TES3 reader tests and the broader unit/CTest suite available in the configured build.
- Rollback is a single parser/test revert because no persisted data format, public API, or dependency changes are introduced.

## Open Questions

- None.
