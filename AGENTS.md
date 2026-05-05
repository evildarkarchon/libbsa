# AGENTS.md

## Project Purpose

This repository is for building a reusable C++ library for reading and writing Bethesda Archive formats.

The behavioral reference is the BSArchPro code from the `TES5Edit` git submodule. Treat that code as prior art and compatibility guidance while designing a clean C++ library surface for this project.

Useful reference areas currently include:

- `TES5Edit/BSArchPro.dpr`
- `TES5Edit/BSArch/`
- `TES5Edit/Core/wbBSArchive.pas`
- `TES5Edit/Core/wbBSA.pas`

## Hard Boundary: TES5Edit Is Read-Only

`TES5Edit/` is a read-only reference submodule. Do not modify it for any reason.

This includes:

- Do not edit files under `TES5Edit/`.
- Do not format files under `TES5Edit/`.
- Do not apply generated changes under `TES5Edit/`.
- Do not update the submodule pointer.
- Do not stage or commit changes inside `TES5Edit/`.
- Do not treat the submodule as vendored source to be compiled into this project.

All implementation work belongs outside `TES5Edit/`.

## Language and Implementation Direction

- The implementation language is C++.
- Keep the library reusable and independent of application-specific UI or tooling.
- Prefer clear, portable C++ interfaces over direct transliteration of Delphi/Pascal structure.
- Preserve archive-format behavior discovered from BSArchPro unless there is a documented reason to diverge.
- When porting behavior, trace the reference code first and record non-obvious compatibility constraints near the new implementation.

## Dependencies

Deflate and LZ4 compression/decompression support are required.

- Do not introduce external dependencies speculatively.
- Prefer the C++ standard library until a real format, compression, filesystem, testing, or packaging requirement justifies more.
- Use `libdeflate` for deflate compression and decompression.
- Use the official `lz4` library for LZ4 compression and decompression.
- Use `DirectXTex` for texture analysis.
- Use `vcpkg` for dependency management.
- If a dependency becomes useful, document the need, the alternatives considered, and the expected project impact before adding it.

## Comments and Documentation

- Never delete an accurate comment as cleanup. Remove or rewrite a comment only when the code it describes is deleted or has changed enough to make the comment wrong.
- If a comment is removed or rewritten, mention it in the final reply.
- Add comments for non-obvious why: format compatibility constraints, ownership/lifetime decisions, error-handling edge cases, threading behavior, cancellation behavior, and deliberate deviations from the reference implementation.
- Add Doxygen-compliant C++ doc comments (/// or /** ... */) for public APIs and for methods that are added or substantially rewritten.
- Trivial private helpers may omit doc comments when their purpose is obvious.

## Validation Expectations

- Add focused tests for archive parsing, writing, round-tripping, and compatibility behavior as those surfaces are implemented.
- Prefer fixture-based tests that prove byte-level or metadata-level compatibility with known archive behavior.
- Do not use the `TES5Edit/` submodule as a mutable test fixture.

## MCP Server Usage

### Exa (`mcp__exa`)

- `web_search_exa` - Use for general web lookups, articles, blog posts, and non-documentation content.
- `get_code_context_exa` - Prefer for code-related web search, tutorials, examples, and SDK/API context.
- `deep_researcher_start` - Use for complex multi-source research, then poll with `deep_researcher_check`.

### Ref (`mcp__ref`)

- `ref_search_documentation` - Search documentation across the web, GitHub, and private resources. Include the programming language and library/framework name in the query.
- `ref_read_url` - Read a URL returned by `ref_search_documentation`. Pass the exact URL, including any `#hash`.

### Context7 (`mcp__context7`)

- `resolve-library-id` - Call before `query-docs` unless an explicit `/org/project` ID is provided.
- `query-docs` - Retrieve current documentation and examples for the resolved library ID.
- Do not call Context7 tools more than three times per question.

### Tool Choice

- Official Microsoft/Azure docs: use Microsoft Learn MCP tools first.
- Library or framework docs: use Context7 or Ref.
- Code-related web search: use Exa `get_code_context_exa`.
- General web search: use Exa `web_search_exa`.
- Deep research: use Exa `deep_researcher_start`.

<!-- GSD:project-start source:PROJECT.md -->
## Project

**libbsa**

libbsa is a reusable C++20 library for reading and writing Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. It is intended for modding tools, asset pipelines, and game utilities that need archive access without UI coupling or Delphi/BSArchPro implementation details leaking into the public API.

The project reimplements BSArchPro-compatible behavior using clean, portable C++ interfaces. TES5Edit is the behavioral reference and compatibility guide, but the library implementation remains independent and lives outside the `TES5Edit/` submodule.

**Core Value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

### Constraints

- **Language**: C++20 - implementation must expose idiomatic, reusable C++ interfaces rather than transliterated Delphi structure.
- **Reference boundary**: `TES5Edit/` is read-only - it may guide behavior but must not be edited, formatted, staged, or compiled into libbsa.
- **Dependencies**: Use `libdeflate`, official `lz4`, and `DirectXTex` via vcpkg - no other external dependencies without documented justification.
- **Portability**: Windows is the primary target, but platform-specific code should be minimized to preserve a future Linux/macOS path.
- **API design**: Public headers should remain minimal and avoid leaking platform, compression, or DirectXTex implementation details.
- **State model**: No global mutable state or singleton-based behavior; thread safety should come from isolated objects and explicit ownership.
- **Error model**: Prefer `std::expected` or explicit error codes for I/O and format failures; reserve exceptions for programmer precondition violations.
- **Performance**: Large archive support must use streaming I/O, with full multi-threaded packing/extraction deferred until the dedicated performance phase.
<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->
## Technology Stack

## Recommended Stack
### Core Technologies
| Technology | Version / Policy | Purpose | Why Recommended | Confidence |
|------------|------------------|---------|-----------------|------------|
| C++ | C++20 language mode; avoid C++23-only public API until the project intentionally raises the standard | Public API and implementation language | Matches project constraints while giving `std::span`, `std::endian`, `std::jthread`, concepts, and value-oriented interfaces. Do **not** expose `std::expected` yet in the public API because it is C++23, not C++20. Use a local `libbsa::result<T>` or explicit error-code API until a C++23 migration is deliberate. | HIGH |
| CMake | Minimum `3.24`; test current CMake `4.3.x` in CI/presets | Cross-platform build, install/export package, CTest integration | CMake is the standard for reusable C++ libraries and vcpkg integration. `3.24` is old enough for broad tool availability but modern enough for clean target/file-set based installs. Current docs show CMake `4.3.2`, so CI should keep a latest-CMake lane to catch policy changes early. | HIGH |
| vcpkg | Manifest mode with committed `vcpkg.json` and `vcpkg-configuration.json` / `builtin-baseline`; update baseline per milestone | Dependency acquisition and reproducible package graph | Microsoft docs recommend manifest mode for most projects; it isolates dependencies per project and is required for versioning. A committed baseline gives repeatable libdeflate/lz4/DirectXTex/Catch2 versions without vendoring dependencies. | HIGH |
| TES5Edit / BSArchPro | Read-only git submodule/reference only | Behavioral compatibility oracle | Required project constraint. Use `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas` to trace format behavior, but never compile, edit, format, stage, or vendor TES5Edit code. | HIGH |
### Required Runtime Libraries
| Library | Current vcpkg Version / Policy | Purpose | When to Use | Why Recommended | Confidence |
|---------|--------------------------------|---------|-------------|-----------------|------------|
| libdeflate | vcpkg `libdeflate` `1.25#0` (package last updated 2025-11-03); use features `compression` and `decompression`; add `zlib` only if compatibility tests prove Bethesda payloads require zlib-wrapped streams | Deflate compression/decompression for TES4/FO3/Skyrim LE BSA and FO4/SF BA2 deflate payloads | Every deflate read/write path | It is purpose-built for fast whole-buffer DEFLATE, maps directly to archive payload chunks, and is already a hard project requirement. Prefer whole-buffer wrappers that validate expected output size before returning data. | HIGH |
| lz4 | vcpkg `lz4` `1.10.0#0` (package last updated 2024-07-25); link official `lz4::lz4` | LZ4 frame and raw block compression/decompression | SSE BSA LZ4 frame paths; Starfield BA2 v3 raw LZ4 block paths | Official LZ4 provides both `LZ4F_*` frame APIs and `LZ4_*` block APIs. libbsa needs both, and the separate code paths reduce the risk of frame-vs-block corruption. | HIGH |
| DirectXTex | vcpkg `directxtex` `2026-03-31#0` (package last updated 2026-04-01); request core library only unless tools are intentionally needed | DDS metadata parsing, DXGI format interpretation, mip analysis, DDS header reconstruction support | BA2 DDS read/write phases only; keep behind an internal texture-analysis boundary | DirectXTex is maintained by Microsoft, available through vcpkg, and its March 2026 release made mip-level helpers public and improved permissive DDS handling. It is the right choice for Bethesda DDS metadata work, but its platform surface must be isolated because some auxiliary paths are Windows-oriented. | HIGH for Windows, MEDIUM for Linux/macOS path |
### Supporting Development Libraries and Tools
| Tool / Library | Version / Policy | Purpose | When to Use | Why Recommended | Confidence |
|----------------|------------------|---------|-------------|-----------------|------------|
| Catch2 | vcpkg `catch2` `3.14.0#0` (package last updated 2026-04-06) | Unit, fixture, round-trip, and compatibility tests | Start in Milestone 1 | Catch2 is C++-native, lightweight for library tests, integrates with CTest via `catch_discover_tests`, and has current thread-safe assertion support for later parallel phases. Prefer it over GoogleTest unless mocking becomes a real need. | HIGH |
| CTest | Bundled with CMake | Test orchestration | All test phases | Keeps test execution build-system-native and avoids a separate runner dependency. Use labels for `unit`, `fixture`, `roundtrip`, `compat`, and optional `slow`. | HIGH |
| CMakePresets.json | Schema compatible with minimum CMake; include `windows-msvc-vcpkg`, `linux-clang-vcpkg` later | Repeatable configure/build/test workflows | From project foundation | Presets encode toolchain file, triplet, build type, warnings, and sanitizer lanes without requiring developers to memorize configure flags. | HIGH |
| Doxygen | Acquire from system package, GitHub Actions, or vcpkg only when doc generation is added | Public API docs | Milestone 10 or earlier if API stabilizes | Project requires Doxygen-style comments. Generate docs in CI after public headers settle; do not make Doxygen a runtime or library dependency. | MEDIUM |
| Sanitizers | Compiler-provided ASan/UBSan on Clang/GCC; MSVC ASan where practical | Memory safety validation | Parser and decompressor hardening phases | Archive parsers handle untrusted binary data. Sanitizers should run against malformed fixtures and fuzz seeds, but should not affect release artifacts. | HIGH |
## Installation / Baseline Shape
## CMake Project Layout Recommendation
- `compression/deflate_codec.*` wraps libdeflate allocation, exact-size decompression, and compression levels.
- `compression/lz4_frame_codec.*` wraps SSE frame payload handling.
- `compression/lz4_block_codec.*` wraps Starfield BA2 raw block handling.
- `texture/dds_analyzer.*` wraps DirectXTex metadata and mip/chunk calculations.
## Alternatives Considered
| Recommended | Alternative | Why Not / When Alternative Makes Sense | Confidence |
|-------------|-------------|----------------------------------------|------------|
| CMake + vcpkg manifest mode | Meson, Bazel, Premake, raw Visual Studio solutions | These can build C++, but CMake + vcpkg is the most common portable library distribution path and aligns with vcpkg package exports. Use another build system only if a downstream consumer requires it later. | HIGH |
| libdeflate | zlib/miniz/zlib-ng | zlib is slower and not required unless archive bytes prove zlib wrapper compatibility is needed. miniz is attractive as a single-file fallback but violates the no-speculative-dependency constraint. zlib-ng is unnecessary while libdeflate satisfies the archive payload requirement. | HIGH |
| official lz4 | bundled LZ4 source, game-specific reimplementation | Official lz4 has stable frame and block APIs and active maintenance. Reimplementing LZ4 risks silent corruption; vendoring adds update/security burden. | HIGH |
| DirectXTex | Hand-written DDS parser, DirectXTK-only utilities, texconv CLI invocation | BA2 DDS needs robust DXGI/mip metadata, not shelling out to tools. A small hand parser may be enough for read-only extraction but will become fragile for write/chunking. DirectXTex should still be isolated so a future non-Windows adapter can replace it if needed. | MEDIUM-HIGH |
| Catch2 | GoogleTest | GoogleTest is excellent for large ecosystems and mocks; libbsa mostly needs data-driven parser/round-trip tests. Catch2 keeps test code terse and dependency surface smaller. Switch only if mocks or existing org standards require GoogleTest. | MEDIUM |
## What NOT to Use
| Avoid | Why | Use Instead | Confidence |
|-------|-----|-------------|------------|
| Editing or compiling TES5Edit code | Violates the hard project boundary and risks Delphi/UI coupling in the library | Treat TES5Edit as read-only behavioral reference and write clean C++ outside `TES5Edit/` | HIGH |
| `std::expected` in a C++20 public API | `std::expected` is standardized in C++23; exposing it while claiming C++20 creates portability and ABI/API confusion | Local `libbsa::result<T>` or explicit `error_code` + output parameter; reconsider when moving to C++23 | HIGH |
| `std::filesystem::path` as the archive path type | Bethesda archive paths are normalized virtual paths, not host OS paths; platform path rules can corrupt separators/case expectations | Store archive paths as normalized UTF-8/byte strings with explicit normalization rules; use filesystem paths only for host I/O boundaries | HIGH |
| LZ4 frame API for Starfield BA2 v3 raw LZ4 blocks | Frame and block formats are different; using the wrong API can silently fail or corrupt output | Route by archive format/version/`CompressionMethod`: `LZ4F_*` for SSE frames, `LZ4_*` safe block APIs for BA2 raw blocks | HIGH |
| DirectXTex types in public headers | Leaks Windows/DXGI implementation details and makes future portability harder | Internal `dds_metadata` / `texture_layout` value types translated from DirectXTex internally | HIGH |
| External logging/formatting libraries by default (`spdlog`, `fmt`) | Not required for a reusable archive library and violates the minimal-dependency constraint | Return structured errors; let consumers log/format as they choose | HIGH |
| `Boost`, `libarchive`, ZIP/7z libraries | They do not implement Bethesda BSA/BA2 semantics and add large dependency/API surface | Purpose-built parsers/writers for BSA/BA2 | HIGH |
| Whole-archive memory loading as the primary design | Starfield archives can be very large; whole-file reads break performance and memory goals | Streaming file/sink abstractions and bounded scratch buffers | HIGH |
| In-place archive mutation in early milestones | Hard to make safe across shifting file tables, compression, and deduplication; not required until polish | Open-read-write-new archive flow; defer in-place updates | HIGH |
## Stack Patterns by Archive Variant
- Use standard C++ binary I/O and hash/path utilities only.
- No compression or DirectXTex required.
- Keep offset math isolated because TES3 offsets are data-section-relative.
- Use libdeflate for deflate payloads.
- Keep embedded-name handling and hash-ordering in format-specific code.
- Do not add zlib unless compatibility fixtures demonstrate wrapped streams are required.
- Use official lz4 frame APIs (`LZ4F_*`) only for compressed payloads.
- Keep this separate from Starfield raw block LZ4 helpers.
- Use libdeflate for FO4 and Starfield v2 / non-LZ4 v3 payloads.
- Use official lz4 block APIs (`LZ4_*safe*`) when Starfield v3 `CompressionMethod == 3`.
- Encode compression method as explicit metadata; never infer solely from file extension.
- Use DirectXTex only through an internal DDS analyzer for dimensions, DXGI format, mip count, array/cubemap metadata, and mip chunk planning.
- Use libdeflate or lz4 block according to archive version and chunk metadata.
- Persist libbsa-native metadata, not DirectXTex objects.
## Version Compatibility
| Component | Compatible With | Notes | Confidence |
|-----------|-----------------|-------|------------|
| CMake `3.24+` | vcpkg toolchain, Catch2 3.x, DirectXTex current CMake package | DirectXTex's 2025 release notes mention CMake minimum `3.21`; choosing `3.24` leaves room for modern install/export patterns. Current CMake docs are `4.3.2`, so test latest too. | HIGH |
| vcpkg manifest mode | libdeflate `1.25`, lz4 `1.10.0`, DirectXTex `2026-03-31`, Catch2 `3.14.0` | Commit baseline rather than hand-pinning every dependency. Use `version>=` only for known minimums and `overrides` only to force a problematic port version. | HIGH |
| DirectXTex `2026-03-31` | Windows desktop / VS 2022 and VS 2026; vcpkg package supports Windows and Linux | March 2026 release retires VS 2019 projects and adds VS 2026 support. Keep CI focused on VS 2022/2026 for DirectXTex-heavy phases; treat Linux/macOS DDS support as a validation item. | MEDIUM |
| libdeflate `1.25` | All vcpkg triplets | vcpkg package supports all triplets and exposes CMake targets. Whole-buffer API fits archive chunk payloads. | HIGH |
| lz4 `1.10.0` | All vcpkg triplets | vcpkg package supports all triplets and exposes `lz4::lz4`. Use stable library APIs, not CLI behavior. | HIGH |
| Catch2 `3.14.0` | CMake/CTest | Use `Catch2::Catch2WithMain` for tests and `catch_discover_tests` once test executables exist. | HIGH |
## CI / Toolchain Recommendation
## Sources
- Context7 `/kitware/cmake` — verified target-based C++20 and install/export patterns.
- Context7 `/microsoft/vcpkg` and Microsoft Learn — verified manifest mode, CMake toolchain integration, `builtin-baseline`, `version>=`, and `overrides` behavior: https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode and https://learn.microsoft.com/en-us/vcpkg/users/versioning
- CMake official docs — current release documentation shows CMake `4.3.2`: https://cmake.org/cmake/help/latest/release/index.html
- vcpkg package page — `libdeflate` `1.25#0`, features, all-triplet support, MIT license, last updated 2025-11-03: https://vcpkg.io/en/package/libdeflate.html
- libdeflate GitHub releases — latest `v1.25`: https://github.com/ebiggers/libdeflate/releases
- vcpkg package page — `lz4` `1.10.0#0`, all-triplet support, BSD-2-Clause license: https://vcpkg.io/en/package/lz4.html
- LZ4 GitHub releases — `v1.10.0` official release, stable library notes, frame/block API context: https://github.com/lz4/lz4/releases
- Context7 `/microsoft/directxtex` and DirectXTex wiki — verified DDS load/metadata APIs (`LoadFromDDSMemory`, `LoadFromDDSFile`, `TexMetadata`, `ScratchImage`).
- DirectXTex GitHub releases — March 2026 release, VS 2026 support, public mip helpers, vcpkg availability: https://github.com/microsoft/DirectXTex/releases
- vcpkg package page — `directxtex` `2026-03-31#0`, features, Windows/Linux support, last updated 2026-04-01: https://vcpkg.io/en/package/directxtex.html
- vcpkg usage files — verified CMake targets for `libdeflate`, `lz4`, and `Microsoft::DirectXTex`.
- vcpkg/Catch2 package page and Catch2 GitHub releases — `catch2` `3.14.0#0`, current release fixes and CTest integration context: https://vcpkg.io/en/package/catch2.html and https://github.com/catchorg/Catch2/releases
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

Architecture not yet mapped. Follow existing patterns found in the codebase.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->

<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
