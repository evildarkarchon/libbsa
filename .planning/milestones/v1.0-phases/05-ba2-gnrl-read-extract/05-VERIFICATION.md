---
phase: 05-ba2-gnrl-read-extract
verified: 2026-05-10T10:00:00Z
status: passed
score: 13/13 must-haves verified
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  gaps_closed:
    - "BA2 GNRL open/list parsing now reads only bounded metadata and filename-table bytes before extraction."
  gaps_remaining: []
---

# Phase 05: BA2 GNRL Read/Extract Verification Report

**Phase Goal:** Consumers can read, inspect, and extract Fallout 4 and Starfield BA2 GNRL archives, including Starfield metadata and compression routing.
**Verified:** 2026-05-10T10:00:00Z
**Status:** passed
**Re-verification:** Yes - after `05-06` gap closure.

## Goal Achievement

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Consumer can open FO4 BA2 GNRL, Starfield v2 GNRL, and structurally valid Starfield v3 GNRL archives by bytes. | VERIFIED | BA2 detector/parser tests and full CTest pass. |
| 2 | Consumer can list BA2 names from the `FileTableOffset` UInt16 length-prefixed filename table. | VERIFIED | BA2 GNRL parser/listing tests and generated manifests cover path parsing. |
| 3 | Public metadata reports raw/stored sizes, offsets, hashes, compression, and Starfield fields. | VERIFIED | BA2 GNRL metadata tests compare generated manifests. |
| 4 | Raw, deflate, and Starfield raw-LZ4-block BA2 GNRL extraction works through public APIs. | VERIFIED | BA2 GNRL extraction/compression tests pass. |
| 5 | Open/list parsing is bounded and does not read payload-region bytes. | VERIFIED | `05-06-SUMMARY.md` records sparse 8 GiB-offset regression coverage and first-payload-offset bounded filename-table reads. |
| 6 | TES5Edit remains untouched. | VERIFIED | Current audit evidence and `05-06-SUMMARY.md` report empty `git -C TES5Edit status --short`. |

**Score:** 13/13 must-haves verified.

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| GNRL-01 | 05-01 through 05-06 | Consumer can read and extract Fallout 4 BA2 GNRL archives. | SATISFIED | Generated FO4 fixture coverage plus bounded-open regression in `05-06`. |
| GNRL-02 | 05-01 through 05-06 | Consumer can read and extract Starfield BA2 v2 GNRL archives. | SATISFIED | Generated Starfield v2 fixture coverage plus bounded-open parser fix. |
| GNRL-03 | 05-01 through 05-06 | Consumer can read and extract structurally valid Starfield BA2 v3 GNRL archives. | SATISFIED | Generated Starfield v3 raw-LZ4-block fixture coverage plus bounded-open parser fix. |
| GNRL-04 | 05-01 through 05-06 | Consumer can parse BA2 filename tables at `FileTableOffset` with length-prefixed names. | SATISFIED | File-backed parser now reads `[FileTableOffset, first_payload_offset)` only. |
| GNRL-05 | 05-01 through 05-05 | Consumer can distinguish raw BA2 entries from compressed entries using `PackedSize` and metadata. | SATISFIED | Parser/extraction tests cover raw-vs-compressed classification. |
| GNRL-06 | 05-01 through 05-05 | Consumer can extract BA2 GNRL entries compressed with deflate. | SATISFIED | Deflate exact-size extraction tests pass. |
| GNRL-07 | 05-01 through 05-05 | Consumer can extract Starfield BA2 v3 raw LZ4 block entries when `CompressionMethod == 3`. | SATISFIED | Raw LZ4 block routing/extraction tests pass. |
| GNRL-08 | 05-01 through 05-05 | Consumer can inspect and preserve Starfield BA2 v2/v3 version-specific header fields. | SATISFIED | Public metadata tests compare manifest Starfield fields. |

No Phase 05 requirements are orphaned.

## Gap Closure

| Prior Gap | Closure Evidence | Status |
|---|---|---|
| BA2 GNRL open read from `FileTableOffset` to EOF and included payload bytes. | `05-06-SUMMARY.md` records first-payload-offset bounded parsing plus sparse 8 GiB-offset public reader regression coverage. | RESOLVED |

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Phase gap gate | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_bounded_open|ba2_gnrl" --output-on-failure` | Passed per `05-06-SUMMARY.md`. | PASS |
| Current milestone full static CTest | `ctest --preset windows-msvc-debug-static --output-on-failure` | Passed 245/245 runnable tests with 2 expected opt-in fixture skips. | PASS |
| TES5Edit boundary | `git -C TES5Edit status --short` | No output in current audit evidence. | PASS |

## Gaps Summary

No gaps remain.

---

_Verified: 2026-05-10T10:00:00Z_
_Verifier: Codex_
