# Phase 3: Format Detection and TES4-Family BSA Read/Extract - Specification

**Created:** 2026-05-07
**Ambiguity score:** 0.18 (gate: <= 0.20)
**Requirements:** 10 locked

## Goal

`archive_reader::open` changes from a non-empty-path `unsupported` stub into a public TES4-family BSA reader slice that detects, inspects, lists, queries, and extracts TES4/Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 archives from archive bytes rather than file extension.

## Background

Phase 1 established the public C++20 package boundary, `libbsa::result`, `libbsa::error`, and a minimal `archive_reader::open` facade that currently returns `error_code::unsupported` for every non-empty path. Phase 2 added internal checked little-endian binary I/O, archive virtual path normalization, bounded payload streaming, exact-size deflate and LZ4 frame/block codecs, explicit compression routing, and TES3/TES4/BA2 hash helpers under `libbsa::detail`. No archive-level parser, detector, public archive metadata, entry listing, path lookup, or extraction behavior exists yet. Phase 3 is triggered by that gap: the next public milestone is a usable TES4-family BSA read/extract slice built on the proven Phase 2 primitives.

## Requirements

1. **Byte-driven format detection**: Opening a TES4-family BSA detects the archive family and variant from archive bytes, including magic, version, and archive type fields, not from the host filename or extension.
   - Current: `archive_reader::open("example.bsa")` returns `error_code::unsupported`, and no archive bytes are parsed.
   - Target: Public open behavior recognizes TES4/Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 BSA archives by header content and rejects unsupported or malformed inputs with structured errors.
   - Acceptance: A test opens valid v103, v104, and v105 fixtures whose filenames do not encode the variant and observes the correct detected variant; a non-BSA byte stream with a `.bsa` filename fails with `error_code::format_error` or `error_code::unsupported`.

2. **Archive-level metadata**: Consumers can inspect archive type, detected variant, version, archive flags, file count, and supported compression behavior after opening a TES4-family BSA.
   - Current: No public archive metadata type or access behavior exists.
   - Target: Opened TES4-family readers expose library-owned metadata for the archive-level fields required by FMT-02 without exposing libdeflate, lz4, TES5Edit, Windows, or implementation-detail types in public headers.
   - Acceptance: Public API tests open v103, v104, and v105 fixtures and verify variant, version, flags, file count, and compression capability values from fixture manifests.

3. **Stable path listing**: Consumers can list every file path in an opened TES4-family BSA through a stable library-owned path representation using archive virtual path semantics.
   - Current: `normalize_archive_path` exists only as an internal helper, and no public archive listing exists.
   - Target: Opened TES4-family readers return the archive's file paths in normalized, deterministic form without using `std::filesystem::path` for archive-internal keys.
   - Acceptance: A fixture with mixed separators and case-equivalent lookup inputs lists each expected archive path exactly once in deterministic order and does not expose host path separators as semantic differences.

4. **Path existence and lookup**: Consumers can check whether a path exists and locate entries using normalized archive virtual path semantics and TES4-family hash behavior compatible with the Phase 2 hash service.
   - Current: TES4 hash helpers and path normalization are internal, but no archive entry index consumes them.
   - Target: Opened TES4-family readers can answer existence and entry lookup requests for case/separator variants of the same archive path while preserving strict rejection of traversal or host-rooted names.
   - Acceptance: Tests verify that `meshes/foo.nif`, `Meshes\\Foo.nif`, and other valid case/separator variants locate the same fixture entry, while traversal-like or rooted lookup strings return an invalid-argument style error and do not match any entry.

5. **Per-entry metadata**: Consumers can retrieve per-entry metadata including raw size, stored size, payload offset, compression method, TES4-family hash values, and applicable BSA record flags.
   - Current: No archive entry metadata exists beyond internal hash functions.
   - Target: Opened TES4-family readers expose library-owned entry metadata sufficient to validate FMT-05 and to drive extraction without reparsing public callers' path strings.
   - Acceptance: Fixture tests compare each exposed metadata field against a manifest for raw, deflate-compressed, LZ4-frame-compressed, and embedded-name entries.

6. **v103 TES4/Oblivion extraction**: Consumers can extract files from TES4/Oblivion BSA v103 archives, including raw and deflate-compressed entries when fixture metadata marks them compressed.
   - Current: No BSA payload extraction exists.
   - Target: Opened v103 readers write extracted payload bytes to caller-provided sinks and route deflate payloads through exact-size validation when required by archive flags and entry state.
   - Acceptance: v103 fixture tests extract at least one raw entry and one deflate-compressed entry and byte-compare the sink output against source fixture bytes.

7. **v104 FO3/FNV/Skyrim LE extraction**: Consumers can extract files from FO3/FNV/Skyrim LE BSA v104 archives, including raw and deflate-compressed entries.
   - Current: No v104 parser or extractor exists.
   - Target: Opened v104 readers parse v104 records, expose v104 metadata, and extract raw or deflate-compressed payloads according to archive and entry compression state.
   - Acceptance: v104 fixture tests extract raw and deflate-compressed entries and verify exact output bytes plus expected metadata values.

8. **v105 Skyrim SE/AE extraction**: Consumers can extract files from Skyrim SE/AE BSA v105 archives, including raw entries and LZ4-frame-compressed entries.
   - Current: LZ4 frame decompression exists internally, but no v105 BSA parser routes archive entries to it.
   - Target: Opened v105 readers parse v105 records and route LZ4-frame-compressed BSA payloads through the LZ4 frame adapter with exact expected output size validation.
   - Acceptance: v105 fixture tests extract at least one raw entry and one LZ4-frame-compressed entry and byte-compare output against source fixture bytes.

9. **Embedded-name payload handling**: Consumers can extract TES4-family BSA entries with embedded file names while preserving payload bytes compatible with BSArchPro behavior.
   - Current: No embedded-name parsing or extraction policy exists.
   - Target: Extraction removes or skips the embedded filename prefix exactly as required for the consumer-visible extracted file payload while retaining metadata needed to explain the record layout.
   - Acceptance: An embedded-name fixture extracts bytes that match the expected file content, not the archive-internal embedded-name prefix, and the test documents the expected BSArchPro-compatible behavior.

10. **Extensible format-family boundary**: Maintainers can add a future archive version or a later non-TES4-family parser without rewriting the TES4-family reader slice or leaking private dependencies into public headers.
    - Current: There is only a single `archive_reader::open` stub and Phase 2 private utilities.
    - Target: Format detection, TES4-family parsing, metadata, lookup, and extraction have a clear boundary that later TES3 and BA2 phases can extend through new format handlers rather than invasive rewrites.
    - Acceptance: Public include boundary tests still reject private dependency and implementation tokens, and a focused unit test can classify unsupported future/unknown versions without changing TES4-family fixture expectations.

## Boundaries

**In scope:**
- Public TES4-family open/read behavior for `archive_reader::open` replacing the current non-empty-path `unsupported` stub.
- Byte-driven detection for TES4/Oblivion BSA v103, FO3/FNV/Skyrim LE BSA v104, and Skyrim SE/AE BSA v105.
- Public library-owned archive metadata, entry metadata, stable path listing, path existence checks, and entry lookup behavior.
- Extraction of TES4-family BSA entries stored raw, deflate-compressed, or LZ4-frame-compressed according to archive version, flags, and record metadata.
- Embedded-name entry extraction behavior with expected output bytes documented and tested.
- Committed tiny legal generated fixtures for default CI coverage of detection, metadata, lookup, extraction, compression routing, and embedded-name behavior.
- Optional local or BSArchPro-derived compatibility tests may supplement coverage but must not be required for default CI completion.

**Out of scope:**
- TES3/Morrowind BSA parsing or extraction - Phase 4 owns TES3 data-section-relative offset behavior.
- BA2 GNRL or BA2 DDS parsing/extraction - Phases 5 and 6 own BA2 records, filename tables, and DDS reconstruction.
- Any write-new BSA or BA2 behavior - writer phases start at Phase 7 after readers are proven.
- In-place archive mutation - v1 explicitly excludes mutating existing archives.
- GUI, CLI tooling, or sample app behavior - libbsa is a reusable library and examples/docs are later scope.
- Whole-archive performance optimization, parallel extraction, and benchmarks - Phase 12 owns performance and concurrency after correctness is established.
- Public exposure of libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, or `std::expected` types - the public boundary remains portable C++20.
- Required committed game-derived archives or writable TES5Edit fixtures - fixture policy forbids copyrighted archives in the repo and treats `TES5Edit/` as read-only.

## Constraints

- Detection must be archive-byte-driven; host filename extension can be used only as caller-provided path text, not as format truth.
- Public APIs must return `libbsa::result<T>` or existing structured error values for I/O and format failures; exceptions remain for programmer precondition violations only.
- Public headers must stay C++20-compatible and must not expose private dependencies, DirectXTex/DXGI, Windows, TES5Edit, or C++23-only `std::expected`.
- Archive-internal paths must use explicit normalized virtual path semantics rather than `std::filesystem::path` semantics.
- Extraction must use bounded sink-oriented output and must not require loading whole archives into memory.
- Deflate and LZ4 frame decompression must use the Phase 2 exact-size adapters and must fail closed on corrupt or size-mismatched payloads.
- Default automated acceptance must rely on committed tiny legal generated fixtures; local game-derived fixtures and BSArchPro-derived comparisons are allowed as optional supplemental tests only.
- `TES5Edit/` remains read-only: no edits, formatting, generated outputs, staging, or submodule pointer changes.

## Acceptance Criteria

- [ ] Opening valid v103, v104, and v105 TES4-family BSA fixtures succeeds and reports the expected variant, version, flags, file count, and compression capability from fixture manifests.
- [ ] Detection uses header bytes rather than filename extension, proven by fixtures whose host filenames do not encode their archive variant and by a non-BSA `.bsa` rejection test.
- [ ] Public listing returns every expected fixture path exactly once in deterministic normalized form.
- [ ] Path existence and lookup find the same entry for valid case/separator variants and reject rooted or traversal-like lookup strings.
- [ ] Per-entry metadata tests verify raw size, stored size, offset, compression method, TES4-family hash, and format-specific record data for representative entries.
- [ ] v103 extraction byte-compares at least one raw entry and one deflate-compressed entry against expected source bytes.
- [ ] v104 extraction byte-compares at least one raw entry and one deflate-compressed entry against expected source bytes.
- [ ] v105 extraction byte-compares at least one raw entry and one LZ4-frame-compressed entry against expected source bytes.
- [ ] Embedded-name extraction returns expected file payload bytes without exposing the embedded-name prefix as extracted content.
- [ ] Malformed, truncated, corrupt-compressed, size-mismatched, and unsupported-version fixtures fail with structured errors and no out-of-bounds reads or ambiguous partial-success extraction.
- [ ] Public include boundary tests continue to reject private dependency names and TES5Edit leakage from installed headers.
- [ ] `git -C TES5Edit status --short` is empty after the phase.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.92  | 0.75  | met    | Full reader slice locked: detect, inspect, list/query, and extract. |
| Boundary Clarity    | 0.80  | 0.70  | met    | TES4-family v103-v105 only; TES3, BA2, writers, CLI, and performance phases excluded. |
| Constraint Clarity  | 0.70  | 0.65  | met    | Byte-driven detection, public boundary, fixture policy, streaming, exact-size codecs, and TES5Edit boundary locked. |
| Acceptance Criteria | 0.80  | 0.70  | met    | Pass/fail criteria cover variants, metadata, lookup, extraction, malformed cases, and boundary checks. |
| **Ambiguity**       | 0.18  | <=0.20| met    | Gate passed after round 1. |

Status: met = dimension meets or exceeds its minimum; below = planner treats as assumption.

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What primary user-visible deliverable must exist after the current `archive_reader::open` stub and Phase 2 primitives? | Full reader slice: public open, detection, metadata/list/query, and extraction behavior. |
| 1 | Researcher | Which TES4-family variants must be proven in this phase? | v103, v104, and v105 are all required, not merely shaped for later. |
| 1 | Researcher | What fixture proof is mandatory for acceptance? | Default CI uses committed tiny legal generated fixtures; optional local/BSArchPro-derived compatibility tests may supplement but cannot block default CI. |
| 1 | Gate | Ambiguity scored 0.18 after round 1; proceed? | User selected `Yes - write SPEC.md`. |

---

*Phase: 03-format-detection-and-tes4-family-bsa-read-extract*
*Spec created: 2026-05-07*
*Next step: /gsd-discuss-phase 3 - implementation decisions (API shape, parser boundaries, fixture generation, and extraction mechanics)*
