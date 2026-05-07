# Feature Research

**Domain:** Reusable C++20 Bethesda BSA/BA2 archive library
**Researched:** 2026-05-07
**Confidence:** HIGH for project-required capabilities and BSArch/BSArchPro-observed features; MEDIUM for broader competitor/library expectations where evidence is from public package pages and tool documentation rather than formal specs.

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist in a serious Bethesda archive library. Missing these means downstream mod managers, asset pipelines, and game utilities will either reject the library or wrap it with their own incompatible code.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Library-only embeddable API | The product is a reusable library, not a GUI/CLI tool; consumers need static or dynamic linking without process spawning or UI coupling. | MEDIUM | Public headers should expose value-oriented C++20 types, stable error reporting, and no Delphi/BSArchPro implementation details. |
| Archive auto-detection by magic/version/type | Consumers expect `open()` to identify TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10/DDS without manual parsing. | MEDIUM | Detection must be byte-driven, not extension-driven. Extension can be advisory only. |
| Full read support for all target variants | Core value requires reading TES3, TES4 v103, FO3/FNV/Skyrim LE v104, SSE/AE v105, FO4 BA2, and Starfield BA2 variants. | HIGH | Variant-specific headers, table layouts, hashes, offsets, compression, and texture records must be isolated behind a common interface. |
| File listing and bulk iteration | Archive consumers commonly inspect contents before extraction and need to enumerate entries efficiently. | LOW-MEDIUM | Include stable ordering options: physical/archive order for compatibility diagnostics and normalized path order for callers. |
| Path existence and metadata queries | Modding tools need fast lookup, sizes, compression status, hash, flags, archive type, and version without extracting data. | MEDIUM | Use normalized virtual archive paths, not `std::filesystem::path`, for internal archive identifiers. |
| Random-access extraction by path or entry handle | Tools rarely extract an entire archive every time; they need individual assets by path/hash. | MEDIUM | Entry handles/IDs avoid repeated path normalization and hash lookup in hot paths. |
| Streaming extraction to caller-provided sink | Large Starfield/FO4 archives can be many GB; whole-archive or whole-output assumptions are unacceptable. | HIGH | Should support bounded scratch buffers and exact-size decompression checks. |
| Transparent decompression | Users expect extracted bytes, not compressed payload management. | HIGH | Required modes: deflate, LZ4 frame for SSE BSA, raw LZ4 block for Starfield BA2 v3 method 3. Route by format/version/metadata, never by extension. |
| BA2 DDS extraction with valid DDS reconstruction | Texture BA2 archives do not simply store standalone DDS files; tools expect extracted textures to be loadable DDS files. | HIGH | Requires metadata-to-DDS header reconstruction, chunk stitching, mip handling, cubemap handling, and DirectXTex-backed validation internally. |
| New archive creation for every supported variant | A reusable archive library is incomplete if it only extracts; mod packaging and asset pipelines need writers. | HIGH | Build read support first, then writers after format behavior is verified. |
| Add files from memory and host filesystem | Library consumers may generate assets in memory or package files from disk. | MEDIUM | Keep host filesystem APIs at boundaries; virtual archive paths remain explicit strings. |
| Archive finalization / write-new flow | Bethesda archives require coherent headers, offsets, indexes, hash tables, and file tables after all payload data is known. | HIGH | Prefer builder/finalizer semantics over in-place mutation for early milestones. |
| Correct hash generation and sorted table generation | Game engines and existing tools depend on format-specific hash and ordering rules. | HIGH | Implement TES3, TES4-family, and FO4/BA2 hash behavior with fixture tests before writer phases. |
| Per-file compression control | BSArch/BSArchPro expose compression controls, and real archives mix compressed and uncompressed entries. | MEDIUM-HIGH | Include archive default plus per-entry override: default/compress/store. |
| Automatic archive/file flag derivation | Users expect valid game-loadable archives without hand-setting bitfields for common cases. | MEDIUM-HIGH | Support explicit override for compatibility diagnostics, but safe automatic flags should be default. |
| Embedded file-name handling for BSA | Existing archives and compatibility quirks use embedded names. | MEDIUM | Reading and writing must preserve or intentionally choose embedded-name behavior per variant and known engine constraints. |
| BA2 GNRL and DDS file table handling | FO4/Starfield BA2 require correct name tables, record fields, and version-specific headers. | HIGH | Starfield v2/v3 and FO4 v1/v7/v8 differences should be represented in metadata, not hidden booleans. |
| BA2 DDS mip/chunk planning on write | Texture archives require chunk records and mip layout, not generic file payload storage. | HIGH | DirectXTex should stay internal; expose libbsa-native texture metadata and errors. |
| Byte-level / metadata-level compatibility tests | The core value is compatibility with official tools and BSArchPro; tests must prove it continuously. | HIGH | Fixture extraction, round-trip, header serialization, hash, compression, and BSArchPro comparison tests are table stakes. |
| Structured error reporting | Consumers need to distinguish unsupported version, malformed archive, missing entry, I/O failure, decompression failure, and invalid DDS metadata. | MEDIUM | Use a C++20-compatible local result/error model rather than public `std::expected`. |
| Bounds-checked malformed archive handling | Archive inputs are untrusted binary data; truncation and bogus sizes must not crash or over-read. | HIGH | Validate offsets, counts, compressed sizes, decompressed sizes, chunk ranges, and name table bounds before use. |
| Minimal dependency and header surface | Reusable libraries are judged by integration friction. | MEDIUM | Do not expose libdeflate, lz4, DirectXTex, Windows headers, or TES5Edit types in public headers. |

### Differentiators (Competitive Advantage)

These features make libbsa more valuable than shelling out to BSArch/Archive2 or using a narrower read-only package. They should align with the core value: embeddable, compatible, format-complete behavior.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Unified all-generations API | One library can handle Morrowind through Starfield without callers learning each archive family. | HIGH | Keep common operations stable while exposing variant-specific metadata through typed optional views. |
| Compatibility mode profiles | Consumers can target a game/tool profile such as TES3, TES4, FO3/FNV, Skyrim LE, SSE/AE, FO4 GNRL, FO4 DDS, SF GNRL, or SF DDS. | MEDIUM-HIGH | Profiles should drive defaults for compression, flags, DDS handling, and known unsafe combinations. |
| BSArchPro-compatible behavior oracle tests | Stronger trust than “works on my archive” because outputs are compared against the repository’s behavioral reference. | HIGH | Do not compile or mutate TES5Edit; use generated fixtures/reference outputs outside the submodule. |
| Rich inspection API without extraction | Downstream tools can display archive diagnostics, compression breakdown, file type flags, DDS dimensions/formats/mips, duplicate payloads, and suspicious records. | MEDIUM | Keep it data-only; no GUI report rendering. |
| Compatibility warnings as data | Libraries can surface warnings for non-ASCII names, compressing audio/string assets, SSE embed-name hazards, unsupported versions, or unusual Starfield compression methods. | MEDIUM | Return warnings alongside successful build/open operations; do not print/log by default. |
| Data deduplication / shared payload support | Saves archive size for identical files and matches BSArch shared-data workflows. | MEDIUM-HIGH | Requires content hashing and careful offset reuse. Make it opt-in/default-profiled rather than hidden magic. |
| Streaming writer with bounded memory | Lets consumers package very large mods without building complete archives in RAM. | HIGH | Requires offset reservation/finalization strategy and temporary index state, but is a major library-quality differentiator. |
| Parallel bulk extraction and packing | Makes large archive workloads practical and competitive with BSArch multithreaded modes. | HIGH | Defer until correctness is stable. Document thread-safety and preserve deterministic output where compatibility requires it. |
| Deterministic archive output mode | Reproducible builds help mod packaging, CI, and compatibility comparisons. | MEDIUM-HIGH | Requires stable ordering, timestamps avoided or normalized, deterministic compression settings, and deterministic dedup decisions. |
| Fixture generation helpers for tests | Consumers and libbsa itself can generate tiny archives covering edge cases without checking in huge game assets. | MEDIUM | Keep helpers in test/support namespace or separate test utility; not a runtime requirement. |
| Fine-grained validation API | Callers can validate an archive or planned package without extracting/writing it. | MEDIUM-HIGH | Useful for asset pipelines. Should report machine-readable warnings/errors. |
| Format-preserving read metadata | Enables tools to inspect and rewrite archives without losing unknown fields like Starfield v2 `Unknown1`/`Unknown2`. | MEDIUM | Preserve known-unknown fields as named opaque metadata; avoid pretending they are understood. |
| Incremental/cancellable operation hooks | Asset pipelines need progress and cancellation for multi-GB operations, even without a GUI. | MEDIUM | Provide callback/cancellation points without taking a dependency on a logging/progress framework. |
| Cross-platform-friendly implementation boundary | Windows is primary, but Linux/macOS consumers in mod tooling benefit if non-Windows paths are not blocked by headers. | MEDIUM | DirectXTex and filesystem interactions must remain behind internal/adaptable boundaries. |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem useful but conflict with the library’s non-goals, portability, or compatibility focus.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| GUI application | Users associate archives with visual tools like BSArchPro. | Violates project non-goal and creates UI/platform coupling. | Expose inspection, warning, progress, and operation APIs so consumers can build their own GUIs. |
| CLI as part of core product | Useful for manual testing and packaging scripts. | Scope creep; can distort API around command-line workflows. | Keep small examples or test utilities later; core deliverable remains library API. |
| Network/URL archive access | Some tools might want remote asset pipelines. | Non-goal and adds security, retries, caching, and dependency concerns. | Accept caller-provided streams/sources; callers own network transport. |
| ZIP/7z/libarchive support | “Archive library” sounds generic. | Dilutes Bethesda-specific compatibility value and violates non-goal. | Stay purpose-built for BSA/BA2; consumers can compose with other libraries. |
| Editing/compiling TES5Edit code | Faster path to behavior parity. | Violates hard repository boundary and leaks Delphi/UI implementation details. | Trace TES5Edit behavior as read-only prior art and reimplement cleanly. |
| Public DirectXTex/libdeflate/lz4 types | Makes advanced users feel closer to internals. | Leaks dependencies and harms ABI/portability. | Translate to libbsa-native metadata and error types at public boundaries. |
| In-place archive mutation early | Appears convenient for mod tools. | Hard to make safe with offset shifts, dedup, compression changes, BA2 file tables, and crash recovery. | Support read existing + write new archive first; revisit transactional in-place update only after compatibility hardening. |
| Whole-archive memory loading | Simpler implementation. | Fails large Starfield/FO4 use cases and contradicts performance goals. | Bounded streaming readers/writers with scratch buffers. |
| Silent best-effort extraction on corrupt archives | Users may prefer getting “something.” | Can hide corruption and create invalid assets. | Provide explicit recovery/lenient mode later; default should fail with precise diagnostics. |
| Global configuration/singletons | Convenient for compression levels/logging. | Breaks thread safety and embedding. | Pass options explicitly through archive open/build contexts. |
| Automatic format inference from output extension only | Convenient for writer API. | `.bsa`/`.ba2` is insufficient for game/version/DDS distinctions and can produce invalid archives. | Require explicit target profile; use extension only as validation hint. |
| Shelling out to Archive2/BSArch/texconv | Quickly obtains compatibility. | Not reusable, not portable, error-prone, and introduces external tool dependencies. | Implement native library behavior and compare against tool outputs in tests. |
| Default compression of every file | Smaller archives look attractive. | BSArch warns that some sounds/voices/strings can fail in compressed archives; game-specific rules matter. | Compatibility profiles with per-file default decisions and warnings. |

## Feature Dependencies

```text
Embeddable C++20 API
    ├──requires──> Structured result/error model
    ├──requires──> Virtual archive path model
    └──requires──> No public dependency leakage

Archive auto-detection
    └──requires──> Header parsing primitives
        ├──enables──> Variant-specific index parsing
        └──enables──> Inspection metadata

Read support
    ├──requires──> Variant-specific index parsing
    ├──requires──> Hash/path normalization
    ├──requires──> Compression codecs
    └──enables──> Random-access extraction
                     ├──enables──> Streaming extraction
                     ├──enables──> Bulk extraction
                     └──enables──> Compatibility fixture validation

BA2 DDS read support
    ├──requires──> BA2 record/chunk parsing
    ├──requires──> Compression codecs
    └──requires──> DDS header reconstruction
        └──enables──> Texture inspection metadata

Write support
    ├──requires──> Hash/path normalization
    ├──requires──> Serialization primitives
    ├──requires──> Compression codecs
    ├──requires──> Flag derivation
    └──requires──> Round-trip read support

BA2 DDS write support
    ├──requires──> DirectXTex-backed DDS analysis boundary
    ├──requires──> DDS mip/chunk planning
    └──requires──> BA2 writer finalization

Data deduplication
    ├──requires──> Streaming/content hash computation
    └──enhances──> Write support

Parallel packing/extraction
    ├──requires──> Correct single-threaded read/write
    ├──requires──> Isolated object state
    └──enhances──> Streaming extraction and writer pipelines

Deterministic output mode
    ├──requires──> Stable sorting and serialization
    ├──requires──> Deterministic compression settings
    └──enhances──> Compatibility and round-trip tests

In-place mutation
    ├──conflicts-with──> Early write-new safety model
    └──defer-until──> Compatibility hardening and transactional design
```

### Dependency Notes

- **Read support must precede write support:** Writers need readers for round-trip verification, fixture inspection, and compatibility debugging.
- **DDS read should precede DDS write:** Correct reconstruction and metadata validation are prerequisites for safe mip/chunk planning.
- **Compression codecs are shared infrastructure:** Deflate, LZ4 frame, and LZ4 block wrappers should be built once and reused by every read/write path.
- **Path normalization is foundational:** Lookup, hash generation, duplicate detection, sorting, and writer compatibility all depend on stable archive-path semantics.
- **Parallelism should be late:** Multithreading amplifies race conditions and non-determinism; implement after single-threaded compatibility is proven.
- **In-place mutation conflicts with early safety:** It should not share milestones with initial writer support because it needs transactional failure semantics and complex offset rewriting.

## MVP Definition

### Launch With (v1)

Minimum viable library capability to validate the core concept with downstream consumers.

- [ ] Embeddable C++20 API with local result/error model — essential for library adoption.
- [ ] Auto-detect and read TES4-family BSA archives, including deflate and SSE LZ4 frame extraction — high-value first compatibility slice.
- [ ] List entries, query metadata, check existence, and extract individual files by path/handle — minimum archive access surface.
- [ ] Streaming extraction with bounded memory — prevents early architecture from becoming unusable for large archives.
- [ ] Focused tests for headers, hashes, compression, extraction fixtures, and BSArchPro-compatible output — proves the compatibility premise.

### Add After Validation (v1.x)

Features to add once the API and first reader slice are working.

- [ ] TES3 read support — completes BSA reader family coverage for Morrowind.
- [ ] BA2 GNRL read support for FO4/Starfield — unlocks modern game general archives.
- [ ] BA2 DDS read support with DDS reconstruction — unlocks texture archives and validates DirectXTex boundary.
- [ ] TES4-family write support — first writer phase after read/round-trip infrastructure exists.
- [ ] BA2 GNRL write support — needed for FO4/Starfield packaging workflows.
- [ ] BA2 DDS write support — needed for texture packaging workflows; defer until DDS read is proven.
- [ ] TES3 write support — valuable for completeness, lower dependency than BA2 DDS but less urgent for modern pipelines.
- [ ] Compatibility warnings API — add once enough quirks are known from fixtures and BSArchPro tracing.

### Future Consideration (v2+)

Features to defer until full correctness and compatibility are established.

- [ ] Parallel packing/extraction — high value for large archives, but unsafe before deterministic single-threaded correctness.
- [ ] Benchmark suite and performance tuning knobs — useful after baseline algorithms and streaming are stable.
- [ ] Deterministic/reproducible archive mode — valuable for CI and package reproducibility; requires stable writer behavior first.
- [ ] Validation-only API for package plans — useful for asset pipelines after write options mature.
- [ ] In-place archive update — only after transactional rewrite/failure semantics are designed.
- [ ] Fuzzing and hardened lenient recovery modes — best added during compatibility/hardening once parsers are complete.

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Embeddable C++20 API | HIGH | MEDIUM | P1 |
| Auto-detection | HIGH | MEDIUM | P1 |
| TES4/FO3/SSE BSA read/extract | HIGH | HIGH | P1 |
| Query/list/metadata APIs | HIGH | MEDIUM | P1 |
| Streaming extraction | HIGH | HIGH | P1 |
| Compression wrappers | HIGH | MEDIUM-HIGH | P1 |
| Compatibility fixture tests | HIGH | HIGH | P1 |
| TES3 read | MEDIUM | MEDIUM | P2 |
| BA2 GNRL read | HIGH | HIGH | P2 |
| BA2 DDS read/reconstruction | HIGH | HIGH | P2 |
| TES4-family write | HIGH | HIGH | P2 |
| BA2 GNRL write | HIGH | HIGH | P2 |
| BA2 DDS write | HIGH | HIGH | P2 |
| TES3 write | MEDIUM | MEDIUM | P2 |
| Data deduplication | MEDIUM-HIGH | MEDIUM-HIGH | P2 |
| Compatibility warnings | MEDIUM-HIGH | MEDIUM | P2 |
| Deterministic output | MEDIUM-HIGH | MEDIUM-HIGH | P3 |
| Parallel packing/extraction | HIGH | HIGH | P3 |
| Benchmark suite | MEDIUM | MEDIUM | P3 |
| Validation-only API | MEDIUM | MEDIUM-HIGH | P3 |
| In-place mutation | LOW-MEDIUM | HIGH | P3 / defer |

**Priority key:**
- P1: Must have for first viable launch/read slice.
- P2: Required for format-complete v1 and competitive library utility.
- P3: Valuable after correctness, compatibility, and writer coverage are stable.

## Competitor / Ecosystem Feature Analysis

| Feature | BSArch / BSArchPro | Existing libraries / packages | libbsa Approach |
|---------|--------------------|-------------------------------|-----------------|
| Format coverage | Public BSArchPro page lists Morrowind through Starfield, including FO4/SF DDS and FO4 NG/AE versions. | Public C++/Rust packages exist, but public search evidence suggests narrower or separate BSA/BA2 surfaces. | Target all known BSA/BA2 variants behind one reusable C++20 API. |
| View/list/inspect | BSArch supports archive info/list/dump; BSArchPro shows detailed asset/archive info. | Libraries typically expose readers, entries, and metadata. | Provide machine-readable listing, metadata, and warnings; no UI rendering. |
| Extract | BSArch supports full extraction, quiet mode, and multithreaded extraction. | Read libraries expose open/read operations. | Provide random-access and streaming extraction first, then parallel bulk extraction later. |
| Pack/write | BSArch packs target game formats with compression, flags, and shared data. | Some libraries expose writer types, but format completeness varies. | Implement write-new builders for every supported variant with compatibility profiles. |
| DDS texture handling | BSArchPro notes DDS format support, cubemap fixes, FO4/SF texture archives, and texture detail inspection. | Generic archive libraries often avoid texture reconstruction details. | Treat BA2 DDS as first-class with internal DirectXTex analysis and public libbsa texture metadata. |
| Compression controls | BSArch exposes compressed/uncompressed choices and selected SF compression type. | Libraries expose lower-level packed/unpacked data depending on package. | Archive default + per-file override + profile warnings. |
| Shared duplicate data | BSArch/BSArchPro expose shared data / find identical assets. | Not universally available in libraries. | Offer opt-in deduplication as a writer differentiator. |
| Warnings | BSArchPro reports potential packing issues, non-ASCII names, and compression hazards. | Libraries may return errors but often lack compatibility warnings. | Surface structured warnings alongside successful operations. |
| Multithreading | BSArch has multithreaded mode/defaults in recent notes. | Library-level thread guarantees vary. | Defer until correctness; then implement documented, deterministic-safe parallel packing/extraction. |

## Deferred Capabilities and Rationale

| Capability | Defer Until | Rationale |
|------------|-------------|-----------|
| Multi-threaded packing/extraction | After all read/write paths are correct | Parallelism makes bugs harder to reproduce and can threaten deterministic output. |
| In-place mutation | After write-new and hardening phases | Requires transactional safety, free-space/offset management, and corruption recovery design. |
| Fuzzing as a public-supported harness | After parsers stabilize | Valuable for hardening, but early parser shapes will change frequently. |
| CLI test/demo tool | After API stabilizes | Useful for examples, but must not drive core design or become product scope. |
| Lenient recovery extraction | After strict validation exists | Default strict behavior should protect consumers from silently corrupted output. |
| Custom allocator / advanced memory hooks | After performance measurements demand it | Adds API complexity; start with bounded buffers and caller sinks first. |

## Sources

- Project context and requirements: `J:\libbsa-gsd\.planning\PROJECT.md` (HIGH confidence).
- Source PRD: `J:\libbsa-gsd\docs\PRD.md` (HIGH confidence).
- Project constraints and TES5Edit read-only boundary: `J:\libbsa-gsd\AGENTS.md` (HIGH confidence).
- BSArchPro NexusMods page, public feature/changelog text: https://www.nexusmods.com/fallout4/mods/63243 (MEDIUM-HIGH confidence for tool feature expectations).
- BSArch NexusMods page excerpt, public command/features text: https://www.nexusmods.com/newvegas/mods/64745 (MEDIUM confidence; page access may vary by login, excerpt captured through search result).
- TES5Edit/BSArch DeepWiki overview: https://deepwiki.com/TES5Edit/TES5Edit/6.1-bsarch-tool (MEDIUM confidence; useful ecosystem summary, not primary source).
- Ryan-rsm-McKenzie C++ `bsa` library public repository page: https://github.com/Ryan-rsm-McKenzie/bsa (LOW-MEDIUM confidence for ecosystem presence only; not used for format claims).
- Rust `bsa` / `ba2` crates public docs and crates pages: https://docs.rs/bsa/latest/bsa and https://docs.rs/ba2/latest/ba2 (LOW-MEDIUM confidence for ecosystem presence only; not used for project requirements).
- Cathedral Assets Optimizer public repository page: https://github.com/Guekka/Cathedral-Assets-Optimizer/ (LOW-MEDIUM confidence for modding workflow context and DirectXTex ecosystem relevance).

---
*Feature research for: reusable Bethesda BSA/BA2 archive library*
*Researched: 2026-05-07*
