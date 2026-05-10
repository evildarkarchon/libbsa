# Phase 10: TES3 Write Support and BSA Format Completeness - Research

**Researched:** 2026-05-09  
**Domain:** C++20 TES3/Morrowind BSA write-new serialization and validation  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Phase Boundary

Phase 10 adds public write-new support for TES3/Morrowind BSA archives. Consumers add disk-file or copied memory entries with explicit archive-internal paths, libbsa serializes raw TES3 metadata tables, hash-sorted name records, data-section-relative file offsets, and raw payloads, then proves output through byte-level structure checks, committed legal writer fixture evidence, and reader-backed reopen/list/find/contains/extract round trips. [VERIFIED: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md]

#### Requirements (locked via SPEC.md)

**8 requirements are locked.** See `10-SPEC.md` for full requirements, boundaries, and acceptance criteria. [VERIFIED: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md]

Downstream agents MUST read `10-SPEC.md` before planning or implementing. Requirements are not duplicated here. [VERIFIED: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md]

**In scope (from SPEC.md):** [VERIFIED: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md]
- Public TES3/Morrowind BSA write-new capability for disk-file entries and memory-buffer entries.
- TES3 raw/uncompressed archive serialization using TES3 header, file records, name offsets, name table, hash table, and data section layout.
- TES3 data-section-relative record offsets with reader-visible archive-absolute metadata after reopen.
- TES3 hash calculation and low32/high32 hash sort order for serialized records.
- Archive path validation, duplicate canonical-path rejection, copied-memory ownership, missing disk source errors, empty archive errors, default overwrite rejection, and explicit overwrite success.
- Committed legal TES3 writer fixture data and manifest evidence from repository-owned synthetic bytes.
- Reader-backed pack/reopen/list/find/contains/extract/byte-compare tests for TES3 writer output.
- Byte-level tests for TES3 writer headers, tables, hash order, name offsets, raw offsets, and payload bytes.
- Regression checks proving existing TES3 reader, TES4-family BSA writer, public include boundary, and TES5Edit read-only behavior remain intact.

**Out of scope (from SPEC.md):** [VERIFIED: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md]
- TES3 compression, embedded file-name payload prefixes, or per-entry compression override behavior. TES3 writer output is raw-only for this phase.
- TES3 payload deduplication or shared non-empty payload regions.
- TES4-family or BA2 writer feature expansion beyond regression coverage.
- Compatibility warning APIs, standalone validation APIs, malformed-input hardening framework, and BSArchPro-derived warning catalogs. Phase 11 owns compatibility warnings and hardening.
- CLI, GUI, or broad BSArchPro tool-option parity.
- Mandatory local Morrowind install validation, real-game archive fixtures, or committed copyrighted game bytes.
- In-place mutation of existing archives.
- Parallel packing, bounded-memory performance guarantees beyond existing writer conventions, benchmarks, and thread-safety documentation. Phase 12 owns performance and concurrency.
- Editing, formatting, compiling into, staging, or using `TES5Edit/` as a fixture workspace.

#### Implementation Decisions

##### Public API Shape
- **D-01:** Expose a dedicated single-purpose `tes3_bsa_writer` public writer. Do not add a one-value TES3 target enum and do not start a generic BSA writer redesign in Phase 10. [VERIFIED: 10-CONTEXT.md]
- **D-02:** Add a minimal `tes3_bsa_writer_options` surface with `overwrite_existing` only. Do not add speculative public knobs for compression, embedded names, deduplication, raw flags, hashes, or future compatibility warnings. [VERIFIED: 10-CONTEXT.md]
- **D-03:** Mirror existing writer method names and lifecycle: `add_file`, `add_bytes`, and `write_to`. [VERIFIED: 10-CONTEXT.md]
- **D-04:** Public docs/comments should explicitly state that TES3 writer output is raw/uncompressed and intentionally has no compression, dedupe, or embedded-name controls in Phase 10. [VERIFIED: 10-CONTEXT.md]

##### Disk Source Timing
- **D-05:** Disk-file TES3 entries remain path-backed until `write_to`, matching TES4 and BA2 GNRL writer behavior. Do not snapshot disk file bytes at `add_file`. [VERIFIED: 10-CONTEXT.md]
- **D-06:** `add_file` should immediately validate the archive path and non-empty host path only. Missing or unreadable disk sources are reported from `write_to` through structured `io_error` results. [VERIFIED: 10-CONTEXT.md]
- **D-07:** Keep `write_to` logically non-consuming like existing writers. Repeated `write_to` calls may reread path-backed disk sources each time; memory entries remain copied at add time. [VERIFIED: 10-CONTEXT.md]
- **D-08:** If any path-backed disk source is missing or unreadable during finalization, fail cleanly with no successfully published partial archive and clean temporary output state before returning. [VERIFIED: 10-CONTEXT.md]

##### Fixture Proof Shape
- **D-09:** Commit one representative canonical TES3 writer-produced archive plus manifest as writer-output fixture evidence. Cover broader validation scenarios through runtime writer tests rather than committing a large fixture matrix. [VERIFIED: 10-CONTEXT.md]
- **D-10:** The committed writer-output fixture must be generated through the new public TES3 writer API, not solely through the existing private/test-only TES3 fixture serializer. [VERIFIED: 10-CONTEXT.md]
- **D-11:** The writer fixture manifest should record full layout facts: source kind, original path, canonical path, stored hash low/high values, raw TES3 data offset, archive-absolute payload offset, raw/stored sizes, and expected payload bytes in hex. [VERIFIED: 10-CONTEXT.md]
- **D-12:** Tests should assert byte-level structural facts for header fields, name offsets, name bytes, hash records, raw data offsets, and payload bytes plus reader-backed round trips. Do not require one full archive byte-for-byte golden comparison unless planner finds it necessary. [VERIFIED: 10-CONTEXT.md]

##### Path Byte Policy
- **D-13:** Preserve caller-provided path case in serialized TES3 names and stored hash computation. Canonical lowercase paths are only for validation, lookup-key derivation, and duplicate detection. [VERIFIED: 10-CONTEXT.md]
- **D-14:** Allow any archive path accepted by the shared archive path validator, including root-level file names if the normalizer accepts them. TES3 has a flat name table and should not inherit TES4 folder-record restrictions. [VERIFIED: 10-CONTEXT.md]
- **D-15:** TES3 stored hashes are writer-owned. Callers provide paths only; the writer computes `detail::hash_tes3` from the serialized name bytes and sorts records by TES3 low32/high32 order. [VERIFIED: 10-CONTEXT.md]

##### Carry-Forward Decisions
- **D-16:** Preserve dependency-light public headers: no public `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, private parser, private writer, private hash, or private compression types. [VERIFIED: 10-CONTEXT.md]
- **D-17:** Preserve existing writer validation style: expected caller-data and I/O failures return `libbsa::result` errors; tests assert stable `error_code` values, not diagnostic text. [VERIFIED: 10-CONTEXT.md]
- **D-18:** Preserve generated/legal fixture policy. All committed Phase 10 writer evidence must be repository-owned synthetic data outside `TES5Edit/`, and `TES5Edit/` must remain untouched. [VERIFIED: 10-CONTEXT.md]
- **D-19:** Preserve reader-backed validation as the acceptance oracle: produced TES3 archives must reopen through `archive_reader`, list/find/contains entries using normalized path semantics, and extract source-equivalent bytes through sink and byte-vector APIs. [VERIFIED: 10-CONTEXT.md]

### the agent's Discretion
- The exact separator byte policy is delegated to researcher/planner. Default to the existing writer policy of converting `\` to `/`, serializing that preserved spelling, and hashing the serialized bytes unless TES3 reference tracing proves caller separator bytes must be preserved exactly. [VERIFIED: 10-CONTEXT.md]
- Researcher/planner may choose exact private file layout, helper boundaries, CMake registration, and Catch2 test organization if public headers stay dependency-light and the decisions above remain intact. [VERIFIED: 10-CONTEXT.md]
- Planner may choose exact synthetic entry paths and payload bytes for the canonical fixture and runtime tests, provided disk source, memory source, zero-byte, mixed-case, mixed-separator, root-level if valid, hash sorting, data-section-relative offsets, and duplicate/invalid path behavior are covered. [VERIFIED: 10-CONTEXT.md]

### Deferred Ideas (OUT OF SCOPE)

None - discussion stayed within phase scope. [VERIFIED: 10-CONTEXT.md]
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WBSA-04 | Consumer can create new TES3/Morrowind BSA archives from disk files or memory buffers. [VERIFIED: .planning/REQUIREMENTS.md] | Use a dedicated `tes3_bsa_writer` public API, raw TES3 table serialization, writer-owned TES3 hash/order computation, path-backed disk sources, copied memory sources, safe output publishing, and reader-backed round-trip tests. [VERIFIED: 10-SPEC.md; VERIFIED: codebase read] |
</phase_requirements>

## Project Constraints (from AGENTS.md)

- `TES5Edit/` is read-only reference material: do not edit, format, generate files under, stage, commit, update the submodule pointer, compile into libbsa, or use it as a fixture workspace. [VERIFIED: AGENTS.md]
- Implementation work belongs outside `TES5Edit/` and should preserve archive-format behavior discovered from BSArchPro unless a documented reason to diverge exists. [VERIFIED: AGENTS.md]
- Public implementation language is C++ and should use clear portable C++ interfaces rather than direct Delphi/Pascal transliteration. [VERIFIED: AGENTS.md]
- Do not introduce speculative dependencies; use the C++ standard library unless a real format/compression/filesystem/testing/packaging need justifies more. [VERIFIED: AGENTS.md]
- Keep public headers dependency-light and avoid leaking libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, private parser, or private compression types. [VERIFIED: AGENTS.md; VERIFIED: .planning/PROJECT.md]
- Add Doxygen-compliant comments for new public APIs and comments for non-obvious compatibility constraints such as TES3 relative offsets and hash ordering. [VERIFIED: AGENTS.md]
- Add focused tests for archive parsing, writing, round-tripping, and compatibility behavior; do not use `TES5Edit/` as a mutable test fixture. [VERIFIED: AGENTS.md]
- Never delete accurate comments as cleanup; rewrite/remove comments only when obsolete and report it. [VERIFIED: AGENTS.md]

## Summary

Phase 10 should be planned as a targeted writer addition, not a writer-system redesign. The public surface is locked to `tes3_bsa_writer_options{ overwrite_existing }` plus `tes3_bsa_writer::add_file`, `add_bytes`, and `write_to`, mirroring existing writer lifecycle while omitting target enums, compression policies, dedupe knobs, embedded-name controls, and raw hash/offset overrides. [VERIFIED: 10-CONTEXT.md; VERIFIED: include/libbsa/writer.hpp]

The critical format facts are stable and cross-verified: TES3/Morrowind BSA begins with version/magic `0x00000100`, then a hash-table offset stored as offset minus 12, file count, file size/raw-offset records, name offsets, null-terminated names, 64-bit hashes, and raw uncompressed payload bytes. File record offsets are relative to the data section, while libbsa public metadata remains archive-absolute after reopen. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]

The planner should split work into API/boundary, private writer state and validation, TES3 serialization, fixture generation/manifest evidence, reader-backed/byte-level tests, and final regression gates. The highest-risk details are hash-sort ordering, path spelling used for serialized names and hashes, uint32 overflow/offset arithmetic, safe publish behavior, and avoiding accidental public exposure of private helpers. [VERIFIED: 10-SPEC.md; VERIFIED: src/detail/bethesda_hash.cpp; VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp]

**Primary recommendation:** Implement a dedicated `tes3_bsa_writer` backed by a private `src/formats/bsa/tes3_bsa_writer.*` serializer that prepares hash-sorted entries, writes raw-only TES3 tables with data-section-relative offsets, publishes through the hardened Phase 8/9-style safe output flow, and proves output through byte-level table tests plus public `archive_reader` round trips. [VERIFIED: 10-CONTEXT.md; VERIFIED: codebase read]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Public TES3 writer API | C++ public library API | Private BSA writer implementation | `include/libbsa/writer.hpp` already owns writer public surfaces, and private format writers live under `src/formats/`. [VERIFIED: include/libbsa/writer.hpp; VERIFIED: CMakeLists.txt] |
| Archive path validation and duplicate canonical detection | Internal shared detail layer | Writer finalization | `detail::normalize_archive_path` canonicalizes and validates paths; existing writers defer duplicate detection to finalization. [VERIFIED: src/detail/archive_path.cpp; VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp] |
| TES3 hash computation and sort order | Internal hash detail layer | TES3 writer serializer | `detail::hash_tes3` and `detail::tes3_hash_sort_key` already encode TES3 hashing and low32/high32 sort behavior. [VERIFIED: src/detail/bethesda_hash.cpp] |
| TES3 binary serialization | Private TES3 BSA writer | Shared binary writer | The parser and fixture generator define fixed record widths, and `detail::binary_writer` provides checked little-endian writes. [VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp; VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp; VERIFIED: src/detail/binary_io.hpp] |
| Output publishing and overwrite behavior | Private writer implementation | Filesystem layer | Existing writers write a temporary output and publish/replace according to explicit options; Phase 10 requires no partial successful archive on source failure. [VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp; VERIFIED: 10-CONTEXT.md] |
| Reader-backed validation | Test suite | Public reader facade | `archive_reader::open`, `entries`, `find`, `contains`, `extract`, and `extract_bytes` are the acceptance oracle for writer output. [VERIFIED: 10-SPEC.md; VERIFIED: tests/unit/tes4_bsa_writer_tests.cpp] |
| Fixture provenance | Test fixture generator | Repository fixture directory | Existing generated fixture tools write synthetic archives/manifests under `tests/fixtures/generated/archives`. [VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp] |

## Standard Stack

### Core

| Library / Component | Version | Purpose | Why Standard |
|---------------------|---------|---------|--------------|
| C++ | C++20 | Public API and implementation language | Project requires C++20 and dependency-light public headers. [VERIFIED: CMakeLists.txt; VERIFIED: AGENTS.md] |
| libbsa internal primitives | Current repo state | Path validation, hash calculation, binary writes, result errors | Required helpers already exist and avoid new dependencies. [VERIFIED: src/detail/archive_path.cpp; VERIFIED: src/detail/bethesda_hash.cpp; VERIFIED: src/detail/binary_io.hpp; VERIFIED: include/libbsa/result.hpp] |
| CMake | 3.24 minimum; local tool `4.3.2` | Source registration, test target registration, CTest orchestration | Project CMake minimum is 3.24 and local CMake is available. [VERIFIED: CMakeLists.txt; VERIFIED: environment audit] |
| Catch2 | vcpkg manifest dependency; existing stack records `3.14.0#0` | Unit, fixture, byte-level, and round-trip tests | Existing test target uses Catch2 and `catch_discover_tests`. [VERIFIED: vcpkg.json; VERIFIED: tests/CMakeLists.txt; CITED: /catchorg/catch2 docs] |
| CTest | Bundled with CMake; local tool `4.3.2` | Running focused and full test suites | Existing test integration uses `catch_discover_tests(... ADD_TAGS_AS_LABELS DISCOVERY_MODE PRE_TEST)`. [VERIFIED: tests/CMakeLists.txt; VERIFIED: environment audit; CITED: /catchorg/catch2 docs] |
| nlohmann-json | vcpkg manifest dependency | Test-only manifest validation | Existing tests link `nlohmann_json::nlohmann_json`; keep it test-only. [VERIFIED: vcpkg.json; VERIFIED: tests/CMakeLists.txt] |

### Supporting

| Library / Component | Version | Purpose | When to Use |
|---------------------|---------|---------|-------------|
| TES5Edit / BSArchPro reference | Read-only submodule | Compatibility tracing for TES3 layout, hash, raw offsets, and raw compression policy | Trace only; never modify, compile, stage, or use as fixture workspace. [VERIFIED: AGENTS.md; VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: TES5Edit/BSArch/frmPack.pas] |
| UESP TES3 BSA format page | Page last edited 2024-02-21 | Independent cross-check of TES3 section layout, hash-table offset, raw offsets, and sort order | Use to validate TES5Edit-derived layout facts. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format] |
| Existing TES3 fixture generator | Current repo state | Layout reference and manifest pattern | Use as test/reference guidance only; Phase 10 writer fixture must be produced through public writer API. [VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp; VERIFIED: 10-CONTEXT.md] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Dedicated `tes3_bsa_writer` | Generic BSA writer with a TES3 target enum | Locked out by D-01; generic redesign increases scope and risks public API churn. [VERIFIED: 10-CONTEXT.md] |
| Raw-only TES3 writer | Compression or per-entry compression options | Locked out by Phase 10 scope and BSArchPro UI note that Morrowind does not support archive compression. [VERIFIED: 10-SPEC.md; VERIFIED: TES5Edit/BSArch/frmPack.pas] |
| Writer-owned hashes | Public hash override knobs | Locked out by D-15 and public boundary goals; caller-provided hashes could create archives the strict reader rejects. [VERIFIED: 10-CONTEXT.md; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp] |
| Public raw-offset field | Manifest-only raw-offset proof | Phase 4 locked `entry_metadata::payload_offset` as archive-absolute and raw TES3 offset as internal/fixture detail. [VERIFIED: 04-CONTEXT.md] |
| Reusing test-only TES3 fixture serializer as production writer | New private production writer | Fixture generator lacks public API semantics, source validation, overwrite behavior, and safe publish; Phase 10 fixture must prove public writer output. [VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp; VERIFIED: 10-CONTEXT.md] |

**Installation:** no new dependencies are recommended for Phase 10. [VERIFIED: vcpkg.json; VERIFIED: AGENTS.md]

```bash
# No package installation required.
# Add source/test files to existing CMake targets and reuse vcpkg manifest dependencies.
```

**Version verification:** This project does not use npm; current dependency shape was verified from `vcpkg.json`, `CMakeLists.txt`, `tests/CMakeLists.txt`, local `cmake --version`, local `ctest --version`, and existing stack notes in `AGENTS.md`. [VERIFIED: vcpkg.json; VERIFIED: CMakeLists.txt; VERIFIED: tests/CMakeLists.txt; VERIFIED: environment audit; VERIFIED: AGENTS.md]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer code
  |
  | constructs tes3_bsa_writer(options)
  v
Public writer API (include/libbsa/writer.hpp)
  |
  | add_file(path, host) validates archive path + non-empty host string
  | add_bytes(path, bytes) validates archive path + copies bytes
  v
Writer-owned entry state
  |
  | write_to(output)
  v
Finalization validation
  |-- empty entries? ------------------> invalid_argument result
  |-- duplicate canonical paths? ------> format_error result
  |-- missing/unreadable disk source? -> io_error result + no published archive
  |-- existing output without option? -> io_error result
  v
Prepare TES3 records
  |
  | preserved spelling: backslash -> slash; case preserved
  | stored hash = detail::hash_tes3(serialized name)
  | record order = detail::tes3_hash_sort_key(hash)
  | raw offsets = payload cursor relative to data section
  v
Serialize TES3 archive bytes
  |
  | version 0x00000100
  | hash_offset_minus_header = hash_table_start - 12
  | file records + name offsets + names + hashes + raw payloads
  v
Safe publish to host-path archive
  |
  v
Validation tests reopen via archive_reader
  |
  | list/find/contains/extract/extract_bytes + byte-level table assertions
  v
WBSA-04 acceptance
```

### Recommended Project Structure

```text
include/libbsa/
├── writer.hpp                         # add tes3_bsa_writer_options + tes3_bsa_writer public contract
└── libbsa.hpp                         # already includes public writer header
src/formats/bsa/
├── tes3_bsa_writer.hpp                # private writer entry state + write_tes3_bsa_archive declaration
└── tes3_bsa_writer.cpp                # public bridge, validation, serializer, safe publish
├── tes3_bsa_writer_tests.cpp          # public API, byte layout, round-trip, validation, overwrite tests
└── public_include_boundary_tests.cpp  # add TES3 writer contract assertions
├── generate_tes3_bsa_writer_fixtures.cpp  # optional dedicated public-writer fixture generator
└── archives/tes3_writer_*.{bsa,json}      # committed writer-produced evidence
```

### Pattern 1: Public Writer Bridge with Private State

**What:** Add public C++20/Doxygen-commented `tes3_bsa_writer_options` and `tes3_bsa_writer`, backed by `std::shared_ptr<state>` and a private `formats::bsa::write_tes3_bsa_archive` function. [VERIFIED: include/libbsa/writer.hpp; VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp]

**When to use:** Use for all public TES3 writer entry points in Phase 10; do not add one-shot functions or public private-helper exposure. [VERIFIED: 10-CONTEXT.md]

**Example:**

```cpp
// Source: include/libbsa/writer.hpp and src/formats/bsa/tes4_bsa_writer.cpp patterns.
struct tes3_bsa_writer_options {
  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing{false};
};

class tes3_bsa_writer {
 public:
  /// Creates a raw/uncompressed TES3/Morrowind BSA writer using default options.
  tes3_bsa_writer();

  /// Creates a raw/uncompressed TES3/Morrowind BSA writer using explicit options.
  explicit tes3_bsa_writer(tes3_bsa_writer_options options);

  [[nodiscard]] const tes3_bsa_writer_options& options() const noexcept;
  result<void> add_file(std::string_view archive_path, std::string_view host_path);
  result<void> add_bytes(std::string_view archive_path, std::span<const std::byte> bytes);
  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;
  std::shared_ptr<state> state_;
};
```

### Pattern 2: Prepare Entries Before Serialization

**What:** Convert writer entries into prepared records containing serialized name, canonical path, raw payload, stored hash, size, raw data-section offset, and ownership/source metadata before writing bytes. [VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp; VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp]

**When to use:** Use at `write_to` so disk sources are read late, duplicates are detected at finalization, and uint32 overflow checks happen before publishing. [VERIFIED: 10-CONTEXT.md]

**Example:**

```cpp
// Source: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp layout builder + detail::hash_tes3.
struct prepared_tes3_entry {
  std::string serialized_name;       // caller spelling with separators normalized to '/'
  std::string canonical_path;        // lowercase lookup key from detail::normalize_archive_path
  std::vector<std::byte> payload;    // copied memory or late-read disk bytes
  std::uint64_t hash{};              // detail::hash_tes3(serialized_name)
  std::uint32_t raw_offset{};        // relative to data section
};

std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
  return libbsa::detail::tes3_hash_sort_key(lhs.hash) < libbsa::detail::tes3_hash_sort_key(rhs.hash);
});
```

### Pattern 3: Writer-Produced Fixture Evidence

**What:** Commit one representative TES3 writer output plus manifest recording source kind, original/canonical path, stored hash halves, raw TES3 offset, archive-absolute offset, sizes, and expected bytes. [VERIFIED: 10-CONTEXT.md]

**When to use:** Use as durable evidence that public writer output exists; keep broader matrix in runtime tests to avoid fixture bloat. [VERIFIED: 10-CONTEXT.md]

**Example manifest fields:**

```json
{
  "variant": "tes3",
  "version": 256,
  "provenance": {
    "generator": "tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp",
    "source": "synthetic bytes generated for libbsa tests; no game or TES5Edit bytes copied"
  },
  "entries": [
    {
      "source_kind": "memory",
      "original_path": "Meshes/Mixed/Probe.NIF",
      "canonical_path": "meshes/mixed/probe.nif",
      "archive_hash": "0x...",
      "hash_low32": "0x...",
      "hash_high32": "0x...",
      "raw_tes3_data_offset": 0,
      "payload_offset": 92,
      "raw_size": 4,
      "stored_size": 4,
      "expected": { "bytes_hex": "42534121" }
    }
  ]
}
```

### Anti-Patterns to Avoid

- **Adding compression controls:** TES3 writer output is raw-only for Phase 10 and BSArchPro states Morrowind archives do not support compression. [VERIFIED: 10-SPEC.md; VERIFIED: TES5Edit/BSArch/frmPack.pas]
- **Hashing canonical lowercase path instead of serialized name:** Phase 10 locks hashing to serialized name bytes/spelling, while canonical lowercase is for validation and lookup only. [VERIFIED: 10-CONTEXT.md]
- **Storing archive-absolute offsets in TES3 file records:** TES3 records store data-section-relative offsets; public metadata converts to archive-absolute after reopen. [CITED: UESP TES3 BSA format; VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]
- **Using the TES3 fixture generator as production writer code:** It is test-only and lacks public API validation/publishing semantics. [VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp; VERIFIED: 10-CONTEXT.md]
- **Writing committed fixtures under `TES5Edit/`:** This violates the read-only submodule boundary. [VERIFIED: AGENTS.md]
- **Testing diagnostics text:** Established tests assert stable `error_code` values, not messages. [VERIFIED: 10-CONTEXT.md; VERIFIED: tests/unit/tes4_bsa_writer_tests.cpp]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Archive path canonicalization | New path parser or `std::filesystem::path` for archive-internal names | `detail::normalize_archive_path` plus preserved spelling string | Existing validator enforces root/drive/segment rules and canonical lowercase `/` lookup keys. [VERIFIED: src/detail/archive_path.cpp] |
| TES3 hash algorithm | New ad hoc hash implementation | `detail::hash_tes3` and `detail::tes3_hash_sort_key` | Existing helper is already traced to TES5Edit and used by parser/tests. [VERIFIED: src/detail/bethesda_hash.cpp] |
| Little-endian byte writes | Manual byte pushes scattered through writer | `detail::binary_writer` plus small local checked helpers where needed | Existing writer primitive centralizes little-endian serialization. [VERIFIED: src/detail/binary_io.hpp] |
| Source read behavior | Eager disk snapshot at `add_file` | Late disk read at `write_to`, copied memory at `add_bytes` | Phase 10 locks disk sources as path-backed until finalization. [VERIFIED: 10-CONTEXT.md] |
| Output proof | Writer-internal assertions only | Public `archive_reader` reopen/list/find/contains/extract plus byte-level table tests | Reader-backed validation is the locked oracle. [VERIFIED: 10-SPEC.md] |
| Fixture bytes | Hand-authored opaque binary blobs | Generated public-writer fixture plus manifest provenance | Project fixture policy requires legal synthetic provenance. [VERIFIED: AGENTS.md; VERIFIED: 10-CONTEXT.md] |

**Key insight:** TES3 writing looks simple but correctness depends on coordinating four independent order/offset views: serialized record/hash order, name-table offset order, raw payload order, and public reader metadata order. Reuse existing hash/path/binary/reader infrastructure so Phase 10 only owns the TES3-specific writer layout. [VERIFIED: UESP TES3 BSA format; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp; VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp]

## Common Pitfalls

### Pitfall 1: Hash byte order and sort key confusion
**What goes wrong:** Writer emits valid-looking 64-bit hashes but sorts by the raw 64-bit value or writes high/low halves in the wrong serialized order. [VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp; VERIFIED: TES5Edit/Core/wbBSArchive.pas]

**Why it happens:** TES3 sorts by lower four bytes first, then higher four bytes, while Pascal `Save` writes `Hash shr 32` then `Hash and $FFFFFFFF` as cardinals and libbsa's `binary_writer::write_u64_le` writes the equivalent little-endian 64-bit value. [CITED: UESP TES3 BSA format; VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: src/detail/bethesda_hash.cpp]

**How to avoid:** Use `detail::tes3_hash_sort_key(hash)` for ordering and byte-level tests that parse both stored hash values and low/high halves. [VERIFIED: src/detail/bethesda_hash.cpp; VERIFIED: 10-CONTEXT.md]

**Warning signs:** Reopen fails with `format_error` for unsorted hash records or stored hash mismatch. [VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]

### Pitfall 2: Hashing canonical lowercase path instead of preserved serialized path
**What goes wrong:** Mixed-case or separator-variant entries reopen with hash mismatch or lose `original_path` behavior. [VERIFIED: 10-SPEC.md; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]

**Why it happens:** The shared normalizer lowercases canonical lookup keys, but Phase 10 locks stored name case preservation and hashes from serialized name bytes. [VERIFIED: 10-CONTEXT.md; VERIFIED: src/detail/archive_path.cpp]

**How to avoid:** Store both `archive_path_original` after `\` to `/` conversion and `archive_path_canonical`; compute `hash_tes3(original_serialized_name)`. [VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp; VERIFIED: 10-CONTEXT.md]

**Warning signs:** Byte-level tests pass for lowercase paths but fail for `Meshes/Mixed/Probe.NIF` or mixed-separator cases. [VERIFIED: 10-SPEC.md]

### Pitfall 3: Absolute-vs-relative payload offsets
**What goes wrong:** Writer stores archive-absolute offsets in TES3 records, causing the reader to add `data_section_start` again and reject or extract wrong bytes. [VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]

**Why it happens:** libbsa public `entry_metadata::payload_offset` is archive-absolute, while TES3 on-disk file records are data-section-relative. [VERIFIED: 04-CONTEXT.md; CITED: UESP TES3 BSA format]

**How to avoid:** Compute `data_section_start`, store `raw_offset = payload_cursor - data_section_start`, and assert reopened `payload_offset == data_section_start + raw_offset`. [VERIFIED: 10-SPEC.md]

**Warning signs:** Existing malformed fixture `tes3_raw_offset_absolute_regression.bsa` exists specifically to catch this class of bug. [VERIFIED: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp]

### Pitfall 4: Unsafe publish leaves partial outputs
**What goes wrong:** A missing disk source or write error leaves a partial `.bsa` at the destination. [VERIFIED: 10-CONTEXT.md]

**Why it happens:** Finalization reads path-backed disk sources late, so errors can occur after some output bytes have been prepared. [VERIFIED: 10-CONTEXT.md]

**How to avoid:** Validate all entries and read/prepare payloads before publishing; write to unique temp output and clean temp on failure; for overwrite, prefer Phase 8/9 backup/rollback hardening over the older deterministic `.tmp` delete/rename pattern. [VERIFIED: STATE.md; VERIFIED: 08-CONTEXT.md; VERIFIED: 09-CONTEXT.md]

**Warning signs:** Tests only check returned error code but not absence/preservation of destination bytes. [VERIFIED: tests/unit/tes4_bsa_writer_tests.cpp]

### Pitfall 5: Root-level paths accidentally rejected due TES4 assumptions
**What goes wrong:** Writer rejects valid TES3 root-level archive names because TES4-family writer requires folder/file split. [VERIFIED: 10-CONTEXT.md; VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp]

**Why it happens:** TES4-family BSA has folder records; TES3 has a flat name table. [CITED: UESP TES3 BSA format; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp]

**How to avoid:** Use shared archive path validation only; do not require a slash for TES3. [VERIFIED: 10-CONTEXT.md; VERIFIED: src/detail/archive_path.cpp]

**Warning signs:** `add_bytes("Readme.txt", ...)` fails despite `detail::normalize_archive_path` accepting it. [VERIFIED: src/detail/archive_path.cpp]

## Code Examples

Verified patterns from repository/reference sources:

### TES3 Table Layout Calculation

```cpp
// Source: tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp and UESP TES3 BSA format.
constexpr std::uint32_t tes3_version = 0x00000100U;
constexpr std::uint32_t header_size = 12U;

const std::uint32_t records_size = checked_u32(entries.size() * 8U, "TES3 file records");
const std::uint32_t name_offsets_size = checked_u32(entries.size() * 4U, "TES3 name offsets");
const std::uint32_t hash_table_start = header_size + records_size + name_offsets_size + name_table_size;
const std::uint32_t data_section_start = hash_table_start + checked_u32(entries.size() * 8U, "TES3 hash table");

writer.write_u32_le(tes3_version);
writer.write_u32_le(hash_table_start - header_size);
writer.write_u32_le(file_count);
```

### TES3 Raw Offset Assignment

```cpp
// Source: TES5Edit/Core/wbBSArchive.pas Save writes Offset - fDataOffset; parser converts back.
std::uint32_t next_raw_offset = 0;
for (auto& entry : prepared_entries) {
  entry.raw_offset = next_raw_offset;
  next_raw_offset = checked_add_u32(next_raw_offset, checked_u32(entry.payload.size(), "TES3 payload"));
}
```

### Reader-Backed Round Trip

```cpp
// Source: tests/unit/tes4_bsa_writer_tests.cpp pattern adapted for TES3.
libbsa::tes3_bsa_writer writer{};
REQUIRE(writer.add_file("Meshes/Disk/Probe.nif", source.string()).has_value());
REQUIRE(writer.add_bytes("Textures/Memory/Probe.dds", memory_bytes).has_value());
REQUIRE(writer.add_bytes("Readme.txt", std::span<const std::byte>{}).has_value());
REQUIRE(writer.write_to(output.string()).has_value());

auto opened = libbsa::archive_reader::open(output.string());
REQUIRE(opened.has_value());
CHECK(opened.value().metadata().value().variant == libbsa::archive_variant::tes3);
CHECK(opened.value().extract_bytes("textures/memory/probe.dds").value() == memory_bytes);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| TES3 support limited to reader fixtures and private/test-only fixture serialization | Add public write-new TES3 API and production serializer | Phase 10 scope | Completes WBSA-04 and BSA write-family coverage. [VERIFIED: 10-SPEC.md; VERIFIED: REQUIREMENTS.md] |
| TES3 raw offset exposed only through generated manifest proof | Writer computes raw offsets and reader exposes archive-absolute offsets after reopen | Phase 4 and Phase 10 decisions | Tests must assert both raw table values and public metadata conversion. [VERIFIED: 04-CONTEXT.md; VERIFIED: 10-SPEC.md] |
| Older TES4 writer deterministic `.tmp` replacement | Prefer Phase 8/9 safer publish/overwrite rollback style | Phase 8/9 decisions | Planner should use hardened publish behavior for new TES3 writer rather than copying older unsafe replacement edge cases. [VERIFIED: STATE.md; VERIFIED: 08-CONTEXT.md; VERIFIED: 09-CONTEXT.md] |

**Deprecated/outdated:**
- Broad BSArchPro tool-option parity is not Phase 10 scope; only TES3 archive format parity is required. [VERIFIED: 10-SPEC.md]
- Real-game Morrowind archive validation is not required for CI acceptance; legal generated fixtures are required. [VERIFIED: 10-SPEC.md]
- Compression, embedded names, and dedupe are explicitly out of Phase 10 TES3 writer scope. [VERIFIED: 10-SPEC.md]

## Assumptions Log

> List all claims tagged `[ASSUMED]` in this research. The planner and discuss-phase use this section to identify decisions that need user confirmation before execution.

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|

**If this table is empty:** All claims in this research were verified or cited — no user confirmation needed.

## Open Questions (RESOLVED)

1. **Should the committed writer fixture generator be a new executable or an extension of `generate_tes3_bsa_fixtures.cpp`?**
   - What we know: The committed writer-output fixture must be generated through the public TES3 writer API, not solely through the current private/test-only TES3 serializer. [VERIFIED: 10-CONTEXT.md]
   - What's unclear: The exact generator file layout is delegated to planner discretion. [VERIFIED: 10-CONTEXT.md]
   - Recommendation: Use a dedicated `generate_tes3_bsa_writer_fixtures.cpp` if it keeps public-writer provenance obvious; otherwise add a clearly separated writer-output mode to the existing generator. [VERIFIED: tests/CMakeLists.txt; VERIFIED: 10-CONTEXT.md]
   - RESOLVED: Plans use a dedicated `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp` public-writer generator and register `generate_tes3_bsa_writer_fixtures_tool` / `generate_tes3_bsa_writer_fixtures`. [VERIFIED: 10-05-PLAN.md]

2. **Should payload bytes be serialized in hash order or alphabetical order?**
   - What we know: UESP and TES5Edit comment that vanilla TES3 raw file data appears/stores alphabetically, but Phase 4 reader allows any non-overlapping bounded payload order. [CITED: UESP TES3 BSA format; VERIFIED: TES5Edit/Core/wbBSArchive.pas; VERIFIED: 04-CONTEXT.md]
   - What's unclear: Phase 10 does not lock payload physical order except raw offsets and round-trip bytes. [VERIFIED: 10-SPEC.md]
   - Recommendation: Serialize payloads in the same order as prepared hash-sorted records for simpler file-record/name/hash alignment unless planner wants closer vanilla evidence; either choice is acceptable if raw offsets match payload positions and tests document the chosen rule. [VERIFIED: 04-CONTEXT.md; VERIFIED: 10-SPEC.md]
   - RESOLVED: Plans require expected serialized order by `detail::tes3_hash_sort_key(detail::hash_tes3(preserved_name))`, with raw offsets and payload bytes asserted against that order. [VERIFIED: 10-03-PLAN.md]

3. **Should safe publish be upgraded beyond TES4 writer's older `.tmp` behavior during Phase 10?**
   - What we know: Phase 8/9 state records hardened unique temp directory and overwrite backup/rollback decisions; Phase 10 requires clean failure without published partial output. [VERIFIED: STATE.md; VERIFIED: 10-CONTEXT.md]
   - What's unclear: Existing TES4 writer still shows an older `.tmp` remove/rename pattern. [VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp]
   - Recommendation: Plan Phase 10 with Phase 8/9-style safe publish helpers or a small shared internal publish helper if one already exists; do not regress to delete-before-success replacement for new code. [VERIFIED: STATE.md]
   - RESOLVED: Plans require a unique sibling temp directory, preservation of caller-owned `<output>.tmp`, overwrite backup/rollback, and no published partial archive on finalization failure. [VERIFIED: 10-02-PLAN.md]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build and CTest integration | ✓ | 4.3.2 local; project minimum 3.24 | — [VERIFIED: environment audit; VERIFIED: CMakeLists.txt] |
| CTest | Focused/full test runs | ✓ | 4.3.2 | — [VERIFIED: environment audit] |
| Git | TES5Edit status check and repository verification | ✓ | 2.54.0.windows.1 | — [VERIFIED: environment audit] |
| vcpkg manifest | Dependency acquisition | ✓ manifest exists | baseline `12dcccadfe573d0eaa6c67a968413ded7805d256`; no global `vcpkg` command found | Use configured toolchain/environment already used by project builds; planner should not require global `vcpkg` unless current presets do. [VERIFIED: environment audit; VERIFIED: vcpkg.json; VERIFIED: vcpkg-configuration.json] |
| Catch2 | Unit and fixture tests | ✓ manifest dependency | Existing stack records `3.14.0#0` | — [VERIFIED: vcpkg.json; VERIFIED: AGENTS.md] |
| nlohmann-json | Test manifest validation | ✓ manifest dependency | Manifest-pinned by vcpkg baseline | — [VERIFIED: vcpkg.json; VERIFIED: tests/CMakeLists.txt] |

**Missing dependencies with no fallback:** None identified for planning. [VERIFIED: environment audit]

**Missing dependencies with fallback:** Global `vcpkg` executable was not found, but manifest files exist and the project already uses vcpkg manifest mode through CMake configuration. [VERIFIED: environment audit; VERIFIED: vcpkg.json]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 via vcpkg manifest; CTest discovery through `catch_discover_tests`. [VERIFIED: vcpkg.json; VERIFIED: tests/CMakeLists.txt; CITED: /catchorg/catch2 docs] |
| Config file | `tests/CMakeLists.txt` plus root `CMakeLists.txt`. [VERIFIED: tests/CMakeLists.txt; VERIFIED: CMakeLists.txt] |
| Quick run command | `ctest --test-dir <build-dir> -R "tes3_bsa_writer|public_include_boundary" --output-on-failure` [VERIFIED: tests/CMakeLists.txt] |
| Full suite command | `ctest --test-dir <build-dir> --output-on-failure` plus `git -C TES5Edit status --short` [VERIFIED: 10-SPEC.md] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| WBSA-04 | Public TES3 writer API compiles and uses only public types | compile/unit | `ctest --test-dir <build-dir> -R public_include_boundary --output-on-failure` | ✅ existing file, ❌ TES3 writer assertions need Wave 0/1 update [VERIFIED: tests/unit/public_include_boundary_tests.cpp] |
| WBSA-04 | Disk and memory entries write raw TES3 archive and reopen/extract | unit/integration | `ctest --test-dir <build-dir> -R tes3_bsa_writer --output-on-failure` | ❌ Wave 0 [VERIFIED: tests/unit directory glob] |
| WBSA-04 | Byte-level TES3 header/table/hash/name/offset/payload assertions | unit | `ctest --test-dir <build-dir> -R tes3_bsa_writer --output-on-failure` | ❌ Wave 0 [VERIFIED: tests/unit directory glob] |
| WBSA-04 | Committed writer fixture and manifest evidence generated through public writer | fixture | `cmake --build <build-dir> --target generate_tes3_bsa_writer_fixtures` or chosen equivalent | ❌ Wave 0 [VERIFIED: tests/CMakeLists.txt] |
| WBSA-04 | TES3 reader/TES4 writer regressions remain green | regression | `ctest --test-dir <build-dir> -R "tes3_bsa_reader|tes4_bsa_writer|public_include_boundary" --output-on-failure` | ✅ existing reader/writer tests [VERIFIED: tests/CMakeLists.txt] |

### Sampling Rate

- **Per task commit:** Run `ctest --test-dir <build-dir> -R "tes3_bsa_writer|public_include_boundary" --output-on-failure` once TES3 writer tests exist. [VERIFIED: tests/CMakeLists.txt]
- **Per wave merge:** Run `ctest --test-dir <build-dir> -R "tes3_bsa_writer|tes3_bsa_reader|tes4_bsa_writer|public_include_boundary" --output-on-failure`. [VERIFIED: 10-SPEC.md]
- **Phase gate:** Full CTest suite green plus `git -C TES5Edit status --short` empty before `/gsd-verify-work`. [VERIFIED: 10-SPEC.md; VERIFIED: AGENTS.md]

### Wave 0 Gaps

- [ ] `src/formats/bsa/tes3_bsa_writer.hpp` — private writer entry state and serializer declaration. [VERIFIED: CMakeLists.txt]
- [ ] `src/formats/bsa/tes3_bsa_writer.cpp` — public bridge, validation, serialization, safe publish. [VERIFIED: CMakeLists.txt]
- [ ] `tests/unit/tes3_bsa_writer_tests.cpp` — covers public API, layout, path/hash/order, disk/memory ownership, overwrite, missing sources, and round trips. [VERIFIED: tests/CMakeLists.txt]
- [ ] Writer fixture generator target or existing generator extension — covers committed public-writer fixture evidence. [VERIFIED: 10-CONTEXT.md; VERIFIED: tests/CMakeLists.txt]
- [ ] `public_include_boundary_tests.cpp` TES3 writer static assertions. [VERIFIED: tests/unit/public_include_boundary_tests.cpp]
- [ ] Root `CMakeLists.txt` and `tests/CMakeLists.txt` source/target registrations. [VERIFIED: CMakeLists.txt; VERIFIED: tests/CMakeLists.txt]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | No authentication surface in local C++ archive writer. [VERIFIED: 10-SPEC.md] |
| V3 Session Management | no | No session state or network interaction. [VERIFIED: 10-SPEC.md] |
| V4 Access Control | no | Library writes caller-specified local host paths; no user authorization model. [VERIFIED: 10-SPEC.md] |
| V5 Input Validation | yes | Validate archive paths with `detail::normalize_archive_path`, reject empty output/source paths, duplicates, empty archive, size/offset overflow, and missing disk sources through `libbsa::result` errors. [VERIFIED: src/detail/archive_path.cpp; VERIFIED: 10-SPEC.md] |
| V6 Cryptography | no | TES3 hash is a compatibility hash, not security cryptography. [VERIFIED: src/detail/bethesda_hash.cpp] |

### Known Threat Patterns for C++ archive writer

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed or hostile archive-internal paths | Tampering | Use existing archive virtual path validator; reject rooted, drive-rooted, empty, `.`, and `..` segments. [VERIFIED: src/detail/archive_path.cpp] |
| Integer overflow in table sizes or offsets | Tampering / DoS | Use checked uint64/uint32 arithmetic before serialization and fail with structured errors. [VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp; VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp] |
| Partial archive publication after I/O failure | Tampering | Prepare/read sources before publish, write to temp, clean temp on failure, and preserve existing output unless overwrite succeeds. [VERIFIED: 10-CONTEXT.md; VERIFIED: STATE.md] |
| Accidental copyrighted fixture inclusion | Compliance / Repudiation | Generate committed fixtures from repository-owned synthetic bytes and record provenance in manifests. [VERIFIED: AGENTS.md; VERIFIED: 10-CONTEXT.md] |
| Mutable reference submodule contamination | Supply-chain / Tampering | Verify `git -C TES5Edit status --short` at phase gate and never write under `TES5Edit/`. [VERIFIED: AGENTS.md; VERIFIED: 10-SPEC.md] |

## Sources

### Primary (HIGH confidence)

- `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md` — locked Phase 10 decisions, boundaries, and canonical refs. [VERIFIED: read]
- `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-SPEC.md` — requirements and acceptance criteria. [VERIFIED: read]
- `.planning/REQUIREMENTS.md` — WBSA-04 and traceability. [VERIFIED: read]
- `.planning/STATE.md` — recent writer publish and Phase 9 completion decisions. [VERIFIED: read]
- `AGENTS.md` — TES5Edit boundary, dependency policy, comment/doc/test rules. [VERIFIED: read]
- `include/libbsa/writer.hpp` — existing public writer style. [VERIFIED: read]
- `src/formats/bsa/tes3_bsa_parser.cpp` — TES3 parser layout, raw-offset conversion, strict hash/path validation. [VERIFIED: read]
- `src/formats/bsa/tes4_bsa_writer.cpp` / `.hpp` — existing BSA writer state, validation, serialization, tests pattern. [VERIFIED: read]
- `src/detail/archive_path.cpp`, `src/detail/bethesda_hash.cpp`, `src/detail/binary_io.hpp` — reusable path/hash/binary primitives. [VERIFIED: read]
- `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` — current TES3 layout builder and manifest model. [VERIFIED: read]
- `tests/CMakeLists.txt`, `CMakeLists.txt`, `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp` — test/build integration patterns. [VERIFIED: read]
- `TES5Edit/Core/wbBSArchive.pas` — read-only reference for TES3 structure, hashing, sorting, writing relative offsets, and extraction. [VERIFIED: read]
- `TES5Edit/BSArch/frmPack.pas` — read-only reference for Morrowind uncompressed archive behavior note. [VERIFIED: read]
- Context7 `/catchorg/catch2` — `catch_discover_tests`, `ADD_TAGS_AS_LABELS`, `DISCOVERY_MODE PRE_TEST`. [CITED: /catchorg/catch2 docs]

### Secondary (MEDIUM confidence)

- UESP Morrowind BSA format page — independent TES3 layout, raw offset, hash sorting, hash algorithm, page last edited 2024-02-21. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format]

### Tertiary (LOW confidence)

- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — project files and local environment verify no new dependency need. [VERIFIED: vcpkg.json; VERIFIED: CMakeLists.txt; VERIFIED: environment audit]
- Architecture: HIGH — public/private writer patterns and TES3 parser/fixture layout exist in the codebase. [VERIFIED: include/libbsa/writer.hpp; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp; VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp]
- Pitfalls: HIGH — key risks are cross-verified by TES5Edit, UESP, current parser validations, and locked Phase 10 acceptance criteria. [VERIFIED: TES5Edit/Core/wbBSArchive.pas; CITED: UESP TES3 BSA format; VERIFIED: 10-SPEC.md]

**Research date:** 2026-05-09  
**Valid until:** 2026-06-08 for project/codebase planning decisions; re-check external package/tool versions if planning is delayed more than 30 days. [VERIFIED: current date]
