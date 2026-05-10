# Phase 06: DDS Boundary and BA2 DX10 Read/Reconstruction - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-08
**Phase:** 06-dds-boundary-and-ba2-dx10-read-reconstruction
**Areas discussed:** Texture metadata surface, DDS output contract, Fixture coverage shape, Analyzer boundary reuse

---

## Area Selection

| Option | Description | Selected |
|--------|-------------|----------|
| Texture metadata surface | How dimensions, format identifiers, cubemap/array state, and chunk layout appear in libbsa-owned public/documented types. | yes |
| DDS output contract | Reconstructed DDS strictness, DirectXTex-loadable expectations, header choices, and failure behavior. | yes |
| Fixture coverage shape | Generated DX10 fixture breadth across variants, compression, cubemap/array, malformed cases, and manifest detail. | yes |
| Analyzer boundary reuse | Whether DirectXTex/DDS analyzer work stays minimal or becomes reusable for Phase 9. | yes |

---

## Texture Metadata Surface

| Question | Selected | Alternatives Presented |
|----------|----------|------------------------|
| Where should BA2 DX10 texture metadata live in the public surface? | Entry optional (Recommended) | Entry optional; Separate method; You decide |
| How should libbsa expose the texture format value without exposing DXGI or DirectXTex types? | Raw DXGI id (Recommended) | Raw DXGI id; libbsa enum; Raw plus helper |
| How much BA2 DX10 chunk layout should be visible to consumers? | Public chunk list (Recommended) | Public chunk list; Chunk count only; Private layout |
| How should cubemap and array state be represented in the libbsa-owned metadata? | Explicit fields (Recommended) | Explicit fields; Layout enum; Header mirror |
| Should the optional texture metadata be populated only for BA2 DX10 entries? | DX10 only (Recommended) | DX10 only; Future texture files; You decide |
| What should `entry_metadata::raw_size` mean for BA2 DX10 entries once extracted output includes a reconstructed DDS header? | DDS output size (Recommended) | DDS output size; Payload bytes only; Separate fields |
| Should texture metadata preserve archive-derived raw record fields that do not yet have clear semantics? | Raw mirrors (Recommended) | Known fields only; Raw mirrors; Manifest only |
| How conservative should the public texture metadata API be for future compatibility? | Minimal public (Recommended) | Add now, pre-v1; Private first; Minimal public |

**Notes:** User chose the recommended dependency-light entry-level surface throughout. No separate texture API was requested.

---

## DDS Output Contract

| Question | Selected | Alternatives Presented |
|----------|----------|------------------------|
| What DDS header strategy should Phase 6 use for reconstructed BA2 DX10 output? | Always DXT10 (Recommended) | Always DXT10; Legacy when possible; You decide |
| What should tests require from reconstructed DDS bytes? | Metadata match (Recommended) | Metadata match; Byte exact fixtures; Metadata plus prefix |
| When should malformed DX10 layout problems fail? | Fail open (Recommended) | Fail open; Fail extraction; Mixed policy |
| How should chunk payloads be ordered into reconstructed DDS output? | Computed DDS order (Recommended) | Computed DDS order; Archive chunk order; Reference decides |
| Should normal extraction call DirectXTex at runtime to validate the reconstructed DDS before returning success? | No runtime validation (Recommended) | No runtime validation; Validate always; Debug/test only |
| How should sink-based extraction handle reconstructed DDS output? | Header then chunks (Recommended) | Header then chunks; Build full DDS first; You decide |
| How should `extract_bytes(path)` behave for BA2 DX10 entries? | Same DDS bytes (Recommended) | Same DDS bytes; Payload only; Metadata dependent |
| How strict should DDS assembly be when BA2 chunk metadata leaves a gap or duplicate in expected mip/face coverage? | Reject archive (Recommended) | Reject archive; Best effort; Manifest decides |
| How should DDS DXT10 fields that are not clearly present in BA2 records be set? | Conservative defaults (Recommended) | Conservative defaults; Reference exact; Expose uncertainty |
| Should Phase 6 require reconstructed DDS output to preserve original BA2 payload bytes exactly after the header? | Decoded bytes exact (Recommended) | Decoded bytes exact; Metadata only; Per fixture |
| What error code policy should DX10 extraction follow for bad compressed chunks or decoded-size mismatches? | format_error (Recommended) | format_error; io_error; Separate later |
| Should Phase 6 preserve BA2 DX10 `find`/`contains` behavior exactly like GNRL for texture paths? | Same path semantics (Recommended) | Same path semantics; Texture-specific paths; You decide |

**Notes:** User chose strict metadata/layout validation, deterministic DXT10 output, and existing path/extraction semantics.

---

## Fixture Coverage Shape

| Question | Selected | Alternatives Presented |
|----------|----------|------------------------|
| What shape should the generated DX10 success fixture set have? | Small rich matrix (Recommended) | Small rich matrix; One huge archive; Many tiny archives |
| How realistic should generated texture payloads be? | Tiny valid textures (Recommended) | Tiny valid textures; Header-only style; Real-like formats |
| What malformed DX10 fixture coverage should be mandatory in Phase 6? | Focused SPEC set (Recommended) | Focused SPEC set; Broad hardening; Minimal malformed |
| How detailed should DX10 fixture manifests be? | Rich dual metadata (Recommended) | Rich dual metadata; Public metadata only; Archive internals only |

**Notes:** User chose compact but high-signal generated fixtures with rich manifests and focused malformed coverage.

---

## Analyzer Boundary Reuse

| Question | Selected | Alternatives Presented |
|----------|----------|------------------------|
| How reusable should the internal DirectXTex/DDS boundary be in Phase 6? | Reusable layout core (Recommended) | Reusable layout core; Read-only minimal; Test-only adapter |
| Where should DirectXTex be linked once CMake wiring is added? | Private libbsa link (Recommended) | Private libbsa link; Tests only; Separate helper target |
| What should the internal analyzer return to the rest of libbsa? | libbsa values (Recommended) | libbsa values; DirectXTex wrappers; Validation result only |
| How much Phase 9 writer preparation is allowed inside Phase 6 analyzer work? | No writer behavior (Recommended) | No writer behavior; Input analysis now; Planner discretion |

**Notes:** User chose a reusable internal layout core that remains private and does not implement Phase 9 writer behavior early.

---

## the agent's Discretion

- Planner may choose exact internal file names, helper boundaries, parser structs, fixture filenames, and CMake target organization if `06-CONTEXT.md` and `06-SPEC.md` decisions are preserved.
- Researcher/planner must trace reference behavior for DX10 record/chunk layout and mip/array/cubemap ordering before implementation details are locked.

## Deferred Ideas

None - discussion stayed within phase scope.
