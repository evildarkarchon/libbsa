# Phase 09: ba2-dx10-write-new-support - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-09
**Phase:** 09-ba2-dx10-write-new-support
**Areas discussed:** DX10 writer surface, Chunk controls, DX10 metadata knobs, Dedupe granularity, DDS proof matrix

---

## DX10 Writer Surface

| Question | Options Presented | User's Choice | Notes |
|----------|-------------------|---------------|-------|
| How closely should the BA2 DX10 public writer mirror the existing BA2 GNRL writer shape? | Mirror GNRL; Texture-specialized; Shared BA2 facade | Mirror GNRL | Dedicated writer shape should follow existing BA2 GNRL pattern. |
| When should DDS host-file validation and DirectXTex analysis happen for `add_dds_file` entries? | Defer to write; Validate at add; You decide | Validate at add | DDS source failures should be surfaced before finalization. |
| Because validate-at-add was selected, should the writer snapshot DDS bytes when a file is added or keep only the host path? | Snapshot bytes; Path plus metadata; Analyze twice | Snapshot bytes | Writer owns validated DDS bytes after add. |
| What per-entry options should the public DX10 writer expose when adding a DDS file? | Compression only; Compression plus chunks; Full texture knobs; You decide | Free-text correction | User said DX10 BA2 archives can only be archive-level compressed because uncompressed DX10 archives are game-engine unstable. |
| How should Phase 9 treat the compression override requirement in light of DX10 engine instability? | Correct SPEC; Keep SPEC; Internal tests only | Correct SPEC | `09-SPEC.md` Requirement 7 must be corrected or superseded before planning. |

---

## Chunk Controls

| Question | Options Presented | User's Choice | Notes |
|----------|-------------------|---------------|-------|
| What public chunk-planning control should Phase 9 expose? | Max bytes; Max mips; Named policies; You decide | Max bytes | One archive-wide decoded-byte limit. |
| Should chunk planning be configured archive-wide only, or can individual DDS entries override it? | Archive-wide only; Per-entry overrides; Both levels | Archive-wide only | Keeps public surface small and consistent with archive-level compression. |
| What should the default chunking strategy be when callers do not set a max-byte limit? | Reference default; One chunk; One mip each; You decide | Reference default | Researcher/planner must trace reference behavior first. |
| For array textures and cubemaps, should every slice or face use the same mip-range chunk pattern? | Uniform pattern; Optimize per face; You decide | Uniform pattern | Same mip-range split repeats per slice/face. |

---

## DX10 Metadata Knobs

| Question | Options Presented | User's Choice | Notes |
|----------|-------------------|---------------|-------|
| After the SPEC correction, what archive-level compression choices should the public DX10 writer expose? | Compressed only; Archive policy; Internal raw only | Compressed only | Public DX10 writer does not expose raw output. |
| How should Starfield BA2 DX10 header fields be exposed? | Mirror GNRL; Defaults only; Compression only; You decide | Mirror GNRL | Defaults plus valid archive-level overrides for Starfield fields. |
| Should the BA2 DX10 record `unknown_tex` byte be caller-overridable? | Default only; Entry override; Target default | Default only | Writer owns this value; reference trace should lock default. |
| How should cubemap and array record fields be determined? | Derive only; Advanced override; You decide | Derive only | DirectXTex metadata and BA2 constants determine fields. |

---

## Dedupe Granularity

| Question | Options Presented | User's Choice | Notes |
|----------|-------------------|---------------|-------|
| When DX10 dedupe is enabled, where may identical chunks share payload offsets? | Archive-wide; Between entries only; Same texture only | Archive-wide | Any eligible chunk in the archive may share. |
| What should make a DX10 chunk eligible to share a payload offset? | Stored plus metadata; Stored bytes only; Decoded bytes | Stored plus metadata | Eligibility includes stored bytes, raw/stored sizes, and compression route. |
| Which chunk should own the physical bytes when multiple eligible chunks dedupe together? | First in order; Smallest offset sort; You decide | First in order | Deterministic record/chunk order owns bytes first. |
| What public dedupe control should the DX10 writer expose? | Boolean option; Policy enum; Always off | Boolean option | Archive-level `deduplicate_payloads`, disabled by default. |
| Should dedupe run after chunk planning and archive-level compression, or can it influence chunk planning? | After encoding; Before compression; Dedupe-aware planning | After encoding | Chunk planning remains predictable. |
| If two DDS entries are identical, should dedupe allow all matching chunks to share offsets while keeping both entries distinct? | Yes; Partial only; No whole-file sharing | Yes | Duplicate DDS entries may share all eligible chunk offsets. |
| Should dedupe eligibility require matching texture semantics beyond stored bytes and chunk sizes? | Chunk metadata only; Texture semantics too; You decide | Chunk metadata only | Texture dimensions, DXGI format, cubemap/array state, and mip range need not match for sharing. |
| What dedupe behavior should tests make mandatory? | Enabled and disabled; Enabled only; Reader proof only | Enabled and disabled | Tests must prove distinct offsets when disabled and shared offsets when enabled. |

---

## DDS Proof Matrix

| Question | Options Presented | User's Choice | Notes |
|----------|-------------------|---------------|-------|
| How should the locked DDS format set be distributed across FO4 and Starfield v3 writer tests? | Split across targets; Every format both; FO4 all, SF sample | Split across targets | Every locked format appears at least once across both targets. |
| How should mip, array, and cubemap structural coverage be represented in the writer fixtures? | Dedicated cases; One mega texture; Every format complex | Dedicated cases | Use focused structural fixtures. |
| How should generated DDS source fixtures be managed? | Committed fixtures; Runtime only; Hybrid | Committed fixtures | Generated legal DDS inputs and manifests are committed outside `TES5Edit/`. |
| What should count as success for extracted DDS output from writer-produced archives? | Metadata plus payload; Full byte equality; Metadata only | Metadata plus payload | DirectXTex metadata and image payload bytes must match; headers may canonicalize. |

---

## the agent's Discretion

- Exact public type and method names are left to researcher/planner as long as they mirror the BA2 GNRL writer pattern and preserve the decisions in CONTEXT.md.
- Exact private file layout, helper decomposition, test file organization, fixture dimensions, fixture names, and format distribution are left to researcher/planner within the locked matrix.

## Deferred Ideas

None. The SPEC Requirement 7 correction is in-scope and should be resolved before planning.
