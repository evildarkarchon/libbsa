# Phase 08: ba2-gnrl-write-new-support - Research

**Researched:** 2026-05-09  
**Domain:** C++20 BA2 GNRL write-new archive serialization, compression routing, and reader-backed validation  
**Confidence:** HIGH for project integration and BA2 GNRL layout; MEDIUM for Starfield v3 GNRL real-game compatibility

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Public API Surface
- **D-01:** Expose a dedicated BA2 GNRL writer object, not a generic BA2 writer shell. Phase 9 should design BA2 DX10/DDS writer behavior separately when texture writing is in scope.
- **D-02:** Mirror the Phase 7 writer-object flow: construct with target/options, add entries with explicit archive-internal paths, copy memory-buffer entries into writer-owned state, and finalize to a host-path archive.
- **D-03:** Keep Phase 8 output host-path only through a `write_to`-style operation. Do not add memory-output or generic sink-output writer destinations in this phase.
- **D-04:** Reuse the existing public `archive_compression_policy` and `entry_compression_policy` enums. The selected BA2 target/profile maps `compressed` to the correct internal codec.
- **D-05:** Reject duplicate canonical BA2 paths at write time with a structured error, matching Phase 7. Do not switch to add-time rejection.
- **D-06:** Preserve the existing dependency-light public boundary: no public libdeflate, lz4, DirectXTex, Windows SDK, `std::expected`, TES5Edit, or private parser/codec types.

#### Starfield Fields
- **D-07:** Starfield writer options use mixed defaults: deterministic profile defaults are provided, and callers may override Starfield `Unknown1`, `Unknown2`, and `CompressionMethod` where applicable.
- **D-08:** Starfield v3 remains one target profile with `CompressionMethod` controlled by writer options. Do not split v3 into separate deflate and raw-LZ4 target profiles.
- **D-09:** Default Starfield `Unknown1` and `Unknown2` values must be reference-derived. Researcher/planner must trace TES5Edit/BSArchPro, generated fixtures, and external BA2 references before locking constants, and implementation should document the compatibility reason near the defaults.
- **D-10:** Starfield v3 `CompressionMethod` is archive-wide. Per-entry compression overrides choose raw versus compressed only; compressed entries use the archive-level method selected by the target/options.
- **D-11:** Method `3` compressed Starfield v3 entries use raw LZ4 block compression. Method `0` compressed entries use deflate. Unsupported methods remain unsupported unless reference evidence justifies them.

#### Record Metadata
- **D-12:** Derive the BA2 GNRL 4-byte extension field from the archive path extension without the dot, pad as needed, and validate unsupported or invalid cases instead of truncating silently.
- **D-13:** Use deterministic safe defaults for the BA2 GNRL unknown/record-flags field, but allow an optional advanced per-entry record-flags override when needed for compatibility evidence.
- **D-14:** The writer computes BA2 name and directory hash fields from archive paths using libbsa hash helpers. Do not expose public hash override knobs.
- **D-15:** The writer owns payload offsets, packed/raw sizes, and the fixed `BAADF00D` sentinel. These are not caller-controlled public fields.
- **D-16:** Existing public `entry_metadata` fields are sufficient for writer-output verification: use `archive_hash`, `record_flags`, sizes, offsets, compression, `path`, and `original_path`. Do not add new BA2-specific public entry metadata in Phase 8 unless implementation proves an acceptance criterion is impossible without it.

#### Ordering and Layout
- **D-17:** Prefer reference-compatible record/name-table ordering. Researcher/planner must trace TES5Edit/BSArchPro or known BA2 behavior and prefer hash/reference ordering; if evidence is inconclusive, use a deterministic canonical-path fallback and document the fallback.
- **D-18:** Records and filename-table entries must stay paired by index in the selected order. Public `entries()` may remain sorted by canonical path after reopening.
- **D-19:** Serialize filename-table paths using caller-provided archive spelling with separators normalized to `/`. Canonical lowercase paths are for validation, lookup-key derivation, and duplicate detection only.
- **D-20:** Writer output physically follows the Phase 8 SPEC shape: header and records first, stored payload bytes next, and the length-prefixed filename table at end of archive with `FileTableOffset` pointing to it.
- **D-21:** When deduplication is enabled, entries are encoded in the selected record order; the first eligible byte-identical final stored payload owns the payload bytes, and later duplicates share that offset.
- **D-22:** Ordering tests should assert structure plus reader-backed round trip: `FileTableOffset` after payloads, paired record/name order, payload offsets, dedupe metadata, reopen/list/find/contains, and extract byte equality. Do not require full archive byte-golden tests for Phase 8.

#### Carry-Forward Decisions
- **D-23:** Preserve Phase 7 final-stored-byte deduplication semantics. Deduplication is disabled by default and compares final stored payload bytes after compression and all metadata-affecting encoding decisions, not source bytes alone.
- **D-24:** Preserve BA2 reader path semantics from Phase 5 and Phase 6: canonical lowercase `/` lookup keys, `original_path` preservation, duplicate canonical path rejection, and `not_found` for valid missing lookup paths.
- **D-25:** Preserve metadata-driven compression routing from earlier phases. Compression selection must come from explicit target/options, raw-vs-packed state, and Starfield `CompressionMethod`, never file extension or archive path guessing.
- **D-26:** Preserve reader-backed validation as the writer acceptance oracle. Writer tests must reopen produced archives with `archive_reader` and assert public metadata, lookup behavior, extraction bytes, stable errors, and the physical layout facts required by SPEC.
- **D-27:** Preserve generated/legal test policy. Do not mutate, format, compile, stage, or use `TES5Edit/` as a fixture workspace.
- **D-28:** Preserve stable error-code testing. Tests should assert `error_code` values and public metadata, not diagnostic message text.

### the agent's Discretion
- Researcher/planner may choose exact public names such as target enum, options struct, and writer class names, provided the API remains dedicated to BA2 GNRL and satisfies the decisions above.
- Researcher/planner may choose exact private source/header file layout, CMake registration, and Catch2 test organization if public headers remain dependency-light and existing tests stay green.
- Researcher/planner may choose exact synthetic entry paths and payload bytes for writer-output tests, provided disk source, memory source, zero-byte, mixed-case lookup, multi-folder, raw/compressed, Starfield v2/v3 metadata, method `0`, method `3`, end filename table, and dedupe acceptance cases are covered.

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WBA2-01 | Consumer can create new Fallout 4 BA2 GNRL archives from disk files or memory buffers. | Use a dedicated `ba2_gnrl_writer` mirroring `tes4_bsa_writer` with `add_file`, copied `add_bytes`, and host-path `write_to`. [VERIFIED: `include/libbsa/writer.hpp`; `08-SPEC.md`] |
| WBA2-02 | Consumer can create new Starfield BA2 GNRL archives with explicit target version and compression method policy. | Provide target profiles for FO4 v1, Starfield v2, and Starfield v3; expose Starfield options for `Unknown1`, `Unknown2`, and v3 `CompressionMethod`. [VERIFIED: `08-CONTEXT.md`; `TES5Edit/Core/wbBSArchive.pas:293-302`] |
| WBA2-03 | Writer can serialize BA2 filename tables at the end of the archive. | Emit header+records first, payloads next, then UInt16-length-prefixed names; update reader host-file parsing to read end tables without treating payload span as metadata. [VERIFIED: `08-SPEC.md`; CITED: https://miere.ru/posts/ba2-archive-format/] |
| WBA2-04 | Writer can compress BA2 GNRL entries with deflate or raw LZ4 block according to target format/version. | Reuse `detail::compress_payload` with `deflate` for FO4/SFv2/SFv3 method 0 and `lz4_block` for SFv3 method 3. [VERIFIED: `src/detail/compression_router.cpp`; `src/formats/ba2/ba2_format_detector.cpp`] |
| WBA2-05 | Writer can preserve or set version-specific BA2 header fields according to documented target profiles. | Default Starfield unknown fields to TES5Edit write defaults `Unknown1=1`, `Unknown2=0`; allow overrides; default SFv3 `CompressionMethod` to method 3 unless caller selects method 0. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1673-1676`, `1707-1710`; MEDIUM for SFv3 GNRL applicability due Wrye Bash note] |
</phase_requirements>

## Summary

Phase 8 should be planned as a writer-on-top-of-reader phase, not as a new format stack. The project already has the public compression policy enums, BA2 metadata optionals, FO4/BA2 hash helper, exact-size deflate/raw-LZ4 codecs, checked little-endian writer, BA2 GNRL reader/extractor, and Phase 7 writer object pattern. [VERIFIED: `include/libbsa/writer.hpp`; `include/libbsa/archive.hpp`; `src/detail/*`; `src/formats/ba2/*`] The work is primarily to add a dedicated BA2 GNRL public writer surface, serialize BA2 v1/v2/v3 GNRL records and an end-of-archive name table, then adjust the existing BA2 GNRL reader so the new end-table layout reopens successfully. [VERIFIED: `08-SPEC.md`; `src/formats/ba2/ba2_gnrl_parser.cpp`]

The highest-risk planning item is the reader/parser gap for end-of-archive name tables: current host-file BA2 GNRL parsing computes the filename table byte range as `first_payload_offset - FileTableOffset` and rejects `FileTableOffset > first_payload_offset`, which directly conflicts with Phase 8's required payload-before-name-table layout. [VERIFIED: `src/formats/ba2/ba2_gnrl_parser.cpp:390-411`; `08-SPEC.md:27-30`] Plan reader support early, before writer serialization tasks depend on reopen tests. [VERIFIED: `08-CONTEXT.md:130`]

**Primary recommendation:** Build `ba2_gnrl_writer` by mirroring Phase 7's writer-owned state and transactional host-path output, but implement BA2-specific record preparation around canonical path hashing, target-profile header fields, explicit deflate/raw-LZ4 routing, and end filename-table reader support. [VERIFIED: `src/formats/bsa/tes4_bsa_writer.cpp`; `08-CONTEXT.md`]

## Project Constraints (from AGENTS.md)

- The implementation language is C++; public APIs should be clean, portable C++ rather than Delphi/Pascal transliteration. [VERIFIED: `AGENTS.md`]
- `TES5Edit/` is read-only reference material: do not edit, format, apply generated changes, update the submodule pointer, stage changes, compile it into libbsa, or use it as a mutable fixture workspace. [VERIFIED: `AGENTS.md`]
- Implementation work belongs outside `TES5Edit/`. [VERIFIED: `AGENTS.md`]
- Preserve archive-format behavior discovered from BSArchPro unless a divergence is documented. [VERIFIED: `AGENTS.md`]
- When porting behavior, trace the reference code first and record non-obvious compatibility constraints near the new implementation. [VERIFIED: `AGENTS.md`]
- Use `libdeflate` for deflate and official `lz4` for LZ4; use vcpkg; do not add dependencies speculatively. [VERIFIED: `AGENTS.md`; `vcpkg.json`]
- Public headers must not leak libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, or C++23 `std::expected`. [VERIFIED: `AGENTS.md`; `.planning/PROJECT.md`; `08-SPEC.md`]
- Add Doxygen-compliant C++ doc comments for public APIs and methods added or substantially rewritten; add comments for non-obvious compatibility, ownership/lifetime, error, or deliberate reference-divergence rules. [VERIFIED: `AGENTS.md`]
- Add focused archive parsing/writing/round-trip/compatibility tests, using fixture-based byte/metadata proof; do not use `TES5Edit/` as a mutable fixture. [VERIFIED: `AGENTS.md`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Public BA2 GNRL writer object | Library Public API | Private writer implementation | Consumers need a dependency-light C++20 surface; serialization details remain private. [VERIFIED: `08-CONTEXT.md`; `include/libbsa/writer.hpp`] |
| Target profile and Starfield fields | Private writer implementation | Public options struct | Public options select profile/overrides; private code owns header layout and metadata mapping. [VERIFIED: `08-CONTEXT.md`; `TES5Edit/Core/wbBSArchive.pas:293-302`] |
| Archive path validation and duplicate detection | Private writer state | Public API returns `result` errors | Existing path helper owns canonical lookup semantics and write-time duplicate rejection matches Phase 7. [VERIFIED: `src/detail/archive_path.cpp`; `src/formats/bsa/tes4_bsa_writer.cpp`] |
| BA2 GNRL record serialization | Private writer implementation | Binary I/O detail layer | Records need checked little-endian writes, hash fields, extension FourCC, offsets, sizes, flags, and sentinel. [VERIFIED: `src/detail/binary_io.hpp`; `src/formats/ba2/ba2_gnrl_parser.cpp`] |
| Payload encoding and dedupe | Private writer implementation | Compression router | Writer decides raw/compressed, computes final stored bytes, and dedupes stored bytes after compression. [VERIFIED: `src/detail/compression_router.cpp`; `src/formats/bsa/tes4_bsa_writer.cpp:468-494`] |
| Reopen/list/find/extract validation | Public reader facade | BA2 parser/reader internals | Writer acceptance must be proved through `archive_reader`, not writer internals. [VERIFIED: `08-CONTEXT.md`; `tests/unit/tes4_bsa_writer_tests.cpp`] |
| Build/test registration | CMake/test harness | — | Public header file set and private source/test registrations must include the writer files. [VERIFIED: `CMakeLists.txt`; `tests/CMakeLists.txt`] |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++ | C++20 | Public writer API and private serialization implementation | Project mandate; existing writer and reader APIs are C++20 and use `libbsa::result`. [VERIFIED: `.planning/PROJECT.md`; `include/libbsa/writer.hpp`] |
| CMake | 4.0 minimum; local 4.3.2 available | Build/test source registration | Existing root project uses CMake 4.0 and local tool reports 4.3.2. [VERIFIED: `CMakeLists.txt`; `cmake --version`] |
| vcpkg manifest mode | baseline `12dcccadfe573d0eaa6c67a968413ded7805d256` | Dependency acquisition | Project has committed `vcpkg.json` and `vcpkg-configuration.json`. [VERIFIED: `vcpkg.json`; `vcpkg-configuration.json`] |
| libdeflate | vcpkg `1.25#0`, release v1.25 | BA2 deflate compression/decompression | vcpkg package is current at 1.25 and libdeflate supports whole-buffer raw DEFLATE compression/decompression. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://raw.githubusercontent.com/ebiggers/libdeflate/v1.25/libdeflate.h] |
| lz4 | vcpkg `1.10.0#0`, release v1.10.0 | Starfield v3 raw LZ4 block compression/decompression | Official `lz4.h` handles blocks, not frames, using `LZ4_compress_default` and `LZ4_decompress_safe`; Phase 8 method 3 needs raw blocks. [CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://raw.githubusercontent.com/lz4/lz4/v1.10.0/lib/lz4.h; VERIFIED: Context7 `/lz4/lz4`] |
| Catch2 + CTest | Catch2 via vcpkg; CTest local 4.3.2 | Writer and reader-backed tests | Existing tests are Catch2 and CTest with label propagation. [VERIFIED: `tests/CMakeLists.txt`; `ctest --version`] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| nlohmann-json | vcpkg manifest dependency | Existing manifest-driven fixture tests | Use only in tests; do not leak to public/runtime library. [VERIFIED: `vcpkg.json`; `tests/CMakeLists.txt`] |
| DirectXTex | vcpkg manifest dependency | Existing BA2 DX10 read validation | Do not use for Phase 8 GNRL writer; it is for DDS/DX10 phases. [VERIFIED: `vcpkg.json`; `08-SPEC.md`] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Dedicated `ba2_gnrl_writer` | Generic `ba2_writer` | Locked out: Phase 8 excludes DX10 writer behavior and requires a dedicated GNRL writer. [VERIFIED: `08-CONTEXT.md`] |
| `detail::compress_payload` | Direct libdeflate/lz4 calls from BA2 writer | Hand-wiring codecs duplicates tested router behavior and increases risk of using LZ4 frame for Starfield raw blocks. [VERIFIED: `src/detail/compression_router.cpp`; `08-CONTEXT.md`] |
| Reader-backed tests | Writer-internal byte assertions only | Locked out: acceptance oracle is `archive_reader` reopen/list/find/extract/byte-compare. [VERIFIED: `08-CONTEXT.md`; `08-SPEC.md`] |
| End filename table | Existing generated fixture layout with names before payload | Locked out: Phase 8 explicitly requires payloads before name table and `FileTableOffset` at the end. [VERIFIED: `08-SPEC.md`; `generate_ba2_gnrl_fixtures.cpp`] |

**Installation:** no new packages are required; use existing manifest dependencies. [VERIFIED: `vcpkg.json`]

```bash
cmake --preset <existing-preset>
cmake --build --preset <existing-build-preset>
ctest --preset <existing-test-preset>
```

**Version verification:** npm is not applicable to this C++/vcpkg project. vcpkg pages verify `libdeflate 1.25#0` updated 2025-11-03 and `lz4 1.10.0#0` updated 2024-07-25; GitHub releases verify libdeflate v1.25 and LZ4 v1.10.0. [CITED: https://vcpkg.io/en/package/libdeflate.html; CITED: https://vcpkg.io/en/package/lz4.html; CITED: https://github.com/ebiggers/libdeflate/releases/tag/v1.25; CITED: https://github.com/lz4/lz4/releases/tag/v1.10.0]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer code
  |
  | construct ba2_gnrl_writer(target, options)
  v
Public writer API (dependency-light C++20 result<T>)
  |
  | add_file/archive_path + add_bytes/archive_path
  v
Writer-owned entry state
  |
  | normalize canonical key; preserve original spelling with '/'
  v
Write-to validation gate
  |
  | duplicate canonical paths? missing sources? invalid target/method?
  +--> structured error_code result
  |
  v
Prepare ordered BA2 entries
  |
  | hash path/name/folder; derive extension FourCC; encode raw/compressed payload
  v
Payload layout + optional final-stored-byte dedupe
  |
  | assign archive-absolute offsets
  v
Serialize BTDX/GNRL header + GNRL records + payload bytes + end filename table
  |
  v
Temporary output archive -> atomic publish to destination
  |
  v
archive_reader::open(output) round-trip tests
  |
  | parse end filename table, list/find/contains, extract, byte-compare
  v
Acceptance evidence
```

### Recommended Project Structure

```text
include/libbsa/
├── writer.hpp                       # Add ba2_gnrl_target/options/writer public surface
└── libbsa.hpp                       # Already includes writer.hpp; keep public boundary green
src/formats/ba2/
├── ba2_gnrl_writer.hpp              # Private writer entry structs and write function
├── ba2_gnrl_writer.cpp              # BA2 header/record/payload/name-table serialization
├── ba2_gnrl_parser.cpp              # Adjust host-file parser for end filename tables
└── ba2_gnrl_reader.cpp              # Reuse extraction unchanged unless metadata shape requires it
├── ba2_gnrl_writer_tests.cpp        # New public API + round-trip tests
└── public_include_boundary_tests.cpp # Extend installed/public header boundary coverage
```

### Pattern 1: Mirror Phase 7 Writer Object

**What:** Add a public writer class with constructors, `target()`, `options()`, `add_file`, `add_bytes`, and `write_to`; `add_bytes` copies caller memory into writer-owned state. [VERIFIED: `include/libbsa/writer.hpp`; `src/formats/bsa/tes4_bsa_writer.cpp`]  
**When to use:** Use for all Phase 8 consumer-facing creation flows. [VERIFIED: `08-CONTEXT.md`]  
**Example:**

```cpp
/// Public writer for creating new BA2 GNRL archives.
class ba2_gnrl_writer {
 public:
  explicit ba2_gnrl_writer(ba2_gnrl_target target);
  explicit ba2_gnrl_writer(ba2_gnrl_target target, ba2_gnrl_writer_options options);
  result<void> add_file(std::string_view archive_path,
                        std::string_view host_path,
                        entry_compression_policy compression = entry_compression_policy::inherit);
  result<void> add_bytes(std::string_view archive_path,
                         std::span<const std::byte> bytes,
                         entry_compression_policy compression = entry_compression_policy::inherit);
  result<void> write_to(std::string_view host_path) const;
};
```

### Pattern 2: Target-Profile Mapping Owns Codec Selection

**What:** `compressed` maps to deflate for FO4 v1, SFv2, and SFv3 method 0; it maps to raw LZ4 block for SFv3 method 3. [VERIFIED: `08-CONTEXT.md`; `src/formats/ba2/ba2_format_detector.cpp`]  
**When to use:** Use in payload preparation; never infer codec from extension. [VERIFIED: `08-CONTEXT.md`]  
**Example:**

```cpp
detail::compression_method method_for_compressed_entry(ba2_gnrl_target target,
                                                       std::uint32_t compression_method) {
  if (target == ba2_gnrl_target::starfield_v3 && compression_method == 3U) {
    return detail::compression_method::lz4_block;
  }
  return detail::compression_method::deflate;
}
```

### Pattern 3: Store Raw Entries with `PackedSize == 0`

**What:** BA2 GNRL parser treats `packed_size == 0` as raw; otherwise the archive profile default compression is used. [VERIFIED: `src/formats/ba2/ba2_gnrl_parser.cpp:198-203`]  
**When to use:** Use for raw overrides and zero-byte entries. [VERIFIED: `08-SPEC.md`; `tests/unit/ba2_gnrl_reader_tests.cpp`]  
**Example:**

```cpp
record.packed_size = effective_compressed ? checked_u32(stored_payload.size(), "BA2 packed payload") : 0U;
record.raw_size = checked_u32(raw_payload.size(), "BA2 raw payload");
```

### Pattern 4: Separate Physical Record Order from Public Listing Order

**What:** Serialize records and filename-table entries in one paired physical order; after reopening, `entries()` may return canonical-path sorted metadata. [VERIFIED: `08-CONTEXT.md`; `src/formats/ba2/ba2_gnrl_parser.cpp:264-267`]  
**When to use:** Use whenever asserting physical layout or dedupe “first owner” behavior. [VERIFIED: `08-CONTEXT.md`]  
**Recommendation:** Use deterministic canonical-path order for Phase 8 if reference evidence does not prove a separate BA2 hash-order requirement. TES5Edit’s BA2 writer creates `fFilesFO4` in the provided file-list order and does not show BA2-specific sorting in the traced block. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1528-1546`]

### Anti-Patterns to Avoid

- **Treating `FileTableOffset` as “before first payload”:** Current reader host-file optimization does this, but Phase 8 requires end tables; fix reader parsing first. [VERIFIED: `src/formats/ba2/ba2_gnrl_parser.cpp:390-411`; `08-SPEC.md`]
- **Using LZ4 frame APIs for Starfield v3 method 3:** LZ4 docs say `lz4.h` block APIs are distinct from frame APIs, and Wrye Bash/xEdit evidence identifies Starfield v3 as raw LZ4 block. [CITED: https://raw.githubusercontent.com/lz4/lz4/v1.10.0/lib/lz4.h; CITED: https://github.com/wrye-bash/wrye-bash/issues/667; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1143-1144`]
- **Comparing compressed bytes to golden outputs:** libdeflate states compressed bytes may differ across versions for the same input; tests should reopen/extract and compare decoded bytes/metadata. [CITED: https://raw.githubusercontent.com/ebiggers/libdeflate/v1.25/libdeflate.h]
- **Exposing hash/offset/sentinel controls publicly:** Phase 8 locks these as writer-owned fields. [VERIFIED: `08-CONTEXT.md`]
- **Silently truncating extensions to four bytes:** Phase 8 locks validation instead of silent truncation. [VERIFIED: `08-CONTEXT.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Archive path normalization | Custom lowercase/slash/path traversal logic | `detail::normalize_archive_path` | Existing readers and Phase 7 writer share canonical lookup semantics. [VERIFIED: `src/detail/archive_path.cpp`; `src/formats/bsa/tes4_bsa_writer.cpp`] |
| FO4/BA2 hashes | Ad hoc CRC/hash code | `detail::hash_fo4` | Existing helper matches TES5Edit-style lower-byte and slash rules. [VERIFIED: `src/detail/bethesda_hash.cpp`] |
| Deflate/LZ4 payload encoding | Direct libdeflate/lz4 calls from BA2 writer | `detail::compress_payload` | Router already handles method enum and keeps dependencies private. [VERIFIED: `src/detail/compression_router.cpp`] |
| Little-endian serialization | Manual pointer casts or packed structs | `detail::binary_writer` | Existing writer avoids host-endian and struct-padding hazards. [VERIFIED: `src/detail/binary_io.hpp`] |
| BA2 reader validation | Writer-internal verification only | `archive_reader::open` + public metadata/extract APIs | Phase 8 acceptance requires reader-backed round trips. [VERIFIED: `08-SPEC.md`] |
| Dependency exposure | Public libdeflate/lz4/DirectXTex types | libbsa-owned enums/options | Public dependency-light boundary is a project invariant. [VERIFIED: `AGENTS.md`; `08-CONTEXT.md`] |

**Key insight:** BA2 GNRL writer correctness depends more on exact layout and existing reader compatibility than on novel algorithms; reusing project helpers prevents extension/path/hash/codec inconsistencies that would make writer output impossible to reopen. [VERIFIED: codebase files cited above]

## Common Pitfalls

### Pitfall 1: End Filename Table Breaks Current Host-File Parser
**What goes wrong:** Writer emits payloads before names, but `archive_reader::open` rejects the archive because `FileTableOffset > first_payload_offset`. [VERIFIED: `src/formats/ba2/ba2_gnrl_parser.cpp:390-411`]  
**Why it happens:** Phase 5 optimized open/list parsing for fixtures where the name table precedes payloads, deriving name-table size from first payload offset. [VERIFIED: `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp:248-263`]  
**How to avoid:** Plan a Wave 0/early task to parse header+records, then seek to `FileTableOffset` and read exactly `file_count` UInt16-prefixed names, stopping after consumed names instead of assuming the table ends at first payload. [VERIFIED: `08-SPEC.md`; `src/formats/ba2/ba2_gnrl_parser.cpp`]  
**Warning signs:** Writer output fails open with `format_error` before extraction tests run. [VERIFIED: current parser behavior]

### Pitfall 2: Confusing Whole Path Hash and Name/Directory Hash Fields
**What goes wrong:** Writer records may not match expected metadata if `NameHash`/`DirHash` are computed from the wrong string components. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1538-1544`; `generate_ba2_gnrl_fixtures.cpp:173-181`]  
**Why it happens:** Current generated fixture uses full canonical path for `archive_hash`, while TES5Edit write code computes `NameHash` from the base name and `DirHash` from directory. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1538-1544`; `generate_ba2_gnrl_fixtures.cpp:173-181`]  
**How to avoid:** Planner should assign a focused hash semantics task: reconcile existing reader fixture expectations with TES5Edit write behavior, then lock the production writer behavior and tests. [VERIFIED: source conflict found in this research]  
**Warning signs:** Reopened `entry_metadata.archive_hash` differs from expected writer hash or external tools cannot locate entries. [VERIFIED: `entry_metadata.archive_hash` is public]

### Pitfall 3: Starfield v3 GNRL Is Structurally Required but Real-Game Evidence Is Mixed
**What goes wrong:** Implementers may assume Starfield v3 GNRL is a vanilla-game archive profile with the same confidence as FO4 v1/SFv2. [CITED: https://github.com/wrye-bash/wrye-bash/issues/667]  
**Why it happens:** Wrye Bash notes valid Starfield BA2 versions 2 and 3, with general BA2s using v2 and texture BA2s using v3; Phase 8 nevertheless explicitly requires structurally valid Starfield v3 GNRL writer output. [CITED: https://github.com/wrye-bash/wrye-bash/issues/667; VERIFIED: `08-SPEC.md`]  
**How to avoid:** Treat SFv3 GNRL as a required structural profile for libbsa round-trip support, document compatibility uncertainty, and keep overrides for method/unknown fields. [VERIFIED: `08-CONTEXT.md`]  
**Warning signs:** Tests overclaim “vanilla Starfield GNRL v3 compatibility” instead of “structurally valid libbsa-supported SFv3 GNRL.” [MEDIUM: external evidence]

### Pitfall 4: Using Compression Size Thresholds Hidden from Public Policy
**What goes wrong:** A caller requests `compressed`, but writer stores raw because compression did not shrink enough. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1841-1848` shows BSArchPro threshold behavior]  
**Why it happens:** TES5Edit/BSArchPro has a “compress only if reduced by 32 bytes unless forced” behavior, but Phase 8 locks explicit policy/overrides and existing Phase 7 avoided hidden size thresholds. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1841-1848`; `07-CONTEXT.md`; `08-CONTEXT.md`]  
**How to avoid:** For Phase 8, honor explicit target/default and per-entry policies; do not silently store raw for non-empty entries when policy says compressed unless compression fails with a structured error. [VERIFIED: `08-CONTEXT.md`; `07-CONTEXT.md`]  
**Warning signs:** Tests for tiny compressed entries see `entry_compression::none` unexpectedly. [VERIFIED: Phase 8 acceptance requires compressed entries]

### Pitfall 5: Dedupe Before Compression
**What goes wrong:** Two entries with identical source bytes but different compression policies share an offset incorrectly. [VERIFIED: `07-CONTEXT.md`; `tests/unit/tes4_bsa_writer_tests.cpp:531-559`]  
**Why it happens:** Source bytes are not necessarily equal to final stored bytes. [VERIFIED: Phase 7 tests]  
**How to avoid:** Dedupe by final stored byte vector after raw/compressed routing and record-affecting decisions; only first owner writes payload bytes. [VERIFIED: `08-CONTEXT.md`; `src/formats/bsa/tes4_bsa_writer.cpp:468-494`]  
**Warning signs:** Raw and compressed duplicates share a payload offset or extraction mismatches one entry. [VERIFIED: Phase 7 test pattern]

## Code Examples

Verified patterns from current project and official sources:

### Add and Copy Writer Entries
```cpp
// Source: J:\libbsa-gsd\src\formats\bsa\tes4_bsa_writer.cpp
result<void> ba2_gnrl_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        entry_compression_policy compression) {
  auto entry = make_ba2_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }
  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}
```

### BA2 GNRL Header Shape
```cpp
// Source: TES5Edit/Core/wbBSArchive.pas lines 287-302 and generate_ba2_gnrl_fixtures.cpp lines 216-229
writer.write_bytes("BTDX");
writer.write_u32_le(version);          // 1, 2, or 3
writer.write_bytes("GNRL");
writer.write_u32_le(file_count);
writer.write_u64_le(file_table_offset);
if (version >= 2) {
  writer.write_u32_le(starfield_unknown1);
  writer.write_u32_le(starfield_unknown2);
}
if (version >= 3) {
  writer.write_u32_le(compression_method);
}
```

### BA2 GNRL Record Shape
```cpp
// Source: src/formats/ba2/ba2_gnrl_parser.cpp lines 151-170; miere BA2 format notes
writer.write_u32_le(record.name_hash);
writer.write_bytes(record.extension_fourcc); // extension without dot, 4 bytes
writer.write_u32_le(record.directory_hash);
writer.write_u32_le(record.record_flags);
writer.write_u64_le(record.payload_offset);
writer.write_u32_le(record.packed_size);     // 0 means raw
writer.write_u32_le(record.raw_size);
writer.write_u32_le(0xBAADF00D);
```

### Raw LZ4 Block Compression
```c
// Source: Context7 /lz4/lz4 and https://raw.githubusercontent.com/lz4/lz4/v1.10.0/lib/lz4.h
int maxDst = LZ4_compressBound(srcSize);
int compressedSize = LZ4_compress_default(src, dst, srcSize, maxDst);
int decodedSize = LZ4_decompress_safe(dst, recovered, compressedSize, srcSize);
```

### Reader-Backed Writer Acceptance
```cpp
// Source: tests/unit/tes4_bsa_writer_tests.cpp pattern
auto written = writer.write_to(output.string());
REQUIRE(written.has_value());

auto opened = libbsa::archive_reader::open(output.string());
REQUIRE(opened.has_value());

auto entry = opened.value().find("Meshes/Example.nif");
REQUIRE(entry.has_value());
REQUIRE(entry.value().has_value());
CHECK(entry.value()->compression == libbsa::entry_compression::none);

auto extracted = opened.value().extract_bytes("Meshes/Example.nif");
REQUIRE(extracted.has_value());
CHECK(extracted.value() == expected_bytes);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| BA2 GNRL fixtures place name table before payloads | Phase 8 writer must place name table at the end after payloads | Phase 8 SPEC, 2026-05-09 | Requires reader host-file parser adjustment before writer round trips pass. [VERIFIED: `08-SPEC.md`; `generate_ba2_gnrl_fixtures.cpp`] |
| Starfield v3 compressed data treated like SSE LZ4 frame | Starfield v3 method 3 uses raw LZ4 block | Starfield support discussions/xEdit; implemented in Phase 5 | Writer must use `compression_method::lz4_block`, not frame. [CITED: https://github.com/wrye-bash/wrye-bash/issues/667; VERIFIED: `src/formats/ba2/ba2_format_detector.cpp`] |
| Writer validation by produced byte-golden archives | Writer validation by reopen/extract/public metadata | Phase 7 and Phase 8 decisions | Avoid brittle compression byte comparisons and proves consumer behavior. [VERIFIED: `07-CONTEXT.md`; `08-CONTEXT.md`] |
| Test generator BA2 serializer | Production `ba2_gnrl_writer` | Phase 8 | Reuse knowledge, not test-only code as public API. [VERIFIED: `generate_ba2_gnrl_fixtures.cpp`; `08-SPEC.md`] |

**Deprecated/outdated:**
- LZ4 frame routing for Starfield v3 method 3 is wrong for Phase 8. [CITED: https://raw.githubusercontent.com/lz4/lz4/v1.10.0/lib/lz4.h; VERIFIED: `src/formats/ba2/ba2_format_detector.cpp`]
- Full compressed byte golden comparisons are brittle because libdeflate does not guarantee stable compressed bytes across library versions. [CITED: https://raw.githubusercontent.com/ebiggers/libdeflate/v1.25/libdeflate.h]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | No claims are intentionally marked `[ASSUMED]`; the main uncertainty is explicitly MEDIUM, not assumed: Starfield v3 GNRL real-game compatibility is not fully proven by current external evidence. | Summary / Pitfalls | Planner should avoid overclaiming vanilla-game compatibility for SFv3 GNRL while still implementing the locked structural profile. |

## Open Questions (RESOLVED)

1. **RESOLVED: Which BA2 hash fields should Phase 8 writer lock for `NameHash` and `DirHash`?**
    - What we know: TES5Edit write code computes `NameHash` from the base name and `DirHash` from the directory, while current generated GNRL fixtures store `archive_hash` from the full canonical path and a separate folder hash. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1538-1544`; `generate_ba2_gnrl_fixtures.cpp:173-181`]
   - Resolution: Production writer behavior should correct toward TES5Edit writer semantics: derive `NameHash` from the base filename and `DirHash` from the directory path using libbsa FO4/BA2 hash helpers, while keeping public lookup behavior path-based and deterministic. Existing generated fixture semantics are test-fixture prior art, not the writer compatibility target. Document the compatibility reason near the writer code and update writer-output tests to assert the reopened public `archive_hash` value that the reader exposes from `NameHash`. [VERIFIED: `AGENTS.md`; `08-CONTEXT.md`]

2. **RESOLVED: Should Starfield v3 GNRL default `CompressionMethod` be 3 or require explicit caller selection?**
    - What we know: TES5Edit sets v3 DDS `CompressionMethod := 3`, and project reader supports v3 method 0 deflate and method 3 raw LZ4. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1707-1710`; `src/formats/ba2/ba2_format_detector.cpp`]
   - Resolution: Starfield v3 GNRL keeps an explicit `CompressionMethod` writer option with a reference-derived default of method `3` for `target_default`, because method `3` is the documented raw LZ4 block path for v3. Tests must cover both method `0` deflate and method `3` raw LZ4 block. Do not overclaim vanilla-game v3 GNRL compatibility; treat v3 GNRL as required structural writer support. [VERIFIED: `08-CONTEXT.md`; `08-SPEC.md`]

3. **RESOLVED: What exact public names should be used?**
    - What we know: Names are discretionary if dedicated to BA2 GNRL and dependency-light. [VERIFIED: `08-CONTEXT.md`]
   - Resolution: Plan around `ba2_gnrl_target`, `ba2_gnrl_writer_options`, and `ba2_gnrl_writer` in `include/libbsa/writer.hpp`, included by `include/libbsa/libbsa.hpp`. Exact member names remain planner/executor discretion if they preserve the dedicated BA2 GNRL writer shape, C++20 result/error style, dependency-light public headers, and Phase 8 decisions. [VERIFIED: current structure; `08-CONTEXT.md`]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build/source registration | ✓ | 4.3.2 | Existing minimum is 4.0. [VERIFIED: `cmake --version`; `CMakeLists.txt`] |
| CTest | Test execution | ✓ | 4.3.2 | Use CMake-bundled CTest. [VERIFIED: `ctest --version`] |
| PowerShell | Local command execution | ✓ | 7.6.1 | — [VERIFIED: `pwsh --version`] |
| vcpkg manifest files | Dependency specification | ✓ | baseline `12dcccad...` | Manifest exists even though `vcpkg` CLI is not on PATH. [VERIFIED: `vcpkg.json`; `vcpkg-configuration.json`; environment probe] |
| vcpkg CLI | Dependency install if configuring fresh | ✗ | — | Use existing configured environment/presets if available, or install/provide vcpkg before fresh configure. [VERIFIED: environment probe] |
| Ninja | Preset generator if required | ✗ | — | Use Visual Studio/MSBuild or install Ninja depending on presets. [VERIFIED: environment probe] |
| MSVC `cl` in current shell | Windows compiler | ✗ | — | Run from Developer PowerShell/VS environment or use configured CI. [VERIFIED: environment probe] |
| TES5Edit submodule cleanliness | Boundary verification | ✓ | clean status | Keep `git -C TES5Edit status --short` empty. [VERIFIED: environment probe] |

**Missing dependencies with no fallback:**
- None for research/planning. [VERIFIED: environment probes]

**Missing dependencies with fallback:**
- vcpkg CLI, Ninja, and MSVC `cl` are not on the current PATH; planner should include normal project configure/test commands but executor may need a developer toolchain shell or existing preset environment. [VERIFIED: environment probes]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 via vcpkg; CTest local 4.3.2. [VERIFIED: `tests/CMakeLists.txt`; `ctest --version`] |
| Config file | `tests/CMakeLists.txt`, root `CMakeLists.txt`. [VERIFIED: files read] |
| Quick run command | `ctest --test-dir <build-dir> -R ba2_gnrl_writer --output-on-failure` [VERIFIED: CTest available; test name planned] |
| Full suite command | `ctest --test-dir <build-dir> --output-on-failure` [VERIFIED: CTest available] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WBA2-01 | Disk-file and memory-buffer BA2 GNRL archives can be created through public API. | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer` | ❌ Wave 0 |
| WBA2-02 | Explicit FO4 v1, Starfield v2, Starfield v3 targets and v3 compression method policy. | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer` | ❌ Wave 0 |
| WBA2-03 | End filename table with `FileTableOffset` after payloads and reader reopen. | integration/parser regression | `ctest --test-dir <build-dir> -R ba2_gnrl_writer` | ❌ Wave 0 |
| WBA2-04 | Deflate and raw LZ4 block compressed entries round-trip by metadata routing. | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer` | ❌ Wave 0 |
| WBA2-05 | Starfield header unknowns and compression method exposed through metadata after reopen. | unit/integration | `ctest --test-dir <build-dir> -R ba2_gnrl_writer` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** targeted `ba2_gnrl_writer`/`ba2_gnrl_reader` CTest filter plus public include boundary if headers change. [VERIFIED: `tests/CMakeLists.txt`]
- **Per wave merge:** full `libbsa_tests`/CTest suite. [VERIFIED: `tests/CMakeLists.txt`]
- **Phase gate:** Full suite green, package consumer smoke green, and `git -C TES5Edit status --short` empty before `/gsd-verify-work`. [VERIFIED: `tests/CMakeLists.txt`; `AGENTS.md`]

### Wave 0 Gaps
- [ ] `tests/unit/ba2_gnrl_writer_tests.cpp` — covers WBA2-01 through WBA2-05 and Phase 8 dedupe/round-trip SPEC requirements. [VERIFIED: no existing file in `tests/CMakeLists.txt`]
- [ ] Extend `tests/unit/public_include_boundary_tests.cpp` — proves public writer API remains dependency-light. [VERIFIED: `08-SPEC.md`; `tests/CMakeLists.txt`]
- [ ] Add `src/formats/ba2/ba2_gnrl_writer.hpp/.cpp` and CMake registration. [VERIFIED: no existing source in `CMakeLists.txt`]
- [ ] Adjust `src/formats/ba2/ba2_gnrl_parser.cpp` host-file path for end filename tables. [VERIFIED: parser conflict]

## Security Domain

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No authentication surface. [VERIFIED: phase scope] |
| V3 Session Management | no | No session state. [VERIFIED: phase scope] |
| V4 Access Control | no | Local library writer; caller controls host paths. [VERIFIED: phase scope] |
| V5 Input Validation | yes | Validate archive paths, duplicate canonical keys, extension FourCC, file counts, sizes, offsets, compression method, and host paths with `result` errors. [VERIFIED: `src/detail/archive_path.cpp`; `08-SPEC.md`] |
| V6 Cryptography | no | No cryptography; hashes are non-cryptographic archive metadata only. [VERIFIED: `src/detail/bethesda_hash.cpp`] |

### Known Threat Patterns for C++ archive writers
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Integer overflow in table/payload offset math | Tampering/DoS | Use checked 64-bit arithmetic and checked u32/u16 conversions before serialization. [VERIFIED: `src/formats/bsa/tes4_bsa_writer.cpp`; `src/detail/binary_io.hpp`] |
| Path traversal or invalid archive virtual paths | Tampering | Use `detail::normalize_archive_path`; never treat archive paths as host filesystem paths. [VERIFIED: `src/detail/archive_path.cpp`] |
| Malicious/corrupt compressed payloads in regression fixtures | DoS/Tampering | Existing extraction uses exact-size deflate/LZ4 decoding and returns structured errors. [VERIFIED: `src/formats/ba2/ba2_gnrl_reader.cpp`] |
| Partial output overwrite on write failure | Tampering | Follow Phase 7 temp-file then publish pattern; default no overwrite unless explicit. [VERIFIED: `src/formats/bsa/tes4_bsa_writer.cpp`] |
| Dependency/header leakage | Information Disclosure / supply-chain surface | Keep libdeflate/lz4 types private and expose only libbsa enums/options. [VERIFIED: `AGENTS.md`; `08-CONTEXT.md`] |

## Sources

### Primary (HIGH confidence)
- `08-CONTEXT.md`, `08-SPEC.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/ROADMAP.md` — locked scope, requirements, decisions, and current project state.
- `AGENTS.md` — project constraints, TES5Edit read-only boundary, dependency policy, documentation/test expectations.
- `include/libbsa/writer.hpp`, `include/libbsa/archive.hpp` — current public writer and metadata API.
- `src/formats/bsa/tes4_bsa_writer.cpp` — Phase 7 writer pattern, payload encoding, dedupe, temp-file publish.
- `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_format_detector.cpp` — current BA2 reader/parser/detector behavior.
- `src/detail/archive_path.cpp`, `src/detail/bethesda_hash.cpp`, `src/detail/binary_io.hpp`, `src/detail/compression_router.cpp` — reusable private services.
- `TES5Edit/Core/wbBSArchive.pas` — read-only behavioral reference for BA2 structs, Starfield fields, compression method handling, and writer defaults.
- Context7 `/lz4/lz4` — raw LZ4 block API examples and block-vs-frame distinction.
- Official raw docs: `https://raw.githubusercontent.com/lz4/lz4/v1.10.0/lib/lz4.h`, `https://raw.githubusercontent.com/ebiggers/libdeflate/v1.25/libdeflate.h`.

### Secondary (MEDIUM confidence)
- `https://miere.ru/posts/ba2-archive-format/` — independent BA2 header/GNRL record/name-table notes.
- `https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html` — independent BTDX GNRL/DX10 split and archive parsing notes.
- `https://github.com/wrye-bash/wrye-bash/issues/667` — Starfield BA2 v2/v3 and raw LZ4 block community/tooling evidence.
- vcpkg package pages: `https://vcpkg.io/en/package/libdeflate.html`, `https://vcpkg.io/en/package/lz4.html`.

### Tertiary (LOW confidence)
- None used for recommendations.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all dependencies and versions were verified from committed manifest, vcpkg pages, and official headers/releases.
- Architecture: HIGH — mostly derived from existing Phase 7 writer, Phase 5/6 BA2 reader, and locked Phase 8 decisions.
- Pitfalls: HIGH for reader end-table and codec routing; MEDIUM for Starfield v3 GNRL real-game compatibility because external evidence suggests vanilla general Starfield BA2s are v2 while Phase 8 requires v3 structural support.

**Research date:** 2026-05-09  
**Valid until:** 2026-06-08 for project/codebase findings; 2026-05-16 for dependency-version currency and Starfield external-format ecosystem notes.
