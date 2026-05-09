# Phase 7: TES4-Family BSA Write-New Support - Specification

**Created:** 2026-05-08
**Ambiguity score:** 0.13 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can create new TES4/Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 BSA archives from disk files or memory buffers, then prove the output by reopening, extracting, and byte-comparing source payloads.

## Background

libbsa currently has a public `archive_reader` facade for opening, listing, finding, and extracting archives, plus internal binary I/O, archive path normalization, TES4-family hashing, and compression routing primitives. TES4-family BSA read/extract support already parses v103/v104/v105 metadata, preserves canonical and original paths, handles embedded-name payload prefixes, routes raw/deflate/LZ4-frame extraction by archive metadata, and validates generated fixtures. The repository also contains a test-only TES4-family fixture generator that serializes small legal BSA archives, but there is no reusable public writer API or production write-new implementation.

## Requirements

1. **Public write-new API**: The library exposes consumer-facing write-new support for TES4-family BSA archives from disk files and memory buffers.
   - Current: Public headers expose `archive_reader` and `payload_sink`; no public archive writer type, write options, source-entry model, or write-new result exists.
   - Target: Consumers can describe archive entries from host files or caller-owned byte buffers, select a TES4-family target profile, and create a new BSA archive without using test fixture generators or private parser APIs.
   - Acceptance: A public-boundary test builds against installed libbsa headers and creates at least one archive from host files and one archive from memory buffers using only public libbsa APIs.

2. **Target profiles**: The writer supports explicit TES4/Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 target profiles.
   - Current: The reader detects and reports these versions, but no writer can emit profile-specific headers, folder records, or default compression metadata.
   - Target: The write API requires an explicit target profile and serializes version-specific BSA header/table layout for v103, v104, and v105.
   - Acceptance: Generated writer-output tests create one v103, one v104, and one v105 archive; reopening each reports `archive_type::bsa`, `archive_variant::tes4`, the expected version, expected file count, and expected default compression.

3. **Format-compatible indexes**: The writer generates folder and file indexes using TES4-family hash and sorting behavior compatible with existing reader expectations.
   - Current: `detail::hash_tes4` and reader-side deterministic metadata listing exist, but production writer-side folder/file ordering does not.
   - Target: Folder records, file records, folder names, and file names are serialized so the existing reader can reopen the archive, validate table offsets, and list every entry under its normalized canonical path.
   - Acceptance: A multi-folder writer-output archive reopens successfully; `entries()` returns all input paths exactly once by canonical path, preserves original archive spelling with `/` separators, and exposes nonzero hash metadata matching `detail::hash_tes4` expectations in fixture-backed tests.

4. **Archive and file flags**: The writer derives archive flags and file flags from target profile, content, compression choices, and embedded-name choices.
   - Current: Reader tests assert known flags from generated fixtures, but no production writer derives or serializes flags.
   - Target: Writer output sets include-directory-name and include-file-name archive flags, sets compression and embedded-name flags only when selected by the archive/write policy, and derives file category flags from entry content according to the supported TES4-family profiles.
   - Acceptance: Reopened writer-output metadata reports expected archive flags and file flags for archives containing mesh, texture, and script-style paths, including cases with and without compression and embedded-name options.

5. **Compression policy and overrides**: The writer supports archive defaults and per-file compression overrides for TES4-family payloads.
   - Current: Internal compression adapters can compress deflate and LZ4-frame payloads, and readers can extract raw, deflate, and LZ4-frame entries, but callers cannot select write-time compression behavior.
   - Target: Consumers can use the target profile default compression policy or override individual entries as raw or compressed; v103/v104 compressed entries use deflate and v105 compressed entries use LZ4-frame.
   - Acceptance: Round-trip tests create v103/v104/v105 archives containing both raw and compressed entries; reopening reports the expected `entry_compression` for each entry and extraction byte-compares equal to the original source bytes.

6. **Embedded-name opt-in**: The writer can emit embedded file names only when explicitly requested and target-compatible, while leaving them absent by default.
   - Current: Reader extraction strips embedded-name prefixes from consumer-visible payload bytes, and generated fixtures prove embedded-name extraction, but no writer controls embedded-name emission.
   - Target: The write API provides an explicit embedded-name option; default writer output has no embedded-name payload prefixes, and requested embedded names are serialized without changing extracted file bytes.
   - Acceptance: Tests create one archive with embedded names disabled and one with embedded names enabled; reopened metadata reports `has_embedded_name == false` for the first and `true` with the expected prefix size for the second, and both archives extract bytes identical to source payloads.

7. **Opt-in payload deduplication**: The writer optionally deduplicates identical stored payloads by content hash when requested.
   - Current: No writer exists, and existing reader metadata can expose payload offsets and stored sizes but does not create shared payload spans.
   - Target: Deduplication is disabled unless the caller enables it; when enabled, identical source payloads with compatible stored encoding may share one stored payload region while preserving distinct archive entries.
   - Acceptance: Tests create duplicate-content entries with deduplication disabled and enabled; disabled output has distinct payload offsets, enabled output has shared payload offsets for eligible identical entries, and both modes reopen and extract all entries with byte-identical content.

8. **Round-trip validation**: Writer output is validated through the existing reader and extraction APIs, not by trusting writer internals.
   - Current: Reader fixture tests prove parse/extract behavior for generated archives, but no production writer-output round-trip tests exist.
   - Target: Every supported target profile has pack, reopen, list/find/contains, extract, and byte-compare coverage for disk and memory sources, including zero-byte payloads and at least one multi-folder archive.
   - Acceptance: CTest labels for TES4-family writer round trips pass for v103, v104, and v105; each test compares extracted bytes against source bytes and verifies `find()`/`contains()` for canonical and mixed-case lookup variants.

## Boundaries

**In scope:**
- Public TES4-family write-new API for creating new BSA archives from disk-file entries and memory-buffer entries.
- Explicit v103, v104, and v105 target profiles.
- TES4-family folder/file hash computation, index sorting, table serialization, and payload offset serialization.
- Archive flag and file flag derivation for supported TES4-family content categories.
- Raw, deflate, and LZ4-frame payload writing through explicit target-profile compression routing.
- Per-file compression overrides relative to the selected archive default.
- Explicit opt-in embedded-name emission with round-trip extraction proof.
- Explicit opt-in identical-payload deduplication with round-trip extraction proof.
- Generated legal writer-output tests that reopen archives with `archive_reader`, extract entries, and byte-compare source payloads.

**Out of scope:**
- TES3/Morrowind BSA writing - Phase 10 owns TES3 write support and data-section-relative offset semantics.
- BA2 GNRL or BA2 DX10 writing - Phases 8 and 9 own BA2 writer behavior.
- In-place mutation of existing archives - v1 explicitly prefers write-new flows until full offset and validation behavior is proven.
- Parallel packing, parallel compression, and performance benchmarking - Phase 12 owns performance and concurrency after single-threaded correctness is established.
- Broad BSArchPro UI/CLI option parity - libbsa is a reusable library and this phase locks only the WBSA requirements listed for Phase 7.
- Compatibility warning API or validator framework - Phase 11 owns structured compatibility warnings and hardening APIs.
- Copyrighted game archives or mutable `TES5Edit/` fixtures - writer validation must use repository-owned generated fixtures or caller-provided local-only data.

## Constraints

- Public APIs must remain C++20-compatible and must not expose `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, or `TES5Edit/` types.
- Writer behavior must use explicit target profiles and metadata-driven compression routing; it must not infer compression solely from file extension.
- `TES5Edit/` remains read-only: no editing, formatting, compiling, staging, or fixture workspace use.
- Single-threaded correctness is the phase boundary; bounded-memory/performance guarantees beyond existing project conventions are deferred unless needed for correctness tests.
- Write-new output must be validated by reopening with existing reader code and extracting through public extraction APIs.

## Acceptance Criteria

- [ ] Public installed-header tests create TES4-family BSA archives from both disk files and memory buffers without private headers.
- [ ] Writer-output archives for v103, v104, and v105 reopen successfully and report the expected version, variant, file count, flags, and default compression.
- [ ] Multi-folder writer-output archives list all input entries exactly once under normalized canonical paths and preserve original path spelling with `/` separators.
- [ ] Raw and compressed entries round-trip for v103/v104 deflate and v105 LZ4-frame profiles, including per-file compression overrides.
- [ ] Embedded-name disabled output reports no embedded names; embedded-name enabled output reports embedded names and extracts source bytes without prefix leakage.
- [ ] Deduplication disabled output stores duplicate-content entries at distinct offsets; deduplication enabled output shares eligible identical payload offsets while preserving distinct entries.
- [ ] Disk-source, memory-source, zero-byte, mixed-case lookup, and multi-folder cases pass pack/reopen/find/contains/extract/byte-compare tests.
- [ ] TES3 writing, BA2 writing, in-place mutation, parallel packing, and compatibility warning APIs are not introduced by this phase.
- [ ] `TES5Edit/` git status remains empty after phase work.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.95  | 0.75  | met    | Public writer API plus v103/v104/v105 round-trip target is locked. |
| Boundary Clarity    | 0.88  | 0.70  | met    | TES3, BA2, in-place mutation, broad tool parity, and performance work are explicit exclusions. |
| Constraint Clarity  | 0.82  | 0.65  | met    | Single-threaded write-new, dependency-light public API, read-only TES5Edit, and metadata-driven compression are locked. |
| Acceptance Criteria | 0.84  | 0.70  | met    | Pass/fail round-trip, metadata, compression, embedded-name, and dedupe checks are specified. |
| **Ambiguity**       | 0.13  | <=0.20| met    | Gate passed after round 2. |

Status: met = meets minimum, below = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What user-visible creation capability should be locked? | Public writer API for write-new archives from disk files and memory buffers. |
| 1 | Researcher | Which TES4-family targets are first-class pass/fail targets? | v103, v104, and v105 all required. |
| 2 | Researcher + Simplifier | What should deduplication require? | Deduplication is explicit opt-in with pass/fail proof. |
| 2 | Researcher + Simplifier | What should embedded-name support require? | Embedded names are explicit opt-in only, absent by default. |
| 2 | Simplifier | What is the irreducible core scope? | Single-threaded write-new correctness with round-trip validation; no performance/concurrency expansion. |

---

*Phase: 07-tes4-family-bsa-write-new-support*
*Spec created: 2026-05-08*
*Next step: /gsd-discuss-phase 7 - implementation decisions only*
