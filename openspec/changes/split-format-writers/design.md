## Context

libbsa currently implements each archive writer family in a single dense translation unit:

| File | Lines | Public class + free functions |
|------|------:|-------------------------------|
| `src/formats/bsa/tes3_bsa_writer.cpp` | 380 | `tes3_bsa_writer`, `validate_entries`, `prepare_entries`, `assign_raw_offsets`, `write_archive_bytes`, `write_tes3_bsa_archive` |
| `src/formats/ba2/ba2_gnrl_writer.cpp` | 785 | `ba2_gnrl_writer`, `validate_entries`, `prepare_entry`/`prepare_entries`, `payloads_equal`, `compare_disk_payload_*`, `assign_payload_offsets`, `write_archive_bytes`, `write_ba2_gnrl_archive` |
| `src/formats/ba2/ba2_dx10_writer.cpp` | 766 | `ba2_dx10_writer`, snapshot helpers, `prepare_chunk`/`prepare_entry`/`prepare_entries`, `assign_payload_offsets`, `write_archive_bytes`, `write_ba2_dx10_archive` |
| `src/formats/bsa/tes4_bsa_writer.cpp` | 1030 | `tes4_bsa_writer`, `validate_entries`, `prepare_one_entry`/`prepare_folders`, `stored_payloads_equal`, `assign_offsets`, `write_archive_bytes`, `write_tes4_bsa_archive` |

Every file has the same five-stage shape:

1. Public writer class (`*_writer`) holding `state` + `add_file`/`add_bytes`/`write_to`.
2. Source preparation — read disk/memory bytes, route compression, build `prepared_entry`/`prepared_folder` structures (and DDS chunk planning for DX10).
3. Payload assignment + deduplication + offset planning — `payloads_equal`, `assign_*_offsets`, table sizing checks.
4. Archive serialization — header, table, name, and payload byte emission to a temporary file.
5. Orchestrator — `write_*_archive` free function that validates, calls preparation → assignment → serialization, and delegates final publication to `detail::publish_writer_output` from `writer-safe-publish`.

The shared `writer-safe-publish` and `shared-parser-primitives` capabilities already extracted the publish step and certain reader primitives respectively. The remaining four writer pre-publish stages stay fused inside each `*_writer.cpp`, and the helpers for those stages have similar names and shapes across files (e.g. `prepare_entries`, `assign_*_offsets`, `validate_entries`, `write_archive_bytes`). That makes drift between writer families easy to introduce when fixing compression routing, dedup equality, or offset math.

Constraints from `AGENTS.md` and the project stack:

- C++20, Windows-only, MSVC + vcpkg manifest mode.
- TES5Edit submodule is read-only and is not part of the libbsa target.
- Public API surface (`include/libbsa/`) must not change, and archive byte layout must remain identical.
- Behavior compatibility with BSArchPro is preserved; this change does not justify a deviation.
- Doc comments are required for new public-to-target internal headers (`/// ...`).

## Goals / Non-Goals

**Goals:**

- Make small, isolated edits to compression routing, deduplication, offset assignment, or serialization touch one stage TU per writer family instead of a single 380–1030-line file.
- Lock the per-family five-stage decomposition as an internal architectural contract via a new `archive-writer-layering` capability so future writer evolution can't quietly re-fuse the stages.
- Give each pre-publish stage a directly testable seam (preparation, assignment + dedup, serialization) without removing the existing orchestrator-level round-trip and compatibility tests.
- Preserve the existing `writer-safe-publish` integration: orchestrators continue to call `detail::publish_writer_output` exactly once, with the same diagnostic prefix and overwrite semantics.

**Non-Goals:**

- No public API changes to `include/libbsa/writer.hpp` or any other public header.
- No archive byte-layout changes, hash changes, deduplication-policy changes, or compression-policy changes. Bytes-out must be identical for every supported target on existing fixtures.
- No new external dependencies (no spdlog, fmt, boost, etc.). vcpkg manifest stays untouched.
- No reader / parser refactoring. This change only touches writer pre-publish stages.
- No multi-threaded redesign. The existing `detail::run_indexed_work` worker pool stays in source preparation as-is.
- No collapsing of family-specific helpers into cross-family generics where the helpers genuinely differ by archive layout (TES3 single-table vs TES4 folder/file two-level vs BA2 GNRL flat list vs BA2 DX10 chunked DDS). Cross-family helpers are limited to genuinely shared utilities already in `detail::`.

## Decisions

### D1. Per-family, per-stage translation units (not cross-family stage TUs)

For each writer family, split its `*_writer.cpp` into the following internal units (suffixes are illustrative — final names are chosen during implementation):

```
src/formats/bsa/
  tes3_bsa_writer.cpp           # public class + thin orchestrator (was 380 lines)
  tes3_bsa_prepare.{hpp,cpp}    # source preparation
  tes3_bsa_layout.{hpp,cpp}     # offset assignment (no dedup for TES3)
  tes3_bsa_serialize.{hpp,cpp}  # archive bytes -> temp file

  tes4_bsa_writer.cpp           # public class + thin orchestrator (was 1030 lines)
  tes4_bsa_prepare.{hpp,cpp}    # validate + prepare_one_entry/prepare_folders + compression routing
  tes4_bsa_layout.{hpp,cpp}     # stored_payloads_equal + assign_offsets (dedup)
  tes4_bsa_serialize.{hpp,cpp}  # write_archive_bytes + name/table/payload writing

src/formats/ba2/
  ba2_gnrl_writer.cpp           # public class + thin orchestrator (was 785 lines)
  ba2_gnrl_prepare.{hpp,cpp}
  ba2_gnrl_layout.{hpp,cpp}     # payloads_equal + compare_disk_payload* + assign_payload_offsets
  ba2_gnrl_serialize.{hpp,cpp}

  ba2_dx10_writer.cpp           # public class + thin orchestrator (was 766 lines)
  ba2_dx10_prepare.{hpp,cpp}    # DDS read + snapshot dir + prepare_chunk/prepare_entry/prepare_entries
  ba2_dx10_layout.{hpp,cpp}     # assign_payload_offsets
  ba2_dx10_serialize.{hpp,cpp}  # write_archive_bytes + chunk/name/payload writing
```

Rationale:

- Per-family is mandated by the data shape: `tes4_writer_entry`/`prepared_folder` is two-level, `ba2_gnrl_writer_entry` is flat, `ba2_dx10_writer_entry` is DDS-chunked, and `tes3_writer_entry` has no dedup at all. Trying to share a stage TU across families would force a generic seam (templates or virtuals) that the current writers do not need or want.
- Per-stage matches the existing `prepare_*`/`assign_*`/`write_archive_bytes` boundary already present in source. The split moves code along seams the writers already use, minimizing semantic drift risk.

**Alternatives considered:**

- *Single `writer_pipeline.cpp` per stage with archive-family virtuals.* Rejected — adds a runtime polymorphism layer with no current downstream consumer and obscures the per-format byte layout. The point of this refactor is to reduce coupling, not invert it through a shared interface.
- *Keep the orchestrator and serialization fused, only extract preparation and layout.* Rejected — `write_archive_bytes` is the largest single block in TES4 (≈90 lines) and BA2 GNRL/DX10 (≈80 lines each). Leaving it inside the orchestrator unit would leave the highest-risk file unsplit.
- *Move helpers into anonymous namespaces in the same file using `// MARK:` style sections.* Rejected — does not address the "edits to compression or dedup require touching a 1000-line file" risk.

### D2. New internal headers carry prepared-entry types and stage entry points

Each `*_prepare.hpp` exposes the family's `prepared_entry` / `prepared_folder` value types plus the stage entry-point function (e.g. `result<std::vector<prepared_folder>> tes4_prepare_folders(...)`). `*_layout.hpp` and `*_serialize.hpp` expose the next-stage entry points and consume those types. All three headers live under `src/formats/{bsa,ba2}/`, are flagged as private to the libbsa target (no install rule), and use the existing `libbsa::formats::bsa` / `libbsa::formats::ba2` namespaces. The existing public free function (e.g. `formats::bsa::write_tes4_bsa_archive`) stays in `tes4_bsa_writer.hpp` and remains the only cross-TU entry point that other parts of libbsa call.

Rationale:

- Keeps cross-TU coupling explicit and discoverable — readers can grep the stage entry point to find every caller.
- Lets each stage TU compile against a minimal header set (`*_prepare.hpp` does not need the serialization helpers, and vice versa) to keep build time stable.
- Carbon-copies the public-API discipline: headers in `src/formats/...` are internal-target-only; nothing leaks to consumers.

**Alternatives considered:**

- *One umbrella `tes4_bsa_internal.hpp` per family.* Rejected — recreates the cross-coupling we are removing by forcing every stage TU to include all three headers.
- *Put prepared-entry types directly in the writer header.* Rejected — would expose internal data shapes through an installable-looking path and complicate future representation changes.

### D3. The `*_writer.cpp` orchestrator becomes a thin compose-and-publish function

After the split, `write_tes4_bsa_archive` (and its peers) will only:

1. Validate entries (`tes*_validate_entries` from the prepare unit).
2. Call the preparation entry point.
3. Call the layout / dedup / offset entry point.
4. Call `detail::publish_writer_output(...)` and pass a lambda that calls the serialization entry point against the temp path.

No cross-cutting helper logic, name normalization, hashing, or table-sizing math survives in `*_writer.cpp`. The class methods (`add_file`, `add_bytes`, `write_to`) keep their current shape.

Rationale:

- Locks `*_writer.cpp` to a small, auditable size so that any change exceeding a thin compose-publish flow is an obvious review signal.
- Keeps the `writer-safe-publish` integration call site exactly once per family, matching the existing spec scenario "Writer delegates final publication".

**Alternatives considered:**

- *Move the orchestrator into a separate `*_pipeline.cpp`.* Rejected — `write_*_archive` is the public free function declared in the existing writer header; moving its definition file is unnecessary churn.

### D4. Genuinely shared utilities continue to live under `src/detail/` or `shared-parser-primitives`

Helpers that are already family-agnostic — checked arithmetic on `std::uint32_t`/`std::uint64_t`, `detail::compress_payload`, `detail::run_indexed_work`, `detail::normalize_archive_path`, `detail::hash_tes4`, `detail::publish_writer_output`, `detail::binary_writer` — stay where they are. Family-specific lookalikes (`add_fits_u64`, `checked_u32`, `checked_u16`, `checked_u8`, `is_ascii_extension_byte`, `extension_fourcc_for`, `header_size_for`, `version_for`) are duplicated across writer files today; this change does **not** opportunistically merge them.

Rationale:

- The opportunistic shared-helper merge is a scope expander that increases risk without addressing the change's stated goal. If duplication is later judged worth removing, that becomes a separate, targeted change with its own spec impact.
- Several lookalikes have subtly different domain ranges (e.g., `checked_u8` only exists in DX10) and merging them would force a new shared API surface where one is not yet justified.

### D5. Tests move with code, then add stage-level unit coverage

For each writer family, the implementation order per family is:

1. Add the new internal headers + empty TUs and update CMake source list.
2. Move source-preparation code (definitions and helpers it owns) into `*_prepare.{hpp,cpp}` and run all writer tests.
3. Move payload-assignment / dedup / offsets into `*_layout.{hpp,cpp}` and re-run tests.
4. Move serialization (`write_archive_bytes` and its helpers like `stream_disk_payload_to_output`, `write_string_terminated`, `append_u32_le`) into `*_serialize.{hpp,cpp}` and re-run tests.
5. Reduce `*_writer.cpp` to public class + thin orchestrator.
6. Add new unit tests that call the stage entry points directly with minimal inputs (one entry, two duplicate entries for dedup, malformed entry for validation) — these become the regression net for the now-isolated stages.

The existing round-trip / fixture / compatibility tests are the primary semantic guard during the move and stay in place untouched.

Rationale:

- Movement-without-rewrite first is the lowest-risk way to keep BSArchPro compatibility intact; behavior changes are explicitly out of scope per the proposal.
- Stage-level unit tests are inexpensive once the stage entry point is a free function with a clear input/output contract — no orchestration mocks required.

### D6. Family rollout order: TES3 → BA2 GNRL → TES4 → BA2 DX10

Order chosen by ascending complexity:

- TES3 has no dedup, no compression, single-table layout (~380 lines) — proves the TU pattern and CMake wiring with the smallest blast radius.
- BA2 GNRL adds compression routing + payload deduplication.
- TES4 adds two-level folder/file layout and embedded-name prefixes.
- BA2 DX10 adds DDS snapshotting and chunk planning, which has the most local helpers.

Rationale: each later family inherits the validated TU pattern and CMake structure, reducing the chance of structure churn while landing the larger files.

## Risks / Trade-offs

- [Risk: Helper-name collisions across new TUs (every family currently has `add_fits_u64`, `checked_u32`, `validate_entries`, `prepare_entries`, `assign_payload_offsets`, `write_archive_bytes`)] → Mitigation: each new TU lives under the existing family namespace (`libbsa::formats::bsa` / `libbsa::formats::ba2`) and the unmoved free functions stay in anonymous namespaces inside their stage TU. Stage entry points use prefixed names (`tes4_prepare_folders`, `tes4_assign_offsets`, `tes4_write_archive_bytes`) to make cross-TU symbols unambiguous in stack traces and grep results.
- [Risk: Behavior drift during the move — silent reordering of validation, compression, or dedup steps] → Mitigation: D5's "move-then-test, do not edit semantics" rule and family-by-family rollout (D6); existing round-trip + compatibility tests must pass at every intermediate commit.
- [Risk: Stage TUs accidentally depend on each other in a circular way through shared helper ownership] → Mitigation: only the data structures and stage entry-point functions are exposed in `*_prepare.hpp` / `*_layout.hpp` / `*_serialize.hpp`; helpers stay in anonymous namespaces inside the TU that owns them. Each header declares the minimum needed for the next stage.
- [Risk: Increased compile time from more TUs with overlapping include sets] → Mitigation: header sets stay minimal per stage (D2). Empirically the new TU count is small (12 new TUs across 4 families) and compile fan-out is bounded by family-internal includes.
- [Risk: Spec is structural rather than behavioral, making it hard to write meaningful scenarios] → Mitigation: requirements are expressed against the orchestrator's externally observable invariants — single publish call site, no source-prep / dedup / serialization logic in `*_writer.cpp`, identical archive bytes — which can be checked by build/static inspection and existing fixture tests.
- [Trade-off: Up-front churn (≈12 new files + 4 modified `*_writer.cpp`) for unchanged runtime behavior] → Accepted because the proposal motivation is recurring edit risk on dense files; reducing that risk needs structural change, not commentary or naming cleanup.
- [Trade-off: Some duplication remains across family helpers (D4)] → Accepted; merging shared helpers belongs to a follow-up change that can lean on this layered structure instead of introducing it during the move.

## Migration Plan

1. **Setup (no behavior change)**
   - Add new private internal headers and empty `.cpp` files for each family's three new stage TUs.
   - Register the new TUs in `CMakeLists.txt` alongside the existing writer source entries (lines 102, 105, 109, 112).
   - Confirm the build still passes with the new empty TUs in place.
2. **TES3 BSA rollout** — apply D5 steps 2–6. Land each step as its own commit; run the full writer test suite (`ctest`) at each step.
3. **BA2 GNRL rollout** — same pattern, with the dedup helpers moving into `ba2_gnrl_layout.{hpp,cpp}`.
4. **TES4 BSA rollout** — same pattern, including the two-level folder/file layout helpers and embedded-name prefix logic.
5. **BA2 DX10 rollout** — same pattern, with snapshot handling and chunk planning kept inside `ba2_dx10_prepare.{hpp,cpp}`.
6. **Final pass** — verify each `*_writer.cpp` is reduced to public class + thin orchestrator; verify each orchestrator has exactly one `detail::publish_writer_output(...)` call; add the stage-level unit tests called out in D5 step 6.

**Rollback strategy:** because each family's rollout is staged across multiple commits and the public surface is unchanged, any single step can be reverted in isolation without touching other families. If a stage move uncovers a hidden semantic dependency, the affected commit is reverted, the dependency is documented in a follow-up note, and the TU is re-extracted with the dependency surfaced explicitly in `*_prepare.hpp` / `*_layout.hpp` / `*_serialize.hpp`.

## Open Questions

- Should the stage entry-point names use a `tes3_*`/`tes4_*`/`ba2_gnrl_*`/`ba2_dx10_*` prefix or rely solely on the `libbsa::formats::bsa` / `libbsa::formats::ba2` namespace plus stage-level header location? Current default in this design is to prefix function names for readability in stack traces; revisit once the TES3 family lands if the prefix proves redundant.
- Whether the `archive-writer-layering` requirements should explicitly forbid future stages adding cross-family abstractions, or simply require the per-family per-stage decomposition. Current default is to require the decomposition only; cross-family abstraction is left open for future, separately-spec'd work.
