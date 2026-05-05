# libbsa

## What This Is

libbsa is a reusable C++20 library for reading and writing Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. It is intended for modding tools, asset pipelines, and game utilities that need archive access without UI coupling or Delphi/BSArchPro implementation details leaking into the public API.

The project reimplements BSArchPro-compatible behavior using clean, portable C++ interfaces. TES5Edit is the behavioral reference and compatibility guide, but the library implementation remains independent and lives outside the `TES5Edit/` submodule.

## Core Value

libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

## Requirements

### Validated

<!-- Shipped and confirmed valuable. -->

(None yet - ship to validate)

### Active

<!-- Current scope. Building toward these. -->

- [ ] Support archive auto-detection from magic bytes, version, and format markers.
- [ ] Read and extract TES4-family BSA archives for Oblivion, Fallout 3/New Vegas, Skyrim LE, and Skyrim SE/AE.
- [ ] Read and extract TES3 Morrowind BSA archives.
- [ ] Read and extract Fallout 4 and Starfield BA2 GNRL archives.
- [ ] Read and extract Fallout 4 and Starfield BA2 DDS archives with DDS header reconstruction.
- [ ] Write TES4-family BSA archives compatible with target game engines.
- [ ] Write Fallout 4 and Starfield BA2 GNRL archives.
- [ ] Write Fallout 4 and Starfield BA2 DDS archives with proper mipmap chunking.
- [ ] Write TES3 Morrowind BSA archives.
- [ ] Provide reusable archive inspection APIs for listing paths, checking existence, metadata retrieval, and archive summary information.
- [ ] Provide streaming I/O paths for large archives and avoid requiring whole-archive memory loading.
- [ ] Add multi-threaded packing and extraction for large archive workloads.
- [ ] Provide comprehensive compatibility, round-trip, fixture, and edge-case tests.
- [ ] Document the public API and thread-safety/error-handling guarantees.

### Out of Scope

<!-- Explicit boundaries. Includes reasoning to prevent re-adding. -->

- GUI or CLI tool - consumers build their own applications around the library.
- Network or URL-based archive access - archive operations are local I/O concerns for this library.
- Non-Bethesda archive formats such as ZIP or 7z - format scope is limited to BSA and BA2.
- Direct compilation or modification of TES5Edit source - TES5Edit is read-only reference material, not vendored implementation code.

## Context

The behavioral reference is BSArchPro inside the `TES5Edit/` submodule, especially `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas`. Format behavior should be traced from that prior art when implementing compatibility-sensitive paths.

Supported archive families include TES3 BSA, TES4/FO3/SSE BSA, Fallout 4 BA2 GNRL/DDS, and Starfield BA2 GNRL/DDS. Compression behavior differs by format: TES4/FO3 use deflate, SSE uses LZ4 frame, Fallout 4 BA2 uses deflate, Starfield BA2 v2 GNRL uses deflate, and Starfield BA2 v3 may use raw LZ4 block depending on `CompressionMethod`.

Important archive-specific behavior includes TES3 data-section-relative offsets, TES4-family hash algorithms, FO4 CRC32-based hashes, BA2 file tables at `FileTableOffset`, BA2 DDS chunk records, and DDS header reconstruction for texture extraction. Known hardening targets include malformed archive handling, SSE `EMBEDNAME` crash bug avoidance, sounds-in-compressed warnings, and Fallout vanilla zlib bug tolerance.

Testing needs to prove compatibility at byte or metadata level. The preferred approach is focused fixtures, generated small archives where feasible, round-trip pack/extract comparisons, and compatibility comparisons against BSArchPro output for the same archive corpus.

## Constraints

- **Language**: C++20 - implementation must expose idiomatic, reusable C++ interfaces rather than transliterated Delphi structure.
- **Reference boundary**: `TES5Edit/` is read-only - it may guide behavior but must not be edited, formatted, staged, or compiled into libbsa.
- **Dependencies**: Use `libdeflate`, official `lz4`, and `DirectXTex` via vcpkg - no other external dependencies without documented justification.
- **Portability**: Windows is the primary target, but platform-specific code should be minimized to preserve a future Linux/macOS path.
- **API design**: Public headers should remain minimal and avoid leaking platform, compression, or DirectXTex implementation details.
- **State model**: No global mutable state or singleton-based behavior; thread safety should come from isolated objects and explicit ownership.
- **Error model**: Prefer `std::expected` or explicit error codes for I/O and format failures; reserve exceptions for programmer precondition violations.
- **Performance**: Large archive support must use streaming I/O, with full multi-threaded packing/extraction deferred until the dedicated performance phase.

## Key Decisions

<!-- Decisions that constrain future work. Add throughout project lifecycle. -->

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Build a reusable library only, not a GUI or CLI | Keeps libbsa embeddable in modding tools, asset pipelines, and utilities | - Pending |
| Treat BSArchPro/TES5Edit as behavioral reference only | Preserves compatibility while avoiding UI coupling and Delphi idioms in the implementation | - Pending |
| Use C++20 with value-oriented APIs | Supports portable, modern interfaces with clear ownership and no raw allocation requirements | - Pending |
| Use vcpkg-provided `libdeflate`, `lz4`, and `DirectXTex` | These dependencies directly map to required compression and DDS capabilities | - Pending |
| Defer full multi-threading until after core read/write coverage | Compatibility and correct format behavior must be established before optimizing parallel workloads | - Pending |

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
*Last updated: 2026-05-05 after initialization*
