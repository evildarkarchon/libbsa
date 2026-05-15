# Phase 17: Writer Hotspot Hardening and Ship Gate - Research

**Researched:** 2026-05-14
**Domain:** C++20 archive writer hardening, dedupe candidate narrowing, BA2 DX10 temp lifecycle, v1.1 ship gate
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### BA2 DX10 writer lifecycle
- **D-01:** BA2 DX10 `write_to` is terminal after any ordinary `write_to` attempt, whether it succeeds or returns a `result` error.
- **D-02:** After a BA2 DX10 writer is consumed, later `add_file` and `write_to` calls return `invalid_argument` through the existing `result` contract. They must not throw, silently no-op, or partially reuse stale staged entries.
- **D-03:** The consumed-writer rule is specific to BA2 DX10 because that writer owns snapshot temp data. Phase 17 must not standardize one-shot behavior across TES3, TES4-family BSA, or BA2 GNRL writers.
- **D-04:** Cleanup takes priority over retryability for BA2 DX10. Downstream planning should not preserve retry-after-failure behavior by retaining snapshot temp data or long-lived duplicate DDS byte storage.

### Dedupe proof and guardrails
- **D-05:** Dedupe narrowing proof should combine runtime regression tests with source/policy guardrails. Runtime tests prove archive behavior; policy tests prevent drift back to unkeyed or all-prior-payload scans.
- **D-06:** Performance evidence should be algorithmic and non-flaky: prove that exact comparisons are bounded behind keyed, identity, or digest candidate narrowing rather than adding timing-sensitive benchmarks.
- **D-07:** TES4 and BA2 GNRL dedupe narrowing should stay in format-local private helpers near their existing layout code unless planning finds a very small shared internal helper that does not become a generic dedupe framework.
- **D-08:** Hashes, digests, sizes, or staged identity values are candidate filters only. Exact final stored-byte equality remains mandatory before any shared payload offset is assigned.

### BA2 DX10 cleanup cases
- **D-09:** Cleanup tests must explicitly cover all named ordinary failure families: validation failure, missing or truncated snapshot data before publish, and output or publish failure.
- **D-10:** If cleanup itself fails during ordinary failure unwinding, the public result should preserve the primary validation, write, or publish error. Cleanup remains best-effort unless a later requirement changes the public contract.
- **D-11:** Phase 17 should also clean up BA2 DX10 temp directories created by failed `add_file` calls when `add_file` reserved a snapshot directory before failing.
- **D-12:** Cleanup tests should identify only new writer-owned `libbsa-dx10-snapshot-*` directories created during the test and assert those are removed. Tests must not require the entire system temp root to be free of unrelated directories.

### Ship gate evidence
- **D-13:** Phase 17 should close with a focused combined gate: focused Debug writer/runtime tests, focused MSVC AddressSanitizer hardening tests for the risky writer paths, and Release package proof for install/export and downstream consumer behavior.
- **D-14:** Optional local game-corpus or BSArchPro comparison tests remain advisory and are not required for the official v1.1 ship gate. The closure gate must be runnable from committed assets.
- **D-15:** BA2 DX10 temp lifecycle guarantees and residual abnormal-termination risk must be documented in public or maintainer-facing docs as well as Phase 17 planning/verification artifacts.
- **D-16:** If Phase 17 passes, updates to `.planning/REQUIREMENTS.md`, `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`, and verification evidence belong in the same closure plan so milestone status does not drift.

### the agent's Discretion
None. The user selected concrete decisions for writer lifecycle, dedupe proof style, cleanup coverage, documentation surface, and ship-gate evidence. Downstream agents should not reopen those choices.

### Deferred Ideas (OUT OF SCOPE)
## Deferred Ideas

None - discussion stayed within Phase 17 scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DEDU-01 | Consumer can write TES4-family archives with dedupe enabled using faster candidate narrowing while preserving exact stored-byte equality behavior. | Use a TES4 format-local dedupe bucket keyed before `tes4_stored_payloads_equal`, then policy-test that unkeyed all-prior scans do not return. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`, `src/formats/bsa/tes4_bsa_layout.cpp`] |
| DEDU-02 | Consumer can write BA2 GNRL archives with dedupe enabled using stronger staged identity or digest narrowing while preserving exact stored-byte equality fallback behavior. | Extend the existing size/hash bucket into an explicit prepared-entry identity/digest key while keeping `ba2_gnrl_payloads_equal` as the only authority for shared offsets. [VERIFIED: `17-SPEC.md`, `src/formats/ba2/ba2_gnrl_layout.cpp`, `src/formats/ba2/ba2_gnrl_prepare.hpp`] |
| DX10-01 | Consumer can write BA2 DX10 archives with temporary staging data cleaned up during normal write completion and ordinary failure unwinding. | Add consumed-state and explicit snapshot cleanup in `ba2_dx10_writer::state`, then test success, validation failure, missing/truncated snapshot, publish/output failure, and failed `add_file` reservation cleanup using writer-owned temp directory deltas. [VERIFIED: `17-CONTEXT.md`, `17-SPEC.md`, `src/formats/ba2/ba2_dx10_writer.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`] |
| DX10-02 | Maintainer can verify and document remaining BA2 DX10 temporary-data lifecycle behavior, including residual abnormal-termination risk. | Update public or maintainer docs plus phase verification evidence to distinguish successful completion, ordinary `result` failure, destructor safety-net cleanup, and residual crash/termination/OS-shutdown risk. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`, `docs/target-format-guide.md`, `docs/integration-examples.md`] |
</phase_requirements>

## Summary

Phase 17 should be planned as three implementation tracks plus one closure gate: TES4 dedupe narrowing, BA2 GNRL dedupe identity/digest hardening, BA2 DX10 snapshot lifecycle cleanup, and v1.1 ship evidence. [VERIFIED: `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/phases/17-writer-hotspot-hardening-and-ship-gate/17-SPEC.md`] The phase is explicitly hardening-only: no public writer API expansion, no dedupe default changes, no generic writer redesign, no crash-proof cleanup claim, no new dependencies, and no `TES5Edit/` edits. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`, `AGENTS.md`]

The most important planning constraint is that dedupe filters are only candidate narrowing, not correctness authorities. [VERIFIED: `17-CONTEXT.md`] TES4 currently compares every prior dedupe-owned payload with `tes4_stored_payloads_equal`, so the plan should add a keyed bucket around that exact comparison rather than changing final equality semantics. [VERIFIED: `src/formats/bsa/tes4_bsa_layout.cpp`] BA2 GNRL already buckets by stored size and `payload_hash`, but Phase 17 requires making the staged identity/digest evidence explicit and guardrailed while preserving disk-source growth/truncation rejection in `ba2_gnrl_payloads_equal`. [VERIFIED: `src/formats/ba2/ba2_gnrl_layout.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `17-SPEC.md`]

BA2 DX10 cleanup should be planned as a consuming `write_to` lifecycle change localized to `ba2_dx10_writer::state` and snapshot-builder integration, not as a cross-writer behavior standardization. [VERIFIED: `17-CONTEXT.md`, `src/formats/ba2/ba2_dx10_writer.cpp`] The writer currently removes `snapshot_dir` only in `state::~state`, while tests only prove teardown cleanup; Phase 17 must prove cleanup immediately after successful and ordinary failing `write_to` calls while the writer object remains alive. [VERIFIED: `src/formats/ba2/ba2_dx10_writer.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `17-SPEC.md`]

**Primary recommendation:** Plan four waves: (1) TES4 keyed dedupe narrowing and policy guard, (2) BA2 GNRL explicit final-stored identity/digest narrowing and policy guard, (3) BA2 DX10 consumed-state plus explicit snapshot cleanup across success/failure/add failure, and (4) documentation + focused Debug/ASan/Release package ship gate evidence. [VERIFIED: `17-CONTEXT.md`, `17-SPEC.md`, `README.md`, `CMakePresets.json`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| TES4 dedupe candidate narrowing | Format pipeline (`src/formats/bsa/`) | Shared detail only for existing host-file helpers | TES4 offset assignment and exact stored-payload equality already live in `tes4_bsa_layout.cpp`; keeping narrowing format-local avoids a generic dedupe framework. [VERIFIED: `src/formats/bsa/tes4_bsa_layout.cpp`, `17-CONTEXT.md`] |
| BA2 GNRL staged identity/digest narrowing | Format pipeline (`src/formats/ba2/`) | Existing BA2 prepare structs | BA2 GNRL preparation computes stored payload and `payload_hash`, while layout assigns offsets and calls exact equality. [VERIFIED: `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`] |
| BA2 DX10 snapshot cleanup and consumed state | Writer orchestration (`ba2_dx10_writer::state`) | Snapshot-builder seam and publish helper | The writer owns `snapshot_dir`, public mutation/finalization methods, and the top-level write pipeline; snapshot-builder creates the directory and files. [VERIFIED: `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`] |
| Publish/output failure interop | Shared detail publish service | BA2 DX10 writer cleanup wrapper | `publish_writer_output` owns temporary output cleanup and returns primary write/publish errors; BA2 DX10 cleanup must run around it without replacing those primary errors. [VERIFIED: `src/detail/writer_publish.hpp`, `src/detail/writer_publish.cpp`, `17-CONTEXT.md`] |
| Lifecycle documentation and ship evidence | Docs/planning verification layer | Tests and policy suites | Phase 17 explicitly requires public or maintainer-facing docs plus planning/verification evidence and consistent planning-state updates. [VERIFIED: `17-CONTEXT.md`, `17-SPEC.md`] |

## Project Constraints (from AGENTS.md)

- libbsa is a Windows-only C++20 reusable library for Bethesda archive formats. [VERIFIED: `AGENTS.md`]
- `TES5Edit/` is read-only reference material and must not be edited, formatted, staged, compiled into libbsa, or have its submodule pointer updated. [VERIFIED: `AGENTS.md`]
- Implementation must remain outside `TES5Edit/` and preserve BSArchPro-compatible behavior unless divergence is documented. [VERIFIED: `AGENTS.md`]
- Public API design should stay clean, idiomatic, reusable, and independent of application UI/tooling. [VERIFIED: `AGENTS.md`]
- Do not introduce speculative dependencies; required dependencies remain `libdeflate`, official `lz4`, `DirectXTex`, and vcpkg. [VERIFIED: `AGENTS.md`, `vcpkg.json`]
- Add focused tests for parsing, writing, round-tripping, and compatibility behavior; do not use `TES5Edit/` as a mutable fixture. [VERIFIED: `AGENTS.md`]
- Never keep production/library code solely for test compatibility; update or remove obsolete tests instead of adding test-only shims. [VERIFIED: `AGENTS.md`]
- Never delete accurate comments as cleanup; add Doxygen comments for public APIs and methods added or substantially rewritten. [VERIFIED: `AGENTS.md`]

## Standard Stack

### Core

| Library / Tool | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| C++ | C++20 via CMake presets | Library implementation and internal hardening | Public headers and presets already lock C++20 behavior; Phase 17 should not introduce C++23-only public types. [VERIFIED: `CMakePresets.json`, `include/libbsa/writer.hpp`, `AGENTS.md`] |
| CMake / CTest | 4.3.2 installed locally; presets require CMake 4.0 minimum | Configure/build/test and verification lanes | Existing project lanes use CMake/CTest presets and Catch2 discovery. [VERIFIED: `cmake --version`, `ctest --version`, `CMakePresets.json`, `tests/CMakeLists.txt`] |
| vcpkg | 2026-04-08 installed locally | Dependency acquisition | Project manifest lists approved dependencies through vcpkg. [VERIFIED: `vcpkg version`, `vcpkg.json`, `AGENTS.md`] |
| Catch2 | Manifest dependency, resolved by vcpkg at configure time | Runtime and policy tests | Existing tests use Catch2 macros, tags, and `catch_discover_tests`. [VERIFIED: `vcpkg.json`, `tests/CMakeLists.txt`, `.planning/codebase/TESTING.md`] |
| nlohmann-json | Manifest dependency, resolved by vcpkg at configure time | Existing manifest-driven tests | Test target links `nlohmann_json::nlohmann_json`; Phase 17 can reuse manifest helpers if needed. [VERIFIED: `vcpkg.json`, `tests/CMakeLists.txt`] |

### Supporting

| Library / Helper | Version / Location | Purpose | When to Use |
|------------------|--------------------|---------|-------------|
| `detail::host_file` helpers | `src/detail/host_file.hpp/.cpp` | Resolved Windows host-file reads and chunk iteration | Use for disk-backed dedupe reads instead of reopening raw narrow string paths. [VERIFIED: `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, Phase 13 state in `.planning/STATE.md`] |
| `detail::publish_writer_output` | `src/detail/writer_publish.hpp` | Temporary output reservation, publish, and output-temp cleanup | Keep BA2 DX10 snapshot cleanup separate but ordered around this helper so output-temp cleanup remains centralized. [VERIFIED: `src/detail/writer_publish.hpp`, `src/detail/writer_publish.cpp`] |
| `ba2_dx10_snapshot_builder` seam | `src/formats/ba2/ba2_dx10_snapshot_builder.*` | Snapshot directory reservation and DDS subresource snapshot file creation | Extend/add cleanup around failed `add_file` reservations and preserve Phase 16 seam separation. [VERIFIED: `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`, `tests/unit/parser_preparer_seam_policy_tests.cpp`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Format-local TES4/BA2 GNRL dedupe helpers | Generic internal dedupe framework | Rejected for Phase 17 because D-07 locks format-local private helpers unless a tiny shared helper is clearly warranted. [VERIFIED: `17-CONTEXT.md`] |
| Algorithmic source/policy proof | Timing benchmark suite | Rejected because D-06 requires non-flaky algorithmic proof instead of timing-sensitive benchmarks. [VERIFIED: `17-CONTEXT.md`] |
| One-shot behavior for all writers | Standardize `write_to` consumption across TES3/TES4/BA2 GNRL/BA2 DX10 | Rejected because D-03 limits consumed-writer behavior to BA2 DX10 snapshot ownership. [VERIFIED: `17-CONTEXT.md`] |
| Crash-proof temp cleanup claim | Document residual abnormal-termination risk | Crash-proof cleanup is out of scope unless implemented and tested; Phase 17 should document residual risk. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`] |
| New digest dependency | Existing/private local fingerprinting plus exact fallback | No new external dependency is allowed; candidate filters do not need to be security authorities because exact equality remains mandatory. [VERIFIED: `AGENTS.md`, `17-CONTEXT.md`] |

**Installation:** No new packages should be added for Phase 17. [VERIFIED: `17-SPEC.md`, `AGENTS.md`, `vcpkg.json`]

```powershell
# Existing supported setup; no new dependency install is planned.
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset windows-msvc-debug-static
```

**Version verification:** CMake 4.3.2, CTest 4.3.2, and vcpkg `2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1` were available in the current shell. [VERIFIED: local shell probes] Plain `cl` was not on the current PowerShell PATH, so MSVC commands may need a Visual Studio Developer shell or preset-driven environment setup. [VERIFIED: local shell probe]

## Architecture Patterns

### System Architecture Diagram

```text
Public writer calls
  |
  | add_file/add_bytes
  v
Writer-owned state (TES4 / BA2 GNRL / BA2 DX10)
  |
  +--> TES4 prepare -> TES4 layout dedupe key bucket -> exact tes4_stored_payloads_equal -> serialize -> publish
  |
  +--> BA2 GNRL prepare (stored bytes/hash/source identity) -> explicit dedupe key bucket -> exact ba2_gnrl_payloads_equal -> serialize -> publish
  |
  +--> BA2 DX10 add_file -> snapshot_builder temp dir/files
                         -> write_to consumes writer
                         -> validate/prepare/layout/serialize/publish
                         -> cleanup snapshot_dir on success or ordinary result failure
                         -> later add_file/write_to return invalid_argument
```

This data-flow follows the repository writer pipeline: validate, prepare, layout, serialize to temp output, publish. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`, `src/formats/ba2/ba2_dx10_writer.cpp`]

### Recommended Project Structure

```text
src/
├── formats/bsa/tes4_bsa_layout.cpp          # TES4 dedupe keying and exact equality remain together
├── formats/ba2/ba2_gnrl_prepare.hpp/.cpp    # BA2 GNRL prepared identity/digest field, if needed
├── formats/ba2/ba2_gnrl_layout.cpp          # BA2 GNRL dedupe keying and exact equality remain together
├── formats/ba2/ba2_dx10_writer.cpp          # consumed state and explicit snapshot cleanup owner
├── formats/ba2/ba2_dx10_snapshot_builder.*  # snapshot reservation/build seam; add-time failure cleanup interop
└── detail/writer_publish.*                  # unchanged output-temp publish boundary

tests/unit/
├── tes4_bsa_writer_tests.cpp                # runtime dedupe behavior coverage
├── ba2_gnrl_writer_tests.cpp                # runtime dedupe/disk-source coverage
├── ba2_dx10_writer_tests.cpp                # success/failure/add cleanup and consumed-state coverage
└── writer_hotspot_policy_tests.cpp          # new focused source-policy guardrails for Phase 17
```

Planner should register any new test file in `tests/CMakeLists.txt`; existing policy patterns use dedicated suites rather than expanding unrelated policy files. [VERIFIED: `tests/CMakeLists.txt`, `17-CONTEXT.md`, `tests/unit/parser_preparer_seam_policy_tests.cpp`]

### Pattern 1: Candidate Narrowing Before Exact Equality

**What:** Bucket by cheap final-stored-byte identity facts, then run exact stored-byte equality only inside the bucket. [VERIFIED: `17-CONTEXT.md`, `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`]

**When to use:** TES4-family BSA and BA2 GNRL dedupe when `deduplicate_payloads = true`. [VERIFIED: `17-SPEC.md`]

**Example:**

```cpp
// Source: existing layout pattern in src/formats/ba2/ba2_gnrl_layout.cpp, adapted for Phase 17 planning.
const auto key = make_final_stored_payload_key(entry);
auto duplicate_bucket = deduplicated_payloads.find(key);
if (duplicate_bucket != deduplicated_payloads.end()) {
  for (const auto& candidate : duplicate_bucket->second) {
    auto equal = exact_stored_payloads_equal(entry, prior_entries[candidate.entry_index]);
    if (!equal) {
      return equal.error();
    }
    if (equal.value()) {
      entry.payload_offset = candidate.offset;
      entry.owns_payload_bytes = false;
      break;
    }
  }
}
```

### Pattern 2: Consuming BA2 DX10 `write_to` Cleanup Wrapper

**What:** Mark BA2 DX10 writer consumed when `write_to` is attempted, preserve the primary result, and always run best-effort snapshot cleanup for ordinary completion/failure. [VERIFIED: `17-CONTEXT.md`, `17-SPEC.md`]

**When to use:** Only `ba2_dx10_writer`, because it owns snapshot temp data. [VERIFIED: `17-CONTEXT.md`]

**Example:**

```cpp
// Source: current ba2_dx10_writer.cpp state ownership and writer_publish primary-error pattern.
result<void> ba2_dx10_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (state_->consumed) {
    return error{error_code::invalid_argument, "BA2 DX10 writer has already been consumed"};
  }
  state_->consumed = true;

  auto written = formats::ba2::write_ba2_dx10_archive(...);
  state_->cleanup_snapshots_noexcept(); // best-effort; do not replace written.error().
  return written;
}
```

Planner should require implementation comments explaining why cleanup preserves the primary `result` error and why BA2 DX10 is one-shot. [VERIFIED: `AGENTS.md`, `17-CONTEXT.md`]

### Pattern 3: Test-Owned Snapshot Directory Delta

**What:** Record snapshot temp directories before the operation, identify only new `libbsa-dx10-snapshot-*` directories created by the writer, then assert those paths are removed. [VERIFIED: `17-CONTEXT.md`, `tests/unit/ba2_dx10_writer_tests.cpp`]

**When to use:** BA2 DX10 success, failure, and failed-add cleanup tests. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`]

**Example:**

```cpp
// Source: existing snapshot_directories helper in tests/unit/ba2_dx10_writer_tests.cpp.
const auto before = snapshot_directories();
libbsa::ba2_dx10_writer writer{libbsa::ba2_dx10_target::fallout4};
REQUIRE(writer.add_file(archive_path, source_path).has_value());
const auto created = snapshot_directories() - before; // implement as set_difference helper.
REQUIRE_FALSE(created.empty());

auto written = writer.write_to(output.string());
CHECK(written.has_value());
for (const auto& path : created) {
  CHECK_FALSE(std::filesystem::exists(path));
}
```

### Anti-Patterns to Avoid

- **Hash-only dedupe sharing:** A hash/digest collision must not assign a shared payload offset; exact equality is mandatory. [VERIFIED: `17-CONTEXT.md`, `17-SPEC.md`]
- **TES4 all-prior scan regression:** Reintroducing a linear unkeyed scan over every prior payload violates DEDU-01 and D-06. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`, `src/formats/bsa/tes4_bsa_layout.cpp`]
- **BA2 GNRL disk-source blind trust:** Disk sources that grow or truncate after preparation must still return `io_error` rather than publishing malformed offsets. [VERIFIED: `src/formats/ba2/ba2_gnrl_layout.cpp`, `17-SPEC.md`]
- **Cross-writer consumed-state standardization:** TES3, TES4, and BA2 GNRL must not become one-shot as part of Phase 17. [VERIFIED: `17-CONTEXT.md`]
- **Destructor-only DX10 cleanup proof:** Destructor cleanup remains a safety net; it is not sufficient for DX10-01. [VERIFIED: `17-SPEC.md`, `src/formats/ba2/ba2_dx10_writer.cpp`]
- **Global temp-root cleanliness assertion:** Tests must track writer-owned new directories instead of assuming no unrelated `libbsa-dx10-snapshot-*` directories exist. [VERIFIED: `17-CONTEXT.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Output publication cleanup | A BA2 DX10-specific final-output temp publisher | `detail::publish_writer_output` | Existing helper already owns validation, temp output directory cleanup, and primary error propagation. [VERIFIED: `src/detail/writer_publish.hpp`, `src/detail/writer_publish.cpp`] |
| Generic dedupe framework | A cross-format dedupe abstraction | Format-local private helpers near TES4/BA2 GNRL layout | D-07 locks format-local work unless a tiny shared helper is clearly better. [VERIFIED: `17-CONTEXT.md`] |
| Timing benchmark gate | Wall-clock perf benchmark | Source/policy proof of keyed candidate narrowing | D-06 requires algorithmic, non-flaky proof. [VERIFIED: `17-CONTEXT.md`] |
| Crash-proof cleanup mechanism | Watchdog/service/global temp sweeper | Best-effort cleanup plus truthful lifecycle docs | Crash-proof cleanup is out of scope and may overpromise behavior not implemented/tested. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`] |
| External digest dependency | OpenSSL/Boost/new hash library | Existing/private digest/fingerprint helper plus exact equality fallback | Project forbids new speculative dependencies. [VERIFIED: `AGENTS.md`, `vcpkg.json`] |

**Key insight:** Phase 17 hardens narrow hotspots; any plan that adds broad architecture, public API, dependency, or cross-writer lifecycle changes is larger than the locked phase boundary. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`]

## Common Pitfalls

### Pitfall 1: Turning candidate filters into correctness authorities
**What goes wrong:** TES4 or BA2 GNRL shares offsets based on stored size, hash, digest, or source identity alone. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`]
**Why it happens:** The phase asks for faster narrowing, which can be misread as replacing exact byte equality. [VERIFIED: `17-CONTEXT.md`]
**How to avoid:** Keep `tes4_stored_payloads_equal` and `ba2_gnrl_payloads_equal` in the final branch before every shared offset assignment. [VERIFIED: `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`]
**Warning signs:** Policy tests cannot find exact equality calls in the dedupe sharing path, or runtime tests still pass only because test data has no collision scenario. [ASSUMED]

### Pitfall 2: BA2 DX10 cleanup hides the primary error
**What goes wrong:** A cleanup failure replaces the validation, missing-snapshot, truncated-snapshot, write, or publish failure that callers need. [VERIFIED: `17-CONTEXT.md`]
**Why it happens:** Cleanup is tempting to report as a new error path. [ASSUMED]
**How to avoid:** Make snapshot cleanup best-effort on ordinary failure unwinding and return the saved primary `result` error. [VERIFIED: `17-CONTEXT.md`, `src/detail/writer_publish.hpp`]
**Warning signs:** Tests assert cleanup error details instead of the original failure code/message. [ASSUMED]

### Pitfall 3: Failed `add_file` leaves a reserved snapshot directory
**What goes wrong:** `ba2_dx10_ensure_snapshot_directory` reserves a temp directory, then source load/analysis/format validation/snapshot write fails, leaving a directory until writer destruction. [VERIFIED: `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`, `17-CONTEXT.md`]
**Why it happens:** Current `add_file` reserves before `ba2_dx10_make_writer_entry` and only destructor cleanup is proven. [VERIFIED: `src/formats/ba2/ba2_dx10_writer.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`]
**How to avoid:** Plan explicit cleanup if `add_file` created the first snapshot directory and then returns an error without staging an entry. [VERIFIED: `17-CONTEXT.md`]
**Warning signs:** A failed malformed/unsupported DDS `add_file` test sees a new `libbsa-dx10-snapshot-*` directory while writer remains alive. [VERIFIED: `tests/unit/ba2_dx10_writer_tests.cpp`, `17-CONTEXT.md`]

### Pitfall 4: Policy tests freeze helper names too tightly
**What goes wrong:** Policy tests break on safe helper renames rather than semantic regressions. [VERIFIED: `17-CONTEXT.md`, `tests/unit/parser_preparer_seam_policy_tests.cpp`]
**Why it happens:** Source-policy tests can overfit exact names. [ASSUMED]
**How to avoid:** Assert role evidence and forbidden collapse patterns: keyed buckets exist, all-prior TES4 scans are absent, BA2 GNRL exact equality remains in the sharing branch, and BA2 DX10 cleanup/consumed-state tokens exist. [VERIFIED: `17-CONTEXT.md`, `tests/unit/parser_preparer_seam_policy_tests.cpp`]
**Warning signs:** Tests require exact new helper names not present in any requirement or decision. [ASSUMED]

## Code Examples

Verified patterns from repository sources:

### TES4 exact equality remains authority

```cpp
// Source: src/formats/bsa/tes4_bsa_layout.cpp
if (deduplicate_payloads) {
  for (const auto& candidate : bucket_for(entry_key)) {
    auto duplicate = tes4_stored_payloads_equal(entry, *candidate.entry);
    if (!duplicate) {
      return duplicate.error();
    }
    if (duplicate.value()) {
      entry.payload_offset = candidate.assignment.offset;
      entry.owns_payload_bytes = false;
      break;
    }
  }
}
```

### BA2 GNRL disk-source equality must reject changed source size

```cpp
// Source: src/formats/ba2/ba2_gnrl_layout.cpp
char extra = '\0';
if (lhs.get(extra) || rhs.get(extra)) {
  return error{error_code::io_error, "BA2 GNRL disk source changed during dedupe preparation"};
}
```

### Publish helper preserves primary write/publish errors

```cpp
// Source: src/detail/writer_publish.hpp
if (!written) {
  cleanup_writer_publish_directory(temp_dir.value());
  return written.error();
}
if (!published) {
  cleanup_writer_publish_directory(temp_dir.value());
  return published.error();
}
```

### BA2 DX10 snapshot directory naming and test discovery

```cpp
// Source: src/formats/ba2/ba2_dx10_snapshot_builder.cpp and tests/unit/ba2_dx10_writer_tests.cpp
const auto candidate = root / ("libbsa-dx10-snapshot-" + suffix.value());
constexpr std::string_view snapshot_directory_prefix = "libbsa-dx10-snapshot-";
```

## State of the Art

| Old Approach | Current Approach for Phase 17 | When Changed / Locked | Impact |
|--------------|--------------------------------|------------------------|--------|
| TES4 linear scan through all prior dedupe payloads | Keyed/bounded candidate buckets before `tes4_stored_payloads_equal` | Locked by Phase 17 SPEC/context on 2026-05-14 | DEDU-01 can be proven algorithmically without timing benchmarks. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`] |
| BA2 GNRL implicit size/hash bucketing only | Explicit staged identity or digest narrowing plus exact fallback | Locked by Phase 17 SPEC/context on 2026-05-14 | DEDU-02 must show stronger narrowing evidence while preserving disk-source rejection. [VERIFIED: `17-SPEC.md`, `src/formats/ba2/ba2_gnrl_layout.cpp`] |
| BA2 DX10 destructor-only snapshot cleanup proof | Explicit cleanup on successful and ordinary failing `write_to`, with destructor as safety net | Locked by Phase 17 SPEC/context on 2026-05-14 | DX10-01 requires cleanup before writer destruction. [VERIFIED: `17-SPEC.md`, `src/formats/ba2/ba2_dx10_writer.cpp`] |
| Retryable BA2 DX10 writer after write failure | Consumed BA2 DX10 writer after any ordinary `write_to` attempt | Locked by D-01 through D-04 | Cleanup predictability takes priority over retryability. [VERIFIED: `17-CONTEXT.md`] |

**Deprecated/outdated:**
- Treating BA2 DX10 teardown cleanup as sufficient is outdated for Phase 17; cleanup must be proven immediately after normal and ordinary failing `write_to`. [VERIFIED: `17-SPEC.md`, `tests/unit/ba2_dx10_writer_tests.cpp`]
- Using timing-sensitive benchmarks as the Phase 17 performance proof is out of scope; use source/policy guardrails and runtime behavior tests. [VERIFIED: `17-CONTEXT.md`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | A policy test that cannot find exact equality calls in the sharing path is a useful warning sign for hash-only dedupe sharing. | Common Pitfalls | Planner may need to make the policy test less source-coupled if implementation hides equality behind a differently named helper. |
| A2 | Cleanup failure reporting is tempting enough to require explicit tests preserving primary errors. | Common Pitfalls | If implementation cannot observe cleanup failure deterministically, policy/documentation proof may be more practical than runtime proof. |
| A3 | Role-based source-policy tests can overfit helper names unless deliberately written around invariants. | Common Pitfalls | Planner should keep policy tests resilient to safe refactors. |

## Open Questions (RESOLVED)

1. **Should TES4 compute a new digest for disk-backed raw payloads or use existing final stored-size/prefix facts as the first key?**
   - What we know: TES4 has `stored_size`, `stored_payload` prefixes, `raw_disk_size`, and exact disk equality helpers. [VERIFIED: `src/formats/bsa/tes4_bsa_prepare.hpp`, `src/formats/bsa/tes4_bsa_layout.cpp`]
   - RESOLVED: Use a format-local final-stored candidate key before `tes4_stored_payloads_equal`, starting with stored size plus cheap prepared in-memory payload/prefix facts. Add a private digest only if implementation review shows those facts cannot bound disk-backed candidates without returning to all-prior-payload scans. Exact stored-byte equality remains the only sharing authority. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`]

2. **How should BA2 GNRL represent “stronger staged identity or digest” without adding dependencies?**
   - What we know: BA2 GNRL already computes `payload_hash` during preparation and layout buckets by `(stored_size, payload_hash)`. [VERIFIED: `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`]
   - RESOLVED: Introduce an explicit private BA2 GNRL dedupe key/fingerprint representation in the prepared-entry/layout path, using existing stored size and prepared payload hash/digest facts rather than a new dependency. Policy tests should assert the named intentional keying path exists, and `ba2_gnrl_payloads_equal` must remain the final authority including disk-source growth/truncation rejection. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`, `src/formats/ba2/ba2_gnrl_layout.cpp`]

3. **Where should lifecycle documentation live?**
   - What we know: `docs/target-format-guide.md` already documents writer temporary output behavior, and `docs/integration-examples.md` already mentions BA2 DX10 snapshotting at add time. [VERIFIED: `docs/target-format-guide.md`, `docs/integration-examples.md`]
   - RESOLVED: Put the durable BA2 DX10 temp lifecycle contract and residual abnormal-termination risk in `docs/target-format-guide.md`. Add a narrow note in `docs/integration-examples.md` if examples mention add-time snapshot behavior, and update `include/libbsa/writer.hpp` only if the consumed-state behavior needs public API-facing documentation for BA2 DX10 writer calls. [VERIFIED: `17-CONTEXT.md`, `docs/target-format-guide.md`, `docs/integration-examples.md`]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build all lanes | ✓ | 4.3.2 | — |
| CTest | Run focused/full test suites | ✓ | 4.3.2 | — |
| vcpkg | Resolve manifest dependencies | ✓ | 2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1 | — |
| MSVC `cl` in current shell PATH | Windows MSVC presets / ASan lane | ✗ in current PowerShell PATH | — | Use Visual Studio Developer PowerShell/Command Prompt or ensure CMake preset generator environment can locate MSVC. |
| Committed fixtures | Runtime writer tests | ✓ | Repository assets | Optional local game-corpus remains advisory only. [VERIFIED: `17-CONTEXT.md`, `tests/fixtures/README.md` via `.planning/codebase/TESTING.md`] |

**Missing dependencies with no fallback:**
- None proven for research; actual execution of MSVC presets must validate MSVC availability in the implementation environment. [VERIFIED: local shell probe]

**Missing dependencies with fallback:**
- Plain `cl` is missing from the current shell PATH; run build/test gates from a Visual Studio Developer environment or configure the environment before executing presets. [VERIFIED: local shell probe]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3 through `Catch2::Catch2WithMain`, discovered by CTest. [VERIFIED: `tests/CMakeLists.txt`] |
| Config file | `tests/CMakeLists.txt`; presets in `CMakePresets.json`. [VERIFIED: `tests/CMakeLists.txt`, `CMakePresets.json`] |
| Quick run command | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|writer_hotspot_policy"` [ASSUMED] |
| Full suite command | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` [VERIFIED: `README.md`, `CMakePresets.json`] |
| ASan gate command | `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure -L "tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|writer_hotspot_policy"` [VERIFIED: `17-CONTEXT.md`, `README.md`, `CMakePresets.json`] |
| Release package proof command | `cmake --build --preset windows-msvc-release-static && ctest --preset windows-msvc-release-static --output-on-failure -R "package_consumer_smoke|package_consumer_runtime_dll_copy"` [VERIFIED: `17-CONTEXT.md`, `README.md`, `tests/CMakeLists.txt`] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| DEDU-01 | TES4 dedupe uses keyed/bounded narrowing and exact stored-byte equality still controls shared offsets. | runtime + source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure -L tes4_bsa_writer` plus policy label after added. | Runtime file ✅ `tests/unit/tes4_bsa_writer_tests.cpp`; policy file ❌ Wave 0 |
| DEDU-02 | BA2 GNRL dedupe uses explicit staged identity/digest narrowing and exact equality still controls shared offsets, including disk-source change rejection. | runtime + source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure -L ba2_gnrl_writer` plus policy label after added. | Runtime file ✅ `tests/unit/ba2_gnrl_writer_tests.cpp`; policy file ❌ Wave 0 |
| DX10-01 | BA2 DX10 cleans snapshot temp data on success, ordinary failure, and failed add reservation; writer is consumed after write attempt. | runtime | `ctest --preset windows-msvc-debug-static --output-on-failure -L ba2_dx10_writer` | ✅ extend `tests/unit/ba2_dx10_writer_tests.cpp` |
| DX10-02 | Lifecycle docs and verification artifacts truthfully describe guarantees and residual abnormal-termination risk. | policy + docs verification | `ctest --preset windows-msvc-debug-static --output-on-failure -L writer_hotspot_policy` after added. | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** Focused affected runtime/policy labels for the touched writer area. [VERIFIED: `17-CONTEXT.md`, `.planning/codebase/TESTING.md`]
- **Per wave merge:** Full Debug suite with `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: `README.md`, `17-CONTEXT.md`]
- **Phase gate:** Focused Debug writer/runtime tests, focused MSVC ASan hardening tests for risky writer paths, and Release package proof. [VERIFIED: `17-CONTEXT.md`]

### Wave 0 Gaps

- [ ] `tests/unit/writer_hotspot_policy_tests.cpp` — covers DEDU-01, DEDU-02, DX10-02 source/docs guardrails. [VERIFIED: `17-CONTEXT.md`; file absent in `tests/CMakeLists.txt`]
- [ ] Add `writer_hotspot_policy_tests.cpp` to `tests/CMakeLists.txt`. [VERIFIED: `tests/CMakeLists.txt`]
- [ ] Extend `tests/unit/ba2_dx10_writer_tests.cpp` with snapshot-directory delta helpers for success, validation failure, missing/truncated snapshot failure, publish/output failure, failed `add_file`, and consumed-state behavior. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`, `tests/unit/ba2_dx10_writer_tests.cpp`]
- [ ] Extend TES4/BA2 GNRL runtime tests only where existing coverage does not prove the new narrowing guard; avoid broad benchmark tests. [VERIFIED: `17-CONTEXT.md`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | No identity/authentication subsystem exists in this local archive library. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`] |
| V3 Session Management | no | No session subsystem exists. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`] |
| V4 Access Control | no | Library does not enforce user authorization; host filesystem permissions are external. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`] |
| V5 Input Validation | yes | Preserve writer validation, duplicate canonical path checks, DDS validation, source size checks, and `result` errors. [VERIFIED: `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`, `include/libbsa/writer.hpp`] |
| V6 Cryptography | limited | BCrypt RNG is used only for random snapshot directory suffixes, not for security claims about dedupe. [VERIFIED: `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`] |

### Known Threat Patterns for C++ archive writer hardening

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Temp data persistence after ordinary failure | Information Disclosure | Explicit best-effort snapshot cleanup on success and `result` failure, plus truthful abnormal-termination documentation. [VERIFIED: `17-SPEC.md`, `17-CONTEXT.md`] |
| Payload offset corruption from hash-only dedupe collision | Tampering | Exact final stored-byte equality before assigning shared offsets. [VERIFIED: `17-CONTEXT.md`, `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`] |
| Disk source changes between prepare and dedupe compare | Tampering | Continue rejecting growth/truncation as `io_error` during exact comparison. [VERIFIED: `src/formats/ba2/ba2_gnrl_layout.cpp`, `17-SPEC.md`] |
| Output temp/publish failure leaves partial or stale artifacts | Tampering / Reliability | Keep `detail::publish_writer_output` as output publication boundary and preserve primary publish errors. [VERIFIED: `src/detail/writer_publish.hpp`, `src/detail/writer_publish.cpp`] |

## Sources

### Primary (HIGH confidence)

- `AGENTS.md` — project constraints, TES5Edit boundary, dependency policy, comments/docs policy, validation expectations.
- `.planning/phases/17-writer-hotspot-hardening-and-ship-gate/17-CONTEXT.md` — locked decisions D-01 through D-16, canonical refs, code insights.
- `.planning/phases/17-writer-hotspot-hardening-and-ship-gate/17-SPEC.md` — locked requirements, acceptance criteria, in/out of scope.
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md` — requirement traceability and current milestone state.
- `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/TESTING.md`, `.planning/codebase/CONCERNS.md` — architecture, test patterns, known hotspot concerns.
- `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`, `src/detail/writer_publish.hpp/.cpp` — implementation facts.
- `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/parser_preparer_seam_policy_tests.cpp`, `tests/CMakeLists.txt` — existing coverage and policy-test style.
- Local shell probes: `cmake --version`, `ctest --version`, `vcpkg version`, `cl` availability.

### Secondary (MEDIUM confidence)

- `README.md`, `docs/target-format-guide.md`, `docs/integration-examples.md`, `docs/thread-safety.md` — verification lane docs and likely documentation surfaces for lifecycle updates.
- `.claude/skills/openspec-*.md` — project skill discovery showed OpenSpec workflow skills; no Phase 17 plan requirement depends on OpenSpec artifacts. [VERIFIED: `.claude/skills/*/SKILL.md`]

### Tertiary (LOW confidence)

- Assumed implementation heuristics in the Assumptions Log, especially exact shape of new digest/key helpers and policy-test warning signs.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — existing repo manifests, presets, tests, and shell probes confirm the tools and no new dependencies are needed.
- Architecture: HIGH — implementation files directly show the dedupe and snapshot ownership boundaries.
- Pitfalls: MEDIUM-HIGH — major pitfalls are locked by SPEC/context and code; some policy-test heuristics are assumed and flagged.

**Research date:** 2026-05-14
**Valid until:** 2026-06-13 for repository-local architecture; re-check tool versions and current source before execution.
