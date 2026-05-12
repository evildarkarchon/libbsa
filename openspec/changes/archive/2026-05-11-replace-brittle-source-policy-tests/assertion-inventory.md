# Assertion Inventory

## Classification

- `tests/unit/bounded_memory_policy_tests.cpp`
  - `behavior`: whole-archive writer byte-vector checks, disk streaming token checks, BA2 DX10 snapshot token checks.
  - `static-boundary`: TES5Edit implementation isolation, public-header private helper isolation, dependency-light public writer header.
- `tests/unit/benchmark_policy_tests.cpp`
  - `static-boundary`: explicit benchmark target registration, report target not registered as a default CTest gate, no fixed speedup threshold gates.
  - `doc-structure`: benchmark README command, schema, and data-policy coverage.
- `tests/unit/validation_policy_tests.cpp`
  - `behavior`: game-fixture ignore behavior through `git check-ignore`.
  - `static-boundary`: Windows-only presets and TES5Edit CI guard.
  - `doc-structure`: label taxonomy, fixture provenance, compatibility evidence entries.
- `tests/unit/docs_policy_tests.cpp`
  - `static-boundary`: optional Doxygen target and public/private Doxyfile boundaries.
  - `doc-structure`: public API mainpage references.
- `tests/unit/thread_safety_docs_policy_tests.cpp`
  - `doc-structure`: canonical sections for public types and public-header references to the guidance.
- `tests/unit/target_format_policy_tests.cpp`
  - `behavior`: package-consumer integration, now proven by CTest package-consumer smoke paths.
  - `doc-structure`: target-format headings, public writer target names, warning-code entries, legal evidence anchors.
- `tests/unit/ba2_gnrl_writer_tests.cpp`
  - `behavior`: disk source mutation, publish no-overwrite/overwrite/temp-sibling/directory/missing-source behavior.
- `tests/unit/ba2_dx10_writer_tests.cpp`
  - `behavior`: DDS source fixtures, source snapshot behavior, publish no-overwrite/overwrite/temp-sibling/non-regular behavior.
  - `static-boundary`: BA2 DX10 writer delegation to the shared publish helper was duplicated here and is covered centrally by `writer_publish_tests.cpp`.

## Behavior Oracle Map

- Memory-bound writer policies: `bsa_writer_execution_tests.cpp`, `ba2_writer_execution_tests.cpp`, BA2 GNRL source mutation tests, BA2 DX10 source snapshot tests, and sparse BA2 reader fixtures.
- Publish safety: `writer_publish_tests.cpp` plus writer-family save tests for TES3, TES4, BA2 GNRL, and BA2 DX10.
- Validation warnings: `compatibility_warning_tests.cpp` exercises `validate_archive` for every public `compatibility_warning_code`; documentation tests only verify catalog structure.
- Package-consumer integration: `package_consumer_smoke` and `package_consumer_runtime_dll_copy` CTest paths validate configure/build/run behavior.
- Malformed parser policies: TES4, BA2 GNRL, BA2 DX10, and validation API malformed fixture tests assert `result` failures and stable `libbsa::error_code` values.
