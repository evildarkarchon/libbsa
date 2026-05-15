# Phase 14: verification-lane-truthfulness - Context

**Gathered:** 2026-05-13
**Status:** Ready for planning

## Phase Boundary

Phase 14 makes the supported Windows verification matrix truthful across presets, CI, maintainer docs, planning surfaces, and policy tests. The phase adds official Release static, Release shared, and MSVC AddressSanitizer static lanes, keeps committed package-consumer proof inside the supported Release contract, and makes drift between the declared matrix and the checked-in repo a test failure.

## Requirements (locked via SPEC.md)

**5 requirements are locked.** See `14-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `14-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Checked-in configure/build/test preset families for `windows-msvc-release-static`, `windows-msvc-release-shared`, and `windows-msvc-asan-static`
- Supported CI execution for the retained debug lanes plus the new Release static, Release shared, and ASan static lanes
- Release-lane install/export verification and at least one checked-in package-consumer smoke flow
- Truthfulness alignment across `.planning`, `README.md`, `tests/fixtures/README.md`, `CMakePresets.json`, `.github/workflows/ci.yml`, and `tests/unit/validation_policy_tests.cpp`
- Explicit preservation of Windows-only scope and opt-in `requires-game-fixture` behavior in the supported verification contract

**Out of scope (from SPEC.md):**
- Linux, WSL, macOS, POSIX, or other cross-platform verification lanes - excluded because libbsa remains Windows-only
- UBSan, TSan, fuzzing, or sanitizer families beyond MSVC AddressSanitizer - excluded to keep Phase 14 limited to the locked supported matrix
- Mandatory BSArchPro-derived or game-corpus checks in the default supported lanes - excluded because supported lanes must remain runnable from committed assets only
- New runtime dependencies, archive-family support, or public API expansion - excluded because Phase 14 is verification hardening, not product-surface growth
- Broader performance, parser, reader-dispatch, dedupe, or DX10 staging work - excluded because those belong to later phases in the v1.1 roadmap

## Implementation Decisions

### ASan Lane Role
- **D-01:** `windows-msvc-asan-static` is an official supported lane, but it is explicitly the `MSVC AddressSanitizer hardening lane`, not the everyday default path.
- **D-02:** CI should show ASan as its own clearly named hardening job rather than burying it as just another row in the main Windows matrix.
- **D-03:** Maintainer guidance should recommend the ASan lane for risky parser, writer, compression, and validation changes before shipping, not for every trivial local edit.
- **D-04:** Docs should explicitly distinguish the ASan hardening lane from the Release package-proof lanes instead of relying on preset names alone.

### Release Package Proof
- **D-05:** Both `windows-msvc-release-static` and `windows-msvc-release-shared` own the full supported package-proof contract.
- **D-06:** The supported Release proof stays inside each Release lane's `ctest` path instead of becoming a separate ad hoc CI-only script flow.
- **D-07:** Release-lane wording and policy checks must call out both parts of the contract: install/export generation and downstream package-consumer smoke.
- **D-08:** A package-consumer smoke failure is a blocking Release-lane failure even when core library tests passed.

### Matrix Communication
- **D-09:** Maintainer-facing docs should use a `quick path + matrix` structure rather than leading with the full matrix immediately.
- **D-10:** The quick day-to-day example remains `windows-msvc-debug-static`.
- **D-11:** After the quick path, the remaining supported lanes should be grouped by role: Debug inner-loop lanes, Release package-proof lanes, and the ASan hardening lane.
- **D-12:** README-level docs should show concise per-role command guidance rather than only preset names or fully repeated commands for every lane.

### Drift Guard Contract
- **D-13:** The truthfulness gate should lock exact contract facts while allowing prose style to vary. Exact facts include lane names, lane roles, Release smoke ownership, Windows-only scope, and the opt-in local-corpus rule.
- **D-14:** Every listed surface should independently name the same contract in its own appropriate form rather than merely pointing elsewhere.
- **D-15:** Policy tests must lock role facts, not just preset presence: Debug stays the quick path, Release lanes own package proof, and ASan is the named hardening lane.
- **D-16:** The same truthfulness gate must also keep `requires-game-fixture` opt-in and prevent mandatory local corpus checks from becoming part of the default supported matrix.

### Planning Surface Updates
- **D-17:** `.planning/PROJECT.md` and `.planning/ROADMAP.md` should carry a role-aware summary of the supported matrix, but not duplicate the full command-level contract.
- **D-18:** `STATE.md` should stay concise and session-oriented, with only a short note about the truthful supported matrix instead of full lane detail.
- **D-19:** `PROJECT.md` must fix the existing hardening-coverage history so the chronology stays truthful: v1.0 history remains accurate, and the supported Release/ASan matrix is described as a v1.1 Phase 14 outcome.
- **D-20:** The most detailed lane-role and drift-guard contract lives in `14-CONTEXT.md`, not in the project-wide planning summaries.

### CI Topology
- **D-21:** CI should use one main Windows matrix for Debug and Release lanes plus one separate ASan hardening job.
- **D-22:** `fail-fast` stays disabled so maintainers can see the full supported matrix result set in one run.
- **D-23:** The separate ASan hardening job should run in parallel with the main matrix rather than waiting for it.
- **D-24:** GitHub Actions job names should expose both lane role and concrete preset, not just one or the other.

### Policy Test Structure
- **D-25:** Phase 14 truthfulness checks stay inside `tests/unit/validation_policy_tests.cpp`, but they should be grouped into a focused verification-matrix section instead of being scattered ad hoc.
- **D-26:** That policy section should use one shared expected-contract table/helper set for supported lanes, lane roles, package-smoke expectations, and exclusions.
- **D-27:** Each surface should be checked independently against the shared contract so one stale file cannot validate another stale file.
- **D-28:** Any future supported-lane, role, smoke, or exclusion change must update the shared expected contract and all affected surfaces in the same PR.

### the agent's Discretion
None. The discussion intentionally locked the user-facing matrix contract, lane roles, CI shape, and drift-guard behavior closely enough that downstream research and planning should not reopen them.

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope And Locked Requirements
- `.planning/PROJECT.md` — current v1.1 hardening goals, Windows-only boundary, and the project-level history that Phase 14 must correct and align.
- `.planning/REQUIREMENTS.md` — `VER-01`, `VER-02`, and `VER-03` traceability for the supported verification matrix.
- `.planning/ROADMAP.md` — Phase 14 goal, dependency ordering, and the milestone-level place of verification-lane truthfulness.
- `.planning/STATE.md` — current session status and the planning surface that should remain concise after this phase.
- `.planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md` — carry-forward context for Windows-only scope, opt-in fixture policy, and the earlier note that verification-lane reconciliation belongs to Phase 14.
- `.planning/phases/14-verification-lane-truthfulness/14-SPEC.md` — locked requirements, boundaries, and acceptance criteria. MUST read before planning.

### Live Verification Surfaces
- `CMakePresets.json` — current supported preset families and the main file that must add Release static, Release shared, and ASan static lanes.
- `.github/workflows/ci.yml` — current Windows CI matrix shape and the workflow that must reflect the new main-matrix-plus-ASan-job topology.
- `README.md` — maintainer-facing quick path and supported matrix communication surface.
- `tests/fixtures/README.md` — fixture policy, Windows-only language, and opt-in `requires-game-fixture` contract that must stay aligned with the supported lanes.
- `tests/unit/validation_policy_tests.cpp` — current policy gate that locks verification-surface truthfulness and should host the focused Phase 14 contract checks.
- `tests/CMakeLists.txt` — where `package_consumer_smoke` is registered under CTest and where the supported test contract already hooks into package proof.
- `tests/package-consumer/CMakeLists.txt` — downstream consumer project shape, including shared-runtime DLL handling.
- `tests/package-consumer/smoke.cmake` — checked-in install/export plus downstream package-consumer smoke flow that Phase 14 should reuse rather than replace.

### Codebase Guidance
- `.planning/codebase/STACK.md` — current tooling stack, CMake/CTest/GitHub Actions usage, and where package-consumer smoke already exists in the build story.
- `.planning/codebase/ARCHITECTURE.md` — current build/test/package flow layering and how verification surfaces connect across presets, tests, and install/export behavior.
- `.planning/codebase/INTEGRATIONS.md` — CI/CD and consumer-integration context for GitHub Actions, package install/export, and package-consumer smoke.
- `.planning/codebase/CONCERNS.md` — the original build-policy drift concern and the explicit hardening gaps Phase 14 is intended to close.

### External Specs
- No external specs — the relevant contract is fully captured in the repository files above.

## Existing Code Insights

### Reusable Assets
- `tests/package-consumer/smoke.cmake`: existing checked-in install/export plus downstream consumer smoke flow that already matches the Phase 14 package-proof goal.
- `tests/package-consumer/CMakeLists.txt`: existing consumer project with shared-install runtime DLL handling, which makes shared Release smoke worth proving directly.
- `tests/unit/validation_policy_tests.cpp`: existing documentation/policy gate that can host the focused verification-matrix truthfulness section without introducing a second truth gate.
- `CMakePresets.json` and `.github/workflows/ci.yml`: existing preset-and-matrix pattern that can be extended incrementally rather than replaced.

### Established Patterns
- Supported verification policy is already guarded by repo-reading tests instead of tribal knowledge; Phase 14 should extend that pattern with a shared expected contract table.
- `ctest` is already the supported local verification entrypoint, and package-consumer smoke already runs as part of the test surface instead of an external helper script.
- README currently demonstrates one concrete command path; Phase 14 should evolve that into a quick-path-plus-role-matrix pattern rather than a docs rewrite.
- Planning documents are summary-oriented, while per-phase context files carry the detailed implementation contract. Phase 14 should preserve that division of labor.

### Integration Points
- `CMakePresets.json`: add Release static/shared and ASan static preset families that align with the role decisions above.
- `.github/workflows/ci.yml`: reshape the CI topology into one Debug/Release matrix plus one separate parallel ASan hardening job.
- `README.md` and `tests/fixtures/README.md`: update the supported matrix language to match the quick-path, role-grouped contract.
- `tests/unit/validation_policy_tests.cpp`: add the focused shared-contract truthfulness checks that compare every listed surface independently.
- `.planning/PROJECT.md`, `.planning/ROADMAP.md`, and `.planning/STATE.md`: align planning summaries with the new truthful supported matrix while keeping their agreed detail level.

## Specific Ideas

- Name the instrumented lane explicitly as the `MSVC AddressSanitizer hardening lane` wherever the supported matrix is summarized.
- Keep `windows-msvc-debug-static` as the first maintainer command path in `README.md`, then group the remaining supported lanes by role.
- Treat `package_consumer_smoke` as part of the blocking Release contract inside `ctest`, not as a CI-only afterthought.
- Build the policy gate around one shared expected-contract table so future lane changes must update all affected surfaces together.

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 14-verification-lane-truthfulness*
*Context gathered: 2026-05-13*
