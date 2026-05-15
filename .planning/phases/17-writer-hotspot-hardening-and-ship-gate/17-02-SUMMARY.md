---
phase: 17-writer-hotspot-hardening-and-ship-gate
plan: 02
subsystem: writer-hardening
tags: [cpp, catch2, ba2-gnrl, dedupe, writer-hotspot-policy, tdd]

requires:
  - phase: 17-writer-hotspot-hardening-and-ship-gate
    provides: [TES4 dedupe narrowing and writer-hotspot policy suite from 17-01]
provides:
  - Explicit BA2 GNRL final-stored dedupe fingerprint evidence in prepared entries
  - BA2 GNRL layout candidate narrowing keyed by stored size and final-stored fingerprint
  - Source-policy proof that exact equality and disk-source change rejection remain in the sharing path
affects: [writer-hotspot-hardening, DEDU-02, ba2-gnrl-writer]

tech-stack:
  added: []
  patterns:
    - Format-local final-stored dedupe key as candidate filter only
    - Source-policy guardrails for staged identity plus exact equality fallback

key-files:
  created: []
  modified:
    - tests/unit/writer_hotspot_policy_tests.cpp
    - src/formats/ba2/ba2_gnrl_prepare.hpp
    - src/formats/ba2/ba2_gnrl_prepare.cpp
    - src/formats/ba2/ba2_gnrl_layout.cpp

key-decisions:
  - "BA2 GNRL prepared entries now expose a named final_stored_dedupe_hash for candidate narrowing; it mirrors the final stored payload fingerprint and is not a correctness authority."
  - "BA2 GNRL layout buckets dedupe candidates by stored size plus final_stored_dedupe_hash, then still calls ba2_gnrl_payloads_equal before sharing offsets."

patterns-established:
  - "BA2 GNRL dedupe narrowing: prepare final-stored fingerprint evidence, look up only matching keyed buckets, and assign shared offsets only after exact equality."
  - "Writer-hotspot policy tests assert deliberate contract names plus equality/disk-change evidence instead of adding timing-sensitive benchmarks."

requirements-completed: [DEDU-02]

duration: 5 min
completed: 2026-05-15
---

# Phase 17 Plan 02: BA2 GNRL Staged Dedupe Identity Hardening Summary

**BA2 GNRL writer dedupe now uses explicit final-stored fingerprint evidence for candidate narrowing while preserving exact stored-byte equality and disk-source change rejection.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-15T01:22:00Z
- **Completed:** 2026-05-15T01:26:46Z
- **Tasks:** 3 completed
- **Files modified:** 4

## Accomplishments

- Added BA2 GNRL writer-hotspot policy assertions for staged final-stored dedupe evidence, exact equality fallback, and disk-source change diagnostics.
- Added `final_stored_dedupe_hash` to prepared BA2 GNRL entries and populated it during final stored-payload preparation without new dependencies.
- Changed BA2 GNRL layout dedupe lookup to use a named final-stored key while keeping `ba2_gnrl_payloads_equal` as the mandatory sharing authority.

## TDD Evidence

### RED

- **Commit:** `a517290` — `test(17-02): add failing test for BA2 GNRL dedupe identity hardening`
- **Command:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_gnrl_writer|writer_hotspot_policy"`
- **Expected failure:** `writer_hotspot_policy requires BA2 GNRL staged dedupe identity before exact equality` failed on missing `final_stored_dedupe_hash`.
- **Failure quality:** Build and test discovery succeeded; the only failure was the planned missing staged identity/digest evidence.

### GREEN

- **Commit:** `20ca31c` — `feat(17-02): implement BA2 GNRL staged dedupe identity narrowing`
- **Command:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_gnrl_writer|writer_hotspot_policy"`
- **Result:** 32/32 focused BA2 GNRL writer and writer-hotspot policy tests passed.

### REFACTOR

- **Commit:** None — no refactor commit was needed after inspection.
- **Command:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_gnrl_writer|writer_hotspot_policy"`
- **Result:** 32/32 focused tests passed with the narrowing local to BA2 GNRL prepare/layout code and policy assertions tied to deliberate contract identifiers.

## Task Commits

Each task was committed atomically where file changes were made:

1. **Task 1: RED - add BA2 GNRL staged identity and fallback tests** - `a517290` (test)
2. **Task 2: GREEN - add staged identity/digest narrowing with exact equality fallback** - `20ca31c` (feat)
3. **Task 3: REFACTOR - keep BA2 GNRL narrowing local and policy resilient** - no commit (no source changes required after inspection; focused gate passed)

## Files Created/Modified

- `tests/unit/writer_hotspot_policy_tests.cpp` - Added BA2 GNRL staged identity, exact equality, and disk-source change source-policy guardrails.
- `src/formats/ba2/ba2_gnrl_prepare.hpp` - Added the prepared-entry `final_stored_dedupe_hash` contract.
- `src/formats/ba2/ba2_gnrl_prepare.cpp` - Populates the explicit final-stored dedupe fingerprint from final stored payload hashing.
- `src/formats/ba2/ba2_gnrl_layout.cpp` - Uses a named final-stored dedupe key for candidate lookup before exact equality comparison.

## Decisions Made

- `final_stored_dedupe_hash` is a candidate filter only; collision safety still comes from `ba2_gnrl_payloads_equal`.
- The BA2 GNRL dedupe key stays format-local in prepare/layout code rather than adding a generic dedupe framework.
- Existing disk-source growth/truncation diagnostics remain source-policy guarded as `io_error` behavior.

## Verification

- **RED gate:** Focused label run failed as intended after successful build/test registration.
- **GREEN gate:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_gnrl_writer|writer_hotspot_policy"` passed: 32/32 tests.
- **Refactor gate:** Same focused command passed again: 32/32 tests.
- **Wave gate:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` passed: 400/400 CTest tests passed, with 2 opt-in local-fixture tests skipped.

## Post-Review Remediation

- **Commit:** `f1d59c9` — `fix(17-02): resolve BA2 GNRL dedupe disk paths`
- **Issue fixed:** The advisory Phase 17 review found that BA2 GNRL disk-backed exact dedupe comparisons still opened raw caller path text instead of prepare-time resolved host paths.
- **Resolution:** Disk-backed exact dedupe comparisons now use `resolved_source_path` with the shared host-file seam, preserving exact stored-byte equality while keeping Windows UTF-8/non-ASCII host paths reliable.
- **Regression:** `BA2 GNRL writer dedupes raw disk sources under non-ASCII host paths` covers duplicate raw disk-backed sources with `deduplicate_payloads = true` under a non-ASCII directory.
- **Verification:** Focused Debug and ASan ship gates now pass 112/112 selected writer-hotspot tests, and the final full Debug gate passes 403/403 tests.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes.

## Issues Encountered

None beyond the planned RED failure.

## Known Stubs

None.

## Threat Flags

None.

## User Setup Required

None - no external service configuration required.

## TDD Gate Compliance

- **RED:** Present (`a517290`)
- **GREEN:** Present after RED (`20ca31c`)
- **REFACTOR:** Not required; no refactor changes were made.
- **Status:** Passed

## Self-Check: PASSED

- `tests/unit/writer_hotspot_policy_tests.cpp` exists.
- `src/formats/ba2/ba2_gnrl_prepare.hpp` exists and contains `final_stored_dedupe_hash`.
- `src/formats/ba2/ba2_gnrl_prepare.cpp` exists and contains `ba2_gnrl_final_stored_dedupe_hash`.
- `src/formats/ba2/ba2_gnrl_layout.cpp` exists and contains `make_ba2_gnrl_final_stored_dedupe_key` and `ba2_gnrl_payloads_equal`.
- Commits `a517290` and `20ca31c` exist in git history.
- No files under `TES5Edit/` were modified.
- No public headers under `include/libbsa/` were modified.
- No dependency files were modified.

## Next Phase Readiness

Ready for Plan 17-04 closure. DEDU-02 now has runtime and source-policy evidence; DX10-02 documentation and final ship-gate evidence remain for the Phase 17 closeout plan.

---
*Phase: 17-writer-hotspot-hardening-and-ship-gate*
*Completed: 2026-05-15*
