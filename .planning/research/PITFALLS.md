# Domain Pitfalls

**Domain:** v1.1 hardening for an existing Windows-only C++20 Bethesda archive library  
**Researched:** 2026-05-12  
**Confidence:** HIGH

## Hardening Goal

This milestone is not feature expansion. The risk is breaking a working v1.0 library while fixing host-path correctness, verification-lane drift, internal structure, dedupe cost, and BA2 DX10 temp-data hygiene. The main roadmap mistake would be treating these as independent cleanup tasks; they overlap in parser behavior, staging behavior, and test expectations.

## Critical Pitfalls

### Pitfall 1: Fixing Windows host-path Unicode handling in only one entry point

**What goes wrong:**
`archive_reader::open`, validation, and format-specific parser opens stop agreeing about what a “path” is. One codepath uses `std::filesystem::path`, another still uses narrow `std::ifstream`, and tests only cover the top-level API.

**Why it happens:**
The bug is host-path-specific, but path opens are currently spread across `src/archive.cpp`, `src/validation.cpp`, and multiple format readers/parsers.

**Consequences:**
- Non-ASCII archive paths open successfully in one API and fail in another.
- Validation can disagree with `archive_reader::open`, violating the existing “validation reuses strict parser” design.
- Later refactors accidentally reintroduce narrow-path regressions.

**Warning signs:**
- Fixes touch only one file in the open/validate stack.
- New helpers return `std::string` paths after initially receiving `std::filesystem::path`.
- Tests cover ASCII and one direct open path, but not `validate_archive` and every archive family.

**Prevention:**
- Introduce one internal host-file open helper that accepts `std::filesystem::path` and is used by every archive open/validate/parser entry.
- Keep archive-internal virtual paths unchanged; only host-path boundaries should change.
- Add committed tests that exercise non-ASCII host paths through open, list, lookup, and validation across at least one TES4 BSA and one BA2 family.

**Detection:**
- `archive_reader::open(path)` passes while `validate_archive(path)` fails on the same file.
- Failures reproduce only when the temp directory or fixture root contains non-ASCII characters.

**Absorb in phase:**
Phase 1 — **Windows host-path correctness**. Do this before refactors so later internal cleanup happens on the correct I/O boundary.

---

### Pitfall 2: Collapsing host-path fixes into archive-path normalization logic

**What goes wrong:**
The hardening work “fixes paths” by pushing `std::filesystem::path` deeper into archive entry lookup, hashing, or extraction-key logic.

**Why it happens:**
Host paths and archive member paths are both called “paths,” but they are different domains. The known bug is about Windows filesystem opens, not Bethesda virtual path semantics.

**Consequences:**
- Hash lookups drift.
- Case/separator behavior changes for archive members.
- A host-path bug fix creates a compatibility regression in archive internals.

**Warning signs:**
- Hash helpers start accepting `std::filesystem::path`.
- Reader lookup logic changes in the same patch as host-file open changes.
- Tests for lookup/contains start being rewritten instead of only expanded.

**Prevention:**
- Explicitly split “host path” and “archive virtual path” helpers.
- Scope v1.1 path changes to disk I/O boundaries only unless a separate compatibility bug proves otherwise.
- Add regression tests proving identical archive-member lookup behavior before and after the host-path fix.

**Detection:**
- ASCII fixture opens improve, but previously passing lookup/hash fixtures start failing.

**Absorb in phase:**
Phase 1 — **Windows host-path correctness**, with a hard no-scope-creep rule.

---

### Pitfall 3: Adding sanitizer or Release lanes without first reconciling policy drift

**What goes wrong:**
The project adds a new preset or CI job, but planning docs, preset names, policy tests, and fixture docs still disagree about what is officially supported.

**Why it happens:**
The current audit already says `.planning/PROJECT.md` claims sanitizer-oriented presets while checked-in tests enforce their absence.

**Consequences:**
- CI goes red for policy reasons unrelated to code correctness.
- Maintainers stop trusting `.planning/` and test-policy suites.
- Hardening lanes exist but are treated as experimental and silently rot.

**Warning signs:**
- `CMakePresets.json` changes land without matching updates to policy tests and docs.
- CI introduces ad hoc command lines rather than preset-backed lanes.
- “Temporary” exceptions are added to validation-policy tests.

**Prevention:**
- Decide first whether v1.1 officially supports: (a) Release lane only, (b) Release + opt-in sanitizer preset, or (c) dedicated hardening workflow.
- Update `.planning/PROJECT.md`, `CMakePresets.json`, `tests/unit/validation_policy_tests.cpp`, and fixture docs in the same phase.
- Treat presets and policy text as product contracts, not incidental tooling.

**Detection:**
- CI passes locally but policy tests fail on required-token checks.
- Contributors cannot answer which preset is authoritative for hardening.

**Absorb in phase:**
Phase 2 — **verification-lane reconciliation**. Do this before parser/refactor work so every later change has an agreed validation target.

---

### Pitfall 4: Turning sanitizer work into a false sense of coverage

**What goes wrong:**
One sanitizer lane is added, but it does not exercise the fragile parser/preparer paths, optional corpus checks, or Release-only behavior. The milestone claims “hardening complete” anyway.

**Why it happens:**
It is easy to add instrumentation, hard to ensure it actually runs the right tests. This codebase’s strongest real-corpus compatibility checks are opt-in and usually skipped.

**Consequences:**
- Memory-safety bugs in malformed parsing remain undiscovered.
- Optimization-sensitive regressions survive because only Debug+sanitizer is exercised.
- Roadmap decisions over-trust the new lane.

**Warning signs:**
- Sanitizer lanes run only tiny unit tests.
- Release behavior is still absent from the checked-in matrix.
- Local game fixture tests stay entirely outside pre-ship verification.

**Prevention:**
- Require at least one Release preset in the official matrix.
- Run malformed-fixture suites and core reader/writer round trips under the hardening lane.
- Keep local corpus checks opt-in, but add a milestone exit step requiring them before release when parser/writer internals changed.

**Detection:**
- New lanes pass, but bugs still reproduce only under optimized builds or real corpus checks.

**Absorb in phase:**
Phase 2 — **verification-lane reconciliation**, plus a Phase 6 ship gate for optional corpus reruns.

---

### Pitfall 5: Refactoring large parser/preparer files before locking behavior with focused tests

**What goes wrong:**
Large translation units are split for cleanliness, but offset arithmetic, overflow checks, compatibility warnings, or exception-to-error translation change accidentally.

**Why it happens:**
The concern is structural, but the files named in the audit also own correctness-critical format logic.

**Consequences:**
- “No functional change” refactors introduce parser drift.
- Round trips still pass while malformed-input behavior or warning codes change.
- Review becomes impossible because mechanical moves and semantic edits are mixed.

**Warning signs:**
- A refactor PR both extracts helpers and changes arithmetic/validation rules.
- Existing tests are updated broadly instead of adding narrow new ones first.
- Files like `ba2_dx10_prepare.cpp` or `tes4_bsa_parser.cpp` shrink dramatically in one step.

**Prevention:**
- Before each extraction, add characterization tests for the exact helper boundary being split out.
- Move one responsibility at a time: bounds checks, offset math, chunk planning, warning translation, payload routing.
- Keep semantic changes out of the first refactor pass.

**Detection:**
- Fixture outputs or stable error codes change after a “mechanical” refactor.
- Diff review cannot separate file movement from logic edits.

**Absorb in phase:**
Phase 3 — **reader/parser/preparer refactors**, only after Phases 1-2 stabilize path and verification baselines.

---

### Pitfall 6: Replacing repeated public-reader dispatch with a strategy layer that changes semantics

**What goes wrong:**
An internal vtable/strategy object removes duplicated branching, but bulk extraction, single-entry extraction, `contains`, and `lookup` no longer behave identically across archive families.

**Why it happens:**
The dispatch duplication is real, but the duplicated code currently hides subtle per-operation behavior that can be lost during consolidation.

**Consequences:**
- Single-entry and bulk-entry paths drift.
- Some archive families lose operation-specific guard behavior.
- Public API stays stable while implementation semantics change underneath.

**Warning signs:**
- Dispatch refactor is validated only by compile success and a few smoke tests.
- Tests cover open/list, but not the full matrix of lookup/contains/extract/extract_entries.
- Strategy objects gain stateful caching without clear invalidation rules.

**Prevention:**
- Build a shared behavior matrix test file before the dispatch refactor.
- Keep strategy objects thin and immutable after open.
- Require identical results between single-entry and bulk-entry paths for the same target fixture set.

**Detection:**
- `contains()` and `lookup()` disagree for the same path after the refactor.
- Bulk extraction passes while direct extraction fails, or vice versa.

**Absorb in phase:**
Phase 3 — **reader/parser/preparer refactors**.

---

### Pitfall 7: “Optimizing” dedupe by weakening the exact-byte correctness rule

**What goes wrong:**
TES4 or BA2 dedupe adds a hash-first index, cached digest, or identity shortcut that stops proving payload equality on final stored bytes.

**Why it happens:**
The current dedupe paths are expensive. The temptation is to trust a faster key completely instead of using it as a prefilter.

**Consequences:**
- Different payloads share offsets incorrectly.
- Compression differences or embedded-name differences collapse to one stored payload.
- Corruption appears only on certain archive mixes, making it hard to triage.

**Warning signs:**
- The implementation stops calling full equality for collisions.
- Dedupe keys are based on source bytes instead of final stored bytes.
- BA2 disk-backed sources are assumed identical because size and hash match once.

**Prevention:**
- Preserve the existing rule: optimization may narrow candidates, never replace exact stored-byte comparison.
- Add adversarial tests where equal size/hash buckets still differ in payload bytes, compression choice, or embedded-name prefixes.
- Benchmark only after correctness fixtures pass.

**Detection:**
- Archive size shrinks unexpectedly on mixed payload fixtures.
- Reopened archives contain wrong bytes for one of two near-duplicate entries.

**Absorb in phase:**
Phase 4 — **dedupe optimization**.

---

### Pitfall 8: Speeding up BA2 GNRL dedupe by caching stale disk-source state

**What goes wrong:**
To reduce repeated file scans, the code caches disk-backed identity or digest data that becomes stale when source files change between preparation and finalization.

**Why it happens:**
The concern audit explicitly notes that BA2 GNRL correctness depends on validating prepared source sizes because disk sources can change between preparation and streaming.

**Consequences:**
- Finalization publishes malformed offsets or wrong payload bytes.
- Dedupe compares a stale view of disk content.
- v1.0’s explicit growth/truncation protections are bypassed.

**Warning signs:**
- Cached digests are treated as permanent truth.
- Finalization size rechecks are removed “for performance.”
- Benchmarks improve, but mutation-between-prepare-and-finalize tests disappear.

**Prevention:**
- Keep source-size and source-identity validation at finalization.
- Cache only as a candidate filter; revalidate before publish.
- Add regression tests for file growth, truncation, and same-size content replacement.

**Detection:**
- Dedupe optimization patches also touch publish/finalization guard code.

**Absorb in phase:**
Phase 4 — **dedupe optimization**.

---

### Pitfall 9: Cleaning up BA2 DX10 temp data only on the happy path

**What goes wrong:**
The writer deletes snapshot files during normal completion, but exceptions, early returns, cancellation, or process termination still leave decoded DDS bytes behind.

**Why it happens:**
Current cleanup is best-effort in writer state destruction. Hardening work can improve normal lifecycle handling while still missing failure paths.

**Consequences:**
- Sensitive or large temp data accumulates under the system temp root.
- Disk usage spikes on failed runs.
- The project claims cleanup hardening without materially reducing caller risk.

**Warning signs:**
- Cleanup logic is added only after successful publish.
- Tests assert temp cleanup after success but not after prepare failure or publish failure.
- Design discussion assumes destructor cleanup is enough for all cases.

**Prevention:**
- Define explicit lifecycle checkpoints: after analysis, after snapshot creation, after chunk planning, after publish, and after failure rollback.
- Make “best effort after crash/forced termination may still leak temp data” an explicit documented residual risk unless the architecture truly removes temp files.
- Add tests for normal failure paths and repeated writer construction/destruction.

**Detection:**
- Temp directories remain after failed DX10 writer tests.
- Cleanup behavior differs between success and publish-failure scenarios.

**Absorb in phase:**
Phase 5 — **BA2 DX10 temp-data cleanup**.

---

### Pitfall 10: Combining temp-data cleanup with staging redesign in one step

**What goes wrong:**
The milestone tries to solve cleanup, memory pressure, and I/O amplification at once by redesigning BA2 DX10 staging, making it impossible to tell whether regressions come from lifecycle fixes or data-flow changes.

**Why it happens:**
The current temp-file design is both a correctness concern and a performance concern, so it attracts over-scoped fixes.

**Consequences:**
- Milestone slips into architecture work instead of hardening.
- DX10 write behavior changes without enough fixture proof.
- Cleanup bugs and chunk-planning bugs get entangled.

**Warning signs:**
- The phase introduces new in-memory staging policies, new chunk planners, and cleanup semantics together.
- No intermediate checkpoint keeps current staging behavior while tightening cleanup only.

**Prevention:**
- Phase 5 should harden lifecycle and cleanup first.
- Any deeper staging redesign belongs in a later milestone unless a minimal targeted change is sufficient and fully testable.
- Keep chunk layout behavior byte-for-byte stable during cleanup work.

**Detection:**
- DX10 fixture outputs change in a cleanup-only phase.

**Absorb in phase:**
Phase 5 now; deeper staging redesign deferred to a future performance milestone.

## Moderate Pitfalls

### Pitfall 1: Letting hardening refactors leak into public API surface

**What goes wrong:** Internal strategy/refactor work changes public types, error semantics, or header dependencies without a product need.

**Prevention:** Treat v1.1 as internal-facing. Keep public API changes out unless they are required for correctness and explicitly documented.

**Absorb in phase:** Phase 3.

### Pitfall 2: Using benchmark wins as a substitute for compatibility proof

**What goes wrong:** Dedupe or staging gets faster, but malformed behavior, offset math, or reopened-bytes correctness regresses.

**Prevention:** Require fixture and round-trip proof before benchmark comparison. Benchmarks are exit evidence, not design truth.

**Absorb in phase:** Phase 4 and any later performance milestone.

### Pitfall 3: Assuming default CI coverage is enough after internal changes

**What goes wrong:** Default committed fixtures pass, but optional local game-corpus compatibility drifts.

**Prevention:** Add a pre-ship manual gate for opt-in corpus checks whenever parser, writer, dedupe, or DX10 staging internals change.

**Absorb in phase:** Phase 6 ship gate.

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| Phase 1: Windows host-path correctness | Mixing host-path fixes with archive-path semantics | Limit changes to disk I/O boundaries; add non-ASCII open + validate coverage |
| Phase 2: verification-lane reconciliation | Adding presets/CI without updating policy tests and docs | Change presets, docs, and policy tests together |
| Phase 3: internal refactors | Semantic drift hidden inside “cleanup” diffs | Add characterization tests first; separate moves from behavior changes |
| Phase 4: dedupe optimization | Replacing exact equality with hash trust | Use hashes only as candidate filters; preserve final stored-byte equality |
| Phase 5: BA2 DX10 temp cleanup | Fixing only happy-path cleanup or over-scoping into staging redesign | Harden lifecycle first; defer architectural staging changes |
| Phase 6: milestone verification/ship | Trusting default CI alone | Rerun Release lane, malformed suites, and opt-in compatibility checks before release |

## Recommended Roadmap Sequence

1. **Phase 1 — Windows host-path correctness**
   - Fix the known user-visible correctness bug first.
   - Locks the right file-open boundary before refactors spread it further.

2. **Phase 2 — Verification-lane reconciliation**
   - Decide and codify the supported Release/sanitizer story.
   - Prevents later hardening work from landing without an agreed validation target.

3. **Phase 3 — Reader/parser/preparer structural refactors**
   - Only after paths and verification baselines are stable.
   - Split monoliths and unify dispatch with characterization tests already in place.

4. **Phase 4 — Dedupe optimization under existing correctness rules**
   - Performance work comes after structure is safer to change.
   - Keep exact stored-byte proof and finalization guards intact.

5. **Phase 5 — BA2 DX10 temp-data lifecycle hardening**
   - Tighten cleanup and caller-visible risk boundaries without redesigning the whole staging model.

6. **Phase 6 — Milestone verification and release gate**
   - Run official Release lane, malformed suites, package/export smoke, and opt-in corpus checks for touched families.
   - Confirm no path, parser, dedupe, or DX10 staging regressions escaped.

## Why This Order

- **Correctness boundary before cleanup:** host-path fixes are the only known shipped bug and affect every format family.
- **Verification before structural edits:** without settled presets and policy, later “hardening” changes produce noisy false failures and weak confidence.
- **Structure before optimization:** refactors reduce review risk in the exact files that dedupe and DX10 staging work must touch next.
- **Optimization before ship gate:** performance/temp cleanup changes are where subtle regressions hide, so they should be last and followed immediately by broader verification.

## Milestone Exit Criteria

- Non-ASCII host-path tests pass through both open and validation flows.
- Supported preset/CI policy is internally consistent across `.planning/`, `CMakePresets.json`, docs, and policy tests.
- Refactor phases show no stable error-code, warning-code, or fixture-behavior drift unless explicitly intended.
- Dedupe optimizations preserve exact stored-byte correctness and finalization revalidation.
- BA2 DX10 failure-path cleanup is tested and documented with honest residual-risk wording.
- Release lane and opt-in compatibility checks are rerun before milestone close.

## Sources

- `.planning/PROJECT.md`
- `.planning/codebase/CONCERNS.md`
- `.planning/codebase/TESTING.md`
