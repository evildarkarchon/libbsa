# Phase 10: tes3-write-support-and-bsa-format-completeness - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-09
**Phase:** 10-tes3-write-support-and-bsa-format-completeness
**Areas discussed:** Public API shape, Disk source timing, Fixture proof shape, Path byte policy

---

## Public API Shape

### Main Public Surface

| Option | Description | Selected |
|--------|-------------|----------|
| Single writer (Recommended) | Add a dedicated `tes3_bsa_writer`; TES3 has one target, and this mirrors existing format-specific writer objects without extra target ceremony. | yes |
| Target enum | Add `tes3_bsa_target::morrowind` plus `tes3_bsa_writer`; more uniform with TES4 but adds a one-value enum. | |
| Generic BSA writer | Move toward a shared BSA writer surface now; broader API change and higher risk for a phase scoped to TES3 only. | |
| You decide | Let researcher/planner pick the smallest surface that fits existing writer conventions. | |

**User's choice:** Single writer (Recommended)
**Notes:** Downstream agents should add a dedicated `tes3_bsa_writer` without a one-value target enum.

### Options Surface

| Option | Description | Selected |
|--------|-------------|----------|
| Overwrite only (Recommended) | Use a minimal `tes3_bsa_writer_options` with `overwrite_existing`; raw-only/no-dedupe/no-compression stays visible through absence of knobs. | yes |
| Future placeholders | Add reserved-looking option fields for future compatibility behavior; more API surface without Phase 10 requirements. | |
| No options struct | Use constructor defaults only and pass overwrite some other way; less consistent with existing writer APIs. | |
| You decide | Let planner choose, constrained to no compression, embedded-name, or dedupe controls. | |

**User's choice:** Overwrite only (Recommended)
**Notes:** Public TES3 options should stay intentionally minimal.

### Method Names

| Option | Description | Selected |
|--------|-------------|----------|
| Mirror existing (Recommended) | Use `add_file`, `add_bytes`, and `write_to`, matching TES4, BA2 GNRL, and public tests. | yes |
| More explicit names | Use names like `add_disk_file`, `add_memory_entry`, and `write_archive`; clearer but inconsistent with existing writers. | |
| You decide | Let planner choose exact names while keeping disk + memory + host-path finalization public. | |

**User's choice:** Mirror existing (Recommended)
**Notes:** Preserve the established writer naming pattern.

### Raw-Only Documentation

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit raw-only (Recommended) | Document that TES3 writer emits raw/uncompressed payloads and intentionally has no compression/dedupe/embedded-name controls in Phase 10. | yes |
| Quiet by omission | Do not call attention to absent controls; tests prove raw-only behavior. | |
| Compatibility note | Add a stronger note that compression/embedded names are out of scope and future compatibility work belongs to later phases. | |
| You decide | Let planner decide doc emphasis while keeping public behavior raw-only. | |

**User's choice:** Explicit raw-only (Recommended)
**Notes:** Public comments/docs should call out raw-only TES3 output.

---

## Disk Source Timing

### Source Read Timing

| Option | Description | Selected |
|--------|-------------|----------|
| Read at write_to (Recommended) | Keep disk entries path-backed until finalization, matching TES4/BA2 GNRL and avoiding forced input snapshots. | yes |
| Snapshot at add_file | Read and copy disk bytes immediately; deterministic if source files change later, but higher memory use and unlike existing raw archive writers. | |
| You decide | Let planner choose, constrained by copied memory entries and structured missing-source errors. | |

**User's choice:** Read at write_to (Recommended)
**Notes:** TES3 follows TES4/BA2 GNRL disk-source timing, not BA2 DX10 add-time snapshotting.

### Immediate Validation

| Option | Description | Selected |
|--------|-------------|----------|
| Paths only (Recommended) | Validate archive path and non-empty host path at add_file; missing/unreadable disk source reports from write_to like TES4. | yes |
| Stat source too | Also check source existence/readability during add_file, but still read bytes at write_to; earlier feedback but duplicated I/O checks. | |
| You decide | Let planner choose the minimum validation that preserves stable errors and existing writer expectations. | |

**User's choice:** Paths only (Recommended)
**Notes:** Missing disk sources should be finalization errors.

### Writer Reuse

| Option | Description | Selected |
|--------|-------------|----------|
| Same as existing (Recommended) | Keep `write_to` logically const and do not consume entries; repeated writes are allowed as implementation permits, with disk entries reread each time. | yes |
| Single-use finalizer | Make successful `write_to` consume or invalidate the writer; simpler lifecycle but inconsistent with current public writer shape. | |
| Not guaranteed | Do not test or document reuse; only require one finalization per writer instance. | |
| You decide | Let planner follow the established writer lifecycle unless implementation risk appears. | |

**User's choice:** Same as existing (Recommended)
**Notes:** Keep the public lifecycle consistent with existing writer objects.

### Missing Source Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Fail cleanly (Recommended) | Return structured `io_error`, leave no successful output, and clean temporary output state before returning. | yes |
| Best effort | Write remaining entries and omit failing sources; unsafe for archive correctness and unlike existing writer behavior. | |
| You decide | Let planner choose exact cleanup mechanics while preserving structured failure and no partial published archive. | |

**User's choice:** Fail cleanly (Recommended)
**Notes:** No best-effort partial archives.

---

## Fixture Proof Shape

### Committed Evidence Scope

| Option | Description | Selected |
|--------|-------------|----------|
| Canonical fixture (Recommended) | Commit one representative writer-produced archive plus manifest; cover the broader matrix with runtime writer tests. | yes |
| Committed matrix | Commit multiple writer-produced archives for disk/memory, path, offset, and validation scenarios; more evidence but more fixture churn. | |
| Manifest only | Commit generated manifest evidence but not writer-output archives; lighter repo impact but weaker byte-level artifact proof. | |
| You decide | Let planner balance fixture count against acceptance coverage. | |

**User's choice:** Canonical fixture (Recommended)
**Notes:** Runtime tests carry the broader coverage burden.

### Fixture Producer

| Option | Description | Selected |
|--------|-------------|----------|
| Public writer (Recommended) | Generate committed writer fixture evidence through the new public TES3 writer API so the artifact proves production behavior. | yes |
| Private serializer | Use a test-only low-level generator like existing TES3 reader fixtures; useful for malformed cases but weaker proof of the writer API. | |
| Both paths | Keep reader-fixture generator for malformed cases and add a public-writer generator for writer-output evidence. | |
| You decide | Let planner choose, but writer-output evidence must not rely only on private parser/test serializer behavior. | |

**User's choice:** Public writer (Recommended)
**Notes:** Existing fixture serializer remains useful context, but writer fixture evidence must prove the public writer.

### Manifest Contents

| Option | Description | Selected |
|--------|-------------|----------|
| Full layout facts (Recommended) | Record source kind, original/canonical path, stored hash low/high, raw TES3 data offset, archive payload offset, sizes, and payload bytes hex. | yes |
| Metadata only | Record paths, sizes, and hashes but omit payload hex and offset details; less useful for byte-level table checks. | |
| Archive hash only | Record a checksum of the whole archive; simple but hides which format rule changed when tests fail. | |
| You decide | Let planner design the manifest as long as it supports acceptance criteria without copyrighted bytes. | |

**User's choice:** Full layout facts (Recommended)
**Notes:** Manifest should support byte-level table checks and reader-backed validation.

### Golden Matching

| Option | Description | Selected |
|--------|-------------|----------|
| Structural bytes (Recommended) | Assert byte-level header/table/hash/name/offset/payload facts plus reader round-trip, without requiring one full-file golden comparison. | yes |
| Full byte golden | Compare the entire writer output to a committed archive byte-for-byte; strongest but can overconstrain harmless deterministic changes. | |
| Reader only | Trust reopen/extract tests without byte-level table assertions; too weak for this phase's TES3 layout requirements. | |
| You decide | Let planner decide exact assertions while preserving byte-level table proof. | |

**User's choice:** Structural bytes (Recommended)
**Notes:** Avoid overconstraining with whole-archive golden comparison unless planner finds a concrete need.

---

## Path Byte Policy

### Separator Handling

| Option | Description | Selected |
|--------|-------------|----------|
| Slash-normalize (Recommended) | Convert `\\` to `/`, serialize that preserved spelling, compute `hash_tes3` from those serialized bytes, and expose reopened `original_path` with `/`. | |
| Preserve bytes exactly | Serialize and hash caller separator bytes exactly; closer to raw input but conflicts with existing writer path policy and lookup consistency. | |
| Reject backslashes | Require callers to pass `/` paths only; strict but less convenient than existing normalization behavior. | |
| You decide | Let planner follow existing path helpers unless reference tracing proves TES3 needs a different byte policy. | yes |

**User's choice:** You decide
**Notes:** CONTEXT.md delegates exact separator byte policy to researcher/planner, with a default preference for existing slash-normalized writer policy unless TES3 evidence says otherwise.

### Case Handling

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve case (Recommended) | Serialize/hash caller-provided case, while canonical lowercase paths remain only for lookup and duplicate detection. | yes |
| Lowercase output | Serialize lowercase names for deterministic output; simpler but loses original path spelling and differs from existing preservation policy. | |
| You decide | Let planner choose exact case handling, constrained by `original_path` preservation and canonical duplicate checks. | |

**User's choice:** Preserve case (Recommended)
**Notes:** Case preservation is locked.

### Root-Level Entries

| Option | Description | Selected |
|--------|-------------|----------|
| Allow valid paths (Recommended) | Use shared archive path validation and allow root-level files if the normalizer accepts them; TES3 layout does not require TES4-style folder records. | yes |
| Require folders | Reject paths without a folder segment for consistency with TES4 writer tests, even though TES3 has a flat name table. | |
| You decide | Let planner trace current path normalizer and TES3 reference behavior before locking root-level support. | |

**User's choice:** Allow valid paths (Recommended)
**Notes:** Do not impose TES4 folder-record restrictions on TES3.

### Stored Hash Control

| Option | Description | Selected |
|--------|-------------|----------|
| Writer-owned hashes (Recommended) | Callers provide paths only; writer computes `hash_tes3` from the serialized name bytes and sorts by TES3 low32/high32 order. | yes |
| Advanced override | Expose a stored-hash override for compatibility experiments; powerful but risks malformed archives and expands public API. | |
| You decide | Let planner choose, constrained by acceptance tests for computed hashes and sorted hash records. | |

**User's choice:** Writer-owned hashes (Recommended)
**Notes:** No public stored-hash override in Phase 10.

---

## the agent's Discretion

- Exact separator byte policy is delegated to researcher/planner. Default to existing slash-normalized serialization/hashing unless TES3 reference tracing proves caller separator bytes must be preserved exactly.
- Exact private helper/file layout, CMake registration, test file organization, synthetic entry names, and payload bytes are delegated within the constraints captured in CONTEXT.md.

## Deferred Ideas

None.
