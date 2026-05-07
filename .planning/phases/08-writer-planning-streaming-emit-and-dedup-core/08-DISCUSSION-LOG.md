# Phase 08: writer-planning-streaming-emit-and-dedup-core - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-06T17:07:47.4910486-07:00
**Phase:** 08-writer-planning-streaming-emit-and-dedup-core
**Areas discussed:** Writer API Shape, Plan Preview Model, Harness Proof Shape, Dedup Semantics

---

## Writer API Shape

### API use model

| Option | Description | Selected |
|--------|-------------|----------|
| Plan then finalize | Free functions such as plan_archive_write(...) then finalize_archive_write(plan, sink); mirrors current operation-style APIs and keeps lifetimes explicit. | yes |
| Writer object | A stateful archive_writer that accumulates entries and finalizes later; convenient, but introduces ownership/lifetime questions. | |
| Builder facade | A fluent builder that produces an immutable plan; nicer ergonomics but more public surface in the foundation phase. | |
| You decide | Let downstream agents choose the smallest API that satisfies SPEC.md and existing public API patterns. | |

**User's choice:** Plan then finalize.
**Notes:** Locks an operation-style plan/finalize flow rather than a stateful writer.

### Writer input entries

| Option | Description | Selected |
|--------|-------------|----------|
| Value entry structs | Public write_entry values with normalized path, in-memory bytes, and per-entry compression policy; straightforward and caller-owned. | yes |
| Source callbacks | Entries read from byte_source/callbacks during finalization; more streaming input, but outside the SPEC's in-memory payload boundary. | |
| Separate add calls | Entries added one at a time before planning; more ergonomic for builders, but couples this to stateful writer shape. | |
| You decide | Let agents choose entry shape within the locked in-memory-input requirement. | |

**User's choice:** Value entry structs.

### Target descriptor

| Option | Description | Selected |
|--------|-------------|----------|
| Capability descriptor | A libbsa-owned target descriptor declares format, version-like identity, archive default compression, and dedup/compression support; good for harness now and real writers later. | yes |
| Archive format enum only | Simpler, but likely too little information for default compression, dedup capability, and future BSA/BA2 writer layouts. | |
| Test-only target details | Keep public API abstract and expose harness target only in tests; reduces public surface but weakens consumer smoke coverage. | |
| You decide | Let agents infer target metadata from SPEC.md and existing archive_summary/compression types. | |

**User's choice:** Capability descriptor.

### Validation timing

| Option | Description | Selected |
|--------|-------------|----------|
| Planning rejects all | plan_archive_write validates normalized paths, duplicate normalized paths, unsupported target/options, and size arithmetic before any sink is touched. | yes |
| Split validation | Plan catches structural issues, finalize catches layout/sink issues; practical but risks consumers seeing a plan that cannot finalize. | |
| Finalize only | Smallest planning API, but makes layout preview less trustworthy and conflicts with no-partial-success semantics. | |
| You decide | Let agents choose exact validation split while preserving structured failures. | |

**User's choice:** Planning rejects all.

---

## Plan Preview Model

### Preview shape

| Option | Description | Selected |
|--------|-------------|----------|
| Regions plus entries | Expose table/index regions, data regions, per-entry layout records, total size, and region references; directly matches SPEC layout-preview acceptance. | yes |
| Entries only | Simpler API with path/offset/size/compression per entry, but table/data layout decisions are less inspectable. | |
| Opaque plan summary | Smallest public surface, but too weak for downstream consumers to inspect deterministic layout choices. | |
| You decide | Let agents choose preview fields from SPEC.md acceptance criteria. | |

**User's choice:** Regions plus entries.

### Ordering contract

| Option | Description | Selected |
|--------|-------------|----------|
| Sorted vectors | Expose deterministic vectors for planned entries and regions; order is part of the preview contract and test assertions. | yes |
| Lookup maps | Easy path lookup, but ordering can be less explicit and less useful for byte-layout previews. | |
| Both vectors and lookup | Convenient but larger Phase 8 surface; could be added later if consumers need it. | |
| You decide | Let agents choose exact container shape while preserving deterministic plan metadata. | |

**User's choice:** Sorted vectors.

### Offset semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Archive-absolute offsets | Use final stream offsets for every planned region/entry, matching existing entry_metadata semantics and read-back tests. | yes |
| Region-relative offsets | Can be useful internally but makes consumer previews less directly comparable to final bytes. | |
| Both absolute and relative | Very explicit, but may overfit the foundation phase before real format writers arrive. | |
| You decide | Let agents select offset semantics as long as tests assert exact layout. | |

**User's choice:** Archive-absolute offsets.

### Planned payload ownership

| Option | Description | Selected |
|--------|-------------|----------|
| Own packed regions | Plan owns post-policy payload bytes per data region, enabling deterministic finalize and post-policy dedup without re-running compression. | yes |
| Metadata only | Less memory, but finalize must recompress/reprocess and the preview may drift from emitted bytes. | |
| Implementation-private cache | Hide payload storage while plan remains public; harder to reason about copy/move and deterministic read-back. | |
| You decide | Let agents balance memory and determinism within SPEC.md's in-memory input boundary. | |

**User's choice:** Own packed regions.

---

## Harness Proof Shape

### Harness format identity

| Option | Description | Selected |
|--------|-------------|----------|
| Test-only generic format | A minimal deterministic libbsa harness format proves plan/finalize/read-back without suggesting BSA/BA2 writer compatibility. | yes |
| Minimal BSA-like format | Closer to Phase 9, but risks making Phase 8 look like partial production BSA writer support. | |
| Minimal BA2-like format | Closer to Phase 10, but overweights BA2 before shared writer foundations are stable. | |
| You decide | Let agents design the safest validation seam from SPEC.md. | |

**User's choice:** Test-only generic format.

### Harness validator location

| Option | Description | Selected |
|--------|-------------|----------|
| Tests only | Keep harness parsing/validation in test helpers so public API does not expose a fake archive format. | yes |
| Private src detail | Reusable for tests and maybe future internals, but can blur the boundary with production code. | |
| Public validation API | Consumer-visible, but likely out of scope because harness archives are not real Bethesda formats. | |
| You decide | Let agents choose organization while avoiding fake public archive support. | |

**User's choice:** Tests only.

### Read-after-write comparison

| Option | Description | Selected |
|--------|-------------|----------|
| Metadata plus bytes | Reopen/parse harness output, compare planned path order, offsets, sizes, compression/dedup references, total size, and extracted payload bytes. | yes |
| Bytes only | Simple golden output checks, but weaker for proving plan metadata matches emitted layout. | |
| Metadata only | Good layout proof, but misses extraction/round-trip payload correctness. | |
| You decide | Let agents select assertions that satisfy the SPEC acceptance criteria. | |

**User's choice:** Metadata plus bytes.

### Harness fixture storage

| Option | Description | Selected |
|--------|-------------|----------|
| Generated in tests | Use source-reviewable builders and assert deterministic bytes/metadata without committing binary fixtures. | yes |
| Committed golden bytes | Strong byte-regression checks, but adds binary fixture maintenance before production formats exist. | |
| Both generated and golden | Maximum coverage, but heavier than needed for the writer-core foundation. | |
| You decide | Let agents decide fixture storage as long as deterministic behavior is proven. | |

**User's choice:** Generated in tests.

### Malformed coverage boundary

| Option | Description | Selected |
|--------|-------------|----------|
| Writer-focused negatives | Cover malformed/invalid writer inputs and emitted-layout validation enough to prove writer safety; leave broad parser fuzzing to Phase 11. | yes |
| Full malformed reader matrix | Exhaustive harness parser corruption coverage, but shifts Phase 8 toward building a fake format reader. | |
| Happy path only | Fastest, but misses SPEC-required structured failures and read-after-write trust. | |
| You decide | Let agents choose negative coverage boundaries from SPEC.md. | |

**User's choice:** Writer-focused negatives.

### Harness structure

| Option | Description | Selected |
|--------|-------------|----------|
| Header/table/data regions | Include a small header, entry table, data-region table, and payload area so preview fields map to real emitted bytes. | yes |
| Flat path plus payload list | Simpler serialization, but weakly exercises table/data region planning. | |
| Real-family-inspired records | Exercises more future concepts, but may blur Phase 9/10 boundaries. | |
| You decide | Let agents define minimal records that prove all plan fields. | |

**User's choice:** Header/table/data regions.

### Harness compression

| Option | Description | Selected |
|--------|-------------|----------|
| Use real codecs | Plan stores post-policy packed regions using existing compress_payload routes, and read-back decompresses through existing codec dispatch. | yes |
| Mock compression markers | Simpler and faster, but does not prove writer compression policy integration. | |
| Raw only | Good for layout, but fails the SPEC acceptance around forced/default compression states and stored sizes. | |
| You decide | Let agents decide exact codec matrix while preserving route-confusion coverage. | |

**User's choice:** Use real codecs.

### Harness helper reuse

| Option | Description | Selected |
|--------|-------------|----------|
| Reusable test helpers | Keep them test-only but organized as shared test utilities so BSA/BA2 writer phases can reuse planning/finalize assertions. | yes |
| Local Phase 8 tests only | Smallest now, but likely creates duplicate helper code in later writer phases. | |
| Production reusable core | Reusable, but risks turning test harness behavior into library implementation surface. | |
| You decide | Let agents decide helper organization based on CMake/test patterns. | |

**User's choice:** Reusable test helpers.

---

## Dedup Semantics

### Dedup comparison identity

| Option | Description | Selected |
|--------|-------------|----------|
| Post-policy bytes | Dedup only after compression policy is resolved and packed bytes are produced, matching the locked SPEC wording. | yes |
| Original input bytes | Pre-policy comparison; this would conflict with SPEC.md unless SPEC is revised. | |
| Path plus bytes | Conservative, but prevents sharing identical payloads across different archive paths. | |
| You decide | Let agents choose comparison identity from SPEC.md. | |

**User's choice:** Post-policy bytes.
**Notes:** User initially selected original input bytes by mistake, then corrected after the SPEC conflict was identified.

### Dedup visibility

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit region refs | Each entry has a data_region_id/reference, and shared regions are visible through region records and offsets. | yes |
| Boolean only | Expose whether an entry is deduped, but hide sharing topology; less useful for layout inspection. | |
| Internal only | Dedup affects bytes but not preview API; conflicts with SPEC acceptance around dedup references. | |
| You decide | Let agents choose exact field names while exposing enough for tests. | |

**User's choice:** Explicit region refs.

### Unsupported target behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Structured failure | Planning returns unsupported_format/invalid option failure; no silent downgrade so callers know dedup was not applied. | yes |
| Silently disable | Easier for consumers, but surprising and weakens acceptance tests for unsupported combinations. | |
| Warn in plan | Requires a warning model not established yet; may be useful later but adds surface now. | |
| You decide | Let agents choose behavior from SPEC.md and existing result patterns. | |

**User's choice:** Structured failure.

### Region identity

| Option | Description | Selected |
|--------|-------------|----------|
| Stable numeric IDs | Assign deterministic data_region_id values in sorted plan order; entry records reference those IDs and offsets. | yes |
| Hash strings | Visible identity maps to content, but exposes implementation details and collision policy questions. | |
| Pointer/object refs | Convenient in C++, but not stable or inspectable in value-oriented public metadata. | |
| You decide | Let agents choose representation while keeping deterministic preview assertions. | |

**User's choice:** Stable numeric IDs.

### Disabled dedup behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Distinct regions | Every entry gets its own data region and offset even if bytes match, proving the option changes layout as SPEC requires. | yes |
| Shared internally | Keep output compact but hide it from preview; violates disabled-mode acceptance. | |
| Implementation discretion | Let agents optimize duplicate handling, but this weakens deterministic test expectations. | |
| You decide | Let agents infer disabled behavior from SPEC.md. | |

**User's choice:** Distinct regions.

### Public hash exposure

| Option | Description | Selected |
|--------|-------------|----------|
| No public hash | Expose sharing through region IDs only; keep hashing/collision mechanics private and avoid accidental API commitments. | yes |
| Public hash field | Useful diagnostics, but commits to a hash algorithm/collision story before needed. | |
| Optional debug hash | Middle ground, but adds surface not required by SPEC. | |
| You decide | Let agents decide if internal hash implementation needs exposure. | |

**User's choice:** No public hash.

### Compression failure interaction

| Option | Description | Selected |
|--------|-------------|----------|
| Fail before dedup | Resolve/compress each entry first; if any route fails, planning fails and no dedup grouping is produced. | yes |
| Dedup raw first | Can avoid duplicate compression work, but must not change post-policy dedup semantics; riskier to specify publicly. | |
| Partial plan with errors | Potentially informative, but no partial-success model exists in current result API. | |
| You decide | Let agents optimize internally while preserving structured failure behavior. | |

**User's choice:** Fail before dedup.

### Path normalization interaction

| Option | Description | Selected |
|--------|-------------|----------|
| Only for uniqueness | Normalize paths to reject duplicate entries, but dedup grouping is based on post-policy payload bytes, not names. | yes |
| Include path in dedup key | Avoids cross-path sharing, but contradicts identical-payload sharing semantics. | |
| Ignore path validation | Dedup payloads only and let later steps handle path issues; risks bad plans. | |
| You decide | Let agents choose as long as duplicate normalized paths fail. | |

**User's choice:** Only for uniqueness.

---

## the agent's Discretion

No selected decision area was left to the agent's discretion.

## Deferred Ideas

None.
