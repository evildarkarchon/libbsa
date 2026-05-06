# Phase 07: ba2-dds-read-and-dds-reconstruction - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-05T21:23:25.3309455-07:00
**Phase:** 07-ba2-dds-read-and-dds-reconstruction
**Areas discussed:** Texture Metadata API, DXGI Format Representation, Chunk Model And Extraction Assembly, DDS Reconstruction Boundary, Fixture Corpus Shape

---

## Texture Metadata API

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| How should consumers retrieve DX10 texture metadata? | Dedicated `texture_metadata(path)` API; extend `entry_metadata`; add a BA2-specific entry type; you decide | Dedicated `texture_metadata(path)` API |
| What should that API return when texture metadata is unavailable? | Structured failure; optional metadata; only expose it on texture archive objects; you decide | Structured failure |
| How much texture layout detail should public metadata include? | Consumer inspection fields plus chunk summaries; consumer inspection fields only; full native BA2 record mirror; you decide | Consumer inspection fields plus chunk summaries |
| Should `texture_metadata` be returned by value or as a view/reference? | Return by value; return a const reference/view; you decide | Return by value |

**User's choice:** Use a dedicated `ba2_archive::texture_metadata(path)` API returning `result<texture_metadata>` by value, with inspection fields and chunk summaries.
**Notes:** This keeps generic `entry_metadata` stable and preserves the existing copied-metadata ownership style.

---

## DXGI Format Representation

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| How should public metadata represent the texture format? | Raw DXGI numeric ID plus optional label; libbsa enum only; raw numeric ID only; you decide | Raw DXGI numeric ID plus optional label |
| What should the public type/name be? | `dxgi_format` value type; `texture_format` value type; plain `std::uint32_t format`; you decide | `dxgi_format` value type |
| How should unknown or future DXGI values behave? | Preserve unknown numeric values; reject unknown values during open; map unknown values to sentinel; you decide | Preserve unknown numeric values |
| Should `dxgi_format` provide built-in name lookup? | Built-in known-name helper; numeric only for Phase 7; you decide | Built-in known-name helper |

**User's choice:** Use a libbsa-owned `dxgi_format` value type that preserves the raw numeric value and provides known-name lookup where possible.
**Notes:** Unknown values remain inspectable; extraction fails only when valid DDS reconstruction cannot be supported.

---

## Chunk Model And Extraction Assembly

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| Should chunk summaries be public, and what should they mean? | Public logical chunk summaries; internal-only chunk model; public native chunk records; you decide | Public logical chunk summaries |
| How should extraction assemble chunk payloads before writing? | Validate/decompress/reconstruct fully before sink write; stream validated chunks; hybrid temporary sink; you decide | Validate/decompress/reconstruct fully before sink write |
| How should DX10 chunk codec routing be decided? | Per chunk from archive variant plus chunk sizes plus archive compression method; reuse entry-level compression only; try codecs until one works; you decide | Per chunk from archive variant plus chunk sizes plus archive compression method |
| Should Phase 7 preserve chunk-to-mip range association? | Expose mip range when derivable; expose order and sizes only; keep mip mapping internal; you decide | Expose mip range when derivable |

**User's choice:** Expose logical chunk summaries and assemble complete DDS bytes before any sink write.
**Notes:** Codec routing remains metadata-driven with no fallback guessing. Inconsistent chunk/mip mapping is malformed.

---

## DDS Reconstruction Boundary

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| What role should DirectXTex play in implementation? | Private validation adapter after reconstruction; tests only; DirectXTex-assisted reconstruction; you decide | Private validation adapter after reconstruction |
| If DirectXTex validation fails, what error shape should extraction return? | Structured malformed/reconstruction failure with no partial write; return raw reconstructed bytes anyway; fallback to decompressed payload without DDS header; you decide | Structured malformed/reconstruction failure with no partial write |
| How should unsupported layouts be handled? | Metadata inspection succeeds and extraction fails structurally; reject archive during open; best-effort DDS output; you decide | Metadata inspection succeeds and extraction fails structurally |
| Should reconstruction be BA2-only or reusable internally? | Internal reusable texture component; BA2 reader detail for now; public utility API; you decide | Internal reusable texture component |

**User's choice:** libbsa reconstructs DDS headers itself, then privately validates with DirectXTex before sink writes where practical.
**Notes:** Unsupported-but-readable textures remain inspectable. Reconstruction helpers should be private and reusable for later texture/writer phases.

---

## Fixture Corpus Shape

| Question | Options Considered | Selected |
|----------|--------------------|----------|
| How should generated BA2 DDS fixture builders be organized? | Reusable test helper module; keep builders inside one test file; commit binary fixture blobs; you decide | Reusable test helper module |
| What should positive fixtures prioritize? | Locked variant plus layout matrix; variant matrix first with minimal layouts; layout matrix first with fewer variants; you decide | Locked variant plus layout matrix |
| How should malformed fixture coverage be shaped? | Targeted malformed builders per failure class; a few broad corrupted archives; defer most malformed coverage to Phase 11; you decide | Targeted malformed builders per failure class |
| How should DirectXTex validation be used in tests? | Validate every positive extracted DDS fixture; validate one smoke fixture only; validate only reconstruction helper tests; you decide | Validate every positive extracted DDS fixture |
| Should tests compare bytes exactly or validate semantic equivalence? | Semantic equivalence first; byte-exact for every positive fixture; DirectXTex loadability only; you decide | Semantic equivalence first |

**User's choice:** Build a reusable private generated-fixture helper corpus that covers required variants, layouts, codec routes, targeted malformed failures, and DirectXTex-validated semantic DDS output.
**Notes:** Byte-exact checks are allowed only where deterministic and meaningful.

---

## the agent's Discretion

No selected area was left to the agent's discretion.

## Deferred Ideas

None - discussion stayed within phase scope.
