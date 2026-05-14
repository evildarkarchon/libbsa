# Phase 16: Parser and Preparer Seam Extraction - Research

**Researched:** 2026-05-14
**Domain:** Internal C++20 parser/preparer refactor with fixture-backed regression and source-policy guardrails
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
Phase 16 is an internal, behavior-preserving maintainability refactor. It extracts smaller private seams from the targeted TES4-family BSA parser hotspot and the BA2 DX10 writer preparer/staging hotspot while keeping the public API, supported archive behavior, existing fixture semantics, Phase 13 host-file boundary, and Phase 15 reader facade/backend separation intact.

**5 requirements are locked.** See `16-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `16-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Internal seam extraction for the targeted TES4-family BSA parser hotspot in `src/formats/bsa/tes4_bsa_parser.cpp` or adjacent private/internal format files.
- Internal seam extraction for the targeted BA2 DX10 writer preparer/staging hotspot in `src/formats/ba2/ba2_dx10_prepare.cpp` or adjacent private/internal format files.
- Focused runtime regression coverage for TES4 parser behavior that is already supported by current generated fixtures and malformed cases.
- Focused runtime regression coverage for BA2 DX10 preparation behavior, including snapshot-backed staging and planned chunk preparation.
- Source/policy guardrails or equivalent committed evidence that the targeted hotspot responsibilities remain split into smaller maintainable seams.
- Preservation of current public reader/writer APIs, current result/error-code behavior, and current fixture-backed parser/preparer behavior.

**Out of scope (from SPEC.md):**
- Public API redesign or new public reader/writer types - v1.1 is hardening-only and this phase changes internal seams, not consumer surface area.
- Full TES4 parser rewrite or broad generic parser framework - the phase targets smaller maintainable seams, not a new architecture for all parsers.
- Full BA2 writer redesign or replacement of the DX10 writer pipeline - the phase targets preparer/staging seams, not a writer architecture reset.
- TES4 or BA2 GNRL dedupe optimization - this belongs to Phase 17 writer hotspot hardening.
- BA2 DX10 temporary-data lifecycle cleanup proof or abnormal-termination risk documentation - this belongs to Phase 17 ship-gate scope, although Phase 16 may create seams that make that later work safer.
- New archive family support, new compression formats, or new DDS feature support - v1.1 does not expand product capability.
- Editing, formatting, compiling, staging, or committing files under `TES5Edit/` - the submodule remains read-only reference material.
- Cross-platform portability work - libbsa remains Windows-only.

### the agent's Discretion
None. The discussion locked seam shape, ownership split, proof balance, verification expectation, and structural guardrail style closely enough that downstream research and planning should not reopen them.

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within Phase 16 scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| REFA-01 | Maintainer can modify the targeted TES4 parser hotspot through smaller internal helpers with focused regression coverage. | Target `tes4_bsa_parser.cpp` table parsing and payload descriptor responsibilities, add focused internal seam tests, and preserve existing reader/malformed suites. [VERIFIED: `.planning/REQUIREMENTS.md`:29] [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:493-586] [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`:218-468] |
| REFA-02 | Maintainer can modify the targeted BA2 DX10 preparer/staging hotspot through smaller internal helpers with focused regression coverage. | Target `ba2_dx10_prepare.cpp` snapshot builder and plan-then-assemble chunk preparation seams, extend writer-stage and DX10 writer tests, and preserve compression routing. [VERIFIED: `.planning/REQUIREMENTS.md`:30] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:297-561] [VERIFIED: `tests/unit/writer_stage_tests.cpp`:444-548] |
</phase_requirements>

## Summary

Phase 16 should be planned as a narrow internal refactor, not a feature phase: keep public reader/writer headers stable, keep current parser/preparer entrypoints stable, and split only the locked TES4 parser and BA2 DX10 preparer hotspots into private/internal seams. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:19-42] The current TES4 hotspot already has separable file-local helpers for header/table parsing, folder/file names, entry materialization, embedded-name sizing, raw-size calculation, and payload-span checks, so the safest plan is to lift these into adjacent private modules without inventing a generic parser framework. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:70-398] [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:111-113]

The BA2 DX10 hotspot mixes DDS host-file loading, DirectXTex analysis handoff, snapshot temp-file staging, chunk snapshot collection, compression routing, prepared-entry construction, indexed parallel chunk work, and canonical sorting in one implementation file. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:78-561] The plan should extract a snapshot-builder seam and a plan-then-assemble chunk seam while preserving `detail::run_indexed_work`, streaming snapshot reads through host-file helpers, and sorting only after entry preparation. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:42-47] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:222-229,517-559]

**Primary recommendation:** Plan three waves: TES4 table/payload seams plus focused tests, BA2 DX10 snapshot/chunk seams plus focused tests, then a dedicated source-policy guard and required debug/ASan verification sweep. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-58] [VERIFIED: `CMakePresets.json`:62-73]

## Project Constraints (from AGENTS.md)

- libbsa is a Windows-only C++20 library; do not add Linux, macOS, POSIX, or cross-platform portability work. [VERIFIED: `AGENTS.md`:16-18] [VERIFIED: `AGENTS.md`:110-114]
- `TES5Edit/` is read-only reference material; do not edit, format, stage, commit, compile, or vendor it. [VERIFIED: `AGENTS.md`:20-33]
- Preserve BSArchPro-compatible behavior discovered from reference code unless a documented reason to diverge exists, but implement clean idiomatic C++ outside the submodule. [VERIFIED: `AGENTS.md`:35-41]
- Do not introduce speculative dependencies; required dependencies remain `libdeflate`, official `lz4`, `DirectXTex`, and vcpkg. [VERIFIED: `AGENTS.md`:43-53]
- Never delete accurate comments as cleanup; add comments for non-obvious compatibility, lifetime, error-handling, threading, and deviation constraints. [VERIFIED: `AGENTS.md`:55-61]
- Add Doxygen-compliant C++ doc comments for public APIs and methods that are added or substantially rewritten; trivial private helpers may omit doc comments. [VERIFIED: `AGENTS.md`:59-61]
- Add focused fixture-based tests for parsing, writing, round-tripping, and compatibility behavior; do not use `TES5Edit/` as a mutable fixture. [VERIFIED: `AGENTS.md`:63-68]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| TES4 metadata table parsing seam | Format pipeline layer (`src/formats/bsa/`) | Shared detail parser primitives | TES4 header, folder records, folder blocks, and filename tables are format-specific, while overflow/span/count helpers come from `detail`. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`:77-89] [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:102-255] |
| TES4 payload descriptor seam | Format pipeline layer (`src/formats/bsa/`) | Shared detail host-file/byte readers | Embedded-name prefixes, raw-size prefixes, compression interpretation, and payload spans depend on TES4 records plus payload-prefix reads. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:257-385] |
| BA2 DX10 snapshot builder | Format pipeline layer (`src/formats/ba2/`) | Texture adapter + shared host-file services | DDS bytes are loaded through host-file helpers, analyzed through the texture adapter, target-validated, and staged as writer-owned subresource snapshots. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:62-84,297-337] [VERIFIED: `.planning/codebase/ARCHITECTURE.md`:91-97] |
| BA2 DX10 chunk assembly/compression | Format pipeline layer (`src/formats/ba2/`) | Shared compression router + parallel work | Planned chunks are assembled from snapshot files, compressed by target method, and coordinated through indexed worker execution. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:206-229,399-461,517-526] |
| Regression proof and policy guardrails | Test layer (`tests/unit/`) | CTest labels | Current repo uses Catch2 suites plus source-reading policy tests for structural invariants. [VERIFIED: `.planning/codebase/TESTING.md`:21-31,202-207] [VERIFIED: `tests/unit/archive_reader_dispatch_policy_tests.cpp`:46-103] |

## Standard Stack

### Core

| Library / Tool | Version / Policy | Purpose | Why Standard |
|----------------|------------------|---------|--------------|
| C++ | C++20 via CMake target features and presets | Internal seam extraction and tests | The project target and presets require C++20; public API must not expose C++23-only constructs. [VERIFIED: `CMakeLists.txt`:65] [VERIFIED: `CMakePresets.json`:15-16] [VERIFIED: `AGENTS.md`:110-118] |
| CMake | Minimum 4.0, local tool 4.3.2 | Build registration for new private modules and tests | The root project requires CMake 4.0 and the local environment has CMake/CTest 4.3.2. [VERIFIED: `CMakeLists.txt`:1] [VERIFIED: bash `cmake --version`] |
| Catch2 + CTest | Catch2 from vcpkg manifest; CTest via CMake | Focused runtime and policy tests | Existing tests link `Catch2::Catch2WithMain`, and `catch_discover_tests` maps tags to CTest labels. [VERIFIED: `vcpkg.json`:14] [VERIFIED: `tests/CMakeLists.txt`:1,65-123,297-302] [CITED: Context7 `/catchorg/catch2` cmake-integration.md] |
| vcpkg manifest mode | Existing `vcpkg.json` | Dependency acquisition | Current manifest already declares libdeflate, lz4, DirectXTex, Catch2, and nlohmann-json; Phase 16 should add no dependency. [VERIFIED: `vcpkg.json`:4-16] [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:66-72] |

### Supporting

| Library / Tool | Version / Policy | Purpose | When to Use |
|----------------|------------------|---------|-------------|
| libdeflate | Existing vcpkg dependency | BA2 DX10 Fallout 4 and Starfield deflate chunk compression through compression router | Preserve current compression routing; do not call libdeflate directly from new seams unless existing router cannot serve the case. [VERIFIED: `vcpkg.json`:5-11] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:437-441] |
| lz4 | Existing vcpkg dependency | Starfield v3 raw LZ4 block compression through compression router | Preserve `ba2_starfield_compression_lz4_block` routing for covered Starfield v3 cases. [VERIFIED: `vcpkg.json`:12] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:206-217] |
| DirectXTex | Existing vcpkg dependency behind texture adapter | DDS analysis for BA2 DX10 writer entry creation | Keep DirectXTex inside `src/texture/directxtex_analyzer.cpp` and pass libbsa-native texture metadata/snapshots across format seams. [VERIFIED: `vcpkg.json`:13] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:11-12,312-324] |
| nlohmann-json | Existing test dependency | Manifest-driven fixture tests | Use only where existing tests already parse generated fixture manifests. [VERIFIED: `vcpkg.json`:15] [VERIFIED: `.planning/codebase/TESTING.md`:122-140] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Adjacent private TES4 parser modules | Generic all-format parser framework | Out of scope; Phase 16 targets one TES4 hotspot and forbids broad parser redesign. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:54-58] |
| Snapshot-builder + chunk-assembler seams | Full BA2 writer pipeline redesign | Out of scope; existing `ba2_dx10_prepare.hpp` contracts must remain stable. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:42-47,90-98] |
| Source-policy test with negative invariants | Exact helper-name policy test | Locked decision requires role-based guardrails that do not freeze exact helper names. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58] |

**Installation:** no package installation should be planned for Phase 16; dependencies are already present in `vcpkg.json`. [VERIFIED: `vcpkg.json`:4-16]

**Version verification:** The local tools required for planning/build/test are available: CMake 4.3.2, CTest 4.3.2, Git 2.54.0.windows.1, Node v25.9.0, and `VCPKG_ROOT=C:\vcpkg`. [VERIFIED: bash environment audit]

## Architecture Patterns

### System Architecture Diagram

```text
TES4 archive bytes / resolved host_file_path
        |
        v
TES4 public/internal parser entrypoints (unchanged)
        |
        v
[NEW private raw table seam]
  - header + version/count/table-size checks
  - folder records / folder blocks
  - filename table strings
        |
        v
[NEW private payload descriptor seam]
  - compression interpretation
  - embedded-name prefix size
  - raw-size prefix read
  - payload span vs metadata checks
        |
        v
entry_metadata vector + archive_metadata (unchanged public behavior)

BA2 DX10 add_file/write flow
        |
        v
ba2_dx10_make_writer_entry / prepare_entries (unchanged internal surface)
        |
        v
[NEW private snapshot-builder seam]
  - resolve/read DDS host file
  - DirectXTex-backed texture analysis handoff
  - target format validation
  - subresource snapshot file writes
        |
        v
[NEW plan-then-assemble chunk seam]
  - texture::plan_dx10_chunks
  - stream snapshot files into one chunk buffer
  - compression_method_for target/options
  - detail::run_indexed_work results by planned index
        |
        v
prepared DX10 entries sorted by canonical path (unchanged writer behavior)
```

### Recommended Project Structure

```text
src/
├── formats/bsa/
│   ├── tes4_bsa_parser.cpp              # thin coordinator + public/internal entrypoints [VERIFIED]
│   ├── tes4_bsa_table.*                 # recommended private raw table bundle seam [RECOMMENDED from locked D-01/D-02]
│   └── tes4_bsa_payload_descriptor.*    # recommended private payload descriptor seam [RECOMMENDED from locked D-03]
├── formats/ba2/
│   ├── ba2_dx10_prepare.cpp             # thin coordinator + existing exported internal functions [VERIFIED]
│   ├── ba2_dx10_snapshot_builder.*      # recommended private snapshot builder seam [RECOMMENDED from locked D-05]
│   └── ba2_dx10_chunk_assembler.*       # recommended plan/assemble/compress seam [RECOMMENDED from locked D-06]
└── texture/
    └── directxtex_analyzer.cpp          # unchanged DirectXTex boundary [VERIFIED]

tests/unit/
├── tes4_bsa_parser_seam_tests.cpp       # direct internal seam coverage [RECOMMENDED]
├── ba2_dx10_preparer_seam_tests.cpp     # direct snapshot/chunk seam coverage [RECOMMENDED]
└── parser_preparer_seam_policy_tests.cpp # role-based source guard [RECOMMENDED]
```

### Pattern 1: Keep Public/Internal Entrypoints Stable, Move Implementation Behind Private Seams

**What:** Preserve `parse_tes4_bsa_archive`, `parse_tes4_bsa_archive_file`, `parse_tes4_bsa_metadata`, `ba2_dx10_make_writer_entry`, `ba2_dx10_validate_entries`, `ba2_dx10_prepare_chunk`, and `ba2_dx10_prepare_entries` signatures. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.hpp`:24-32] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.hpp`:55-84]

**When to use:** Use for all Phase 16 code changes so existing callers and tests continue to compile unchanged. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:66-72]

**Example:**
```cpp
// Source: `src/formats/ba2/ba2_dx10_prepare.hpp` lines 74-84.
result<ba2_dx10_prepared_chunk> ba2_dx10_prepare_chunk(
    ba2_dx10_target target,
    const ba2_dx10_writer_options& options,
    const ba2_dx10_writer_entry& source,
    const texture::planned_texture_chunk& planned);
```

### Pattern 2: Direct Internal Seam Tests May Include Private Headers

**What:** Add internal headers under `src/formats/...` and include them from `tests/unit/*_seam_tests.cpp`; do not add public headers under `include/libbsa/`. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:36-41] [VERIFIED: `tests/CMakeLists.txt`:125-129]

**When to use:** Use for raw table bundles, payload descriptors, snapshot builders, and chunk assembly units that need direct regression coverage. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-52]

**Example:**
```cpp
// Source pattern: `tests/unit/writer_stage_tests.cpp` lines 1-10 include internal headers.
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "texture/dds_layout.hpp"
```

### Pattern 3: Role-Based Source Policy Guard

**What:** Read source files in a Catch2 policy test and assert negative invariants or separated-role evidence instead of exact helper names. [VERIFIED: `tests/unit/archive_reader_dispatch_policy_tests.cpp`:16-42,46-103] [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58]

**When to use:** Use after runtime tests exist, so the policy guard prevents collapse but does not substitute for behavior proof. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-58]

**Example:**
```cpp
// Source pattern: `tests/unit/archive_reader_dispatch_policy_tests.cpp` lines 37-42.
void require_absent_tokens(std::string_view body, std::span<const std::string_view> forbidden_tokens) {
  for (const auto token : forbidden_tokens) {
    INFO("forbidden token: " << token);
    REQUIRE(body.find(token) == std::string_view::npos);
  }
}
```

### Anti-Patterns to Avoid

- **Public API expansion:** Do not add Phase 16 public reader/writer types or signatures under `include/libbsa/`. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:54-56,74-84]
- **Generic parser/preparer framework:** Do not replace targeted seams with a broad all-format architecture. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:54-58]
- **Whole-snapshot memory assembly beyond the current chunk buffer:** Chunk assembly must stream staged snapshot bytes through host-file chunk helpers and size checks, preserving current bounded-memory behavior. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:45-47] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:222-229]
- **Phase 17 scope creep:** Do not claim TES4/BA2 dedupe optimization or BA2 DX10 temp lifecycle cleanup as complete. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:58-60,84]

## Current Hotspot Inventory

| Hotspot | Responsibilities Present Today | Likely Seam(s) | Tests to Reuse/Extend |
|---------|--------------------------------|----------------|------------------------|
| `src/formats/bsa/tes4_bsa_parser.cpp` | Header read, table-size math, folder records, folder blocks, file names, table validation, entry metadata materialization, embedded-name prefix sizing, raw-size prefix read, hash validation, duplicate canonical path detection, payload span checks. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:70-398] | Raw table bundle seam; payload descriptor helper; thin coordinator. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:36-40] | `tes4_bsa_reader_tests.cpp` plus new direct seam tests. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`:218-468] |
| `src/formats/ba2/ba2_dx10_prepare.cpp` | DDS read, snapshot dir/file management, DirectXTex analysis handoff, target validation, chunk snapshot collection, raw chunk assembly, compression routing, entry preparation, indexed worker scheduling, canonical sorting. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:78-561] | Snapshot builder; chunk plan/assembler/compressor; thin entry coordinator. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:42-47] | `writer_stage_tests.cpp`, `ba2_dx10_writer_tests.cpp`, and new direct preparer seam tests. [VERIFIED: `tests/unit/writer_stage_tests.cpp`:444-548] [VERIFIED: `tests/unit/ba2_dx10_writer_tests.cpp`:723-827] |

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Compression routing | Direct libdeflate/lz4 calls from extracted seams | `detail::compress_payload` / existing `detail::compression_method` | Current BA2 DX10 preparer routes target/options to compression method before using the shared router. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:206-217,437-441] |
| DDS parsing | Custom DDS parser in BA2 seam | `texture::analyze_dds_source` and `texture::plan_dx10_chunks` | Current texture adapter owns DDS analysis and chunk planning; public/format seams consume libbsa-native metadata. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:312-324,470-477] [VERIFIED: `.planning/codebase/ARCHITECTURE.md`:91-97] |
| Host path conversion/opening | Raw `std::ifstream` from public UTF-8 strings in new seams | `detail::resolve_host_file_path`, `detail::read_host_file_exact`, `detail::for_each_host_file_chunk`, `detail::open_host_file` | Phase 13 locked resolved host-file boundaries and current BA2/TES4 code already uses shared host-file helpers. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:68] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:78-84,222-229] [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:617-662] |
| Worker scheduling | New scheduler abstraction | `detail::run_indexed_work` | Locked decision preserves indexed parallel chunk work and result-by-planned-index behavior. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:45-47] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:517-526] |
| Policy enforcement framework | New parser/policy DSL | Catch2 source-reading policy test | Existing repo policy tests read files directly and assert tokens/structure. [VERIFIED: `tests/unit/archive_reader_dispatch_policy_tests.cpp`:16-103] [VERIFIED: `tests/unit/validation_policy_tests.cpp`:20-31,219-224] |

**Key insight:** This phase is safer if it extracts existing responsibilities into explicit private seams and proves unchanged behavior; new libraries, new public API, new schedulers, and new format frameworks would increase risk without satisfying REFA-01/REFA-02 better. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:44-84]

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|-----------------|
| Stored data | None — Phase 16 changes internal parsing/preparation code and fixture tests; no database/datastore-backed identifiers are involved. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:44-63] | None. |
| Live service config | None — project is a local C++ library with filesystem-only runtime boundaries and no service configuration for this phase. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`:202-206] | None. |
| OS-registered state | None — no Windows service/task registration is part of parser/preparer seams. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:44-63] | None. |
| Secrets/env vars | `VCPKG_ROOT` is used by CMake presets; no secret or env var name needs renaming. [VERIFIED: `CMakePresets.json`:17,30,43,56,69] | Keep preset behavior unchanged. |
| Build artifacts | New `.cpp`/test files must be registered in `CMakeLists.txt` / `tests/CMakeLists.txt`; existing build artifacts under `build/` are regenerated by normal configure/build. [VERIFIED: `CMakeLists.txt`:112-173] [VERIFIED: `tests/CMakeLists.txt`:65-113] | Add sources/tests to CMake; rebuild. |

## Common Pitfalls

### Pitfall 1: Extracting names but not responsibilities
**What goes wrong:** New helper names are created, but `parse_tes4_bsa_archive_impl` or `ba2_dx10_prepare.cpp` still owns most mixed decisions. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58]
**Why it happens:** File-local helpers already exist, so a shallow move can look like seam extraction without changing review boundaries. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:70-398]
**How to avoid:** Plan source-policy evidence around responsibilities: raw table bundle, payload descriptor, snapshot builder, and chunk assembler/compressor. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:36-47]
**Warning signs:** Policy test only checks helper names or line counts, not separated roles. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58]

### Pitfall 2: Regressing TES4 stable error codes while moving checks
**What goes wrong:** Boundary checks move earlier/later and return different `error_code` values for malformed fixtures. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`:218-390]
**Why it happens:** Table sizing, record-count validation, hash validation, duplicate path rejection, and payload-span validation are currently interleaved. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:493-586]
**How to avoid:** Add direct seam tests plus keep the public malformed suite in the phase gate. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-52]
**Warning signs:** New table seam returns normalized entries or does duplicate path checks, violating the locked raw table bundle shape. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:36-40]

### Pitfall 3: Losing BA2 DX10 snapshot immutability or bounded-memory behavior
**What goes wrong:** Snapshot builder or chunk assembler loads too much into memory, reads changed source files after `add_file`, or publishes after pre-publish snapshot failure. [VERIFIED: `tests/unit/ba2_dx10_writer_tests.cpp`:723-827] [VERIFIED: `tests/unit/writer_stage_tests.cpp`:499-521]
**Why it happens:** DDS source loading, snapshot writing, chunk assembly, and publish failure behavior currently touch adjacent code paths. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:297-337,399-461] [VERIFIED: `src/formats/ba2/ba2_dx10_writer.cpp`:64]
**How to avoid:** Keep snapshot files as the writer-owned source of truth after `add_file`, assemble chunks by streaming snapshot bytes, and run writer-stage plus public writer tests. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:43-47]
**Warning signs:** New chunk seam accepts source DDS path or complete DDS bytes instead of snapshot handles and planned chunks. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:42-47]

### Pitfall 4: Reopening Phase 17 work inside Phase 16
**What goes wrong:** Planner adds dedupe performance work or temp lifecycle cleanup proof to seam extraction tasks. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:58-60]
**Why it happens:** BA2 DX10 staging fragility and dedupe bottlenecks are adjacent to the touched files. [VERIFIED: `.planning/codebase/CONCERNS.md`:49-65]
**How to avoid:** Mark dedupe and temp lifecycle tasks as explicitly deferred and verify Phase 17 requirements remain pending. [VERIFIED: `.planning/REQUIREMENTS.md`:32-40,84-87]
**Warning signs:** Plan claims DX10 abnormal-termination cleanup or dedupe optimization as success evidence. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:24-30]

## Code Examples

### TES4 payload descriptor extraction target

```cpp
// Source: `src/formats/bsa/tes4_bsa_parser.cpp` lines 351-369.
const auto stored_size = record.size_flags & ~tes4_bsa_file_size_compression_toggle;
if (!span_fits(record.offset, stored_size, archive_size)) {
  return error{error_code::format_error, "TES4 BSA entry payload span is outside the archive"};
}
if (non_empty_span_intersects_prefix(record.offset, stored_size, metadata_size)) {
  return error{error_code::format_error, "TES4 BSA entry payload span overlaps metadata"};
}
const auto compression = compression_for(header, record.size_flags);
```

### BA2 DX10 streamed snapshot assembly target

```cpp
// Source: `src/formats/ba2/ba2_dx10_prepare.cpp` lines 222-229.
result<void> append_snapshot_bytes(std::vector<std::byte>& bytes,
                                   const ba2_dx10_subresource_snapshot& snapshot) {
  return detail::for_each_host_file_chunk(
      snapshot.snapshot_path,
      snapshot.size,
      ba2_dx10_snapshot_source_context,
      [&](std::span<const std::byte> chunk) -> result<void> {
        return detail::append_byte_vector(bytes, chunk, "BA2 DX10 raw texture chunk bytes");
      });
}
```

### BA2 DX10 indexed chunk work preservation

```cpp
// Source: `src/formats/ba2/ba2_dx10_prepare.cpp` lines 517-526.
std::vector<std::optional<ba2_dx10_prepared_chunk>> chunks_by_index(planned_chunks.value().size());
auto work = [&](std::size_t index) -> result<void> {
  auto chunk = ba2_dx10_prepare_chunk(target, options, entry, planned_chunks.value()[index]);
  if (!chunk) {
    return chunk.error();
  }
  chunks_by_index[index] = std::move(chunk.value());
  return {};
};
auto prepared_chunks = detail::run_indexed_work(planned_chunks.value().size(), worker_count, work);
```

## State of the Art

| Old Approach | Current Approach for Phase 16 | When Changed | Impact |
|--------------|-------------------------------|--------------|--------|
| Monolithic parser/preparer edits with broad public behavior tests only | Private seams plus focused direct seam tests plus source-policy guardrails | Locked by Phase 16 context/spec on 2026-05-14 | Planner should create behavior-preserving internal refactor tasks, not feature tasks. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:6-22] |
| Repeated reader-family dispatch in public methods | One open-time-selected reader backend seam from Phase 15 | Phase 15 completed 2026-05-14 | Phase 16 must not reintroduce public facade family branching while touching parser internals. [VERIFIED: `.planning/STATE.md`:72-75] [VERIFIED: `.planning/phases/15-reader-backend-dispatch-cleanup/15-CONTEXT.md`:33-58] |
| No live ASan lane | MSVC ASan static preset with `LIBBSA_ENABLE_MSVC_ASAN=ON` | Phase 14 completed 2026-05-14 | Phase 16 should use ASan before closure because it is a risky parser/writer refactor. [VERIFIED: `CMakePresets.json`:62-73] [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-52] |

**Deprecated/outdated:**
- Treating `TES5Edit/` as editable or vendored source is forbidden; it is reference-only. [VERIFIED: `AGENTS.md`:20-33]
- Using raw narrow-string host-path opens in parser/preparer paths is forbidden where Phase 13 has established host-file helpers. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:68]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|

**If this table is empty:** All claims in this research were verified or cited — no user confirmation needed.

## Open Questions (RESOLVED)

1. **Exact private module names**
    - What we know: Dedicated private seams must exist for TES4 raw tables/payload descriptors and BA2 DX10 snapshot/chunk preparation. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:36-47]
    - What's unclear: Exact filenames/helper names are intentionally not locked. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58]
    - Recommendation: Planner should allow implementation-chosen filenames while requiring role-based CMake registration and source-policy evidence. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58]
   - Resolution: The plan set uses the concrete private module names from the pattern map (`tes4_bsa_table`, `tes4_bsa_payload_descriptor`, `ba2_dx10_snapshot_builder`, and `ba2_dx10_chunk_assembler`) for deterministic execution, while Plan 16-03's policy guard remains role-based per D-13 and does not freeze helper function names beyond dedicated private seam existence.

2. **Whether to add two seam test files or one combined seam test file**
    - What we know: Proof must be balanced across TES4 and BA2 DX10. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-52]
    - What's unclear: The spec does not require separate files per hotspot. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md`:74-84]
    - Recommendation: Prefer two focused runtime seam test files plus one policy test to keep ownership clear. [VERIFIED: `.planning/codebase/TESTING.md`:21-31]
   - Resolution: The plan set uses two focused runtime seam files (`tests/unit/tes4_bsa_parser_seam_tests.cpp` and `tests/unit/ba2_dx10_preparer_seam_tests.cpp`) plus one dedicated policy file (`tests/unit/parser_preparer_seam_policy_tests.cpp`) to satisfy balanced proof per D-09, TES4 coverage per D-10, BA2 DX10 coverage per D-11, and dedicated guardrail placement per D-14.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build source and tests | ✓ | 4.3.2 | None needed. [VERIFIED: bash environment audit] |
| CTest | Run label-filtered suites and full presets | ✓ | 4.3.2 | None needed. [VERIFIED: bash environment audit] |
| Git | Diff/status and policy tests reading repository files | ✓ | 2.54.0.windows.1 | None needed. [VERIFIED: bash environment audit] |
| vcpkg root | Presets use vcpkg toolchain | ✓ | `C:\vcpkg` | None needed. [VERIFIED: bash environment audit] [VERIFIED: `CMakePresets.json`:17] |
| Node | Optional GSD graph tooling | ✓ | v25.9.0 | Graph absent; not blocking. [VERIFIED: bash environment audit] [VERIFIED: graph status command returned `NO_GRAPH`] |

**Missing dependencies with no fallback:** None found. [VERIFIED: bash environment audit]

**Missing dependencies with fallback:** Planning graph absent; continue with file/code research. [VERIFIED: graph status command returned `NO_GRAPH`]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3 via `Catch2::Catch2WithMain`; CTest discovery via `catch_discover_tests`. [VERIFIED: `tests/CMakeLists.txt`:1,65-123,297-302] |
| Config file | `tests/CMakeLists.txt`; root build source registration in `CMakeLists.txt`. [VERIFIED: `tests/CMakeLists.txt`:65-113] [VERIFIED: `CMakeLists.txt`:112-173] |
| Task smoke command | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` for runtime seam tasks, or `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` for policy guard work. [VERIFIED: `CMakePresets.json`:103-143] [VERIFIED: `tests/CMakeLists.txt`:297-302] |
| Affected-suite wave command | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"` [VERIFIED: `CMakePresets.json`:103-143] [VERIFIED: `tests/CMakeLists.txt`:297-302] |
| Full suite command | `ctest --preset windows-msvc-debug-static --output-on-failure` [VERIFIED: `CMakePresets.json`:103-143] |
| Hardening lane command | `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure` [VERIFIED: `CMakePresets.json`:62-73,98-100,137-143] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| REFA-01 | TES4 raw table seam preserves table sizing, folder/file-name offsets, name/hash validation, and duplicate canonical path behavior. | unit + fixture/malformed | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa"` plus new seam labels | ❌ Wave 0: add `tests/unit/tes4_bsa_parser_seam_tests.cpp`; existing `tests/unit/tes4_bsa_reader_tests.cpp` exists. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`:218-468] |
| REFA-01 | TES4 payload descriptor seam preserves embedded-name prefix sizing, raw-size calculation, compression interpretation, and payload-span-over-metadata rejection. | unit + malformed | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa"` | ❌ Wave 0: direct seam tests needed; existing public malformed tests exist. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`:373-390,431-468] |
| REFA-02 | BA2 DX10 snapshot builder preserves source snapshot immutability and target DDS format validation. | unit + writer integration | `ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_dx10_writer"` | ❌ Wave 0: add `tests/unit/ba2_dx10_preparer_seam_tests.cpp`; existing writer tests exist. [VERIFIED: `tests/unit/ba2_dx10_writer_tests.cpp`:608-615,723-827] |
| REFA-02 | BA2 DX10 chunk seam preserves streamed snapshot-backed assembly, multi-mip/array/cubemap order, and Fallout 4 vs Starfield v3 compression routing. | unit + writer-stage | `ctest --preset windows-msvc-debug-static --output-on-failure -L "writer-stage"` | ⚠️ Existing `tests/unit/writer_stage_tests.cpp` covers single/multi/cubemap/truncated; add direct routing/plan-then-assemble seam tests. [VERIFIED: `tests/unit/writer_stage_tests.cpp`:444-548] |
| REFA-01/REFA-02 | Structural guardrail fails if responsibilities collapse back into monolithic flows. | source-policy unit | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` | ❌ Wave 0: add `tests/unit/parser_preparer_seam_policy_tests.cpp`. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58] |

### Sampling Rate

- **Per task commit:** `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` for runtime seam tasks, or `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` for policy guard work; these are the sub-30s Nyquist smoke loops after incremental build. [VERIFIED: `tests/CMakeLists.txt`:297-302]
- **Per wave merge:** `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"`; full debug is retained as Plan 16-03/phase-gate evidence. [VERIFIED: `CMakePresets.json`:103-143]
- **Phase gate:** Full debug suite plus MSVC ASan static build/test before `/gsd-verify-work`. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-52] [VERIFIED: `CMakePresets.json`:62-73]

### Wave 0 Gaps

- [ ] `tests/unit/tes4_bsa_parser_seam_tests.cpp` — covers REFA-01 table and payload descriptor seams. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-52]
- [ ] `tests/unit/ba2_dx10_preparer_seam_tests.cpp` — covers REFA-02 snapshot builder and chunk plan/assembly/compression seams. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:48-52]
- [ ] `tests/unit/parser_preparer_seam_policy_tests.cpp` — guards role separation without freezing exact helper names. [VERIFIED: `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md`:54-58]
- [ ] `CMakeLists.txt` — add any new private source `.cpp` files to `libbsa_library_sources`. [VERIFIED: `CMakeLists.txt`:112-173]
- [ ] `tests/CMakeLists.txt` — add new Catch2 test files to `libbsa_tests`. [VERIFIED: `tests/CMakeLists.txt`:65-113]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | Local archive library; no identity subsystem. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`:202-206] |
| V3 Session Management | no | No sessions or network service state. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`:202-206] |
| V4 Access Control | no | No authorization subsystem; file access is caller/OS-owned. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`:202-206] |
| V5 Input Validation | yes | Preserve parser/preparer count, span, size, hash, duplicate-path, target-format, and compression-method validation using existing `result<T>` errors. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:102-121,140-153,267-385] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:144-220,368-397] |
| V6 Cryptography | yes, limited | Do not hand-roll randomness; snapshot temp directory suffix already uses `BCryptGenRandom`. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:86-105] |

### Known Threat Patterns for libbsa parser/preparer stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed archive count/size causing allocation or overflow | Denial of Service / Tampering | Use `validate_metadata_count`, `multiply_fits`, `add_fits`, `span_fits`, checked integer narrowing, and malformed fixture tests. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:102-121,398-435] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:144-170] |
| Payload offsets overlapping metadata | Tampering | Keep TES4 payload descriptor span checks and malformed regression coverage. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`:351-359] [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`:373-390] |
| Changed or truncated snapshot files before publish | Tampering / Denial of Service | Stream fixed-size snapshot files through host-file chunk helpers and fail before publish changes destination. [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`:222-229] [VERIFIED: `tests/unit/writer_stage_tests.cpp`:499-521] |
| Temporary texture data persistence on abnormal termination | Information Disclosure | Do not claim cleanup proof in Phase 16; keep Phase 17 requirement open. [VERIFIED: `.planning/codebase/CONCERNS.md`:33-40] [VERIFIED: `.planning/REQUIREMENTS.md`:37-40] |

## Sources

### Primary (HIGH confidence)

- `.planning/phases/16-parser-and-preparer-seam-extraction/16-CONTEXT.md` — locked Phase 16 decisions, scope, guardrails, and code references.
- `.planning/phases/16-parser-and-preparer-seam-extraction/16-SPEC.md` — locked requirements, boundaries, constraints, and acceptance criteria.
- `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/ROADMAP.md` — milestone traceability and Phase 15 dependency state.
- `AGENTS.md` — project constraints and TES5Edit/read-only/dependency/comment/test rules.
- `src/formats/bsa/tes4_bsa_parser.cpp` / `.hpp` — current TES4 parser hotspot and stable internal entrypoints.
- `src/formats/ba2/ba2_dx10_prepare.cpp` / `.hpp` — current BA2 DX10 preparer hotspot and stable internal surface.
- `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/writer_stage_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/archive_reader_dispatch_policy_tests.cpp` — existing runtime and policy test patterns.
- `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json` — build/test/dependency registration and supported verification lanes.
- Context7 `/catchorg/catch2` — verified `catch_discover_tests`, `ADD_TAGS_AS_LABELS`, and `DISCOVERY_MODE PRE_TEST` usage.

### Secondary (MEDIUM confidence)

- `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/TESTING.md`, `.planning/codebase/CONCERNS.md` — codebase maps from 2026-05-12, corroborated against current files during this research.

### Tertiary (LOW confidence)

- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — verified from `vcpkg.json`, CMake files, local tool audit, and Context7 Catch2 docs.
- Architecture: HIGH — phase decisions, code hotspots, and existing architecture maps align with current source files.
- Pitfalls: HIGH — pitfalls are derived from locked decisions, current tests, and current hotspot responsibilities.

**Research date:** 2026-05-14
**Valid until:** 2026-06-13 for this stable internal refactor scope; refresh sooner if Phase 15/16 source files change before planning.
