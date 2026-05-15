# libbsa

## What This Is

libbsa is a reusable Windows-only C++20 library for reading, writing, validating, and extracting Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. It is for modding tools, asset pipelines, and game utilities that need archive access without UI coupling or Delphi/BSArchPro implementation details leaking into the public API.

The library reimplements BSArchPro-compatible behavior using clean, idiomatic Windows C++ interfaces. TES5Edit remains the behavioral reference and compatibility guide, but the implementation is independent and lives outside the `TES5Edit/` submodule.

## Core Value

libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

## Current Milestone: v1.1 Hardening

**Goal:** Strengthen libbsa's reliability and maintainability by addressing the highest-risk concerns in correctness, internal architecture, hardening coverage, and writer staging/performance without expanding the public product scope.

**Target features:**
- Fix non-ASCII Windows host-path handling for archive open and validation paths.
- Refactor fragile reader/parser/preparer hotspots, including repeated public-reader dispatch and oversized format-specific translation units.
- Reconcile hardening-policy drift and add stronger verification coverage such as sanitizer and/or Release-mode lanes.
- Reduce high-cost or fragile staging paths such as payload dedupe hotspots and BA2 DX10 temp-file lifecycle risk.

## Requirements

### Validated

<!-- Shipped and confirmed valuable. -->

- [x] v1.0 delivered the reusable C++20 package foundation: CMake/vcpkg builds, static/shared library outputs, dependency-light public headers, local `result`/`error_code` APIs, Catch2/CTest validation, and CI/package-consumer smoke coverage.
- [x] v1.0 delivered safe binary, path, hash, payload-streaming, compression, and DDS support behind internal boundaries, including libdeflate, official lz4 frame/raw-block routes, and DirectXTex analysis without public dependency leakage.
- [x] v1.0 delivered read, list, lookup, validation, and extraction support for TES3 BSA, TES4-family BSA v103/v104/v105, Fallout 4 BA2 GNRL/DX10, and Starfield BA2 GNRL/DX10 archive families.
- [x] v1.0 delivered write-new support for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives, including reader-backed round trips, compression routing, safe publish behavior, and legal generated fixture evidence.
- [x] v1.0 delivered structured validation reports, typed compatibility warnings, malformed-input hardening coverage, bounded-memory extraction/finalization, opt-in parallel worker execution, benchmark reporting, Doxygen setup, thread-safety guidance, and compile-checked integration examples.
- [x] v1.1 fixed non-ASCII Windows host-path handling for archive open and validation flows through one shared internal host-file boundary plus representative black-box regression coverage. Validated in Phase 13.
- [x] v1.1 restored truthful hardening verification coverage and aligned planning claims with the supported debug, Release package-proof, and MSVC AddressSanitizer build/test lanes. Validated in Phase 14.
- [x] v1.1 collapsed public reader backend dispatch to one open-time seam while preserving cross-family reader behavior under focused runtime and policy coverage. Validated in Phase 15.
- [x] v1.1 reduced fragility in the targeted TES4 parser and BA2 DX10 preparer hotspots through private table/payload and snapshot/chunk seams with focused runtime and source-policy regression coverage. Validated in Phase 16.
- [x] v1.1 reduced writer hotspot risk by adding exact-equality-preserving TES4 and BA2 GNRL dedupe narrowing, BA2 DX10 ordinary-path snapshot cleanup, lifecycle documentation, and focused Debug/ASan/Release package ship-gate evidence. Validated in Phase 17.

### Active

<!-- Current scope. Building toward these. -->

None — v1.1 hardening requirements are complete as of the Phase 17 ship gate.

### Out of Scope

<!-- Explicit boundaries. Includes reasoning to prevent re-adding. -->

- GUI application - consumers build their own tools on top of the library.
- First-party CLI as core product scope - a CLI can be an example or later tooling deliverable, but it must not shape the core library API.
- Network or URL-based archive access - archive I/O remains local/stream based until a concrete consumer requirement appears.
- Non-Bethesda archive formats such as ZIP, 7z, or generic libarchive support - the value is Bethesda-specific compatibility.
- Editing, formatting, compiling, or staging TES5Edit code - TES5Edit is read-only behavioral reference material only.
- In-place mutation of existing archives - write-new flows are safer while offsets, compression, DDS chunks, and transactional publishing stay correctness-critical.
- External dependencies beyond libdeflate, official lz4, DirectXTex, Catch2, CMake, and vcpkg unless a documented requirement justifies them.

## Context

### Current State

v1.0 shipped on 2026-05-10 after 12 phases, 75 plans, and 81 completed v1 requirements. v1.1 shipped on 2026-05-15 after Phases 13-17 hardened host-path correctness, verification lanes, reader dispatch, parser/preparer seams, and writer hotspots. The live planning surface is now compact: the full v1 roadmap, requirements, milestone audit, and phase execution artifacts are archived under `.planning/milestones/`.

v1.1 shifted focus from feature completeness to hardening work driven by the codebase concerns audit. Phase 13 is complete and verified: archive open, validation, parser entry, and post-open extraction now route through a shared Windows-correct host-file boundary with committed non-ASCII regression proof. Phase 14 is complete and verified: the supported debug inner-loop lanes, Release package-proof lanes, and the MSVC AddressSanitizer hardening lane agree across presets, CI, docs, and planning without rewriting v1.0 history. Phase 15 is complete and verified: `archive_reader::open` now selects one file-local backend table once, then reuses it across listing, lookup, extract, extract_bytes, and bulk extraction while focused runtime and policy suites lock the seam against behavior drift. Phase 16 is complete and verified: the targeted TES4 parser hotspot now routes through private raw-table and payload-descriptor seams, the BA2 DX10 preparer now routes through private snapshot-builder and chunk-assembler seams, and dedicated policy guardrails lock those responsibilities against coordinator collapse. Phase 17 is complete and verified: TES4-family BSA and BA2 GNRL dedupe now use non-authoritative candidate filters before exact stored-byte equality, BA2 DX10 snapshot cleanup runs on ordinary success/failure paths as best-effort cleanup, residual abnormal-termination risk is documented, and the final ship gate passed focused Debug, MSVC AddressSanitizer, and Release package-consumer proof lanes.

The current codebase exposes public reader, writer, validation, result, metadata, and execution-option APIs from `include/libbsa/`. Public headers remain dependency-light and C++20-compatible. Implementation code owns format parsing, archive writing, compression routing, DDS metadata analysis, validation reports, compatibility warnings, bounded-memory streaming, and optional worker-count execution.

The latest audit accepted the milestone with no requirement gaps, no integration gaps, no E2E flow gaps, and no milestone-blocking tech debt. The latest Phase 13 verification snapshot passed the Windows MSVC static build and 362 runnable CTest tests with 2 expected opt-in skips, including the cross-family non-ASCII host-path proof suite. Advisory review still found writer finalize/dedupe host-path migration work, but verification confirmed that gap is outside Phase 13's locked read/open/validate boundary.

### Reference Boundary

The behavioral reference is BSArchPro inside the `TES5Edit/` submodule, especially `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas`. That submodule must remain read-only and must not be staged, formatted, compiled into libbsa, or modified.

### Supported Families

Supported archive families include TES3 BSA, TES4 BSA v103, FO3/FNV/Skyrim LE BSA v104, Skyrim SE/AE BSA v105, Fallout 4 BA2 GNRL/DX10 variants, and Starfield BA2 GNRL/DX10 variants. Starfield BA2 v2 general archives add `Unknown1` and `Unknown2`; Starfield BA2 v3 adds `CompressionMethod`, where observed method `3` selects raw LZ4 block compression and other observed values retain deflate behavior.

## Constraints

- **Language**: C++20 - implementation must expose idiomatic, reusable C++ interfaces rather than transliterated Delphi structure.
- **Reference boundary**: `TES5Edit/` is read-only - it may guide behavior but must not be edited, formatted, staged, or compiled into libbsa.
- **Dependencies**: Use `libdeflate`, official `lz4`, and `DirectXTex` via vcpkg - no other external dependencies without documented justification.
- **Build and tests**: Use CMake, vcpkg manifest mode, Catch2, and CTest for repeatable library builds and validation.
- **Platform support**: Windows is the only supported target. Do not add Linux, macOS, POSIX, or cross-platform portability requirements unless the user explicitly reopens platform support.
- **API design**: Public headers should remain minimal and avoid leaking platform, compression, or DirectXTex implementation details.
- **State model**: No global mutable state or singleton-based behavior; thread safety should come from isolated objects and explicit ownership.
- **Error model**: In C++20 public APIs, prefer a local `libbsa::result<T>` or explicit error-code style for I/O and format failures; reserve exceptions for programmer precondition violations.
- **Performance**: Large archive support uses streaming I/O and bounded scratch buffers, with opt-in bounded parallel packing/extraction validated in Phase 12.

## Key Decisions

<!-- Decisions that constrain future work. Add throughout project lifecycle. -->

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Build a reusable library only, not a CLI or GUI | The core value is embeddable archive functionality for downstream tools | Implemented in v1.0 |
| Treat TES5Edit/BSArchPro as read-only reference material | Preserves clean ownership, avoids Delphi/UI coupling, and respects the submodule boundary | Implemented in v1.0 |
| Use C++20 with CMake and vcpkg | Matches project constraints and supports Windows reusable library packaging | Implemented in v1.0 |
| Treat libbsa as Windows-only | Review agents should not spend effort on Linux, macOS, POSIX, or cross-platform portability concerns | Active after v1.0 |
| Treat the supported verification matrix as a role-based Windows contract | v1.1 Phase 14 needs truthful agreement across quick-path debug work, Release package proof, and the MSVC AddressSanitizer hardening lane | Implemented in Phase 14 |
| Use libdeflate for deflate payloads | Required dependency and a good fit for archive chunk compression/decompression | Implemented in v1.0 |
| Use official lz4 for both frame and raw block paths | SSE BSA uses LZ4 frame while Starfield BA2 v3 uses raw LZ4 blocks; separate APIs reduce corruption risk | Implemented in v1.0 |
| Use DirectXTex only behind an internal texture-analysis boundary | DDS metadata work needs robust DXGI handling without leaking DirectXTex into public headers | Implemented in v1.0 |
| Avoid `std::expected` in the C++20 public API | It is C++23; a local result type or explicit error-code API preserves C++20 portability | Implemented in v1.0 |
| Sequence implementation read-first, then write, then performance/hardening | Correct parsing and extraction are prerequisites for reliable round-trip and compatibility validation | Implemented in v1.0 |
| TES4-family reader state stores host path plus metadata, not whole archive bytes | Phase 03 verification required bounded open/extract behavior for large archives | Implemented in Phase 03 |
| `libbsa::result::error()` reports success-result misuse with `std::logic_error` | Public API precondition mistakes should not terminate the process or return a misleading default error | Implemented in Phase 03 |
| TES4-family BSA writing uses explicit target profiles and writer-owned entry state | Phase 07 needed a stable C++20 writer API with deterministic validation before BA2 writer work builds on it | Implemented in Phase 07 |
| TES4-family stored-byte deduplication is opt-in and operates on final stored bytes | Matching source bytes may encode differently because of compression or embedded-name prefixes, so deduplication must compare the final bytes written to the archive | Implemented in Phase 07 |
| BA2 GNRL writer deduplication is opt-in and disabled by default | Payload sharing can cross correctness boundaries unless selected explicitly | Implemented in Phase 08 |
| BA2 DX10 writer input is DDS-host-file-only and compressed-only at archive level | Keeps the public writer contract narrow and compatible with DirectXTex-backed metadata validation | Implemented in Phase 09 |
| TES3 writer fixture evidence is generated only through the public writer API | Keeps fixture evidence legal and independent of TES5Edit | Implemented in Phase 10 |
| `validate_archive` reuses `archive_reader::open` as the strict parser source of truth | Prevents validation from becoming a lenient second reader | Implemented in Phase 11 |
| Public compatibility warnings expose stable codes and severities, not parser coordinates | Keeps byte offsets, record indexes, and chunk indexes out of the stable API | Implemented in Phase 11 |
| Public parallel worker counts are bounded and result-mapped | Worker-count options are public input, so oversized values and worker startup failures must not escape the `result` error contract | Implemented in Phase 12 |
| BA2 GNRL disk-backed finalization validates prepared source sizes | Disk sources can change between preparation and streaming; finalization and dedupe comparisons must reject growth or truncation before publishing malformed offsets | Implemented in Phase 12 |
| BA2 GNRL publish uses no-replace and rollback helpers | No-overwrite mode must preserve raced destinations, and overwrite failures must report backup restoration failures distinctly | Implemented in Phase 12 |
| Host-file I/O resolves UTF-8 once into `detail::host_file_path` and reopens only from the resolved path | Windows non-ASCII correctness depends on one shared boundary for archive open, validation, parser entry, and post-open extraction instead of repeated narrow-string file opens | Implemented in Phase 13 |
| Keep parser/preparer hotspot extractions private and policy-guarded | Phase 16 needed safer internal change seams without public API expansion or freezing exact helper names | Implemented in Phase 16 |
| Keep dedupe candidate filters non-authoritative | Phase 17 required faster TES4 and BA2 GNRL candidate narrowing without allowing hashes, staged identities, or sizes to replace exact final stored-byte equality | Implemented in Phase 17 |
| Treat BA2 DX10 snapshot cleanup as best-effort ordinary-path cleanup with residual abnormal-termination risk | Phase 17 prioritized prompt cleanup after normal result-returning write attempts while avoiding unsupported crash-proof guarantees or public API expansion | Implemented in Phase 17 |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition**:

1. Requirements invalidated? Move to Out of Scope with reason.
2. Requirements validated? Move to Validated with phase reference.
3. New requirements emerged? Add to Active.
4. Decisions to log? Add to Key Decisions.
5. "What This Is" still accurate? Update if drifted.

**After each milestone**:

1. Review all sections.
2. Re-check Core Value.
3. Audit Out of Scope reasoning.
4. Update Context with current state.

---
*Last updated: 2026-05-15 after Phase 17 verification and v1.1 ship-gate closure*
