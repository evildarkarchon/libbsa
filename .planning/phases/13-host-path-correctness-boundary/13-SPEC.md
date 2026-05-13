# Phase 13: Host Path Correctness Boundary - Specification

**Created:** 2026-05-13
**Ambiguity score:** 0.10 (gate: <= 0.20)
**Requirements:** 4 locked

## Goal

Consumers can open, validate, and perform a minimal post-open payload read on representative supported archives from Windows host paths containing non-ASCII characters, without changing the public host-path API.

## Background

The shipped library already exposes `archive_reader::open(std::string_view host_path)` and `validate_archive(std::string_view host_path, ...)`, but the read-side implementation still opens host files through narrow-string `std::ifstream` calls in `src/archive.cpp`, `src/validation.cpp`, and multiple parser and reader translation units for TES4 BSA and BA2 families. On Windows, that creates a correctness gap for non-ASCII host paths even though archive-internal path handling is already a separate normalized string domain. `src/detail/writer_disk_source.cpp` already routes writer-side disk-source access through `std::filesystem::path`, so the immediate shipped bug is concentrated in archive open, validation, and follow-on read paths. Current tests exercise ASCII fixture and temp paths, but they do not lock non-ASCII host-path behavior for representative shipped BSA and BA2 families.

## Requirements

1. **Read-side host-path boundary**: The read/validate stack uses one shared internal Windows-aware host-path boundary instead of repeating narrow host-file opens.
   - Current: `src/archive.cpp`, `src/validation.cpp`, and read-side parser/reader files open host files with narrow `std::ifstream{std::string{host_path}}` patterns, so host-path conversion is repeated and Windows non-ASCII handling is not a single enforced policy boundary.
   - Target: Archive open, validation, parser, and read-side extraction paths that touch the host filesystem route through one shared internal host-path I/O boundary that accepts the existing public UTF-8 host-path text and opens files through Windows-correct filesystem path handling.
   - Acceptance: The Phase 13 implementation removes direct narrow-string host-file opens from the read/validate stack files touched by `archive_reader::open`, `validate_archive`, and representative follow-on read operations, while the public `archive_reader::open` and `validate_archive` signatures remain unchanged.

2. **Representative open success**: Consumers can open representative supported archives from Windows host paths containing non-ASCII characters.
   - Current: The repo has fixture coverage for TES4 BSA, Fallout 4 BA2 GNRL, Fallout 4 BA2 DX10, and Starfield BA2 GNRL behavior, but those archives are opened from ASCII fixture roots or ASCII temp paths.
   - Target: Committed regression tests copy representative TES4 BSA, Fallout 4 BA2 GNRL, Fallout 4 BA2 DX10, and Starfield BA2 GNRL archives to non-ASCII Windows temp paths and prove `archive_reader::open` succeeds for each one.
   - Acceptance: Automated tests pass only if `archive_reader::open` succeeds on those four representative archives when each archive file itself resides under a non-ASCII host path.

3. **Representative validation success**: Consumers can validate representative supported archives from Windows host paths containing non-ASCII characters, including extractability validation.
   - Current: `validate_archive` first checks host-path readability and then reuses the reader path, but current tests do not lock non-ASCII host-path validation behavior, and the default validation coverage does not prove follow-on archive reads under `validate_entry_extractability=true`.
   - Target: Committed regression tests validate the same representative TES4 BSA, Fallout 4 BA2 GNRL, Fallout 4 BA2 DX10, and Starfield BA2 GNRL archives from non-ASCII host paths with `validate_entry_extractability = true`.
   - Acceptance: Automated tests pass only if `validate_archive(non_ascii_path, {.validate_entry_extractability = true})` returns a successful `validation_report` with no fatal errors for each representative archive.

4. **Minimal post-open read proof**: The phase proves that at least one payload read still works after a successful non-ASCII-path open.
   - Current: Existing reader tests prove extraction behavior on normal paths, but non-ASCII host-path coverage does not currently lock any follow-on payload read after `archive_reader::open` succeeds.
   - Target: Committed regression tests open each representative archive from a non-ASCII host path and successfully perform at least one payload extraction through the public reader surface.
   - Acceptance: Automated tests pass only if each representative archive opened from a non-ASCII host path can successfully extract at least one known entry and the extracted bytes match the existing expected fixture payload.

## Boundaries

**In scope:**
- One shared internal host-path I/O boundary for the read/validate stack used by `archive_reader::open`, `validate_archive`, representative parser entry points, and representative extraction reads
- Regression coverage for non-ASCII Windows host paths on `TES4 BSA`, `Fallout 4 BA2 GNRL`, `Fallout 4 BA2 DX10`, and `Starfield BA2 GNRL`
- Public-surface proof for `archive_reader::open` from a non-ASCII host path
- Public-surface proof for `validate_archive(..., {.validate_entry_extractability = true})` from a non-ASCII host path
- Public-surface proof for at least one successful payload extraction after opening each representative archive from a non-ASCII host path

**Out of scope:**
- `TES3 BSA` non-ASCII host-path coverage - excluded to keep the representative set focused on the locked Phase 13 acceptance set
- Writer-side host-path correctness (`add_file`, writer disk-source reads, `write_to`) - excluded because Phase 13 is locked to archive open, validation, and read-side follow-on reads
- Public API changes such as replacing `std::string_view host_path` with `std::filesystem::path` - excluded because v1.1 is hardening-only and does not expand or redesign the public surface
- Archive-internal path normalization, hashing, lookup, or separator semantics - excluded because the bug is at the Windows host-filesystem boundary, not in Bethesda virtual-path behavior
- CI/preset/supported verification-lane reconciliation - excluded because that belongs to Phase 14
- New archive family support or broader behavior expansion - excluded because v1.1 is a hardening milestone for already-supported families

## Constraints

- Windows-only: the requirements apply to Windows host-path behavior and do not reopen cross-platform support.
- Keep the public API unchanged: `archive_reader::open` and `validate_archive` continue to accept `std::string_view host_path`.
- Keep archive virtual-path behavior unchanged: the fix may change host-file opening only and must not alter archive member normalization, hashing, lookup keys, or separator policy.
- Do not add new runtime dependencies for host-path handling.
- Use committed regression tests against representative existing fixtures copied to non-ASCII temp paths rather than inventing a new public feature surface.

## Acceptance Criteria

- [ ] `archive_reader::open` succeeds when a representative `TES4 BSA` archive is copied to a non-ASCII Windows host path.
- [ ] `archive_reader::open` succeeds when representative `Fallout 4 BA2 GNRL`, `Fallout 4 BA2 DX10`, and `Starfield BA2 GNRL` archives are copied to non-ASCII Windows host paths.
- [ ] `validate_archive(non_ascii_path, {.validate_entry_extractability = true})` returns a successful report for the representative `TES4 BSA`, `Fallout 4 BA2 GNRL`, `Fallout 4 BA2 DX10`, and `Starfield BA2 GNRL` archives.
- [ ] After opening each representative archive from a non-ASCII host path, at least one known entry can be extracted successfully and matches the existing expected fixture payload bytes.
- [ ] The read/validate stack no longer uses direct narrow-string host-file opens in the code paths exercised by `archive_reader::open`, `validate_archive`, and the representative follow-on payload reads.
- [ ] The public host-path API remains `std::string_view`-based with no new public path types or host-path overloads introduced in Phase 13.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.92  | 0.75  | ✓      | Consumer-visible outcome and representative archive set are explicit |
| Boundary Clarity    | 0.94  | 0.70  | ✓      | In-scope and out-of-scope edges are explicitly locked |
| Constraint Clarity  | 0.88  | 0.65  | ✓      | No public API changes, no archive-path semantic changes, Windows-only |
| Acceptance Criteria | 0.86  | 0.70  | ✓      | Pass/fail checks cover open, validate, extraction, and boundary invariants |
| **Ambiguity**       | 0.10  | <=0.20| ✓      | Gate passed after three rounds |

Status: ✓ = met minimum, ⚠ = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective      | Question summary | Decision locked |
|-------|------------------|------------------|-----------------|
| 1 | Researcher | Which families count as representative, and is this a behavior fix or just refactor prep? | Phase 13 closes a shipped Windows correctness bug and must include at least one TES4-family BSA plus BA2 coverage behind a shared internal read/validate host-path boundary |
| 2 | Researcher + Simplifier | What is the minimum viable archive/test surface for done? | Required representative set is `TES4 BSA`, `FO4 BA2 GNRL`, `FO4 BA2 DX10`, and `Starfield BA2 GNRL`; validation must include extractability; each archive also needs one successful post-open extraction |
| 3 | Boundary Keeper | What must stay out of scope even if adjacent code is tempting to touch? | `TES3`, writer-side host-path work, public API changes, archive-path semantics changes, and Phase 14 verification-lane work are all explicitly excluded |

---

*Phase: 13-host-path-correctness-boundary*
*Spec created: 2026-05-13*
*Next step: /gsd-discuss-phase 13 - implementation decisions (how to build what's specified above)*
