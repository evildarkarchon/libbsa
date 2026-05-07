# Phase 08 Security Audit: Writer Planning, Streaming Emit, and Dedup Core

**Verdict:** SECURED  
**Phase:** 08 — writer-planning-streaming-emit-and-dedup-core  
**Threats Closed:** 18/18  
**Threats Open:** 0  
**ASVS Level:** Not declared in phase artifacts  
**Audit Date:** 2026-05-06

## Scope and Method

This audit verified the declared mitigations in `08-01-PLAN.md` through `08-06-PLAN.md` against implementation and test code. Implementation files were treated as read-only; only this `SECURITY.md` was created.

Project-local skill directories `.claude/skills/` and `.agents/skills/` were not present. No `<config>` block and no `## Threat Flags` sections were present in the Phase 08 summaries.

## Verification Evidence

- Focused writer/smoke tests passed: `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_writer_tests|libbsa.public_header_smoke"` — 19/19 passed.
- TES5Edit immutability check passed: `git status --short -- "TES5Edit"` — no output.
- Public writer boundary check passed: `rg -n "DirectXTex|DXGI_FORMAT|Windows\.h|libdeflate|TES5Edit|lz4\.h|lz4frame\.h|LZ4_" "include/libbsa/writer.hpp" "tests/public_header_smoke.cpp"` — no output.
- Harness production-leakage check passed: `rg -n "writer_harness" "include/libbsa" "src"` — no output.
- CMake glob check passed: `rg -n "GLOB|GLOB_RECURSE" "CMakeLists.txt"` — no output.

## Threat Verification

| Threat ID | Category | Disposition | Status | Evidence |
|-----------|----------|-------------|--------|----------|
| T-08-01 | Tampering | mitigate | CLOSED | `include/libbsa/writer.hpp:3-13` includes only libbsa/public standard headers; `writer_target` is a libbsa-owned value type at `include/libbsa/writer.hpp:17-29`; forbidden private-token grep had no matches. |
| T-08-02 | Denial of Service | mitigate | CLOSED | Placeholder-only behavior was replaced by structured validation: unsupported targets return `unsupported_format` at `src/writer.cpp:317-319`, layout overflow returns `malformed_archive` at `src/writer.cpp:21-23`, and focused writer tests passed. |
| T-08-03 | Information Disclosure | mitigate | CLOSED | Public preview exposes numeric region IDs and owned bytes only: `include/libbsa/writer.hpp:61-73`, `include/libbsa/writer.hpp:76-88`; no public content digest or implementation pointer fields are present. |
| T-08-04 | Tampering | mitigate | CLOSED | Paths are normalized with `normalize_archive_path` at `src/writer.cpp:138`; duplicate normalized strings are rejected before layout at `src/writer.cpp:144-145`; tests cover duplicate rejection at `tests/writer_core_tests.cpp:422-433`. |
| T-08-05 | Denial of Service | mitigate | CLOSED | Checked arithmetic helpers exist at `src/writer.cpp:46-62`; layout uses them for entry-table size, data-region table size, offsets, and payload cursor at `src/writer.cpp:340-407`; overflow test at `tests/writer_core_tests.cpp:477-490`. |
| T-08-06 | Tampering | mitigate | CLOSED | Planning routes compression through `resolve_write_compression`, `resolve_payload_codec`, and `compress_payload` at `src/writer.cpp:159-180`; Starfield method routing prevents fallback at `src/writer.cpp:86-94`; unsupported route test at `tests/writer_core_tests.cpp:407-420`. |
| T-08-07 | Tampering | mitigate | CLOSED | Dedup grouping compares exact post-policy `stored_payload` bytes after compression/storage (`src/writer.cpp:330-338`, `src/writer.cpp:361-376`); paths only determine normalized sorted entry identity. |
| T-08-08 | Information Disclosure | mitigate | CLOSED | Region IDs are numeric (`include/libbsa/writer.hpp:67-73`, `include/libbsa/writer.hpp:81-88`); no `hash`, `content_hash`, or `payload_hash` field appears in the public writer API. |
| T-08-09 | Denial of Service | mitigate | CLOSED | Dedup requests on unsupported targets fail before planning work at `src/writer.cpp:321-323`; test asserts exact `unsupported_format` message at `tests/writer_core_tests.cpp:264-279`. |
| T-08-10 | Repudiation | mitigate | CLOSED | `write_chunk` returns the first `sink.write` error unchanged at `src/writer.cpp:230-235`; finalization propagates write failures at `src/writer.cpp:447-473`; injected sink test at `tests/writer_core_tests.cpp:45-57` and `tests/writer_core_tests.cpp:332-345`. |
| T-08-11 | Tampering | mitigate | CLOSED | `finalize_archive_write` emits prebuilt header/table chunks and `planned_data_region::stored_payload` at `src/writer.cpp:438-477`; compression/normalization calls are confined to planning (`src/writer.cpp:138`, `src/writer.cpp:159-180`) and absent from finalization. |
| T-08-12 | Denial of Service | mitigate | CLOSED | Public finalization returns `result<void>` to a caller-owned `byte_sink` (`include/libbsa/writer.hpp:114-120`); implementation writes header/table chunks and each payload via `sink.write` rather than returning a whole archive image (`src/writer.cpp:447-471`). |
| T-08-13 | Tampering | mitigate | CLOSED | Test harness validates parsed metadata against the write plan in `require_harness_matches_plan` at `tests/writer_harness_helpers.cpp:147-177`; read-back test invokes it at `tests/writer_core_tests.cpp:347-365`. |
| T-08-14 | Information Disclosure | mitigate | CLOSED | Harness declarations are under `tests/writer_harness_helpers.hpp:12-80` in `libbsa::test`; production leakage grep for `writer_harness` under `include/libbsa` and `src` had no matches. |
| T-08-15 | Denial of Service | mitigate | CLOSED | Harness extraction uses production codec dispatch and decompression (`tests/writer_harness_helpers.cpp:136-143`); unsupported codec route has no fallback and is tested at `tests/writer_core_tests.cpp:407-420`. |
| T-08-16 | Information Disclosure | mitigate | CLOSED | Public smoke includes and uses `<libbsa/writer.hpp>` at `tests/public_header_smoke.cpp:10` and `tests/public_header_smoke.cpp:99-121`; forbidden private-token grep over `include/libbsa/writer.hpp` and smoke code had no matches. |
| T-08-17 | Repudiation | mitigate | CLOSED | Summary records focused/full validation and boundary gates at `08-06-SUMMARY.md:94-105`; independent focused writer/smoke rerun passed 19/19 during this audit. |
| T-08-18 | Tampering | mitigate | CLOSED | Phase summaries recorded no TES5Edit changes (`08-06-SUMMARY.md:104`, `08-06-SUMMARY.md:120`); independent `git status --short -- "TES5Edit"` returned no output. |

## Findings by Severity

### BLOCKER

None.

### WARNING

None. No unregistered threat flags were declared in Phase 08 summaries.

### INFO

- Plan 08-05 changed production codec routing in `src/writer.cpp` to prevent unsupported Starfield compression-method fallback; this maps to declared threats T-08-06 and T-08-15.
- Plan 08-03 rewrote public comments to avoid content-digest terminology; this maps to declared threats T-08-08 and T-08-16.

## Remediation

No remediation required for the declared Phase 08 threat model. Re-run this audit after any changes to writer planning, finalization, harness helpers, public headers, CMake source wiring, or TES5Edit submodule state.
