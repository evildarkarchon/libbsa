---
last_mapped: 2026-05-15
last_mapped_commit: ead68b6856b37c3ea3acbf007180951c3cc22698
---

# Codebase Concerns

**Analysis Date:** 2026-05-15

## Tech Debt

**Large-format logic concentrated in single parser/preparer files:**
- Issue: Several format modules carry parsing, validation, metadata materialization, and compatibility decisions in large translation units. `src/formats/ba2/ba2_dx10_parser.cpp` is the highest-risk example: it owns header parsing, record parsing, filename-table parsing, DX10 extension/hash validation, chunk ordering, public texture metadata construction, and file-backed parsing. `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, and `src/formats/bsa/tes4_bsa_prepare.cpp` follow the same broad pattern.
- Files: `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_prepare.cpp`
- Impact: Small archive-format fixes can accidentally change unrelated behavior such as hash validation, filename normalization, metadata allocation policy, or extraction ordering. Future Starfield/BA2 edge cases are likely to touch the same files repeatedly.
- Fix approach: Split by responsibility when modifying these areas: keep fixed-header/record readers, filename-table readers, span validation, and public metadata materialization in separate helpers with focused tests in `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and related malformed fixture tests.

**Strict parser policy may reject real-world archives with tolerated quirks:**
- Issue: Parsers reject duplicate canonical paths, hash mismatches, invalid extension fields, record/payload overlap, nonstandard DX10 chunk header sizes, and unsorted TES3 hash records. This is safer for untrusted input, but some legacy toolchains may produce archives accepted by games or by BSArchPro with warnings rather than hard failure.
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `docs/compatibility-evidence.md`
- Impact: Compatibility gaps may appear as `format_error` during `archive_reader::open()` even when a downstream tool expects best-effort listing or extraction.
- Fix approach: Keep default strict parsing for security, but add opt-in compatibility evidence before relaxing behavior. Any lenient mode should preserve stable hard failures for unsafe payload spans and should be covered by generated malformed fixtures under `tests/fixtures/generated/archives` plus optional local corpus checks in `tests/unit/local_game_fixture_tests.cpp`.

**Validation warning coverage is intentionally narrow:**
- Issue: `validate_archive()` emits only three public compatibility warning codes: compressed sound payloads, embedded-name risk, and target-family mismatch. Many known format risks remain parser failures or are not represented as nonfatal warnings.
- Files: `include/libbsa/validation.hpp`, `src/validation.cpp`, `docs/compatibility-evidence.md`, `tests/unit/compatibility_warning_tests.cpp`
- Impact: Consumers using validation as a preflight may miss compatibility advisories for future edge cases, and adding warning codes changes public API surface.
- Fix approach: Add each new warning as a stable enum value in `include/libbsa/validation.hpp`, document mandatory evidence in `docs/compatibility-evidence.md`, and extend policy tests in `tests/unit/validation_policy_tests.cpp` so warning taxonomy stays auditable.

**Metadata count limits are internal and high:**
- Issue: Parser safety limits are fixed at 1,000,000 entries/chunks and 65,536 BSA folders. These caps avoid unbounded archive-controlled allocation, but they are not caller-configurable and still allow large metadata reservations.
- Files: `src/detail/parser_primitives.hpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`
- Impact: Hostile but structurally bounded archives can still force substantial memory/time during metadata reserve, duplicate detection, sorting, and public `entries()` copying. Very large legitimate archives above these limits are rejected with `format_error`.
- Fix approach: Keep limits centralized in `src/detail/parser_primitives.hpp`. If real fixtures require larger limits, add a documented reader/validation policy rather than scattering literal overrides.

**OpenSpec skills exist but are process-only:**
- Issue: Project skills under `.claude/skills/` define OpenSpec workflows and do not encode libbsa-specific architecture, parser, or C++ rules beyond the general process.
- Files: `.claude/skills/openspec-apply-change/SKILL.md`, `.claude/skills/openspec-explore/SKILL.md`, `.claude/skills/openspec-verify-change/SKILL.md`
- Impact: Future implementation agents must rely on `AGENTS.md`, `.planning/codebase/*.md`, `docs/*.md`, and tests for libbsa-specific constraints; skills do not prevent accidental edits under `TES5Edit/` or public API leakage.
- Fix approach: Keep project-specific rules in `AGENTS.md` and codebase maps. If a reusable libbsa skill is added, include the TES5Edit read-only boundary, C++20 public API rule, Windows-only scope, and fixture policy.

## Known Bugs

**No confirmed functional bugs detected in current scan:**
- Symptoms: Not detected from static inspection. Existing tests cover generated success fixtures, malformed archives, round trips, validation, export surface, package consumer smoke, and MSVC ASan CI.
- Files: `tests/CMakeLists.txt`, `.github/workflows/ci.yml`, `tests/unit/validation_api_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`
- Trigger: Not applicable.
- Workaround: Not applicable.

**Potential BA2 GNRL overlap arithmetic fragility:**
- Symptoms: The helper `spans_overlap_u64()` adds `start + length` without explicit overflow checks. Current callers validate record payload spans with `span_fits_u64()` before overlap checks, which bounds the specific inputs by archive size, but the helper itself is unsafe if reused on unchecked spans.
- Files: `src/formats/ba2/ba2_gnrl_parser.cpp`
- Trigger: Future reuse of `spans_overlap_u64()` before a `span_fits_u64()` gate.
- Workaround: Keep every call preceded by span validation, or change the helper to use checked/end-saturating arithmetic before any new use.

## Security Considerations

**CI downloads CMake without checksum verification:**
- Risk: The Windows CI lane installs CMake by downloading a release ZIP from GitHub and expanding it without verifying a checksum or signature.
- Files: `.github/workflows/ci.yml`
- Current mitigation: The workflow pins `LIBBSA_CMAKE_VERSION: 4.3.2` and verifies the reported version string after install.
- Recommendations: Add a pinned SHA256 check for `cmake-4.3.2-windows-x86_64.zip` before `Expand-Archive`, or use a trusted setup action pinned to an immutable commit.

**CI dependencies use moving GitHub-hosted infrastructure:**
- Risk: `runs-on: windows-latest` and `actions/checkout@v6` are convenient but can change runner images or action implementation under the same workflow definition.
- Files: `.github/workflows/ci.yml`
- Current mitigation: CMake, vcpkg registry baseline, CMake presets, and project dependencies are pinned separately in `.github/workflows/ci.yml`, `vcpkg-configuration.json`, and `CMakePresets.json`.
- Recommendations: Pin actions by full commit SHA for release hardening. Keep `windows-latest` only if the goal is intentionally tracking current hosted MSVC images; otherwise pin a specific Windows runner image when available.

**Host-path publication and extraction touch caller-controlled filesystem paths:**
- Risk: Writer output publication and file-backed readers operate on caller-provided host paths. Bugs here can become overwrite, reparse-point, or path-encoding vulnerabilities.
- Files: `src/detail/host_file_path.cpp`, `src/detail/host_file.cpp`, `src/detail/writer_publish.cpp`, `src/detail/atomic_file_ops.hpp`, `src/archive.cpp`
- Current mitigation: Public host paths decode from UTF-8 to wide Windows paths in `src/detail/host_file_path.cpp`; writers reject replacement of detectable reparse-point outputs and non-regular files in `src/detail/writer_publish.cpp`; archive-internal paths reject absolute, drive-rooted, empty, `.` and `..` segments in `src/detail/archive_path.cpp`.
- Recommendations: Keep new host-path entry points routed through `detail::resolve_host_file_path()`. Keep archive virtual paths as normalized strings and never convert entry paths to host filesystem paths without a dedicated extraction-sink policy.

**Local game fixture policy prevents accidental copyrighted or reference-data commits:**
- Risk: Compatibility work needs real game archives and BSArchPro-derived expectations, which can leak copyrighted bytes or mutate the `TES5Edit/` reference submodule.
- Files: `tests/fixtures/README.md`, `tests/unit/local_game_fixture_tests.cpp`, `.github/workflows/ci.yml`, `AGENTS.md`
- Current mitigation: Local archives are ignored under `tests/fixtures/local`, optional corpus tests use `LIBBSA_GAME_FIXTURES`/`LIBBSA_BSARCHPRO_EXPECTED`, CI checks `git status --short TES5Edit`, and `TES5Edit/` is documented as read-only.
- Recommendations: Keep any compatibility corpus outside git or under ignored local paths. Never use `TES5Edit/` as a fixture workspace.

## Performance Bottlenecks

**Convenience extraction materializes full decoded payloads:**
- Problem: `archive_reader::extract_bytes()` allocates a vector for the entire raw payload size and then extracts into it.
- Files: `include/libbsa/archive.hpp`, `src/archive.cpp`, `src/detail/payload_stream.cpp`
- Cause: This API is intentionally a convenience wrapper around `payload_sink`, bounded only by platform/vector size checks.
- Improvement path: Use `archive_reader::extract()` with a streaming `payload_sink` for large files. If consumers need stronger safeguards, add an extract-bytes size option or document caller-side limits.

**Deflate and raw LZ4 block decompression are not fully streaming:**
- Problem: Compressed BA2 GNRL entries and Starfield raw LZ4 blocks are read into memory and decoded into a full output vector before chunked sink writes. LZ4 frame extraction has a streaming path, but deflate and raw LZ4 block extraction remain whole-payload paths.
- Files: `src/detail/compression_router.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_block_codec.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`
- Cause: `libdeflate` and raw `LZ4_decompress_safe` helpers are one-shot wrappers around archive chunk payloads.
- Improvement path: Keep chunk-level exact-size checks, but add bounded decode-to-sink strategies where feasible. For BA2 DX10, continue planning chunk boundaries so a single chunk, not the full texture archive, is the maximum materialized unit.

**Compressed writer paths often materialize entire source payloads:**
- Problem: Writers stream raw disk payloads in some paths, but compressed entries require reading source bytes into memory before compression. BA2 DX10 sources are fully loaded by DirectXTex, copied into `dds_source_analysis`, then written to snapshot temp files.
- Files: `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`, `src/texture/directxtex_analyzer.cpp`
- Cause: Whole-buffer compression libraries and DirectXTex metadata/image APIs operate on materialized source bytes.
- Improvement path: Keep raw disk streaming for uncompressed entries. If large compressed entries become a bottleneck, add archive-format-specific chunked compression plans and tests that assert bounded peak memory.

**`entries()` copies all public metadata on every call:**
- Problem: Reader `entries()` returns a new `std::vector<entry_metadata>` copy each time. BA2 DX10 entries can include nested `texture_metadata::chunks`, making copies more expensive for large texture archives.
- Files: `include/libbsa/archive.hpp`, `src/archive.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`
- Cause: Public API favors independent value results and does not expose internal spans or iterators.
- Improvement path: Keep value semantics for safety. Add paged listing, visitor-based listing, or immutable view APIs only with a clear lifetime contract in `docs/thread-safety.md`.

**Dedupe equality can re-read large disk payloads:**
- Problem: Payload deduplication hashes stored bytes and then performs equality checks on hash collisions. Some mixed disk/memory comparisons read an entire disk payload into memory.
- Files: `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`
- Cause: Exact byte equality is required before payload regions can be shared safely.
- Improvement path: Keep dedupe opt-in via writer options. For large archives with many duplicate-size entries, prefer streaming equality for both sides and avoid full RHS materialization.

## Fragile Areas

**BA2 DX10 texture reconstruction and chunk ordering:**
- Files: `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/ba2/ba2_dx10_chunk_assembler.cpp`, `src/texture/dds_layout.cpp`, `src/texture/directxtex_analyzer.cpp`
- Why fragile: Correctness depends on multiple coupled rules: `cube_maps_raw == 2049`, inferred array size from start-mip sequences, DXGI format byte width tables, DXT10 header reconstruction, per-chunk compression routing, and logical chunk ordering from `texture::validate_and_order_chunks()`.
- Safe modification: Add or update fixtures with both metadata and extracted DDS byte expectations. Keep parser chunk order distinct from archive raw order, and keep DirectXTex types behind `src/texture/directxtex_analyzer.*`.
- Test coverage: Strong generated fixture coverage exists in `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, and `tests/unit/dds_layout_tests.cpp`; real corpus coverage remains opt-in through `tests/unit/local_game_fixture_tests.cpp`.

**Compression route selection by archive version and Starfield method:**
- Files: `src/detail/compression_router.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`, `src/formats/ba2/ba2_format_detector.cpp`, `src/formats/bsa/bsa_format_detector.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_chunk_assembler.cpp`
- Why fragile: SSE BSA uses LZ4 frame APIs, Starfield BA2 v3 method `3` uses raw LZ4 blocks, and BA2 method `0` uses deflate. Mixing frame/block APIs corrupts or rejects payloads.
- Safe modification: Route only from detected archive family/version and explicit `CompressionMethod`; never infer codec from extension. Keep tests in `tests/unit/compression_router_tests.cpp` and `tests/unit/lz4_codec_tests.cpp` focused on exact-size behavior.
- Test coverage: Unit tests cover codec helpers and router behavior, plus reader/writer fixture tests for BA2 and BSA variants.

**Atomic writer publication and temporary data cleanup:**
- Files: `src/detail/writer_publish.cpp`, `src/detail/atomic_file_ops.hpp`, `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`, `tests/unit/writer_publish_tests.cpp`
- Why fragile: Writers create temporary directories beside outputs and BA2 DX10 snapshots under the system temp root. Reparse points, failed cleanup, and concurrent output paths are sensitive filesystem cases on Windows.
- Safe modification: Keep all output replacement checks in `src/detail/writer_publish.cpp`, and keep snapshot directory creation randomized with `BCryptGenRandom()` in `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`. Preserve tests that skip gracefully when the host cannot create symlinks.
- Test coverage: `tests/unit/writer_publish_tests.cpp` covers overwrite, non-overwrite, symlink/reparse behavior when supported, and race-style publish cases.

**Public API/result semantics:**
- Files: `include/libbsa/result.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `tests/unit/result_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`
- Why fragile: Public APIs use a local C++20 `result<T>` rather than exceptions or C++23 `std::expected`. Calling `value()` on an error or `error()` on success throws `std::logic_error`, which is intentional programmer-error behavior.
- Safe modification: Preserve C++20 public headers and avoid leaking `DirectXTex`, `libdeflate`, `lz4`, Windows handles, or private format structs. Public additions need doc comments and export-surface tests.
- Test coverage: Policy tests check public include boundaries, exported symbols, and package-consumer smoke.

**TES5Edit reference boundary:**
- Files: `TES5Edit/`, `AGENTS.md`, `.github/workflows/ci.yml`, `tests/fixtures/README.md`
- Why fragile: Format behavior depends on TES5Edit/BSArchPro reference code, but the submodule is read-only and must not be modified, compiled into libbsa, or used as fixture workspace.
- Safe modification: Read `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas` only for behavior tracing. Record non-obvious compatibility constraints in libbsa source comments and tests outside `TES5Edit/`.
- Test coverage: CI runs `git status --short TES5Edit` after builds/tests, and fixture policy tests assert the boundary text.

## Scaling Limits

**Archive metadata scale:**
- Current capacity: `metadata_entry_count_limit` is 1,000,000 entries, `metadata_dx10_chunk_count_limit` is 1,000,000 texture chunks, and `metadata_bsa_folder_count_limit` is 65,536 folders.
- Limit: Opens fail with `format_error` above these caps, and near-cap archives can consume large memory due to vectors, unordered sets, sorting, and public metadata copies.
- Scaling path: Add caller policy only after fixtures prove need. Keep default limits conservative for untrusted archives.

**Payload size limits:**
- Current capacity: Many stored/raw sizes serialize as `uint32_t` for BSA and BA2 records; host file spans are tracked with `uint64_t` internally. `validation_options::max_extractability_entry_bytes` defaults to 64 MiB when extractability validation is enabled.
- Limit: Convenience extraction and one-shot deflate/LZ4 block decode can allocate up to a full entry or chunk. Writer compressed payloads must fit record-specific UInt32 fields.
- Scaling path: Prefer streaming sink APIs for reads, keep large compressed writes bounded by explicit format limits, and add chunked compression/decompression before supporting very large compressed entries.

**Worker concurrency:**
- Current capacity: `detail::run_indexed_work()` rejects `worker_count == 0` and `worker_count > 1024`, and clamps actual worker count to task count.
- Limit: Too many requested workers become `invalid_argument`; per-task memory use still multiplies across active workers.
- Scaling path: Keep worker counts caller-explicit. Add benchmarks under `benchmarks/libbsa_benchmarks.cpp` before increasing defaults or automatic worker selection.

## Dependencies at Risk

**DirectXTex:**
- Risk: DirectXTex is required for DDS metadata/source analysis and is Windows-oriented. API or behavior changes can affect accepted texture formats, cubemap interpretation, and subresource ordering.
- Impact: BA2 DX10 writer and DDS metadata analysis can reject or reorder textures unexpectedly.
- Migration plan: Keep all DirectXTex calls isolated in `src/texture/directxtex_analyzer.cpp`; expose only libbsa-native `texture_metadata` in `include/libbsa/archive.hpp`.

**libdeflate and lz4:**
- Risk: Codec wrappers assume exact-size raw deflate, LZ4 frame, and raw LZ4 block behavior. Wrong wrapper selection or upstream behavior changes can cause format errors or corrupt archives.
- Impact: TES4/FO3/FNV BSA deflate, SSE LZ4 frame, FO4/Starfield BA2 deflate, and Starfield v3 raw LZ4 block support are affected.
- Migration plan: Keep wrappers in `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, and `src/detail/lz4_block_codec.cpp`. Keep frame and block tests separate in `tests/unit/lz4_codec_tests.cpp`.

**vcpkg registry baseline without per-package version declarations:**
- Risk: `vcpkg-configuration.json` pins a registry baseline, but `vcpkg.json` lists dependencies without explicit version constraints.
- Impact: Updating the baseline changes dependency versions for `libdeflate`, `lz4`, `directxtex`, `catch2`, and `nlohmann-json` together.
- Migration plan: Treat baseline updates as dependency changes with CI validation. Add `version>=` constraints or `overrides` only when a package version must be held independently of the registry baseline.

## Missing Critical Features

**Configurable leniency/compatibility mode:**
- Problem: Current readers are strict. There is no public option for best-effort listing/extraction of noncanonical but game-tolerated archives.
- Blocks: Tooling that needs to inspect damaged or legacy third-party archives may need to catch `format_error` and fall back to another reader.

**Fully streaming compressed payload decode for all codecs:**
- Problem: Deflate and raw LZ4 block decode materialize decoded payloads before sink writes.
- Blocks: Very large compressed entries or chunks have higher peak memory than the streaming sink API suggests.

**Broad compatibility warning catalog:**
- Problem: Validation warnings cover only three compatibility risks.
- Blocks: Consumers cannot distinguish many valid-but-risky archive conditions from ordinary successful validation.

## Test Coverage Gaps

**Default suite relies on generated fixtures more than real corpus:**
- What's not tested: Broad official game archives and BSArchPro-derived expectations are optional local checks, not default CI inputs.
- Files: `tests/unit/local_game_fixture_tests.cpp`, `tests/fixtures/README.md`, `docs/compatibility-evidence.md`
- Risk: Edge cases present in official archives may remain undiscovered by generated fixtures.
- Priority: High for format-compatibility milestones; keep optional corpus checks documented and never commit copyrighted data.

**Symlink/reparse publish tests are host-capability dependent:**
- What's not tested: Reparse-point publish rejection may be skipped on hosts that cannot create file symlinks.
- Files: `tests/unit/writer_publish_tests.cpp`
- Risk: CI may not exercise every reparse-point branch depending on runner privileges and filesystem behavior.
- Priority: Medium; keep branch coverage through helper-level tests where possible and preserve CI `windows-msvc-*` lanes.

**Performance and memory ceilings are policy-tested more than stress-tested:**
- What's not tested: Near-limit archives with hundreds of thousands of entries/chunks, very large compressed payloads, and high worker-count packing/extraction workloads.
- Files: `benchmarks/libbsa_benchmarks.cpp`, `tests/unit/bounded_memory_policy_tests.cpp`, `tests/unit/benchmark_policy_tests.cpp`, `src/detail/parser_primitives.hpp`
- Risk: Regressions in peak memory or runtime may pass unit tests.
- Priority: Medium; add benchmark report thresholds or stress fixtures only when they remain legal, deterministic, and practical for Windows CI.

---

*Concerns audit: 2026-05-15*
