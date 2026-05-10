# Codebase Structure

**Analysis Date:** 2026-05-10

## Directory Layout

```text
libbsa/
+-- AGENTS.md                       # Project rules, TES5Edit boundary, GSD guidance
+-- CMakeLists.txt                  # Main CMake target, install/export, docs, benchmarks, tests
+-- CMakePresets.json               # Windows MSVC static/shared and Linux sanitizer presets
+-- vcpkg.json                      # vcpkg manifest dependencies
+-- vcpkg-configuration.json        # vcpkg registry baseline
+-- cmake/
|   +-- libbsaConfig.cmake.in        # Installed package config template
+-- include/
|   +-- libbsa/                      # Installed public C++20 headers
|       +-- archive.hpp              # Reader, metadata, extraction sink contracts
|       +-- writer.hpp               # Writer classes, targets, compression/write options
|       +-- validation.hpp           # Validation reports and compatibility warnings
|       +-- result.hpp               # C++20 result/error contract
|       +-- version.hpp              # Version declaration surface
|       +-- libbsa.hpp               # Umbrella include
+-- src/
|   +-- archive.cpp                  # Reader facade and format dispatch
|   +-- validation.cpp               # Public validation implementation
|   +-- libbsa.cpp                   # Version translation unit
|   +-- detail/                      # Internal reusable primitives
|   +-- formats/
|   |   +-- bsa/                     # TES3/TES4-family BSA detect/parse/read/write
|   |   +-- ba2/                     # BA2 GNRL/DX10 detect/parse/read/write
|   +-- texture/                     # DirectXTex adapter and DDS/DX10 layout helpers
+-- tests/
|   +-- CMakeLists.txt               # Catch2 target, fixture generators, package smoke, manifest checks
|   +-- unit/                        # Unit, fixture, policy, roundtrip, malformed tests
|   +-- fixtures/
|   |   +-- README.md                # Fixture legality/provenance policy
|   |   +-- generated/               # Committed legal generated archives, DDS sources, manifests, generators
|   |   +-- local/                   # Ignored local game/BSArchPro fixture workspace
|   +-- package-consumer/            # Installed package smoke project
+-- docs/                            # Public docs, target-format guide, Doxygen inputs
+-- benchmarks/                      # Synthetic benchmark runner and README
+-- .github/workflows/ci.yml         # Windows static/shared CI and TES5Edit cleanliness gate
+-- .planning/                       # GSD project state, archived v1 plans, codebase docs
+-- TES5Edit/                        # Read-only reference submodule
+-- build/                           # Local CMake build output, ignored
+-- .cocoindex_code/                 # Local code index data, ignored
```

## Directory Purposes

**`include/libbsa/`:**
- Purpose: Installed public API for consumers of `libbsa::libbsa`.
- Contains: Public headers only: `archive.hpp`, `writer.hpp`, `validation.hpp`, `result.hpp`, `version.hpp`, and `libbsa.hpp`.
- Key files: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`.

**`src/`:**
- Purpose: Private implementation for the library target.
- Contains: Public method implementations, format-specific code, reusable detail helpers, and texture/DDS adapters.
- Key files: `src/archive.cpp`, `src/validation.cpp`, `src/libbsa.cpp`.

**`src/detail/`:**
- Purpose: Internal format-neutral helpers.
- Contains: Archive path normalization, binary little-endian readers/writers, Bethesda hash functions, byte-vector allocation guards, compression router/codecs, payload streaming, worker scheduling, and atomic file publishing.
- Key files: `src/detail/archive_path.hpp`, `src/detail/binary_io.hpp`, `src/detail/bethesda_hash.hpp`, `src/detail/compression_router.hpp`, `src/detail/parallel_work.hpp`, `src/detail/atomic_file_ops.hpp`.

**`src/formats/bsa/`:**
- Purpose: BSA container implementation.
- Contains: BSA format detection, TES3 parser/reader/writer, and TES4-family parser/reader/writer.
- Key files: `src/formats/bsa/bsa_format_detector.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`.

**`src/formats/ba2/`:**
- Purpose: BA2 container implementation.
- Contains: BA2 format detection, BA2 GNRL parser/reader/writer, BA2 DX10 parser/reader/writer, and BA2 publish rollback helper.
- Key files: `src/formats/ba2/ba2_format_detector.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_publish.hpp`.

**`src/texture/`:**
- Purpose: Internal DDS and DirectXTex boundary for BA2 DX10 support.
- Contains: DirectXTex metadata/source analysis and pure libbsa DDS layout/chunk helpers.
- Key files: `src/texture/directxtex_analyzer.hpp`, `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.hpp`, `src/texture/dds_layout.cpp`.

**`tests/unit/`:**
- Purpose: Catch2 tests for public APIs, internal helpers, generated fixtures, malformed archives, policies, and round trips.
- Contains: One focused `*_tests.cpp` file per area.
- Key files: `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/validation_api_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.

**`tests/fixtures/generated/`:**
- Purpose: Committed legal generated data used by default tests.
- Contains: Fixture generator source files, generated archives, manifests, DDS source files, and Python manifest validator.
- Key files: `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, `tests/fixtures/generated/compatibility_matrix.json`, `tests/fixtures/generated/validate_fixture_manifests.py`.

**`tests/fixtures/local/`:**
- Purpose: Local-only game/BSArchPro fixture workspace.
- Contains: `.gitkeep` only in git; local archives/manifests stay uncommitted.
- Key files: `tests/fixtures/local/.gitkeep`, `tests/fixtures/README.md`.

**`tests/package-consumer/`:**
- Purpose: Installed package smoke test for consumer usage.
- Contains: A standalone CMake project and compile-checked examples using only public headers.
- Key files: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`.

**`docs/`:**
- Purpose: Public docs, Doxygen inputs, target-format policy, compatibility evidence, integration examples, thread-safety policy, and PRD.
- Contains: Markdown docs plus `Doxyfile.in`.
- Key files: `docs/thread-safety.md`, `docs/target-format-guide.md`, `docs/compatibility-evidence.md`, `docs/integration-examples.md`, `docs/api-mainpage.md`, `docs/Doxyfile.in`.

**`benchmarks/`:**
- Purpose: Synthetic benchmark tooling.
- Contains: Benchmark runner and README.
- Key files: `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`.

**`cmake/`:**
- Purpose: CMake package install/export support.
- Contains: Config template used by `configure_package_config_file`.
- Key files: `cmake/libbsaConfig.cmake.in`.

**`.github/workflows/`:**
- Purpose: CI.
- Contains: Windows MSVC static/shared build/test matrix and TES5Edit read-only cleanliness guard.
- Key files: `.github/workflows/ci.yml`.

**`.planning/`:**
- Purpose: GSD project state, current project docs, archived v1 milestone artifacts, quick task artifacts, research docs, and generated codebase map docs.
- Contains: `STATE.md`, `PROJECT.md`, milestone archives, research docs, and `.planning/codebase/`.
- Key files: `.planning/STATE.md`, `.planning/PROJECT.md`, `.planning/milestones/v1.0-ROADMAP.md`, `.planning/milestones/v1.0-REQUIREMENTS.md`, `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/STRUCTURE.md`.

**`TES5Edit/`:**
- Purpose: Read-only reference submodule for BSArchPro behavior.
- Contains: External submodule content from `https://github.com/TES5Edit/TES5Edit.git`.
- Key files: `.gitmodules`, `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, `TES5Edit/Core/wbBSA.pas`.

## Key File Locations

**Entry Points:**
- `include/libbsa/libbsa.hpp`: Umbrella public include for consumers.
- `include/libbsa/archive.hpp`: Public reader/extraction and metadata API.
- `include/libbsa/writer.hpp`: Public write-new API.
- `include/libbsa/validation.hpp`: Public validation API.
- `src/archive.cpp`: `archive_reader` implementation and format dispatch.
- `src/validation.cpp`: `validate_archive` implementation.
- `CMakeLists.txt`: Library, install/export, docs, benchmark, and test entry point.

**Configuration:**
- `CMakePresets.json`: Configure/build/test presets.
- `vcpkg.json`: Dependency manifest.
- `vcpkg-configuration.json`: vcpkg baseline config.
- `cmake/libbsaConfig.cmake.in`: Installed package config template.
- `.github/workflows/ci.yml`: CI matrix and read-only submodule check.
- `.clangd`: Local clangd compile command configuration.

**Core Logic:**
- `src/formats/bsa/bsa_format_detector.cpp`: BSA magic/version detection.
- `src/formats/bsa/tes3_bsa_parser.cpp`: TES3 BSA parsing and metadata materialization.
- `src/formats/bsa/tes4_bsa_parser.cpp`: TES4-family BSA parsing and metadata materialization.
- `src/formats/bsa/tes3_bsa_reader.cpp`: TES3 listing/lookup/raw extraction.
- `src/formats/bsa/tes4_bsa_reader.cpp`: TES4-family listing/lookup/raw/compressed extraction.
- `src/formats/bsa/tes3_bsa_writer.cpp`: TES3 write-new finalization.
- `src/formats/bsa/tes4_bsa_writer.cpp`: TES4-family write-new finalization.
- `src/formats/ba2/ba2_format_detector.cpp`: BA2 magic/version/subtype detection.
- `src/formats/ba2/ba2_gnrl_parser.cpp`: BA2 GNRL parsing and metadata materialization.
- `src/formats/ba2/ba2_dx10_parser.cpp`: BA2 DX10 parsing and texture chunk metadata materialization.
- `src/formats/ba2/ba2_gnrl_reader.cpp`: BA2 GNRL listing/lookup/raw/compressed extraction.
- `src/formats/ba2/ba2_dx10_reader.cpp`: BA2 DX10 reconstructed DDS extraction.
- `src/formats/ba2/ba2_gnrl_writer.cpp`: BA2 GNRL write-new finalization.
- `src/formats/ba2/ba2_dx10_writer.cpp`: BA2 DX10 DDS snapshot and write-new finalization.
- `src/texture/directxtex_analyzer.cpp`: DirectXTex adapter for DDS metadata/source analysis.
- `src/texture/dds_layout.cpp`: DDS header, mip-size, chunk-planning, and chunk-order logic.
- `src/detail/compression_router.cpp`: Format-selected compression/decompression dispatch.

**Testing:**
- `tests/CMakeLists.txt`: Registers `libbsa_tests`, fixture generators, `package_consumer_smoke`, and `validate_fixture_manifests`.
- `tests/unit/*_tests.cpp`: Unit and policy tests.
- `tests/fixtures/generated/archives/`: Committed generated `.bsa`, `.ba2`, and manifest fixtures.
- `tests/fixtures/generated/source/`: Committed generated DDS source fixtures.
- `tests/fixtures/generated/validate_fixture_manifests.py`: Fixture/matrix manifest validation.
- `tests/package-consumer/main.cpp`: Consumer API compile-smoke examples.

**Documentation:**
- `docs/thread-safety.md`: Canonical thread-safety contract.
- `docs/target-format-guide.md`: Supported format and compression route policy.
- `docs/compatibility-evidence.md`: Public compatibility warning evidence catalog.
- `docs/integration-examples.md`: Compile-checked consumer examples.
- `docs/api-mainpage.md`: Doxygen main page.

**Generated or Local-Only:**
- `build/`: Local build output; ignored by git.
- `.cocoindex_code/`: Local code index data; ignored by git.
- `tests/fixtures/local/`: Local fixture workspace; only `.gitkeep` is tracked.

## Naming Conventions

**Files:**
- Public headers use concise API names under `include/libbsa/`: `archive.hpp`, `writer.hpp`, `validation.hpp`, `result.hpp`, `version.hpp`, `libbsa.hpp`.
- Private C++ files use lower snake case with matching `.hpp` / `.cpp` pairs: `src/detail/binary_io.hpp` and `src/detail/binary_io.cpp`.
- Format files are prefixed by container/variant and operation: `tes3_bsa_parser.cpp`, `tes4_bsa_reader.cpp`, `ba2_gnrl_writer.cpp`, `ba2_dx10_parser.cpp`.
- Tests use `*_tests.cpp` names matching the unit or policy surface: `compression_router_tests.cpp`, `ba2_dx10_writer_tests.cpp`, `validation_api_tests.cpp`.
- Fixture generators use `generate_<format>_fixtures.cpp`: `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.
- Generated fixture manifests pair archive names with `_manifest.json`: `tests/fixtures/generated/archives/tes4_v105_manifest.json`.

**Directories:**
- Directories use lowercase names: `src/detail/`, `src/formats/`, `src/texture/`, `tests/unit/`, `tests/fixtures/`.
- Format-specific code nests by container family: `src/formats/bsa/` and `src/formats/ba2/`.
- Public headers always live below `include/libbsa/`; private headers stay below `src/`.
- GSD project artifacts live below `.planning/`; generated codebase docs live below `.planning/codebase/`.

**C++ Symbols:**
- Public types use `snake_case` names: `archive_reader`, `entry_metadata`, `tes4_bsa_writer`, `validation_report`.
- Public enum values use lower snake case: `archive_type::ba2`, `archive_variant::starfield`, `entry_compression::lz4_block`.
- Private helper functions use lower snake case: `normalize_archive_path`, `detect_ba2_format`, `parse_tes4_bsa_archive_file`, `write_ba2_gnrl_archive`.
- Namespaces separate public and private concerns: `libbsa`, `libbsa::detail`, `libbsa::formats::bsa`, `libbsa::formats::ba2`, `libbsa::texture`.

## Where to Add New Code

**New Public Reader Capability:**
- Primary code: Add public value types or methods in `include/libbsa/archive.hpp` only when the consumer contract changes; implement dispatch in `src/archive.cpp`.
- Format code: Add or extend parser/reader files under `src/formats/bsa/` or `src/formats/ba2/`.
- Tests: Add public behavior tests under `tests/unit/archive_reader_tests.cpp` or a format-specific test file such as `tests/unit/ba2_gnrl_reader_tests.cpp`.

**New Public Writer Capability:**
- Primary code: Add public target/options/methods in `include/libbsa/writer.hpp` only when consumers need them.
- Implementation: Add public method bodies in the relevant writer implementation file under `src/formats/bsa/` or `src/formats/ba2/`.
- Finalization: Keep byte layout, offset assignment, compression, sorting, and publish behavior in private `*_writer.cpp` files.
- Tests: Add roundtrip/policy tests under `tests/unit/*_writer_tests.cpp`.

**New Archive Family or Variant:**
- BSA implementation: Add detection/parser/reader/writer code under `src/formats/bsa/` and route from `src/archive.cpp`.
- BA2 implementation: Add detection/parser/reader/writer code under `src/formats/ba2/` and route from `src/archive.cpp`.
- Public metadata: Reuse `archive_metadata`, `entry_metadata`, `ba2_archive_metadata`, and `texture_metadata` from `include/libbsa/archive.hpp`; add public fields only when stable consumer-visible metadata is required.
- Build wiring: Add new source files to `target_sources(libbsa ...)` in `CMakeLists.txt`.
- Tests/fixtures: Add generated fixtures under `tests/fixtures/generated/` and register generator targets in `tests/CMakeLists.txt`.

**New Shared Utility:**
- Shared helpers: Add format-neutral internals to `src/detail/`.
- Texture/DDS helpers: Add DDS, DXGI, DirectXTex, mip, or chunk-order logic to `src/texture/`.
- Tests: Add focused helper tests under `tests/unit/`, matching the helper name such as `archive_path_tests.cpp` or `dds_layout_tests.cpp`.

**New Validation Rule or Compatibility Warning:**
- Public code: Add stable warning/error public surface in `include/libbsa/validation.hpp` only if callers need a new programmatic code.
- Implementation: Add validation logic in `src/validation.cpp`.
- Evidence: Update `docs/compatibility-evidence.md`, `docs/target-format-guide.md`, and policy tests under `tests/unit/compatibility_warning_tests.cpp` / `tests/unit/validation_policy_tests.cpp`.

**New Compression Route:**
- Primary code: Add codec adapter or router branch under `src/detail/`.
- Format routing: Select the route from parsed archive metadata in `src/formats/**`.
- Public API: Expose only stable `entry_compression` values in `include/libbsa/archive.hpp`.
- Tests: Add codec/router coverage under `tests/unit/compression_router_tests.cpp` and format extraction/writer tests.

**New DDS/Texture Behavior:**
- Primary code: Add DirectXTex-dependent analysis in `src/texture/directxtex_analyzer.cpp`; add pure layout/chunk logic in `src/texture/dds_layout.cpp`.
- Format code: Connect BA2 DX10 parse/extract/write behavior in `src/formats/ba2/ba2_dx10_*.cpp`.
- Tests: Add `tests/unit/dds_layout_tests.cpp`, `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, or `tests/unit/ba2_dx10_writer_tests.cpp`.

**New Fixture Data:**
- Generator source: Add or extend `tests/fixtures/generated/generate_*_fixtures.cpp`.
- Outputs: Put generated archives/manifests under `tests/fixtures/generated/archives/`; put generated DDS sources under `tests/fixtures/generated/source/`.
- Policy: Update `tests/fixtures/README.md` when provenance or generation policy changes.
- Do not use: Do not write generated files under `TES5Edit/` or `tests/fixtures/local/` for committed default fixtures.

**New Documentation:**
- Public API docs: Add Doxygen comments in `include/libbsa/` for public APIs.
- Consumer guidance: Add or update Markdown under `docs/`.
- Doxygen scope: Keep public docs rooted in `docs/Doxyfile.in` and public headers, not `src/`, `tests/`, `build/`, or `TES5Edit/`.

## Special Directories

**`TES5Edit/`:**
- Purpose: Read-only behavioral reference submodule for BSArchPro compatibility.
- Generated: No.
- Committed: Yes, as a git submodule pointer tracked by `.gitmodules`.
- Rules: Do not edit, format, compile into libbsa, stage, commit, or generate outputs under `TES5Edit/`.

**`tests/fixtures/generated/`:**
- Purpose: Legal generated fixture corpus and generator source.
- Generated: Yes.
- Committed: Yes.
- Rules: Fixtures must be tiny/legal, generated by checked-in tooling, and covered by manifests/policy tests.

**`tests/fixtures/local/`:**
- Purpose: Local-only game-derived or BSArchPro-derived fixture workspace.
- Generated: Local data may be generated or copied by maintainers.
- Committed: Only `tests/fixtures/local/.gitkeep` is committed.
- Rules: Do not commit game archives, BSArchPro outputs, or copyrighted bytes.

**`build/`:**
- Purpose: CMake configure/build/test outputs.
- Generated: Yes.
- Committed: No.
- Rules: Do not place source changes or fixtures here; regenerate through CMake presets.

**`.cocoindex_code/`:**
- Purpose: Local code index database/config.
- Generated: Yes.
- Committed: No.
- Rules: Treat as local tooling state, not source.

**`.planning/`:**
- Purpose: GSD workflow state, project documents, archived milestone/phase artifacts, research docs, and codebase maps.
- Generated: Yes, by GSD workflows and mapper agents.
- Committed: Project-dependent; current planning artifacts are present in the repo.
- Rules: Mapper write scope for this run is only `.planning/codebase/ARCHITECTURE.md` and `.planning/codebase/STRUCTURE.md`.

**`.planning/codebase/`:**
- Purpose: Generated codebase intelligence consumed by later GSD planning/execution workflows.
- Generated: Yes.
- Committed: Intended as GSD codebase map output.
- Rules: Architecture mapper writes `ARCHITECTURE.md` and `STRUCTURE.md`; other mapper agents own stack, integration, convention, testing, and concern documents.

**`.github/workflows/`:**
- Purpose: CI automation.
- Generated: No.
- Committed: Yes.
- Rules: Preserve the TES5Edit cleanliness guard in `.github/workflows/ci.yml`.

**Project Skills:**
- Purpose: Repo-local GSD/Codex skill overrides.
- Generated: Not applicable.
- Committed: Not detected.
- Rules: No `.codex/skills/` or `.agents/skills/` directories are present. Add future project-specific skills under one of those paths with a `SKILL.md` index if needed.

---

*Structure analysis: 2026-05-10*
