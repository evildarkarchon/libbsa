# Phase 09: bsa-writers - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-06T23:07:43.3423504-07:00
**Phase:** 09-bsa-writers
**Areas discussed:** BSA API shape, Disk input model, Compression and embed names, BSA ordering policy

---

## BSA API Shape

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Public entry point shape | BSA-specific wrappers | Add BSA-focused helpers layered over Phase 8 `plan_archive_write` / `finalize_archive_write`, matching existing BSA operation style. | yes |
| Public entry point shape | Generic writer only | Keep `plan_archive_write` as the sole public planner and express BSA behavior only through `writer_target` options. | |
| Public entry point shape | You decide | Let researcher/planner choose the smallest API that satisfies `09-SPEC.md` and existing patterns. | |
| Variant selection | Explicit target enum | Expose a BSA-specific target value for TES3, TES4 v103, FO3/FNV/Skyrim LE v104, and SSE v105. | yes |
| Variant selection | Factory helpers | Expose named helpers like `morrowind_bsa_target()` and `skyrim_se_bsa_target()`. | |
| Variant selection | You decide | Let the planner pick enum vs helpers as long as all variants are unambiguous. | |
| Plan preview detail | Expose BSA details | Include BSA-relevant preview metadata such as folder/file table regions, hashes, flags, and payload offsets. | yes |
| Plan preview detail | Generic preview only | Keep public preview to generic plan records; inspect native details privately in tests. | |
| Plan preview detail | You decide | Expose only the minimum needed for acceptance and consumer usefulness. | |
| Harness treatment | Keep harness test-only | Production BSA wrappers emit native BSA bytes; LBSW remains tests-only. | yes |
| Harness treatment | Keep generic public path | Allow generic harness output to remain public while BSA wrappers are added separately. | |
| Harness treatment | You decide | Preserve existing tests while avoiding false production writer claims. | |

**User's choices:** BSA-specific wrappers; explicit target enum; expose BSA details; keep harness test-only.
**Notes:** Existing `open_bsa` / `extract_bsa_entry` operation style and Phase 8 writer core are the main code-context anchors.

---

## Disk Input Model

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Disk input shape | Explicit file pairs | Callers provide host file path plus archive virtual path per entry. | yes |
| Disk input shape | Root directory mapping | Callers provide a root folder and libbsa derives archive paths from relative disk paths. | |
| Disk input shape | Both forms | Expose explicit pairs plus a convenience root-relative helper. | |
| Read timing | During planning | Planning reads disk files into owned stored payload regions; finalization retains no file handles. | yes |
| Read timing | During finalization | Plans retain host file references and finalization streams file bytes later. | |
| Read timing | You decide | Planner chooses smallest approach preserving deterministic preview. | |
| Directory scanning | No scanning | Only accept explicit file mappings; consumers own recursion/filtering/symlink policy. | yes |
| Directory scanning | Basic scanner | Add a root-recursive helper with simple relative-path derivation. | |
| Directory scanning | You decide | Include scanning only if necessary for SPEC acceptance. | |
| Entry type shape | Separate entry structs | Use distinct public value types for memory entries and disk file mappings. | yes |
| Entry type shape | Single variant entry | One public entry type can hold either bytes or a host path. | |
| Entry type shape | You decide | Planner chooses minimal type shape with explicit ownership. | |

**User's choices:** Explicit file pairs; read during planning; no scanning; separate entry structs.
**Notes:** Disk behavior should not become productized packing workflow policy.

---

## Compression and Embed Names

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| Archive-default compression | Explicit option | Target/options state whether archive-default compression is on; per-entry policy can force raw/compressed. | yes |
| Archive-default compression | Target defaults | Pick built-in defaults per variant and let callers override only when needed. | |
| Archive-default compression | You decide | Planner chooses defaults as long as tests lock behavior. | |
| Per-file override encoding | Compute XOR flags | Callers request semantic policy; writer computes BSA-native high bit using reader XOR model. | yes |
| Per-file override encoding | Expose raw flag bit | Let callers directly control the BSA file-size compression flag. | |
| Per-file override encoding | You decide | Planner chooses but output must reopen with expected compression state. | |
| Embedded-name writing | Archive-level opt-in | Default off; if enabled, set `ARCHIVE_EMBEDNAME` and prefix each payload with its archive name. | yes |
| Embedded-name writing | Per-entry opt-in | Allow embedded names on selected entries only. | |
| Embedded-name writing | Do not support | Reject embedded-name writing in Phase 9. | |
| Unsupported combinations | Structured failure | Return `unsupported_format` or `malformed_archive`; do not silently downgrade or use fallback codecs. | yes |
| Unsupported combinations | Best-effort downgrade | Emit raw/non-embedded archive when requested behavior cannot be represented. | |
| Unsupported combinations | You decide | Planner chooses exact error codes while preserving no-fallback policy. | |

**User's choices:** Explicit option; compute XOR flags; archive-level opt-in embedded names; structured failure.
**Notes:** Existing reader behavior in `src/bsa_reader.cpp` is the compatibility anchor.

---

## BSA Ordering Policy

| Question | Option | Description | Selected |
|----------|--------|-------------|----------|
| TES3 order | TES3 hash order | Sort records by TES3-compatible hash order regardless of caller input, with deterministic tie handling. | yes |
| TES3 order | Normalized path order | Use Phase 8 path sort for consistency, then write hashes alongside entries. | |
| TES3 order | You decide | Researcher/planner traces BSArchPro and locks exact TES3 order. | |
| TES4-family order | Reference-compatible order | Trace BSArchPro/TES5Edit and emit deterministic folder/file order matching target expectations. | yes |
| TES4-family order | Normalized path order | Keep Phase 8 sorted path order and group folders from that order. | |
| TES4-family order | You decide | Planner chooses after research, with deterministic emitted-byte tests. | |
| BSA dedup | Only if proven compatible | Verify shared-offset behavior before enabling; otherwise dedup requests fail. | yes |
| BSA dedup | Enable for BSA | Allow identical post-policy payloads to share offsets when dedup is requested. | |
| BSA dedup | Disable for BSA | Always reject dedup for BSA targets in Phase 9. | |
| Empty folders | No empty folders | Write only folders implied by file entries. | yes |
| Empty folders | Allow empty folders | Expose zero-file folder record support. | |
| Empty folders | You decide | Include empty folders only if reference research proves it required. | |

**User's choices:** TES3 hash order; reference-compatible TES4-family order; dedup only if proven compatible; no empty folders.
**Notes:** Ordering and BSA dedup compatibility are explicit research targets.

---

## the agent's Discretion

No selected area was left to the agent's discretion.

## Deferred Ideas

None - discussion stayed within phase scope.
