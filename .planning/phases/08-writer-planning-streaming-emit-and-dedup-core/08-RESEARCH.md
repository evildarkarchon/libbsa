# Phase 08: writer-planning-streaming-emit-and-dedup-core - Research

**Researched:** 2026-05-06  
**Domain:** C++20 archive writer planning, streaming emission, compression-policy integration, and deduplication  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### Writer API Shape
- **D-01:** Use a plan-then-finalize API shape. Consumers request a deterministic write plan first, then pass that plan to a finalization function with a caller-owned `byte_sink`.
- **D-02:** Prefer operation-style public functions over a stateful writer object for Phase 8, such as `plan_archive_write(...)` and `finalize_archive_write(plan, sink)`. Exact names are left to the planner, but the API must keep lifetimes explicit and align with existing `open_*` / `extract_*` operation patterns.
- **D-03:** Writer inputs should be public value entry structs containing a normalized archive path, in-memory payload bytes, and per-entry compression policy. Do not add source callbacks, disk traversal, or incremental add APIs in this phase.
- **D-04:** The public writer target should be an explicit libbsa-owned capability descriptor, not just `archive_format`. It should carry the target format identity plus the metadata needed for planning, including archive default compression behavior and whether compression and shared data regions are supported.
- **D-05:** Planning must reject invalid writer inputs before any finalization step can touch a sink. Duplicate normalized paths, invalid paths, unsupported target/options, unsupported compression/dedup combinations, and impossible size/offset arithmetic should return structured failures from planning.

### Plan Preview Model
- **D-06:** Expose write-plan preview as deterministic region and entry metadata, not as an opaque plan. The plan should include table/index regions, data/payload regions, per-entry layout records, total planned size, resolved compression states, stored sizes, dedup references, and offsets.
- **D-07:** Plan preview record order should be stable and testable through sorted vectors. Deterministic ordering is part of the consumer-visible preview contract and must not depend on caller input order.
- **D-08:** Planned offsets should be archive-absolute final stream offsets. This matches existing `entry_metadata::offset` semantics and makes read-after-write metadata comparison direct.
- **D-09:** The plan should own the post-policy packed payload bytes for each planned data region. Finalization should emit from the already-planned region bytes instead of recompressing or reprocessing payloads later, so preview metadata cannot drift from emitted bytes.
- **D-10:** Lookup conveniences over the plan are optional for later phases. Phase 8 should prioritize deterministic vectors and exact layout assertions over a larger ergonomic API surface.

### Harness Proof Shape
- **D-11:** Use a generated test-only generic harness archive format to prove Phase 8 writer-core behavior. Do not use a minimal BSA-like or BA2-like format that could be mistaken for partial production writer compatibility.
- **D-12:** Keep the harness reader/validator in tests or shared test helpers only. Do not expose a public fake archive format or public harness validation API.
- **D-13:** Harness read-after-write tests should compare both metadata and bytes: planned path order, table/data regions, data-region references, offsets, sizes, compression states, dedup references, total size, and extracted payload bytes.
- **D-14:** Harness bytes should be generated inside tests with source-reviewable builders. Do not commit binary golden harness archives unless a later phase discovers a concrete need.
- **D-15:** Harness malformed coverage should stay writer-focused: invalid writer inputs, unsupported target/options, emitted-layout validation, checked arithmetic, codec-route failures, dedup on/off behavior, and sink failure. Broad parser fuzzing or a full fake-format malformed matrix remains out of scope.
- **D-16:** The harness format should include enough structure to exercise real layout decisions: a small header, table/index region, data-region table or equivalent, and payload area. A flat path/payload list is too weak for Phase 8 layout-preview acceptance.
- **D-17:** Harness compression tests should use the real existing codec paths through `resolve_write_compression`, `compress_payload`, `resolve_payload_codec`, and `decompress_payload`. Do not use mock compression markers or raw-only coverage.
- **D-18:** Organize harness builders and assertions as reusable test helpers for Phase 9 and Phase 10 writer tests, while keeping them out of production public API.

### Dedup Semantics
- **D-19:** Dedup identity is byte-identical post-policy payload bytes. The user briefly selected original input bytes by mistake, then corrected the decision after the SPEC conflict was identified. Downstream agents must follow the SPEC-aligned post-policy rule.
- **D-20:** The plan preview must make dedup sharing explicit. Each entry should reference a deterministic data region, and shared entries should visibly point at the same data-region ID and archive-absolute offset.
- **D-21:** If dedup is requested but the target capability says shared data regions are unsupported, planning must return a structured failure. Do not silently downgrade to non-dedup output and do not add a warning-only path in this phase.
- **D-22:** Represent dedup region identity with stable deterministic numeric IDs, assigned in sorted plan order. Do not expose pointer/object references or public content hashes as the sharing identity.
- **D-23:** When dedup is disabled, duplicate payloads must produce distinct data regions and distinct offsets. This behavior should be asserted because it proves the option changes layout.
- **D-24:** Do not expose a public content hash in plan metadata for Phase 8. Hashing and collision handling should remain implementation-private; public preview exposes sharing through region IDs and offsets.
- **D-25:** Compression and stored-payload planning happen before dedup grouping. If compression policy resolution or compression fails for any entry, planning fails structurally and no partial dedup grouping is returned.
- **D-26:** Path normalization affects entry uniqueness, not dedup grouping. Duplicate normalized paths are invalid writer input; dedup grouping is based on post-policy payload bytes independent of path names.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may choose exact type names, function names, helper file names, and test organization details as long as the decisions above, `08-SPEC.md`, existing public API patterns, and project constraints are satisfied.

### Deferred Ideas (OUT OF SCOPE)
## Deferred Ideas

None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WRT-05 | Consumer can finalize new archives through streaming output without requiring the entire archive image in memory. [VERIFIED: `.planning/REQUIREMENTS.md`] | Use existing caller-owned `byte_sink` / `memory_sink` contracts and emit planned header/table/data chunks in order rather than returning one archive byte vector. [VERIFIED: `include/libbsa/io.hpp`, `08-SPEC.md`] |
| WRT-06 | Consumer can opt into content deduplication so identical files share a data region when the target format allows it. [VERIFIED: `.planning/REQUIREMENTS.md`] | Plan post-policy packed payloads first, group byte-identical region bytes only when `writer_target_capabilities::supports_shared_data_regions` is true, and expose numeric `data_region_id` references. [VERIFIED: `08-CONTEXT.md`] |
| WRT-07 | Maintainer can verify archives written by libbsa through read-after-write, round-trip extraction, and metadata comparison tests. [VERIFIED: `.planning/REQUIREMENTS.md`] | Use a test-only generated harness archive with a header, table/index region, data-region table, payload area, harness reader, and metadata/extraction assertions. [VERIFIED: `08-CONTEXT.md`, `08-SPEC.md`] |
</phase_requirements>

## Summary

Phase 8 should build a reusable writer core, not a partial BSA or BA2 writer. [VERIFIED: `08-SPEC.md`] The planner should split work into public writer value types, deterministic planning, streaming finalization, test-only harness read-back, and validation/boundary gates. [VERIFIED: `08-CONTEXT.md`] The public API should be operation-style (`plan_*`, `finalize_*`) and should use `libbsa::result`, `archive_path`, `compression_policy`, `compression_state`, `archive_format`, and `byte_sink` rather than new lifetime-owning writer objects or external dependency types. [VERIFIED: `include/libbsa/*.hpp`, `08-CONTEXT.md`]

The most important implementation invariant is that planning is the only phase that normalizes paths, resolves compression, builds stored payload bytes, checks arithmetic, and groups dedup regions. [VERIFIED: `08-CONTEXT.md`] Finalization should be a deterministic emitter over already-planned bytes and metadata; it should not recompress, renormalize, re-sort, or re-decide dedup. [VERIFIED: `08-CONTEXT.md`] This keeps layout preview and emitted bytes consistent and gives tests a stable surface for exact offset, region, and metadata assertions. [VERIFIED: `08-SPEC.md`]

The existing codebase already provides the needed primitives: caller-owned `byte_sink`, normalized archive paths, copied metadata patterns, writer compression policy helpers, codec dispatch, and Catch2/CTest fixture infrastructure. [VERIFIED: `include/libbsa/io.hpp`, `include/libbsa/archive_path.hpp`, `include/libbsa/archive_view.hpp`, `include/libbsa/compression.hpp`, `CMakeLists.txt`] The missing surface is writer-specific plan/finalize types, checked layout arithmetic, dedup grouping, harness emit/read-back helpers, and focused failure tests. [VERIFIED: `08-SPEC.md`, codebase grep]

**Primary recommendation:** Implement `include/libbsa/writer.hpp` plus `src/writer.cpp` as a plan-then-finalize core whose public plan exposes deterministic vectors of table regions, data regions, and entry records, while test-only harness helpers prove read-after-write without claiming production BSA/BA2 writer compatibility. [VERIFIED: `08-CONTEXT.md`, `CMakeLists.txt`]

## Project Constraints (from AGENTS.md)

- `TES5Edit/` is read-only: do not edit, format, apply generated changes under, update submodule pointer, stage, commit, compile, or vendor it. [VERIFIED: `AGENTS.md`]
- Implementation language is C++; public interfaces should be clean, portable C++ rather than Delphi/Pascal transliteration. [VERIFIED: `AGENTS.md`]
- Keep libbsa reusable and independent of application UI/tooling. [VERIFIED: `AGENTS.md`]
- Preserve BSArchPro-discovered archive behavior unless a divergence is documented. [VERIFIED: `AGENTS.md`]
- Trace non-obvious compatibility constraints and record them near new implementation. [VERIFIED: `AGENTS.md`]
- Do not introduce speculative dependencies; use the C++ standard library unless a concrete format/compression/filesystem/testing/packaging requirement justifies more. [VERIFIED: `AGENTS.md`]
- Required dependencies remain `libdeflate`, official `lz4`, DirectXTex, and vcpkg; Phase 8 should add no new runtime dependency. [VERIFIED: `AGENTS.md`, `vcpkg.json`]
- Never delete accurate comments as cleanup; add comments for non-obvious compatibility, ownership, error-handling, threading, cancellation, or deliberate deviations. [VERIFIED: `AGENTS.md`]
- Add Doxygen-compliant C++ doc comments for public APIs and methods added or substantially rewritten. [VERIFIED: `AGENTS.md`]
- Add focused writing, round-trip, and compatibility tests as writer surfaces are implemented. [VERIFIED: `AGENTS.md`]
- Do not use `TES5Edit/` as a mutable test fixture. [VERIFIED: `AGENTS.md`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Writer input validation and path normalization | API / Library Core | — | Planning owns data-shaped validation before any sink is touched. [VERIFIED: `08-CONTEXT.md`, `include/libbsa/archive_path.hpp`] |
| Compression-policy resolution and payload packing | API / Library Core | Codec adapters | Writer planning must call existing compression policy and codec dispatch; codec implementation stays private. [VERIFIED: `include/libbsa/compression.hpp`, `src/compression.cpp`] |
| Dedup grouping | API / Library Core | — | Dedup identity is post-policy payload bytes and affects plan regions/offsets, so it belongs in planning. [VERIFIED: `08-CONTEXT.md`] |
| Layout preview | API / Library Core | Tests | Public/test-visible plan records expose table/data regions and entry offsets before finalization. [VERIFIED: `08-SPEC.md`] |
| Streaming finalization | API / Library Core | Caller-owned sink | Finalization writes deterministic planned bytes through `byte_sink` and does not retain sink lifetime. [VERIFIED: `include/libbsa/io.hpp`, `08-SPEC.md`] |
| Harness read-back verification | Test tier | API / Library Core | Harness format and reader are test-only proof of the writer core, not public production format support. [VERIFIED: `08-CONTEXT.md`] |
| Production BSA/BA2 serialization | Later phases | — | Complete BSA and BA2 writers are explicitly out of Phase 8 scope. [VERIFIED: `08-SPEC.md`, `.planning/ROADMAP.md`] |

## Standard Stack

### Core

| Library / Component | Version / Status | Purpose | Why Standard |
|---------------------|------------------|---------|--------------|
| C++ | C++20 via `target_compile_features(libbsa PUBLIC cxx_std_20)`. [VERIFIED: `CMakeLists.txt`] | Public API and implementation language. [VERIFIED: `AGENTS.md`] | Matches project constraint and avoids exposing C++23 `std::expected`; use existing `libbsa::result`. [VERIFIED: `include/libbsa/result.hpp`, `AGENTS.md`] |
| CMake | Minimum 3.24 in project; local command reports 4.3.2. [VERIFIED: `CMakeLists.txt`, `cmake --version`] | Build, install header file sets, CTest integration. [VERIFIED: `CMakeLists.txt`] | Existing project uses explicit `target_sources` and file sets; CMake docs support installable `FILE_SET HEADERS`. [CITED: https://github.com/kitware/cmake/blob/master/Help/command/install.rst] |
| vcpkg manifest mode | `vcpkg.json` has builtin baseline `12dcccadfe573d0eaa6c67a968413ded7805d256`. [VERIFIED: `vcpkg.json`] | Dependency acquisition. [VERIFIED: `vcpkg.json`] | Microsoft recommends manifest mode for most users and requires it for versioning/custom registries. [CITED: https://learn.microsoft.com/vcpkg/concepts/manifest-mode] |
| libbsa writer core | New `include/libbsa/writer.hpp` + `src/writer.cpp`. [ASSUMED] | Writer planning, layout preview, dedup, and finalization. [VERIFIED: `08-SPEC.md`] | Keeps writer API separate from BSA/BA2 production APIs while reusing existing public primitives. [VERIFIED: `08-CONTEXT.md`] |

### Supporting

| Library / Component | Version / Status | Purpose | When to Use |
|---------------------|------------------|---------|-------------|
| libdeflate | vcpkg package page lists `libdeflate 1.25`, features `compression` and `decompression`, last updated 2025-11-03. [CITED: https://vcpkg.io/en/package/libdeflate.html] | Deflate compression for writer-planned stored payloads. [VERIFIED: `include/libbsa/compression.hpp`] | Use only through `compress_payload(compression_algorithm::deflate, ...)`. [VERIFIED: `src/compression.cpp`] |
| lz4 | vcpkg package page lists `lz4 1.10.0`, all triplets, last updated 2024-07-25. [CITED: https://vcpkg.io/en/package/lz4.html] | LZ4 frame/block compression through private codecs. [VERIFIED: `src/compression.cpp`] | Use only through `compress_payload`, never direct public LZ4 APIs. [VERIFIED: `include/libbsa/compression.hpp`] |
| Catch2 | vcpkg package page lists `catch2 3.14.0`, all triplets, last updated 2026-04-06. [CITED: https://vcpkg.io/en/package/catch2.html] | Unit, fixture, codec, round-trip tests. [VERIFIED: `CMakeLists.txt`] | Add `libbsa_writer_tests` and register with `catch_discover_tests(... ADD_TAGS_AS_LABELS ...)`. [CITED: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md] |
| Existing compression dispatch | Current code resolves write compression, payload codec, compression, and decompression. [VERIFIED: `include/libbsa/compression.hpp`, `src/compression.cpp`] | Ensures Phase 8 does not infer codecs from extensions or mock compression. [VERIFIED: `08-CONTEXT.md`] | Use in planning and harness read-back. [VERIFIED: `08-CONTEXT.md`] |
| Existing I/O contracts | `byte_sink::write` returns `result<void>` and `memory_sink` appends bytes. [VERIFIED: `include/libbsa/io.hpp`, `tests/io_tests.cpp`] | Streaming finalization target. [VERIFIED: `08-SPEC.md`] | Use for `finalize_archive_write(plan, sink)`. [VERIFIED: `08-CONTEXT.md`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Operation-style plan/finalize | Stateful `archive_writer` object | Rejected by context because it introduces ownership/lifetime questions and conflicts with existing `open_*` / `extract_*` operation style. [VERIFIED: `08-CONTEXT.md`, `08-DISCUSSION-LOG.md`] |
| Test-only harness format | Minimal BSA-like or BA2-like writer | Rejected because it risks implying Phase 8 production writer compatibility before Phases 9/10. [VERIFIED: `08-CONTEXT.md`] |
| Post-policy byte dedup | Original input byte dedup or public content hashes | Rejected because compression policy can change stored bytes; public preview should expose stable numeric region IDs, not hashes. [VERIFIED: `08-CONTEXT.md`] |
| Existing dependencies only | New hashing/logging/formatting dependency | Rejected by project dependency policy; implementation-private grouping can use `std::map`/`std::vector` byte keys for Phase 8 scale. [VERIFIED: `AGENTS.md`] |

**Installation / dependency changes:** No new package installation is recommended for Phase 8. [VERIFIED: `vcpkg.json`, `08-SPEC.md`]

```bash
# Existing manifest already lists libdeflate, lz4, directxtex, and catch2.
cmake --preset windows-msvc-vcpkg
cmake --build --preset windows-msvc-vcpkg
ctest --preset windows-msvc-vcpkg
```

**Version verification:** Package versions above were checked through vcpkg package pages, and local tool versions were checked with `cmake --version`, `ctest --version`, and `git --version`. [VERIFIED: shell audit, CITED: vcpkg package pages]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer writer entries + writer target + writer options
        |
        v
Normalize paths and validate duplicates/target/options
        |-- invalid --> result error, no sink access
        v
Resolve compression policy per sorted entry
        |-- unsupported route/compression failure --> result error, no plan
        v
Build post-policy packed payload bytes per entry
        v
If dedup enabled and target supports sharing?
        | yes --> group identical packed bytes into deterministic data regions
        | no  --> one data region per sorted entry
        v
Compute checked layout: header/table/data-region table/payload offsets
        |-- overflow/impossible size --> result error
        v
write_plan: sorted entry previews + region previews + owned region bytes
        |
        v
finalize_archive_write(plan, byte_sink&)
        |-- sink failure --> structured failure
        v
Deterministic harness/archive byte stream emitted through caller-owned sink
```

### Recommended Project Structure

```text
include/libbsa/
├── writer.hpp                  # Public writer entries, targets, options, plan records, plan/finalize operations. [ASSUMED]
src/
├── writer.cpp                  # Private planning, checked arithmetic, dedup grouping, and emitter. [ASSUMED]
tests/
├── writer_core_tests.cpp       # Public writer planning/finalize/dedup/sink failure tests. [ASSUMED]
├── writer_harness_helpers.hpp  # Test-only harness archive builder/reader assertions. [ASSUMED]
└── writer_harness_helpers.cpp  # Test-only harness serialization/read-back helpers. [ASSUMED]
```

### Pattern 1: Plan Is Public Metadata Plus Private/Owned Region Bytes

**What:** A `write_plan` should expose deterministic vectors for table/index regions, data regions, and entry records while owning the stored bytes needed for finalization. [VERIFIED: `08-CONTEXT.md`]  
**When to use:** Always for Phase 8 planning. [VERIFIED: `08-SPEC.md`]  
**Example:**

```cpp
// Source: Phase 8 context decisions D-06 through D-10. [VERIFIED: 08-CONTEXT.md]
struct planned_data_region {
    std::uint32_t id{};
    std::uint64_t offset{};
    std::uint64_t stored_size{};
    compression_state compression{compression_state::unknown};
};

struct planned_entry {
    std::string path;
    std::uint64_t size{};
    std::uint64_t stored_size{};
    std::uint64_t offset{};
    std::uint32_t data_region_id{};
    compression_state compression{compression_state::unknown};
};
```

### Pattern 2: Planning Resolves Compression Before Dedup

**What:** Convert each input payload into its post-policy stored bytes before grouping duplicates. [VERIFIED: `08-CONTEXT.md`]  
**When to use:** Required for all dedup-enabled and dedup-disabled plans. [VERIFIED: `08-CONTEXT.md`]  
**Example:**

```cpp
// Source: existing compression API. [VERIFIED: include/libbsa/compression.hpp]
auto state = resolve_write_compression(target.format, entry.compression, target.archive_default_compressed);
if (!state.has_value()) {
    return failure<write_plan>(state.error());
}

payload_codec_request request{target.format, state.value(), target.compression_method};
auto algorithm = resolve_payload_codec(request);
if (!algorithm.has_value()) {
    return failure<write_plan>(algorithm.error());
}

auto stored = compress_payload(algorithm.value(), std::span<const std::byte>{entry.payload});
if (!stored.has_value()) {
    return failure<write_plan>(stored.error());
}
```

### Pattern 3: Finalization Is a Simple Ordered Sink Emitter

**What:** Emit already-planned byte chunks to `byte_sink` in exact layout order and propagate the first sink error. [VERIFIED: `08-SPEC.md`]  
**When to use:** `finalize_archive_write(plan, sink)` after a successful plan. [VERIFIED: `08-CONTEXT.md`]  
**Example:**

```cpp
// Source: existing byte_sink contract. [VERIFIED: include/libbsa/io.hpp]
for (const auto& chunk : plan.emission_chunks()) {
    auto written = sink.write(std::span<const std::byte>{chunk.bytes});
    if (!written.has_value()) {
        return written;
    }
}
return success();
```

### Anti-Patterns to Avoid

- **Recompress during finalization:** This can make preview metadata drift from emitted bytes. [VERIFIED: `08-CONTEXT.md`]
- **Use `archive_view` to detect duplicate writer paths:** `archive_view` stores entries by normalized key, so writer duplicates must be rejected before map-like storage can overwrite them. [VERIFIED: `08-CONTEXT.md`, `include/libbsa/archive_view.hpp`]
- **Infer compression from file extension:** Existing compression API requires explicit archive format, entry state, and Starfield compression method. [VERIFIED: `include/libbsa/compression.hpp`]
- **Expose content hashes for dedup:** Phase 8 preview must use numeric region IDs and offsets, not public hashes. [VERIFIED: `08-CONTEXT.md`]
- **Claim BSA/BA2 writer compatibility from harness tests:** Production BSA and BA2 writers are later phases. [VERIFIED: `08-SPEC.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Structured errors | Exceptions or ad hoc booleans | `libbsa::result<T>` / `result<void>` | Existing public API uses structured errors for data-shaped failures. [VERIFIED: `include/libbsa/result.hpp`] |
| Path normalization | `std::filesystem::path` or string lowercasing | `normalize_archive_path` | Archive paths are virtual normalized UTF-8 paths, not host paths. [VERIFIED: `include/libbsa/archive_path.hpp`] |
| Output abstraction | `std::ostream`, returned whole archive byte vector only | `byte_sink` | Existing streaming API uses caller-owned sinks and reports write failures. [VERIFIED: `include/libbsa/io.hpp`] |
| Compression routing | Extension probing or direct libdeflate/LZ4 calls | `resolve_write_compression`, `resolve_payload_codec`, `compress_payload` | Existing dispatch prevents deflate/LZ4 frame/raw-block confusion and keeps dependency headers private. [VERIFIED: `include/libbsa/compression.hpp`, `src/compression.cpp`] |
| Test harness infrastructure | Binary golden archives only | Source-built generated fixtures/helpers | Prior phases use generated source-reviewable fixtures and Phase 8 locks that approach. [VERIFIED: `tests/ba2_dds_fixture_helpers.cpp`, `08-CONTEXT.md`] |
| Build/test discovery | Recursive source globbing | Explicit `target_sources`, explicit test targets, `catch_discover_tests` | Existing CMake deliberately prevents accidental TES5Edit inclusion. [VERIFIED: `CMakeLists.txt`] |

**Key insight:** The hard part is not writing bytes; it is freezing all byte-affecting choices in the plan so finalization becomes deterministic, auditable, and sink-only. [VERIFIED: `08-CONTEXT.md`]

## Common Pitfalls

### Pitfall 1: Treating the Plan as an Opaque Handle
**What goes wrong:** Consumers cannot assert or preview table regions, data regions, offsets, compression states, or dedup sharing. [VERIFIED: `08-SPEC.md`]  
**Why it happens:** Writer APIs often hide layout until finalization. [ASSUMED]  
**How to avoid:** Expose sorted vectors of region and entry preview records. [VERIFIED: `08-CONTEXT.md`]  
**Warning signs:** Tests only compare final bytes and never inspect plan metadata. [VERIFIED: `08-SPEC.md`]

### Pitfall 2: Deduping Original Bytes Instead of Stored Bytes
**What goes wrong:** Two inputs that compress differently, or two different inputs that produce identical stored bytes, get incorrect sharing semantics. [VERIFIED: `08-CONTEXT.md`]  
**Why it happens:** Original payload identity is simpler but contradicts the locked post-policy rule. [VERIFIED: `08-CONTEXT.md`]  
**How to avoid:** Resolve compression and produce stored bytes before dedup grouping. [VERIFIED: `08-CONTEXT.md`]  
**Warning signs:** Dedup code runs before `compress_payload`. [VERIFIED: `08-CONTEXT.md`]

### Pitfall 3: Silently Disabling Dedup
**What goes wrong:** Caller requests dedup, but output layout has duplicate regions without a failure. [VERIFIED: `08-CONTEXT.md`]  
**Why it happens:** Warning-only downgrade can feel ergonomic. [ASSUMED]  
**How to avoid:** If dedup is requested and target capabilities disallow shared data regions, return `unsupported_format`. [VERIFIED: `08-CONTEXT.md`]  
**Warning signs:** Tests assert success for dedup on unsupported targets. [VERIFIED: `08-SPEC.md`]

### Pitfall 4: Partial Success on Sink Failure
**What goes wrong:** Finalization returns success after a `byte_sink` write failed, or tests only use `memory_sink`. [VERIFIED: `08-SPEC.md`]  
**Why it happens:** Sink failures are easy to ignore in append loops. [ASSUMED]  
**How to avoid:** Use a custom failing/counting sink and return the first `result<void>` failure. [VERIFIED: `include/libbsa/io.hpp`, `08-SPEC.md`]  
**Warning signs:** No test injects a failing sink. [VERIFIED: `08-SPEC.md`]

### Pitfall 5: Offset Arithmetic Overflow
**What goes wrong:** Planned offsets wrap, overlap, or produce final sizes that cannot be emitted. [VERIFIED: `08-SPEC.md`]  
**Why it happens:** Layout uses mixed `size_t`, `uint32_t`, and `uint64_t` fields. [VERIFIED: existing reader code uses checked range validation in `src/ba2_reader.cpp`]  
**How to avoid:** Centralize checked add/multiply helpers in writer implementation and test near-`UINT64_MAX` synthetic descriptors. [VERIFIED: `08-SPEC.md`]  
**Warning signs:** Direct `cursor += size` without overflow checks. [VERIFIED: `08-SPEC.md`]

## Code Examples

Verified patterns from official/project sources:

### Register Writer Tests with Catch2 and CTest

```cmake
# Source: current CMake + Catch2 docs. [VERIFIED: CMakeLists.txt] [CITED: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md]
add_executable(libbsa_writer_tests
  tests/writer_core_tests.cpp
  tests/writer_harness_helpers.cpp)
target_link_libraries(libbsa_writer_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain)
catch_discover_tests(libbsa_writer_tests TEST_PREFIX "libbsa_writer_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture;codec;roundtrip")
```

### Public Writer Smoke Shape

```cpp
// Source: public-header smoke precedent. [VERIFIED: tests/public_header_smoke.cpp]
#include <libbsa/writer.hpp>

int main() {
    libbsa::writer_target target{};
    target.format = libbsa::archive_format::fo4_ba2_gnrl;
    target.archive_default_compressed = false;
    target.supports_compression = true;
    target.supports_shared_data_regions = true;

    libbsa::writer_entry entry{};
    entry.path = "meshes/armor/iron.nif";
    entry.payload = {std::byte{0x01}, std::byte{0x02}};
    entry.compression = libbsa::compression_policy::force_raw;

    auto plan = libbsa::plan_archive_write(target, std::vector{entry}, libbsa::writer_options{.deduplicate = true});
    if (!plan.has_value()) return 1;
    libbsa::memory_sink sink;
    return libbsa::finalize_archive_write(plan.value(), sink).has_value() ? 0 : 1;
}
```

### Failing Sink Test Helper

```cpp
// Source: byte_sink contract. [VERIFIED: include/libbsa/io.hpp]
class failing_sink final : public libbsa::byte_sink {
public:
    explicit failing_sink(std::size_t fail_after) : fail_after_(fail_after) {}

    [[nodiscard]] libbsa::result<void> write(std::span<const std::byte> bytes) override {
        if (written_ + bytes.size() > fail_after_) {
            return libbsa::failure({libbsa::error_code::io_failure, "injected sink failure"});
        }
        written_ += bytes.size();
        return libbsa::success();
    }

    std::size_t written_{};
    std::size_t fail_after_{};
};
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Stateful writer object accumulates files | Operation-style plan/finalize API | Locked in Phase 8 context on 2026-05-06. [VERIFIED: `08-CONTEXT.md`] | Planner should create pure planning/finalization functions and avoid hidden lifetimes. [VERIFIED: `08-CONTEXT.md`] |
| Dedup by original input bytes or public hash | Dedup by byte-identical post-policy stored payload bytes | Corrected during Phase 8 discussion. [VERIFIED: `08-CONTEXT.md`] | Compression must precede dedup grouping. [VERIFIED: `08-CONTEXT.md`] |
| Golden binary fixtures | Generated source-reviewable harness archives | Locked in Phase 8 and follows Phase 6/7 style. [VERIFIED: `08-CONTEXT.md`, `tests/ba2_dds_fixture_helpers.cpp`] | Tests should build bytes in source and avoid committing fake archive blobs. [VERIFIED: `08-CONTEXT.md`] |
| Full production writer proof | Harness read-back proof only | Phase 8 boundary. [VERIFIED: `08-SPEC.md`] | Compatibility claims wait for Phases 9-11. [VERIFIED: `.planning/ROADMAP.md`] |

**Deprecated/outdated:**
- Public `std::expected` is not valid for this C++20 project; use `libbsa::result`. [VERIFIED: `.planning/STATE.md`, `include/libbsa/result.hpp`]
- Public DirectXTex, libdeflate, LZ4, platform, Delphi, or TES5Edit types remain forbidden in public writer headers. [VERIFIED: `AGENTS.md`, `08-SPEC.md`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Use names like `include/libbsa/writer.hpp`, `src/writer.cpp`, `writer_target`, `writer_entry`, `write_plan`, and `finalize_archive_write`. | Standard Stack / Architecture Patterns / Code Examples | Low: exact names are explicitly left to planner discretion. |
| A2 | Implementation-private dedup can use standard containers over stored byte vectors without adding a content-hash dependency. | Standard Stack / Alternatives | Medium: very large inputs could be slower, but performance is Phase 12 and public content hashes are forbidden in Phase 8. |
| A3 | Writer plan may expose vectors and keep stored region bytes in the plan object even if byte vectors are not all public fields. | Architecture Patterns | Medium: planner must balance inspectability with not exposing excessive internals. |
| A4 | Failing sink can be implemented in tests with an injected byte threshold. | Code Examples | Low: `byte_sink` is a public virtual interface designed for caller-owned sinks. |

## Open Questions

1. **Should the harness writer be public-test-visible through `writer_target` or entirely private to tests?**
   - What we know: Phase 8 requires public or test-visible writer foundation types, but the fake harness format itself must not become a public fake archive format. [VERIFIED: `08-SPEC.md`, `08-CONTEXT.md`]
   - What's unclear: Whether the public target descriptor needs a generic/custom variant now or whether tests can use internal helpers to exercise the same planner. [VERIFIED: `08-CONTEXT.md` leaves exact type names/details to planner]
   - Recommendation: Keep target capability descriptor public but make harness-specific serializer/reader helpers test-only. [VERIFIED: `08-CONTEXT.md`]

2. **How should finalization report partial writes after sink failure?**
   - What we know: It must propagate structured sink failure and not expose partial success as success. [VERIFIED: `08-SPEC.md`]
   - What's unclear: Current `error_code` lacks a writer-specific category; `io_failure` is the closest existing code. [VERIFIED: `include/libbsa/result.hpp`]
   - Recommendation: Use `error_code::io_failure` for sink failures unless the planner adds writer-specific messages within existing codes. [VERIFIED: `include/libbsa/result.hpp`]

3. **Do Phase 8 tests need no-partial-write semantics for finalization?**
   - What we know: Acceptance requires structured sink failure; prior DDS extraction intentionally touches the sink only after validation succeeds. [VERIFIED: `08-SPEC.md`, `src/ba2_reader.cpp`]
   - What's unclear: Streaming archive finalization cannot generally roll back caller-owned sinks after some chunks have been accepted. [ASSUMED]
   - Recommendation: Assert failure propagation and exact byte count from a failing sink; do not promise rollback semantics for arbitrary streaming sinks. [VERIFIED: `include/libbsa/io.hpp`]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build and file-set wiring | ✓ | 4.3.2 [VERIFIED: `cmake --version`] | — |
| CTest | Test execution | ✓ | 4.3.2 [VERIFIED: `ctest --version`] | — |
| Git | TES5Edit status/boundary gate and optional commit | ✓ | 2.54.0.windows.1 [VERIFIED: `git --version`] | — |
| vcpkg toolchain file | Preset dependency resolution | ✓ | `C:\vcpkg\scripts\buildsystems\vcpkg.cmake` exists [VERIFIED: `Test-Path`] | — |
| vcpkg CLI on PATH | Dependency maintenance commands | ✗ | `vcpkg` not on PATH, but `C:\vcpkg\vcpkg.exe` exists [VERIFIED: shell audit] | Invoke `C:\vcpkg\vcpkg.exe` or rely on CMake toolchain. |
| MSVC `cl` on PATH | Local compile from plain shell | ✗ | Not in current PATH [VERIFIED: shell audit] | Use Visual Studio/CMake preset environment or developer shell. |
| Ninja | Alternative generator | ✗ | Not found [VERIFIED: shell audit] | Existing preset uses Visual Studio generator. [VERIFIED: `CMakePresets.json`] |

**Missing dependencies with no fallback:**
- None for planning. [VERIFIED: environment audit]

**Missing dependencies with fallback:**
- `vcpkg` is not on PATH, but the configured toolchain and executable exist under `C:\vcpkg`. [VERIFIED: environment audit]
- `cl` is not on PATH in this shell; use the Visual Studio preset/developer environment. [VERIFIED: environment audit, `CMakePresets.json`]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 `3.14.0#0` via vcpkg package page. [CITED: https://vcpkg.io/en/package/catch2.html] |
| Config file | `CMakeLists.txt` with explicit test executables and `catch_discover_tests`. [VERIFIED: `CMakeLists.txt`] |
| Quick run command | `ctest --preset windows-msvc-vcpkg -R libbsa_writer_tests --output-on-failure` [ASSUMED] |
| Full suite command | `ctest --preset windows-msvc-vcpkg --output-on-failure` [VERIFIED: `CMakePresets.json`] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| WRT-05 | Finalize through caller-owned `byte_sink`, successful output size equals plan size, sink failure returns structured error. [VERIFIED: `08-SPEC.md`] | unit/fixture | `ctest --preset windows-msvc-vcpkg -R libbsa_writer_tests --output-on-failure` [ASSUMED] | ❌ Wave 0 |
| WRT-06 | Dedup enabled shares post-policy data regions on supported target; disabled mode emits distinct regions; unsupported target fails. [VERIFIED: `08-SPEC.md`] | unit/fixture/codec | `ctest --preset windows-msvc-vcpkg -R libbsa_writer_tests --output-on-failure` [ASSUMED] | ❌ Wave 0 |
| WRT-07 | Harness read-after-write compares metadata, offsets, regions, compression states, dedup refs, and extracted bytes. [VERIFIED: `08-SPEC.md`] | fixture/roundtrip | `ctest --preset windows-msvc-vcpkg -R libbsa_writer_tests --output-on-failure` [ASSUMED] | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** `ctest --preset windows-msvc-vcpkg -R "libbsa_writer_tests|libbsa_compression_policy_tests|libbsa.public_header_smoke" --output-on-failure` [ASSUMED]
- **Per wave merge:** `ctest --preset windows-msvc-vcpkg --output-on-failure` [VERIFIED: `CMakePresets.json`]
- **Phase gate:** Full suite green, public-header dependency leakage check, CMake source-list gate, and `git status --short TES5Edit` clean. [VERIFIED: `08-SPEC.md`, `CMakeLists.txt`]

### Wave 0 Gaps

- [ ] `include/libbsa/writer.hpp` — public writer input/target/options/plan/finalize API for WRT-05/WRT-06/WRT-07. [ASSUMED]
- [ ] `src/writer.cpp` — planning, checked arithmetic, stored payload planning, dedup grouping, and finalizer. [ASSUMED]
- [ ] `tests/writer_core_tests.cpp` — plan determinism, preview, dedup, compression, finalization, sink failure, invalid inputs. [ASSUMED]
- [ ] `tests/writer_harness_helpers.hpp/.cpp` — test-only generated harness builder/reader and assertions. [ASSUMED]
- [ ] `CMakeLists.txt` updates — explicit public header, source, and test target wiring. [VERIFIED: `CMakeLists.txt`]
- [ ] `tests/public_header_smoke.cpp` update — consumer-style writer compile/link coverage. [VERIFIED: `tests/public_header_smoke.cpp`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | No authentication surface in reusable archive writer core. [VERIFIED: `08-SPEC.md`] |
| V3 Session Management | no | No sessions or network state. [VERIFIED: `08-SPEC.md`] |
| V4 Access Control | no | No authorization boundary; caller supplies in-memory entries. [VERIFIED: `08-SPEC.md`] |
| V5 Input Validation | yes | Normalize paths, reject duplicates/unsupported target/options, check offsets/sizes, and propagate structured failures. [VERIFIED: `08-SPEC.md`, `include/libbsa/archive_path.hpp`] |
| V6 Cryptography | no | No cryptography; dedup public identity must not expose content hashes. [VERIFIED: `08-CONTEXT.md`] |

### Known Threat Patterns for C++ Binary Writer Core

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Integer overflow in layout offsets/sizes | Tampering / Denial of Service | Central checked arithmetic helpers and overflow tests. [VERIFIED: `08-SPEC.md`] |
| Path traversal or host path confusion | Tampering | Use `normalize_archive_path`; do not use `std::filesystem::path` for archive paths. [VERIFIED: `include/libbsa/archive_path.hpp`] |
| Codec confusion | Tampering / Denial of Service | Route through `resolve_write_compression`, `resolve_payload_codec`, and `compress_payload`; no fallback probing. [VERIFIED: `include/libbsa/compression.hpp`, `src/compression.cpp`] |
| Unbounded output buffering | Denial of Service | Stream final archive chunks to `byte_sink`; plan owns per-region stored payloads by locked decision but finalization must not require whole archive image buffering. [VERIFIED: `08-SPEC.md`, `08-CONTEXT.md`] |
| Partial/ignored sink failure | Repudiation / Integrity | Return first sink `io_failure` and test custom failing sinks. [VERIFIED: `include/libbsa/io.hpp`, `08-SPEC.md`] |

## Sources

### Primary (HIGH confidence)
- `.planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-CONTEXT.md` — locked implementation decisions, dedup semantics, harness shape, code integration points. [VERIFIED]
- `.planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-SPEC.md` — locked requirements, boundaries, constraints, and acceptance criteria. [VERIFIED]
- `.planning/REQUIREMENTS.md` — WRT-05, WRT-06, WRT-07 definitions and traceability. [VERIFIED]
- `.planning/ROADMAP.md` — Phase 8 position and deferred production writer phases. [VERIFIED]
- `AGENTS.md` — TES5Edit boundary, dependency policy, comments/docstrings, validation expectations. [VERIFIED]
- `include/libbsa/io.hpp`, `compression.hpp`, `archive.hpp`, `archive_path.hpp`, `archive_view.hpp`, `result.hpp` — existing public primitives. [VERIFIED]
- `src/compression.cpp`, `src/ba2_reader.cpp`, `src/bsa_reader.cpp` — current codec dispatch and sink emission patterns. [VERIFIED]
- `tests/*`, `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json` — test/build/dependency patterns. [VERIFIED]
- Context7 `/catchorg/catch2` — Catch2 CMake integration and `catch_discover_tests` arguments. [CITED: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md]
- Context7 `/kitware/cmake` — `target_sources(FILE_SET HEADERS)` and install behavior. [CITED: https://github.com/kitware/cmake/blob/master/Help/command/install.rst]
- Microsoft Learn vcpkg manifest mode — manifest-mode and baseline behavior. [CITED: https://learn.microsoft.com/vcpkg/concepts/manifest-mode]

### Secondary (MEDIUM confidence)
- vcpkg package pages for libdeflate, lz4, Catch2 versions and update dates. [CITED: https://vcpkg.io/en/package/libdeflate.html, https://vcpkg.io/en/package/lz4.html, https://vcpkg.io/en/package/catch2.html]
- ccc semantic search results for writer planning, sink behavior, and fixture patterns. [VERIFIED: ccc index/search]

### Tertiary (LOW confidence)
- Assumed exact names and file organization; all such claims are listed in the Assumptions Log. [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — existing stack and package versions were verified via codebase, vcpkg manifest/pages, Microsoft Learn, and Context7. [VERIFIED/CITED]
- Architecture: HIGH — phase context and spec lock the API shape, plan semantics, dedup order, and harness boundary. [VERIFIED]
- Pitfalls: HIGH — most pitfalls are direct inversions of locked decisions and acceptance criteria. [VERIFIED]
- Environment: MEDIUM — local shell probes verify current availability, but MSVC availability can differ in a Visual Studio developer environment. [VERIFIED]

**Research date:** 2026-05-06  
**Valid until:** 2026-06-05 for project decisions; re-check package/tool versions before dependency or CI changes. [ASSUMED]
