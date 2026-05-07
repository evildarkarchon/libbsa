# Phase 09: bsa-writers - Research

**Researched:** 2026-05-07  
**Domain:** C++20 TES3 and TES4-family BSA archive serialization, layout planning, compression, hashes, and read-after-write verification  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### BSA API Shape
- **D-01:** Phase 9 should expose BSA-specific writer wrappers layered over the Phase 8 writer core. The API should feel parallel to existing BSA operations like `open_bsa` and `extract_bsa_entry`, while reusing deterministic plan/finalize internals.
- **D-02:** Callers choose the BSA output variant through an explicit BSA-specific target enum or equivalent libbsa-owned value. Required choices are TES3 Morrowind, TES4 v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105; do not infer the target from file extensions or loose game-name strings.
- **D-03:** BSA writer plans should expose BSA-native layout details needed for inspection and testing, including folder/file table regions, folder/file hashes, BSA flags, and archive-absolute payload offsets. Do not limit the production BSA preview to the generic Phase 8 harness metadata if that would hide compatibility-critical layout decisions.
- **D-04:** The Phase 8 `LBSW` harness must remain test-only once production BSA writers exist. Public production BSA writer paths must emit native BSA bytes and must not imply that the harness is a real archive format.

### Disk Input Model
- **D-05:** Disk-backed BSA inputs should use explicit host-file plus archive-virtual-path pairs. Consumers or tools own directory traversal, filtering, symlink handling, include/exclude policy, and ordering policy.
- **D-06:** Disk file bytes should be read during planning, not deferred to finalization. Plans should own the post-policy stored payload bytes so finalization remains deterministic, retains no file handles, and only streams already-planned bytes to the caller-owned `byte_sink`.
- **D-07:** Phase 9 should not include directory scanning or root-recursive packing helpers. Those are application/tooling policy and are not needed to satisfy BSA writer acceptance.
- **D-08:** Memory-backed entries and disk-backed file mappings should be represented with separate public value types or clearly separate helper APIs. Avoid a single variant-like entry type with invalid combinations of bytes and host paths.

### Compression And Embedded Names
- **D-09:** TES4-family archive-default compression should be an explicit caller-visible option on the BSA target/options, while per-entry policy can still request archive default, force raw, or force compressed. Writer behavior must not infer compression from path or extension.
- **D-10:** Callers should request semantic compression policy; the writer computes the BSA-native per-file compression flag using the existing reader's archive-default XOR model. Do not expose the raw high-bit table flag as the normal consumer control.
- **D-11:** Embedded-name writing should be archive-level opt-in and default off. When enabled, the writer sets `ARCHIVE_EMBEDNAME` and prefixes each TES4-family payload with the archive name in the format the reader already skips.
- **D-12:** Compression or embedded-name requests that cannot be represented safely for the selected BSA target must fail structurally during planning, using appropriate `unsupported_format` or `malformed_archive` errors. Do not silently downgrade to raw output, skip embedded names, or try fallback codecs.

### BSA Ordering Policy
- **D-13:** TES3 BSA record/hash/name ordering must follow TES3-compatible hash ordering regardless of caller input order. Ties, if possible, must be handled deterministically and covered by tests.
- **D-14:** TES4-family folder and file table ordering must be deterministic and reference-compatible. Research should trace BSArchPro/TES5Edit ordering expectations before planning exact sorting rules; do not assume Phase 8 normalized path order is sufficient if reference behavior differs.
- **D-15:** BSA deduplication may be enabled only if research proves shared payload offsets/data regions are compatible for the target BSA variant. If compatibility is not proven for a target, dedup requests must fail structurally instead of silently emitting non-dedup output or risky shared offsets.
- **D-16:** Phase 9 should write only folders implied by file entries. Do not add public directory-only or empty-folder input support unless reference research proves it is required.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may choose exact type names, function names, helper file names, and test organization details as long as the decisions above, `09-SPEC.md`, existing public API patterns, and project constraints are satisfied.

### Deferred Ideas (OUT OF SCOPE)
## Deferred Ideas

None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WRT-01 | Consumer can create TES4-family BSA archives for Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE from disk paths or in-memory buffers. [VERIFIED: `.planning/REQUIREMENTS.md`] | Use BSA-specific target values for v103/v104/v105, serialize native TES4-family header/folder/file/name/payload regions, route v103/v104 compression to deflate and v105 compression to LZ4-frame, and prove output through `open_bsa` / `extract_bsa_entry`. [VERIFIED: `09-SPEC.md`, `src/bsa_reader.cpp`, `src/compression.cpp`] |
| WRT-04 | Consumer can create TES3 Morrowind BSA archives with correct hash sorting and data-section-relative offsets. [VERIFIED: `.planning/REQUIREMENTS.md`] | Sort TES3 records by `hash_tes3_path`, emit file records/name offsets/name block/hash table before raw data, and store record offsets relative to the TES3 data section while plan metadata exposes archive-absolute offsets. [VERIFIED: `09-SPEC.md`, `src/bsa_reader.cpp`, `src/hash.cpp`, `TES5Edit/Core/wbBSArchive.pas:1353-1386,1588-1616`] |
</phase_requirements>

## Summary

Phase 9 should add production BSA writer APIs rather than extending the Phase 8 `LBSW` harness into a pseudo-format. [VERIFIED: `09-CONTEXT.md`, `09-SPEC.md`] The writer should keep Phase 8's plan-then-finalize invariant: planning reads disk bytes, normalizes archive paths, computes hashes, resolves compression, builds native table bytes/payload bytes, checks all size/offset arithmetic, and owns the planned stored payloads; finalization only streams planned native BSA chunks to a caller-owned `byte_sink`. [VERIFIED: `09-CONTEXT.md`, `include/libbsa/writer.hpp`, `src/writer.cpp`]

The highest-risk compatibility decisions are ordering and native size/offset fields. [VERIFIED: `09-CONTEXT.md`, `09-SPEC.md`] TES3 records are sorted by TES3 hash, with `HashOffset` stored relative to byte 12 and file record offsets stored relative to the data section. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1353-1386,1588-1616`, `src/bsa_reader.cpp:200-296`] TES4-family records are sorted by directory hash then file hash, folder records precede per-folder file record blocks, all file names are stored in a global null-terminated file-name table, and v105 folder records use a 24-byte record with 64-bit folder block offsets. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1248-1497,1619-1655`, `src/bsa_reader.cpp:326-457`, `src/bsa_reader.hpp`]

**Primary recommendation:** Implement a BSA-specific public wrapper surface, likely `plan_bsa_write(...)` / `finalize_bsa_write(...)` with explicit `bsa_write_target`, separate memory and disk entry types, and a `bsa_write_plan` that exposes native BSA table regions, hashes, flags, offsets, and compression states while internally reusing Phase 8 normalization/compression/dedup/finalization helpers where practical. [VERIFIED: `09-CONTEXT.md`, `include/libbsa/bsa.hpp`, `include/libbsa/writer.hpp`]

## Project Constraints (from AGENTS.md)

- `TES5Edit/` is read-only reference material; do not edit, format, generate changes under, update the submodule pointer, stage, commit, compile, or vendor it. [VERIFIED: `AGENTS.md`]
- Implementation language is C++; public interfaces must be reusable, portable C++ rather than direct Delphi/Pascal transliteration. [VERIFIED: `AGENTS.md`]
- Preserve archive-format behavior discovered from BSArchPro/TES5Edit unless a divergence is documented. [VERIFIED: `AGENTS.md`]
- Trace non-obvious compatibility constraints near new implementation. [VERIFIED: `AGENTS.md`]
- Do not introduce speculative dependencies; use `libdeflate`, official `lz4`, DirectXTex, and vcpkg only where already justified. [VERIFIED: `AGENTS.md`, `CMakeLists.txt`]
- Public headers must not leak libdeflate, LZ4, DirectXTex, Windows/platform, Delphi, UI, or TES5Edit types. [VERIFIED: `AGENTS.md`, `09-SPEC.md`]
- Never delete accurate comments as cleanup; add comments for non-obvious format compatibility, ownership/lifetime, error-handling, threading, cancellation, and deliberate deviations. [VERIFIED: `AGENTS.md`]
- Add Doxygen-compliant C++ doc comments for public APIs and substantially rewritten methods. [VERIFIED: `AGENTS.md`]
- Add focused tests for writing, round-tripping, and compatibility behavior; do not use `TES5Edit/` as a mutable fixture. [VERIFIED: `AGENTS.md`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| BSA writer public API | API / Library Core | Tests | BSA writing is a reusable library operation parallel to `open_bsa` and `extract_bsa_entry`; tests compile it through public headers. [VERIFIED: `include/libbsa/bsa.hpp`, `09-CONTEXT.md`, `09-SPEC.md`] |
| Disk input ingestion | API / Library Core | Host filesystem boundary | Planning reads explicit host-file plus archive-path mappings; traversal/filtering remains caller-owned tooling policy. [VERIFIED: `09-CONTEXT.md`] |
| Archive path normalization and duplicate rejection | API / Library Core | — | Writer inputs must use libbsa archive-virtual path rules and reject duplicate normalized paths before layout. [VERIFIED: `include/libbsa/archive_path.hpp`, `src/writer.cpp`, `09-SPEC.md`] |
| BSA table/hash/layout planning | API / Library Core | TES5Edit reference / tests | Native folder/file records, name tables, hashes, flags, and offsets are compatibility-critical planner-visible decisions. [VERIFIED: `09-CONTEXT.md`, `TES5Edit/Core/wbBSArchive.pas:1248-1655`] |
| Compression and embedded-name payload shaping | API / Library Core | Private codec adapters | Planning must add embedded-name and uncompressed-size prefixes before stored payloads and route compression through existing private codec dispatch. [VERIFIED: `src/bsa_reader.cpp:476-523`, `src/compression.cpp`, `TES5Edit/Core/wbBSArchive.pas:1863-1885`] |
| Dedup sharing | API / Library Core | Phase 8 writer core | Native BSA record formats can represent shared size/offset records; when enabled, sharing should occur only for byte-identical post-policy stored regions. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1017-1068`, `08-CONTEXT.md`, `09-CONTEXT.md`] |
| Finalization | API / Library Core | Caller-owned sink | Emission streams planned bytes through `byte_sink`, propagating the first sink failure and retaining no sink lifetime. [VERIFIED: `include/libbsa/io.hpp`, `src/writer.cpp`, `09-SPEC.md`] |
| Read-after-write proof | Test tier | API / Library Core | Production writer acceptance depends on reopening generated bytes through libbsa's BSA reader and extracting payloads. [VERIFIED: `09-SPEC.md`, `tests/bsa_reader_tests.cpp`] |

## Standard Stack

### Core

| Library / Component | Version / Status | Purpose | Why Standard |
|---------------------|------------------|---------|--------------|
| C++ | C++20 via `target_compile_features(libbsa PUBLIC cxx_std_20)`. [VERIFIED: `CMakeLists.txt`] | Public API and implementation. [VERIFIED: `AGENTS.md`] | Matches project constraints and existing `libbsa::result` C++20 error model. [VERIFIED: `include/libbsa/result.hpp`, `.planning/STATE.md`] |
| Existing BSA API | `open_bsa` / `extract_bsa_entry` in `include/libbsa/bsa.hpp`. [VERIFIED: `include/libbsa/bsa.hpp`] | Public API style precedent and read-after-write verifier. [VERIFIED: `09-CONTEXT.md`] | Writer wrappers should feel parallel to existing BSA operations. [VERIFIED: `09-CONTEXT.md`] |
| Phase 8 writer core | `include/libbsa/writer.hpp`, `src/writer.cpp`. [VERIFIED: codebase] | Plan/finalize model, stored payload ownership, compression policy resolution, dedup option, and sink emission precedent. [VERIFIED: `include/libbsa/writer.hpp`, `src/writer.cpp`] | Reuse/adapt internals, but do not emit `LBSW` for production BSA. [VERIFIED: `09-CONTEXT.md`] |
| Existing BSA reader internals | `src/bsa_reader.cpp`, `src/bsa_reader.hpp`. [VERIFIED: codebase] | Native BSA constants, parsing expectations, XOR compression model, embedded-name skip behavior, and metadata semantics. [VERIFIED: `src/bsa_reader.cpp`, `src/bsa_reader.hpp`] | Writer output must satisfy these readers for Phase 9 acceptance. [VERIFIED: `09-SPEC.md`] |
| Existing hash routines | `src/hash.cpp`, `src/hash.hpp`. [VERIFIED: codebase] | TES3/TES4 hash emission and ordering. [VERIFIED: `src/hash.cpp`] | Reuse one implementation for reader tests and writer records to avoid drift. [VERIFIED: `09-CONTEXT.md`] |

### Supporting

| Library / Component | Version / Status | Purpose | When to Use |
|---------------------|------------------|---------|-------------|
| libdeflate | Existing private dependency in `CMakeLists.txt`; stack research lists vcpkg `libdeflate 1.25#0`. [VERIFIED: `CMakeLists.txt`, `.planning/research/STACK.md`] | TES4 v103/v104 compressed BSA payloads. [VERIFIED: `src/compression.cpp`, `09-SPEC.md`] | Use only through `compress_payload(compression_algorithm::deflate, ...)`. [VERIFIED: `include/libbsa/compression.hpp`, `src/compression.cpp`] |
| lz4 | Existing private dependency in `CMakeLists.txt`; stack research lists vcpkg `lz4 1.10.0#0`. [VERIFIED: `CMakeLists.txt`, `.planning/research/STACK.md`] | Skyrim SE/AE v105 LZ4-frame compressed BSA payloads. [VERIFIED: `src/compression.cpp`, `09-SPEC.md`] | Use only through `compress_payload(compression_algorithm::lz4_frame, ...)`; do not use Starfield raw LZ4 block for BSA v105. [VERIFIED: `src/compression.cpp`, `.planning/research/STACK.md`] |
| Catch2 / CTest | Existing tests and CMake wiring use Catch2 and `catch_discover_tests`. [VERIFIED: `CMakeLists.txt`] | Writer unit, fixture, codec, round-trip, public-header smoke, and failure tests. [VERIFIED: `09-SPEC.md`] | Add BSA writer tests either to `libbsa_writer_tests` or a focused `libbsa_bsa_writer_tests` target with labels. [ASSUMED] |
| `memory_source` / `memory_sink` | Existing public I/O helpers. [VERIFIED: `include/libbsa/io.hpp`] | Generated read-after-write tests and finalization size assertions. [VERIFIED: `tests/bsa_reader_tests.cpp`, `tests/writer_core_tests.cpp`] | Use for in-memory BSA output, `open_bsa`, and `extract_bsa_entry` round trips. [VERIFIED: `09-SPEC.md`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| BSA-specific target enum/value | Raw `archive_format` plus booleans | Rejected because context requires explicit BSA target choices and BSA-native options; raw `archive_format` cannot express target-specific writer options cleanly. [VERIFIED: `09-CONTEXT.md`] |
| Separate memory and disk entry value types | Single variant-like entry with path and bytes/host path fields | Rejected by locked D-08 because invalid combinations are easy to construct. [VERIFIED: `09-CONTEXT.md`] |
| Native BSA writer finalizer | Generic Phase 8 `finalize_archive_write` emitting `LBSW` | Rejected because production BSA paths must emit native BSA bytes and `LBSW` remains test-only. [VERIFIED: `09-CONTEXT.md`, `src/writer.cpp`] |
| Hash-order sorting | Lexicographic normalized path sorting | Rejected for native records: TES3 and TES4-family reference creation sorts by hashes. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1353-1361,1396-1450`] |
| Compression fallback/downgrade | Try raw if compression fails or unsupported | Rejected by locked no-fallback decisions; unsupported compression or embedded-name requests fail structurally. [VERIFIED: `09-CONTEXT.md`, `src/compression.cpp`] |

**Installation / dependency changes:** No new package installation is recommended for Phase 9. [VERIFIED: `AGENTS.md`, `CMakeLists.txt`, `09-SPEC.md`]

```bash
cmake --preset windows-msvc-vcpkg
cmake --build --preset windows-msvc-vcpkg
ctest --preset windows-msvc-vcpkg --output-on-failure
```

**Version verification:** Local tool audit reports CMake/CTest 4.3.2, Git 2.54.0.windows.1, and `C:\vcpkg\vcpkg.exe` version `2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1`; `cl` is not on PATH in this shell. [VERIFIED: shell audit]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer chooses explicit BSA target + options
        |
        v
Memory entries OR disk file mappings with archive-virtual paths
        |
        v
Planning phase
  - read disk files now, no deferred handles
  - normalize archive paths and reject duplicates
  - split path into BSA folder/file names
  - compute TES3/TES4 hashes and target flags
  - resolve compression policy and embedded-name prefixes
  - build post-policy stored payload bytes
  - optional dedup: share only byte-identical stored regions when target allows
  - sort native records by target-compatible hash order
  - compute checked header/table/name/payload offsets
        |-- invalid/unsupported/overflow/missing file --> result error, no sink access
        v
BSA write plan with native table regions, hashes, flags, offsets, payload regions
        |
        v
finalize_bsa_write(plan, byte_sink&)
        |-- sink write failure --> original structured error
        v
Native TES3 or TES4-family BSA bytes
        |
        v
Test verification: memory_source -> open_bsa -> metadata compare -> extract_bsa_entry
```

### Recommended Project Structure

```text
include/libbsa/
├── bsa.hpp                  # Existing BSA read API; either extend with BSA writer declarations or include a writer header. [ASSUMED]
├── bsa_writer.hpp           # Alternative focused public BSA writer target/entry/plan/finalize declarations. [ASSUMED]
src/
├── bsa_writer.cpp           # Native TES3/TES4-family planning and finalization. [ASSUMED]
├── bsa_reader.hpp           # Shared BSA constants may move or be reused internally without public leakage. [ASSUMED]
tests/
├── bsa_writer_tests.cpp     # Native writer fixtures, round trips, failures, and layout assertions. [ASSUMED]
├── bsa_writer_fixture_helpers.* # Optional source-built test helpers for native byte inspection. [ASSUMED]
└── public_header_smoke.cpp  # Extend consumer compile coverage for BSA writer APIs. [VERIFIED: existing file]
```

### Pattern 1: BSA-Specific Plan Wrapper Over Phase 8 Semantics

**What:** Keep a plan value that exposes BSA-native metadata, not just generic data regions. [VERIFIED: `09-CONTEXT.md`]  
**When to use:** All Phase 9 write paths. [VERIFIED: `09-SPEC.md`]  
**Example:**

```cpp
// Source: Phase 9 context D-01 through D-04. [VERIFIED: 09-CONTEXT.md]
enum class bsa_write_target {
    tes3_morrowind,
    oblivion_v103,
    fo3_fnv_skyrim_le_v104,
    skyrim_se_ae_v105,
};

struct planned_bsa_entry {
    std::string path;
    std::uint64_t directory_hash{};
    std::uint64_t file_hash{};
    std::uint64_t offset{};       // Archive-absolute payload offset.
    std::uint64_t stored_size{};  // Native payload record size, including BSA prefixes.
    compression_state compression{compression_state::unknown};
};
```

### Pattern 2: TES3 Native Layout Planning

**What:** Emit TES3 header, file records, name offsets, null-terminated name block, hash table, then raw payloads; record offsets are data-section-relative. [VERIFIED: `src/bsa_reader.cpp:200-296`, `TES5Edit/Core/wbBSArchive.pas:1588-1616`]  
**When to use:** `bsa_write_target::tes3_morrowind`. [VERIFIED: `09-SPEC.md`]  
**Example:**

```cpp
// Source: TES5Edit save layout and current reader parser. [VERIFIED: TES5Edit/Core/wbBSArchive.pas:1588-1616] [VERIFIED: src/bsa_reader.cpp:200-296]
tes3_header.magic = 0x00000100;
tes3_header.file_count = entries.size();
tes3_header.hash_offset = record_table_size + name_offset_table_size + name_block_size; // Stored relative to byte 12.
record.relative_offset = payload_archive_offset - data_section_offset;
```

### Pattern 3: TES4-Family Native Layout Planning

**What:** Build a 36-byte header, sorted folder records, per-folder folder-name/file-record blocks, a global file-name table, then payloads. [VERIFIED: `src/bsa_reader.hpp`, `src/bsa_reader.cpp:326-457`, `TES5Edit/Core/wbBSArchive.pas:1619-1655`]  
**When to use:** v103/v104/v105 targets. [VERIFIED: `09-SPEC.md`]  
**Example:**

```cpp
// Source: current reader constants and reference save path. [VERIFIED: src/bsa_reader.hpp] [VERIFIED: TES5Edit/Core/wbBSArchive.pas:1629-1655]
header.magic = 0x00415342; // "BSA\0"
header.version = target_version; // 0x67, 0x68, or 0x69.
header.folders_offset = 36;
header.flags = ARCHIVE_PATHNAMES | ARCHIVE_FILENAMES | optional_compress | optional_embed_name;
folder_record_size = (target_version == 0x69) ? 24 : 16;
file_record.size_field = stored_payload_size | (archive_default_compressed != entry_compressed ? 0x40000000 : 0);
```

### Anti-Patterns to Avoid

- **Using Phase 8 lexicographic order for native BSA records:** Native TES3/TES4 record order is hash-sorted in the reference creation path. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1353-1361,1396-1450`]
- **Finalization reads host files:** Disk reads are locked to planning so plans are deterministic and handle-free. [VERIFIED: `09-CONTEXT.md`]
- **Exposing raw `FILE_SIZE_COMPRESS` as caller policy:** Caller policy is semantic; writer computes XOR high-bit fields. [VERIFIED: `09-CONTEXT.md`, `src/bsa_reader.cpp:189-198`, `TES5Edit/Core/wbBSArchive.pas:812-819,1882-1885`]
- **Embedding names by default:** Phase 9 requires archive-level opt-in default off, even though BSArchPro sets some automatic embed flags. [VERIFIED: `09-CONTEXT.md`, `TES5Edit/Core/wbBSArchive.pas:1298,1506-1509`]
- **Using `std::filesystem::path` as archive path:** Archive virtual paths use libbsa normalization; host paths only belong to disk input source fields. [VERIFIED: `include/libbsa/archive_path.hpp`, `09-SPEC.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Structured writer failures | Exceptions, booleans, or warning-only downgrades | `libbsa::result<T>` / `result<void>` | Existing project error model uses structured errors for I/O/format failures. [VERIFIED: `include/libbsa/result.hpp`, `09-SPEC.md`] |
| Archive path normalization | Host filesystem canonicalization or ad hoc lowercase | `normalize_archive_path` | Archive paths are relative virtual paths with forward slashes and ASCII lowercase rules. [VERIFIED: `include/libbsa/archive_path.hpp`] |
| TES3/TES4 hashes | New hash implementations in writer | Existing `detail::hash_tes3_path` / `detail::hash_tes4_path` equivalents | Existing hashes are already tested against reference behavior and used by readers. [VERIFIED: `src/hash.cpp`, `09-CONTEXT.md`] |
| Compression routing | Direct libdeflate/LZ4 calls from BSA writer | `resolve_write_compression`, `resolve_payload_codec`, `compress_payload` | Keeps codec details private and prevents deflate/LZ4-frame/raw-block confusion. [VERIFIED: `include/libbsa/compression.hpp`, `src/compression.cpp`] |
| Output streaming | Returned whole archive only or `std::ostream` | `byte_sink` finalization | Phase 8 and Phase 9 require caller-owned streaming finalization and sink failure propagation. [VERIFIED: `include/libbsa/io.hpp`, `09-SPEC.md`] |
| Test fixtures | Binary blobs only | Source-built generated fixture helpers plus read-after-write through `open_bsa` | Existing test style is source-reviewable fixtures, and Phase 9 acceptance requires libbsa read-back. [VERIFIED: `tests/bsa_reader_tests.cpp`, `09-SPEC.md`] |

**Key insight:** Native BSA writing is mostly deterministic table construction; the compatibility risk is hidden in field semantics: hash order, relative-vs-absolute offsets, XOR compression flags, embedded-name prefix sizing, and version-specific folder record width. [VERIFIED: `src/bsa_reader.cpp`, `TES5Edit/Core/wbBSArchive.pas:1248-1901`]

## Common Pitfalls

### Pitfall 1: TES3 Hash Table Byte Order Drift
**What goes wrong:** The writer emits a 64-bit hash with helper `append_u64`, but TES5Edit writes the high 32 bits then low 32 bits as two little-endian cardinals. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1612-1616`]  
**Why it happens:** The existing test helper uses `append_u64`, while reference save code writes two 32-bit values explicitly. [VERIFIED: `tests/bsa_reader_tests.cpp:141-143`, `TES5Edit/Core/wbBSArchive.pas:1612-1616`]  
**How to avoid:** Add tests that assert serialized TES3 hash table bytes for known hashes and decide whether to adjust reader/writer together to reference byte order or keep current reader-compatible order with documented rationale. [VERIFIED: `09-SPEC.md`]  
**Warning signs:** Tests only reopen through libbsa and never inspect raw TES3 hash table bytes. [VERIFIED: `09-SPEC.md`]

### Pitfall 2: Confusing TES3 Data-Section-Relative Offsets With Plan Metadata
**What goes wrong:** File records store archive-absolute offsets, causing the TES3 reader to add the data offset twice. [VERIFIED: `src/bsa_reader.cpp:263-267`, `TES5Edit/Core/wbBSArchive.pas:1599-1602`]  
**Why it happens:** Public `entry_metadata::offset` and Phase 8 plan offsets are archive-absolute. [VERIFIED: `include/libbsa/archive.hpp`, `08-CONTEXT.md`]  
**How to avoid:** Store relative offsets in TES3 file records but expose archive-absolute offsets in the BSA plan and read-back metadata. [VERIFIED: `09-SPEC.md`, `src/bsa_reader.cpp`]  
**Warning signs:** A TES3 writer test compares record offset directly to `metadata.offset` instead of `metadata.offset - data_section_offset`. [VERIFIED: `09-SPEC.md`]

### Pitfall 3: TES4 File Names Table Placement
**What goes wrong:** Payload offsets point before the global file-name table or folder offsets ignore `FileNamesLength`. [VERIFIED: `src/bsa_reader.cpp:370-417`, `TES5Edit/Core/wbBSArchive.pas:1483-1497`]  
**Why it happens:** Folder records point to folder blocks, but payloads start only after all folder blocks and the global file-name table. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1483-1497,1651-1654`]  
**How to avoid:** Compute folder block offsets and payload start from header + folder records + all folder blocks + all file names, with checked arithmetic and byte-level table-region tests. [VERIFIED: `09-SPEC.md`]  
**Warning signs:** Single-folder tests pass, but multi-folder/multi-file payload offsets overlap names. [ASSUMED]

### Pitfall 4: XOR Compression Flag Inversion
**What goes wrong:** Per-file `FILE_SIZE_COMPRESS` is set whenever an entry is compressed, even when archive default compression is already enabled; the reader then interprets the entry as raw. [VERIFIED: `src/bsa_reader.cpp:189-198`, `TES5Edit/Core/wbBSArchive.pas:812-819,1882-1885`]  
**Why it happens:** Native BSA stores a toggle bit, not a direct compressed boolean. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:812-819,1882-1885`]  
**How to avoid:** Compute `toggle = archive_default_compressed XOR entry_is_compressed` and OR `0x40000000` only when the toggle is true. [VERIFIED: `src/bsa_reader.cpp:189-198`]  
**Warning signs:** Archive-default compressed and force-compressed test cases have identical size flags. [VERIFIED: `09-SPEC.md`]

### Pitfall 5: Embedded-Name Stored Size Accounting
**What goes wrong:** Metadata `stored_size` excludes the embedded-name prefix or compressed-size prefix, causing extraction range validation or size reads to fail. [VERIFIED: `src/bsa_reader.cpp:433-445,488-507`, `TES5Edit/Core/wbBSArchive.pas:1863-1885`]  
**Why it happens:** The reader treats embedded names and uncompressed-size prefixes as bytes inside the native stored payload. [VERIFIED: `src/bsa_reader.cpp:483-523`]  
**How to avoid:** Build native payload as `[embedded len+name?][uncompressed u32 if compressed][compressed/raw bytes]` and set file record size to the full native payload length before XOR bit masking. [VERIFIED: `src/bsa_reader.cpp:476-523`, `TES5Edit/Core/wbBSArchive.pas:1863-1885`]  
**Warning signs:** Embedded-name raw entries extract one byte short or compressed entries read the prefix as the uncompressed-size field. [ASSUMED]

### Pitfall 6: Treating Reference Auto Flags as Locked Product Policy
**What goes wrong:** Writer silently turns on `ARCHIVE_EMBEDNAME`, `ARCHIVE_STARTUPSTR`, or `ARCHIVE_RETAINNAME` from path/file categories even though Phase 9 locks embedded names as explicit opt-in default off. [VERIFIED: `09-CONTEXT.md`, `TES5Edit/Core/wbBSArchive.pas:1506-1517`]  
**Why it happens:** BSArchPro contains automatic flag heuristics for application packing behavior. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1410-1525`]  
**How to avoid:** Use reference heuristics as compatibility research, but expose explicit libbsa-owned writer options for archive-default compression and embedded names; document any chosen automatic file flag computation. [VERIFIED: `09-CONTEXT.md`, `09-SPEC.md`]  
**Warning signs:** A texture-only v104 archive gets embedded names without the caller opting in. [VERIFIED: `09-CONTEXT.md`]

## Code Examples

Verified patterns from project/reference sources:

### TES4-family Compression Toggle

```cpp
// Source: current reader and TES5Edit reference. [VERIFIED: src/bsa_reader.cpp:189-198] [VERIFIED: TES5Edit/Core/wbBSArchive.pas:1882-1885]
const bool archive_default = options.archive_default_compressed;
const bool entry_compressed = entry.compression != compression_state::raw;
std::uint32_t size_field = checked_u32(native_payload.size());
if (archive_default != entry_compressed) {
    size_field |= 0x40000000U;
}
```

### Embedded and Compressed Native Payload Shape

```cpp
// Source: extraction order in reader. [VERIFIED: src/bsa_reader.cpp:488-523]
std::vector<std::byte> native_payload;
if (options.embedded_names) {
    append_u8(native_payload, checked_u8(embedded_name.size()));
    append_ascii_bytes(native_payload, embedded_name);
}
if (entry_compressed) {
    append_u32(native_payload, checked_u32(unpacked_payload.size()));
    append_bytes(native_payload, compressed_payload);
} else {
    append_bytes(native_payload, unpacked_payload);
}
```

### TES3 Layout Cursor

```cpp
// Source: TES3 reader and reference save layout. [VERIFIED: src/bsa_reader.cpp:200-296] [VERIFIED: TES5Edit/Core/wbBSArchive.pas:1593-1616]
const auto record_table_size = file_count * 8ULL;
const auto name_offset_table_size = file_count * 4ULL;
const auto hash_offset = record_table_size + name_offset_table_size + name_block_size;
const auto hash_table_start = 12ULL + hash_offset;
const auto data_section_offset = hash_table_start + file_count * 8ULL;
record.relative_offset = payload_offset - data_section_offset;
```

## BSA Native Layout Notes

| Topic | Required Writer Behavior | Source / Confidence |
|-------|--------------------------|---------------------|
| TES3 header | First 4 bytes are `0x00000100`; then `HashOffset`, then `FileCount`. [VERIFIED: `src/bsa_reader.hpp`, `TES5Edit/Core/wbBSArchive.pas:501,1593-1597`] | HIGH |
| TES3 `HashOffset` | Stored relative to byte 12, equal to record table + name offset table + name block length. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1371-1379`, `src/bsa_reader.cpp:207-219`] | HIGH |
| TES3 record offset | Stored relative to data section; libbsa metadata exposes absolute payload offsets. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1599-1602`, `src/bsa_reader.cpp:263-284`] | HIGH |
| TES3 ordering | Reference sorts file records by TES3 hash, then stores payload data alphabetically in vanilla-like order; because record offsets can point anywhere, Phase 9 can use hash-sorted records and deterministic payload region order, but tests must assert both. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1353-1386`] | HIGH |
| TES4 magic/version | Magic is `BSA\0`; versions are `0x67`, `0x68`, `0x69`. [VERIFIED: `src/bsa_reader.hpp`, `TES5Edit/Core/wbBSArchive.pas:501-525`] | HIGH |
| TES4 header size | Header is 36 bytes: magic, version, folders offset, flags, counts, folder/file name lengths, file flags. [VERIFIED: `src/bsa_reader.hpp`, `tests/bsa_reader_tests.cpp:83-96`] | HIGH |
| TES4 folder records | v103/v104 use 16-byte folder records; v105 uses 24-byte records with an extra 32-bit field and 64-bit offset. [VERIFIED: `src/bsa_reader.hpp`, `src/bsa_reader.cpp:344-367`, `TES5Edit/Core/wbBSArchive.pas:1632-1641`] | HIGH |
| TES4 ordering | Reference sorts by directory hash, then file hash before grouping folders. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1248-1267,1396-1450`] | HIGH |
| TES4 folder names length | Reference increments folder names length by folder name length + terminator only, not the length-prefix byte. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1466-1470`] | MEDIUM because current reader does not validate this field. [VERIFIED: `src/bsa_reader.cpp:326-457`] |
| TES4 file names length | Stored as sum of file name length + null terminator; reader validates this range and then reads exactly `file_count` null-terminated names. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1477-1479`, `src/bsa_reader.cpp:410-417`] | HIGH |
| Compression codecs | v103/v104 compressed payloads use deflate; v105 compressed payloads use LZ4 frame. [VERIFIED: `src/compression.cpp`, `TES5Edit/Core/wbBSArchive.pas:1301-1317`] | HIGH |
| Embedded names | Reader skips a one-byte length plus name bytes before optional compressed-size field; reference only writes embedded names for FO3/SSE in `PackData`, while Phase 9 locks TES4-family opt-in behavior. [VERIFIED: `src/bsa_reader.cpp:488-507`, `TES5Edit/Core/wbBSArchive.pas:1863-1865`, `09-CONTEXT.md`] | HIGH |

## Dedup Compatibility Finding

Shared payload offsets are representable for TES3, v103, v104, and v105 BSA records because reference `FindPackedData` copies `Size` and `Offset` from an earlier BSA record for `baTES3`, `baTES4`, `baFO3`, and `baSSE`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1017-1036`] Reference `AddPackedData` stores packed-data records only when `fShareData` is enabled. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1053-1068`] Therefore Phase 9 may enable dedup for all required BSA targets if it shares only byte-identical post-policy native stored payload regions and exposes shared `data_region_id` / archive-absolute offsets in the plan. [VERIFIED: `08-CONTEXT.md`, `09-CONTEXT.md`, `TES5Edit/Core/wbBSArchive.pas:1017-1068`]

Two compatibility caveats should shape tests. [VERIFIED: `09-SPEC.md`] First, embedded-name prefixes include an entry-specific name, so two otherwise identical entries generally will not dedup once embedded names are enabled unless the final native payload bytes are actually identical. [VERIFIED: `src/bsa_reader.cpp:488-507`] Second, BSArchPro's dedup lookup uses original `DataSize` and an MD5 hash passed before compression, whereas Phase 8 locked libbsa dedup identity to byte-identical post-policy stored payload bytes. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1816-1830`, `08-CONTEXT.md`] This difference is safer for libbsa native emission because it only shares records when the exact bytes at the shared offset match both entries' native stored payload requirements. [VERIFIED: `08-CONTEXT.md`, `09-CONTEXT.md`]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Phase 8 generic `LBSW` harness | Production native BSA serialization | Phase 9 scope lock on 2026-05-07. [VERIFIED: `09-CONTEXT.md`, `09-SPEC.md`] | Planner must create native BSA finalizers and keep harness test-only. [VERIFIED: `09-CONTEXT.md`] |
| Extension/name-inferred compression | Explicit archive default option + per-entry semantic policy | Locked in Phase 9 context. [VERIFIED: `09-CONTEXT.md`] | Writer computes native XOR flags from semantic policy. [VERIFIED: `09-CONTEXT.md`, `src/bsa_reader.cpp`] |
| Directory-recursive packing helper | Explicit host-file plus archive-path mappings | Locked in Phase 9 context. [VERIFIED: `09-CONTEXT.md`] | Planner should not add traversal, symlink, include/exclude, or ordering policy helpers. [VERIFIED: `09-CONTEXT.md`] |
| Broad external corpus proof | Generated read-after-write proof through libbsa | Phase 9 / Phase 11 boundary. [VERIFIED: `09-SPEC.md`, `.planning/ROADMAP.md`] | Planner should use generated fixtures now and leave BSArchPro corpus comparison to Phase 11. [VERIFIED: `09-SPEC.md`] |

**Deprecated/outdated:**
- Public C++23 `std::expected` remains inappropriate for this C++20 API; use `libbsa::result`. [VERIFIED: `.planning/STATE.md`, `include/libbsa/result.hpp`]
- Public DirectXTex/libdeflate/LZ4/TES5Edit/platform types remain forbidden in BSA writer headers. [VERIFIED: `AGENTS.md`, `09-SPEC.md`]
- Treating BSA dedup as unproven is now outdated for these targets; reference code shows shared `Size`/`Offset` for TES3/TES4/FO3/SSE when sharing is enabled. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1017-1036`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Exact public names such as `bsa_writer.hpp`, `bsa_write_target`, `bsa_write_plan`, `plan_bsa_write`, and `finalize_bsa_write`. | Summary / Architecture Patterns / Project Structure | Low: Phase 9 context leaves exact type and function names to planner discretion. |
| A2 | BSA writer tests may be a new `libbsa_bsa_writer_tests` target rather than extending `libbsa_writer_tests`. | Standard Stack / Validation Architecture | Low: CMake organization is planner discretion if labels and commands remain clear. |
| A3 | Multi-folder TES4 table placement is the most likely offset bug. | Common Pitfalls | Medium: inferred from format complexity and tests should validate it either way. |

## Open Questions

1. **Should TES3 hash table byte order be changed to match TES5Edit exactly or kept reader-compatible first?**
   - What we know: TES5Edit writes `Hash shr 32` then `Hash and $FFFFFFFF` as two 32-bit values, while the existing test helper emits `append_u64(hash)` and the current reader reads `le_u64`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1612-1616`, `tests/bsa_reader_tests.cpp:141-143`, `src/bsa_reader.cpp:273-285`]
   - What's unclear: Whether prior golden-vector tests intentionally normalized this byte-order difference or missed raw hash table byte assertions. [ASSUMED]
   - Recommendation: Make the first TES3 writer plan include a failing byte-level test for known hash table bytes, then adjust writer and reader tests together if reference byte order differs from current assumptions. [VERIFIED: `09-SPEC.md`]

2. **Which TES4-family file flags should be automatically computed versus caller-specified?**
   - What we know: BSArchPro computes file flags from folders/extensions and applies some version-specific masks/flags. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1410-1525`]
   - What's unclear: Phase 9 SPEC requires flags to be correct but does not lock a public caller override shape. [VERIFIED: `09-SPEC.md`]
   - Recommendation: Start with deterministic reference-style computed `FileFlags` from path/extension and expose them in the plan; add public override only if tests or user requirements need it. [ASSUMED]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build and explicit source wiring | ✓ | 4.3.2 [VERIFIED: shell audit] | — |
| CTest | Test execution | ✓ | 4.3.2 [VERIFIED: shell audit] | — |
| Git | TES5Edit boundary/status gate and optional commit | ✓ | 2.54.0.windows.1 [VERIFIED: shell audit] | — |
| vcpkg executable | Dependency restore via toolchain | ✓ | `2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1` at `C:\vcpkg\vcpkg.exe` [VERIFIED: shell audit] | Existing CMake toolchain path in preset. [VERIFIED: `CMakePresets.json`] |
| MSVC `cl` on current PATH | Direct local compile from plain shell | ✗ | — [VERIFIED: shell audit] | Use Visual Studio generator/preset or developer shell. [VERIFIED: `CMakePresets.json`] |

**Missing dependencies with no fallback:**
- None for planning. [VERIFIED: environment audit]

**Missing dependencies with fallback:**
- `cl` is not on PATH in this shell; use configured Visual Studio/CMake preset environment. [VERIFIED: shell audit, `CMakePresets.json`]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 via existing CMake/vcpkg test setup. [VERIFIED: `CMakeLists.txt`] |
| Config file | `CMakeLists.txt` with explicit `add_executable` and `catch_discover_tests`. [VERIFIED: `CMakeLists.txt`] |
| Quick run command | `ctest --preset windows-msvc-vcpkg -R "libbsa_bsa_writer_tests|libbsa_writer_tests|libbsa_bsa_reader_tests|libbsa_compression_policy_tests|libbsa.public_header_smoke" --output-on-failure` [ASSUMED] |
| Full suite command | `ctest --preset windows-msvc-vcpkg --output-on-failure` [VERIFIED: `CMakePresets.json`] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| WRT-01 | v103/v104/v105 BSA writer emits native headers/tables/hashes/flags/compression/embedded-name payloads and read-after-write succeeds. [VERIFIED: `09-SPEC.md`] | unit/fixture/codec/roundtrip | `ctest --preset windows-msvc-vcpkg -R libbsa_bsa_writer_tests --output-on-failure` [ASSUMED] | ❌ Wave 0 |
| WRT-01 | Disk and memory inputs produce read-back-equivalent archives. [VERIFIED: `09-SPEC.md`] | fixture/roundtrip | `ctest --preset windows-msvc-vcpkg -R libbsa_bsa_writer_tests --output-on-failure` [ASSUMED] | ❌ Wave 0 |
| WRT-04 | TES3 writer emits hash-sorted records, correct name offsets/hash table, raw payloads, and data-section-relative offsets. [VERIFIED: `09-SPEC.md`] | unit/fixture/roundtrip | `ctest --preset windows-msvc-vcpkg -R libbsa_bsa_writer_tests --output-on-failure` [ASSUMED] | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** Run focused BSA writer/read/compression/public-smoke regex above. [ASSUMED]
- **Per wave merge:** Run `ctest --preset windows-msvc-vcpkg --output-on-failure`. [VERIFIED: `CMakePresets.json`]
- **Phase gate:** Full suite green, BSA writer tests green, writer-core tests green, BSA reader tests green, compression policy tests green, public-header smoke green, private-token grep over public headers clean, and `git status --short TES5Edit` clean. [VERIFIED: `09-SPEC.md`, `AGENTS.md`]

### Wave 0 Gaps

- [ ] Public BSA writer declarations — explicit target enum/value, options, separate memory/disk entry types, native plan records, and plan/finalize functions. [VERIFIED: `09-SPEC.md`, `09-CONTEXT.md`]
- [ ] Private BSA writer implementation — native TES3/TES4-family planning, table serialization, payload shaping, dedup compatibility, and sink finalization. [VERIFIED: `09-SPEC.md`]
- [ ] BSA writer test target/file — generated native BSA layout assertions and read-after-write round trips. [VERIFIED: `09-SPEC.md`]
- [ ] Public-header smoke update — create one TES3, v103, v104, and v105 archive using public headers only. [VERIFIED: `09-SPEC.md`, `tests/public_header_smoke.cpp`]
- [ ] CMake explicit source/header/test wiring — no globbing and no `TES5Edit/` inclusion. [VERIFIED: `CMakeLists.txt`, `AGENTS.md`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | No authentication surface in local archive writer library. [VERIFIED: `09-SPEC.md`] |
| V3 Session Management | no | No session or network state. [VERIFIED: `09-SPEC.md`] |
| V4 Access Control | no | No authorization boundary; callers provide files/bytes. [VERIFIED: `09-SPEC.md`] |
| V5 Input Validation | yes | Normalize archive paths, reject duplicates/missing disk files/unsupported targets/options, and check all table/payload arithmetic. [VERIFIED: `09-SPEC.md`, `include/libbsa/archive_path.hpp`] |
| V6 Cryptography | no | Dedup may use implementation-private hashing/byte comparisons but exposes no cryptographic API and must not rely on public hashes for security. [VERIFIED: `08-CONTEXT.md`, `09-CONTEXT.md`] |

### Known Threat Patterns for C++ BSA Writer

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Layout integer overflow or truncation to 32-bit BSA fields | Tampering / Denial of Service | Checked add/multiply and explicit `uint32_t` fit checks before finalization. [VERIFIED: `09-SPEC.md`, `src/writer.cpp`] |
| Host path traversal or archive path confusion | Tampering | Separate host disk path values from archive virtual paths; normalize archive path through `normalize_archive_path`. [VERIFIED: `09-CONTEXT.md`, `include/libbsa/archive_path.hpp`] |
| Codec confusion | Tampering / Denial of Service | Route through existing codec dispatcher; v103/v104 deflate and v105 LZ4-frame only. [VERIFIED: `src/compression.cpp`, `09-SPEC.md`] |
| Silent compression/embedded-name downgrade | Repudiation / Integrity | Return structured `unsupported_format` / `malformed_archive` during planning. [VERIFIED: `09-CONTEXT.md`] |
| Partial/ignored sink failure | Repudiation / Integrity | Return the first `byte_sink::write` failure unchanged and test custom failing sinks. [VERIFIED: `include/libbsa/io.hpp`, `tests/writer_core_tests.cpp`] |
| Untrusted disk file size changes during planning | Tampering | Read disk bytes during planning and store planned payload bytes; finalization never reopens files. [VERIFIED: `09-CONTEXT.md`] |

## Sources

### Primary (HIGH confidence)
- `.planning/phases/09-bsa-writers/09-CONTEXT.md` — locked decisions, disk input model, compression/embedded-name policy, ordering, dedup boundary, canonical references. [VERIFIED]
- `.planning/phases/09-bsa-writers/09-SPEC.md` — locked requirements, acceptance criteria, constraints, and phase boundaries. [VERIFIED]
- `.planning/REQUIREMENTS.md` — WRT-01 and WRT-04 definitions and traceability. [VERIFIED]
- `.planning/ROADMAP.md` — Phase 9 goal, sequencing, dependencies, and Phase 10/11 boundaries. [VERIFIED]
- `.planning/STATE.md` — carry-forward writer-core and project decisions. [VERIFIED]
- `AGENTS.md` — TES5Edit boundary, dependency policy, public API policy, documentation/comment policy, and validation expectations. [VERIFIED]
- `include/libbsa/bsa.hpp`, `writer.hpp`, `archive.hpp`, `archive_path.hpp`, `compression.hpp`, `io.hpp` — public API primitives and writer-core contracts. [VERIFIED]
- `src/bsa_reader.cpp`, `src/bsa_reader.hpp`, `src/hash.cpp`, `src/compression.cpp`, `src/writer.cpp` — reader expectations, constants, hash functions, codec routing, and writer-core behavior. [VERIFIED]
- `TES5Edit/Core/wbBSArchive.pas` — read-only BSArchPro/TES5Edit reference for BSA constants, hashing, sorting, table layout, compression flags, embedded names, dedup, and save behavior. [VERIFIED]
- `tests/bsa_reader_tests.cpp`, `tests/writer_core_tests.cpp`, `tests/compression_policy_tests.cpp`, `tests/public_header_smoke.cpp`, `CMakeLists.txt` — generated fixture style, existing validation, and build/test wiring. [VERIFIED]

### Secondary (MEDIUM confidence)
- `.planning/research/STACK.md` — current dependency/package version research for libdeflate, lz4, DirectXTex, Catch2, CMake, and vcpkg. [VERIFIED]

### Tertiary (LOW confidence)
- Assumed exact API/file/test target names and planner slicing choices; all are listed in the Assumptions Log. [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Phase 9 uses existing codebase primitives and dependencies, with no new packages. [VERIFIED]
- Architecture: HIGH — plan/finalize semantics and BSA-specific API decisions are locked, and native layouts are verified against current reader plus TES5Edit reference. [VERIFIED]
- Pitfalls: HIGH — most pitfalls are direct mismatches between current reader/reference field semantics and likely writer mistakes. [VERIFIED]
- Dedup: HIGH — reference proves shared offsets for required BSA targets; libbsa's post-policy-byte rule is locked from Phase 8. [VERIFIED]
- Environment: MEDIUM — local tools were audited, but Visual Studio compiler availability depends on shell/preset environment. [VERIFIED]

**Research date:** 2026-05-07  
**Valid until:** 2026-06-06 for project/layout decisions; re-check tool/dependency versions before dependency or CI changes. [ASSUMED]
