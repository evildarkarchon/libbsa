---
phase: 10-ba2-writers
verified: 2026-05-07T12:00:45Z
status: gaps_found
score: 8/14 must-haves verified
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  previous_score: 10/14
  gaps_closed:
    - "DDS writer format support is no longer BC1-only; supported set is R8G8B8A8_UNORM, BC1_UNORM, BC3_UNORM, BC5_UNORM, and BC7_UNORM."
    - "DX10 compressed chunks that are not smaller now fall back to raw storage before native table emission."
    - "README now names the supported DDS writer format boundary and unsupported-format planning behavior."
  gaps_remaining:
    - "Native BA2 extension fields are still serialized with a leading dot."
    - "Multi-mip DDS arrays/cubemaps are still chunked mip-major while DDS payload layout is item-major."
    - "Cubemap arrays are accepted but serialized as a single-cube native field."
  regressions: []
gaps:
  - truth: "Consumer can create Fallout 4 and Starfield BA2 GNRL archives with version-specific headers, file tables, offsets, hashes, and compression metadata."
    status: failed
    reason: "Native BA2 GNRL records still write the four-byte extension field starting at the period (for example `.nif`) instead of extension text without the separator. This is native table layout corruption that libbsa read-back does not catch because the reader ignores the field."
    artifacts:
      - path: "src/ba2_writer.cpp"
        issue: "append_extension4 uses path.substr(dot, ...) at lines 103-109; both GNRL and DX10 serialization call this helper."
      - path: "tests/ba2_writer_tests.cpp"
        issue: "No raw-record assertion verifies extension bytes are `nif\0`/`dds\0` rather than `.nif`/`.dds`."
    missing:
      - "Change append_extension4 to start at dot + 1 and NUL-pad/truncate the extension text."
      - "Add native byte tests for GNRL and DX10 records covering 3-character and longer extensions."
  - truth: "Consumer can create Fallout 4 and Starfield BA2 DDS archives from DDS inputs with proper metadata analysis and mipmap chunking."
    status: failed
    reason: "DDS array/cubemap chunk payloads are built in mip-major order, but DDS arrays/cubemaps are item-major. Multi-mip arrays/cubemaps therefore round-trip through libbsa as valid-looking DDS with reordered image payload bytes."
    artifacts:
      - path: "src/texture/dds_analysis.cpp"
        issue: "Chunk payload construction loops chunk_mip outside item at lines 149-165, producing mip-major payload order for arrays/cubemaps."
      - path: "src/texture/dds_reconstruction.cpp"
        issue: "reconstruct_dds appends chunk bytes directly at line 112, so it cannot restore item-major ordering when chunks are mip-major."
      - path: "tests/ba2_writer_tests.cpp"
        issue: "Existing tests cover multi-mip single-item textures and single-mip arrays/cubemaps, but no multi-mip array/cubemap payload-order regression."
    missing:
      - "Either reject array_size > 1 with mip_count > 1 until safe reordering exists, or preserve/reconstruct per-item/per-mip chunk layout correctly."
      - "Add a multi-mip array or cubemap test with distinct bytes per item/mip and compare extracted DDS payload order against the source."
  - truth: "Malformed, unsupported, or unrepresentable DDS inputs fail structurally during planning with no partial plan."
    status: failed
    reason: "Cubemap arrays are accepted by DDS analysis, but BA2 DX10 record serialization always writes the native cubemap/array field as 6 for any cubemap, silently collapsing additional cubes."
    artifacts:
      - path: "src/texture/dds_analysis.cpp"
        issue: "DirectXTex cubemap metadata with arraySize 12, 18, etc. is accepted; there is no rejection for cubemap arrays at lines 141-148."
      - path: "src/ba2_writer.cpp"
        issue: "DX10 serialization writes `texture.is_cubemap ? 6U : texture.array_size` at line 692, discarding cube-array count."
      - path: "tests/ba2_writer_tests.cpp"
        issue: "No test proves cubemap arrays are either preserved or rejected structurally."
    missing:
      - "Reject cubemap DDS inputs where DirectXTex reports array_size != 6, or implement native cubemap-array encoding and matching read/extract tests."
  - truth: "Consumer can read back libbsa-written BA2 archives and retrieve matching paths, metadata, and payload bytes."
    status: failed
    reason: "Read-after-write tests pass for current fixtures, but the implementation still corrupts or loses data for accepted DDS inputs: multi-mip arrays/cubemaps reorder payload bytes, and cubemap arrays lose native cube count."
    artifacts:
      - path: "src/texture/dds_analysis.cpp"
        issue: "Mip-major chunk payload construction for arrays/cubemaps."
      - path: "src/ba2_writer.cpp"
        issue: "Cubemap native field hard-coded to 6."
    missing:
      - "Close the DDS ordering and cubemap-array gaps, then add read-after-write tests that compare source and extracted payload bytes for those cases."
  - truth: "Maintainer can validate BA2 DDS pack/extract behavior with mip, cubemap, DXGI format, chunking, and compression fixtures."
    status: failed
    reason: "The focused BA2 tests pass, but they do not validate the native extension field, multi-mip array/cubemap payload order, or cubemap-array behavior that the writer currently mishandles."
    artifacts:
      - path: "tests/ba2_writer_tests.cpp"
        issue: "Coverage includes supported DXGI formats and compression fallback, but omits the review-identified native byte and multi-mip array/cubemap cases."
    missing:
      - "Add fixture tests for native extension bytes, multi-mip array/cubemap payload preservation, and cubemap-array rejection or preservation."
human_verification: []
---

# Phase 10: BA2 Writers Verification Report

**Phase Goal:** Consumers can create Fallout 4 and Starfield BA2 GNRL and DDS archives with correct version-specific layout and texture chunking.
**Verified:** 2026-05-07T12:00:45Z
**Status:** gaps_found
**Re-verification:** Yes — after Plan 10-07 gap closure

## Goal Achievement

Phase 10 is **not yet achieved**. Plan 10-07 closed the original DDS format-support, DX10 equal-size compression, and README boundary gaps. However, the advisory code review identified additional blockers, and source inspection confirms they are real codebase gaps rather than review noise. The current tests pass because they miss native extension bytes and accepted DDS layouts that corrupt silently.

## Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can discover BA2 writing through a dedicated public header and variant-safe targets. | ✓ VERIFIED | `include/libbsa/ba2_writer.hpp` declares BA2 writer targets, input types, preview structs, planners, and finalizer with no public DirectXTex/DXGI/codec header leakage found by validation greps. |
| 2 | Consumer can create BA2 GNRL archives for FO4 v1/v7/v8 and Starfield v2/v3 from memory entries. | ✗ FAILED | GNRL records are emitted, but `append_extension4` writes `.nif`-style native extension fields starting at the dot (`src/ba2_writer.cpp:103-109`, used at line 473). This violates native table layout despite libbsa read-back passing. |
| 3 | Consumer can create BA2 GNRL archives from disk mappings and memory payloads with equivalent read-back. | ✓ VERIFIED | Disk GNRL planner validates archive paths before host reads and delegates to memory planning; focused tests for disk/memory equivalence passed. |
| 4 | GNRL entries support raw, deflate, and Starfield v3 method-3 raw LZ4-block compression. | ✓ VERIFIED | Compression routing is wired through writer codec helpers; focused GNRL compression tests passed. |
| 5 | Opt-in GNRL dedup shares identical post-policy stored payload offsets and disabled dedup emits distinct regions. | ✓ VERIFIED | Dedup logic is present in `src/ba2_writer.cpp`; focused GNRL dedup test passed. |
| 6 | Consumer can create BA2 DDS/DX10 archives from memory/disk DDS inputs. | ✗ FAILED | The five-format support gap is closed, but accepted multi-mip arrays/cubemaps and cubemap arrays are still not safely written. |
| 7 | DDS inputs are analyzed during planning into libbsa-owned texture metadata and target-rule chunks. | ✗ FAILED | Analysis is private and substantive, but `dds_analysis.cpp:149-165` builds array/cubemap chunk payloads in mip-major order, which is wrong for multi-mip DDS arrays/cubemaps. |
| 8 | DDS chunking is automatic and derived from DDS analysis plus target rules. | ✗ FAILED | Chunking exists but is incorrect for accepted multi-mip array/cubemap layouts; `reconstruct_dds` appends chunks directly and cannot restore item-major payload order. |
| 9 | DDS chunks support raw, deflate, and Starfield method-3 LZ4-block compression without fallback/corruption. | ✓ VERIFIED | Plan 10-07 added raw fallback when compressed DX10 chunks are not smaller (`src/ba2_writer.cpp:624-628`); focused compression tests passed. |
| 10 | Malformed, unsupported, or unrepresentable DDS inputs fail structurally during planning with no partial plan. | ✗ FAILED | Cubemap arrays are unrepresentable by the current serialization but are accepted; `src/ba2_writer.cpp:692` serializes every cubemap as native field `6`. |
| 11 | Written BA2 archives can be read back with matching paths, metadata, and payload bytes. | ✗ FAILED | Current fixtures pass, but accepted multi-mip array/cubemap and cubemap-array inputs do not preserve payload/metadata semantics. |
| 12 | Public consumers can create/finalize/reopen/extract representative FO4 and Starfield GNRL/DDS archives using only public headers. | ✓ VERIFIED | `libbsa.public_header_smoke` passed in the focused run. |
| 13 | Final gates prove focused tests, public-header leakage checks, stale-placeholder checks, and TES5Edit read-only boundary. | ✓ VERIFIED | User-provided post-merge build and 179/179 CTest pass evidence accepted; focused BA2 writer/DDS/smoke CTest rerun passed 40/40. |
| 14 | Requirements WRT-02 and WRT-03 are accounted for against implementation evidence. | ✓ VERIFIED | Both IDs are present in plan frontmatter and REQUIREMENTS.md. WRT-02 is blocked by native extension-field layout; WRT-03 is blocked by DDS ordering/cubemap-array gaps, but traceability is accounted for. |

**Score:** 8/14 truths verified

## Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/ba2_writer.hpp` | Public BA2 writer contracts | ✓ VERIFIED | Exists, substantive, public API exposes writer targets/plans/finalize without private dependency leakage. |
| `src/ba2_writer.cpp` | Native GNRL and DX10 planning/finalization | ✗ BLOCKER | Substantive and wired, but `append_extension4` includes the dot and DX10 cubemap serialization collapses all cubemaps to field `6`. |
| `src/texture/dds_analysis.cpp` | DirectXTex-backed DDS analysis | ✗ BLOCKER | Five-format allowlist exists, but chunk payload order is mip-major for arrays/cubemaps and cubemap arrays are not rejected. |
| `src/texture/dds_reconstruction.cpp` | DDS header reconstruction | ⚠️ PARTIAL | Five-format reconstruction exists; direct chunk append cannot fix analyzer's mip-major array/cubemap payload ordering. |
| `tests/ba2_writer_tests.cpp` | Focused BA2 writer tests | ⚠️ PARTIAL | 40/40 focused tests pass, but missing native extension-byte, multi-mip array/cubemap, and cubemap-array coverage. |
| `tests/public_header_smoke.cpp` | Consumer-style public BA2 writer smoke | ✓ VERIFIED | Public smoke passed. |
| `README.md` | Support boundaries | ✓ VERIFIED | Names supported DDS writer formats and unsupported-format behavior; no current overclaim on format set. |

## Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `include/libbsa/ba2_writer.hpp` | `src/ba2_writer.cpp` | Public plan/finalize functions implemented | ✓ WIRED | Public functions are implemented and linked. |
| `src/ba2_writer.cpp` | `src/ba2_reader.cpp` | Emitted bytes reopened through `open_ba2` | ⚠️ WIRED BUT PARTIAL | Wiring exists and tests pass for sampled fixtures; native extension field is ignored by libbsa reader, so this link masks an external compatibility gap. |
| `src/ba2_writer.cpp` | `src/texture/dds_analysis.cpp` | DDS planner calls `detail::analyze_dds` | ✓ WIRED | `normalize_dds_entries` calls analyzer before DX10 layout. |
| `src/ba2_writer.cpp` | `src/compression.cpp` | Compression routing | ✓ WIRED | Raw/deflate/LZ4 routes are tested; equal-or-larger DX10 compressed chunks now fall back to raw. |
| `tests/ba2_writer_tests.cpp` | `README.md` | Supported format set documented and tested | ✓ WIRED | Same five formats are present, though gsd-sdk pattern check failed due order (`README` lists R8G8B8A8 first). |

## Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `src/ba2_writer.cpp` GNRL planner | `normalized_entries`, `table_bytes`, `data_regions` | Caller memory/disk entries -> normalize/read -> compression -> plan regions | Partially | ⚠️ FLOWING WITH NATIVE FIELD BUG: payload bytes flow, but extension table bytes are wrong. |
| `src/ba2_writer.cpp` DDS planner | `entry.texture.chunks`, `plan.dds.textures`, `data_regions` | Caller DDS bytes -> DirectXTex analysis -> chunk payloads -> compression | Partially | ⚠️ FLOWING WITH LAYOUT BUG: valid data flows for covered simple fixtures, but multi-mip array/cubemap ordering and cubemap arrays are unsafe. |
| `finalize_ba2_write` | `plan.table_bytes`, `plan.data_regions[*].stored_payload` | Previously computed plan-owned bytes | Yes | ✓ FLOWING; warning remains that a default-constructed invalid plan finalizes as success. |

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused BA2 writer/DDS/smoke tests pass | `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_writer_tests|libbsa_ba2_dds_reader_tests|libbsa.public_header_smoke"` | 40/40 tests passed | ✓ PASS |
| Plan 10-07 artifact checks | `gsd-sdk query verify.artifacts .planning/phases/10-ba2-writers/10-07-PLAN.md` | 5/5 artifacts passed | ✓ PASS |
| Plan 10-07 key-link checks | `gsd-sdk query verify.key-links .planning/phases/10-ba2-writers/10-07-PLAN.md` | 2/3 strict patterns passed; README/test format-set mismatch was order-only | ⚠️ PASS WITH NOTE |
| Post-merge full suite | User-provided evidence: `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug` | 179/179 passed | ✓ PASS |

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| WRT-02 | 10-01, 10-02, 10-03, 10-06 | Consumer can create Fallout 4 and Starfield BA2 GNRL archives with version-specific headers, file tables, offsets, and compression metadata. | ✗ BLOCKED | GNRL planner is implemented and tests pass, but native extension field serialization is wrong (`.nif` instead of `nif\0`), so file table metadata is not correct. |
| WRT-03 | 10-01, 10-04, 10-05, 10-06, 10-07 | Consumer can create Fallout 4 and Starfield BA2 DDS archives from DDS inputs with proper metadata analysis and mipmap chunking. | ✗ BLOCKED | Five-format support and compression fallback are implemented, but accepted multi-mip arrays/cubemaps corrupt payload order and cubemap arrays are silently collapsed. |

No orphaned Phase 10 requirements found in `.planning/REQUIREMENTS.md`; Phase 10 maps exactly WRT-02 and WRT-03.

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/ba2_writer.cpp` | 103-109 | `path.substr(dot, ...)` in native extension writer | 🛑 Blocker | Produces invalid native extension fields in GNRL and DX10 records. |
| `src/texture/dds_analysis.cpp` | 149-165 | Mip-major loop around array item loop | 🛑 Blocker | Multi-mip arrays/cubemaps reorder image payload bytes. |
| `src/ba2_writer.cpp` | 692 | `texture.is_cubemap ? 6U : texture.array_size` | 🛑 Blocker | Cubemap arrays accepted from DDS analysis lose additional cube count. |
| `src/ba2_writer.cpp` | 736-748 | Default-constructed invalid plan finalizes as success | ⚠️ Warning | Public mutable plans can produce empty/non-BA2 output without error; review WR-01 is real but secondary to WRT-02/WRT-03 blockers. |

## Human Verification Required

None. The blocking gaps are observable in source code and missing coverage, not visual/manual behavior.

## Gaps Summary

Plan 10-07 successfully closed the previously recorded BC1-only, DX10 equal-size compression, and README boundary gaps. Phase 10 still cannot pass because the writer emits wrong native extension bytes, corrupts accepted multi-mip array/cubemap DDS payload ordering, and silently collapses cubemap arrays. These are Phase 10 writer correctness issues, not deferred Phase 11 external corpus comparison.

---

_Verified: 2026-05-07T12:00:45Z_
_Verifier: the agent (gsd-verifier)_
