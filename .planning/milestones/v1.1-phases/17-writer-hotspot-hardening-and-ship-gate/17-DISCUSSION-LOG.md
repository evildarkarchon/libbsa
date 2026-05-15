# Phase 17: writer-hotspot-hardening-and-ship-gate - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-14
**Phase:** 17-writer-hotspot-hardening-and-ship-gate
**Areas discussed:** DX10 writer reuse, Dedupe proof style, Failure cleanup cases, Ship gate evidence

---

## DX10 writer reuse

### After BA2 DX10 snapshot cleanup runs, how should the writer object behave?

| Option | Description | Selected |
|--------|-------------|----------|
| Consumed after write | Treat BA2 DX10 write_to as terminal after success or ordinary failure; cleanup is reliable and retry semantics are explicit. | yes |
| Preserve retries | Keep or rebuild enough staged data to allow another write_to after cleanup; this is heavier and risks reintroducing temp or memory pressure. | |
| You decide | Let the implementation pick the smallest safe behavior consistent with SPEC.md and existing docs. | |

**User's choice:** Consumed after write
**Notes:** This locks cleanup over retryability for BA2 DX10.

### Once a BA2 DX10 writer is consumed, what should later calls do?

| Option | Description | Selected |
|--------|-------------|----------|
| Return invalid_argument | add_file and write_to fail through result with a stable misuse-style message; no throwing and no silent partial reuse. | yes |
| Only write_to fails | write_to reports consumed state, but add_file still accepts entries; this is more surprising because old entries are unusable. | |
| You decide | Planner can choose the clearest result-based failure shape. | |

**User's choice:** Return invalid_argument
**Notes:** Later calls stay inside the existing `result` error model.

### Should this consumed-writer rule apply only to BA2 DX10, or should Phase 17 standardize it across all writer families?

| Option | Description | Selected |
|--------|-------------|----------|
| DX10 only | Only BA2 DX10 owns snapshot temp data; avoid changing TES3, TES4, or BA2 GNRL behavior outside this phase. | yes |
| All writers | Make every writer one-shot after write_to; broader behavior change and likely outside Phase 17 hardening scope. | |
| You decide | Let the implementation keep the narrowest behavior change that satisfies cleanup. | |

**User's choice:** DX10 only
**Notes:** Other writer families keep existing lifecycle behavior.

### Should BA2 DX10 become consumed after any ordinary write_to attempt, or only after attempts that actually created/read snapshot data?

| Option | Description | Selected |
|--------|-------------|----------|
| Any write attempt | Simple terminal rule: success or any result-returning failure cleans snapshots and prevents stale staged state reuse. | yes |
| Only after staging | Validation failures before snapshot use might leave the writer reusable, but callers get a more complex state model. | |
| You decide | Let the implementation define the least surprising terminal boundary after scouting call order. | |

**User's choice:** Any write attempt
**Notes:** The state model should be simple and terminal after `write_to` is attempted.

---

## Dedupe proof style

### What proof style should Phase 17 lock for dedupe narrowing?

| Option | Description | Selected |
|--------|-------------|----------|
| Runtime plus policy | Keep behavior tests for offsets/extraction and add policy evidence that candidate narrowing exists before exact equality. | yes |
| Runtime only | Prove outputs stay correct but do not guard the internal narrowing shape as strongly. | |
| Policy only | Lock source structure but risk missing behavior regressions in archive output. | |
| You decide | Let planning choose the smallest evidence mix that satisfies SPEC.md. | |

**User's choice:** Runtime plus policy
**Notes:** Behavior and structural guardrails are both required.

### How should faster candidate narrowing be proven without turning Phase 17 into a benchmark phase?

| Option | Description | Selected |
|--------|-------------|----------|
| Algorithmic evidence | Assert keyed/digest buckets bound exact comparisons; avoid timing gates that can be flaky on CI. | yes |
| Small benchmark | Add a tiny timing or operation-count benchmark; more proof, but risks flakiness or broad performance scope. | |
| No speed proof | Rely on code review and behavior tests only; weaker link to DEDU-01/DEDU-02 wording. | |
| You decide | Let downstream agents pick appropriate non-flaky evidence. | |

**User's choice:** Algorithmic evidence
**Notes:** No timing benchmark is required.

### Where should dedupe narrowing logic live?

| Option | Description | Selected |
|--------|-------------|----------|
| Format-local helpers | Keep TES4 and BA2 GNRL helpers near their layout code; avoids a broad generic dedupe framework. | yes |
| Shared detail helper | Create one internal helper if both formats genuinely share the same key/candidate mechanics. | |
| You decide | Prefer minimal code movement unless a shared helper clearly reduces risk. | |

**User's choice:** Format-local helpers
**Notes:** A generic dedupe framework is out of scope.

### For hash or digest collisions, what should the tests and policy emphasize?

| Option | Description | Selected |
|--------|-------------|----------|
| Fallback is mandatory | Lock that size/hash/digest only selects candidates; exact stored-byte equality must remain present before offset sharing. | yes |
| Collision fixture required | Require a constructed same-key/different-bytes case where feasible; stronger but may force test-only seams or brittle hashes. | |
| You decide | Let planning choose policy and runtime cases that prove fallback without test-only product code. | |

**User's choice:** Fallback is mandatory
**Notes:** The exact equality fallback is the locked correctness authority.

---

## Failure cleanup cases

### Which BA2 DX10 ordinary failure paths should be explicitly locked by tests?

| Option | Description | Selected |
|--------|-------------|----------|
| All named cases | Cover validation failure, missing/truncated snapshot pre-publish failure, and output/publish failure. | yes |
| Two required cases | Only cover one pre-publish snapshot failure and one output-path failure from SPEC.md. | |
| You decide | Let planning pick representative failures if coverage stays tied to DX10-01. | |

**User's choice:** All named cases
**Notes:** Cleanup proof should be broad across ordinary failure families.

### If snapshot cleanup itself fails during ordinary failure unwinding, what should the public result report?

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve primary error | Return the original validation/write/publish error; cleanup remains best-effort and documented. | yes |
| Report cleanup error | Surface cleanup failure instead, but this can hide the error that actually caused write_to to fail. | |
| You decide | Let implementation preserve the clearest result contract after checking current helpers. | |

**User's choice:** Preserve primary error
**Notes:** Cleanup errors do not mask the primary result.

### Should Phase 17 also clean up BA2 DX10 temp dirs created by failed add_file calls, if add_file reserved a snapshot directory before failing?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, if created | Tightens ordinary failure cleanup for the same temp lifecycle without expanding public capability. | yes |
| write_to only | Stay strictly on SPEC.md write_to wording; destructor still handles add_file failure leftovers. | |
| You decide | Let planning include this only if the existing call order makes it a small lifecycle fix. | |

**User's choice:** Yes, if created
**Notes:** Failed `add_file` cleanup is included when a temp directory was reserved.

### How should cleanup tests identify the temp artifacts to assert on?

| Option | Description | Selected |
|--------|-------------|----------|
| Track new writer dirs | Snapshot temp dirs before/after and assert only new writer-owned libbsa-dx10-snapshot-* dirs are removed. | yes |
| Assert temp root clean | Stronger-looking but unsafe because unrelated temp dirs from other runs could exist. | |
| You decide | Let implementation use the least flaky temp-directory assertion pattern. | |

**User's choice:** Track new writer dirs
**Notes:** Avoid flaky global temp-root assertions.

---

## Ship gate evidence

### What verification gate should close Phase 17 and v1.1?

| Option | Description | Selected |
|--------|-------------|----------|
| Focused combined gate | Run focused debug writer tests, focused ASan hardening tests, and Release package proof without requiring every lane for every test. | yes |
| Full matrix | Run all supported Debug, Release, and ASan lanes broadly; strongest but expensive for final phase closure. | |
| Debug only | Fastest, but weaker for risky writer/temp lifecycle changes and conflicts with Phase 14 hardening guidance. | |
| You decide | Let planning choose the final lane mix from Phase 14's supported matrix. | |

**User's choice:** Focused combined gate
**Notes:** Each lane has a distinct role in final evidence.

### Should optional local game-corpus or BSArchPro comparison tests be required for this ship gate?

| Option | Description | Selected |
|--------|-------------|----------|
| Not required | Keep the official gate runnable from committed assets; mention optional local corpus checks as advisory only. | yes |
| Require local corpus | Stronger compatibility confidence, but violates the Phase 14 opt-in fixture contract and may not run on all machines. | |
| You decide | Let planning keep local corpus optional unless a real fixture requirement appears. | |

**User's choice:** Not required
**Notes:** Official closure remains committed-assets-only.

### Where should BA2 DX10 temp lifecycle guarantees and abnormal-termination risk be documented?

| Option | Description | Selected |
|--------|-------------|----------|
| Public docs plus planning | Update user/maintainer docs and Phase 17 verification; downstream agents get a durable public-facing contract. | yes |
| Planning only | Satisfies phase evidence but leaves future consumers less likely to see the cleanup and abnormal-termination notes. | |
| Code comments only | Useful near implementation, but too hidden for the SPEC.md documentation requirement. | |
| You decide | Let planning pick the smallest durable documentation surface. | |

**User's choice:** Public docs plus planning
**Notes:** Documentation should be visible beyond phase artifacts.

### How should final planning-state updates be handled if Phase 17 passes?

| Option | Description | Selected |
|--------|-------------|----------|
| Same closure plan | The Phase 17 plan should include final consistent updates to REQUIREMENTS, PROJECT, ROADMAP, STATE, and verification artifacts. | yes |
| Separate closeout task | Implementation finishes first; a later task updates planning state, but milestone status can drift temporarily. | |
| You decide | Let planning decide how to sequence closure docs while keeping the final state consistent. | |

**User's choice:** Same closure plan
**Notes:** Planning state updates belong in Phase 17 closure, not a later loose follow-up.

---

## the agent's Discretion

None. The user selected concrete options for every discussed area.

## Deferred Ideas

None. Discussion stayed within Phase 17 scope.
