# Feature Categories: v1.1 Hardening

**Project:** libbsa
**Milestone:** v1.1 Hardening
**Scope:** Reliability and maintainability improvements for an existing Windows-only C++20 archive library
**Researched:** 2026-05-12
**Confidence:** HIGH

## Milestone Framing

This milestone should be planned as an **internal-quality milestone**, not a surface-area milestone. The library already ships reader, writer, validation, and extraction APIs for the supported archive families. v1.1 should therefore treat new work as reliability features that improve correctness, maintainability, and verification depth without expanding the public product contract.

The right requirement split is:

1. **Must-have hardening work** that closes concrete correctness or reliability gaps already identified in the codebase.
2. **Should-have cleanup** that meaningfully reduces regression risk or maintenance cost but is not itself a user-visible correctness blocker.
3. **Future hardening** that is valuable but should not inflate this milestone unless capacity remains after the must-have items are complete.

## Must-Have Hardening Categories

These are the table-stakes categories for this milestone. Missing any of them would leave the milestone under-scoped relative to its stated goal.

| Category | Why It Is Table Stakes For v1.1 | Complexity | Testability | Dependency Hints |
|---------|----------------------------------|------------|-------------|------------------|
| Host-path correctness on Windows | This is the clearest shipped correctness gap: archive open/validate flows can fail on non-ASCII host paths. That is a real reliability bug, not a refactor nice-to-have. | MEDIUM | High: path-based integration tests can prove open/validate success on wide-character paths. | Should happen early because verification and regression tests depend on the fixed path-opening path. |
| Reader-path dispatch hardening | Repeated archive-family dispatch in `archive_reader` creates drift risk across listing, lookup, extraction, and bulk extraction. This is a hotspot for future regressions. | MEDIUM | High: existing reader behavior can be regression-checked against unchanged public APIs. | Best after host-path fixes or in parallel if refactor preserves the same open path semantics. |
| Parser/preparer modularization at fragile hotspots | Large translation units currently mix offset math, overflow checks, format rules, and error translation. Hardening without reducing blast radius will not stick. | HIGH | Medium-High: helper extraction should be backed by focused unit tests around the moved logic. | Depends on identifying the highest-risk files first; should stay targeted, not become a broad rewrite. |
| Hardening verification lane reconciliation | The repo currently has policy drift around sanitizer support and gaps in supported build/test lane claims. A hardening milestone must make supported verification lanes explicit and real. | MEDIUM | High: presets, CI, and policy tests can prove what is supported. | Should be planned alongside implementation so new fixes land with live verification, not after. |
| Stronger default or opt-in verification coverage | Release-mode and sanitizer-oriented coverage are currently weaker than the milestone intent implies. A hardening milestone needs at least one stronger verification lane added or restored. | MEDIUM | High: CI/preset coverage is directly testable. | Depends on the reconciliation work above because the project first needs a truthful support policy. |
| Dedupe-path cost and fragility reduction | TES4 and BA2 GNRL dedupe paths are known expensive/fragile hotspots. This is in-scope because it improves writer reliability and reduces risky late-stage behavior. | HIGH | Medium: unit/fixture tests can prove correctness; perf-oriented assertions should stay coarse. | Prefer targeted internal indexing/caching improvements that preserve current dedupe semantics. |
| BA2 DX10 temp-staging lifecycle cleanup | Temp snapshot handling can leave decoded texture data behind on abnormal termination and adds fragile writer state. This is an explicit milestone concern and a real hardening issue. | HIGH | Medium: lifecycle tests can prove normal cleanup; abnormal termination risk should be documented if not fully eliminated. | Likely depends on preparer modularization because staging behavior currently lives in a large hotspot file. |

## Should-Have Categories

These are good milestone requirements if capacity allows, but they should not displace the must-have categories above.

| Category | Why It Matters | Complexity | Notes |
|---------|----------------|------------|-------|
| Planning/codebase policy alignment | Cleanly align `.planning/` claims with actual presets, CI lanes, and supported workflows. | LOW-MEDIUM | Important for maintainers, but only valuable if paired with real verification decisions. |
| Focused regression-test expansion around extracted helpers | Each refactor hotspot should gain narrow tests so future edits do not re-open the same class of bug. | MEDIUM | This should accompany must-have refactors, but can be tracked as its own requirement category. |
| Documentation of remaining temp-data persistence risk | If crash-proof cleanup is not fully achievable in v1.1, the remaining caller-visible risk should be documented honestly. | LOW | Useful if the implementation stops short of fully removing disk-backed snapshots. |
| Real-corpus verification workflow tightening | Improve the way optional local compatibility checks are used before shipping sensitive parser/writer changes. | MEDIUM | Valuable, but not as important as fixing default supported lanes first. |

## Explicit Non-Goals For v1.1

These should be categorized as anti-features for this milestone to prevent scope drift.

| Non-Goal | Why It Should Stay Out |
|---------|-------------------------|
| New archive family support | v1.1 is about reliability of already-supported families, not feature expansion. |
| New public API product surfaces | The milestone intent is internal hardening; avoid inventing new consumer-facing workflows unless required for correctness. |
| Full writer redesign | The codebase needs targeted refactors, not a new writer architecture. |
| Broad performance program | Only dedupe/staging work tied to concrete fragile hotspots is in scope. General benchmarking or optimization campaigns belong later. |
| Fuzzing program as a major new subsystem | Valuable later, but too large if it delays concrete bug fixes and verification-lane cleanup now. |
| In-place archive mutation or new packaging workflows | Not part of the hardening goal and likely to destabilize the milestone. |

## Requirement-Ready Feature Breakdown

The milestone can be translated into requirement-ready feature buckets like this.

### 1. Host Path Reliability

**Goal:** Ensure archive open and validation flows behave correctly for Windows host paths that require wide-character filesystem handling.

**Include:**
- open-path handling through `std::filesystem::path`-safe boundaries
- validation-path handling through the same corrected path-opening route
- regression coverage for non-ASCII path cases

**Exclude:**
- changes to archive-internal virtual path normalization semantics
- general path API redesign

**Complexity:** Medium

**Depends on:** none; good first requirement slice

### 2. Reader Dispatch Stability

**Goal:** Remove repeated archive-family branching from public reader operations so open-time dispatch happens once and behavior stays consistent across entry points.

**Include:**
- internal strategy/vtable-style dispatch in reader state
- shared handling across list, lookup, contains, extract, and bulk extract
- regression checks that public behavior is unchanged

**Exclude:**
- new reader capabilities
- public polymorphism or ABI-facing type changes

**Complexity:** Medium

**Depends on:** ideally after or alongside host path reliability

### 3. Fragile Parser/Preparer Refactor

**Goal:** Split the highest-risk parser and preparer hotspots into smaller internal helpers with narrower responsibilities and focused tests.

**Include:**
- offset math/helper extraction
- validation/bounds helper extraction
- payload-routing or chunk-planning helper extraction where files are currently monolithic

**Exclude:**
- broad architecture cleanup across unrelated files
- format behavior changes unless required to fix a verified bug

**Complexity:** High

**Depends on:** stable regression tests; should be sequenced file-by-file

### 4. Verification Policy Reconciliation

**Goal:** Make planning claims, preset support, CI behavior, and policy tests agree on what hardening coverage actually exists.

**Include:**
- supported preset inventory cleanup
- policy test updates
- `.planning/` wording updates where current claims are inaccurate

**Exclude:**
- aspirational verification claims without live automation

**Complexity:** Medium

**Depends on:** none, but should be resolved before final milestone signoff

### 5. Stronger Hardening Lanes

**Goal:** Add or restore at least one meaningful stronger verification lane beyond the current default debug-centric path.

**Preferred acceptable outcomes:**
- sanitizer-oriented preset/workflow restored and supported, or
- Release-mode build/test lane added to checked-in presets/CI, ideally with a hardening-focused companion lane

**Include:**
- lane implementation
- documentation/policy alignment
- at least one proof path run in automation

**Exclude:**
- large fuzzing infrastructure unless it stays narrowly scoped

**Complexity:** Medium

**Depends on:** verification policy reconciliation

### 6. Dedupe Hotspot Cleanup

**Goal:** Reduce fragile or excessively expensive payload dedupe paths without changing dedupe correctness rules.

**Include:**
- indexed/hash-first candidate narrowing for TES4 payload dedupe
- better staged identity or digest reuse for BA2 GNRL dedupe
- unchanged byte-accurate equality fallback semantics

**Exclude:**
- changing dedupe defaults or public behavior policy
- approximate dedupe

**Complexity:** High

**Depends on:** good fixture coverage for writer correctness

### 7. BA2 DX10 Temp-Staging Cleanup

**Goal:** Reduce temp-file lifecycle risk and staging fragility in BA2 DX10 preparation/finalization.

**Include:**
- tighter snapshot ownership/lifetime control
- fewer or shorter-lived temp artifacts where feasible
- clear cleanup checkpoints and regression coverage for normal lifecycle cleanup

**Exclude:**
- a total redesign of texture writing unless narrowly justified
- new public staging configuration surface unless required

**Complexity:** High

**Depends on:** parser/preparer refactor work if the relevant logic is too entangled to harden safely in place

## Suggested Prioritization

### P1 — Must Ship

1. Host Path Reliability
2. Verification Policy Reconciliation
3. Stronger Hardening Lanes
4. Reader Dispatch Stability

These provide the clearest milestone value: fix the real correctness bug, make the hardening story truthful, add stronger proof, and reduce a central regression hotspot.

### P2 — Strongly Recommended

5. Fragile Parser/Preparer Refactor
6. BA2 DX10 Temp-Staging Cleanup
7. Dedupe Hotspot Cleanup

These are core to making the library safer to maintain, but they should stay tightly scoped to the named hotspots.

### P3 — Only If Capacity Remains

8. Real-corpus workflow tightening
9. Extra documentation/reporting polish around remaining risks

## Dependency Hints

```text
Host Path Reliability
    -> enables trustworthy regression coverage for open/validate path bugs

Verification Policy Reconciliation
    -> enables Stronger Hardening Lanes

Reader Dispatch Stability
    -> reduces public-reader drift risk before deeper parser refactors

Fragile Parser/Preparer Refactor
    -> makes BA2 DX10 Temp-Staging Cleanup safer to implement
    -> makes hotspot-specific tests easier to add

Dedupe Hotspot Cleanup
    -> should preserve current writer semantics
    -> depends on existing writer fixture/regression coverage

BA2 DX10 Temp-Staging Cleanup
    -> should be validated after helper extraction if current files are too monolithic
```

## Milestone Acceptance Shape

For planning purposes, each category should be considered complete only when it has:

- a scoped code change tied to a named concern,
- targeted regression coverage,
- no public-surface expansion unless strictly required,
- and updated planning/policy docs where verification behavior changed.

## Recommendation

Treat **Host Path Reliability**, **Verification Policy Reconciliation + Stronger Hardening Lanes**, **Reader Dispatch Stability**, **Fragile Parser/Preparer Refactor**, **Dedupe Hotspot Cleanup**, and **BA2 DX10 Temp-Staging Cleanup** as the main feature categories for v1.1.

That set is specific, testable, and faithful to the milestone goal: improve reliability of the shipped library rather than add new end-user features.

## Sources

- `J:\libbsa\.planning\PROJECT.md` — milestone goal, active requirements, constraints.
- `J:\libbsa\.planning\codebase\CONCERNS.md` — concrete bugs, fragile areas, performance bottlenecks, and verification gaps.
