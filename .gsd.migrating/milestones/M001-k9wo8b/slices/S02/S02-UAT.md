# S02: Public API Reality Check — UAT

**Milestone:** M001-k9wo8b
**Written:** 2026-05-20T02:28:47.867Z

# S02 UAT: Public API Reality Check

## UAT Type
Documentation/API contract and installed-package smoke verification.

## Preconditions
1. Work from `J:/libbsa` on Windows with the `windows-msvc-debug-static` preset available.
2. No copyrighted local game fixtures, BSArchPro-derived outputs, or mutable `TES5Edit/` fixture data are required.
3. The S02 artifacts are present: `docs/public-api-reality-check.md`, `docs/coverage-audit-matrix.md`, updated public docs, package-consumer smoke source, and updated policy tests.

## Steps
1. Open `docs/public-api-reality-check.md` and confirm it covers the core public API journey: umbrella include, `archive_reader`, lookup/contains, single-entry extraction, `extract_bytes`, bulk extraction, validation reports, compatibility warnings, stable error-code branching, and all writer families.
2. In the same audit document and `docs/coverage-audit-matrix.md`, confirm `COV-GAP-001`, `COV-GAP-003`, and `COV-GAP-004` are explicitly routed rather than silently closed.
3. Open `docs/integration-examples.md` and verify a first consumer can follow guidance for host filesystem paths versus archive virtual paths, streaming extraction versus bounded `extract_bytes`, bulk extraction `worker_count`, validation/report handling, writer finalization, and BA2 DX10 one-shot writer lifecycle.
4. Inspect `tests/package-consumer/main.cpp` and confirm it includes only `<libbsa/libbsa.hpp>` for libbsa API usage while compile-checking representative reader lookup/extraction and missing-path error handling.
5. Run `cmake --preset windows-msvc-debug-static`.
6. Run `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`.
7. Run `ctest --preset windows-msvc-debug-static -L "public-api|docs_policy|target_format_policy|package_consumer|validation_api|coverage_audit_matrix" --output-on-failure`.

## Expected Outcomes
- The audit document is discoverable from public docs and names proof status, sharp edges, helper/API gap decisions, and downstream ownership.
- Public docs do not overstate optional local corpus or BSArchPro-derived compatibility evidence as default proof.
- Package-consumer smoke compiles and links representative examples through the installed/exported libbsa target and umbrella header.
- Runtime package-consumer proof stays legal and cheap by using missing/invalid host-path checks rather than local copyrighted fixtures.
- The labeled CTest command passes, including public-boundary, docs-policy, target-format-policy, package-consumer, validation API, and coverage-matrix checks.

## Edge Cases to Check
- Missing or invalid archive host paths produce stable error-code branches without relying on diagnostic-message text.
- Docs distinguish archive virtual paths from host filesystem paths.
- Public headers remain dependency-light C++20 and do not expose libdeflate, LZ4, DirectXTex/DXGI, private Windows internals, or C++23-only public types.
- Package-consumer smoke has no dependency on ignored/local fixture directories or mutable `TES5Edit/` data.
- BA2 DX10 writer guidance remains clear that it has a one-shot/finalization lifecycle.

## Not Proven By This UAT
- Every supported archive family opened or written through the installed-package consumer at runtime; that remains COV-GAP-003 for S05.
- New reader/writer/round-trip behavior fixes; those belong to S03/S04.
- Full optional local game-corpus or BSArchPro-derived compatibility proof; those checks remain advisory.
- Runtime observability or telemetry; S02 is a documentation/API-proof slice.
