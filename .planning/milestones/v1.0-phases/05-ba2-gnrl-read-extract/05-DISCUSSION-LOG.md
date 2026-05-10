# Phase 05: BA2 GNRL Read/Extract - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-08
**Phase:** 05-ba2-gnrl-read-extract
**Areas discussed:** Metadata surface, Fixture matrix, V3 method policy, DX10 rejection

---

## Metadata Surface

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| How should Phase 5 expose Starfield BA2 version-specific header fields? | Typed optional struct | Flat optional fields; internal preserve only; agent decides |
| What naming style should that typed BA2 metadata use for currently unknown Starfield fields? | Keep raw names | Neutral raw fields; semantic guesses; agent decides |
| How should the BA2 metadata distinguish field presence across Fallout 4, Starfield v2, and Starfield v3? | Version-gated optionals | Zero default values; variant-specific structs; agent decides |
| Should Phase 5 add any BA2-specific entry metadata beyond the existing `entry_metadata` fields? | No unless required | Expose raw BA2 record; manifest-only details; agent decides |

**Notes:** The public API should expose a small typed optional BA2 metadata object, keep parser structs private, retain raw Starfield field names, and avoid extra entry metadata unless reference tracing proves the existing fields are insufficient.

---

## Fixture Matrix

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| What should the Phase 5 success fixture shape be? | One per variant | One rich archive; full cross-product; agent decides |
| How much behavior should each success fixture pack in? | Edge-heavy fixtures | Compact representative; minimal happy path; agent decides |
| What malformed BA2 coverage should be mandatory rather than deferred to Phase 11? | Focused BA2 parser failures | Only open blockers; broad malformed suite; agent decides |
| What should BA2 fixture manifests record for downstream tests? | Rich metadata manifest | Consumer-only manifest; raw-structure manifest; agent decides |
| Should BA2 fixture generation reuse existing BSA generator style or split into a dedicated BA2 generator? | Dedicated BA2 generator | Shared mega-generator; agent decides |
| Which valid edge cases should the edge-heavy BA2 success fixtures deliberately include? | Paths and metadata | Compression-focused; path-focused; agent decides |
| Should Phase 5 require optional local real-game BA2 validation, or keep it generated-only? | Generated-only gate | Optional local tests; require local validation; agent decides |
| How strict should success fixture test assertions be? | Metadata and bytes | Bytes only; full raw structure; agent decides |

**Notes:** The fixture set should include one generated legal success archive per required variant, be edge-heavy for paths and metadata, include focused malformed/unsupported BA2 cases, and assert both public metadata and exact extracted bytes.

---

## V3 Method Policy

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| How should Phase 5 map Starfield v3 `CompressionMethod` for compressed GNRL entries? | 3 LZ4, others deflate if observed | 3 LZ4, all others deflate; only method 3; agent decides |
| What should happen for a v3 `CompressionMethod` value that is not supported by reference evidence? | Fail as unsupported | Fail as format_error; list but fail extract; agent decides |
| When should unsupported v3 compression methods be rejected? | During open | During extraction; only if compressed entries exist; agent decides |
| Should v3 compression routing ever infer behavior from file extension or BA2 record extension fields? | Never infer | Only as fallback; agent decides |

**Notes:** Method `3` is raw LZ4 block. Other methods may route to deflate only when TES5Edit/BSArchPro evidence supports them. Unknown methods fail `open` as unsupported, and routing must never infer from extensions.

---

## DX10 Rejection

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| When Phase 5 sees valid BA2 `BTDX` bytes with archive type `DX10`, what should `archive_reader::open` do? | Return unsupported | Parse minimal metadata; return format_error; agent decides |
| How much should Phase 5 inspect before rejecting DX10? | Header only | Validate records too; magic only; agent decides |
| Should generated Phase 5 fixtures include a DX10 rejection case? | Yes, tiny header fixture | No, defer to Phase 6; full tiny DX10 archive; agent decides |
| How should Phase 5 distinguish unsupported BA2 profiles from malformed BA2 bytes? | Supported type strictness | Everything non-GNRL unsupported; everything BA2-like format_error; agent decides |

**Notes:** Phase 5 should only classify DX10 by fixed header, return unsupported, include a tiny DX10 rejection fixture, and leave all texture record/chunk parsing for Phase 6.

---

## the agent's Discretion

- Exact internal file names, parser structs, helper boundaries, detector dispatch shape, and CMake/test organization are left to researcher/planner.
- Exact fixture filenames and payload strings are left to planner, provided the generated set proves the decisions captured in CONTEXT.md.

## Deferred Ideas

None - discussion stayed within phase scope.
