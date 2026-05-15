# Phase 14: verification-lane-truthfulness - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-13
**Phase:** 14-verification-lane-truthfulness
**Areas discussed:** ASan lane role, Release smoke scope, Matrix emphasis, Drift guard style, Planning updates, CI topology, Policy structure

---

## ASan lane role

### Question 1

| Option | Description | Selected |
|--------|-------------|----------|
| Hardening lane | Official supported lane, but framed as the instrumented hardening lane rather than the default path | ✓ |
| Equal peer lane | Same prominence as Debug and Release everywhere | |
| You decide | Leave framing to downstream planning | |

**User's choice:** Hardening lane

### Question 2

| Option | Description | Selected |
|--------|-------------|----------|
| Separate hardening job | Show ASan as its own clearly named CI job | ✓ |
| Main matrix row | Keep it as another row in the primary Windows matrix | |
| You decide | Leave the CI shape to implementation | |

**User's choice:** Separate hardening job

### Question 3

| Option | Description | Selected |
|--------|-------------|----------|
| Risky changes | Recommend the lane before shipping parser, writer, compression, or validation changes | ✓ |
| Every change | Make it part of every local verification loop | |
| Debug memory issues | Treat it as mostly troubleshooting-only | |

**User's choice:** Risky changes

### Question 4

| Option | Description | Selected |
|--------|-------------|----------|
| Name the distinction | Explicitly call it the `MSVC AddressSanitizer hardening lane` and separate it from Release package proof | ✓ |
| Minimal distinction | Let presets and CI speak for themselves | |
| Detailed caveats | Add heavier caveat wording inline | |

**User's choice:** Name the distinction
**Notes:** The user wanted ASan supported and visible, but clearly differentiated from the normal Release package-proof lanes.

---

## Release smoke scope

### Question 1

| Option | Description | Selected |
|--------|-------------|----------|
| Both release lanes | Run full install/export plus downstream consumer smoke on Release static and Release shared | ✓ |
| One canonical lane | Only one Release lane owns full downstream smoke | |
| You decide | Leave smoke ownership to downstream planning | |

**User's choice:** Both release lanes

### Question 2

| Option | Description | Selected |
|--------|-------------|----------|
| Inside ctest | Keep package-consumer smoke inside each Release preset's `ctest` flow | ✓ |
| Separate CI step | Run it as explicit CI-only steps after core tests | |
| Hybrid | Keep it checked in but give CI an extra explicit package-proof step | |

**User's choice:** Inside ctest

### Question 3

| Option | Description | Selected |
|--------|-------------|----------|
| Call out both | Explicitly mention install/export generation and downstream consumer smoke | ✓ |
| Emphasize consumer smoke | Treat install/export as implicit detail | |
| Minimal wording | Keep the wording short and indirect | |

**User's choice:** Call out both

### Question 4

| Option | Description | Selected |
|--------|-------------|----------|
| Blocking failure | A package-smoke failure means the Release lane is not truthful | ✓ |
| Advisory signal | Keep the smoke result visible but non-blocking | |
| You decide | Leave the failure posture to downstream planning | |

**User's choice:** Blocking failure
**Notes:** The user treated package proof as part of the supported Release contract, not as optional extra coverage.

---

## Matrix emphasis

### Question 1

| Option | Description | Selected |
|--------|-------------|----------|
| Quick path + matrix | Lead with one obvious day-to-day path, then show the full supported matrix | ✓ |
| Matrix first | Open with the full supported matrix immediately | |
| Release-led story | Lead with the Release story first | |

**User's choice:** Quick path + matrix

### Question 2

| Option | Description | Selected |
|--------|-------------|----------|
| Debug static | Keep `windows-msvc-debug-static` as the quick maintainer example | ✓ |
| Release static | Make Release static the first example | |
| Two-lane quickstart | Show Debug static and Release static together up front | |

**User's choice:** Debug static

### Question 3

| Option | Description | Selected |
|--------|-------------|----------|
| By lane role | Group lanes as Debug, Release package-proof, and ASan hardening | ✓ |
| Flat list | Present all supported presets without role grouping | |
| CI order only | Mirror CI ordering without adding documentation grouping | |

**User's choice:** By lane role

### Question 4

| Option | Description | Selected |
|--------|-------------|----------|
| Per-role commands | Show concise commands for each lane role after the quick path | ✓ |
| Names only | List only preset names | |
| Full commands everywhere | Repeat full command blocks for every lane | |

**User's choice:** Per-role commands
**Notes:** The user wanted the docs to stay easy to scan while still making the supported matrix concrete and runnable.

---

## Drift guard style

### Question 1

| Option | Description | Selected |
|--------|-------------|----------|
| Exact contract, flexible prose | Lock exact facts in tests while allowing wording style to vary | ✓ |
| Everything exact | Enforce near-verbatim wording everywhere | |
| Invariants only | Keep the tests at a high-level presence check | |

**User's choice:** Exact contract, flexible prose

### Question 2

| Option | Description | Selected |
|--------|-------------|----------|
| Each listed surface | Every named surface should independently describe the same contract | ✓ |
| Docs plus tests | Keep most explicit wording only in docs and tests | |
| Presets as source | Treat presets and CI as the real contract and keep docs lighter | |

**User's choice:** Each listed surface

### Question 3

| Option | Description | Selected |
|--------|-------------|----------|
| Lock role facts | Policy tests assert lane roles, not just preset presence | ✓ |
| Names plus exclusions | Only lock lane names and exclusions | |
| Minimal role checks | Keep role wording out of tests | |

**User's choice:** Lock role facts

### Question 4

| Option | Description | Selected |
|--------|-------------|----------|
| Same policy gate | Keep the opt-in local corpus rule in the same truthfulness policy suite | ✓ |
| Docs only | Document the rule but don’t test it here | |
| Separate test file | Guard it in a different policy suite | |

**User's choice:** Same policy gate
**Notes:** The user wanted a strong drift-prevention gate, but not one so prose-fragile that harmless wording edits would churn tests.

---

## Planning updates

### Question 1

| Option | Description | Selected |
|--------|-------------|----------|
| Role-aware summary | Planning docs name the supported roles without duplicating command-level detail | ✓ |
| Full matrix detail | Duplicate more of the full matrix inside planning surfaces | |
| Context-only detail | Keep planning surfaces very high-level and push most detail to context/docs | |

**User's choice:** Role-aware summary

### Question 2

| Option | Description | Selected |
|--------|-------------|----------|
| Stay concise | Keep `STATE.md` focused on state/resume info with only a short matrix note | ✓ |
| Carry the matrix summary | Put fuller lane detail into `STATE.md` | |
| Context link only | Keep `STATE.md` procedural and point elsewhere | |

**User's choice:** Stay concise

### Question 3

| Option | Description | Selected |
|--------|-------------|----------|
| Fix the history | Keep chronology truthful and make Phase 14 the v1.1 outcome for the new lanes | ✓ |
| Leave it as capability | Let older wording stand once the lanes exist | |
| Minimize history edits | Avoid touching older validated wording unless obviously broken | |

**User's choice:** Fix the history

### Question 4

| Option | Description | Selected |
|--------|-------------|----------|
| In 14-CONTEXT | Keep the most detailed downstream contract in the phase context file | ✓ |
| In PROJECT.md | Move more of the detail into the project-wide planning doc | |
| Split evenly | Duplicate the detail across both | |

**User's choice:** In 14-CONTEXT
**Notes:** The user wanted project-level planning docs to stay truthful and helpful, but not turn into the full detailed contract that downstream agents consume.

---

## CI topology

### Question 1

| Option | Description | Selected |
|--------|-------------|----------|
| Main matrix + ASan job | One Debug/Release matrix plus one separate ASan hardening job | ✓ |
| Debug/Release split | Separate Debug and Release matrices plus a separate ASan job | |
| All separate jobs | Give every lane its own top-level job | |

**User's choice:** Main matrix + ASan job

### Question 2

| Option | Description | Selected |
|--------|-------------|----------|
| Keep fail-fast off | Let the full supported matrix finish even after an early failure | ✓ |
| Fail fast | Stop early to save CI time | |
| Role-based fail-fast | Use different fail-fast behavior by role | |

**User's choice:** Keep fail-fast off

### Question 3

| Option | Description | Selected |
|--------|-------------|----------|
| Run in parallel | Start the ASan hardening job alongside the main matrix | ✓ |
| Wait for main matrix | Treat ASan as a second-stage job | |
| You decide | Leave the dependency shape to downstream planning | |

**User's choice:** Run in parallel

### Question 4

| Option | Description | Selected |
|--------|-------------|----------|
| Role + preset | Job names show both lane role and concrete preset | ✓ |
| Preset only | Use only raw preset names | |
| Role only | Use only broad role labels | |

**User's choice:** Role + preset
**Notes:** The user preferred clearer CI visibility over maximum workflow minimalism.

---

## Policy structure

### Question 1

| Option | Description | Selected |
|--------|-------------|----------|
| Focused section in existing file | Keep the truthfulness gate in `validation_policy_tests.cpp` with a focused Phase 14 section | ✓ |
| Dedicated new file | Split verification-matrix truthfulness into its own file | |
| Spread them out | Scatter assertions near other files/tests | |

**User's choice:** Focused section in existing file

### Question 2

| Option | Description | Selected |
|--------|-------------|----------|
| Shared contract table | Use one expected-contract table/helper set for lanes, roles, smoke, and exclusions | ✓ |
| Ad hoc string checks | Keep adding one-off `find(...)` assertions | |
| Surface-by-surface only | Duplicate expected facts separately in each test | |

**User's choice:** Shared contract table

### Question 3

| Option | Description | Selected |
|--------|-------------|----------|
| Each surface vs contract | Check each surface independently against the same shared contract | ✓ |
| Pairwise comparisons | Compare files mostly to each other | |
| Execution files first | Treat presets/CI as primary and docs/planning as looser followers | |

**User's choice:** Each surface vs contract

### Question 4

| Option | Description | Selected |
|--------|-------------|----------|
| Same-PR contract update | Future lane changes must update contract and surfaces together in one PR | ✓ |
| Allow staged updates | Let execution and docs drift briefly across follow-up work | |
| Docs follow later | Allow docs/policy to trail execution changes | |

**User's choice:** Same-PR contract update
**Notes:** The user wanted one obvious truthfulness gate with a shared contract model, not scattered or staged alignment.

---

## the agent's Discretion

None.

## Deferred Ideas

None.
