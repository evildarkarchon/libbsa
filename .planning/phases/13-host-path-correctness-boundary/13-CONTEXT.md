# Phase 13: host-path-correctness-boundary - Context

**Gathered:** 2026-05-13
**Status:** Ready for planning

## Phase Boundary

Phase 13 hardens the Windows host-filesystem boundary for archive open, validation, and follow-on payload reads without changing the public host-path API or archive-internal path semantics. The implementation must replace repeated read-side narrow host-file opens with one shared internal host-file boundary, then prove non-ASCII host-path correctness through committed black-box regression coverage for the locked representative archive set.

## Requirements (locked via SPEC.md)

**4 requirements are locked.** See `13-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `13-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- One shared internal host-path I/O boundary for the read/validate stack used by `archive_reader::open`, `validate_archive`, representative parser entry points, and representative extraction reads
- Regression coverage for non-ASCII Windows host paths on `TES4 BSA`, `Fallout 4 BA2 GNRL`, `Fallout 4 BA2 DX10`, and `Starfield BA2 GNRL`
- Public-surface proof for `archive_reader::open` from a non-ASCII host path
- Public-surface proof for `validate_archive(..., {.validate_entry_extractability = true})` from a non-ASCII host path
- Public-surface proof for at least one successful payload extraction after opening each representative archive from a non-ASCII host path

**Out of scope (from SPEC.md):**
- `TES3 BSA` non-ASCII host-path coverage - excluded to keep the representative set focused on the locked Phase 13 acceptance set
- Writer-side host-path correctness (`add_file`, writer disk-source reads, `write_to`) - excluded because Phase 13 is locked to archive open, validation, and read-side follow-on reads
- Public API changes such as replacing `std::string_view host_path` with `std::filesystem::path` - excluded because v1.1 is hardening-only and does not expand or redesign the public surface
- Archive-internal path normalization, hashing, lookup, or separator semantics - excluded because the bug is at the Windows host-filesystem boundary, not in Bethesda virtual-path behavior
- CI/preset/supported verification-lane reconciliation - excluded because that belongs to Phase 14
- New archive family support or broader behavior expansion - excluded because v1.1 is a hardening milestone for already-supported families

## Implementation Decisions

### Shared Host-Path Boundary
- **D-01:** Generalize the existing `src/detail/writer_disk_source.*` boundary into a neutrally named shared host-file helper in `src/detail/` rather than adding a parallel read-only helper.
- **D-02:** Rename the helper and its types now so the names match the new shared read/write role, and migrate existing writer call sites in the same phase instead of leaving compatibility shims behind.
- **D-03:** The shared boundary must cover all read-side host-file opens touched by the Phase 13 read/validate flow, including detection-prefix reads, size probes, parser opens, validation setup, and payload extraction opens. Do not limit the cleanup to only the representative test path.
- **D-04:** The helper surface should be layered: small neutral primitives plus focused helpers for common patterns like size inspection, prefix reads, exact reads, and bounded chunk iteration.
- **D-05:** Callers should continue to own stream lifetime locally, but they must obtain read-side streams through the shared helper. The helper should hand back `std::ifstream` streams, not introduce a new custom file wrapper in Phase 13.
- **D-06:** Error reporting stays context-driven. Shared mechanics are centralized, but callers still supply operation-specific diagnostics so public/read-side messages remain stable and phase-appropriate.
- **D-07:** Even though TES3 non-ASCII regression coverage is out of scope, TES3 read-side narrow opens should migrate to the same shared host-file boundary while the refactor is in flight so the repo actually lands on one read-side policy boundary.

### Reader State And Path Ownership
- **D-08:** Keep the public API unchanged: callers still pass UTF-8 host paths as `std::string_view`.
- **D-09:** Resolve the UTF-8 host path into the Windows-correct filesystem path once at the public API boundary, not lazily per helper call and not once per later operation.
- **D-10:** Store both forms internally after open succeeds: the original UTF-8 text for diagnostics/traceability and one resolved `std::filesystem::path` for actual host-file I/O.
- **D-11:** Represent those two forms as one small shared internal path value in `src/detail/` instead of widening signatures with separate text/path parameters or keeping subsystem-specific copies.
- **D-12:** After open succeeds, the resolved filesystem path is the only I/O source. The original UTF-8 text is diagnostics-only and must not become a fallback path-opening route.
- **D-13:** Follow-on extraction reads must reopen from the path stored in `archive_reader::state`; they must not re-resolve raw caller text elsewhere.
- **D-14:** Open, validation, and migrated writer call sites should converge on the same shared internal path type.

### Validation Contract
- **D-15:** Remove `validate_archive`'s duplicate readability preflight and trust the shared host-file boundary plus `archive_reader::open` as the one real setup path.
- **D-16:** Preserve the existing public result/report split. Empty input still fails fast with `invalid_argument`; unreadable or non-openable host paths still fail as direct `io_error` results; readable malformed archive bytes still become `validation_report` diagnostics.
- **D-17:** Unreadable host-path failures should surface the shared helper/open diagnostics directly rather than being reworded through a validation-specific preflight layer.
- **D-18:** If setup fails before parsing any archive bytes, `validate_archive` should return a failed `result<validation_report>` with no partial report object.
- **D-19:** `validate_entry_extractability` must continue to prove payload reads through the opened reader state and the public extraction path. Do not add a validation-only host-path or extraction codepath.
- **D-20:** Phase 13 tests should prove the validation cleanup only through black-box public behavior. Do not add white-box hooks or open-count assertions that couple tests to the exact internal call graph.

### Regression Coverage
- **D-21:** Add one dedicated cross-family Phase 13 regression suite for the non-ASCII host-path proof instead of scattering the main story across existing per-format tests.
- **D-22:** The suite should cover all three existing TES4 BSA fixtures (`v103`, `v104`, `v105`) plus the locked representative BA2 fixtures for Fallout 4 GNRL, Fallout 4 DX10, and Starfield GNRL.
- **D-23:** Copy only the archive-under-test into the non-ASCII temp location. Keep manifests and expected payload data in the normal generated-fixture tree.
- **D-24:** The non-ASCII proof must cover both a non-ASCII directory and a non-ASCII archive filename, not just one path segment.
- **D-25:** Use one stable curated naming token across the suite: ``libbsa-Ångström-日本語``. Keep the token deterministic and BMP-only rather than using a broader script matrix or random names.
- **D-26:** For the post-open payload proof, each representative archive only needs one canonical manifest-backed extraction target with expected bytes. Do not rerun every manifest entry from the non-ASCII path in this dedicated suite.
- **D-27:** The suite should stay black-box and deterministic: prove `archive_reader::open`, `validate_archive(..., {.validate_entry_extractability = true})`, and the canonical extraction result from the public API surface only.

### the agent's Discretion
None. The implementation shape is intentionally locked for downstream research and planning.

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone And Phase Scope
- `.planning/PROJECT.md` — v1.1 hardening constraints, Windows-only boundary, and the no-public-API-expansion rule that still applies to Phase 13
- `.planning/REQUIREMENTS.md` — `HOST-01`, `HOST-02`, and `HOST-03` requirement traceability for this phase
- `.planning/ROADMAP.md` — Phase 13 goal, dependency ordering, and milestone context
- `.planning/phases/13-host-path-correctness-boundary/13-SPEC.md` — locked requirements, scope boundaries, and acceptance criteria that must not be re-litigated
- `.planning/STATE.md` — current milestone position and sequencing context for downstream work

### Codebase Guidance
- `.planning/codebase/ARCHITECTURE.md` — current facade/detail layering, `archive_reader` state shape, and validation flow to preserve while inserting the host-file boundary
- `.planning/codebase/TESTING.md` — fixture-driven Catch2 patterns, temp-path setup style, manifest-backed assertions, and black-box testing conventions to reuse
- `.planning/codebase/CONCERNS.md` — known non-ASCII host-path bug, affected read-side file list, and the hardening rationale for this phase

### External Specs
- No external specs — requirements are fully captured in the planning artifacts above.

## Existing Code Insights

### Reusable Assets
- `src/detail/writer_disk_source.hpp` and `src/detail/writer_disk_source.cpp`: existing Windows-correct host-file boundary with exact/prefix/chunk helpers and caller-supplied diagnostics; this is the direct generalization target for Phase 13.
- `tests/fixtures/generated/archives/`: committed representative TES4 and BA2 archives plus manifests already exist, so Phase 13 can reuse legal checked-in fixtures instead of inventing new archive content.
- `tests/unit/validation_api_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `tests/unit/ba2_dx10_extraction_tests.cpp`: established helper patterns for temp-path setup, manifest-backed expected bytes, and black-box public API assertions.

### Established Patterns
- Public archive and validation entry points take `std::string_view` host paths, while implementation details own filesystem conversion and stream handling.
- `validate_archive` already treats `archive_reader::open` as the parser source of truth; Phase 13 should strengthen that pattern by removing the extra readability preflight instead of creating a parallel validation path.
- Read-side code currently opens files locally with `std::ifstream`; the Phase 13 boundary should replace those narrow opens without changing result/error-code semantics or archive-internal path rules.
- Tests prefer real committed fixtures, black-box behavior checks, and deterministic temp-file setup over internal hooks or test-only instrumentation.

### Integration Points
- `src/archive.cpp`: detection-prefix reads, file-size probing, `archive_reader::state`, and follow-on extraction dispatch all sit here.
- `src/validation.cpp`: public setup guards, setup/result/report split, and extractability validation flow must preserve current public semantics.
- Read-side narrow host-file opens currently exist in `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, and `src/formats/ba2/ba2_dx10_reader.cpp`.
- Existing writer call sites already rely on the current detail helper and must be migrated to the neutral shared names in the same phase.

## Specific Ideas

- Use the exact non-ASCII token ``libbsa-Ångström-日本語`` in both the temp directory path and the renamed archive filename for the dedicated Phase 13 suite.
- Keep the new suite focused on host-path correctness: copy only the archive under test into the non-ASCII location, but choose the canonical extraction target and expected bytes from the existing manifest data.
- Treat the dedicated Phase 13 suite as the cross-family audit surface for the host-path fix, while leaving broad per-format extraction matrices in their current ASCII-path suites.

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 13-host-path-correctness-boundary*
*Context gathered: 2026-05-13*
