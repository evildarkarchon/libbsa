# Feature Research

**Domain:** Reusable C++20 library for Bethesda BSA/BA2 archive reading, writing, inspection, and extraction  
**Researched:** 2026-05-05  
**Confidence:** HIGH for PRD-aligned core capabilities; MEDIUM for competitor-derived differentiators; LOW for console/PS4/GNF expectations because public tooling support is inconsistent or explicitly tentative.

## Feature Landscape

### Table Stakes (Consumers Cannot Rely on the Library Without These)

Features consumers assume exist in a reusable archive-format library. Missing these either blocks whole classes of consumers or causes compatibility failures against game archives.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Archive family auto-detection | Consumers should not need to pre-classify BSA vs BA2 vs generation before opening a file. | MEDIUM | Detect from magic, version, and subtype; should be exposed as both a lightweight sniffing API and full validated open. |
| Full read support for TES3, TES4-family BSA, BA2 GNRL, and BA2 DDS | Core value is coverage from Morrowind through Starfield; partial read support makes the library unreliable for mod managers and asset pipelines. | HIGH | Include Morrowind, Oblivion, FO3/FNV/Skyrim LE, Skyrim SE/AE, Fallout 4, Fallout 76-adjacent BA2 versions where structurally compatible, and Starfield variants listed in the PRD. |
| Transparent extraction with correct decompression | Users expect `read file` or `extract file` to return original bytes regardless of archive compression. | HIGH | Requires deflate, LZ4 frame, and raw LZ4 block routing by format/version/compression method. Incorrect routing silently corrupts output. |
| Random-access file lookup by normalized archive path | Library consumers need single-file reads for virtual filesystems, asset viewers, and patch tooling. | MEDIUM | Support lookup by canonical archive path and, where useful, raw hash. Path normalization must mirror Bethesda behavior without assuming host filesystem semantics. |
| Complete file listing and metadata inspection | Browsers, mod managers, diagnostics, and build tools need path lists, counts, sizes, offsets, compression flags, hashes, archive type, version, and flags. | MEDIUM | Entry metadata should be available without extracting payload bytes. |
| Streaming read/extraction API | Large Skyrim/Fallout/Starfield archives make whole-archive or whole-file buffering unacceptable. | HIGH | Provide caller-owned output sinks/readers. In-memory convenience APIs can wrap streaming, not replace it. |
| BA2 DDS header reconstruction | Texture archives do not store ordinary DDS files directly; extracting usable `.dds` files requires reconstruction from texture metadata/chunks. | HIGH | Must handle dimensions, mip counts, DXGI formats, cubemaps, and chunk decompression correctly. B.A.E. history shows width/height, cubemap, LZ4 size, and DXGI variant bugs are real-world failure modes. |
| Archive creation/write support for supported formats | A reusable library is expected to build archives, not only unpack them; PRD explicitly targets read/write parity. | HIGH | Provide builders for TES3, TES4-family BSA, BA2 GNRL, and BA2 DDS. Writing must be format-specific; avoid a single magical builder that hides incompatible policies. |
| Per-file compression policy | Builders need archive default, force compressed, and force raw choices to reproduce official/reference output and support edge-case file types. | MEDIUM | Must validate unsupported combinations, e.g., format/version-specific compression methods. |
| Correct hash generation and hash-ordered tables | Bethesda engines frequently identify entries by hashes and expect sorted indices. | HIGH | Implement TES3, TES4-family, and FO4/BA2 hash behavior; expose hash metadata for inspection and diagnostics. |
| Correct file/archive flag derivation | Game loadability depends on flags for file types, embedded names, named directories/files, and compression. | HIGH | Provide sensible automatic derivation plus explicit override hooks for expert callers. |
| BA2 DDS write support from DDS inputs | Texture packers need to create loadable BA2 DX10 archives, not just extract them. | HIGH | Requires DirectXTex-backed DDS analysis, DXGI format mapping, mip range/chunk generation, cubemap support, and per-chunk compression. |
| Round-trip and compatibility validation surfaces | Consumers need confidence that generated archives load in target engines and extracted files match reference behavior. | MEDIUM | Include testing utilities or documented examples for round-trip pack/extract and metadata comparison; do not make tests a runtime dependency. |
| Robust error reporting for malformed/truncated archives | Archive tools routinely encounter corrupt mods, interrupted downloads, and hand-written archives. | MEDIUM | Return explicit errors for invalid magic/version, impossible offsets, truncated tables, decompression failures, path hazards, and unsupported variants. |
| Stable, reusable C++ API with no UI coupling | The project is a library; consumers need embeddable primitives, not application workflows. | MEDIUM | Public headers should avoid Delphi/BSArchPro concepts, platform APIs, and dependency leakage. |

### Differentiators (Competitive Advantage / Polish)

These are valuable once the table-stakes compatibility surface exists. They should be planned deliberately because several are expensive or can blur the library/application boundary.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Unified archive facade plus format-specific expert APIs | Simple consumers can open/list/read any archive; advanced callers can access format-specific metadata and builder policy. | MEDIUM | `dream_archive` and Ryan McKenzie's `bsa` both show value in ergonomic high-level entry points with lower-level escape hatches. |
| Byte-first archive path model with explicit encoding helpers | Avoids mojibake and unreproducible hashes for legacy code-page paths while still supporting user-facing Unicode workflows. | HIGH | Treat archive paths as bytes internally; provide explicit encode/decode helpers for common Windows code pages instead of guessing. |
| Safety-hardened disk extraction helper | Gives embedders a secure default for extracting to disk without requiring every caller to rediscover traversal/NUL/absolute-path issues. | MEDIUM | Reject absolute paths, `..`, NUL, colon-containing components, and unsafe overwrite modes. Keep this as optional helper policy above core streaming APIs. |
| Bulk extraction with progress, cancellation, and partial-failure reporting | Mod managers and asset tools need long-running extraction to be observable and recoverable. | MEDIUM | Library-level callbacks are appropriate; GUI progress windows are not. Report per-entry successes/failures without aborting unnecessarily. |
| Parallel bulk extraction and packing | Large Starfield and texture archives benefit materially from parallel compression/decompression. | HIGH | PRD defers full performance work; implement after correctness. Use explicit threading guarantees and avoid global thread pools unless caller-configurable. |
| Archive diff/compare primitives | Useful for mod conflict analysis, regression tests, and build validation. BSA Browser exposes archive comparison as a user-facing feature. | MEDIUM | Provide metadata/content comparison APIs, not UI. Content diff should be streaming/hash-based. |
| Verification/lint reports for engine compatibility | Turns hidden game-loading quirks into actionable diagnostics before shipping an archive. | HIGH | Detect long paths, extended-ASCII/hash hazards, unsupported compression/layout, excessive BA2 chunks, compressed sounds, SSE `EMBEDNAME` hazards, and malformed tables. |
| Plan-before-write / dry-run builder validation | Lets tools preview output entries, sizes, compression choices, conflicts, and warnings before writing multi-GB archives. | MEDIUM | Especially useful when archive creation streams data and failures are expensive late in the process. |
| Preserve unchanged archive members during rebuild | Enables open-read-modify-write workflows without extracting everything into memory first, while still avoiding true in-place mutation. | HIGH | This reconciles the PRD non-goal: no in-place patching, but rebuilding with preserved members is acceptable. |
| Content deduplication during writing | Reduces archive size when identical assets occur under multiple paths. | MEDIUM | PRD lists it; make policy opt-in or transparent with clear metadata implications. |
| Texture metadata preservation/copy path | Avoids reinterpreting/rechunking unchanged BA2 DDS entries when rebuilding archives. | HIGH | High payoff for update/rebuild workflows; must preserve compatibility exactly. |
| Small integration examples | Lowers adoption friction for CMake consumers and tool authors. | LOW | Examples should cover open/list/read, streaming extract, create BSA, create BA2 GNRL, create BA2 DDS, and error handling. |
| Fuzz/property-test harnesses for parsers | Differentiates on robustness against malformed files. | MEDIUM | Keep as development/testing artifact, not public runtime feature. |
| Optional memory-mapped input backend | Can reduce overhead for index-heavy reads and random access. | MEDIUM | Ryan McKenzie's Rust port advertises mmap to reduce DOM memory bloat; in C++ this should remain a backend option behind the I/O abstraction. |

### Anti-Features (Deliberately Do Not Build)

These are commonly tempting but conflict with the PRD, project constraints, or reusable-library scope.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| GUI archive browser/extractor | Many ecosystem tools are GUI browsers, and users ask for browse/search/extract UX. | Explicit PRD non-goal; creates UI framework, platform, and product support burden. | Provide library APIs and examples; consumers build GUI/CLI tools. |
| CLI application as a primary deliverable | Helpful for testing and end users. | Explicit project scope says library only; a CLI can dominate roadmap and API design. | Keep tiny test/example executables internal if needed, but do not productize them in core roadmap. |
| Network/URL archive access | Remote mod repositories and cloud pipelines may want direct URLs. | Explicit PRD non-goal; introduces HTTP, caching, auth, retries, and security surface unrelated to format correctness. | Accept caller-provided streams/files; let applications handle transport. |
| Non-Bethesda formats such as ZIP/7z | Archive-library users may ask for a universal abstraction. | Explicit PRD non-goal and dilutes compatibility work. | Provide BSA/BA2 only; let callers compose with other archive libraries. |
| True in-place mutation of existing archives | Seems faster for small changes. | Explicit PRD non-goal; archive tables, offsets, compression sizes, and chunking make safe mutation fragile. | Support open-read-modify-write via rebuild, member preservation, and atomic replace helpers where appropriate. |
| Directly compiling or wrapping TES5Edit/BSArchPro code | Fastest apparent path to compatibility. | Violates read-only reference boundary and leaks Delphi/UI architecture into C++ API. | Trace behavior and reimplement cleanly with compatibility notes/tests. |
| Game-specific mod management workflows | Mod managers need load-order, conflict, plugin, and deployment logic. | Outside archive-format library scope; risks becoming a mod manager. | Expose archive metadata and diff primitives that mod managers can use. |
| Texture transcoding/optimization as archive feature | BA2 DDS handling touches texture formats, so conversion requests are likely. | Requires asset-processing policy, quality settings, GPU format decisions, and expands beyond archive packaging. | Parse/analyze DDS enough to pack/extract correctly; leave conversion to DirectXTex-using caller tools. |
| Console-only swizzled texture payload support by default | BSA Browser mentions PS4/GNF support; some users may ask for console archive parity. | Public support appears tentative, and correct GNF support needs console swizzle/unswizzle semantics beyond raw archive extraction. | Treat PC Bethesda BSA/BA2 as core; consider metadata-only or explicit future research for console variants. |
| Proprietary XMem compression support by default | Legacy Skyrim/Xbox-related compression flags exist. | Proprietary/platform-specific and conflicts with portability; Ryan McKenzie's bsa makes XMem opt-in with a separate proxy. | Do not include in core. If ever needed, make optional and separately documented. |
| Global registry/singleton configuration | Convenient for applications. | Conflicts with no-global-state requirement and complicates thread safety/tests. | Use explicit format registry/config objects owned by caller or archive instance. |
| Silent best-effort path decoding | Makes file names look nicer in UI. | Bethesda archives do not reliably declare filename encodings; guessing can break lookup/hash behavior. | Byte-first paths plus explicit encoding helpers and diagnostics. |

## Feature Dependencies

```text
Archive auto-detection
    └──requires──> Binary header readers and format registry
                    └──enables──> Unified open/list/read facade

Transparent extraction
    ├──requires──> Format-specific table parsing
    ├──requires──> Compression routing (deflate / LZ4 frame / LZ4 block)
    └──requires──> Streaming I/O abstraction

BA2 DDS extraction
    ├──requires──> BA2 DX10 record/chunk parsing
    ├──requires──> Per-chunk decompression
    └──requires──> DDS header reconstruction

Archive writing
    ├──requires──> Hash generation
    ├──requires──> Sorted table generation
    ├──requires──> Compression encoders
    ├──requires──> Streaming output/finalization
    └──enables──> Round-trip compatibility tests

BA2 DDS writing
    ├──requires──> Archive writing
    ├──requires──> DDS analysis / DXGI metadata
    └──requires──> Mipmap chunking policy

Compatibility lint/verification
    ├──requires──> Full metadata inspection
    ├──requires──> Path normalization / byte-first path model
    └──enhances──> Archive writing and rebuild workflows

Parallel extraction/packing
    ├──requires──> Correct streaming extraction/writing
    ├──requires──> Thread-safety contract
    └──enhances──> Bulk extraction and archive creation

Preserve unchanged members during rebuild
    ├──requires──> Stable entry identity and metadata
    ├──requires──> Streaming copy path
    └──conflicts──> True in-place mutation
```

### Dependency Notes

- **Auto-detection before all feature facades:** A reusable API can expose format-specific readers, but most consumers need a safe `open archive` path that identifies the container first.
- **Streaming before performance:** Parallelism should not be built around whole-archive memory buffers; the PRD explicitly values large archive support and defers full multi-threading until correctness exists.
- **DDS read before DDS write:** Header reconstruction and chunk parsing prove the library understands BA2 texture metadata before it attempts to generate it.
- **Hash/path correctness before writing:** Generated archives are only useful if game engines can locate entries; hash and normalization bugs are worse than simple extraction failures.
- **Verification after broad format coverage:** Compatibility linting needs enough metadata across all formats to produce meaningful warnings instead of format-specific guesses.
- **Rebuild workflows instead of in-place mutation:** Open-read-modify-write is acceptable when implemented as a new archive build that preserves or copies unchanged entries; direct mutation should remain excluded.

## MVP Definition

### Launch With (v1)

Minimum viable library release for consumer validation should focus on reliable read/extract/inspect across the most common PC formats before attempting full writer parity.

- [ ] Archive auto-detection for TES4-family BSA and BA2 containers — establishes the public open path.
- [ ] TES4/FO3/SSE BSA read, list, metadata, lookup, and streaming extraction — first high-value compatibility target from the PRD roadmap.
- [ ] Transparent deflate and LZ4 frame decompression — required for the first BSA read milestone.
- [ ] Explicit error model for invalid headers, missing entries, decompression failure, and truncated payloads — needed before downstream tools can safely embed the library.
- [ ] Fixture and compatibility tests against BSArchPro/reference outputs — validates the central correctness claim.

### Add After Validation (v1.x)

- [ ] TES3 BSA read/extract — simpler format but distinct hash/offset behavior.
- [ ] BA2 GNRL read/extract for Fallout 4 and Starfield — expands to modern archives and LZ4 block routing.
- [ ] BA2 DDS read/extract with DDS reconstruction — essential for texture archive consumers.
- [ ] Unified archive facade over all read-capable formats — add once multiple backends exist.
- [ ] Safety-hardened disk extraction helper — valuable once streaming extraction exists.

### Future Consideration (v2+)

- [ ] Write support for TES4-family BSA, BA2 GNRL, BA2 DDS, and TES3 BSA — table stakes for final product, but should follow proven read/parsing behavior.
- [ ] Compatibility verification/lint reports — high-value differentiator once metadata coverage is complete.
- [ ] Parallel extraction/packing and benchmarks — defer until streaming correctness and thread-safety contracts are stable.
- [ ] Archive diff/compare APIs — useful but not required for core archive access.
- [ ] Preserve unchanged members in rebuild workflows — valuable for update tools, but depends on stable write support.

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Auto-detection | HIGH | MEDIUM | P1 |
| TES4-family BSA read/extract | HIGH | HIGH | P1 |
| Streaming extraction | HIGH | HIGH | P1 |
| Deflate/LZ4 decompression routing | HIGH | HIGH | P1 |
| Metadata/list/exists APIs | HIGH | MEDIUM | P1 |
| Error model and malformed archive handling | HIGH | MEDIUM | P1 |
| TES3 read/extract | MEDIUM | MEDIUM | P2 |
| BA2 GNRL read/extract | HIGH | HIGH | P2 |
| BA2 DDS read/extract | HIGH | HIGH | P2 |
| Unified archive facade | HIGH | MEDIUM | P2 |
| TES4-family write | HIGH | HIGH | P2 |
| BA2 GNRL write | HIGH | HIGH | P2 |
| BA2 DDS write | HIGH | HIGH | P2 |
| TES3 write | MEDIUM | MEDIUM | P2 |
| Compatibility lint/verify | MEDIUM | HIGH | P3 |
| Parallel bulk operations | MEDIUM | HIGH | P3 |
| Archive diff/compare | MEDIUM | MEDIUM | P3 |
| Preserve unchanged members during rebuild | MEDIUM | HIGH | P3 |
| Optional mmap backend | LOW | MEDIUM | P3 |

**Priority key:**
- P1: Must have for first usable library validation.
- P2: Required for the full PRD product promise, but can follow initial read/extract validation.
- P3: Differentiator or scale/polish feature; build after compatibility is established.

## Competitor / Ecosystem Feature Analysis

| Capability | Ecosystem Evidence | Implication for libbsa |
|------------|--------------------|------------------------|
| Browse/list/extract archives | BSA Browser and B.A.E. center on browsing/extraction; bethesda-structs exposes parsing/extraction examples. | Listing, lookup, metadata, and extraction are non-negotiable table stakes. |
| Library, not user-facing app | Ryan McKenzie's C++ `bsa`, Rust `ba2`, and `dream_archive` position themselves as programmatic archive libraries. | libbsa should compete on clean C++20 API and compatibility, not GUI features. |
| Read/write support | Ryan McKenzie's `bsa` and `dream_archive` include building/writing examples; bethesda-structs explicitly lacks writers. | Write support is a major table stake for this PRD and a differentiator over parser-only packages. |
| Low-overhead/random access | Ryan McKenzie's `bsa` emphasizes low overhead/no-copy views; Rust `ba2` highlights memory-mapped I/O. | Keep metadata/index operations cheap and avoid whole-archive loading. |
| DDS/texture correctness | B.A.E. changelog lists real fixes for dimensions, cubemaps, LZ4 sizes, and DXGI variants. | BA2 DDS handling must be tested deeply; texture extraction/writing is high-risk table stakes. |
| Path and encoding correctness | Ryan McKenzie's docs warn about extended ASCII, slash normalization, and hash behavior; `dream_archive` uses byte-string archive paths. | Byte-first path model and explicit normalization/encoding helpers are a strong differentiator. |
| Safety and verification | `dream_archivetool` rejects unsafe extraction paths and exposes verify/report types. | A reusable C++ library should provide optional safe extraction and diagnostics instead of leaving hazards to every consumer. |
| Archive comparison/search | BSA Browser exposes regex/wildcard search and archive comparison. | Search can remain application-level over list APIs; compare/diff primitives are a useful later differentiator. |

## Sources

- Project context: `.planning/PROJECT.md` (HIGH) — core value, active requirements, out-of-scope boundaries, dependency and API constraints.
- Product requirements: `docs/PRD.md` (HIGH) — supported formats, core read/write/query capabilities, milestones, non-goals, risks.
- Project constraints: `AGENTS.md` (HIGH) — C++ direction, TES5Edit read-only boundary, dependency restrictions, validation expectations.
- Ryan McKenzie's C++ `bsa` documentation: https://ryan-rsm-mckenzie.github.io/bsa/ (MEDIUM) — programmatic C++ library expectations, low-overhead API, read/write examples, path/hash gotchas, BA2 chunk warning, optional XMem stance.
- Ryan McKenzie's `bsa` repository: https://github.com/Ryan-rsm-McKenzie/bsa (MEDIUM) — C++ Bethesda archive library with CMake/tests/examples and Starfield-era scope.
- `dream_archive` docs: https://docs.rs/dream_archive/latest/dream_archive/ (MEDIUM) — unified facade, read/write support, byte-first paths, feature flags, explicit unsupported console/XMem scope.
- `dream_archivetool` docs: https://docs.rs/dream_archivetool/latest/dream_archivetool/ (MEDIUM) — application-oriented wrappers, safe extraction policy, verify/diff/create plan API surface.
- BSA Browser repository: https://github.com/AlexxEG/BSA_Browser (MEDIUM) — browse/extract/search/preview/multi-archive/compare features in user-facing tooling.
- B.A.E. Nexus listing/changelog excerpts: https://www.nexusmods.com/fallout4/mods/78 (MEDIUM) — real-world BA2/BSA extraction expectations and BA2 texture correctness pitfalls.
- bethesda-structs repository/docs: https://github.com/stephen-bunn/bethesda-structs (LOW/MEDIUM) — parser/extractor precedent and explicit writer gap; archived project, so useful mostly as historical ecosystem context.

---
*Feature research for: libbsa Bethesda BSA/BA2 C++ library*  
*Researched: 2026-05-05*
