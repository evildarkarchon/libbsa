# Phase 10: ba2-writers - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-07T03:16:56.0241173-07:00
**Phase:** 10-ba2-writers
**Areas discussed:** BA2 API shape, Target options, Native preview detail, DDS input handling

---

## BA2 API Shape

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Where should production BA2 writer entry points live? | Dedicated ba2_writer.hpp (Recommended) | Mirrors Phase 9 `bsa_writer.hpp`, keeps BA2 write types discoverable without crowding read-only `ba2.hpp`. | Yes |
| Where should production BA2 writer entry points live? | Extend ba2.hpp | Keeps all BA2 operations in one public header, but mixes reader metadata APIs with a large writer surface. | |
| Where should production BA2 writer entry points live? | Generic writer.hpp only | Smallest public surface, but conflicts with the SPEC need for BA2-native DDS/GNRL behavior beyond the Phase 8 harness. | |
| Where should production BA2 writer entry points live? | You decide | Let researcher/planner choose the public header placement while preserving public-header boundary rules. | |
| How should BA2 memory and disk inputs be represented publicly? | Separate input structs (Recommended) | Mirror BSA writer: distinct memory and disk entry types prevent invalid byte/path combinations and keep disk reads in planning. | Yes |
| How should BA2 memory and disk inputs be represented publicly? | Single variant-like entry | One entry type can hold either bytes or a host path, but needs validation for impossible combinations. | |
| How should BA2 memory and disk inputs be represented publicly? | Builder object | A stateful builder can collect mixed inputs, but adds lifetime/state complexity not used elsewhere in libbsa. | |
| How should BA2 memory and disk inputs be represented publicly? | You decide | Planner chooses the exact entry model while satisfying disk/memory acceptance criteria. | |
| Should GNRL and DDS writer inputs share one BA2 entry type or use subtype-specific entry types? | Subtype-specific entries (Recommended) | Use separate GNRL payload entries and DDS input entries so DDS analysis requirements are visible and not mixed with arbitrary payloads. | Yes |
| Should GNRL and DDS writer inputs share one BA2 entry type or use subtype-specific entry types? | One BA2 entry type | Simpler surface, but GNRL payload bytes and DDS source bytes need different validation and derived metadata. | |
| Should GNRL and DDS writer inputs share one BA2 entry type or use subtype-specific entry types? | You decide | Planner chooses exact shapes as long as DDS inputs analyze actual DDS bytes and GNRL handles ordinary payloads. | |
| What public operation shape should BA2 writers use? | Plan/finalize pairs (Recommended) | Mirror `plan_bsa_write`, `plan_bsa_write_from_disk`, and `finalize_bsa_write`; preserves existing operation-style API and plan-owned bytes. | Yes |
| What public operation shape should BA2 writers use? | Stateful ba2_writer object | Can accumulate files incrementally, but conflicts with the current stateless operation pattern and lifetime model. | |
| What public operation shape should BA2 writers use? | Finalize-only helpers | Convenient for callers, but hides required native preview and layout validation before sink writes. | |
| What public operation shape should BA2 writers use? | You decide | Planner picks exact names while keeping plan-then-finalize semantics. | |

**User's choice:** Dedicated `ba2_writer.hpp`, separate input structs, subtype-specific GNRL/DDS entries, and plan/finalize operation pairs.
**Notes:** User selected the recommended option for every BA2 API shape question.

---

## Target Options

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| How explicit should BA2 target selection be? | Exact variant enum (Recommended) | Separate values for FO4 GNRL v1/v7/v8, Starfield GNRL v2/v3, FO4 DX10 v1/v7/v8, and Starfield DX10 v3; no version/subtype mismatch possible. | Yes |
| How explicit should BA2 target selection be? | Subtype plus version fields | Fewer enum values, but planning must reject invalid combinations like Starfield DX10 v2. | |
| How explicit should BA2 target selection be? | Game-name presets | Ergonomic, but ambiguous for FO4 v1/v7/v8 and conflicts with avoiding loose strings. | |
| How explicit should BA2 target selection be? | You decide | Planner chooses target representation while preserving exact version support. | |
| How should Starfield v3 CompressionMethod be selected for BA2 writing? | Archive option (Recommended) | Target is Starfield v3, option chooses method 0 or method 3; matches reader summary and keeps codec routing archive-level. | Yes |
| How should Starfield v3 CompressionMethod be selected for BA2 writing? | Separate enum targets | Distinct targets like Starfield v3 deflate and Starfield v3 LZ4, very explicit but doubles target values. | |
| How should Starfield v3 CompressionMethod be selected for BA2 writing? | Per-entry method | Most flexible, but BA2 v3 stores `CompressionMethod` at archive level and would invite invalid mixed-codec archives. | |
| How should Starfield v3 CompressionMethod be selected for BA2 writing? | You decide | Planner chooses exact API while supporting only method 0 and method 3. | |
| What should the default compression behavior be for BA2 writer options? | Target defaults plus overrides (Recommended) | Each target has a safe archive default, while per-entry/per-chunk policy can force raw or compressed without extension inference. | Yes |
| What should the default compression behavior be for BA2 writer options? | Caller must choose every time | Maximum explicitness, but noisy for normal callers and inconsistent with Phase 9 option defaults. | |
| What should the default compression behavior be for BA2 writer options? | Infer from paths | Convenient for app tools, but rejected elsewhere because libbsa should not infer compression from extensions. | |
| What should the default compression behavior be for BA2 writer options? | You decide | Planner chooses defaults while preserving explicit compression policy and no extension inference. | |
| How much target information should the write plan expose back to callers? | Echo native target fields (Recommended) | Plan includes subtype, version, header size, and compression method so tests/tools can inspect exactly what will be emitted. | Yes |
| How much target information should the write plan expose back to callers? | Only enum target | Simpler, but downstream tests must parse emitted bytes to confirm basic target choices. | |
| How much target information should the write plan expose back to callers? | You decide | Planner decides plan target metadata while satisfying byte-level header acceptance tests. | |

**User's choice:** Exact target enum, archive-level Starfield v3 compression method option, target defaults plus overrides, and native target fields echoed in the plan.
**Notes:** User selected the recommended option for every target option question.

---

## Native Preview Detail

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| How should BA2 write plans represent GNRL versus DDS layout preview? | Subtype-specific sections (Recommended) | One `ba2_write_plan` can carry GNRL records or DDS texture/chunk records explicitly, avoiding meaningless fields for the other subtype. | Yes |
| How should BA2 write plans represent GNRL versus DDS layout preview? | One flat entry list | Simpler, but DDS chunk/mip details and GNRL record fields become awkward or hidden. | |
| How should BA2 write plans represent GNRL versus DDS layout preview? | Separate plan types | Very clear type separation, but duplicates finalization and common target/region metadata. | |
| How should BA2 write plans represent GNRL versus DDS layout preview? | You decide | Planner chooses exact type layout while keeping GNRL and DDS native details inspectable. | |
| Which GNRL native fields should be visible in the plan preview? | Full record/table preview (Recommended) | Expose name/directory hashes, FileTableOffset, record/table regions, payload offsets, packed/unpacked sizes, and compression state for tests and dry-run inspection. | Yes |
| Which GNRL native fields should be visible in the plan preview? | Read-back metadata only | Expose only path, sizes, offsets, and compression like `entry_metadata`; byte-level fields are verified by reopening/parsing. | |
| Which GNRL native fields should be visible in the plan preview? | You decide | Planner decides exact GNRL preview fields while acceptance still inspects emitted bytes. | |
| Which DDS/DX10 native fields should be visible in the plan preview? | Texture and chunk preview (Recommended) | Expose texture record metadata, chunk regions, mip ranges, DXGI format, dimensions, offsets, sizes, and compression for fixture and dry-run checks. | Yes |
| Which DDS/DX10 native fields should be visible in the plan preview? | Texture summary only | Expose final `texture_metadata`-like fields but hide native chunk table details. | |
| Which DDS/DX10 native fields should be visible in the plan preview? | You decide | Planner decides exact DDS preview fields while preserving chunk/mip acceptance coverage. | |
| How should BA2 deduplication appear in native preview metadata? | Shared region IDs and offsets (Recommended) | Mirror Phase 8/9: entries or chunks point to stable data-region IDs and archive-absolute offsets, with no public content hashes. | Yes |
| How should BA2 deduplication appear in native preview metadata? | Only offsets | Users can infer sharing from equal offsets, but tests lose an explicit stable sharing handle. | |
| How should BA2 deduplication appear in native preview metadata? | Public content hashes | Useful for tooling, but Phase 8 explicitly avoided public digest semantics. | |
| How should BA2 deduplication appear in native preview metadata? | You decide | Planner chooses representation while proving dedup-on/off changes native layout. | |

**User's choice:** Subtype-specific preview sections, full GNRL record/table preview, DDS texture/chunk preview, and dedup shown through shared region IDs plus offsets.
**Notes:** User selected the recommended option for every native preview question.

---

## DDS Input Handling

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| What should BA2 DDS writer inputs contain? | Actual DDS bytes/files only (Recommended) | Public callers provide DDS memory bytes or disk DDS files plus archive paths; libbsa derives all texture metadata and chunk records internally. | Yes |
| What should BA2 DDS writer inputs contain? | DDS plus optional descriptors | Allows callers to override metadata, but risks mismatches between source bytes and BA2 records. | |
| What should BA2 DDS writer inputs contain? | Caller-supplied descriptors | Flexible for synthetic cases, but conflicts with the SPEC requirement to analyze actual DDS inputs. | |
| What should BA2 DDS writer inputs contain? | You decide | Planner chooses exact entry fields while preserving actual DDS input analysis. | |
| When should DDS analysis and validation happen? | During planning (Recommended) | Planning succeeds only after DDS bytes are loadable/analyzable and the plan owns derived chunk payloads; finalization only streams planned bytes. | Yes |
| When should DDS analysis and validation happen? | During finalization | Defers work, but finalization could fail after table decisions and would need retained file/input lifetimes. | |
| When should DDS analysis and validation happen? | Split analysis and final validation | Analyze during planning but validate reconstructed DDS only in tests; faster, but weaker no-surprise planning semantics. | |
| When should DDS analysis and validation happen? | You decide | Planner chooses timing while preserving plan-owned bytes and no retained input lifetimes. | |
| How much DDS chunking control should public callers get in Phase 10? | Automatic chunking (Recommended) | libbsa derives mip/chunk layout from DDS analysis and target rules; no public native chunk descriptor API in Phase 10. | Yes |
| How much DDS chunking control should public callers get in Phase 10? | Optional chunk hints | Callers can influence chunk grouping, but planner must validate hints against DDS mip ranges and BA2 target rules. | |
| How much DDS chunking control should public callers get in Phase 10? | Manual chunks | Maximum control, but exposes BA2-native DDS internals and risks bypassing analysis. | |
| How much DDS chunking control should public callers get in Phase 10? | You decide | Planner chooses controls while meeting proper mipmap chunking acceptance. | |
| How should unsupported or malformed DDS inputs fail? | Fail during planning (Recommended) | Return structured errors for malformed DDS, unsupported layouts, unrepresentable mip/chunk mapping, or unsupported DXGI output; no partial plan. | Yes |
| How should unsupported or malformed DDS inputs fail? | Best-effort metadata only | May allow opening metadata but fail later, matching some reader behavior, but weaker for a writer that must emit valid archives. | |
| How should unsupported or malformed DDS inputs fail? | You decide | Planner picks exact error classes while preserving no transcoding and no invalid output. | |

**User's choice:** Actual DDS bytes/files only, analysis and validation during planning, automatic chunking, and planning-time structured failures for malformed or unsupported DDS inputs.
**Notes:** User selected the recommended option for every DDS input handling question.

---

## the agent's Discretion

No areas were delegated to the agent's discretion.

## Deferred Ideas

None.
