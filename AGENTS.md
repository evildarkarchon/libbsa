# AGENTS.md

## Project Purpose

This repository is for building a reusable C++ library for reading and writing Bethesda Archive formats.

The behavioral reference is the BSArchPro code from the `TES5Edit` git submodule. Treat that code as prior art and compatibility guidance while designing a clean C++ library surface for this project.

Useful reference areas currently include:

- `TES5Edit/BSArchPro.dpr`
- `TES5Edit/BSArch/`
- `TES5Edit/Core/wbBSArchive.pas`
- `TES5Edit/Core/wbBSA.pas`

## Platform Support

libbsa is a Windows-only library. Development, review, CI, packaging, and dependency validation target Windows with MSVC and vcpkg. Do not raise portability findings or add implementation work solely to support Linux, macOS, POSIX, or general cross-platform behavior unless the user explicitly reopens platform support.

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
- Prefer clear, idiomatic Windows-oriented C++ interfaces over direct transliteration of Delphi/Pascal structure.
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

libbsa is a reusable Windows-only C++20 library for reading and writing Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. It is for modding tools, asset pipelines, and game utilities that need archive access without UI coupling or Delphi/BSArchPro implementation details leaking into the public API.

The library reimplements BSArchPro-compatible behavior using clean, idiomatic C++ interfaces for Windows. TES5Edit is the behavioral reference and compatibility guide, but the implementation remains independent and lives outside the `TES5Edit/` submodule.

**Core Value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

### Constraints

- **Language**: C++20 - implementation must expose idiomatic, reusable C++ interfaces rather than transliterated Delphi structure.
- **Reference boundary**: `TES5Edit/` is read-only - it may guide behavior but must not be edited, formatted, staged, or compiled into libbsa.
- **Dependencies**: Use `libdeflate`, official `lz4`, and `DirectXTex` via vcpkg - no other external dependencies without documented justification.
- **Build and tests**: Use CMake, vcpkg manifest mode, Catch2, and CTest for repeatable library builds and validation.
- **Platform support**: Windows is the only supported target. Reviewers should not request Linux, macOS, POSIX, or cross-platform portability work unless the user explicitly changes this constraint.
- **API design**: Public headers should remain minimal and avoid leaking platform, compression, or DirectXTex implementation details.
- **State model**: No global mutable state or singleton-based behavior; thread safety should come from isolated objects and explicit ownership.
- **Error model**: In C++20 public APIs, prefer a local `libbsa::result<T>` or explicit error-code style for I/O and format failures; reserve exceptions for programmer precondition violations.
- **Performance**: Large archive support must use streaming I/O, with full multi-threaded packing/extraction deferred until the dedicated performance phase.
<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->
## Technology Stack

## Recommended Stack
### Core Technologies
| Technology | Version / Policy | Purpose | Why Recommended | Confidence |
|------------|------------------|---------|-----------------|------------|
| C++ | C++20 language mode; do not expose C++23-only library types in public headers | Library implementation and public API | The PRD requires C++20. C++20 gives `std::span`, `std::endian`, concepts, `std::jthread`, and strong value-oriented interfaces without forcing newer toolchains. Use a local `libbsa::result<T>` or explicit error-code API rather than public `std::expected`, because `std::expected` is C++23. | HIGH |
| CMake | Minimum `3.24`; test latest CMake `4.3.x` in CI | Build system, install/export package generation, CTest orchestration | CMake is the standard distribution path for reusable C++ libraries and integrates cleanly with vcpkg. `3.24` is a pragmatic floor for modern target/file-set/install patterns while remaining broadly available; latest-CMake CI catches policy drift early. | HIGH |
| vcpkg | Manifest mode with committed `vcpkg.json`, `vcpkg-configuration.json`, and a `builtin-baseline` | Reproducible dependency acquisition | Microsoft recommends manifest mode for most projects; it is required for versioning and keeps dependencies project-scoped. A committed baseline gives repeatable libdeflate/lz4/DirectXTex/Catch2 versions without vendoring. | HIGH |
| TES5Edit / BSArchPro | Read-only git submodule / behavioral reference only | Compatibility oracle for Bethesda archive quirks | The project requires BSArchPro-compatible behavior but forbids editing, formatting, staging, compiling, or vendoring `TES5Edit/`. Trace `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas`; implement clean C++ outside the submodule. | HIGH |
### Required Runtime Libraries
| Library | Version / Policy | Purpose | When to Use | Why Recommended | Confidence |
|---------|------------------|---------|-------------|-----------------|------------|
| libdeflate | vcpkg `libdeflate` `1.25#0`; enable `compression` and `decompression`; do not enable `gzip`/`zlib` unless fixtures prove wrapped-stream need | Deflate compression/decompression | TES4/FO3/FNV/Skyrim LE BSA payloads; Fallout 4 BA2; Starfield BA2 v2 and v3 when not LZ4 | libdeflate is optimized for fast whole-buffer DEFLATE, matching BSA/BA2 chunk payloads better than streaming zlib wrappers. Wrap it behind exact-size helpers that fail if decompressed bytes do not match archive metadata. | HIGH |
| lz4 | vcpkg `lz4` `1.10.0#0`; link official `lz4::lz4`; do not depend on the CLI | LZ4 frame and raw block compression/decompression | Skyrim SE/AE BSA uses LZ4 frame APIs; Starfield BA2 v3 `CompressionMethod == 3` uses raw LZ4 block APIs | Official liblz4 exposes both `LZ4F_*` frame APIs and `LZ4_*safe*` block APIs. Keeping separate wrappers for frame-vs-block paths prevents a high-risk class of silent corruption. | HIGH |
| DirectXTex | vcpkg `directxtex` `2026-03-31#0`; use core library, avoid optional image/tool features unless needed | DDS metadata parsing, DXGI format interpretation, mip/cubemap analysis, DDS header reconstruction support | BA2 DX10/DDS read and write phases only | DirectXTex is the maintained Microsoft library for DDS metadata (`GetMetadataFromDDSMemory`, `LoadFromDDSMemory`, `TexMetadata`, `ScratchImage`) and has current vcpkg support. Keep it behind an internal adapter so public headers do not leak DirectX/DXGI or platform details. | HIGH on Windows |
### Development and Validation Tools
| Tool / Library | Version / Policy | Purpose | When to Use | Why Recommended | Confidence |
|----------------|------------------|---------|-------------|-----------------|------------|
| Catch2 | vcpkg `catch2` `3.14.0#0`; consider `thread-safe-assertions` feature before parallel test phases | Unit, fixture, round-trip, compatibility, and regression tests | From Milestone 1 | Catch2 is C++-native, concise for data-driven binary fixture tests, and integrates with CTest via `catch_discover_tests`. Prefer it over GoogleTest unless mocking becomes a concrete requirement. | HIGH |
| CTest | Bundled with CMake | Test orchestration and CI reporting | All milestones | Keeps tests build-system-native. Use labels such as `unit`, `fixture`, `roundtrip`, `compat`, `malformed`, `slow`, and `requires-game-fixture`. | HIGH |
| CMakePresets.json | Schema compatible with CMake `3.24+` | Repeatable Windows configure/build/test workflows | Project foundation | Presets should encode the vcpkg toolchain, build type, and static/shared Windows variants so contributors do not hand-type fragile CMake commands. | HIGH |
| Sanitizers | Compiler-provided ASan/UBSan on Clang/GCC; MSVC ASan where practical | Parser/decompressor hardening | Start with parsing and malformed fixture phases | Archive parsers consume untrusted binary data. Sanitizers should run on malformed headers, oversized sizes, truncated payloads, and decompression failure cases. | HIGH |
| Doxygen | System package or CI/vcpkg tool when docs generation is added | Public API documentation | Once public headers stabilize | Project requires Doxygen comments for public APIs; generate docs in CI later, but do not add it as a runtime dependency. | MEDIUM |
## Installation / Baseline Shape
## CMake Project Layout Recommendation
- `include/libbsa/` — stable public headers only: archive open/read/write API, metadata value types, error/result types, stream/sink abstractions.
- `src/formats/tes3/`, `src/formats/bsa/`, `src/formats/ba2/` — format-specific parsing, hashing, sorting, and serialization.
- `src/compression/deflate_codec.*` — owns libdeflate allocators, compression levels, exact-size decompression, and error translation.
- `src/compression/lz4_frame_codec.*` — owns SSE BSA LZ4 frame handling via `LZ4F_*`.
- `src/compression/lz4_block_codec.*` — owns Starfield BA2 raw block handling via `LZ4_*safe*`.
- `src/texture/dds_analyzer.*` — owns DirectXTex use and converts `TexMetadata` into libbsa-native metadata.
- `tests/fixtures/` — small legal handcrafted archives and metadata manifests; never mutate `TES5Edit/` or rely on it as a writable fixture location.
- `tests/compat/` — optional compatibility tests comparing outputs to BSArchPro-generated golden data.
## Alternatives Considered
| Recommended | Alternative | Why Not / When Alternative Makes Sense | Confidence |
|-------------|-------------|----------------------------------------|------------|
| CMake + vcpkg manifest mode | Meson, Bazel, Premake, raw Visual Studio solutions | These can build C++, but CMake + vcpkg is the selected Windows library distribution path and aligns with vcpkg package exports. Add another build system only for a real downstream integration need. | HIGH |
| libdeflate | zlib, miniz, zlib-ng | zlib is slower and oriented around zlib streams; miniz adds speculative vendored code; zlib-ng is unnecessary while libdeflate satisfies required DEFLATE payloads. Add zlib compatibility only if fixtures prove Bethesda data uses wrapped zlib streams. | HIGH |
| official lz4 | Bundled LZ4 source, game-specific LZ4 reimplementation | Official lz4 has stable frame and block APIs. Reimplementation risks silent corruption; vendoring creates update/security burden. | HIGH |
| DirectXTex behind an adapter | Hand-written DDS parser, DirectXTK utilities, texconv CLI invocation | A hand parser may be tempting for read-only extraction, but BA2 DDS write support needs robust DXGI, mip, array, and cubemap metadata. Shelling out to tools is not suitable for an embeddable library. Keep DirectXTex internal so the public API stays clean. | MEDIUM-HIGH |
| Catch2 + CTest | GoogleTest | GoogleTest is strong for large orgs and mocking-heavy code. libbsa primarily needs fixture-driven parser and round-trip tests, where Catch2 is lighter and terser. Switch only if mocks or org standards become real requirements. | MEDIUM |
## What NOT to Use
| Avoid | Why | Use Instead | Confidence |
|-------|-----|-------------|------------|
| Editing, formatting, compiling, staging, or vendoring `TES5Edit/` | Violates the hard project boundary and risks Delphi/UI coupling in libbsa | Treat TES5Edit/BSArchPro as read-only behavior reference; implement clean C++ outside the submodule | HIGH |
| Public `std::expected` while claiming C++20 | `std::expected` is C++23; exposing it breaks the stated C++20 API contract | `libbsa::result<T>` or explicit `std::error_code`-style APIs; reconsider on an intentional C++23 migration | HIGH |
| `std::filesystem::path` for archive-internal paths | Bethesda virtual paths are normalized archive keys, not host filesystem paths; host separator/case/encoding rules can corrupt lookups and hashes | Store archive paths as normalized UTF-8/byte strings with explicit normalization; use filesystem paths only at host I/O boundaries | HIGH |
| LZ4 frame API for Starfield BA2 v3 raw LZ4 blocks | LZ4 frame and raw block formats are different; wrong API selection can fail or corrupt output | Route by archive family/version/`CompressionMethod`: `LZ4F_*` for SSE frames, `LZ4_*safe*` for Starfield raw blocks | HIGH |
| DirectXTex or DXGI types in public headers | Leaks implementation/platform details into downstream consumers | Internal `dds_metadata` / `texture_layout` value types translated from DirectXTex internally | HIGH |
| External logging/formatting libraries by default (`spdlog`, `fmt`) | Not required by a reusable archive library and violates minimal-dependency constraints | Return structured errors and let consumers log/format however they choose | HIGH |
| Boost, libarchive, ZIP/7z libraries | They do not implement Bethesda BSA/BA2 semantics and add large dependency/API surface | Purpose-built BSA/BA2 parsers/writers | HIGH |
| Whole-archive memory loading as primary design | Starfield archives can be very large; whole-file reads break performance and memory goals | Streaming sources/sinks and bounded scratch buffers | HIGH |
| In-place archive mutation in early milestones | Hard to make safe with shifting tables, compression, DDS chunks, and deduplication | Open/read/write-new archive flow; defer in-place updates to polish/hardening | HIGH |
## Stack Patterns by Archive Variant
- Use standard C++ binary I/O, explicit little-endian reads, TES3 hash/path utilities, and no compression library.
- Keep data-section-relative offset math isolated in TES3 code.
- Use libdeflate for deflate payloads.
- Keep embedded-name handling, archive flags, file flags, and hash-ordering in format-specific code.
- Do not add zlib unless compatibility fixtures demonstrate zlib-wrapped streams are required.
- Use official LZ4 frame APIs (`LZ4F_*`) only.
- Keep this wrapper separate from Starfield raw block LZ4 helpers.
- Use libdeflate for FO4 and Starfield v2 / non-LZ4 v3 payloads.
- Use official raw block APIs (`LZ4_*safe*`) when Starfield v3 `CompressionMethod == 3`.
- Encode compression method as explicit metadata; never infer solely from extension.
- Use DirectXTex only through `dds_analyzer` for dimensions, DXGI format, mip count, array/cubemap metadata, and mip chunk planning.
- Use libdeflate or raw LZ4 block according to archive version and chunk metadata.
- Persist libbsa-native metadata, not DirectXTex objects.
## Version Compatibility
| Component | Compatible With | Notes | Confidence |
|-----------|-----------------|-------|------------|
| CMake `3.24+` | vcpkg toolchain, Catch2 3.x, DirectXTex current CMake package | CMake current docs show `4.3.2`; use `3.24` as the minimum and a latest-CMake lane to catch policy changes. | HIGH |
| vcpkg manifest mode | libdeflate `1.25`, lz4 `1.10.0`, DirectXTex `2026-03-31`, Catch2 `3.14.0` | Commit a baseline. Use `version>=` for known minimums; use `overrides` only to force a problematic package version. | HIGH |
| libdeflate `1.25` | All vcpkg triplets | vcpkg package supports all triplets; whole-buffer API fits archive payload chunks. | HIGH |
| lz4 `1.10.0` | All vcpkg triplets | vcpkg package supports all triplets; library license is BSD-2-Clause. Do not use CLI GPL terms or CLI behavior as library API. | HIGH |
| DirectXTex `2026-03-31` | vcpkg Windows triplets | libbsa is Windows-only; validate DirectXTex through Windows vcpkg/MSVC presets. | HIGH |
| Catch2 `3.14.0` | CMake/CTest | vcpkg package supports all triplets; `catch_discover_tests` has recent fixes and is appropriate for fixture labels. | HIGH |
## CI / Toolchain Recommendation
- Primary lane: Windows + Visual Studio 2026/VS 18.x, vcpkg manifest mode, Debug and Release.
- Secondary lane: Windows + latest CMake `4.3.x` to expose policy warnings early.
- No Linux, macOS, POSIX, or cross-platform portability lane is supported unless the user explicitly reopens platform support.
- Build both static and shared library configurations before publishing an install/export package.
## Sources
- Project context: `J:\libbsa-gsd\.planning\PROJECT.md`, `J:\libbsa-gsd\docs\PRD.md`, `J:\libbsa-gsd\AGENTS.md` — constraints, milestones, TES5Edit boundary, required dependencies.
- Context7 `/kitware/cmake` — verified C++20 target features and install/export package patterns.
- Context7 `/microsoft/vcpkg` and Microsoft Learn — verified manifest mode, `builtin-baseline`, version constraints, overrides, and CMake toolchain behavior: https://learn.microsoft.com/vcpkg/concepts/manifest-mode and https://learn.microsoft.com/vcpkg/users/versioning
- CMake official docs — current release documentation shows CMake `4.3.2`: https://cmake.org/cmake/help/latest/release/index.html
- vcpkg package page — `libdeflate` `1.25#0`, features, all-triplet support, MIT license, last updated 2025-11-03: https://vcpkg.io/en/package/libdeflate.html
- libdeflate GitHub releases — latest `v1.25`: https://github.com/ebiggers/libdeflate/releases
- vcpkg package page — `lz4` `1.10.0#0`, all-triplet support, BSD-2-Clause license, last updated 2024-07-25: https://vcpkg.io/en/package/lz4.html
- LZ4 GitHub releases — `v1.10.0` official release, stable library notes, frame/block API context: https://github.com/lz4/lz4/releases
- Context7 `/microsoft/directxtex` — verified DDS metadata/load APIs (`GetMetadataFromDDSMemory`, `LoadFromDDSMemory`, `TexMetadata`, `ScratchImage`).
- DirectXTex GitHub releases — March 2026 release, public mip helpers, permissive DDS reader update, VS 2026 support, vcpkg availability: https://github.com/microsoft/DirectXTex/releases
- vcpkg package page — `directxtex` `2026-03-31#0`, feature/package metadata, last updated 2026-04-01: https://vcpkg.io/en/package/directxtex.html
- vcpkg package page and Catch2 GitHub releases — `catch2` `3.14.0#0`, thread-safe assertion feature, latest release fixes: https://vcpkg.io/en/package/catch2.html and https://github.com/catchorg/Catch2/releases
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
