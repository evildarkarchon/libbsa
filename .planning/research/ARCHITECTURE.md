# Architecture Research: v1.1 Hardening Integration

**Project:** libbsa  
**Milestone:** v1.1 Hardening  
**Researched:** 2026-05-12  
**Scope:** Integration work only; keep the existing public API and format-family layout intact.  
**Confidence:** HIGH for integration points and build order; MEDIUM for the exact BA2 DX10 temp-staging redesign until implemented against fixtures.

## Executive Recommendation

Treat v1.1 as a **targeted internal-boundary cleanup**, not an architecture rewrite. The existing shape is sound: public API in `include/libbsa/`, orchestration in `src/archive.cpp` / `src/validation.cpp`, and format families under `src/formats/bsa` and `src/formats/ba2`.

The right integration move is to add **three small internal seams**:

1. **One Windows host-path boundary** for all host-file open/read/size operations.
2. **One open-time reader backend** stored in `archive_reader::state` so dispatch happens once.
3. **One BA2 DX10 staging/session boundary** so temp-snapshot lifetime is scoped to `write_to`, not the writer object's lifetime.

Do **not** redesign the public API, do **not** invent a generic plugin/registry system, and do **not** split every parser/preparer file in the milestone. Extract only the hotspots already called out by the concerns audit.

---

## Current Integration Baseline

The current architecture already gives good anchors:

- Public API remains dependency-light and C++20-safe.
- `archive_reader::open` is the single entry point for read/list/extract state.
- Format families are already separated by directory.
- Writers already use staged prepare → layout → serialize → publish flow.

The hardening issues are integration problems at the seams:

- host-path I/O is duplicated and inconsistent
- `archive.cpp` repeats format branching after open
- large parser/preparer units mix file access, validation, and business rules
- BA2 DX10 temp snapshots live too long and clean up too late

---

## Recommended Integration Map

| Concern | Modify Existing | Introduce New | Why This Shape Fits v1.1 |
|---|---|---|---|
| Non-ASCII host-path support | `src/archive.cpp`, `src/validation.cpp`, all `*_parser.cpp` / `*_reader.cpp` host-file entry points, `src/detail/writer_disk_source.cpp` | `src/detail/host_path_io.hpp/.cpp` | Centralizes Windows path conversion once without changing public headers. |
| Reader-dispatch cleanup | `src/archive.cpp` | `src/detail/archive_reader_backend.hpp` or a private backend struct local to `src/archive.cpp` | Keeps open-time format selection but removes repeated runtime branching. |
| Parser/preparer extraction | `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp` | Focused helpers adjacent to those files | Shrinks fragile units without forcing a cross-format abstraction. |
| Temp-staging risk reduction | `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.hpp` | `src/formats/ba2/ba2_dx10_stage_session.hpp/.cpp` or `ba2_dx10_snapshot_store.hpp/.cpp` | Shortens temp-data lifetime and makes cleanup explicit inside the write pipeline. |

---

## 1. Non-ASCII Host-Path Support

### Architectural decision

Keep the **public API unchanged** (`std::string_view host_path`), but make that string pass through a single internal Windows-aware host-path layer before any file open or size inspection.

### New component

**Introduce:** `src/detail/host_path_io.hpp/.cpp`

Recommended responsibilities:

- convert public UTF-8 host-path text into a Windows `std::filesystem::path`
- open `std::ifstream` / `std::ofstream` using the filesystem path overloads
- provide shared helpers for:
  - `read_prefix`
  - `file_size`
  - `open_input`
  - `open_output`
  - optional `path_exists` / `is_regular_file`

### Existing files to modify

- `src/archive.cpp`
  - replace `read_detection_prefix` narrow open
  - replace `archive_file_size` narrow open
- `src/validation.cpp`
  - replace `host_path_can_be_opened`
- `src/formats/bsa/tes3_bsa_parser.cpp`
- `src/formats/bsa/tes4_bsa_parser.cpp`
- `src/formats/ba2/ba2_gnrl_parser.cpp`
- `src/formats/ba2/ba2_dx10_parser.cpp`
- `src/formats/bsa/tes3_bsa_reader.cpp`
- `src/formats/bsa/tes4_bsa_reader.cpp`
- `src/formats/ba2/ba2_gnrl_reader.cpp`
- `src/formats/ba2/ba2_dx10_reader.cpp`
- `src/detail/writer_disk_source.cpp`
  - rebase its open/inspect logic onto the same helper so writer and reader behavior match

### Integration rule

Do not let parser/reader files construct `std::ifstream{std::string{host_path}}` directly anymore. Host-path conversion should become an internal policy boundary, not a repeated local choice.

### Test impact

Add or extend:

- `tests/unit/archive_reader_tests.cpp`
- `tests/unit/validation_api_tests.cpp`
- `tests/unit/writer_disk_source_tests.cpp`
- one reader test per family that opens a fixture copied to a non-ASCII temp path

### Warning

Do **not** widen the public API to `std::filesystem::path` in v1.1. That is a public-surface decision, not a hardening fix.

---

## 2. Reader-Dispatch Cleanup

### Architectural decision

Dispatch once during `archive_reader::open`, then store backend operations in `archive_reader::state`.

The concerns audit is correct: `entries`, `find`, `contains`, `extract`, and `extract_entries` should not all re-check `archive_variant`, `archive_type`, and `is_ba2_dx10`.

### Recommended shape

Keep `archive_reader::state` private inside `src/archive.cpp`, but add a backend bundle:

```cpp
struct archive_reader_backend {
  result<std::vector<entry_metadata>> (*entries)(std::span<const entry_metadata>);
  result<std::optional<entry_metadata>> (*find)(std::span<const entry_metadata>, std::string_view);
  result<bool> (*contains)(std::span<const entry_metadata>, std::string_view);
  result<void> (*extract)(std::string_view host_path, const entry_metadata&, payload_sink&);
};
```

Then extend `archive_reader::state` with:

- `archive_reader_backend backend`
- keep `metadata`, `entries`, `host_path`
- remove `is_ba2_dx10` once backend selection fully replaces it

### Existing files to modify

- `src/archive.cpp` only for the dispatch refactor

Optional small follow-up if needed:

- reader headers stay unchanged
- format reader implementations stay unchanged

### Why this is the right boundary

- no public ABI change
- no format-registry framework
- no virtual inheritance required
- open-time branching remains explicit and easy to review
- later archive-family additions touch one open-time selection block instead of every public method

### Test impact

Primary coverage already exists in:

- `tests/unit/archive_reader_tests.cpp`
- `tests/unit/bulk_extraction_tests.cpp`
- family-specific reader tests

Add targeted tests for:

- `find` / `contains` / `extract` consistency across all families
- bulk extraction still matching single-entry extraction after backend refactor

### Warning

Do **not** build a generic cross-library plugin registry, RTTI-heavy hierarchy, or heap-owned polymorphic graph here. A tiny internal function-pointer or non-virtual strategy bundle is enough.

---

## 3. Parser / Preparer Extraction

### Architectural decision

Extract **file-I/O and staging helpers**, not the core format rules. The format rules should stay beside their owning parser/preparer.

### 3A. TES4 BSA parser extraction

`src/formats/bsa/tes4_bsa_parser.cpp` is too broad, but the safest v1.1 split is narrow.

**Modify:**

- `src/formats/bsa/tes4_bsa_parser.cpp`

**Introduce one or both of:**

- `src/formats/bsa/tes4_bsa_file_loader.hpp/.cpp`
- `src/formats/bsa/tes4_bsa_table_bounds.hpp/.cpp`

Recommended extraction targets:

- file-open + fixed-header + metadata-table read path now in `parse_tes4_bsa_archive_file`
- table-size / table-span / payload-prefix helper code that can be tested independently

Keep inside `tes4_bsa_parser.cpp`:

- header interpretation
- folder/file materialization rules
- public `entry_metadata` construction
- compatibility-critical invariant logic

### 3B. BA2 DX10 prepare extraction

`src/formats/ba2/ba2_dx10_prepare.cpp` currently mixes:

- temp directory management
- snapshot file writes
- DDS analysis
- chunk collection
- chunk compression
- prepared-entry assembly

That is the highest-value split in the milestone.

**Modify:**

- `src/formats/ba2/ba2_dx10_prepare.cpp`
- `src/formats/ba2/ba2_dx10_prepare.hpp`
- `src/formats/ba2/ba2_dx10_writer.cpp`

**Introduce:**

- `src/formats/ba2/ba2_dx10_snapshot_store.hpp/.cpp`
  - reserve/remove staging directory
  - write/read/delete snapshot files
  - own temp cleanup policy
- `src/formats/ba2/ba2_dx10_chunk_prepare.hpp/.cpp`
  - `collect_chunk_snapshots`
  - `append_snapshot_bytes`
  - `ba2_dx10_prepare_chunk`

Keep inside `ba2_dx10_prepare.cpp`:

- target validation
- entry validation
- top-level `prepare_entry` and `ba2_dx10_prepare_entries`
- high-level ordering/sorting logic

### Why this extraction is enough

It reduces file size and review scope while preserving current format ownership. It avoids the common failure mode of introducing a “generic preparer framework” that obscures BA2 DX10's DDS-specific behavior.

---

## 4. BA2 DX10 Temp-Staging Risk Reduction

### Architectural decision

Move BA2 DX10 snapshot ownership from **writer-object lifetime** to **write-session lifetime**.

Today `ba2_dx10_writer::state` owns `snapshot_dir`, creates it during `add_file`, and relies on best-effort destructor cleanup. That is the wrong lifetime for hardening.

### Recommended shape

**Introduce:** `ba2_dx10_stage_session.hpp/.cpp` or fold it into `ba2_dx10_snapshot_store.hpp/.cpp`

Recommended responsibilities:

- create temp staging root lazily inside `write_ba2_dx10_archive`
- own cleanup with explicit scope end in the write pipeline
- support eager per-entry cleanup after chunk bytes are prepared
- provide one place to document abnormal-termination temp persistence risk

### Existing files to modify

- `src/formats/ba2/ba2_dx10_writer.cpp`
  - remove long-lived `snapshot_dir` from writer state
  - `add_file` should stage logical entry intent, not temp-directory ownership
- `src/formats/ba2/ba2_dx10_prepare.hpp`
  - update entry/session types
- `src/formats/ba2/ba2_dx10_prepare.cpp`
  - consume a stage session/store during preparation

### Recommended data ownership shift

Current:

- `add_file` analyzes DDS and writes temp snapshot files immediately
- writer state stores snapshot paths

Recommended v1.1 direction:

- writer state stores normalized archive path + DDS source host path + validated metadata needed for later prep
- write-time session materializes snapshots only while preparing/writing
- snapshots are deleted as soon as their prepared chunk payloads are owned in memory

### Why this is worth doing

- materially reduces temp-data residency window
- makes cleanup deterministic in normal execution
- keeps public writer API unchanged
- aligns temp staging with the existing prepare → serialize → publish pipeline

### Test impact

Add or extend:

- `tests/unit/ba2_dx10_writer_tests.cpp`
- `tests/unit/writer_stage_tests.cpp`
- `tests/unit/writer_ownership_tests.cpp`

Specific assertions to add:

- temp staging does not exist before `write_to`
- successful `write_to` removes staging artifacts
- failed `write_to` removes staging artifacts on normal unwinding
- remaining abnormal-termination leakage is documented, not silently ignored

### Warning

Do **not** try to solve crash-proof cleanup perfectly in v1.1. You cannot guarantee cleanup after process termination. The goal is to shorten lifetime and centralize cleanup, not promise impossible durability semantics.

---

## New vs Modified Components

### New components to introduce

| Component | Purpose |
|---|---|
| `src/detail/host_path_io.hpp/.cpp` | Single Windows-aware host-path conversion and stream-opening boundary for readers, parsers, validation, and disk-source helpers. |
| `src/detail/archive_reader_backend.hpp` or private backend block in `src/archive.cpp` | Open-time-selected backend operations for `entries`, `find`, `contains`, and `extract`. |
| `src/formats/bsa/tes4_bsa_file_loader.hpp/.cpp` | Pull host-file metadata-table loading out of the large parser unit. |
| `src/formats/ba2/ba2_dx10_snapshot_store.hpp/.cpp` | Encapsulate BA2 DX10 temp directory and snapshot file lifecycle. |
| `src/formats/ba2/ba2_dx10_chunk_prepare.hpp/.cpp` | Isolate chunk assembly/compression helpers from top-level prepare flow. |

### Existing components to modify

| File / Area | Change |
|---|---|
| `src/archive.cpp` | Replace repeated type/variant branching with backend dispatch; replace direct narrow host-file opens. |
| `src/validation.cpp` | Route readability/open checks through host-path helper. |
| `src/detail/writer_disk_source.cpp` | Reuse shared host-path helper so writer-side path handling matches reader-side behavior. |
| `src/formats/bsa/tes3_bsa_parser.cpp` | Replace direct narrow host-file open. |
| `src/formats/bsa/tes4_bsa_parser.cpp` | Replace direct narrow host-file open and extract file-loading helper. |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | Replace direct narrow host-file open. |
| `src/formats/ba2/ba2_dx10_parser.cpp` | Replace direct narrow host-file open. |
| `src/formats/bsa/*_reader.cpp` | Replace direct narrow host-file open. |
| `src/formats/ba2/*_reader.cpp` | Replace direct narrow host-file open. |
| `src/formats/ba2/ba2_dx10_writer.cpp` | Remove writer-lifetime snapshot-dir ownership; hand staging to write-time session. |
| `src/formats/ba2/ba2_dx10_prepare.cpp/.hpp` | Split snapshot/chunk helpers and integrate stage-session boundary. |

---

## Suggested Build Order

| Order | Slice | Why First / Next |
|---|---|---|
| 1 | Add `detail/host_path_io` + unit tests | Lowest-risk change with broad correctness payoff; unblock all non-ASCII fixes. |
| 2 | Rewire `archive.cpp`, `validation.cpp`, parser file-open paths, reader file-open paths, and `writer_disk_source.cpp` | Converts the entire library to one host-path policy before refactoring other seams. |
| 3 | Add `archive_reader` backend dispatch inside `src/archive.cpp` | Pure internal cleanup after behavior is stabilized by path tests. |
| 4 | Extract `ba2_dx10_snapshot_store` and move snapshot lifetime into write-time session | Highest-risk hardening change; do it only after path and reader behavior are green. |
| 5 | Extract `ba2_dx10_chunk_prepare` helpers | Reduces prepare-file fragility once staging ownership is clear. |
| 6 | Extract narrow TES4 BSA file-loader helper | Finish by shrinking the remaining parser hotspot without reopening prior changes. |

---

## Test-Backed Refactor Plan

### Must-have test additions before or alongside code changes

1. **Non-ASCII host-path regression tests**
   - open + validate from Unicode temp path
   - at least one BSA family and one BA2 family fixture

2. **Reader-dispatch equivalence tests**
   - `find`, `contains`, single extract, and bulk extract all still agree after backend introduction

3. **BA2 DX10 staging lifecycle tests**
   - no eager staging at `add_file`
   - cleanup after success
   - cleanup after ordinary failure paths

4. **Focused helper tests**
   - `host_path_io`
   - DX10 snapshot store
   - DX10 chunk-prepare helper where practical

---

## Over-Refactoring Warnings

### Do not change these in v1.1

- public headers under `include/libbsa/`
- public result/error model
- directory structure by format family
- write-new publish model
- validation architecture that reuses `archive_reader::open`

### Avoid these traps

#### 1. Generic archive framework rewrite
Bad move. The problem is duplicated dispatch, not missing abstraction purity.

#### 2. Splitting every large file at once
Bad move. Extract one seam at a time or you will lose fixture confidence.

#### 3. Moving format rules into `src/detail/`
Bad move. Shared detail should own generic I/O and staging mechanics, not BA2 DX10 or TES4-specific semantics.

#### 4. Solving temp staging by keeping huge DDS byte vectors alive in writer state
Bad move. That swaps disk-risk for RAM-risk and defeats earlier ownership decisions.

#### 5. Mixing dedupe optimization into this refactor set
Bad move. Dedup hotspots are real, but they are separable from these integration seams. Keep them out unless a small helper extraction is unavoidable.

---

## Milestone Planning Implications

Recommended implementation order for roadmap/planning:

1. **Host-path boundary hardening**
2. **Reader backend dispatch cleanup**
3. **BA2 DX10 write-session staging redesign**
4. **BA2 DX10 preparer extraction**
5. **TES4 parser extraction**

That order minimizes regression risk because it fixes correctness first, then removes duplicated dispatch, then tackles the riskiest writer-lifecycle cleanup with tests already in place.

---

## Sources

- `.planning/PROJECT.md`
- `.planning/codebase/ARCHITECTURE.md`
- `.planning/codebase/CONCERNS.md`
- `src/archive.cpp`
- `src/validation.cpp`
- `src/detail/writer_disk_source.cpp`
- `src/formats/bsa/tes4_bsa_parser.cpp`
- `src/formats/ba2/ba2_dx10_prepare.cpp`
- `src/formats/ba2/ba2_dx10_prepare.hpp`
- `src/formats/ba2/ba2_dx10_writer.cpp`
