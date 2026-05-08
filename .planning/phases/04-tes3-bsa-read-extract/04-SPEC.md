# Phase 4: TES3 BSA Read/Extract - Specification

**Created:** 2026-05-08
**Ambiguity score:** 0.17 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can open, list, query, and extract TES3/Morrowind BSA archives through the existing `archive_reader` API while preserving known TES3 read/extract quirks, especially data-section-relative entry offsets.

## Background

Phase 3 completed a bounded TES4-family BSA read/extract slice through `archive_reader`: byte-driven detection, archive metadata, deterministic entries, normalized lookup, path-first extraction, `extract_bytes`, generated fixtures, malformed fixture checks, and no whole-archive resident buffer in reader state. The current detector only recognizes TES4-family `BSA\0` headers with versions 103, 104, and 105, and `archive_reader::open` always dispatches to the TES4-family parser after detection. Public metadata already includes `archive_variant::tes3`, `entry_compression::none`, and reusable entry fields, and Phase 2 already provides TES3 hash helpers. No TES3/Morrowind BSA detector, parser, generated fixture, malformed fixture, listing, lookup, or extraction behavior exists yet.

## Requirements

1. **TES3 detection and metadata**: `archive_reader::open` must recognize structurally valid TES3/Morrowind BSA archives from archive bytes and expose archive metadata for the TES3 variant.

Current: `detect_bsa_format` recognizes only TES4-family `BSA\0` archives with versions 103, 104, and 105, so TES3 archives are rejected as unsupported.

Target: A valid generated TES3 BSA opens successfully and reports `archive_type::bsa`, `archive_variant::tes3`, the parsed TES3 version/header identity, file count, no default compression, and stable archive-level fields that fit the existing public metadata shape.

Acceptance: A fixture-backed unit test opens a generated TES3 archive and asserts type, variant, version/header value, file count, and `entry_compression::none`; a non-TES3/non-TES4 byte stream still returns `error_code::unsupported`.

2. **TES3 entry listing**: TES3 file records and name data must be exposed as deterministic `entry_metadata` values through `archive_reader::entries()`.

Current: `entries()` returns metadata parsed by the TES4-family parser only; there is no TES3 table or name parser.

Target: A generated TES3 archive with multiple files lists canonical normalized paths, original archive spelling with `/` separators, raw size, stored size, payload offset, TES3 hash metadata, `entry_compression::none`, no embedded-name prefix, and deterministic sorting by canonical path.

Acceptance: A fixture-backed unit test compares every listed TES3 entry against a generated manifest for canonical path, original path, size fields, payload offset, hash value, compression, embedded-name flags, and sorted order.

3. **TES3 lookup semantics**: TES3 archives must support `find()` and `contains()` using the same library-owned archive path normalization semantics as the existing reader API.

Current: Lookup delegates to TES4-family entry helpers and only searches TES4-family parsed entries.

Target: TES3 lookup accepts valid case and separator variants for existing paths, returns an empty optional for valid missing paths, and returns `error_code::invalid_argument` for malformed archive paths.

Acceptance: Unit tests verify `find()` and `contains()` for at least one TES3 entry using original spelling, case variation, slash/backslash variation, a valid missing path, and an invalid path containing rejected syntax.

4. **TES3 data-section-relative extraction**: TES3 extraction must interpret entry offsets relative to the TES3 data section rather than as archive-absolute TES4-family offsets.

Current: Extraction seeks to `entry.payload_offset` as an archive-absolute position, matching TES4-family behavior but not TES3 data-section-relative semantics.

Target: TES3 `extract(path, sink)` and `extract_bytes(path)` return the exact source payload bytes for every generated fixture entry by applying TES3 data-section-relative offset semantics.

Acceptance: A generated TES3 fixture includes at least two files whose data-section-relative offsets would be wrong if treated as archive-absolute offsets, and extraction tests byte-compare both sink extraction and `extract_bytes()` output against manifest payload bytes.

5. **Known TES3 read quirks**: Phase 4 must cover all TES3 read/extract quirks identified during reference tracing for this phase, limited to opening, listing, querying, and extracting existing archives.

Current: TES3 hash helpers exist, but there is no TES3 archive parser to validate table layout, file name offsets, hash records, file sizes, or payload spans.

Target: The TES3 reader validates and exposes the known TES3 read structures required for fixture-backed open/list/query/extract behavior: header fields, file size records, data-section-relative file offsets, name offsets/name table, TES3 hashes, and raw uncompressed payload spans.

Acceptance: Generated success and malformed tests fail if TES3 table counts, name offsets, hash records, file sizes, or payload spans are internally inconsistent; each non-obvious TES3 compatibility rule used by the implementation is covered by a focused fixture, reference-trace note, or code comment.

6. **Generated success fixtures**: Phase 4 must add legal generated TES3 fixture data that proves success behavior without relying on copyrighted game archives or mutating `TES5Edit/`.

Current: `tests/fixtures/generated/archives` contains generated TES4-family success and malformed archives only.

Target: The fixture generator or equivalent generated-fixture path produces at least one committed TES3 success archive and manifest with multiple files, path/hash metadata, expected offsets, and expected payload bytes.

Acceptance: The test suite can regenerate or validate the TES3 success fixture from repository-owned code/data, and no test requires local Morrowind archives or writes anywhere under `TES5Edit/`.

7. **Malformed TES3 rejection**: Malformed TES3 archives must fail closed with structured errors rather than out-of-bounds reads, undefined behavior, or partial success.

Current: Malformed fixture coverage exists for TES4-family archives only.

Target: Focused TES3 malformed fixtures cover truncated headers/tables, invalid name spans, invalid data-section-relative payload spans, duplicate canonical paths, and internally inconsistent counts or offsets.

Acceptance: Unit tests open each malformed TES3 fixture and assert a stable `error_code::format_error` or `error_code::unsupported` as appropriate; extraction of a corrupted TES3 payload span fails without writing partial success to the caller sink.

8. **Reader API continuity**: TES3 support must extend the existing public reader surface without adding TES3-specific public classes or leaking private dependencies.

Current: `archive_reader` already exposes the intended public read/extract shape, and public headers remain free of TES5Edit, compression library, DirectXTex, and private implementation types.

Target: TES3 read/extract behavior is available through `archive_reader::open`, `metadata`, `entries`, `find`, `contains`, `extract`, and `extract_bytes`; public headers do not gain TES3-only parser objects, `std::filesystem::path` for archive-internal paths, TES5Edit symbols, or private dependency types.

Acceptance: Existing public include boundary tests still pass, TES3 tests use the same public `archive_reader` API as TES4-family tests, and `git -C TES5Edit status --short` produces no output after the phase.

## Boundaries

**In scope:**
- TES3/Morrowind BSA byte detection for read/open behavior.
- TES3 archive metadata exposed through the existing public metadata shape.
- TES3 entry listing, normalized lookup, and contains behavior through `archive_reader`.
- TES3 raw extraction through `extract(path, sink)` and `extract_bytes(path)`.
- TES3 data-section-relative payload offset semantics.
- Known TES3 read/extract quirks identified during this phase's reference tracing and fixture design.
- Generated legal TES3 success fixture(s), manifests, and focused malformed fixture coverage.
- Tests proving TES3 behavior without local game archives and without mutating `TES5Edit/`.

**Out of scope:**
- TES3/Morrowind BSA writing - Phase 10 owns TES3 write-new support.
- TES4-family behavior changes except integration points required to dispatch TES3 safely - Phase 3 behavior is already verified and should not be reworked for scope expansion.
- BA2 GNRL or DDS parsing/extraction - Phases 5 and 6 own BA2 read support.
- Compression handling for TES3 entries - TES3 BSA read/extract scope is raw/uncompressed payloads.
- Filesystem-directory extraction convenience APIs - Phase 4 uses the existing sink and byte-vector extraction APIs only.
- CLI tooling, GUI tooling, progress callbacks, cancellation, parallel extraction, and performance benchmarking - later phases or v2 tooling own those concerns.
- Required real-game archive validation or committed copyrighted data - generated legal fixtures are the required acceptance evidence; local archives may be supplemental only.
- Required BSArchPro-generated golden output files - reference tracing informs compatibility, but generated fixture bytes are sufficient for this phase gate.

## Constraints

- The public API remains C++20-compatible and must not expose `std::expected`, TES5Edit, libdeflate, lz4, DirectXTex, Windows SDK, or private parser types.
- `TES5Edit/` is read-only and must not be edited, formatted, compiled into libbsa, used as a fixture workspace, staged, or committed.
- TES3 archive-internal paths remain normalized archive virtual paths, not host `std::filesystem::path` values.
- TES3 extraction must use bounded host-file reads for the selected entry and must not require retaining whole archive bytes in `archive_reader` state.
- TES3 entries are uncompressed for this phase; compression codecs must not be invoked for TES3 payload extraction.
- Generated fixtures must be legal repository-owned test data and must not depend on local Morrowind installations for normal CI/test success.

## Acceptance Criteria

- [ ] A generated TES3 success archive opens through `archive_reader::open` and reports `archive_variant::tes3`, `archive_type::bsa`, file count, parsed version/header identity, and `entry_compression::none`.
- [ ] `entries()` for the generated TES3 archive returns deterministic metadata matching its manifest for canonical path, original path, sizes, payload offsets, TES3 hash values, compression, and embedded-name fields.
- [ ] `find()` and `contains()` work for TES3 paths with case and separator variants, valid missing paths, and invalid path syntax using the existing public API semantics.
- [ ] `extract(path, sink)` and `extract_bytes(path)` byte-compare against expected payload bytes for multiple TES3 entries whose offsets prove data-section-relative interpretation.
- [ ] Malformed TES3 fixtures for truncated structures, invalid name spans, invalid payload spans, duplicate canonical paths, and inconsistent counts/offsets fail with stable structured errors.
- [ ] TES3 tests and fixtures are generated or validated from legal repository-owned data and do not require local game archives.
- [ ] Public include boundary tests still pass and no TES3-specific public parser/extractor classes or dependency types leak into installed headers.
- [ ] `git -C TES5Edit status --short` produces no output after the phase.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.90  | 0.75  | met    | Consumer-visible open/list/query/extract target is locked. |
| Boundary Clarity    | 0.75  | 0.70  | met    | Known TES3 read/extract quirks are in scope; writers, BA2, CLI, performance, real-game fixtures, and golden BSArchPro outputs are out. |
| Constraint Clarity  | 0.82  | 0.65  | met    | TES3 data-section-relative offsets, raw payloads, bounded selected-entry reads, public API boundary, and generated legal fixtures are explicit. |
| Acceptance Criteria | 0.84  | 0.70  | met    | Pass/fail fixture, metadata, lookup, extraction, malformed, boundary, and TES5Edit checks are defined. |
| **Ambiguity**       | 0.17  | <=0.20| met    | Gate passed after round 2. |

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What is the required Phase 4 deliverable from the consumer's point of view? | TES3 support must include open, list, query, and extract through the existing reader API. |
| 1 | Researcher | What fixture evidence must exist for Phase 4 to be accepted? | Generated success and malformed TES3 fixtures are required. |
| 1 | Researcher | What compatibility rule triggered the phase? | All known TES3 read/extract quirks matter, with data-section-relative offsets as the key protected rule. |
| 2 | Researcher + Simplifier | What does “all TES3 quirks” mean for this phase? | Scope is all known TES3 read/extract quirks identified during current reference tracing and fixtures, not every unknown future edge. |
| 2 | Simplifier | What is the irreducible minimum success? | Detect, list/query, and extract a generated TES3 fixture correctly. |
| 2 | Simplifier | Which validation is required rather than nice-to-have? | Legal generated fixtures are required; local game archives and BSArchPro golden files are optional supplemental evidence only. |

---

*Phase: 04-tes3-bsa-read-extract*
*Spec created: 2026-05-08*
*Next step: /gsd-discuss-phase 4 - implementation decisions only*
