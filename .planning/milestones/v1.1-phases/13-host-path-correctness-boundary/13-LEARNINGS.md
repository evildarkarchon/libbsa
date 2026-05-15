---
phase: 13
phase_name: "host-path-correctness-boundary"
project: "libbsa"
generated: "2026-05-14"
counts:
  decisions: 10
  lessons: 8
  patterns: 8
  surprises: 6
missing_artifacts:
  - "13-UAT.md"
---

# Phase 13 Learnings: host-path-correctness-boundary

## Decisions

### Land Neutral Host File Naming Before Path Semantics
The writer-only `writer_disk_source` helper family was renamed into a neutral shared `host_file` seam before introducing the shared path contract.

**Rationale:** Later open, parser, validation, and extraction work needed to build on final shared naming instead of carrying helper-rename debt through the phase.
**Source:** 13-01-SUMMARY.md

---

### Preserve Caller-Owned Diagnostics
Writer call sites kept their file-specific diagnostic strings instead of centralizing user-facing error text inside the shared helper.

**Rationale:** The phase changed internal host-file plumbing, not the observable diagnostic contract for already-migrated writer flows.
**Source:** 13-01-SUMMARY.md

---

### Use Temporary Compatibility Only Within The Same Plan
A temporary compatibility bridge was acceptable during the helper rename, but it was removed before Plan 13-01 completed.

**Rationale:** The full test target linked the whole writer stack, so the bridge allowed an incremental RED/GREEN migration without leaving backward-compatibility code behind.
**Source:** 13-01-SUMMARY.md

---

### Resolve Once Into `host_file_path`
The shared `detail::host_file_path` value stores original UTF-8 text and one resolved filesystem path, with file I/O routed through the resolved path.

**Rationale:** Migrated callers need one Windows path interpretation policy while retaining original text only for diagnostics.
**Source:** 13-02-SUMMARY.md

---

### Allow Resolved-Path Helper Overloads
`host_file` helpers accept either `host_file_path` or already-resolved `std::filesystem::path` inputs.

**Rationale:** Internal resolved paths such as BA2 DX10 snapshot files should not require synthetic UTF-8 round trips just to call shared helpers.
**Source:** 13-02-SUMMARY.md

---

### Keep The Public Reader API Stable
`archive_reader::open(std::string_view)` stayed unchanged while internal reader state moved to `detail::host_file_path`.

**Rationale:** Phase 13 needed Windows-correct host-path handling without expanding or changing the public API surface.
**Source:** 13-03-SUMMARY.md

---

### Pass `host_file_path` Through Parser Seams
TES3, TES4, BA2 GNRL, and BA2 DX10 parser entry points consume `detail::host_file_path` rather than raw path text.

**Rationale:** Parser metadata opens must stay coupled to the same resolved path selected at open time, with original UTF-8 text remaining diagnostics-only.
**Source:** 13-03-SUMMARY.md

---

### Reuse Reader Open As Validation Setup
`validate_archive` now treats `archive_reader::open` as the single host-path setup path and removes its duplicate readability preflight.

**Rationale:** Unreadable paths should remain direct setup failures while readable malformed archives should remain report-level diagnostics.
**Source:** 13-04-SUMMARY.md

---

### Keep Phase 13 Regression Proof Black-Box
The final regression suite proves behavior through `archive_reader::open`, `validate_archive`, and extraction APIs rather than white-box hooks.

**Rationale:** The phase goal was consumer-visible non-ASCII Windows host-path correctness, so evidence needed to come from the unchanged public API.
**Source:** 13-05-SUMMARY.md

---

### Reconstruct BA2 DX10 Expected Bytes From Manifests
BA2 DX10 canonical extraction expectations are reconstructed from manifest-backed DDS layout metadata plus committed payload bytes.

**Rationale:** The suite stays deterministic and black-box without copying expected payload trees into the temporary non-ASCII test root.
**Source:** 13-05-SUMMARY.md

---

## Lessons

### Rename Migrations Can Break Whole-Target Linking Mid-Plan
Renaming a helper before all active callers are migrated can leave unresolved legacy symbols because `libbsa_tests` links the whole writer stack.

**Context:** Plan 13-01 needed a short-lived bridge between Task 1 and Task 2, then removed it in Task 3.
**Source:** 13-01-SUMMARY.md

---

### Generated Build Files Can Preserve Deleted Source References
The existing build directory briefly kept a stale generated reference to `src/detail/host_path.cpp`.

**Context:** Rerunning `cmake --preset windows-msvc-debug-static` refreshed project files before execution continued.
**Source:** 13-01-SUMMARY.md

---

### Native Windows Path Construction Matters In Tests
Non-ASCII temp paths must be constructed with filesystem-native components before converting back to explicit UTF-8 at the public API boundary.

**Context:** The first smoke setup used narrow UTF-8 strings for temp-root construction and threw on Windows before the public API proof could run.
**Source:** 13-05-SUMMARY.md

---

### Static Representative Matrices Avoid Dangling References
Representative archive case storage should outlive test case access.

**Context:** Returning a temporary container and taking `.front()` created a dangling reference risk; the matrix was promoted to a static `std::array` returned by reference.
**Source:** 13-05-SUMMARY.md

---

### Stay Within The Project's C++20 Contract
Test code cannot use `std::string::contains` while the project is locked to C++20.

**Context:** The first RED pass in Plan 13-05 used `std::string::contains`; it was corrected to `find(...) != npos`.
**Source:** 13-05-SUMMARY.md

---

### Use `result<T>::value()` Explicitly
The local `result<T>` type should not be treated as if it exposes `operator->`.

**Context:** The first GREEN pass in Plan 13-05 used `result<T>` incorrectly and was corrected to use `.value()` before rerunning the representative matrix.
**Source:** 13-05-SUMMARY.md

---

### `original_utf8` Needs Follow-Up Scrutiny
`original_utf8` is documented as diagnostics-only state, but verification found current helper overloads only forward `resolved` and no current diagnostic uses `original_utf8`.

**Context:** Verification marked this as a warning-level dead-state/comment-drift concern that does not invalidate the Phase 13 open/validate goal.
**Source:** 13-VERIFICATION.md

---

### Writer-Side Non-ASCII Disk Sources Remain Future Scope
Some writer finalize/dedupe paths still reopen raw `host_path` values with `std::ifstream`.

**Context:** Verification explicitly classified this as a real future-scope issue that does not block Phase 13 because the phase contract covered read/open/validate flows.
**Source:** 13-VERIFICATION.md

---

## Patterns

### Shared `host_file` Helper Seam
Use one internal helper family for host-file open, inspect, prefix read, exact read, and chunk iteration.

**When to use:** Use when reader, parser, writer-prep, and validation-adjacent code need consistent bounded file I/O behavior and caller-owned diagnostics.
**Source:** 13-01-SUMMARY.md

---

### Resolve-Once Host Path Contract
Resolve public UTF-8 host-path text once, store both original and resolved forms, and perform later I/O only through the resolved path.

**When to use:** Use at public or internal host-file boundaries where repeated conversion could create inconsistent Windows path behavior.
**Source:** 13-02-SUMMARY.md

---

### Open-Time Stored Reader Path
Store `detail::host_file_path` in `archive_reader` state and pass it through detection, parser, and extraction dispatch.

**When to use:** Use when follow-on operations after a successful open must continue using the same resolved host-file boundary.
**Source:** 13-03-SUMMARY.md

---

### Parser Entry Seams On Resolved Host Paths
Parser archive-file entry declarations accept `detail::host_file_path` and implementations open streams through `detail::open_host_file`.

**When to use:** Use when parser metadata reads still need file access but must not reinterpret raw caller path text.
**Source:** 13-03-SUMMARY.md

---

### Validation Setup Through Public Reader Open
Let `validate_archive` use `archive_reader::open` as the single setup path, then keep extractability checks on the public extraction path.

**When to use:** Use when validation should preserve direct setup failures separately from diagnostics for readable malformed archives.
**Source:** 13-04-SUMMARY.md

---

### Source-Policy Seam Guards
Use source-policy tests to lock internal boundary migrations without widening the public host-path API.

**When to use:** Use when a migration must prevent regression to raw narrow-string opens while public API tests cannot observe every internal seam directly.
**Source:** 13-04-SUMMARY.md

---

### Manifest-Backed Canonical Extraction Proof
Use committed fixture manifests and canonical entries to compute expected bytes for black-box extraction assertions.

**When to use:** Use for deterministic archive regression suites where copying expected payload trees would duplicate data and invite drift.
**Source:** 13-05-SUMMARY.md

---

### Phase-Scoped Smoke Policy Gate
Add smoke policy checks that keep a dedicated regression suite limited to the phase-owned public API proof.

**When to use:** Use when a phase-close suite must avoid drifting into adjacent scopes such as writer-side coverage, TES3 requirements, or later verification-lane work.
**Source:** 13-05-SUMMARY.md

---

## Surprises

### Helper Rename Needed A Temporary Bridge
Removing old helper sources before all callers moved left unresolved legacy symbols in the full test target.

**Impact:** Plan 13-01 needed an auto-fixed temporary bridge, then had to delete it in the same plan to avoid leaving compatibility code behind.
**Source:** 13-01-SUMMARY.md

---

### CMake Project Files Held A Stale Source Reference
The build directory briefly referenced a stale generated source path after helper file changes.

**Impact:** Execution needed a configure refresh before continuing verification.
**Source:** 13-01-SUMMARY.md

---

### Narrow UTF-8 Temp Path Construction Failed On Windows
The first non-ASCII temp-root setup threw before the API proof could run.

**Impact:** The regression suite had to create directories and filenames with native `std::filesystem::path` components, then convert only for public UTF-8 API calls.
**Source:** 13-05-SUMMARY.md

---

### Representative Case Storage Had A Dangling Reference Risk
The initial representative-case helper returned a temporary container and then accessed `.front()` by reference.

**Impact:** The suite promoted the representative matrix to stable static storage before relying on it for smoke setup and full matrix checks.
**Source:** 13-05-SUMMARY.md

---

### C++20 Test Seed Accidentally Used C++23 Syntax
The first Plan 13-05 RED pass used `std::string::contains` even though the project targets C++20.

**Impact:** The test seed had to be corrected before the TDD cycle could proceed.
**Source:** 13-05-SUMMARY.md

---

### Verification Found Real But Non-Blocking Scope Debt
Verification identified both unused diagnostics-only `original_utf8` state and remaining raw writer-side reopen paths.

**Impact:** These became future cleanup concerns rather than blockers because the locked Phase 13 goal was read/open/validate and post-open extraction correctness.
**Source:** 13-VERIFICATION.md
