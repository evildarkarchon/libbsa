# S02: Public API Reality Check — Research

## Summary
The public API story is already much stronger than a “basic extraction/writing” surface: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and `include/libbsa/validation.hpp` expose a dependency-light C++20 reader/writer/validation core with structured `result<T>` failure paths, stable enums, and no direct libdeflate/LZ4/DirectXTex leakage. The current evidence suggests S02 is primarily a proof-and-story pass, not a facade rewrite.

The main remaining gaps are not in the header surface itself. S01 already marked `COV-GAP-001` for direct variant-specific validation success rows and `COV-GAP-003` for package-consumer runtime proof; both are adjacent to the public API story, but neither currently justifies a broad API redesign. The likely S02 outcome is a docs/example refinement or a very small helper if real consumer friction is found.

## Findings
- `include/libbsa/archive.hpp` exposes the consumer-facing reader core: `archive_reader`, `archive_metadata`, `entry_metadata`, `payload_sink`, bulk extraction types, and thread-safety doc comments.
- `include/libbsa/writer.hpp` exposes the family-specific writer core with explicit target enums, compression policy enums, and `write_execution_options`.
- `include/libbsa/validation.hpp` exposes structured `validation_report`, fatal diagnostics, and compatibility warnings with stable public codes.
- `docs/api-mainpage.md` and `docs/integration-examples.md` already present a coherent public story for open/list/extract, bulk extract, validation, and all four writer families.
- `tests/unit/public_include_boundary_tests.cpp` and `tests/unit/export_surface_policy_tests.cpp` already prove the public header boundary is dependency-light and that public APIs are explicitly annotated/exported.
- `tests/package-consumer/main.cpp` proves installed-package compilation against `<libbsa/libbsa.hpp>` and exercises representative APIs, but it is still compile/smoke oriented rather than runtime archive-coverage proof.
- `tests/package-consumer/smoke.cmake` and `tests/package-consumer/verify-runtime-dll-copy.cmake` keep the consumer lane stable and Windows-package oriented.
- `docs/coverage-audit-matrix.md` and `docs/compatibility-evidence.md` show the remaining story risks are evidence-granularity issues, not missing public types.

## Implementation Landscape
### Files and purpose
- `include/libbsa/archive.hpp` — reader/public metadata/sink surface.
- `include/libbsa/writer.hpp` — writer targets, options, and finalization APIs.
- `include/libbsa/validation.hpp` — validation and compatibility-warning surface.
- `docs/api-mainpage.md` — high-level public API overview.
- `docs/integration-examples.md` — package-consumer examples compiled by the installed-package smoke.
- `tests/package-consumer/main.cpp` — compile-checked consumer usage example.
- `tests/package-consumer/smoke.cmake` — install/build smoke path.
- `tests/unit/public_include_boundary_tests.cpp` — dependency-light public include contract.
- `tests/unit/export_surface_policy_tests.cpp` — export macro/public API annotation contract.
- `tests/unit/target_format_policy_tests.cpp` — docs/package-consumer proof wiring.
- `tests/unit/validation_api_tests.cpp` — current validation success/error behavior proof.

### Natural seams
1. **Docs-only seam**: refine `docs/api-mainpage.md` or `docs/integration-examples.md` if the consumer story needs clearer entry points or a better “what to use when” explanation.
2. **Small helper seam**: only if a concrete consumer friction is found; keep helpers tiny and header-light so they do not become a facade layer.
3. **Validation proof seam**: direct per-variant validation success rows belong to `tests/unit/validation_api_tests.cpp` if S04 needs tighter public proof.
4. **Package-consumer seam**: if runtime proof is needed later, extend the package-consumer lane instead of adding API surface.

### First proof
The fastest high-signal proof is already in place:
- `tests/unit/public_include_boundary_tests.cpp`
- `tests/unit/export_surface_policy_tests.cpp`
- `tests/package-consumer/main.cpp`
- `docs/integration-examples.md`

If the planner wants to add anything, it should start from a doc/example mismatch or a consumer-visible helper gap that those files expose.

## Risks / Constraints
- Windows-only contract remains in force; do not spend S02 effort on portability.
- TES5Edit/ is read-only and must not be used as a mutable fixture source.
- Public headers must stay dependency-light and C++20-only; do not leak libdeflate, LZ4, DirectXTex, DXGI, or `std::expected` into the installed surface.
- `package_consumer_smoke` is intentionally compile/install oriented; it is not a runtime archive corpus proof.
- The validation API still has partial-proof granularity in `COV-GAP-001`, but that is a validation-evidence issue rather than an API-shape issue.

## Verification
Recommended proof commands for any S02 follow-up:
- `cmake --preset windows-msvc-debug-static`
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`
- `ctest --preset windows-msvc-debug-static -R "(public_include_boundary|export_surface|target_format_policy|package_consumer|validation_api)" --output-on-failure`

If a tiny helper/doc change is proposed, re-run the package-consumer smoke lane and the public-header boundary tests immediately after.

## Skills Discovered
Installed during this research pass:
- `cmake` — `mohitmishra786/low-level-dev-skills@cmake`
- `cpp-testing` — `affaan-m/everything-claude-code@cpp-testing`

No additional installable skill looked more directly relevant than those two for this slice.

## Recommendation
Do not expand S02 into a facade redesign. Treat the public API as already structurally sound, and only make a small docs/example/helper adjustment if the planner finds a concrete consumer friction point that current evidence does not already cover.