# Phase 07: TES4-Family BSA Write-New Support - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-08
**Phase:** 07-tes4-family-bsa-write-new-support
**Areas discussed:** Writer API shape, Path handling, Compression controls, Compatibility knobs

---

## Writer API Shape

| Question | Options | Selected |
|----------|---------|----------|
| What public API shape should downstream agents plan around for creating TES4-family BSA archives? | Writer object; One-shot function; Both now | Writer object |
| For memory-buffer entries added to the writer object, who should own the bytes before the archive is written? | Writer copies bytes; Caller keeps alive; Support both | Writer copies bytes |
| What output destination should Phase 7 expose first? | Host path only; Path and memory; Generic sink | Host path only |
| How should write-new behave when the destination archive path already exists? | Fail by default; Overwrite by default; You decide | Fail by default |

**Notes:** The selected shape favors a safe writer object with copied memory entries, host-path output, and explicit overwrite behavior if overwrite support exists.

---

## Path Handling

| Question | Options | Selected |
|----------|---------|----------|
| When callers add a disk file, how should they provide the archive-internal path? | Explicit archive path; Infer from root; Support both | Explicit archive path |
| How much original path spelling should the writer preserve in the archive name tables? | Preserve caller spelling; Canonicalize on write; You decide | Preserve caller spelling |
| What should happen if two added entries normalize to the same archive path? | Reject at add time; Reject at write time; Last one wins | Reject at write time |
| Should empty directories be representable in Phase 7 writer output? | No, files only; Yes, explicit dirs; You decide | No, files only |

**Notes:** The selected path behavior keeps host filesystem layout separate from archive-internal paths and preserves existing reader canonical/original path semantics.

---

## Compression Controls

| Question | Options | Selected |
|----------|---------|----------|
| How should per-entry compression options be expressed in the public writer API? | Inherit/raw/compressed; Exact codec enum; Boolean override | Inherit/raw/compressed |
| What should the archive-wide default compression option mean? | Profile default; Always raw default; Always compressed default | Profile default |
| How should zero-byte entries interact with compression requests? | Store raw; Honor compressed; You decide | Store raw |
| If compression fails or a requested compression mode is invalid for the target profile, how strict should the writer be? | Fail the write; Fallback to raw; Warn and continue | Fail the write |
| Should Phase 7 expose compression level controls, or keep compression level internal for now? | Internal only; Archive-level level; Per-entry level | Clarified as per-entry raw/compressed selection, not codec level tuning |
| Should codec compression level tuning stay internal for Phase 7, while per-entry raw/compressed selection is public? | Keep levels internal; Expose levels too; You decide | Keep levels internal |
| Should the writer choose the archive compression flag to match the archive-wide default, then use per-file toggle bits only for exceptions? | Yes; Always raw default flag; You decide | Yes |
| Should the writer compress tiny non-empty files when the effective policy says compressed? | Honor policy; Auto-store if larger; You decide | Honor policy |

**Notes:** The user clarified that the desired per-entry control is raw-vs-compressed selection per entry, not public codec compression-level tuning.

---

## Compatibility Knobs

| Question | Options | Selected |
|----------|---------|----------|
| How much direct control should callers have over archive flags in Phase 7? | Derive with safe options; Expose raw flags; Derive only | Derive with safe options |
| How should file category flags be derived for mixed-content archives? | Union by extension; Caller category option; Trace then decide | Union by extension |
| For embedded names, what compatibility stance should planning assume beyond the SPEC's explicit opt-in rule? | Default off, opt-in global; Per-entry opt-in; Trace then decide | Default off, opt-in global |
| For opt-in deduplication, which entries should be eligible to share payload offsets? | Same stored bytes; Same source bytes; You decide | Same stored bytes |

**Notes:** Compatibility controls should be named and safe rather than raw bitfields. Exact flag mappings remain a research/planning task, but the user-facing policy is locked.

---

## the agent's Discretion

- Exact public type names, function names, private helper boundaries, CMake organization, test filenames, fixture names, and manifest shape are left to researcher/planner discretion.
- Reference tracing is required for binary layout details, flag mapping, embedded-name compatibility, and sort/order behavior.

## Deferred Ideas

None - discussion stayed within phase scope.
