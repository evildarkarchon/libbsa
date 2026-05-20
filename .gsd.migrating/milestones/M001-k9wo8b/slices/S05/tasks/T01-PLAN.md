---
estimated_steps: 8
estimated_files: 1
skills_used: []
---

# T01: Add installed package runtime archive smoke

Expected executor skills/frontmatter: `cpp-testing`, `cmake`, `verify-before-complete`.

Why: S02 proved the installed target and umbrella header compile/link representative public APIs, but S05 must close `COV-GAP-003` by proving runtime archive behavior from the installed package for every current archive family. The package consumer is the right layer because it uses `find_package(libbsa CONFIG REQUIRED)` and links `libbsa::libbsa` rather than the in-tree test target.

Do: Extend `tests/package-consumer/main.cpp` with a self-contained runtime smoke helper. It should include only `<libbsa/libbsa.hpp>` from libbsa plus standard C++20 headers; keep direct `<libbsa/archive.hpp>`, `<libbsa/writer.hpp>`, `<libbsa/validation.hpp>`, and `<libbsa/result.hpp>` includes forbidden. Create a clean working directory under the consumer process working directory, write any temporary source files there, and remove/recreate it on each run. Use public writer APIs to produce one small archive per family: TES3 BSA, a TES4-family BSA target, BA2 GNRL, and BA2 DX10. For BA2 DX10, generate a tiny legal DDS DXT10 source file inside the consumer working directory using standard C++ bytes; `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` may be consulted for the DDS header shape, but the package-consumer executable must not read generated fixture files or any source-tree fixture path at runtime. Open each writer-produced archive with `archive_reader::open`, verify metadata type/variant at a family level, list entries, use `find`/`contains`, extract through `extract_bytes`, and use the existing sink helper where practical. Validate each archive with `validate_archive` and `validate_entry_extractability = true`, using the correct expected top-level family. Preserve the existing missing-path `io_error` smoke check so public error-code branching remains covered.

Done when: The package-consumer executable still compiles through the installed umbrella header only, writes and reopens all four family archives without local fixtures, returns nonzero on any failed package runtime proof, and retains the missing archive `io_error` check.

Q3 Threat surface: This task writes only tiny synthetic files under the consumer build working directory. It must not read environment-provided local corpora, copy copyrighted data, or touch `TES5Edit/`. No secrets, network, auth, or user-submitted data are involved.

Q5 Failure modes: If install, runtime DLL copy, DDS generation, writer finalization, archive open, validation, or extraction fails, the consumer executable should return a failing exit code through CTest rather than silently degrading to compile-only proof.

Q6 Load profile: The smoke creates four tiny archives and one tiny DDS source. It is intentionally not a performance or large-archive stress test.

Q7 Negative tests: Preserve the missing-path `io_error` assertion; policy tests in T02 will assert no forbidden fixture/env/TES5Edit tokens are introduced.

## Inputs

- `tests/package-consumer/main.cpp`
- `tests/package-consumer/CMakeLists.txt`
- `tests/package-consumer/smoke.cmake`
- `tests/CMakeLists.txt`
- `include/libbsa/libbsa.hpp`
- `include/libbsa/archive.hpp`
- `include/libbsa/writer.hpp`
- `include/libbsa/validation.hpp`
- `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`

## Expected Output

- `tests/package-consumer/main.cpp`

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa
ctest --preset windows-msvc-debug-static -R package_consumer_smoke --output-on-failure

## Observability Impact

Package-consumer failures become executable CTest failures at `package_consumer_smoke`, localizing installed-target runtime breakage separately from in-tree unit fixture coverage.
