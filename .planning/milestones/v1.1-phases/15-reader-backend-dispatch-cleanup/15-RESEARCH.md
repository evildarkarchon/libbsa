# Phase 15: Reader Backend Dispatch Cleanup - Research

**Researched:** 2026-05-14
**Domain:** Internal C++20 reader-dispatch refactor for `archive_reader`. [VERIFIED: 15-SPEC.md]
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

Copied verbatim from `15-CONTEXT.md`. [VERIFIED: 15-CONTEXT.md]

### Locked Decisions
- **D-01:** The private reader backend seam should be a function table selected once at `archive_reader::open` time.
- **D-02:** The function-table type and related seam wiring should stay local to `src/archive.cpp`, not grow into a broader private detail API.
- **D-03:** Phase 15 should reuse only the current shared reader state shape (`metadata`, `entries`, and resolved `detail::host_file_path`) and should not add backend-specific payload state unless later phases explicitly need it.
- **D-04:** Open-time backend selection should use one small private helper to assemble the chosen function table and final reader state so the `open` body stays centralized but readable.
- **D-05:** The backend seam should own the family-sensitive lookup and entry-payload primitives, while shared convenience orchestration stays in the facade.
- **D-06:** `extract()` should reuse the same backend-selected lookup flow as `extract_bytes()` and `extract_entries()` instead of repeating archive-family lookup branching inline.
- **D-07:** `contains()` should become a shared wrapper over the backend-selected `find()` path rather than remaining a separate backend callback.
- **D-08:** Exact request-string coalescing and per-request result mirroring in `extract_entries()` stay in shared facade logic; they are public-surface orchestration, not backend-specific behavior.
- **D-09:** The open-time seam should use separate private backend identities for `ba2_gnrl` and `ba2_dx10`.
- **D-10:** Fallout 4 BA2 GNRL and Starfield BA2 GNRL v3 should stay under one shared `ba2_gnrl` backend identity; their codec and metadata differences remain internal to the GNRL reader/parser path.
- **D-11:** Backend identity should replace the current `is_ba2_dx10` flag in reader state instead of keeping both in parallel.
- **D-12:** Private backend naming should mirror the current repo vocabulary: `tes3_bsa`, `tes4_bsa`, `ba2_gnrl`, and `ba2_dx10`.
- **D-13:** Add one dedicated cross-family Phase 15 runtime regression suite rather than scattering the whole dispatch contract only across existing family suites.
- **D-14:** That dedicated suite should exercise `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries` at least once per representative family while reusing existing committed fixtures instead of recreating every deeper family-specific matrix.
- **D-15:** The source-policy guard over `src/archive.cpp` should be method-scoped: fail if the affected public reader methods regain archive-family branching, while still allowing one centralized open-time/backend seam elsewhere in the file.
- **D-16:** The source-policy guard should lock only the negative invariant and should not require one exact helper, callback-table, or backend identifier name.

### the agent's Discretion
None. The discussion locked the seam shape, ownership split, BA2 backend granularity, and proof style closely enough that downstream research and planning should not reopen them.

### Deferred Ideas
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DISP-01 | Consumer can list, look up, check, extract, and bulk extract entries through one open-time reader backend selection with unchanged behavior across supported archive families. [VERIFIED: REQUIREMENTS.md] | Reuse the existing `archive_reader::open` detection path, store one backend table in reader state, keep shared orchestration in `extract_bytes()` and `extract_entries()`, and cover TES3, TES4, FO4 BA2 GNRL, Starfield BA2 GNRL v3, and BA2 DX10 through public APIs. [VERIFIED: src/archive.cpp; VERIFIED: 15-CONTEXT.md; VERIFIED: tests/CMakeLists.txt] |
| DISP-02 | Maintainer can add or adjust reader-backend behavior without duplicating archive-family branching across each public reader operation. [VERIFIED: REQUIREMENTS.md] | Move family-sensitive list/find/payload callbacks behind one file-local function table, replace `is_ba2_dx10` with backend identity, and add a negative source-policy test that fails if `entries`, `find`, `contains`, `extract`, `extract_bytes`, or `extract_entries` regain repeated family branching. [VERIFIED: 15-CONTEXT.md; VERIFIED: src/archive.cpp] |
</phase_requirements>

## Summary

`archive_reader::open` already does the expensive once-per-open work: it resolves the host path, reads a detection prefix, detects archive family, parses metadata plus entries, and returns an opened reader. Later public reader operations still branch again on `metadata.variant`, `metadata.type`, and `is_ba2_dx10`, so the current duplication is localized to `src/archive.cpp` rather than spread across the format modules. [VERIFIED: src/archive.cpp; VERIFIED: 15-SPEC.md]

The codebase already has the right ingredients for a low-risk cleanup. Every reader family exports the same three public-reader primitives today: `entries(...)`, `find_..._entry(...)`, and `extract_..._payload(...)`; every current `contains_..._entry(...)` implementation is already just a wrapper over `find_..._entry(...)`; and bulk extraction already keeps exact request-string coalescing plus per-request result mirroring in the facade. That means Phase 15 can be implemented as seam rewiring, not a behavior redesign. [VERIFIED: src/formats/bsa/tes3_bsa_reader.hpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.hpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.hpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.hpp; VERIFIED: src/formats/bsa/tes3_bsa_reader.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp; VERIFIED: src/archive.cpp]

The planning focus should therefore be narrow and prescriptive: add one file-local backend function table in `src/archive.cpp`, select it once in `archive_reader::open`, store backend identity instead of `is_ba2_dx10`, keep `contains()` and `extract()` as shared wrappers over backend-selected primitives, and prove the result with one dedicated cross-family runtime suite plus one negative method-scoped source-policy guard. [VERIFIED: 15-CONTEXT.md; VERIFIED: 15-SPEC.md]

**Primary recommendation:** Implement a file-local open-time-selected backend table in `src/archive.cpp`, keep bulk orchestration in the facade, and add one cross-family runtime suite plus one negative source-policy suite to lock the seam. [VERIFIED: 15-CONTEXT.md]

## Project Constraints (from AGENTS.md)

- Work only in C++ and keep the library reusable and independent of UI/tooling concerns. [VERIFIED: AGENTS.md]
- Keep the project Windows-only; do not expand scope for Linux, macOS, POSIX, or general portability. [VERIFIED: AGENTS.md]
- Treat `TES5Edit/` as read-only reference code; never edit, format, stage, or compile it into libbsa. [VERIFIED: AGENTS.md]
- Preserve BSArchPro-compatible archive behavior unless there is a documented reason to diverge. [VERIFIED: AGENTS.md]
- Do not add speculative dependencies; prefer the standard library unless a real need is documented. [VERIFIED: AGENTS.md]
- Keep public headers minimal and avoid leaking implementation details; prefer `libbsa::result<T>` or explicit error-code style for public failures. [VERIFIED: AGENTS.md]
- Keep thread safety object-scoped; no global mutable state or singleton-based behavior. [VERIFIED: AGENTS.md]
- Add Doxygen-compliant comments to public APIs and substantially rewritten methods, and keep accurate existing comments. [VERIFIED: AGENTS.md; VERIFIED: C:\Users\evild\.config\kilo\AGENTS.md]
- Add focused fixture-based tests for parsing, extraction, and compatibility behavior; do not keep production shims only for test compatibility. [VERIFIED: AGENTS.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Archive detection and backend selection at `open()` | API / Backend | Database / Storage | `archive_reader::open` resolves the host path, reads detection bytes, detects format, parses entries, and is the only place Phase 15 should choose a backend. [VERIFIED: src/archive.cpp; VERIFIED: 15-CONTEXT.md] |
| Canonical entry listing and lookup | API / Backend | — | `entries()` and `find()` operate on cached metadata plus entries and should become backend-table callbacks over in-memory state rather than repeated public branching. [VERIFIED: src/archive.cpp; VERIFIED: src/formats/bsa/tes3_bsa_reader.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp] |
| Payload extraction for one entry | API / Backend | Database / Storage | The facade owns public errors and convenience wrappers, but actual payload reads reopen the resolved host archive path through format readers. [VERIFIED: src/archive.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp] |
| Bulk extraction exact-request coalescing and result mirroring | API / Backend | — | `extract_entries()` groups exact request strings, mirrors per-group results back to request order, and must remain shared facade behavior by D-08. [VERIFIED: src/archive.cpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp; VERIFIED: 15-CONTEXT.md] |
| Archive-byte reopen boundary | Database / Storage | API / Backend | Reader backends consume the stored `detail::host_file_path` and must not reintroduce raw host-path reopening. [VERIFIED: src/archive.cpp; VERIFIED: 13-CONTEXT.md; VERIFIED: tests/unit/host_file_writer_name_tests.cpp] |

## Standard Stack

### Core
| Library / Mechanism | Version / Policy | Purpose | Why Standard |
|---------------------|------------------|---------|--------------|
| C++20 | Project policy; `CMAKE_CXX_STANDARD` is `20`. [VERIFIED: CMakePresets.json; VERIFIED: CMakeLists.txt] | Implement the private seam without changing the public API. [VERIFIED: 15-SPEC.md] | The repo is already locked to C++20, and Phase 15 needs no newer language feature. [VERIFIED: AGENTS.md; VERIFIED: CMakePresets.json] |
| File-local reader backend function table | Locked Phase 15 design. [VERIFIED: 15-CONTEXT.md] | One open-time-selected dispatch seam for `entries`, `find`, and payload extraction. [VERIFIED: 15-CONTEXT.md] | This directly addresses the repeated public-facade branching called out in architecture and concerns docs. [VERIFIED: .planning/codebase/ARCHITECTURE.md; VERIFIED: .planning/codebase/CONCERNS.md] |
| Existing family reader helpers | Current repo surface in `src/formats/bsa/*_reader.*` and `src/formats/ba2/*_reader.*`. [VERIFIED: src/formats/bsa/tes3_bsa_reader.hpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.hpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.hpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.hpp] | Reuse the existing list/find/extract primitives behind the new seam. [VERIFIED: 15-CONTEXT.md] | The helper APIs are already near-uniform and do not require a new plugin or registry layer. [VERIFIED: 15-CONTEXT.md] |

### Supporting
| Library / Mechanism | Version / Policy | Purpose | When to Use |
|---------------------|------------------|---------|-------------|
| Catch2 | Catch2 3 via `Catch2::Catch2WithMain`. [VERIFIED: tests/CMakeLists.txt; CITED: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md?plain=1#L86#usage] | Runtime regression suite and source-policy suite. [VERIFIED: tests/CMakeLists.txt] | Use for the dedicated cross-family Phase 15 suite and the method-scoped negative policy guard. [VERIFIED: 15-CONTEXT.md] |
| CTest + `catch_discover_tests` | Current repo pattern. [VERIFIED: tests/CMakeLists.txt; CITED: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md?plain=1#L86#usage] | Auto-discover Catch2 test cases and preserve label-based filtering. [VERIFIED: tests/CMakeLists.txt] | Use for Phase 15 tests so they enter the supported Windows preset lanes automatically. [VERIFIED: CMakePresets.json; VERIFIED: tests/CMakeLists.txt] |
| Manifest-backed committed fixtures | Existing generated archives and JSON manifests under `tests/fixtures/generated/archives/`. [VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/fixtures/generated/archives] | Cross-family runtime proof without inventing new binary fixtures. [VERIFIED: 15-CONTEXT.md] | Use TES3 success, TES4 v103/v104/v105, FO4 BA2 GNRL, Starfield BA2 GNRL v3, and BA2 DX10 fixtures already in repo. [VERIFIED: tests/fixtures/generated/archives] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| File-local function table in `src/archive.cpp` | Generic plugin/registry framework | Explicitly out of scope and too broad for a semantics-preserving hardening phase. [VERIFIED: 15-CONTEXT.md; VERIFIED: 15-SPEC.md] |
| Backend identity replacing `is_ba2_dx10` | Keep `is_ba2_dx10` plus `metadata.type`/`metadata.variant` branches | Preserves the current duplication and violates D-11. [VERIFIED: src/archive.cpp; VERIFIED: 15-CONTEXT.md] |
| Shared facade orchestration for `contains()` and `extract_entries()` | Separate backend callbacks for every public operation | Adds needless callback surface and risks drift in error semantics and duplicate-request behavior. [VERIFIED: 15-CONTEXT.md; VERIFIED: src/archive.cpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp] |

**Installation:**
```bash
# No new packages are required for Phase 15.
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static --output-on-failure
```
Current Phase 15 work reuses the existing project stack and does not justify new dependencies. [VERIFIED: AGENTS.md; VERIFIED: CMakePresets.json; VERIFIED: tests/CMakeLists.txt]

**Version verification:** No dependency version change is required for this phase; the current repo stack is C++20 plus Catch2/CTest under the checked-in Windows presets. [VERIFIED: CMakePresets.json; VERIFIED: tests/CMakeLists.txt; VERIFIED: vcpkg.json]

## Architecture Patterns

### System Architecture Diagram

Dispatch should collapse into this flow. The open-time choice happens once; later public methods route through the selected backend seam. [VERIFIED: 15-CONTEXT.md; VERIFIED: src/archive.cpp]

```text
Caller
  |
  v
archive_reader::open(host_path)
  |
  +--> resolve_host_file_path
  +--> read_detection_prefix / inspect_host_file_size
  +--> detect BSA or BA2
  +--> parse metadata + sorted entries
  +--> select backend identity + function table once
  v
state { metadata, entries, host_path, backend }
  |
  +--> entries() --------------------------> backend.list(entries)
  +--> find(path) -------------------------> backend.find(entries, path)
  +--> contains(path) ---------------------> shared wrapper over find(path)
  +--> extract(path, sink) ----------------> shared find(path) -> backend.extract(host_path, entry, sink)
  +--> extract_bytes(path) ----------------> shared find(path) -> backend.extract(... vector sink ...)
  +--> extract_entries(requests, factory) -> shared coalescing/mirroring -> backend.extract(host_path, entry, sink)
```

### Recommended Project Structure
```text
src/
├── archive.cpp                 # file-local backend seam, open-time selection, shared facade wrappers
├── formats/bsa/               # tes3_bsa and tes4_bsa concrete reader helpers
├── formats/ba2/               # ba2_gnrl and ba2_dx10 concrete reader helpers
└── detail/                    # host_file_path, payload_stream, parallel work

tests/
├── unit/archive_reader_dispatch_tests.cpp         # dedicated Phase 15 cross-family runtime suite
└── unit/archive_reader_dispatch_policy_tests.cpp  # negative source-policy guard
```
This structure stays inside the current facade/detail/format layering and avoids growing a wider private API. [VERIFIED: .planning/codebase/ARCHITECTURE.md; VERIFIED: 15-CONTEXT.md]

### Pattern 1: Open-time backend selection with file-local callbacks
**What:** Build one backend table during `archive_reader::open` and store it in reader state so later public methods stop branching on family metadata. [VERIFIED: 15-CONTEXT.md; VERIFIED: 15-SPEC.md]
**When to use:** Always for this phase; the seam shape is locked and there is no discretion to reopen alternatives. [VERIFIED: 15-CONTEXT.md]
**Example:**
```cpp
// Source: Phase 15 design contract in 15-CONTEXT.md
struct reader_backend {
  result<std::vector<entry_metadata>> (*entries)(std::span<const entry_metadata>);
  result<std::optional<entry_metadata>> (*find)(std::span<const entry_metadata>, std::string_view);
  result<void> (*extract)(const detail::host_file_path&, const entry_metadata&, payload_sink&);
};
```
This is a design sketch for planning, not existing code. [ASSUMED]

### Pattern 2: Keep wrappers shared when behavior is already common
**What:** `contains()` should wrap `find()` and shared convenience APIs should keep ownership of orchestration that is not family-specific. [VERIFIED: 15-CONTEXT.md; VERIFIED: src/formats/bsa/tes3_bsa_reader.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp]
**When to use:** For `contains()`, `extract()`, `extract_bytes()`, and `extract_entries()` in this phase. [VERIFIED: 15-CONTEXT.md]
**Example:**
```cpp
// Source: src/formats/bsa/tes4_bsa_reader.cpp
result<bool> contains_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path) {
  auto found = find_tes4_bsa_entry(entries, path);
  if (!found) {
    return found.error();
  }
  return found.value().has_value();
}
```

### Pattern 3: Keep exact-request coalescing in the facade
**What:** `extract_entries()` groups caller-identical request strings before extraction and mirrors the per-group result back to each original request index. [VERIFIED: src/archive.cpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp]
**When to use:** Preserve this logic exactly where it is conceptually today: shared facade orchestration. [VERIFIED: 15-CONTEXT.md]
**Example:**
```cpp
// Source: src/archive.cpp
const auto [group, inserted] = group_by_path.emplace(request.path, groups.size());
if (inserted) {
  groups.push_back(bulk_request_group{request.path, std::vector<std::size_t>{index}});
  continue;
}
groups[group->second].result_indices.push_back(index);
```

### Anti-Patterns to Avoid
- **Per-method archive-family branching in the public facade:** This is the exact concern Phase 15 exists to remove. [VERIFIED: .planning/codebase/ARCHITECTURE.md; VERIFIED: .planning/codebase/CONCERNS.md; VERIFIED: src/archive.cpp]
- **Generic plugin or registry framework:** Explicitly out of scope and disproportionate to the problem. [VERIFIED: 15-CONTEXT.md; VERIFIED: 15-SPEC.md]
- **Backend-specific orchestration for duplicate exact request strings:** D-08 keeps this behavior shared in the facade. [VERIFIED: 15-CONTEXT.md]
- **Parallel `is_ba2_dx10` and backend identity fields:** D-11 says replace the boolean rather than carry both. [VERIFIED: 15-CONTEXT.md; VERIFIED: src/archive.cpp]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Reader-family dispatch cleanup | A plugin/registration framework | A file-local function table in `src/archive.cpp` | The scope is one facade seam, not a new extensibility system. [VERIFIED: 15-CONTEXT.md; VERIFIED: 15-SPEC.md] |
| Shared `contains()` behavior | Four separate backend `contains` callbacks in the new seam | One shared facade wrapper over backend-selected `find()` | The concrete reader implementations already prove `contains == has_value(find)` today. [VERIFIED: src/formats/bsa/tes3_bsa_reader.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp; VERIFIED: 15-CONTEXT.md] |
| Bulk extraction duplicate handling | Backend-owned duplicate-request logic | Existing facade grouping and result mirroring | Current tests lock exact-string coalescing and sibling-failure isolation at the public surface. [VERIFIED: src/archive.cpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp] |
| Additional backend payload caches in reader state | New backend-specific mutable state | Existing `metadata`, `entries`, and `detail::host_file_path` only | D-03 explicitly forbids growing extra backend payload state in this phase. [VERIFIED: 15-CONTEXT.md] |

**Key insight:** The repo already has uniform concrete reader helpers; the missing piece is dispatch ownership, not new reader capability. [VERIFIED: src/formats/bsa/tes3_bsa_reader.hpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.hpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.hpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.hpp]

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — the library has no database or persistent service datastore; runtime archive access is filesystem-only. [VERIFIED: .planning/codebase/INTEGRATIONS.md] | None. [VERIFIED: .planning/codebase/INTEGRATIONS.md] |
| Live service config | None — no external services or UI-managed reader configuration were found. [VERIFIED: .planning/codebase/INTEGRATIONS.md] | None. [VERIFIED: .planning/codebase/INTEGRATIONS.md] |
| OS-registered state | None — Phase 15 touches library source/tests only and no OS registration surface is documented for reader dispatch. [VERIFIED: .planning/codebase/INTEGRATIONS.md; VERIFIED: 15-CONTEXT.md] | None. [VERIFIED: 15-CONTEXT.md] |
| Secrets/env vars | `VCPKG_ROOT` exists for builds, but Phase 15 does not rename or change any environment-variable contract. [VERIFIED: .planning/codebase/INTEGRATIONS.md; VERIFIED: environment audit 2026-05-14] | None for runtime behavior; keep existing build environment. [VERIFIED: environment audit 2026-05-14] |
| Build artifacts | No rename/migration-specific installed artifact or package-name change is in scope; normal rebuilds are sufficient after source edits. [VERIFIED: 15-CONTEXT.md; VERIFIED: CMakePresets.json] | Rebuild test targets after implementation; no data migration. [VERIFIED: CMakePresets.json] |

## Common Pitfalls

### Pitfall 1: Preserving lookup and extraction success while changing failure shape
**What goes wrong:** A refactor accidentally changes `invalid_argument`, `not_found`, or `io_error` behavior while collapsing branches. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: src/archive.cpp]
**Why it happens:** `extract()` currently does its own inline lookup branch, while `extract_bytes()` and `extract_entries()` follow slightly different shared paths. [VERIFIED: src/archive.cpp]
**How to avoid:** Make `extract()` call the same backend-selected `find()` flow as the other extraction APIs, then keep the existing shared not-found translation in the facade. [VERIFIED: 15-CONTEXT.md; VERIFIED: src/archive.cpp]
**Warning signs:** Tests that previously distinguished invalid path syntax from missing canonical paths start failing together. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp]

### Pitfall 2: Treating BA2 as one backend identity again
**What goes wrong:** The seam still needs special-case DX10 branching because BA2 GNRL and BA2 DX10 were collapsed too aggressively. [VERIFIED: 15-CONTEXT.md]
**Why it happens:** Current state uses `metadata.type == ba2` plus `is_ba2_dx10`, which makes BA2 look like one family with a side flag. [VERIFIED: src/archive.cpp]
**How to avoid:** Model `ba2_gnrl` and `ba2_dx10` as separate backend identities and keep Fallout 4 plus Starfield GNRL variants behind the shared GNRL backend. [VERIFIED: 15-CONTEXT.md]
**Warning signs:** New seam code still branches on `is_ba2_dx10` outside the open-time selector. [VERIFIED: 15-CONTEXT.md]

### Pitfall 3: Freezing one exact helper name in the policy test
**What goes wrong:** The source-policy test becomes brittle and blocks harmless helper renames instead of enforcing the actual invariant. [VERIFIED: 15-CONTEXT.md]
**Why it happens:** Repo-reading tests can easily overfit string matches. [VERIFIED: tests/unit/host_file_writer_name_tests.cpp; VERIFIED: tests/unit/validation_policy_tests.cpp]
**How to avoid:** Make the Phase 15 guard negative-only and method-scoped: assert that the affected public methods do not contain archive-family branching tokens, while allowing one centralized selector elsewhere in the file. [VERIFIED: 15-CONTEXT.md]
**Warning signs:** The policy test references one exact callback-table type or helper name instead of forbidden branching patterns. [VERIFIED: 15-CONTEXT.md]

### Pitfall 4: Moving duplicate exact-request semantics into the backend
**What goes wrong:** `extract_entries()` changes how duplicate request strings are coalesced or mirrored back to result order. [VERIFIED: src/archive.cpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp]
**Why it happens:** Bulk extraction mixes lookup, sink creation, extraction, and result projection in one method today. [VERIFIED: src/archive.cpp]
**How to avoid:** Keep exact-string grouping and `copy_group_result(...)` in the facade, and let the backend seam own only lookup/payload primitives. [VERIFIED: 15-CONTEXT.md; VERIFIED: src/archive.cpp]
**Warning signs:** `sink_factory.create(...)` gets called more than once for a duplicated exact request string. [VERIFIED: tests/unit/bulk_extraction_tests.cpp]

## Code Examples

Verified patterns from the current repo and official Catch2 docs:

### Shared `contains()` wrapper over `find()`
```cpp
// Source: src/formats/ba2/ba2_gnrl_reader.cpp
result<bool> contains_ba2_gnrl_entry(std::span<const entry_metadata> entries, std::string_view path) {
  auto found = find_ba2_gnrl_entry(entries, path);
  if (!found) {
    return found.error();
  }
  return found.value().has_value();
}
```

### Exact request-string coalescing in bulk extraction
```cpp
// Source: src/archive.cpp
const auto [group, inserted] = group_by_path.emplace(request.path, groups.size());
if (inserted) {
  groups.push_back(bulk_request_group{request.path, std::vector<std::size_t>{index}});
  continue;
}
groups[group->second].result_indices.push_back(index);
```

### Catch2 discovery integrated with CTest
```cmake
# Source: tests/CMakeLists.txt and Catch2 docs
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```
[VERIFIED: tests/CMakeLists.txt; CITED: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md?plain=1#L86#usage]

## State of the Art

| Old Approach | Current / Recommended Approach | When Changed | Impact |
|--------------|-------------------------------|--------------|--------|
| Repeated public-method branching on `metadata.variant`, `metadata.type`, and `is_ba2_dx10` | One open-time-selected backend seam reused by public reader methods | Planned for Phase 15. [VERIFIED: 15-SPEC.md; VERIFIED: 15-CONTEXT.md] | One edit point for future backend behavior and less drift between extraction paths. [VERIFIED: .planning/codebase/CONCERNS.md; VERIFIED: 15-CONTEXT.md] |
| BA2 dispatch expressed as `metadata.type == ba2` plus `is_ba2_dx10` boolean | Explicit backend identities `ba2_gnrl` and `ba2_dx10`; GNRL keeps FO4 and Starfield v3 together | Planned for Phase 15. [VERIFIED: 15-CONTEXT.md] | Removes parallel flag logic and matches the concrete reader split already in the repo. [VERIFIED: src/archive.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.hpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.hpp] |
| Scattered family-specific runtime proof | One dedicated cross-family runtime suite plus existing deep family suites | Planned for Phase 15. [VERIFIED: 15-CONTEXT.md] | Better proof that the facade contract stays stable while deeper family tests continue guarding format details. [VERIFIED: 15-CONTEXT.md; VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp] |

**Deprecated/outdated for this phase plan:**
- Public-facade repeated family branching in `entries`, `find`, `contains`, and `extract`. [VERIFIED: src/archive.cpp; VERIFIED: .planning/codebase/ARCHITECTURE.md]
- `is_ba2_dx10` as the long-term read-side backend discriminator. [VERIFIED: src/archive.cpp; VERIFIED: 15-CONTEXT.md]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The new backend table can be expressed with simple function-pointer-style callbacks similar to the planning sketch. | Architecture Patterns | Low — the planner may need to swap the exact storage type, but the locked seam shape and ownership split do not change. |

## Open Questions (RESOLVED)

1. **Which test file should own the negative source-policy guard?**
   - What we know: The repo already uses source-reading policy suites in `tests/unit/host_file_writer_name_tests.cpp` and `tests/unit/validation_policy_tests.cpp`. [VERIFIED: tests/unit/host_file_writer_name_tests.cpp; VERIFIED: tests/unit/validation_policy_tests.cpp]
   - Decision: Use a dedicated `tests/unit/archive_reader_dispatch_policy_tests.cpp` file for the Phase 15 negative guard instead of extending an unrelated policy suite. [RESOLVED: 2026-05-14]
   - Rationale: The dispatch invariant is phase-specific, mirrors the dedicated runtime suite strategy, and stays easier to find and edit when isolated from host-path and validation policy coverage. [VERIFIED: 15-CONTEXT.md; VERIFIED: 15-PATTERNS.md; VERIFIED: 15-01-PLAN.md]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Configure/build/test presets | ✓ [VERIFIED: environment audit 2026-05-14] | 4.3.2 [VERIFIED: environment audit 2026-05-14] | — |
| CTest | Running Catch2-discovered tests and package smoke | ✓ [VERIFIED: environment audit 2026-05-14] | 4.3.2 [VERIFIED: environment audit 2026-05-14] | — |
| Python | `validate_fixture_manifests` and fixture tooling | ✓ [VERIFIED: environment audit 2026-05-14] | 3.14.5 [VERIFIED: environment audit 2026-05-14] | — |
| `VCPKG_ROOT` | Preset toolchain resolution | ✓ [VERIFIED: environment audit 2026-05-14] | `C:\vcpkg` [VERIFIED: environment audit 2026-05-14] | — |
| MSVC `cl.exe` in current shell | Building the Windows presets locally | ✗ [VERIFIED: environment audit 2026-05-14] | — | Use a Visual Studio Developer PowerShell / Developer Command Prompt before configure/build. [ASSUMED] |
| Ninja | Optional alternate generator | ✗ [VERIFIED: environment audit 2026-05-14] | — | None needed; current presets do not require Ninja explicitly. [VERIFIED: CMakePresets.json] |

**Missing dependencies with no fallback:**
- None for planning-only work. [VERIFIED: current environment audit 2026-05-14]

**Missing dependencies with fallback:**
- MSVC compiler environment is not active in the current shell; run the existing presets from a VS developer shell. [VERIFIED: environment audit 2026-05-14; ASSUMED]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 3 via `Catch2::Catch2WithMain`. [VERIFIED: tests/CMakeLists.txt; CITED: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md?plain=1#L86#usage] |
| Config file | `tests/CMakeLists.txt`. [VERIFIED: tests/CMakeLists.txt] |
| Quick run command | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|bulk_extraction"` after adding the Phase 15 suite names. [ASSUMED] |
| Full suite command | `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: .planning/codebase/TESTING.md; VERIFIED: CMakePresets.json] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DISP-01 | Public `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries` still behave the same for TES3, TES4, FO4 BA2 GNRL, Starfield BA2 GNRL v3, and BA2 DX10 after one open-time backend selection. [VERIFIED: REQUIREMENTS.md; VERIFIED: 15-CONTEXT.md] | runtime fixture regression | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch"` [ASSUMED] | ❌ Wave 0 |
| DISP-02 | Public reader methods no longer duplicate archive-family branching; future drift fails a negative source-policy guard. [VERIFIED: REQUIREMENTS.md; VERIFIED: 15-CONTEXT.md] | source-policy | `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "archive_reader_dispatch_policy"` [ASSUMED] | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` after Wave 0 adds the new suite names. [ASSUMED]
- **Per wave merge:** `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: .planning/codebase/TESTING.md]
- **Phase gate:** Full suite green before `/gsd-verify-work`. [VERIFIED: .planning/config.json]

### Wave 0 Gaps
- [ ] `tests/unit/archive_reader_dispatch_tests.cpp` — dedicated cross-family runtime suite covering all Phase 15 operations once per representative backend. [VERIFIED: 15-CONTEXT.md]
- [ ] `tests/unit/archive_reader_dispatch_policy_tests.cpp` (or a clearly isolated equivalent) — method-scoped negative guard over `src/archive.cpp`. [VERIFIED: 15-CONTEXT.md]
- [ ] Add the new test file(s) to `tests/CMakeLists.txt` so Catch2 discovery includes them. [VERIFIED: tests/CMakeLists.txt]
- [ ] Standardize discoverable test names or tags for fast regex runs. [ASSUMED]

## Security Domain

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | Not applicable; the library has no identity subsystem. [VERIFIED: .planning/codebase/ARCHITECTURE.md] |
| V3 Session Management | no | Not applicable; no sessions or tokens exist. [VERIFIED: .planning/codebase/INTEGRATIONS.md] |
| V4 Access Control | no | Not applicable; local library APIs do not implement user/role access control. [VERIFIED: .planning/codebase/INTEGRATIONS.md] |
| V5 Input Validation | yes | Preserve parser invariants, archive-path normalization, and stable `result<T>` error-code paths. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: src/archive.cpp; VERIFIED: .planning/codebase/ARCHITECTURE.md] |
| V6 Cryptography | no | Not applicable to this phase; archive reader dispatch uses no crypto primitives. [VERIFIED: .planning/codebase/INTEGRATIONS.md] |

### Known Threat Patterns for this stack
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed archive input triggering wrong parser or payload path | Tampering | Keep detection in `open()`, preserve parser-specific validation, and do not bypass existing format readers. [VERIFIED: src/archive.cpp; VERIFIED: .planning/codebase/CONCERNS.md] |
| Path confusion between host paths and archive-internal paths | Tampering | Reuse Phase 13 `detail::host_file_path` for host I/O and `normalize_archive_path` for archive keys. [VERIFIED: 13-CONTEXT.md; VERIFIED: src/archive.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp] |
| Partial sink writes causing ambiguous extraction success | Tampering | Preserve current `io_error` handling for partial sink acceptance and keep tests that prove it. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp] |

## Sources

### Primary (HIGH confidence)
- `J:\libbsa\.planning\phases\15-reader-backend-dispatch-cleanup\15-CONTEXT.md` - locked seam shape, ownership split, BA2 backend identities, and proof requirements.
- `J:\libbsa\.planning\phases\15-reader-backend-dispatch-cleanup\15-SPEC.md` - phase goal, acceptance criteria, and in/out-of-scope boundaries.
- `J:\libbsa\.planning\REQUIREMENTS.md` - `DISP-01` and `DISP-02` requirement text.
- `J:\libbsa\src\archive.cpp` - current repeated public-facade dispatch, open path, bulk extraction orchestration, and `is_ba2_dx10` state.
- `J:\libbsa\src\formats\bsa\tes3_bsa_reader.hpp/.cpp` - current TES3 list/find/contains/extract helper shape.
- `J:\libbsa\src\formats\bsa\tes4_bsa_reader.hpp/.cpp` - current TES4 list/find/contains/extract helper shape.
- `J:\libbsa\src\formats\ba2\ba2_gnrl_reader.hpp/.cpp` - current BA2 GNRL list/find/contains/extract helper shape.
- `J:\libbsa\src\formats\ba2\ba2_dx10_reader.hpp/.cpp` - current BA2 DX10 list/find/contains/extract helper shape.
- `J:\libbsa\tests\unit\bulk_extraction_tests.cpp` - duplicate request coalescing, sibling failure isolation, and chunked extraction behavior.
- `J:\libbsa\tests\unit\host_file_writer_name_tests.cpp` and `tests\unit\validation_policy_tests.cpp` - existing repo-reading policy-test patterns.
- `J:\libbsa\tests\CMakeLists.txt` - Catch2 discovery, test registration, and fixture targets.
- `J:\libbsa\.planning\codebase\ARCHITECTURE.md`, `CONCERNS.md`, `INTEGRATIONS.md`, and `TESTING.md` - current architecture, concerns, integration surface, and test patterns.
- `J:\libbsa\AGENTS.md` and `C:\Users\evild\.config\kilo\AGENTS.md` - project constraints and comment/doc rules.

### Secondary (MEDIUM confidence)
- Catch2 CMake integration docs - `catch_discover_tests` usage pattern: https://github.com/catchorg/catch2/blob/devel/docs/cmake-integration.md?plain=1#L86#usage

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Phase 15 adds no new dependencies and the seam shape is locked by context plus current repo structure. [VERIFIED: 15-CONTEXT.md; VERIFIED: CMakePresets.json; VERIFIED: tests/CMakeLists.txt]
- Architecture: HIGH - The duplication is plainly localized in `src/archive.cpp`, and the concrete reader helper surfaces already align with the required seam. [VERIFIED: src/archive.cpp; VERIFIED: src/formats/bsa/tes3_bsa_reader.hpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.hpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.hpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.hpp]
- Pitfalls: HIGH - The most likely regressions are directly evidenced by current branching and existing bulk-extraction/public-error tests. [VERIFIED: src/archive.cpp; VERIFIED: tests/unit/bulk_extraction_tests.cpp; VERIFIED: include/libbsa/archive.hpp]

**Research date:** 2026-05-14
**Valid until:** 2026-06-13
