# Phase 15: reader-backend-dispatch-cleanup - Context

**Gathered:** 2026-05-14
**Status:** Ready for planning

## Phase Boundary

Phase 15 collapses repeated archive-family dispatch in the public `archive_reader` facade into one open-time-selected private backend seam while preserving the existing public API, lookup and extraction behavior, bulk extraction behavior, and current error semantics across the supported reader families.

## Requirements (locked via SPEC.md)

**3 requirements are locked.** See `15-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `15-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Internal dispatch cleanup for the public `archive_reader` surface in `src/archive.cpp`
- One open-time-selected private backend seam reused by `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries`
- Behavior-preserving runtime regression coverage for representative current reader backends and BA2 GNRL variant coverage
- Source/policy guardrails that fail if public reader operations regain repeated family branching
- Preservation of the current public reader API and current reader-visible semantics while this internal cleanup lands

**Out of scope (from SPEC.md):**
- Parser hotspot extraction or BA2 DX10 preparer refactors - excluded because those belong to Phase 16
- Validation-surface behavior changes - excluded because Phase 15 is locked to the public reader surface, not `validate_archive`
- New archive-family support or reader capability expansion - excluded because v1.1 is hardening-only and this phase is semantics-preserving cleanup
- Public API redesign such as new reader-specific public types, new public methods, or replacing the current `archive_reader` shape - excluded because the user locked this phase to outcome preservation, not API expansion
- Generic plugin, registry, or extensibility frameworks - excluded because the architecture research explicitly recommends a small private seam instead of a broad framework
- Writer, dedupe, DX10 temp-staging, or broader performance work - excluded because those belong to later roadmap phases

## Implementation Decisions

### Backend seam
- **D-01:** The private reader backend seam should be a function table selected once at `archive_reader::open` time.
- **D-02:** The function-table type and related seam wiring should stay local to `src/archive.cpp`, not grow into a broader private detail API.
- **D-03:** Phase 15 should reuse only the current shared reader state shape (`metadata`, `entries`, and resolved `detail::host_file_path`) and should not add backend-specific payload state unless later phases explicitly need it.
- **D-04:** Open-time backend selection should use one small private helper to assemble the chosen function table and final reader state so the `open` body stays centralized but readable.

### Operation ownership
- **D-05:** The backend seam should own the family-sensitive lookup and entry-payload primitives, while shared convenience orchestration stays in the facade.
- **D-06:** `extract()` should reuse the same backend-selected lookup flow as `extract_bytes()` and `extract_entries()` instead of repeating archive-family lookup branching inline.
- **D-07:** `contains()` should become a shared wrapper over the backend-selected `find()` path rather than remaining a separate backend callback.
- **D-08:** Exact request-string coalescing and per-request result mirroring in `extract_entries()` stay in shared facade logic; they are public-surface orchestration, not backend-specific behavior.

### BA2 backend split
- **D-09:** The open-time seam should use separate private backend identities for `ba2_gnrl` and `ba2_dx10`.
- **D-10:** Fallout 4 BA2 GNRL and Starfield BA2 GNRL v3 should stay under one shared `ba2_gnrl` backend identity; their codec and metadata differences remain internal to the GNRL reader/parser path.
- **D-11:** Backend identity should replace the current `is_ba2_dx10` flag in reader state instead of keeping both in parallel.
- **D-12:** Private backend naming should mirror the current repo vocabulary: `tes3_bsa`, `tes4_bsa`, `ba2_gnrl`, and `ba2_dx10`.

### Guardrail tests
- **D-13:** Add one dedicated cross-family Phase 15 runtime regression suite rather than scattering the whole dispatch contract only across existing family suites.
- **D-14:** That dedicated suite should exercise `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries` at least once per representative family while reusing existing committed fixtures instead of recreating every deeper family-specific matrix.
- **D-15:** The source-policy guard over `src/archive.cpp` should be method-scoped: fail if the affected public reader methods regain archive-family branching, while still allowing one centralized open-time/backend seam elsewhere in the file.
- **D-16:** The source-policy guard should lock only the negative invariant and should not require one exact helper, callback-table, or backend identifier name.

### the agent's Discretion
None. The discussion locked the seam shape, ownership split, BA2 backend granularity, and proof style closely enough that downstream research and planning should not reopen them.

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and locked requirements
- `.planning/PROJECT.md` — current v1.1 hardening boundaries, Windows-only scope, and the no-public-API-expansion rule that still applies here.
- `.planning/REQUIREMENTS.md` — `DISP-01` and `DISP-02` traceability for this phase.
- `.planning/ROADMAP.md` — Phase 15 goal, dependency ordering, and milestone context.
- `.planning/STATE.md` — current sequencing and milestone position.
- `.planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md` — carry-forward decision that the resolved `detail::host_file_path` boundary is the only post-open reopen path.
- `.planning/phases/14-verification-lane-truthfulness/14-CONTEXT.md` — recent repo-reading policy-test patterns and the current planning/documentation layering expectations.
- `.planning/phases/15-reader-backend-dispatch-cleanup/15-SPEC.md` — locked requirements, boundaries, and acceptance criteria. MUST read before planning.

### Live reader seam and integration files
- `include/libbsa/archive.hpp` — unchanged public `archive_reader` API surface that Phase 15 must preserve.
- `src/archive.cpp` — current reader state, open-time detection path, repeated public-facade dispatch, and bulk extraction orchestration that this phase is cleaning up.
- `src/formats/bsa/tes3_bsa_reader.hpp` — current TES3 lookup and payload helper shape that can map directly into the private dispatch table.
- `src/formats/bsa/tes4_bsa_reader.hpp` — current TES4-family lookup and payload helper shape that can map directly into the private dispatch table.
- `src/formats/ba2/ba2_gnrl_reader.hpp` — current BA2 GNRL lookup and payload helper shape, including Starfield GNRL codec-routing behavior.
- `src/formats/ba2/ba2_dx10_reader.hpp` — current BA2 DX10 lookup and DDS-reconstruction payload helper shape.

### Behavior and policy tests
- `tests/unit/archive_reader_tests.cpp` — compile-surface and basic public reader contract coverage.
- `tests/unit/bulk_extraction_tests.cpp` — current shared bulk extraction behavior, including duplicate exact-request coalescing and request-order result mirroring that must stay facade-owned.
- `tests/unit/tes3_bsa_reader_tests.cpp` — existing TES3 reader behavior coverage to preserve.
- `tests/unit/tes4_bsa_reader_tests.cpp` — existing TES4-family reader behavior coverage to preserve.
- `tests/unit/ba2_gnrl_reader_tests.cpp` — existing BA2 GNRL reader behavior coverage to preserve.
- `tests/unit/ba2_dx10_extraction_tests.cpp` — existing BA2 DX10 extraction behavior coverage to preserve.
- `tests/unit/host_file_writer_name_tests.cpp` — existing source-reading policy-test pattern against `src/archive.cpp` and related private seams.
- `tests/unit/validation_policy_tests.cpp` — existing repo-reading policy-test style that shows how the repo already locks structural invariants with tests.

### Codebase guidance
- `.planning/codebase/STACK.md` — current C++20, Catch2, and test/tooling stack.
- `.planning/codebase/ARCHITECTURE.md` — current facade/detail layering, `archive_reader` state model, and the specific repeated-dispatch concern already called out in architecture notes.
- `.planning/codebase/INTEGRATIONS.md` — current filesystem-only runtime boundary and test/tooling integration context.
- `.planning/codebase/CONCERNS.md` — the explicit repeated public-reader dispatch concern this phase is addressing.

### External specs
- No external specs — requirements are fully captured in the planning artifacts above.

## Existing Code Insights

### Reusable Assets
- `src/archive.cpp`: `archive_reader::open` already performs one-time detection and parser selection, so Phase 15 can build on an existing open-time seam rather than inventing a new detection layer.
- `archive_reader::state` in `src/archive.cpp`: the current immutable shared state already carries the common data that every backend needs after open.
- `src/formats/bsa/tes3_bsa_reader.hpp`, `src/formats/bsa/tes4_bsa_reader.hpp`, `src/formats/ba2/ba2_gnrl_reader.hpp`, and `src/formats/ba2/ba2_dx10_reader.hpp`: the current reader helper APIs are already close to a uniform callback-table shape.
- `tests/unit/bulk_extraction_tests.cpp`: existing regression coverage already locks bulk duplicate coalescing, request-order result mirroring, and worker-count semantics.
- `tests/unit/host_file_writer_name_tests.cpp`: existing source-reading policy-test pattern already checks `src/archive.cpp` without relying on runtime behavior alone.

### Established Patterns
- Public reader APIs return stable `libbsa::result<T>` values and currently distinguish lookup absence, invalid archive path syntax, and extraction failures through shared error-code semantics that must remain unchanged.
- After open succeeds, archive reopen I/O stays on the resolved `detail::host_file_path` boundary from Phase 13; the new backend seam must reuse that contract rather than reopen raw caller text.
- Format-specific reader logic is already split by concrete family/subtype helper modules, not by a generic registry or plugin framework.
- Convenience APIs are already composed in the facade over lower-level primitives (`extract_bytes` and `extract_entries`), which supports keeping non-family-specific orchestration shared.

### Integration Points
- `src/archive.cpp`: add the private backend function table, wire open-time selection, and collapse per-method family branching into backend-selected calls.
- Family reader helpers and implementations under `src/formats/bsa/` and `src/formats/ba2/`: these stay the concrete backend operation providers.
- `tests/unit/`: add one dedicated Phase 15 cross-family runtime suite and one method-scoped source-policy guard for `src/archive.cpp`.

## Specific Ideas

- Mirror the current family vocabulary for backend identities: `tes3_bsa`, `tes4_bsa`, `ba2_gnrl`, and `ba2_dx10`.
- Keep the source-policy guard negative-only and method-scoped so it protects the contract without freezing one exact helper name.
- Use the dedicated Phase 15 runtime suite to cover the full public reader surface once per representative family, while leaving deeper family-specific edge cases in their existing suites.

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 15-reader-backend-dispatch-cleanup*
*Context gathered: 2026-05-14*
