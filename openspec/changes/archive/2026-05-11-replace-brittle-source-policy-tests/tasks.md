## 1. Policy Test Inventory

- [x] 1.1 Audit `bounded_memory_policy_tests.cpp`, `benchmark_policy_tests.cpp`, `validation_policy_tests.cpp`, `docs_policy_tests.cpp`, `thread_safety_docs_policy_tests.cpp`, `target_format_policy_tests.cpp`, `ba2_gnrl_writer_tests.cpp`, and `ba2_dx10_writer_tests.cpp` for source-text and exact-prose assertions.
- [x] 1.2 Classify each assertion as `behavior`, `static-boundary`, or `doc-structure` before changing it.
- [x] 1.3 Map each `behavior` assertion to an existing or new executable oracle so no policy coverage is removed without replacement.

## 2. Behavior-Backed Coverage

- [x] 2.1 Replace memory-bound writer source-token checks with writer execution, writer stage, sparse fixture, source mutation, or bounded validation tests that exercise the relevant code path.
- [x] 2.2 Replace publish source-token checks with shared publish helper tests and writer-family save tests that assert destination bytes, overwrite/no-overwrite behavior, diagnostics, and cleanup.
- [x] 2.3 Replace validation warning coverage checks that only inspect docs or source with tests that derive public warning codes and exercise `validate_archive` or compatibility matrix rows where possible.
- [x] 2.4 Replace package-consumer integration policy checks with a compile or smoke path for the package-consumer target where the behavior can be validated by CMake/CTest.
- [x] 2.5 Replace malformed parser policy token checks with generated malformed fixture, mutated archive, or stable manifest tests that assert `result` failures and expected `libbsa::error_code` values.

## 3. Static Boundary and Documentation Checks

- [x] 3.1 Keep narrow static checks for `TES5Edit/` implementation isolation, public-header dependency isolation, private helper leakage, Windows-only build profiles, Doxygen public/private input boundaries, and benchmark target gating.
- [x] 3.2 Narrow documentation policy tests to stable structures such as headings, public type names, public warning-code entries, anchors, and evidence fields instead of incidental prose where possible.
- [x] 3.3 Rename or update test cases and failure messages so retained text checks clearly identify the hard boundary or documentation structure being protected.

## 4. Validation

- [x] 4.1 Build the test target with `cmake --build --preset windows-msvc-debug-static --target libbsa_tests --config Debug`.
- [x] 4.2 Run focused tests for the touched policy, writer publish, writer execution, validation, malformed fixture, and target-format areas.
- [x] 4.3 Run the package-consumer smoke/build path if package-consumer policy checks are changed.
- [x] 4.4 Run the full Windows Debug static CTest preset with output on failure when focused validation passes.
- [x] 4.5 Run `openspec validate "replace-brittle-source-policy-tests" --strict`.
