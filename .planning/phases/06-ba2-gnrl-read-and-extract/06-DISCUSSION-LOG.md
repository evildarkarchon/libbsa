# Phase 06: ba2-gnrl-read-and-extract - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-05
**Phase:** 06-ba2-gnrl-read-and-extract
**Areas discussed:** BA2 API shape, Metadata exposure, Compression semantics, Fixture proof shape, Verification gap closure

---

## BA2 API Shape

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Top-level public API | Mirror BSA | Add `include/libbsa/ba2.hpp`, `ba2_archive`, `open_ba2`, and `extract_ba2_entry`, matching the established BSA surface without unifying families yet. | Yes |
| Top-level public API | Extend BSA header | Put BA2 APIs next to `bsa_archive` in `bsa.hpp`; fewer files, but muddier archive-family separation. | |
| Top-level public API | Generic archive API | Introduce a family-neutral `archive` API now; more elegant long-term, but SPEC explicitly defers replacing existing BSA entry points. | |
| BA2 archive methods | Core plus summary | Keep methods parallel to BSA (`summary`, `paths`, `contains`, `entry`) and rely on `archive_summary`/`entry_metadata` for BA2-specific fields. | Yes |
| BA2 archive methods | Add BA2 accessors | Add methods like `compression_method()` or `file_table_offset()` on `ba2_archive`; convenient, but duplicates fields already in `archive_summary`. | |
| BA2 archive methods | Minimal class only | Expose only construction/open/extract and use `archive_view` externally; smaller, but weaker parity with existing `bsa_archive`. | |
| Public header smoke | Open and extract | Compile consumer-style code that includes `ba2.hpp`, names `ba2_archive`, takes `&open_ba2`, and takes `&extract_ba2_entry`. | Yes |
| Public header smoke | Header include only | Just include `ba2.hpp`; fastest but does not prove the public function/type signatures are usable. | |
| Public header smoke | Full sample flow | Add a richer compile/run smoke path with a tiny BA2 fixture; useful but duplicates fixture tests. | |
| Source lifetime | Metadata only | Like `bsa_archive`, it owns copied metadata only; callers pass the same `byte_source` again to extraction. | Yes |
| Source lifetime | Retain source ref | Store a source reference inside `ba2_archive`; extraction is simpler but creates lifetime coupling. | |
| Source lifetime | Own source wrapper | Have the archive own a source object; convenient, but changes ownership policy and likely expands API scope. | |

**User's choice:** Mirror BSA API, core summary-backed methods, open/extract smoke coverage, metadata-only source lifetime.
**Notes:** This preserves existing BSA API patterns without starting the deferred family-neutral API redesign.

---

## Metadata Exposure

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Archive fields | Detected fields only | Expose format, version, subtype, file_count, file_table_offset, and Starfield v3 compression_method; do not invent BA2-only public fields until needed. | Yes |
| Archive fields | Every header word | Preserve all version-specific header/reserved fields publicly; maximal fidelity, but may freeze uncertain semantics into API. | |
| Archive fields | Minimal identity | Expose only format/version/count; simpler, but Starfield v2/v3 acceptance asks for relevant header metadata. | |
| Hash metadata | Hash fields mapped | Populate `name_hash`/`directory_hash` with BA2 hash components where representable, and document exact mapping in tests/comments. | Yes |
| Hash metadata | Raw BA2 record only | Add BA2-specific raw hash fields publicly; higher fidelity, but expands API for one family. | |
| Hash metadata | No hash exposure | Use hashes internally for parsing only; simpler, but conflicts with SPEC metadata fidelity. | |
| Size semantics | All three sizes | Set `size` = unpacked bytes, `packed_size` = compressed payload bytes or raw size, and `stored_size` = on-disk bytes read for extraction/range validation. | Yes |
| Size semantics | Packed plus unpacked | Use `size` and `packed_size` only; simpler, but loses the range-validation distinction established for BSA. | |
| Size semantics | Raw record semantics | Expose BA2 record values directly even if names differ from libbsa conventions; more exact, but less consistent for consumers. | |
| Offset semantics | Absolute validated offsets | Continue Phase 5's pattern: convert format-native offsets to absolute payload offsets after validation. | Yes |
| Offset semantics | Native record offsets | Expose the BA2 record field directly; preserves raw format shape, but forces consumers to know table/layout semantics. | |
| Offset semantics | Hide offsets | Avoid exposing offsets publicly; simpler API, but violates current metadata inspection expectations. | |

**User's choice:** Detected archive fields only, hash components mapped into existing fields, all three size semantics, absolute validated offsets.
**Notes:** Metadata should stay consistent with existing libbsa models rather than freezing raw BA2 record structures into public API.

---

## Compression Semantics

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Entry compression state | Resolved state | Populate `raw`, `deflate`, or `lz4_block` per entry so metadata tells consumers what extraction will do. | Yes |
| Entry compression state | Archive default | Use `archive_default` when BA2 record says compressed and let extraction resolve later; closer to routing API but less inspectable. | |
| Entry compression state | Boolean only | Raw vs compressed only; insufficient for Starfield v3 LZ4-block vs deflate distinction. | |
| Starfield v3 LZ4 route | Compressed + method 3 | Only compressed entries route to LZ4 block when archive `compression_method == 3`; raw entries stay raw regardless of method. | Yes |
| Starfield v3 LZ4 route | All method 3 entries | Every entry in method-3 archives routes through LZ4 block; simpler, but corrupts raw entries. | |
| Starfield v3 LZ4 route | Entry flag only | Ignore archive `compression_method`; conflicts with existing explicit routing and SPEC constraints. | |
| BA2 compressed framing | Record sizes drive it | Use BA2 record packed/unpacked sizes directly; do not assume BSA-style embedded uncompressed-size prefixes. | Yes |
| BA2 compressed framing | BSA-style prefix | Expect a 4-byte unpacked-size prefix before compressed payloads; matches current BSA path but may be wrong for BA2. | |
| BA2 compressed framing | Auto-detect prefix | Try both prefix and no-prefix; more tolerant, but risks masking corrupt fixtures and codec confusion. | |
| Route errors | Fail structured | Return `unsupported_format` or `malformed_archive`; never fall back to another codec or write partial bytes. | Yes |
| Route errors | Try fallback codec | Attempt deflate/LZ4 alternatives; may extract more damaged inputs, but weakens route-confusion guarantees. | |
| Route errors | Expose warning only | Write bytes and warn; project error model does not currently have warning plumbing. | |

**User's choice:** Resolved per-entry compression state, Starfield v3 LZ4 only for compressed method-3 entries, BA2 record-size framing, structured failures for route conflicts.
**Notes:** This keeps metadata inspection aligned with extraction and preserves Phase 3 codec-confusion protections.

---

## Fixture Proof Shape

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Primary corpus | Generated builders | Small deterministic BA2 fixtures generated in test code; no external corpus and no committed binary blobs unless necessary. | Yes |
| Primary corpus | Committed binaries | Check in tiny `.ba2` binaries; close to real files, but harder to review and update. | |
| Primary corpus | External corpus | Use real game/BSArchPro archives; stronger compatibility, but SPEC defers corpus comparison to Phase 11. | |
| Version/compression matrix | Spec matrix | Cover FO4 v1/v7/v8, Starfield v2, Starfield v3 raw/deflate/LZ4-block, plus `.dds`-named GNRL payload. | Yes |
| Version/compression matrix | Minimal smoke | One FO4 and one Starfield fixture only; faster, but misses locked version requirements. | |
| Version/compression matrix | Broad variants | Add extra edge permutations beyond SPEC; stronger, but risks expanding Phase 6 beyond planning needs. | |
| Builder location | Test-local first | Keep builders in `tests/ba2_reader_tests.cpp` or a small test helper only; promote later only if writer phases reuse them. | Yes |
| Builder location | Reusable test helper | Create shared fixture-builder files now; more reusable, but extra structure before BA2 writers exist. | |
| Builder location | Library internals | Place builders under `src/`; not appropriate because fixture generation is test-only behavior. | |
| Malformed cases | Core parser failures | Test truncated headers/records, impossible payload offsets, truncated name table, mismatched file count/name count, and codec route confusion. | Yes |
| Malformed cases | Name table only | Only test malformed name tables now because SPEC highlights FileTableOffset parsing. | |
| Malformed cases | Full fuzz-style set | Add many malformed permutations now; useful, but Phase 11 owns comprehensive hardening. | |

**User's choice:** Generated test-local fixture builders, SPEC matrix coverage, core parser failure coverage.
**Notes:** Real-corpus and BSArchPro comparison stay deferred to Phase 11 validation.

---

## the agent's Discretion

No areas were left to the agent's discretion.

---

## Verification Gap Closure

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Gap areas to lock | Both gaps | Capture decisions for duplicate normalized BA2 names and zero-entry `FileTableOffset` validation so the verifier gaps can be closed together. | Yes |
| Gap areas to lock | Duplicate names only | Focus on rejecting normalized name collisions before `archive_view` can collapse entries. | |
| Gap areas to lock | Zero-entry offsets only | Focus on validating `FileTableOffset` even when `file_count` is zero. | |
| Duplicate BA2 names | Reject archive | Fail `open_ba2` with `malformed_archive` before constructing `ba2_archive`; preserves file-count/name association and avoids silent overwrite. | Yes |
| Duplicate BA2 names | Keep first entry | Open archive but ignore later duplicates; deterministic, but silently drops record data. | |
| Duplicate BA2 names | Keep last entry | Open archive and preserve current `archive_view` overwrite behavior; leaves verifier gap unresolved. | |
| Empty BA2 `FileTableOffset` | Must be in-bounds | Allow empty BA2 only if `FileTableOffset <= source.size()` and record table invariants pass; impossible offsets fail as malformed. | Yes |
| Empty BA2 `FileTableOffset` | Must equal table end | Stricter: empty BA2 must use exactly the header/record-table end as `FileTableOffset`. | |
| Empty BA2 `FileTableOffset` | Ignore offset | Keep current permissive behavior; leaves verifier gap unresolved. | |

**User's choice:** Close both gaps by rejecting duplicate normalized names and requiring empty archives to keep `FileTableOffset` in-bounds.
**Notes:** These are targeted D-16/BA2-04 closure decisions, not a broader Phase 11 malformed-input expansion.

## Deferred Ideas

None - discussion stayed within phase scope.
