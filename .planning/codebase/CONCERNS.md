# Codebase Concerns

**Analysis Date:** 2026-05-12

## Tech Debt

**Monolithic format implementations:**
- Issue: Core parser and writer-preparation logic is concentrated in a few very large translation units, which makes behavior changes expensive and raises regression risk when one format rule changes.
- Files: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/archive.cpp`
- Impact: Review scope stays large, overflow/offset logic is harder to isolate, and small compatibility fixes require editing files that already own many responsibilities.
- Fix approach: Split planning, validation, offset math, and payload-routing helpers into smaller internal modules with narrow tests per helper.

**Repeated archive-family dispatch in the public reader path:**
- Issue: `archive_reader` repeats variant/type branching across open, listing, lookup, contains, extraction, and bulk extraction.
- Files: `src/archive.cpp`, `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`
- Impact: Adding or changing a format requires touching multiple public-reader branches, which increases drift risk between single-entry and bulk-entry behavior.
- Fix approach: Introduce an internal reader vtable/strategy object inside `archive_reader::state` so open-time dispatch happens once.

**Planning/build policy drift around sanitizer support:**
- Issue: planning artifacts say sanitizer-oriented presets were delivered, while the checked-in build and policy tests explicitly enforce their absence.
- Files: `.planning/PROJECT.md`, `CMakePresets.json`, `tests/unit/validation_policy_tests.cpp`, `tests/fixtures/README.md`
- Impact: Maintainers can make wrong assumptions about current hardening coverage and verification expectations.
- Fix approach: Reconcile `.planning/` claims with the actual supported preset set and CI policy.

## Known Bugs

**Non-ASCII Windows host paths are handled inconsistently:**
- Symptoms: Reading or validating archives can fail on Windows paths that require wide-character filesystem handling, while writer-side helpers already use `std::filesystem::path` in some codepaths.
- Files: `src/archive.cpp`, `src/validation.cpp`, `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`
- Trigger: Open or validate an archive from a host path that is not representable through the narrow-string `std::ifstream` constructors used in reader/parser paths.
- Workaround: Keep archive host paths ASCII-only, or route all host-path opens through `std::filesystem::path`-based helpers like `src/detail/writer_disk_source.cpp`.

## Security Considerations

**BA2 DX10 snapshot temp files can outlive the writer on abnormal termination:**
- Risk: DDS subresource snapshots are copied into a writer-owned temp directory under the system temp root; a crash or forced termination can leave decoded texture bytes behind.
- Files: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Current mitigation: Randomized temp-directory names and best-effort cleanup in `ba2_dx10_writer::state::~state()`.
- Recommendations: Prefer tighter lifecycle control for snapshots, document temp-data persistence as a caller-visible risk, and consider a mode that streams directly from analyzed DDS bytes when feasible.

**Malformed-input hardening is fixture-based, not sanitizer- or fuzz-gated in default automation:**
- Risk: Integer-overflow, bounds, or codec edge cases can survive normal CI if they only appear under sanitizer instrumentation or fuzz-style mutation.
- Files: `CMakePresets.json`, `.github/workflows/ci.yml`, `tests/unit/validation_policy_tests.cpp`, `tests/fixtures/README.md`
- Current mitigation: Large committed malformed-fixture coverage in `tests/unit/*` plus validation/reporting policy tests.
- Recommendations: Add an opt-in sanitizer preset back to the live build, or a separate hardening workflow, and track a public fuzz harness as a maintained tool.

## Performance Bottlenecks

**TES4 payload deduplication is quadratic with full byte comparisons:**
- Problem: Dedup scans every earlier stored payload candidate and may compare full payload bytes before assigning offsets.
- Files: `src/formats/bsa/tes4_bsa_layout.cpp`
- Cause: The dedupe path keeps a linear `std::vector` of prior payloads and calls `tes4_stored_payloads_equal` for each candidate.
- Improvement path: Add a hash-indexed first pass so only equal-size/equal-hash candidates require full byte comparison.

**BA2 GNRL deduplication can re-read large disk payloads repeatedly:**
- Problem: Dedup reopens and compares on-disk sources when hash/size buckets collide, which grows expensive on large archives.
- Files: `src/formats/ba2/ba2_gnrl_layout.cpp`
- Cause: Exact dedupe correctness is preserved by falling back to full disk-to-disk or disk-to-memory comparisons after the payload hash key match.
- Improvement path: Keep the correctness rule, but add stronger staged identity metadata or chunked cached digests to reduce repeated file scans.

**BA2 DX10 staging multiplies disk I/O and temporary storage:**
- Problem: Every DDS subresource is snapshotted to temp files before chunk planning and archive serialization.
- Files: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Cause: The writer snapshots analyzed DDS subresources to avoid keeping one long-lived full DDS byte vector in writer state.
- Improvement path: Add thresholds for in-memory staging, batch cleanup checkpoints, or a streaming chunk planner that avoids one temp file per subresource.

## Fragile Areas

**Real-corpus compatibility coverage is opt-in and usually skipped:**
- Files: `tests/unit/local_game_fixture_tests.cpp`, `tests/fixtures/README.md`, `.github/workflows/ci.yml`
- Why fragile: The strongest BSArchPro-derived comparisons depend on local environment variables and uncommitted corpora, so default CI does not continuously prove behavior against external real-world data.
- Safe modification: Keep changes behind committed fixture coverage first, then run the opt-in local corpus checks before shipping parser or writer compatibility changes.
- Test coverage: Default CI covers generated fixtures and policy tests, but not the local `requires-game-fixture` comparison path.

**Large parser/preparer files are easy to destabilize with small edits:**
- Files: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`
- Why fragile: Offset arithmetic, overflow checks, format compatibility rules, and exception-to-error translation are mixed together in the same units.
- Safe modification: Add or update a focused regression test in `tests/unit/` before editing these files, and prefer extracting one helper at a time.
- Test coverage: Strong unit coverage exists, but the implementation surface is still broad enough that unrelated codepaths sit in the same file.

## Scaling Limits

**Convenience extraction materializes entire payloads in memory:**
- Current capacity: `archive_reader::extract_bytes` allocates a full decoded payload vector for one entry at a time.
- Limit: Very large entries are bounded by process address space and `std::vector<std::byte>::max_size()`.
- Scaling path: Prefer streaming extraction through `payload_sink` for large content and keep `extract_bytes` for bounded convenience cases.
- Files: `src/archive.cpp`, `src/detail/payload_stream.cpp`, `include/libbsa/archive.hpp`

**Writer `add_bytes` paths copy all caller-provided data into staged vectors:**
- Current capacity: In-memory staging scales with the full sum of all added byte-backed entries.
- Limit: Large write jobs can balloon RAM usage before finalization starts.
- Scaling path: Prefer `add_file` for large payloads, or add a staged streaming/file-backed source abstraction for memory-backed inputs.
- Files: `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `include/libbsa/writer.hpp`

**Parallel work is intentionally capped and process-local:**
- Current capacity: Worker counts above `1024` are rejected.
- Limit: Throughput scaling is limited to a single-process `std::jthread` pool with a hard cap.
- Scaling path: Keep the cap for safety, but document expected throughput bands and only raise it with measured benchmark evidence.
- Files: `src/detail/parallel_work.cpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`

## Dependencies at Risk

**DirectXTex toolchain coupling:**
- Risk: BA2 DX10 read/write support depends on `Microsoft::DirectXTex`, which ties texture functionality to Windows SDK and vcpkg package health.
- Impact: If the dependency or runner image changes incompatibly, BA2 DX10 build and test paths fail even when non-texture formats are unchanged.
- Migration plan: Keep the adapter boundary in `src/texture/directxtex_analyzer.cpp` and `src/texture/dds_layout.cpp` narrow so version pinning or replacement stays localized.
- Files: `CMakeLists.txt`, `vcpkg.json`, `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.cpp`

## Missing Critical Features

**Always-on hardening lane for malformed/parser/codec coverage:**
- Problem: The repository has malformed fixtures and validation APIs, but the default supported configure/test profiles do not include a sanitizer or fuzzing lane.
- Blocks: Continuous detection of memory-safety regressions that only appear under specialized instrumentation.
- Files: `CMakePresets.json`, `.github/workflows/ci.yml`, `tests/fixtures/README.md`

## Test Coverage Gaps

**Default automation does not run BSArchPro-derived local compatibility compares:**
- What's not tested: External compare manifests and optional local game/archive corpus checks.
- Files: `tests/unit/local_game_fixture_tests.cpp`, `tests/fixtures/README.md`, `.github/workflows/ci.yml`
- Risk: Compatibility drift can remain invisible until a maintainer runs the opt-in path manually.
- Priority: High

**Release-mode behavior is not part of the checked-in preset/CI matrix:**
- What's not tested: Optimized build behavior, packaging, and timing-sensitive regressions under a Release preset.
- Files: `CMakePresets.json`, `.github/workflows/ci.yml`
- Risk: Debug-only passing coverage can miss optimization-sensitive bugs or performance regressions.
- Priority: Medium

---

*Concerns audit: 2026-05-12*
