# Phase 03: Format Detection and TES4-Family BSA Read/Extract - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-08
**Phase:** 03-format-detection-and-tes4-family-bsa-read-extract
**Areas discussed:** Public reader surface, Path display policy, Extraction contract, Fixture proof strategy

---

## Public Reader Surface

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| Primary public surface after successful open | Methods on reader; Separate view objects; Iterator-first API | Methods on reader |
| Archive-level metadata exposure | Required fields only; Extensible union now; String map details | Required fields only |
| Entry listing return shape | Entry metadata values; Paths only; Live entry handles | Entry metadata values |
| Valid-but-missing lookup behavior | `result<std::optional<entry>>`; Error for missing; `contains` only | `result<std::optional<entry>>` |

**Notes:** User selected the recommended minimal public surface centered on the existing `archive_reader` facade.

---

## Path Display Policy

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| Public path fields | Canonical plus original; Canonical only; Original only | Canonical plus original |
| Duplicate canonical paths | Fail as format error; First wins; Expose duplicates | Fail as format error |
| Deterministic listing order | Canonical path order; Archive record order; Hash/index order | Canonical path order |
| Public field names | `path` and `original_path`; `key` and `display_path`; Planner decides | `path` and `original_path` |
| Archives without usable names | Fail open/read as unsupported; Open with hash-only entries; Planner decides | Fail open/read as unsupported |
| Original path literalness | Archive spelling, joined predictably; Exact raw bytes decoded; Same as canonical | Archive spelling, joined predictably |
| Lookup input policy | Normalize all lookup input; Canonical input only; Separate exact lookup | Normalize all lookup input |
| Invalid lookup path error | `invalid_argument`; `format_error`; `unsupported` | `invalid_argument` |

**Notes:** User chose to keep canonical paths as identity and original paths as display/source metadata only.

---

## Extraction Contract

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| Primary extraction target | Sink interface first; Return `std::vector<std::byte>`; Extract to host path | Sink interface first |
| Memory convenience helper | Yes, bounded convenience; No, sink only; Planner decides | Yes, bounded convenience |
| Extraction identifier | By path string first; By entry metadata token; Both immediately | By path string first |
| Partial sink acceptance | `io_error` failure; Retry remaining bytes; Treat as success | `io_error` failure |
| Compressed entry buffering | One-entry bounded buffer; Fully streaming decompression; Planner decides | One-entry bounded buffer |
| Decompression failure code | `format_error`; `io_error`; Variant-specific code | `format_error` |
| Embedded-name metadata | Expose flag and prefix size; Hide entirely; Expose embedded name text | Expose flag and prefix size |
| Filesystem extraction convenience | No, defer filesystem output; Yes, single-file only; Yes, bulk extract | No, defer filesystem output |
| Missing path extraction error | Add `not_found` code; Use `invalid_argument`; Require lookup first | Add `not_found` code |
| Sink synchrony | Synchronous only; Allow async sinks; Planner decides | Synchronous only |
| Progress/cancellation hooks | No, defer hooks; Cancellation only; Progress and cancellation | No, defer hooks |
| Method naming | Simple verbs; Explicit destination names; Planner decides | Simple verbs |

**Notes:** User chose correctness-first synchronous extraction with bounded one-entry buffering and no filesystem/progress/cancel scope.

---

## Fixture Proof Strategy

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| Default proof shape | Generated archives plus JSON manifests; Inline byte arrays only; Local game archives | Generated archives plus JSON manifests |
| Success fixture count | One per variant minimum; One combined fixture; Many per variant | One per variant minimum |
| Required success cases | Raw, compressed, embedded; Minimal one-file archives; Planner decides | Raw, compressed, embedded |
| Fixture generator commitment | Commit generator code; Binary plus README recipe; Planner decides | Commit generator code |
| Malformed fixture coverage | Focused malformed set; Only non-BSA rejection; Exhaustive fuzz corpus | Focused malformed set |
| BSArchPro comparisons | Optional supplement; Required gate; Not in Phase 3 | Optional supplement |
| Generator location | Tests fixture tools; Production src; External script only | Tests fixture tools |
| JSON dependency | No new dependency; Add JSON library; Planner decides | Add JSON library |
| JSON dependency boundary | Test-only exception; Research first; Runtime dependency allowed | Test-only exception |

**Notes:** User intentionally selected a documented test-only JSON dependency exception for manifest parsing. It must not leak into runtime or public headers.

---

## Agent Discretion

- Exact internal parser file/class names and handler registration mechanics.
- Exact JSON library choice after research, constrained to test/tool-only use.
- Exact public type names where not explicitly locked, constrained by the decisions in `03-CONTEXT.md`.

## Deferred Ideas

None.
