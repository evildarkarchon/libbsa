# Phase 04: tes3-bsa-read-extract - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-08
**Phase:** 04-tes3-bsa-read-extract
**Areas discussed:** Offset Metadata, Hash Field, Strict Hash Checks, Fixture Shape

---

## Offset Metadata

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| What should public `entry_metadata::payload_offset` mean for TES3 entries? | Absolute offset | Absolute offset; Raw TES3 offset; Both offsets; You decide |
| Should generated manifests include the raw data-section-relative offset in addition to public absolute `payload_offset`? | Include raw proof | Include raw proof; Public only; You decide |
| When a TES3 entry's raw offset plus data-section base points outside the archive, when should libbsa reject it? | Fail during open | Fail during open; Fail on extract; You decide |
| What evidence should downstream agents use to lock the TES3 offset conversion rule? | Trace reference too | Trace reference too; Public docs enough; You decide |
| Should CONTEXT explicitly reinterpret the SPEC's `payload_offset` acceptance checks as public archive-absolute offsets, with separate raw-offset fixture assertions? | Yes, explicit | Yes, explicit; No, leave flexible; You decide |
| After TES3 parse validation converts raw offsets to absolute public offsets, should the reader keep the raw TES3 offset in runtime state? | No runtime raw | No runtime raw; Keep internally; You decide |
| If a TES3 fixture entry has raw offset `0`, what should public `payload_offset` assert? | Data section start | Data section start; Zero; You decide |
| Should malformed TES3 offset tests include a case where a raw offset is valid as a number but wrong if treated as archive-absolute? | Yes, prove quirk | Yes, prove quirk; Extraction enough; You decide |
| Should `entry_metadata::payload_offset` be documented as archive-absolute for all variants? | Yes, document | Yes, document; No change; You decide |
| Should overlapping metadata/name/hash/data regions fail during open? | Fail overlap | Fail overlap; Allow if usable; You decide |
| How should tests prove absolute `payload_offset` points at the right bytes? | Read archive bytes | Read archive bytes; Manifest only; Extraction only; You decide |
| Should the code comment for TES3 offset conversion cite both the format docs and the TES5Edit trace location? | Yes, both | Yes, both; TES5Edit only; No comment |
| Should TES3 zero-byte entries be considered valid if their offset is within the data section and extraction returns an empty payload? | Allow empty | Allow empty; Reject empty; You decide |
| Should TES3 entries with overlapping payload byte ranges be rejected during open? | Reject overlap | Reject overlap; Allow if bounded; You decide |
| Should libbsa require TES3 raw file data to be alphabetically ordered by filename? | Do not require | Do not require; Require order; You decide |
| If two TES3 entries have valid non-overlapping spans but payload offsets are not sorted by entry/hash order, should opening still succeed? | Allow any order | Allow any order; Require sorted; You decide |

**Notes:** Public metadata uses absolute offsets; raw TES3 offsets are fixture/test evidence only. Opened readers should expose trustworthy validated metadata.

---

## Hash Field

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| What should Phase 4 do with the public hash field? | Rename neutral | Rename neutral; Add neutral field; Reuse tes4_hash; You decide |
| What neutral field name should downstream agents prefer? | archive_hash | archive_hash; path_hash; format_hash; You decide |
| For TES3 entries, should `archive_hash` expose the stored hash table value or the hash recomputed from parsed names? | Stored value | Stored value; Recomputed value; Both values; You decide |
| Should TES3 fixture manifests represent hashes as a single 64-bit hex string, two 32-bit halves, or both? | Both forms | Both forms; 64-bit only; Halves only; You decide |
| When validating/recomputing TES3 hashes, which path spelling should be the source of truth? | Archive name bytes | Archive name bytes; Canonical path; You decide |

**Notes:** Phase 4 should clean up the public pre-v1 API by replacing `tes4_hash` with `archive_hash` rather than carrying a misleading TES4-specific field into TES3 and BA2 phases.

---

## Strict Hash Checks

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| Should TES3 open validate each stored hash against the parsed archive name? | Fail mismatch | Fail mismatch; Warn only; Expose stored; You decide |
| If two TES3 entries have different names but the same TES3 archive hash, what should happen? | Fail collision | Fail collision; Allow both; You decide |
| Should TES3 open require stored hash table/order-dependent records to be sorted by TES3 hash order? | Require sorted | Require sorted; Do not require; You decide |
| Which error category should strict TES3 hash failures use? | format_error | format_error; unsupported; invalid_argument; You decide |
| Should libbsa reject stored TES3 names that contain uppercase ASCII if their hash still matches under TES3 lower-byte rules? | Allow uppercase | Allow uppercase; Reject uppercase; You decide |
| Should TES3 stored names using `/` instead of `\` be accepted if they pass archive path normalization and hash validation? | Accept separators | Accept separators; Require backslash; You decide |
| Which strict hash malformed fixtures should be mandatory? | Mismatch collision unsorted | Mismatch collision unsorted; Mismatch only; Planner chooses |
| When hash validation fails, should tests assert only `error_code::format_error` or also check diagnostic message text? | Code only | Code only; Code and message; You decide |

**Notes:** Strict structural/hash failures use `format_error`, while safe archive-name spelling variants are tolerated when hash validation and path normalization pass.

---

## Fixture Shape

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| How much success-fixture variety should Phase 4 require? | One rich archive | One rich archive; Two archives; Many cases; You decide |
| What should the rich TES3 success archive include? | Representative paths | Representative paths; Minimal files; Edge-heavy archive; You decide |
| How should Phase 4 fixture generation be organized? | Separate TES3 tool | Separate TES3 tool; Extend TES4 tool; Manual fixture bytes; You decide |
| Which malformed TES3 fixture set should be locked as mandatory? | Spec plus hash | Spec plus hash; Spec only; Broad matrix; You decide |

**Notes:** Use one representative generated archive for success behavior and a focused malformed set covering SPEC failures plus strict hash and offset-regression cases.

---

## the agent's Discretion

- Exact internal parser/source names, dispatch mechanics, and fixture entry names are left to researcher/planner discretion.
- Exact diagnostic message wording is discretionary; tests should assert error codes.

## Deferred Ideas

None.
