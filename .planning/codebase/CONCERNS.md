# Codebase Concerns

**Analysis Date:** 2026-05-10

## Tech Debt

**Allocation error translation is incomplete outside `detail::byte_vector`:**
- Issue: Some archive-controlled or user-controlled sizes still flow into direct `std::vector` construction, `reserve`, `insert`, or codec output allocation instead of the result-translating helpers in `src/detail/byte_vector.hpp`.
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`, `src/detail/binary_io.cpp`, `src/detail/byte_vector.hpp`
- Impact: Malformed sparse archives or very large caller inputs can still trigger `std::bad_alloc` / `std::length_error` outside the public `libbsa::result<T>` error contract even though parser arithmetic checks exist.
- Fix approach: Route all byte-vector allocation and append paths through `detail::make_byte_vector`, `detail::reserve_byte_vector`, and `detail::append_byte_vector`; add sparse-file tests where the declared archive size is large enough to pass span checks but allocation should return `error_code::format_error`.

**Safe publish logic is duplicated across writer families:**
- Issue: Temporary-directory reservation, output existence checks, overwrite handling, backup selection, rollback, and cleanup are repeated with small differences in each writer implementation.
- Files: `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/detail/atomic_file_ops.hpp`, `src/formats/ba2/ba2_publish.hpp`
- Impact: Future fixes can land in one writer and miss another. The BA2 DX10 no-overwrite path uses `std::filesystem::copy_file` in `src/formats/ba2/ba2_dx10_writer.cpp`, while BSA writers and BA2 GNRL use shared no-replace publish helpers.
- Fix approach: Extract one `detail::publish_completed_archive` helper that owns no-overwrite, overwrite, backup, rollback, and cleanup behavior for every writer; keep format-specific error prefixes as parameters.

**Large format writer files carry high change risk:**
- Issue: Writer preparation, compression routing, deduplication, offset assignment, serialization, and publish behavior live in single large translation units.
- Files: `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/bsa/tes3_bsa_writer.cpp`
- Impact: Small changes to compression, dedupe, or publish behavior require editing dense files with multiple cross-coupled responsibilities and similar helper names.
- Fix approach: Split only along existing responsibility boundaries: source preparation, payload assignment/deduplication, archive serialization, and publish. Move behavior with focused regression tests before changing semantics.

**Source-text policy tests are useful but brittle:**
- Issue: Several policy tests assert that specific source tokens appear or do not appear rather than exercising behavior directly.
- Files: `tests/unit/bounded_memory_policy_tests.cpp`, `tests/unit/benchmark_policy_tests.cpp`, `tests/unit/validation_policy_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`
- Impact: Refactors can fail tests without changing behavior, and behavior regressions can pass if the expected token remains in source text or documentation.
- Fix approach: Keep policy tests for boundary rules that are hard to exercise, but prefer executable behavior tests for memory bounds, publish behavior, sanitizer paths, warning coverage, and package integration.

**Windows shared-library exports are broad:**
- Issue: `CMakeLists.txt` uses `WINDOWS_EXPORT_ALL_SYMBOLS ON` instead of an explicit public export macro and hidden-by-default internal symbols.
- Files: `CMakeLists.txt`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`
- Impact: Dynamic-library consumers may accidentally link to unintended symbols, and the project has no stable ABI discipline despite shipping shared builds.
- Fix approach: Add an explicit `LIBBSA_API` export macro before committing to a binary ABI policy; keep internal symbols private and preserve C++20 source compatibility.

## Known Bugs

**Confirmed runtime bugs:**
- Symptoms: Not detected in the current default validation set. The milestone audit records no milestone-blocking tech debt, no requirement gaps, no integration gaps, and no E2E flow gaps.
- Files: `.planning/milestones/v1.0-MILESTONE-AUDIT.md`, `.planning/STATE.md`, `tests/CMakeLists.txt`, `.github/workflows/ci.yml`
- Trigger: Not applicable.
- Workaround: Continue treating the concern areas below as risk items rather than confirmed bugs until reproduced with failing tests.

## Security Considerations

**Malformed archive allocation should stay inside `result` errors:**
- Risk: Parser helpers that allocate direct vectors from archive-derived counts can still throw if a hostile archive declares large but span-valid metadata or name table sizes.
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/detail/byte_vector.hpp`
- Current mitigation: Count/offset arithmetic is checked before many allocations, and `src/detail/byte_vector.hpp` translates allocation failures where it is used.
- Recommendations: Make allocation translation mandatory in parser read helpers and add tests that exercise large sparse host files without requiring actual large committed fixtures.

**Archive virtual paths are not host output paths:**
- Risk: `entry_metadata::original_path` can preserve archive spelling and separators; future CLI or extraction-to-directory helpers could introduce path traversal if they join archive paths directly onto host paths.
- Files: `src/detail/archive_path.cpp`, `include/libbsa/archive.hpp`, `docs/integration-examples.md`, `tests/unit/archive_path_tests.cpp`
- Current mitigation: `detail::normalize_archive_path` rejects empty, rooted, drive-rooted, `.` / `..`, repeated separator, and embedded-NUL virtual paths; public examples keep archive virtual paths separate from host filesystem paths.
- Recommendations: Any future sample CLI or extract-to-directory helper must normalize, revalidate, and safely join paths under an output root with canonical containment checks.

**Local compatibility corpora must stay external:**
- Risk: Optional game archives and BSArchPro-derived expected data can contain copyrighted payloads or local machine paths.
- Files: `tests/unit/local_game_fixture_tests.cpp`, `tests/fixtures/README.md`, `docs/compatibility-evidence.md`, `.gitignore`
- Current mitigation: Local corpus checks require `LIBBSA_GAME_FIXTURES` or `LIBBSA_BSARCHPRO_EXPECTED`, are labeled `requires-game-fixture`, and skip in the default suite.
- Recommendations: Keep local corpora ignored and never generate expected data under `tests/fixtures/generated/`; use a private CI artifact or machine-local path for real-game compatibility runs.

**TES5Edit boundary is protected but still easy to violate manually:**
- Risk: `TES5Edit/` is a submodule and reference corpus; accidental edits or submodule pointer changes break the project boundary.
- Files: `AGENTS.md`, `.github/workflows/ci.yml`, `TES5Edit/`
- Current mitigation: CI checks `git status --short TES5Edit`, and `AGENTS.md` forbids edits, formatting, staging, compiling, or vendoring under `TES5Edit/`.
- Recommendations: Keep future scripts, fixture generators, and docs tools scoped away from `TES5Edit/`; do not use it as a writable fixture workspace.

## Performance Bottlenecks

**Compressed extraction buffers per entry or per texture chunk:**
- Problem: Raw extraction streams in 64 KiB chunks, but compressed extraction materializes the stored compressed payload and decoded output before writing to the sink.
- Files: `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`, `tests/unit/bulk_extraction_tests.cpp`
- Cause: `libdeflate`, LZ4 frame, and raw LZ4 block helpers expose exact-size one-shot vector APIs.
- Improvement path: Add streaming or bounded-window decompression APIs where library support permits it; otherwise document and expose caller-selectable extraction size limits outside `validate_archive`.

**Open/list metadata is fully materialized and copied:**
- Problem: `archive_reader::open` builds a full `std::vector<entry_metadata>`, and `archive_reader::entries()` returns a copy.
- Files: `src/archive.cpp`, `include/libbsa/archive.hpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`
- Cause: The public API returns value vectors and optional value copies for stable ownership.
- Improvement path: For large-archive workflows, add an iterator/view-style API or a callback listing API while preserving the current value-returning API for simple consumers.

**BA2 DX10 writer snapshots DDS inputs through DirectXTex and temp files:**
- Problem: `ba2_dx10_writer::add_file` loads DDS bytes, copies subresource bytes, and writes writer-owned snapshots to a temp directory before final archive output.
- Files: `src/texture/directxtex_analyzer.cpp`, `src/texture/directxtex_analyzer.hpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.hpp`, `include/libbsa/writer.hpp`
- Cause: The writer owns input bytes after `add_file` so callers can mutate or delete the source DDS before `write_to`.
- Improvement path: Keep the ownership guarantee, but consider a future disk-backed source policy that validates immutable file metadata and streams subresources from caller-approved stable sources.

**Benchmarks are correctness-checked but not performance-gated:**
- Problem: Benchmark output records timing and correctness, but default tests intentionally reject fixed speed thresholds.
- Files: `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`, `tests/unit/benchmark_policy_tests.cpp`, `CMakeLists.txt`
- Cause: Host-dependent timing makes fixed CI thresholds noisy.
- Improvement path: Store historical benchmark reports outside the default test gate and compare trends in a separate maintainer workflow.

## Fragile Areas

**Format parser offset math and record validation:**
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`
- Why fragile: Each parser combines Bethesda-specific table layouts, count-derived span math, hash/order rules, duplicate path checks, name-table parsing, and payload span validation.
- Safe modification: Add or update generated malformed fixtures in `tests/fixtures/generated/`, update `tests/fixtures/generated/compatibility_matrix.json`, and cover the affected parser through `tests/unit/*reader_tests.cpp` or `tests/unit/*parser_tests.cpp`.
- Test coverage: Strong generated malformed coverage exists, but real-world corpus coverage remains optional through `tests/unit/local_game_fixture_tests.cpp`.

**Starfield BA2 v2/v3 header fields and compression methods:**
- Files: `src/formats/ba2/ba2_format_detector.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `docs/target-format-guide.md`
- Why fragile: `Unknown1`, `Unknown2`, and `CompressionMethod` are preserved public metadata, while method `3` routes to raw LZ4 block and method `0` routes to deflate. Other values fail closed.
- Safe modification: Do not infer new Starfield behavior from extension or target alone; add fixture-backed policy and update target-format docs before accepting new method values.
- Test coverage: Generated FO4/Starfield fixtures exist in `tests/fixtures/generated/archives/`, but current real-corpus checks are optional.

**Deduplication and mutable disk-source handling:**
- Files: `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/bsa_writer_execution_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`
- Why fragile: Dedupe eligibility depends on final stored bytes, compression route, raw size, packed size, metadata compatibility, and whether disk files changed after preparation.
- Safe modification: Preserve the current opt-in dedupe defaults and add tests for source growth/truncation, serial-vs-parallel determinism, and read-back extraction before changing dedupe keys.
- Test coverage: Good focused coverage exists for writer outputs, but dedupe behavior is spread across writer-specific tests rather than a shared contract test suite.

**Validation warning catalog coupling:**
- Files: `include/libbsa/validation.hpp`, `src/validation.cpp`, `docs/compatibility-evidence.md`, `tests/unit/compatibility_warning_tests.cpp`, `tests/unit/validation_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`
- Why fragile: Every new public `compatibility_warning_code` must be represented in public docs, evidence catalog text, policy tests, validation behavior, and package-consumer examples when relevant.
- Safe modification: Add the enum value, add an executable warning test, update `docs/compatibility-evidence.md`, and keep diagnostics free of parser offsets or record indexes.
- Test coverage: Policy tests enforce catalog presence, but warning semantics still need behavior tests per warning.

**BA2 DX10 texture layout reconstruction:**
- Files: `src/texture/dds_layout.cpp`, `src/texture/dds_layout.hpp`, `src/texture/directxtex_analyzer.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `tests/unit/dds_layout_tests.cpp`, `tests/unit/ba2_dx10_*`
- Why fragile: DDS header reconstruction, mip range planning, cubemap/array inference, chunk ordering, and DXGI format allowlists must stay aligned across parser, reader, writer, and DirectXTex-backed tests.
- Safe modification: Treat every DXGI format, chunking, cubemap, or array change as a cross-module change; update generated DDS sources and validate extracted DDS metadata.
- Test coverage: Strong generated coverage exists for supported fixtures, but unsupported real-world DDS variants require new fixtures before behavior changes.

## Scaling Limits

**Worker count has a hard library limit:**
- Current capacity: `src/detail/parallel_work.cpp` accepts `1..1024` workers and rejects `0` or larger values.
- Limit: There is no automatic worker-count selection based on CPU count, archive size, or codec cost.
- Scaling path: Add an explicit `auto_worker_count` policy only if callers need it; keep `worker_count == 0` invalid to preserve current API semantics in `include/libbsa/archive.hpp` and `include/libbsa/writer.hpp`.

**Archive metadata index size scales with file count:**
- Current capacity: Parsers check count-derived arithmetic and materialize all public entries in memory.
- Limit: Archives with very large file counts require memory for record vectors, path strings, hash sets, texture chunks, and copied `entries()` results.
- Scaling path: Add streaming list APIs, internal string interning, or borrowed views while preserving the current value-returning API for compatibility.

**Compressed payload size is bounded by platform vector size and codec one-shot limits:**
- Current capacity: `src/detail/lz4_block_codec.cpp` rejects sizes over `int` limits for raw LZ4 block APIs; deflate and LZ4 frame allocate expected output vectors.
- Limit: Very large compressed payloads can fail or consume large memory even when raw extraction paths are chunked.
- Scaling path: Add format-specific chunking policies or streaming codec adapters; keep `validate_archive` size caps in `include/libbsa/validation.hpp` for validation workflows.

## Dependencies at Risk

**DirectXTex Windows dependency and version behavior:**
- Risk: DDS parsing and BA2 DX10 writer support depend on DirectXTex behavior and availability through vcpkg.
- Impact: Future DirectXTex metadata changes can affect `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.cpp`, and BA2 DX10 writer/parser tests.
- Migration plan: Keep DirectXTex behind `src/texture/directxtex_analyzer.*` for API cleanliness, but treat Windows as the only supported platform.

**vcpkg manifest dependency pinning needs maintenance:**
- Risk: `vcpkg.json` and `vcpkg-configuration.json` pin a baseline, but there is no separate dependency-update validation workflow.
- Impact: Dependency security fixes or API changes in `libdeflate`, `lz4`, `DirectXTex`, `Catch2`, or `nlohmann-json` can remain untested until a manual baseline update.
- Migration plan: Add a periodic dependency refresh branch/workflow that runs Windows static/shared tests, benchmark report generation, and package-consumer smoke tests.

**Binary ABI policy is deferred:**
- Risk: Public headers expose value types containing `std::string`, `std::vector`, `std::optional`, and pimpl-owning classes, but the project has no stable ABI contract.
- Impact: Source consumers are fine, but binary package consumers can be broken by standard-library, compiler, or layout changes.
- Migration plan: Treat `include/libbsa/` as source-compatible API for now; before binary distribution, define ABI policy, export macros, symbol visibility, compiler support matrix, and versioning rules.

## Missing Critical Features

**No active v2 milestone scope:**
- Problem: Optional sample CLI, public fuzzing harnesses, lenient corrupt-archive recovery, and stable binary ABI policy are deferred candidates rather than active work.
- Blocks: Consumers needing first-party CLI tooling, fuzz-target integration, best-effort recovery from damaged archives, or binary ABI guarantees must build their own layer.
- Files: `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/milestones/v1.0-REQUIREMENTS.md`

**No top-level README quickstart:**
- Problem: Consumer guidance exists in `docs/integration-examples.md`, `docs/target-format-guide.md`, `docs/thread-safety.md`, and Doxygen configuration, but `README.md` is not present at the repo root.
- Blocks: New consumers lack an obvious entrypoint for configure/build/test/install commands and supported archive families.
- Files: `docs/integration-examples.md`, `docs/target-format-guide.md`, `docs/api-mainpage.md`, `CMakePresets.json`, `README.md`

**No public fuzzing harness:**
- Problem: Malformed generated fixtures exist, but there is no committed libFuzzer/AFL-style public harness for parser entry points.
- Blocks: Continuous malformed-input discovery beyond the curated fixture matrix.
- Files: `tests/fixtures/generated/compatibility_matrix.json`, `tests/fixtures/README.md`, `CMakePresets.json`, `src/archive.cpp`

## Test Coverage Gaps

**Real game and BSArchPro-derived corpus checks are opt-in:**
- What's not tested: Default CI does not compare against a real local game corpus or BSArchPro-derived expected manifest.
- Files: `tests/unit/local_game_fixture_tests.cpp`, `docs/compatibility-evidence.md`, `tests/fixtures/README.md`, `.github/workflows/ci.yml`
- Risk: Generated fixtures can miss real-world archive quirks, especially in Starfield BA2 variants and uncommon DDS layouts.
- Priority: High for future compatibility hardening.

**Release lane is absent from CI:**
- What's not tested: `.github/workflows/ci.yml` runs Windows MSVC Debug static and shared presets only.
- Files: `.github/workflows/ci.yml`, `CMakePresets.json`, `CMakeLists.txt`
- Risk: Optimization-sensitive issues and package export differences are not continuously exercised.
- Priority: Medium.

**Benchmark report target is manual:**
- What's not tested: `libbsa_benchmark_report` is defined and policy-tested, but normal CI build/test does not generate or archive benchmark reports.
- Files: `CMakeLists.txt`, `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`, `tests/unit/benchmark_policy_tests.cpp`, `.github/workflows/ci.yml`
- Risk: Performance regressions or benchmark breakage can sit outside pull-request feedback.
- Priority: Medium.

**Allocation-failure paths need larger sparse-file regression tests:**
- What's not tested: The default malformed matrix covers many oversized arithmetic cases, but not every direct allocation path with a large sparse archive that passes span checks.
- Files: `tests/fixtures/generated/compatibility_matrix.json`, `tests/unit/tes4_bsa_reader_tests.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`
- Risk: `std::bad_alloc` can escape in code paths that should return `error_code::format_error`.
- Priority: High.

---

*Concerns audit: 2026-05-10*
