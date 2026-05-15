---
last_mapped: 2026-05-15
last_mapped_commit: ead68b6856b37c3ea3acbf007180951c3cc22698
---
# Codebase Structure

**Analysis Date:** 2026-05-15

## Directory Layout

```text
libbsa/
├── include/libbsa/           # Public C++20 API headers and export macros
├── src/                      # Library implementation and private architecture
│   ├── detail/               # Shared low-level helpers: paths, I/O, codecs, publish, threading
│   ├── formats/              # Archive-family implementations
│   │   ├── bsa/              # TES3 and TES4-family BSA engines
│   │   └── ba2/              # BA2 GNRL and BA2 DX10 engines
│   └── texture/              # DDS layout and DirectXTex adapter boundary
├── tests/                    # Catch2 tests, fixture generators, package-consumer smoke tests
│   ├── unit/                 # Unit and policy tests
│   ├── fixtures/             # Generated legal fixtures and ignored local game fixture area
│   ├── package-consumer/     # Installed package consumer proof
│   └── export-surface/       # Shared DLL export checks
├── benchmarks/               # Synthetic benchmark runner and report documentation
├── docs/                     # Public/internal documentation, Doxygen config input, policies
├── cmake/                    # CMake package config templates
├── openspec/                 # OpenSpec specifications and active/archived change artifacts
├── .planning/                # GSD planning, research, phase, and codebase-map artifacts
├── .claude/skills/           # Project OpenSpec skill indexes
├── agents/                   # GSD agent prompt definitions
├── TES5Edit/                 # Read-only reference submodule; never modify or compile into libbsa
├── get-shit-done/            # Local GSD tooling/reference assets
├── .github/                  # GitHub workflows/configuration
├── CMakeLists.txt            # Root library/build/install/test graph
├── CMakePresets.json         # Windows MSVC/vcpkg build and test lanes
├── vcpkg.json                # vcpkg manifest dependencies
├── vcpkg-configuration.json  # vcpkg registry/baseline configuration
├── README.md                 # Project overview and supported verification lanes
└── AGENTS.md                 # Repository-specific agent/project constraints
```

## Directory Purposes

**`include/libbsa/`:**
- Purpose: Stable public include surface for consumers of the reusable library.
- Contains: C++20 public API headers only: archive reading/extraction, writing, validation, result/error model, version, export annotations.
- Key files: `include/libbsa/libbsa.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`, `include/libbsa/export.hpp`, `include/libbsa/version.hpp`.
- Rule: Do not include private `src/` headers or third-party implementation headers here. Keep public headers dependency-light.

**`src/`:**
- Purpose: Library implementation for public API adapters and internal components.
- Contains: Top-level adapter files (`src/archive.cpp`, `src/validation.cpp`, `src/libbsa.cpp`), shared helpers under `src/detail/`, format engines under `src/formats/`, texture helpers under `src/texture/`.
- Key files: `src/archive.cpp` for reader dispatch, `src/validation.cpp` for validation reports, `src/libbsa.cpp` for library translation unit.
- Rule: Implementation work belongs here unless it is a public API change under `include/libbsa/` or a test change under `tests/`.

**`src/detail/`:**
- Purpose: Internal shared services used by multiple archive families.
- Contains: Archive path normalization, Bethesda hashing, binary readers/writers, parser bounds checks, payload streaming, host file/path resolution, compression routing/codecs, parallel work, safe writer publish helpers.
- Key files: `src/detail/archive_path.*`, `src/detail/binary_io.*`, `src/detail/parser_primitives.*`, `src/detail/payload_stream.*`, `src/detail/compression_router.*`, `src/detail/deflate_codec.*`, `src/detail/lz4_frame_codec.*`, `src/detail/lz4_block_codec.*`, `src/detail/host_file_path.*`, `src/detail/host_file.*`, `src/detail/writer_publish.*`, `src/detail/parallel_work.*`, `src/detail/bethesda_hash.*`.
- Rule: Put cross-format utilities here only when at least two engines need them or when they centralize safety policy. Do not put format-table layout rules here.

**`src/formats/bsa/`:**
- Purpose: BSA-family implementation for TES3/Morrowind and TES4-family BSA variants.
- Contains: Format detection, parser, reader, writer, prepare, layout, serialize, constants, and table helpers.
- Key files: `src/formats/bsa/bsa_format_detector.*`, `src/formats/bsa/tes3_bsa_parser.*`, `src/formats/bsa/tes3_bsa_reader.*`, `src/formats/bsa/tes3_bsa_writer.*`, `src/formats/bsa/tes4_bsa_parser.*`, `src/formats/bsa/tes4_bsa_reader.*`, `src/formats/bsa/tes4_bsa_writer.*`, `src/formats/bsa/tes4_bsa_prepare.*`, `src/formats/bsa/tes4_bsa_layout.*`, `src/formats/bsa/tes4_bsa_serialize.*`, `src/formats/bsa/tes4_bsa_table.*`.
- Rule: TES3-specific relative data offset and TES4-specific embedded-name/compression/hash behavior stays here.

**`src/formats/ba2/`:**
- Purpose: BA2-family implementation for Fallout 4 and Starfield GNRL/DX10 archive variants.
- Contains: Format detection, GNRL parser/reader/writer pipeline, DX10 parser/reader/writer pipeline, texture snapshot/chunk helpers, constants.
- Key files: `src/formats/ba2/ba2_format_detector.*`, `src/formats/ba2/ba2_gnrl_parser.*`, `src/formats/ba2/ba2_gnrl_reader.*`, `src/formats/ba2/ba2_gnrl_writer.*`, `src/formats/ba2/ba2_dx10_parser.*`, `src/formats/ba2/ba2_dx10_reader.*`, `src/formats/ba2/ba2_dx10_writer.*`, `src/formats/ba2/ba2_dx10_chunk_assembler.*`, `src/formats/ba2/ba2_dx10_snapshot_builder.*`.
- Rule: BA2 Starfield v2/v3 header fields, v3 `CompressionMethod`, GNRL vs DX10 subtype rules, and DX10 chunk metadata stay here.

**`src/texture/`:**
- Purpose: Private DDS/DX10 texture boundary.
- Contains: DDS DXT10 header construction, DXGI/mip sizing, BA2 DX10 chunk planning, chunk validation/order, DirectXTex-backed DDS metadata/source analysis.
- Key files: `src/texture/dds_layout.*`, `src/texture/directxtex_analyzer.*`.
- Rule: Convert DirectXTex data to libbsa-owned metadata here; do not leak DirectXTex/DXGI types into `include/libbsa/`.

**`tests/unit/`:**
- Purpose: Catch2 unit, integration-style, policy, regression, fixture, and packaging behavior tests.
- Contains: One `.cpp` test file per component or policy area.
- Key files: `tests/unit/archive_reader_tests.cpp`, `tests/unit/archive_reader_dispatch_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/compression_router_tests.cpp`, `tests/unit/writer_publish_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.
- Rule: Add focused tests next to the behavior area. Policy tests enforce architectural constraints such as public include boundaries, export surface, thread-safety docs, target formats, and bounded memory.

**`tests/fixtures/`:**
- Purpose: Legal generated fixture corpus plus ignored local game-derived fixture area.
- Contains: `tests/fixtures/generated/` source and archive fixtures, `tests/fixtures/local/` ignored local data placeholder, fixture policy documentation.
- Key files: `tests/fixtures/README.md`, `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.
- Rule: Committed fixtures must be tiny, legal, synthetic, and generated under this tree. Do not use `TES5Edit/` as a writable fixture workspace.

**`tests/package-consumer/`:**
- Purpose: Verify installed CMake package consumption and runtime DLL copying for shared builds.
- Contains: Consumer `CMakeLists.txt`, smoke script, runtime DLL copy/verify scripts, minimal `main.cpp`.
- Key files: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`, `tests/package-consumer/copy-runtime-dlls.cmake`, `tests/package-consumer/verify-runtime-dll-copy.cmake`.
- Rule: Package/export changes must keep this smoke path passing in Release static/shared lanes.

**`tests/export-surface/`:**
- Purpose: Validate shared DLL exports stay explicit and intended.
- Contains: CMake script using dumpbin/linker hints.
- Key files: `tests/export-surface/check-dll-exports.cmake`.

**`benchmarks/`:**
- Purpose: Synthetic benchmark runner and benchmark policy documentation.
- Contains: `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`.
- Rule: Benchmark inputs are legal synthetic data only; do not use game archives or `TES5Edit/` as benchmark inputs.

**`docs/`:**
- Purpose: Project documentation and Doxygen support.
- Contains: Thread-safety policy, compatibility evidence, public API docs config input, PRD and other docs.
- Key files: `docs/thread-safety.md`, `docs/compatibility-evidence.md`, `docs/Doxyfile.in`, `docs/PRD.md`.
- Rule: Public API threading/lifetime behavior must be reflected here when relevant.

**`cmake/`:**
- Purpose: CMake package template support.
- Contains: `cmake/libbsaConfig.cmake.in`.
- Rule: Add package dependencies here when adding private link dependencies that installed consumers must find.

**`openspec/`:**
- Purpose: OpenSpec specs and change artifacts for structured project changes.
- Contains: Capability specs such as `openspec/specs/writer-safe-publish/spec.md` and `openspec/specs/writer-state-ownership/spec.md`.
- Rule: Follow `.claude/skills/openspec-*` workflows when creating/applying/syncing/archiving OpenSpec changes.

**`.planning/`:**
- Purpose: GSD planning, research, roadmap, phase, and generated codebase map artifacts.
- Contains: `.planning/codebase/`, `.planning/research/`, `.planning/phases/`, `.planning/quick/`, roadmap/state/requirements files.
- Key files: `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/STRUCTURE.md`, `.planning/PROJECT.md`, `.planning/STATE.md`, `.planning/ROADMAP.md`.
- Rule: Codebase mapping documents are generated here and consumed by future GSD planning/execution commands.

**`TES5Edit/`:**
- Purpose: Read-only behavioral reference submodule for BSArchPro/xEdit compatibility.
- Contains: Delphi/Pascal reference code, including areas named in `AGENTS.md` such as `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, `TES5Edit/Core/wbBSA.pas`.
- Rule: Never edit, format, stage, compile, or write fixtures into this directory.

## Key File Locations

**Entry Points:**
- `include/libbsa/libbsa.hpp`: Public umbrella header.
- `include/libbsa/archive.hpp`: Public reader/extraction contracts and metadata.
- `include/libbsa/writer.hpp`: Public writer contracts and target/option enums.
- `include/libbsa/validation.hpp`: Public validation API.
- `src/archive.cpp`: Reader open/detect/dispatch and extraction implementation.
- `src/validation.cpp`: Validation report implementation.
- `src/formats/bsa/tes3_bsa_writer.cpp`: TES3 public writer bridge and write pipeline entry.
- `src/formats/bsa/tes4_bsa_writer.cpp`: TES4-family public writer bridge and write pipeline entry.
- `src/formats/ba2/ba2_gnrl_writer.cpp`: BA2 GNRL public writer bridge and write pipeline entry.
- `src/formats/ba2/ba2_dx10_writer.cpp`: BA2 DX10 public writer bridge and one-shot snapshot cleanup entry.

**Configuration:**
- `CMakeLists.txt`: Root library target, dependencies, source list, install/export, docs, benchmarks, tests.
- `tests/CMakeLists.txt`: Catch2 test graph, fixture generator targets, package-consumer tests, export-surface tests.
- `CMakePresets.json`: Supported Windows MSVC static/shared/debug/release/ASan presets.
- `vcpkg.json`: Manifest dependencies.
- `vcpkg-configuration.json`: vcpkg registry baseline/configuration.
- `cmake/libbsaConfig.cmake.in`: Installed CMake package dependency discovery.
- `.clangd`: Local C++ language-server settings.
- `docs/Doxyfile.in`: Doxygen configuration input for public API documentation.

**Core Logic:**
- `src/detail/binary_io.*`: Little-endian readers/writers for archive fields.
- `src/detail/parser_primitives.*`: Archive metadata bounds, overflow checks, safe parser allocations, file byte reads.
- `src/detail/archive_path.*`: Canonical virtual archive path normalization.
- `src/detail/host_file_path.*`: Caller UTF-8 host path to resolved Windows-native path seam.
- `src/detail/host_file.*`: Shared host file open/size/read helpers.
- `src/detail/payload_stream.*`: Bounded payload source/sink transfer helpers.
- `src/detail/compression_router.*`: Compression/decompression dispatch with exact-size checks.
- `src/detail/deflate_codec.*`: libdeflate implementation boundary.
- `src/detail/lz4_frame_codec.*`: LZ4 frame implementation boundary for Skyrim SE BSA payloads.
- `src/detail/lz4_block_codec.*`: Raw LZ4 block implementation boundary for Starfield BA2 v3 payloads.
- `src/detail/writer_publish.*`: Safe no-overwrite/overwrite publication helper.
- `src/detail/parallel_work.*`: Shared positive-worker-count scheduler.
- `src/texture/dds_layout.*`: DDS header/chunk layout logic.
- `src/texture/directxtex_analyzer.*`: DirectXTex adapter.

**BSA Format Logic:**
- `src/formats/bsa/bsa_format_detector.*`: Detect TES3/TES4-family BSA variants.
- `src/formats/bsa/tes3_bsa_parser.*`: Parse TES3 BSA header/table/name/hash metadata.
- `src/formats/bsa/tes3_bsa_reader.*`: TES3 lookup and raw extraction.
- `src/formats/bsa/tes3_bsa_prepare.*`: TES3 writer source preparation.
- `src/formats/bsa/tes3_bsa_layout.*`: TES3 writer offset/layout assignment.
- `src/formats/bsa/tes3_bsa_serialize.*`: TES3 writer byte emission.
- `src/formats/bsa/tes4_bsa_parser.*`: Parse TES4-family header/table/name metadata.
- `src/formats/bsa/tes4_bsa_reader.*`: TES4-family lookup, embedded-name stripping, deflate/LZ4-frame extraction.
- `src/formats/bsa/tes4_bsa_prepare.*`: TES4-family writer validation, grouping, hashing, sorting, compression.
- `src/formats/bsa/tes4_bsa_layout.*`: TES4-family writer offsets and dedupe.
- `src/formats/bsa/tes4_bsa_serialize.*`: TES4-family writer byte emission.
- `src/formats/bsa/tes4_bsa_table.*`: TES4-family table helpers.
- `src/formats/bsa/tes4_bsa_constants.hpp`: TES4-family constants.

**BA2 Format Logic:**
- `src/formats/ba2/ba2_format_detector.*`: Detect BA2 GNRL/DX10, Fallout 4/Starfield versions, Starfield v3 compression method.
- `src/formats/ba2/ba2_gnrl_parser.*`: Parse BA2 GNRL metadata and filename table.
- `src/formats/ba2/ba2_gnrl_reader.*`: BA2 GNRL lookup, raw/deflate/raw-LZ4 extraction.
- `src/formats/ba2/ba2_gnrl_prepare.*`: BA2 GNRL writer validation and payload preparation.
- `src/formats/ba2/ba2_gnrl_layout.*`: BA2 GNRL payload/file table layout and dedupe.
- `src/formats/ba2/ba2_gnrl_serialize.*`: BA2 GNRL byte emission.
- `src/formats/ba2/ba2_dx10_parser.*`: Parse BA2 DX10 texture records, chunks, and logical texture metadata.
- `src/formats/ba2/ba2_dx10_reader.*`: BA2 DX10 lookup, DDS header reconstruction, raw/deflate/raw-LZ4 chunk extraction.
- `src/formats/ba2/ba2_dx10_prepare.*`: BA2 DX10 target validation, texture chunk preparation, compression, sorting.
- `src/formats/ba2/ba2_dx10_layout.*`: BA2 DX10 payload/file table layout and dedupe.
- `src/formats/ba2/ba2_dx10_serialize.*`: BA2 DX10 byte emission.
- `src/formats/ba2/ba2_dx10_snapshot_builder.*`: DDS add-time snapshot support.
- `src/formats/ba2/ba2_dx10_chunk_assembler.*`: BA2 DX10 chunk payload assembly.
- `src/formats/ba2/ba2_constants.hpp`: BA2 constants.

**Testing:**
- `tests/unit/*_tests.cpp`: Catch2 test sources.
- `tests/fixtures/generated/*.cpp`: Fixture generator tools compiled by CMake.
- `tests/fixtures/generated/archives/`: Committed synthetic archive fixtures and manifests.
- `tests/fixtures/generated/source/`: Committed synthetic DDS/source fixture inputs.
- `tests/fixtures/local/.gitkeep`: Ignored local game-derived fixture location.
- `tests/package-consumer/main.cpp`: Minimal downstream package consumer.
- `tests/export-surface/check-dll-exports.cmake`: Shared export test script.

**Documentation and Policy:**
- `README.md`: Build lanes, platform support, reference boundary.
- `AGENTS.md`: Project constraints for implementation and agent behavior.
- `docs/thread-safety.md`: Thread-safety contract matching public API docs.
- `tests/fixtures/README.md`: Fixture/legal-data policy and manifest schema.
- `benchmarks/README.md`: Benchmark input/report policy.
- `.planning/research/STACK.md`: Researched technology/dependency baseline.

## Naming Conventions

**Files:**
- Public headers use concise API nouns under `include/libbsa/`: `archive.hpp`, `writer.hpp`, `validation.hpp`, `result.hpp`.
- Internal implementation files use snake_case `.hpp`/`.cpp` pairs: `src/detail/payload_stream.hpp`, `src/detail/payload_stream.cpp`.
- Format files are prefixed by archive family/subtype and stage: `tes4_bsa_prepare.cpp`, `tes4_bsa_layout.cpp`, `tes4_bsa_serialize.cpp`, `ba2_dx10_reader.cpp`.
- Tests end in `_tests.cpp`: `tests/unit/compression_router_tests.cpp`, `tests/unit/writer_publish_tests.cpp`.
- Fixture generators start with `generate_`: `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.

**Directories:**
- Public API: `include/libbsa/`.
- Private shared internals: `src/detail/`.
- Format engines: `src/formats/<family>/`, currently `src/formats/bsa/` and `src/formats/ba2/`.
- Texture-only internals: `src/texture/`.
- Unit tests: `tests/unit/`.
- Generated legal fixtures: `tests/fixtures/generated/`.
- Local ignored game fixtures: `tests/fixtures/local/`.
- Package consumer proof: `tests/package-consumer/`.

**Namespaces:**
- Public API lives in `namespace libbsa`.
- Shared internals live in `namespace libbsa::detail`.
- BSA format internals live in `namespace libbsa::formats::bsa`.
- BA2 format internals live in `namespace libbsa::formats::ba2`.
- Texture internals live in `namespace libbsa::texture`.

**Types and Functions:**
- Types/classes/enums use snake_case: `archive_reader`, `entry_metadata`, `tes4_bsa_writer`, `error_code`.
- Functions use snake_case and include format/stage prefixes when internal: `parse_ba2_dx10_archive_file`, `tes4_prepare_folders`, `ba2_gnrl_assign_payload_offsets`.
- Public writer target/option types include the archive family: `tes4_bsa_target`, `ba2_gnrl_writer_options`, `ba2_dx10_writer_options`.
- Internal prepared records use `prepared` in names: `tes4_prepared_entry`, `ba2_dx10_prepared_chunk`.

## Where to Add New Code

**New Public Reader Feature:**
- Public API: add stable value types or methods in `include/libbsa/archive.hpp` only when needed by consumers.
- Dispatch/adaptation: update `src/archive.cpp` if the feature is common to all reader backends.
- Format implementation: add behavior in the specific `src/formats/bsa/*_reader.*` or `src/formats/ba2/*_reader.*` file.
- Tests: add or update `tests/unit/archive_reader*_tests.cpp` plus format-specific reader tests such as `tests/unit/tes4_bsa_reader_tests.cpp` or `tests/unit/ba2_dx10_extraction_tests.cpp`.

**New Public Writer Feature:**
- Public API: add options/types/methods in `include/libbsa/writer.hpp` with Doxygen comments.
- Public bridge: update the relevant writer file such as `src/formats/bsa/tes4_bsa_writer.cpp` or `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Format pipeline: place validation/source work in `*_prepare.*`, offsets/dedupe in `*_layout.*`, serialization in `*_serialize.*`.
- Safe output: keep final path publication through `src/detail/writer_publish.*`.
- Tests: add focused writer tests under `tests/unit/*_writer_tests.cpp` or policy tests if changing invariants.

**New Archive Variant or Subtype:**
- Detection: extend `src/formats/bsa/bsa_format_detector.*` or `src/formats/ba2/ba2_format_detector.*`.
- Parser: add `*_parser.*` under the relevant family directory.
- Reader: add `*_reader.*` and wire backend selection in `src/archive.cpp`.
- Writer: add `*_writer.*`, `*_prepare.*`, `*_layout.*`, `*_serialize.*` when write-new support is in scope.
- Public API: extend `archive_variant`, target enums, metadata, or writer classes in `include/libbsa/` only when the variant is part of the supported contract.
- Tests/fixtures: add generated legal fixtures under `tests/fixtures/generated/` and tests under `tests/unit/`.

**New Shared Utility:**
- Cross-format parsing, allocation, or overflow helper: `src/detail/parser_primitives.*`.
- Byte-order helper: `src/detail/binary_io.*`.
- Archive virtual path helper: `src/detail/archive_path.*`.
- Host filesystem helper: `src/detail/host_file.*` or `src/detail/host_file_path.*`.
- Payload streaming helper: `src/detail/payload_stream.*`.
- Compression helper: `src/detail/compression_router.*` plus codec-specific files.
- Thread scheduling helper: `src/detail/parallel_work.*`.
- Tests: add a focused `tests/unit/<utility>_tests.cpp` and list it in `tests/CMakeLists.txt`.

**New Texture/DDS Behavior:**
- DDS metadata/source analysis: `src/texture/directxtex_analyzer.*`.
- DDS header, format size, mip/chunk planning/order: `src/texture/dds_layout.*`.
- BA2 DX10 integration: `src/formats/ba2/ba2_dx10_prepare.*`, `src/formats/ba2/ba2_dx10_reader.*`, `src/formats/ba2/ba2_dx10_serialize.*`.
- Tests: `tests/unit/dds_layout_tests.cpp`, `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`.

**New Tests:**
- Add the test source under `tests/unit/` and append it to the `libbsa_tests` source list in `tests/CMakeLists.txt`.
- If testing internal headers, include from `src/` using the existing test include directories (`tests/CMakeLists.txt:129`).
- Add fixture generators under `tests/fixtures/generated/` and register custom targets/BYPRODUCTS in `tests/CMakeLists.txt`.
- Use local game-derived data only under `tests/fixtures/local/` or via environment variables documented in `tests/fixtures/README.md`; do not commit game bytes.

**New Build/Packaging Behavior:**
- Root target/dependency/install/export changes: `CMakeLists.txt`.
- Installed package dependency discovery: `cmake/libbsaConfig.cmake.in`.
- Windows preset lane changes: `CMakePresets.json`.
- Package-consumer proof changes: `tests/package-consumer/` and `tests/CMakeLists.txt`.
- Export-surface checks: `tests/export-surface/check-dll-exports.cmake`.

**New Documentation:**
- Public API/threading/lifetime docs: `docs/` and Doxygen comments in `include/libbsa/`.
- Fixture policy updates: `tests/fixtures/README.md`.
- Benchmark policy updates: `benchmarks/README.md`.
- Planning/codebase maps: `.planning/codebase/`.

## Special Directories

**`TES5Edit/`:**
- Purpose: Read-only BSArchPro/xEdit reference submodule.
- Generated: No.
- Committed: Yes as a submodule pointer.
- Rule: Never edit, format, stage, compile, or write generated artifacts here.

**`vcpkg_installed/`:**
- Purpose: Local vcpkg installed package tree created by builds.
- Generated: Yes.
- Committed: No; treat as build output.
- Rule: Do not read it for project architecture except to confirm dependency presence; do not edit generated package files.

**`build/`, `CMakeFiles/`, `Testing/`:**
- Purpose: Local CMake/CTest build and test outputs.
- Generated: Yes.
- Committed: No.
- Rule: Do not place source files here. Use build presets to regenerate.

**`tests/fixtures/generated/`:**
- Purpose: Committed legal fixture source/archive data generated by project tools.
- Generated: Yes, by CMake fixture targets.
- Committed: Yes for tiny synthetic fixtures and manifests.
- Rule: Keep provenance legal and synthetic; update generator code rather than hand-editing binary fixture outputs.

**`tests/fixtures/local/`:**
- Purpose: Ignored local game-derived fixture location.
- Generated: No, user/local data.
- Committed: No, except `.gitkeep`.
- Rule: Never commit game archives or local BSArchPro expected data.

**`.planning/codebase/`:**
- Purpose: Generated codebase-map documents consumed by GSD planning/execution.
- Generated: Yes by mapper agents.
- Committed: Project-dependent planning artifact.
- Rule: Keep file paths current and actionable.

**`.claude/skills/`:**
- Purpose: Project skill indexes for OpenSpec workflows.
- Generated: Yes by OpenSpec skill installation.
- Committed: Yes.
- Rule: Use `SKILL.md` as the lightweight index; load deeper `rules/*.md` only when implementing a matching workflow.

**`.kilocode/`, `.opencode/`, `.agent/`, `.cursor/`, `.codex/`, `.gemini/`:**
- Purpose: Tool-specific workflow/skill/agent compatibility assets.
- Generated: Mostly tool-managed.
- Committed: Present in repo.
- Rule: For this project, new commands and agents belong under `.kilo/` per environment config; do not add new Kilo assets under `.kilocode/` or `.opencode/`.

**`openspec/`:**
- Purpose: OpenSpec specifications and change artifacts.
- Generated: Yes through OpenSpec workflows.
- Committed: Yes when specs/changes are part of project process.
- Rule: Follow OpenSpec skills for proposal/design/spec/tasks lifecycle; implementation still belongs in `include/`, `src/`, `tests/`, `docs/`, and `cmake/`.

---

*Structure analysis: 2026-05-15*
