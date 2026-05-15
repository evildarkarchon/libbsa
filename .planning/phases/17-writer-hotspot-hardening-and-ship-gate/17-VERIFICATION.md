---
phase: 17-writer-hotspot-hardening-and-ship-gate
verified: 2026-05-15T02:15:00Z
status: passed
score: 16/16 must-haves verified
overrides_applied: 0
---

# Phase 17: Writer Hotspot Hardening and Ship Gate Verification Report

**Phase Goal:** The remaining high-cost and fragile writer staging paths are hardened under existing semantics, and the milestone closes with verified ship-ready evidence rather than a broad redesign.
**Verified:** 2026-05-15T02:15:00Z
**Status:** passed
**Re-verification:** No — previous artifact had no `gaps:` section; this is a final structured goal-backward verification update.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | D-01: BA2 DX10 `write_to` is terminal after any ordinary attempt. | ✓ VERIFIED | `src/formats/ba2/ba2_dx10_writer.cpp:94-105` checks `consumed`, sets it before `write_ba2_dx10_archive`, then cleans snapshots. Runtime tests include consumed-after-success and failure cases in focused gates. |
| 2 | D-02: After BA2 DX10 consumption, later `add_file` and `write_to` return `invalid_argument` through `result`. | ✓ VERIFIED | `ba2_dx10_writer.cpp:62-65` and `95-97` return `error_code::invalid_argument`; focused Debug/ASan writer gates passed 112/112. |
| 3 | D-03: Consumed-writer behavior remains BA2 DX10-specific. | ✓ VERIFIED | Consumed state exists only in `src/formats/ba2/ba2_dx10_writer.cpp`; `writer_hotspot_policy` guards BA2 DX10 public declaration shape and no cross-writer lifecycle expansion. |
| 4 | D-04: BA2 DX10 cleanup takes priority over retryability. | ✓ VERIFIED | `ba2_dx10_writer.cpp:101-105` marks consumed before write pipeline and calls `cleanup_snapshot_dir`; docs explain discarded snapshot data and no retry reuse. |
| 5 | D-05: Dedupe hardening combines runtime tests with source/policy guardrails. | ✓ VERIFIED | Runtime labels `tes4_bsa_writer`, `ba2_gnrl_writer` plus `tests/unit/writer_hotspot_policy_tests.cpp` all ran in focused Debug and ASan gates. |
| 6 | D-06: Dedupe proof is algorithmic/keyed rather than timing-based. | ✓ VERIFIED | TES4 uses `std::map<tes4_dedupe_identity,...>` and BA2 GNRL uses `std::map<ba2_gnrl_final_stored_dedupe_key,...>` before exact equality; policy tests assert these structures. |
| 7 | D-07: TES4 and BA2 GNRL narrowing stay format-local. | ✓ VERIFIED | TES4 narrowing is private to `src/formats/bsa/tes4_bsa_layout.cpp`; BA2 GNRL narrowing is private to `src/formats/ba2/ba2_gnrl_prepare.*` and `ba2_gnrl_layout.cpp`; no generic dedupe framework or new dependency. |
| 8 | D-08: Exact final stored-byte equality remains mandatory before shared offsets. | ✓ VERIFIED | TES4 shared offset assignment occurs only under `tes4_stored_payloads_equal` at `tes4_bsa_layout.cpp:274-284`; BA2 GNRL under `ba2_gnrl_payloads_equal` at `ba2_gnrl_layout.cpp:140-149`. |
| 9 | D-09: BA2 DX10 ordinary failure cleanup tests cover validation, missing/truncated snapshot data, and output/publish failure. | ✓ VERIFIED | `ba2_dx10_writer_tests.cpp` contains cleanup tests for validation failure, missing snapshot, truncated snapshot, output failure, and add-file reservation cleanup; focused gates passed. |
| 10 | D-10: Cleanup failure does not replace primary errors. | ✓ VERIFIED | `cleanup_snapshot_dir()` is `noexcept`, uses `std::error_code`, ignores cleanup errors, and write path returns saved `written.error()` (`ba2_dx10_writer.cpp:31-38`, `103-108`). |
| 11 | D-11: Failed BA2 DX10 `add_file` cleans a newly reserved snapshot directory. | ✓ VERIFIED | `ba2_dx10_writer.cpp:70-82` records empty snapshot dir and cleans on entry creation failure; test `BA2 DX10 writer cleans newly reserved snapshot directory when add_file fails` passed. |
| 12 | D-12: Cleanup tests track only newly created writer-owned snapshot directories. | ✓ VERIFIED | `ba2_dx10_writer_tests.cpp` uses `snapshot_directories`, `new_snapshot_directories_since`, and `libbsa-dx10-snapshot-` prefix. |
| 13 | D-13: Maintainer can run focused Debug, focused MSVC ASan, full Debug, and Release package proof gates. | ✓ VERIFIED | Independent reruns: focused Debug 112/112, focused ASan 112/112, full Debug 403/403 with 2 opt-in skips, Release package proof 2/2. |
| 14 | D-14: Official ship-gate evidence is runnable from committed assets; optional local corpus/BSArchPro remains advisory. | ✓ VERIFIED | All commands ran through CTest presets from committed assets. `17-VERIFICATION.md` and `17-05-SUMMARY.md` explicitly exclude optional local corpus/BSArchPro comparison from official sign-off. |
| 15 | D-15: Ship-gate evidence includes BA2 DX10 lifecycle guarantees and residual abnormal-termination risk documentation. | ✓ VERIFIED | `include/libbsa/writer.hpp:397-470`, `docs/target-format-guide.md:74-84`, and `docs/integration-examples.md:60-62` document consumed write, cleanup, destructor safety net, and residual abnormal-termination risk. |
| 16 | D-16: Planning surfaces mark DEDU-01, DEDU-02, DX10-01, and DX10-02 complete only with closure evidence. | ✓ VERIFIED | `.planning/REQUIREMENTS.md:34-40` and traceability rows 84-87 are complete; `.planning/ROADMAP.md:123-146` lists 5/5 Phase 17 plans and ship gate evidence. |

**Score:** 16/16 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `src/formats/bsa/tes4_bsa_layout.cpp` | TES4 format-local dedupe candidate narrowing with exact fallback | ✓ VERIFIED | `tes4_dedupe_identity`, `make_tes4_dedupe_identity`, keyed `deduplicated_payloads`, and exact `tes4_stored_payloads_equal` branch are present. |
| `src/formats/ba2/ba2_gnrl_prepare.hpp/.cpp` | Explicit BA2 GNRL final-stored identity/digest evidence | ✓ VERIFIED | `final_stored_dedupe_hash` exists on prepared entries and is populated from `payload_hash`; disk streams store `resolved_source_path`. |
| `src/formats/ba2/ba2_gnrl_layout.cpp` | BA2 GNRL candidate narrowing plus exact fallback | ✓ VERIFIED | Uses `ba2_gnrl_final_stored_dedupe_key`; exact fallback uses `resolved_source_path` and size-change `io_error` diagnostics. |
| `src/formats/ba2/ba2_dx10_writer.cpp` | BA2 DX10 consumed state and best-effort cleanup | ✓ VERIFIED | `state::consumed`, `cleanup_snapshot_dir() noexcept`, add-time cleanup, and write-time cleanup are implemented. |
| `include/libbsa/writer.hpp` | Public BA2 DX10 lifecycle documentation without signature expansion | ✓ VERIFIED | Doxygen documents consumed `write_to`, cleanup, and `invalid_argument`; policy test locks public declaration shape. |
| `docs/target-format-guide.md` / `docs/integration-examples.md` | BA2 DX10 lifecycle and residual risk docs | ✓ VERIFIED | Both docs state best-effort cleanup and residual crash/termination/OS-shutdown/interference risk without crash-proof claims. |
| `tests/unit/writer_hotspot_policy_tests.cpp` | Source/docs/public API guardrails | ✓ VERIFIED | Five policy tests cover TES4, BA2 GNRL, disk-source diagnostics, BA2 DX10 docs, and public API shape; passed in Debug and ASan. |
| `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/PROJECT.md`, `.planning/STATE.md` | Closure status consistent with ship evidence | ✓ VERIFIED | Requirement status and roadmap Phase 17 completion match gate evidence. |
| `.planning/phases/17-writer-hotspot-hardening-and-ship-gate/17-REVIEW.md` | Clean advisory review after CR-01 fix | ✓ VERIFIED | Frontmatter `status: clean`, zero findings; git shows fix commit `f1d59c9` and clean review commit `3db00f7`. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| TES4 layout candidate key | `tes4_stored_payloads_equal` | Bucketed duplicate loop | ✓ WIRED | `tes4_assign_offsets` computes identity, looks up bucket, then calls exact equality before assigning shared offset. |
| BA2 GNRL prepare | BA2 GNRL layout | `final_stored_dedupe_hash` and `resolved_source_path` fields | ✓ WIRED | Prepare fills fields; layout reads them for dedupe keying and disk-backed exact comparison. |
| BA2 GNRL layout | Shared host-file seam | `detail::host_file_path` in exact comparison | ✓ WIRED | `ba2_gnrl_payloads_equal` calls compare helpers with `resolved_source_path`; CR-01 raw path issue is fixed. |
| BA2 DX10 writer | Snapshot cleanup | `cleanup_snapshot_dir()` after add/write paths and destructor | ✓ WIRED | Add-time failure, write success/failure, and destructor safety net all reach cleanup helper. |
| Public docs | Policy tests | Source/docs token checks | ✓ WIRED | `writer_hotspot_policy` reads header, docs, integration examples, and DX10 source. |
| Ship-gate artifact | Requirements/roadmap closure | Requirement IDs and phase status | ✓ WIRED | Requirement IDs appear in final evidence and planning surfaces are complete. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `tes4_bsa_layout.cpp` | `tes4_dedupe_identity{stored_size, fingerprint}` | Final stored payload bytes and disk chunks via host-file helper | Yes | ✓ FLOWING |
| `ba2_gnrl_prepare.hpp/.cpp` → `ba2_gnrl_layout.cpp` | `final_stored_dedupe_hash` | Prepared final stored payload hash for memory/compressed paths; disk hash for raw disk path | Yes | ✓ FLOWING |
| `ba2_gnrl_layout.cpp` | `resolved_source_path` | Prepare-time host path resolution | Yes | ✓ FLOWING |
| `ba2_dx10_writer.cpp` | `snapshot_dir`, `consumed` | Snapshot builder reservation and write lifecycle | Yes | ✓ FLOWING |
| Docs/policy | Lifecycle terms and forbidden overclaim tokens | Committed header/docs/source text | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused Debug writer/runtime and policy gate | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|writer_hotspot_policy"` | 112/112 tests passed | ✓ PASS |
| Focused MSVC ASan writer-hotspot gate | `ctest --preset windows-msvc-asan-static --output-on-failure -L "tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|writer_hotspot_policy"` | 112/112 tests passed | ✓ PASS |
| Full Debug regression gate | `ctest --preset windows-msvc-debug-static --output-on-failure` | 403/403 tests passed; 2 opt-in local-fixture tests skipped | ✓ PASS |
| Release package proof | `ctest --preset windows-msvc-release-static --output-on-failure -R "package_consumer_smoke|package_consumer_runtime_dll_copy"` | 2/2 tests passed | ✓ PASS |
| Writer hotspot policy | `ctest --preset windows-msvc-debug-static --output-on-failure -L writer_hotspot_policy` | 5/5 tests passed | ✓ PASS |
| Boundary check | `git status --short -- "TES5Edit" "vcpkg.json" "include/libbsa/writer.hpp" ".planning/phases/17-writer-hotspot-hardening-and-ship-gate/17-VERIFICATION.md"` | No output | ✓ PASS |
| Review fix evidence | `git show --stat --oneline --no-renames f1d59c9 3db00f7` | Fix and clean-review commits found | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
|---|---|---|---|
| N/A | No `probe-*.sh` scripts were declared for Phase 17; verification criteria use CTest gates. | Skipped by design | ✓ SKIP |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| DEDU-01 | 17-01, 17-05 | Consumer can write TES4-family archives with dedupe enabled using faster candidate narrowing while preserving exact stored-byte equality behavior. | ✓ SATISFIED | TES4 keyed narrowing in `tes4_bsa_layout.cpp`; tests for shared offsets and mismatch separation; focused Debug/ASan gates passed. |
| DEDU-02 | 17-02, 17-05 | Consumer can write BA2 GNRL archives with stronger staged identity/digest narrowing while preserving exact equality fallback behavior. | ✓ SATISFIED | `final_stored_dedupe_hash`, keyed BA2 GNRL buckets, exact fallback through resolved host paths, CR-01 fix `f1d59c9`, non-ASCII disk-source dedupe regression. |
| DX10-01 | 17-03, 17-05 | Consumer can write BA2 DX10 archives with temporary staging data cleaned up during normal write completion and ordinary failure unwinding. | ✓ SATISFIED | `cleanup_snapshot_dir`, consumed state, runtime cleanup tests for success/failure/add-time failure, focused Debug/ASan gates passed. |
| DX10-02 | 17-04, 17-05 | Maintainer can verify and document BA2 DX10 temporary-data lifecycle including residual abnormal-termination risk. | ✓ SATISFIED | Header/docs/integration examples document lifecycle and risk; policy tests guard terms and public API shape. |

No orphaned Phase 17 requirement IDs were found in `.planning/REQUIREMENTS.md`; all four Phase 17 IDs are claimed by plans and marked complete.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| N/A | N/A | No unreferenced `TBD`, `FIXME`, or `XXX` markers found in Phase 17 implementation/test files. General empty `return {}` matches are ordinary `result<void>` success returns, not stubs. | ℹ️ Info | No blocker or warning. |

### Human Verification Required

None. Residual abnormal-termination behavior is intentionally documented and policy-tested; crash/forced-termination cleanup is out of scope and not a required manual gate for Phase 17.

### Gaps Summary

No gaps found. The codebase implements the Phase 17 must-haves, the post-review CR-01 remediation is present and tested, the clean review artifact is committed, and the Debug/ASan/Release ship gates pass from committed assets.

---

_Verified: 2026-05-15T02:15:00Z_
_Verifier: the agent (gsd-verifier)_
