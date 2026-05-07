---
phase: 10-ba2-writers
verified: 2026-05-07T11:22:13Z
status: gaps_found
score: 10/14 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Consumer can create Fallout 4 and Starfield BA2 DDS archives from DDS inputs with proper metadata analysis."
    status: failed
    reason: "DDS analysis is hard-coded to DXGI_FORMAT_BC1_UNORM and rejects other valid DDS/DX10 texture formats while public Phase 10 support is documented as BA2 DDS/DX10 writer support."
    artifacts:
      - path: "src/texture/dds_analysis.cpp"
        issue: "Lines 13, 67-68, and 121-123 restrict analysis to format 71 (BC1_UNORM) and return unsupported_format for all other valid DDS formats."
      - path: "README.md"
        issue: "BA2 writer support section does not disclose a BC1-only limitation."
    missing:
      - "Support the intended BA2 DDS/DX10 format set, or explicitly document/test a narrower supported-format policy that still satisfies the phase contract."
      - "Carry DirectXTex-derived metadata.format through planning for every supported writer format."
  - truth: "DDS chunks support raw, deflate, and Starfield v3 method-3 raw LZ4-block compression without fallback or corruption."
    status: failed
    reason: "Compressed DX10 chunks whose compressed size equals unpacked size are emitted with PackedSize == Size, which the BA2 reader treats as raw, causing extraction to return compressed bytes as image data."
    artifacts:
      - path: "src/ba2_writer.cpp"
        issue: "Lines 619-637 set chunk.packed_size to stored_size for compressed chunks without handling the BA2 DX10 PackedSize == Size raw marker ambiguity."
      - path: "src/ba2_reader.cpp"
        issue: "Lines 141-147 classify any DX10 chunk with packed_size == size as raw."
      - path: "tests/ba2_writer_tests.cpp"
        issue: "Compression tests assert packed_size > 0 and successful current fixtures, but do not force or cover the equal-size compressed chunk edge case."
    missing:
      - "Handle equal-size compressed DX10 chunks by storing them raw or returning a structured planning failure before table emission."
      - "Add focused coverage for the equal-size compressed DX10 decision path."
  - truth: "Documentation claims BA2 writer support only within implemented Phase 10 boundaries."
    status: failed
    reason: "README claims production BA2 DDS/DX10 writer support without naming the implemented BC1-only DDS limitation, so consumer-facing boundaries are broader than actual implementation."
    artifacts:
      - path: "README.md"
        issue: "Lines 180-186 describe production DDS/DX10 support, actual DDS-byte analysis, and compression routes without the BC1-only caveat present in code."
    missing:
      - "Align README with implementation limits or expand implementation so the documented DDS/DX10 support is true."
---

# Phase 10: BA2 Writers Verification Report

**Phase Goal:** Consumers can create Fallout 4 and Starfield BA2 GNRL and DDS archives with correct version-specific layout and texture chunking.
**Verified:** 2026-05-07T11:22:13Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

Phase 10 is **not fully achieved**. The GNRL writer path is well covered and wired, and the DDS/DX10 path exists, is public, and passes generated BC1 read-after-write tests. However, the code review's two critical findings are confirmed in the actual codebase and both affect WRT-03: DDS writer support rejects valid non-BC1 DDS/DX10 inputs, and compressed DX10 chunks can be emitted in a size state that the reader interprets as raw.

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can discover BA2 writing through a dedicated public header and variant-safe targets. | ✓ VERIFIED | `include/libbsa/ba2_writer.hpp` declares `ba2_write_target`, GNRL/DDS input types, plan structs, and plan/finalize functions; public-header leakage grep passed with no private dependency matches. |
| 2 | Consumer can create BA2 GNRL archives for FO4 v1/v7/v8 and Starfield v2/v3 from memory entries. | ✓ VERIFIED | `plan_ba2_gnrl_write` resolves all GNRL targets in `src/ba2_writer.cpp:119-135`; focused tests `plans and finalizes BA2 GNRL archives for every required version` passed. |
| 3 | Consumer can create BA2 GNRL archives from disk mappings and memory payloads with equivalent read-back. | ✓ VERIFIED | `plan_ba2_gnrl_write_from_disk` reads disk bytes during planning at `src/ba2_writer.cpp:487-514`; disk/memory GNRL equivalence tests passed. |
| 4 | GNRL entries support raw, deflate, and Starfield v3 method-3 raw LZ4-block compression. | ✓ VERIFIED | `resolve_gnrl_compression` and `stored_payload_for` route through explicit compression APIs at `src/ba2_writer.cpp:274-306`; GNRL codec tests passed. |
| 5 | Opt-in GNRL dedup shares identical post-policy stored payload offsets and disabled dedup emits distinct regions. | ✓ VERIFIED | GNRL dedup is implemented at `src/ba2_writer.cpp:426-445`; `BA2 GNRL dedup shares exact post-policy stored payloads` passed. |
| 6 | Consumer can create BA2 DDS/DX10 archives from memory/disk DDS inputs. | ✗ FAILED | The path exists and BC1 fixtures pass, but `src/texture/dds_analysis.cpp:13,67-68,121-123` rejects all non-BC1 DX10 formats, so the public DDS/DX10 writer claim is not true for valid BA2 texture inputs. |
| 7 | DDS inputs are analyzed during planning into libbsa-owned texture metadata and target-rule chunks. | ✗ FAILED | Analysis exists and is private, but metadata analysis is not proper for the DDS/DX10 writer scope because only `DXGI_FORMAT_BC1_UNORM` is accepted. |
| 8 | DDS chunking is automatic and derived from DDS analysis plus target rules. | ✓ VERIFIED | `analyze_dds` groups mip ranges in `src/texture/dds_analysis.cpp:137-156`; preview and read-back tests cover one-mip, multi-mip, cubemap, and array BC1 inputs. |
| 9 | DDS chunks support raw, deflate, and Starfield method-3 LZ4-block compression without fallback/corruption. | ✗ FAILED | `src/ba2_writer.cpp:619-637` can emit compressed chunks with `PackedSize == Size`; `src/ba2_reader.cpp:141-147` treats that exact state as raw. This is a confirmed corruption edge. |
| 10 | Malformed, unsupported, or unrepresentable DDS inputs fail structurally during planning with no partial plan. | ✓ VERIFIED | `BA2 DDS writer rejects malformed unsupported and unsafe inputs` passed; unsupported Starfield method, duplicate paths, malformed bytes, and sink failures are covered. |
| 11 | Written BA2 archives can be read back with matching paths, metadata, and payload bytes. | ✗ FAILED | Generated current fixtures pass, but the equal-size DX10 compressed chunk edge creates written archives that reopen as raw and extract compressed bytes; read-back truth does not hold generally for supported compressed DDS writes. |
| 12 | Public consumers can create/finalize/reopen/extract representative FO4 and Starfield GNRL/DDS archives using only public headers. | ✓ VERIFIED | `tests/public_header_smoke.cpp:112-171` uses public headers and APIs for BA2 GNRL/DDS smoke; focused CTest includes `libbsa.public_header_smoke` and passed. |
| 13 | Final gates prove focused tests, public-header leakage checks, stale-placeholder checks, and TES5Edit read-only boundary. | ✓ VERIFIED | Focused BA2 lane passed 66/66 locally; private-token grep and stale-placeholder grep had no matches; `git status --short TES5Edit` returned no output. |
| 14 | Requirements WRT-02 and WRT-03 are accounted for against implementation evidence. | ✓ VERIFIED | WRT-02 is satisfied by GNRL implementation evidence; WRT-03 is traced and fails on DDS format coverage and DX10 compressed-size ambiguity. |

**Score:** 10/14 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/ba2_writer.hpp` | Public BA2 writer target, option, input, preview, plan, and finalize declarations | ✓ VERIFIED | Exists, substantive, Doxygen documented, public-only includes; no DirectXTex/DXGI/Windows/libdeflate/LZ4/TES5Edit leakage found. |
| `src/ba2_writer.cpp` | Native GNRL and DX10 planning/finalization | ⚠️ SUBSTANTIVE WITH BLOCKER | Implements GNRL/DDS planners and finalizer, but DX10 compressed equal-size handling is missing at lines 619-637. |
| `src/texture/dds_analysis.hpp` | Private DDS analysis contract returning libbsa-owned metadata/chunks | ✓ VERIFIED | Exists and declares `detail::analyze_dds` plus libbsa-owned `analyzed_dds_texture`/chunk values. |
| `src/texture/dds_analysis.cpp` | DirectXTex-backed DDS analysis | ⚠️ SUBSTANTIVE WITH BLOCKER | Includes DirectXTex privately and performs chunking, but hard-codes BC1-only format support. |
| `tests/ba2_writer_tests.cpp` | Focused BA2 writer tests | ⚠️ PARTIAL | Substantive tests pass, but they only generate BC1 DDS data and do not cover equal-size compressed DX10 chunk ambiguity. |
| `tests/public_header_smoke.cpp` | Consumer-style public BA2 writer smoke | ✓ VERIFIED | Public-only BA2 GNRL/DDS write/reopen/extract smoke exists and passes for representative BC1 DDS inputs. |
| `README.md` | Phase 10 BA2 writer support documentation | ⚠️ PARTIAL | Documents support and deferred boundaries, but overclaims DDS/DX10 support by omitting the BC1-only limitation. |
| `.planning/phases/10-ba2-writers/10-VALIDATION.md` | Validation sign-off and gate evidence | ✓ VERIFIED | Records `nyquist_compliant: true`, focused/full gate results, and boundary checks. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `include/libbsa/ba2_writer.hpp` | `src/ba2_writer.cpp` | Public plan/finalize functions implemented | ✓ WIRED | All public functions have implementations: `plan_ba2_gnrl_write`, disk variant, `plan_ba2_dds_write`, disk variant, and `finalize_ba2_write`. |
| `src/ba2_writer.cpp` | `src/ba2_reader.cpp` | Emitted BTDX/GNRL/DX10 bytes reopened through `open_ba2` | ✓ WIRED | Tests use `open_ba2` and `extract_ba2_entry`; focused BA2 tests passed. Wiring exists, but DX10 edge corruption remains. |
| `src/ba2_writer.cpp` | `src/texture/dds_analysis.cpp` | `plan_ba2_dds_write` calls `detail::analyze_dds` during planning | ✓ WIRED | `normalize_dds_entries` calls `detail::analyze_dds` at `src/ba2_writer.cpp:248`. |
| `src/ba2_writer.cpp` | `src/compression.cpp` | `resolve_write_compression`, `resolve_payload_codec`, `compress_payload` | ✓ WIRED | GNRL and DDS storage helpers call codec routing before storing plan-owned bytes. |
| `CMakeLists.txt` | BA2 writer sources/tests | Explicit source and test target entries | ✓ WIRED | `src/ba2_writer.cpp`, `src/texture/dds_analysis.cpp`, public header, `libbsa_ba2_writer_tests`, and smoke target are explicitly wired. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `src/ba2_writer.cpp` GNRL planner | `normalized_entries`, `table_bytes`, `data_regions` | Caller memory/disk entries → normalize/read → compression routing → plan-owned regions | Yes | ✓ FLOWING |
| `src/ba2_writer.cpp` DDS planner | `normalized_dds_entry::texture`, `plan.dds.textures`, `data_regions` | Caller DDS bytes/disk files → `detail::analyze_dds` → chunk payloads → compression routing | Partially | ⚠️ FLOWING WITH FORMAT BLOCKER: real data flows for BC1 only; valid non-BC1 DDS inputs fail. |
| `finalize_ba2_write` | `plan.table_bytes`, `plan.data_regions[*].stored_payload` | Previously computed plan-owned bytes | Yes | ✓ FLOWING |
| `README.md` BA2 writer claims | Documentation content | Implementation and validation evidence | Partially | ⚠️ OVERCLAIM: docs do not reflect BC1-only implementation. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused BA2 writer/read/smoke tests pass | `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_reader_tests|libbsa_ba2_dds_reader_tests|libbsa.public_header_smoke"` | 66/66 tests passed locally | ✓ PASS |
| Public headers avoid private dependency leakage | `rg -n "DirectXTex|DXGI_FORMAT|Windows\.h|libdeflate|TES5Edit|lz4\.h|lz4frame\.h|LZ4_" include/libbsa` | No matches, expected exit path | ✓ PASS |
| Stale BA2 writer placeholders absent | `rg -n "BA2 (GNRL|DDS) writer planning is not implemented|BA2 writer finalization is not implemented" src/ba2_writer.cpp tests/ba2_writer_tests.cpp tests/public_header_smoke.cpp` | No matches, expected exit path | ✓ PASS |
| TES5Edit remains untouched | `git status --short TES5Edit` | Empty output | ✓ PASS |
| Full VS2026 fallback regression suite | Evidence from execution context and validation file | 176/176 CTest tests passed after execution; VS2022 preset unavailable locally | ✓ PASS (accepted fallback evidence) |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| WRT-02 | 10-01, 10-02, 10-03, 10-06 | Consumer can create Fallout 4 and Starfield BA2 GNRL archives with version-specific headers, file tables, offsets, and compression metadata. | ✓ SATISFIED | GNRL targets, disk/memory inputs, raw/deflate/LZ4 routing, dedup, invalid input failures, finalization, and read-after-write tests are implemented and passing. |
| WRT-03 | 10-01, 10-04, 10-05, 10-06 | Consumer can create Fallout 4 and Starfield BA2 DDS archives from DDS inputs with proper metadata analysis and mipmap chunking. | ✗ BLOCKED | DDS writer exists but only supports BC1 and has confirmed DX10 compressed equal-size corruption edge; this does not meet proper DDS/DX10 metadata/compression support. |

No orphaned Phase 10 requirements found in `.planning/REQUIREMENTS.md`; Phase 10 maps exactly WRT-02 and WRT-03.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/texture/dds_analysis.cpp` | 13, 67-68, 121-123 | Hard-coded `supported_bc1_unorm = 71U` as only writer-accepted DDS format | 🛑 Blocker | Valid BA2 DDS/DX10 formats such as BC3/BC5/BC7/R8G8B8A8 are rejected despite DDS/DX10 writer support claims. |
| `src/ba2_writer.cpp` | 619-637 | Compressed DX10 chunk `packed_size` emitted as `stored_size` without equal-size ambiguity handling | 🛑 Blocker | A compressed chunk whose stored size equals unpacked size is parsed as raw and extracts corrupt bytes. |
| `tests/ba2_writer_tests.cpp` | 28, 160, 530 | Tests generate/assert only BC1 DDS format | ⚠️ Warning | Test suite passes while missing valid non-BC1 writer behavior. |
| `tests/ba2_writer_tests.cpp` | 606-651 | DDS compression tests do not force equal-size compressed chunk edge | ⚠️ Warning | Test suite passes while missing the CR-01 corruption edge. |

### Human Verification Required

None. The blocking gaps are observable in source code and do not require visual/manual validation. External corpus comparison is explicitly Phase 11 scope and is not used to gate Phase 10 here.

### Gaps Summary

The BA2 GNRL writer portion satisfies WRT-02. The BA2 DDS writer portion does not yet satisfy WRT-03 because it overclaims general DDS/DX10 archive creation while accepting only BC1 DDS input, and because compressed DX10 chunk metadata can encode a state that the existing reader interprets as raw. These are not deferred Phase 11 compatibility-corpus issues; they are Phase 10 writer correctness gaps in the delivered code.

---

_Verified: 2026-05-07T11:22:13Z_
_Verifier: the agent (gsd-verifier)_
