# Phase 08: ba2-gnrl-write-new-support - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-09
**Phase:** 08-ba2-gnrl-write-new-support
**Areas discussed:** Starfield fields, API surface, Record metadata, Ordering policy

---

## Starfield Fields

### Writer Exposure

| Option | Description | Selected |
|--------|-------------|----------|
| Mixed defaults (Recommended) | Use deterministic profile defaults, but allow optional caller overrides for Unknown1/Unknown2/CompressionMethod when needed. | Yes |
| Preset only | No raw field knobs; libbsa chooses fixed target-profile values and documents them. | |
| Caller required | Require callers to supply Starfield raw fields explicitly for Starfield profiles. | |
| You decide | Planner/researcher picks the smallest API that satisfies SPEC and compatibility evidence. | |

**User's choice:** Mixed defaults (Recommended)
**Notes:** Starfield raw header fields should have deterministic defaults, but callers can override them when needed.

### V3 Method Shape

| Option | Description | Selected |
|--------|-------------|----------|
| Profile option (Recommended) | Keep one Starfield v3 target and set CompressionMethod through writer options, with default and override documented. | Yes |
| Separate targets | Expose distinct v3 deflate and v3 raw-LZ4 target profiles for more explicit construction. | |
| You decide | Planner chooses based on the cleanest public enum/options shape. | |

**User's choice:** Profile option (Recommended)
**Notes:** Do not split Starfield v3 into separate target profiles solely for compression method.

### Default Values

| Option | Description | Selected |
|--------|-------------|----------|
| Reference-derived (Recommended) | Researcher traces TES5Edit/fixtures/reference archives and locks deterministic constants with comments/tests. | Yes |
| Zero defaults | Use zero unless callers override; simplest, but may be less compatibility-aligned. | |
| Caller required | Starfield v2/v3 construction fails unless callers provide Unknown1/Unknown2 explicitly. | |
| You decide | Planner chooses after reference tracing, keeping the API minimal. | |

**User's choice:** Reference-derived (Recommended)
**Notes:** Defaults are compatibility facts to research and document.

### CompressionMethod Scope

| Option | Description | Selected |
|--------|-------------|----------|
| Archive-wide codec (Recommended) | CompressionMethod selects the codec for all compressed entries; per-entry override only chooses raw vs compressed. | Yes |
| Per-entry codec | Try to let entries choose deflate vs raw-LZ4 individually, even though BA2 v3 stores method at archive level. | |
| You decide | Planner chooses the safest metadata-driven behavior. | |

**User's choice:** Archive-wide codec (Recommended)
**Notes:** Per-entry overrides remain raw versus compressed; compressed entries use the archive-level method.

---

## API Surface

### Writer Shape

| Option | Description | Selected |
|--------|-------------|----------|
| Dedicated GNRL writer (Recommended) | Add a `ba2_gnrl_writer`-style object mirroring Phase 7, leaving DX10 writer design to Phase 9. | Yes |
| Generic BA2 writer | Add one broader BA2 writer shell now and select GNRL/DX10 through options later. | |
| You decide | Planner picks the smallest public surface that satisfies the SPEC. | |

**User's choice:** Dedicated GNRL writer (Recommended)
**Notes:** Keep Phase 8 focused on GNRL; DX10 writer design belongs to Phase 9.

### Compression Policy Enums

| Option | Description | Selected |
|--------|-------------|----------|
| Reuse policies (Recommended) | Use `archive_compression_policy` and `entry_compression_policy`; target/profile maps `compressed` to deflate or raw-LZ4. | Yes |
| BA2-specific policies | Create BA2-only compression policy enums to name BA2 methods more explicitly. | |
| You decide | Planner chooses the cleanest dependency-light API after checking compile-boundary tests. | |

**User's choice:** Reuse policies (Recommended)
**Notes:** Keep public compression controls aligned with Phase 7.

### Output Destination

| Option | Description | Selected |
|--------|-------------|----------|
| Host-path only (Recommended) | Finalize to a host path through `write_to`; defer memory/sink output until there is a concrete need. | Yes |
| Add memory output | Also return archive bytes from memory in Phase 8. | |
| You decide | Planner decides if BA2 needs a different destination shape. | |

**User's choice:** Host-path only (Recommended)
**Notes:** Memory-output and sink-output writer destinations stay out of scope.

### Duplicate Timing

| Option | Description | Selected |
|--------|-------------|----------|
| Write-time rejection (Recommended) | Match Phase 7: `add_*` accepts entries independently; `write_to` rejects duplicates with `format_error`. | Yes |
| Add-time rejection | Reject a duplicate as soon as it is added, giving earlier feedback. | |
| You decide | Planner picks the behavior that best preserves existing writer patterns. | |

**User's choice:** Write-time rejection (Recommended)
**Notes:** Match Phase 7 writer behavior.

---

## Record Metadata

### Extension Field

| Option | Description | Selected |
|--------|-------------|----------|
| Derive and validate (Recommended) | Derive from the archive path extension without the dot, pad as needed, and fail unsupported/invalid cases instead of guessing. | Yes |
| Derive and truncate | Always take the first four extension bytes, even when the source extension is longer. | |
| Per-entry override | Expose an advanced caller-supplied extension field on each entry. | |
| You decide | Planner chooses after tracing BA2 compatibility evidence. | |

**User's choice:** Derive and validate (Recommended)
**Notes:** No silent truncation and no public extension override by default.

### Record Flags

| Option | Description | Selected |
|--------|-------------|----------|
| Default plus override (Recommended) | Use a deterministic safe default, but expose an optional advanced per-entry record-flags override if needed. | Yes |
| Default only | Do not expose record flags in the writer API; writer chooses deterministic values. | |
| Caller required | Require callers to provide record flags for every entry. | |
| You decide | Planner chooses the smallest API after reference tracing. | |

**User's choice:** Default plus override (Recommended)
**Notes:** Keep normal API safe while preserving an advanced compatibility escape hatch.

### Hash Fields

| Option | Description | Selected |
|--------|-------------|----------|
| Writer computes (Recommended) | Compute name and directory hashes from archive paths using libbsa helpers; no public hash override. | Yes |
| Advanced override | Allow callers to supply raw hash fields for compatibility experiments. | |
| You decide | Planner decides based on reference needs and API safety. | |

**User's choice:** Writer computes (Recommended)
**Notes:** Callers should not supply raw BA2 hash fields.

### Public Metadata

| Option | Description | Selected |
|--------|-------------|----------|
| Existing fields only (Recommended) | Use `entry_metadata::archive_hash`, `record_flags`, offsets, sizes, compression, and path fields for verification. | Yes |
| Add BA2 metadata | Add a BA2-specific optional entry metadata object for extension/hash/flags details. | |
| You decide | Planner chooses if existing fields are insufficient after tracing writer needs. | |

**User's choice:** Existing fields only (Recommended)
**Notes:** Avoid expanding public entry metadata unless implementation proves it necessary.

---

## Ordering Policy

### Record and Name Order

| Option | Description | Selected |
|--------|-------------|----------|
| Reference order (Recommended) | Research TES5Edit/BSArchPro or known BA2 behavior; prefer hash/reference ordering, with deterministic canonical fallback if evidence is inconclusive. | Yes |
| Canonical path | Sort by canonical normalized archive path for simple deterministic output. | |
| Add order | Preserve the caller's entry addition order in records and filename table. | |
| You decide | Planner chooses after tracing compatibility evidence. | |

**User's choice:** Reference order (Recommended)
**Notes:** Compatibility evidence should drive ordering; use deterministic canonical fallback only if evidence is inconclusive.

### Filename Table Spelling

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve spelling (Recommended) | Preserve caller-provided spelling with separators normalized to `/`; canonical lowercase is only for validation and duplicate checks. | Yes |
| Canonical lowercase | Serialize normalized lowercase canonical paths into the filename table. | |
| You decide | Planner chooses based on existing reader behavior and compatibility evidence. | |

**User's choice:** Preserve spelling (Recommended)
**Notes:** Preserve display spelling while normalizing separators.

### Dedupe Placement

| Option | Description | Selected |
|--------|-------------|----------|
| First owned payload (Recommended) | Encode records in selected order; first eligible stored payload owns bytes, later duplicates share that offset. | Yes |
| Source add order | Use original add order to decide which duplicate owns the payload, even if records are sorted differently. | |
| You decide | Planner chooses the clearest rule that preserves reader-backed extraction. | |

**User's choice:** First owned payload (Recommended)
**Notes:** Dedupe ownership follows selected record order.

### Ordering Verification

| Option | Description | Selected |
|--------|-------------|----------|
| Structure plus roundtrip (Recommended) | Assert FileTableOffset after payloads, paired record/name order, offsets/dedupe metadata, and reopen/extract results; avoid full byte-golden archives. | Yes |
| Full byte golden | Assert entire writer-output bytes against golden fixtures. | |
| Roundtrip only | Only reopen/extract and byte-compare payloads, without checking physical ordering details. | |
| You decide | Planner chooses the smallest reliable ordering proof. | |

**User's choice:** Structure plus roundtrip (Recommended)
**Notes:** Tests should prove physical layout details without depending on full archive byte equality.

---

## the agent's Discretion

- Exact public type and method names remain planner discretion if the dedicated BA2 GNRL writer shape is preserved.
- Exact private file layout, CMake organization, and test file organization remain planner discretion.
- Exact synthetic test paths and payload bytes remain planner discretion if the SPEC and CONTEXT coverage matrix is satisfied.

## Deferred Ideas

None - discussion stayed within phase scope.
