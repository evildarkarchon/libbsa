# libbsa

## What This Is

libbsa is a reusable C++20 library for reading and writing Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. It is for modding tools, asset pipelines, and game utilities that need archive access without UI coupling or Delphi/BSArchPro implementation details leaking into the public API.

The library reimplements BSArchPro-compatible behavior using clean, portable C++ interfaces. TES5Edit is the behavioral reference and compatibility guide, but the implementation remains independent and lives outside the `TES5Edit/` submodule.

## Core Value

libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

## Requirements

### Validated

<!-- Shipped and confirmed valuable. -->

- [x] Phase 07 validated TES4-family BSA write-new support for public writer construction, target profiles, raw and compressed payloads, embedded names, deduplication, and reopen/extract round-trips.
- [x] Phase 10 validated TES3/Morrowind BSA write-new support for public disk/memory writer APIs, byte-accurate serialization, reader-backed round-trip extraction, synthetic fixture evidence, embedded-NUL path rejection, and no-replace publish semantics.
- [x] Phase 12 validated bounded-memory extraction and writer finalization, opt-in parallel worker execution, benchmark reporting, public API documentation, thread-safety guidance, and compile-checked consumer examples.

### Active

<!-- Current scope. Building toward these. -->

- [x] v1 roadmap requirements are complete as of Phase 12 verification; future active scope should come from the next milestone or v2 requirement promotion.

### Out of Scope

<!-- Explicit boundaries. Includes reasoning to prevent re-adding. -->

- GUI or CLI application - consumers will build their own tools on top of the library.
- Network or URL-based archive access - archive I/O is local/stream based for v1.
- Non-Bethesda archive formats such as ZIP, 7z, or generic libarchive support - the value is Bethesda-specific compatibility.
- Editing or compiling TES5Edit code - TES5Edit is read-only behavioral reference material only.
- In-place archive mutation in early milestones - open/read/write-new flows are safer while format support is being established.
- External dependencies beyond libdeflate, official lz4, DirectXTex, Catch2, CMake, and vcpkg unless a documented requirement justifies them.

## Context

The behavioral reference is BSArchPro inside the `TES5Edit/` submodule, especially `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas`. That submodule must remain read-only and must not be staged, formatted, compiled into libbsa, or modified.

Supported archive families include TES3 BSA, TES4 BSA v103, FO3/FNV/Skyrim LE BSA v104, Skyrim SE/AE BSA v105, Fallout 4 BA2 GNRL/DX10 variants, and Starfield BA2 GNRL/DX10 variants. Starfield BA2 v2 general archives add `Unknown1` and `Unknown2`; Starfield BA2 v3 adds `CompressionMethod`, where observed method `3` selects raw LZ4 block compression and other observed values retain deflate behavior.

Read support must cover format detection, folder/file index parsing, path/hash lookup, random-access extraction, streaming extraction, bulk iteration, transparent decompression, and DDS reconstruction. Write support must cover archive creation, adding files from disk or memory, per-file compression override, content deduplication, automatic flags, DDS mipmap chunking, and finalization.

Testing must prove byte-level and metadata-level compatibility. Expected coverage includes unit tests for hashing, compression, header parsing, and serialization; integration tests with fixture archives; round-trip tests; compatibility comparisons against BSArchPro output; and optional fuzzing in hardening phases.

## Constraints

- **Language**: C++20 - implementation must expose idiomatic, reusable C++ interfaces rather than transliterated Delphi structure.
- **Reference boundary**: `TES5Edit/` is read-only - it may guide behavior but must not be edited, formatted, staged, or compiled into libbsa.
- **Dependencies**: Use `libdeflate`, official `lz4`, and `DirectXTex` via vcpkg - no other external dependencies without documented justification.
- **Build and tests**: Use CMake, vcpkg manifest mode, Catch2, and CTest for repeatable library builds and validation.
- **Portability**: Windows is the primary target, but platform-specific code should be minimized to preserve a future Linux/macOS path.
- **API design**: Public headers should remain minimal and avoid leaking platform, compression, or DirectXTex implementation details.
- **State model**: No global mutable state or singleton-based behavior; thread safety should come from isolated objects and explicit ownership.
- **Error model**: In C++20 public APIs, prefer a local `libbsa::result<T>` or explicit error-code style for I/O and format failures; reserve exceptions for programmer precondition violations.
- **Performance**: Large archive support uses streaming I/O and bounded scratch buffers, with opt-in bounded parallel packing/extraction validated in Phase 12.

## Key Decisions

<!-- Decisions that constrain future work. Add throughout project lifecycle. -->

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Build a reusable library only, not a CLI or GUI | The core value is embeddable archive functionality for downstream tools | - Pending |
| Treat TES5Edit/BSArchPro as read-only reference material | Preserves clean ownership, avoids Delphi/UI coupling, and respects the submodule boundary | - Pending |
| Use C++20 with CMake and vcpkg | Matches project constraints and supports portable reusable library packaging | - Pending |
| Use libdeflate for deflate payloads | Required dependency and a good fit for archive chunk compression/decompression | - Pending |
| Use official lz4 for both frame and raw block paths | SSE BSA uses LZ4 frame while Starfield BA2 v3 uses raw LZ4 blocks; separate APIs reduce corruption risk | - Pending |
| Use DirectXTex only behind an internal texture-analysis boundary | DDS metadata work needs robust DXGI handling without leaking DirectXTex into public headers | - Pending |
| Avoid `std::expected` in the C++20 public API | It is C++23; a local result type or explicit error-code API preserves C++20 portability | - Pending |
| Sequence implementation read-first, then write, then performance/hardening | Correct parsing and extraction are prerequisites for reliable round-trip and compatibility validation | - Pending |
| TES4-family reader state stores host path plus metadata, not whole archive bytes | Phase 03 verification required bounded open/extract behavior for large archives | Implemented in Phase 03 |
| `libbsa::result::error()` reports success-result misuse with `std::logic_error` | Public API precondition mistakes should not terminate the process or return a misleading default error | Implemented in Phase 03 |
| TES4-family BSA writing uses explicit target profiles and writer-owned entry state | Phase 07 needed a stable C++20 writer API with deterministic validation before BA2 writer work builds on it | Implemented in Phase 07 |
| TES4-family stored-byte deduplication is opt-in and operates on final stored bytes | Matching source bytes may encode differently because of compression or embedded-name prefixes, so deduplication must compare the final bytes written to the archive | Implemented in Phase 07 |
| Public parallel worker counts are bounded and result-mapped | Worker-count options are public input, so oversized values and worker startup failures must not escape the `result` error contract | Implemented in Phase 12 |
| BA2 GNRL disk-backed finalization validates prepared source sizes | Disk sources can change between preparation and streaming; finalization and dedupe comparisons must reject growth or truncation before publishing malformed offsets | Implemented in Phase 12 |
| BA2 GNRL publish uses no-replace and rollback helpers | No-overwrite mode must preserve raced destinations, and overwrite failures must report backup restoration failures distinctly | Implemented in Phase 12 |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? -> Move to Out of Scope with reason
2. Requirements validated? -> Move to Validated with phase reference
3. New requirements emerged? -> Add to Active
4. Decisions to log? -> Add to Key Decisions
5. "What This Is" still accurate? -> Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check - still the right priority?
3. Audit Out of Scope - reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-05-10 after Phase 12*
