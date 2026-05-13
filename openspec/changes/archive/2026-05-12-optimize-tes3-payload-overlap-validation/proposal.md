## Why

TES3 BSA parsing currently checks each materialized payload span against every previously seen span, making malformed-overlap validation quadratic in the number of entries. Large Morrowind archives should preserve the same format validation while avoiding parser work that grows unnecessarily for valid high-entry-count archives.

## What Changes

- Replace TES3 payload overlap detection with an ordered span validation path that avoids per-entry scans across all prior entries.
- Preserve existing TES3 hash-order, duplicate-hash, hash/name, canonical-path, offset-relative, and archive-bound validation semantics before changing the overlap logic.
- Add focused coverage proving overlapping payload spans are still rejected and non-overlapping large span sets validate without quadratic behavior.
- Do not change public APIs, archive metadata shape, compression behavior, or dependencies.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `archive-record-metadata-validation`: Add TES3 payload overlap validation requirements that preserve format-compliant behavior while avoiding quadratic overlap checks.

## Impact

- Affected code: `src/formats/bsa/tes3_bsa_parser.cpp` and focused parser/fixture tests for TES3 BSA validation.
- Compatibility reference: `TES5Edit/Core/wbBSArchive.pas` confirms TES3 payload offsets are data-section-relative and should remain interpreted against the computed data section start.
- APIs and dependencies: no public API changes and no new dependencies.
