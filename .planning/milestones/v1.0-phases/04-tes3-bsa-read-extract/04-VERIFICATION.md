---
phase: 04-tes3-bsa-read-extract
verified: 2026-05-10T10:00:00Z
status: passed
score: 12/12 must-haves verified
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  gaps_closed:
    - "TES3 malformed hash-collision fixture now exercises the intended duplicate stored-hash branch."
    - "TES3 extraction now dispatches to a bounded host-file streaming helper before generic payload buffering."
  gaps_remaining: []
---

# Phase 04: TES3 BSA Read/Extract Verification Report

**Phase Goal:** Consumers can read, list, query, and extract TES3/Morrowind BSA archives while preserving TES3-specific data-section-relative offset behavior.
**Verified:** 2026-05-10T10:00:00Z
**Status:** passed
**Re-verification:** Yes - after `04-05` gap closure.

## Goal Achievement

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Consumer can open TES3 BSA archives and list/query entries through the public `archive_reader` API. | VERIFIED | Phase 04 summaries and current TES3 reader tests cover open/list/query behavior. |
| 2 | TES3 payload offsets use data-section-relative source semantics converted to archive-absolute metadata. | VERIFIED | TES3 fixtures and reader tests compare raw data offsets, public payload offsets, and extracted bytes. |
| 3 | TES3 fixtures isolate hash and offset rules. | VERIFIED | `04-05-SUMMARY.md` records separated stored-hash mismatch and collision structural markers. |
| 4 | TES3 extraction streams bounded chunks directly from the host archive to the sink. | VERIFIED | `04-05-SUMMARY.md` records host-path extraction dispatch before generic payload buffering and 64 KiB chunk enforcement. |
| 5 | Malformed TES3 spans, overlaps, invalid paths, duplicate paths, hash mismatches, and duplicate stored hashes fail closed. | VERIFIED | TES3 malformed fixture generation/tests passed after `04-05` gap closure. |
| 6 | Public metadata remains format-neutral and dependency-light. | VERIFIED | Public include boundary tests passed. |
| 7 | TES5Edit remains untouched. | VERIFIED | Current audit evidence and `04-05-SUMMARY.md` report empty `git -C TES5Edit status --short`. |

**Score:** 12/12 must-haves verified.

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| BSA-04 | 04-01 through 04-05 | Consumer can read and extract files from TES3/Morrowind BSA archives. | SATISFIED | `04-05-SUMMARY.md` closes the prior hash-collision and bounded-extraction gaps; current full CTest passes. |
| BSA-08 | 04-01 through 04-05 | Consumer can extract TES3 entries using data-section-relative offset semantics. | SATISFIED | TES3 fixture manifests and reader tests prove data-section-relative offset handling. |

No Phase 04 requirements are orphaned.

## Gap Closure

| Prior Gap | Closure Evidence | Status |
|---|---|---|
| Collision fixture was masked by stored-hash mismatch validation. | `04-05-SUMMARY.md` records parser validation reordering plus manifest/test branch-intent assertions. | RESOLVED |
| TES3 sink extraction pre-buffered selected payloads. | `04-05-SUMMARY.md` records host-file streaming helper and bounded chunk regression coverage. | RESOLVED |

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Phase gap gate | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_malformed|tes3_bsa_extract|tes4_bsa|public_include_boundary" --output-on-failure` | Passed per `04-05-SUMMARY.md`. | PASS |
| Current milestone full static CTest | `ctest --preset windows-msvc-debug-static --output-on-failure` | Passed 245/245 runnable tests with 2 expected opt-in fixture skips. | PASS |
| TES5Edit boundary | `git -C TES5Edit status --short` | No output in current audit evidence. | PASS |

## Gaps Summary

No gaps remain.

---

_Verified: 2026-05-10T10:00:00Z_
_Verifier: Codex_
