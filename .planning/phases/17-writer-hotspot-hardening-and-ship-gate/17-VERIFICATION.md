---
phase: 17-writer-hotspot-hardening-and-ship-gate
artifact: verification
status: passed
created: 2026-05-15
---

# Phase 17 Verification Evidence

This artifact records the committed-assets ship-gate evidence for Phase 17 and v1.1 closure. Optional local game-corpus or BSArchPro comparison tests remain advisory and are not required for official sign-off.

## Requirement Coverage

| Requirement | Evidence | Status |
|-------------|----------|--------|
| DEDU-01 | TES4-family writer runtime tests plus writer_hotspot_policy source-policy guardrails in focused Debug and ASan gates. | PASS |
| DEDU-02 | BA2 GNRL writer runtime tests plus writer_hotspot_policy source-policy guardrails in focused Debug and ASan gates. | PASS |
| DX10-01 | BA2 DX10 writer cleanup/consumed-state runtime tests in focused Debug and ASan gates. | PASS |
| DX10-02 | BA2 DX10 lifecycle documentation and public declaration-shape policy tests in focused Debug and ASan gates. | PASS |

## Task 1: Focused Debug and ASan Writer-Hotspot Gates

### Debug focused writer/runtime and policy gate

- **Command:** `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|writer_hotspot_policy"`
- **Status:** PASS
- **Result:** 111/111 selected CTest tests passed.
- **Coverage:** DEDU-01, DEDU-02, DX10-01, and DX10-02.
- **Notes:** The selected label set included TES4-family writer behavior, BA2 GNRL writer behavior, BA2 DX10 writer lifecycle behavior, and five writer_hotspot_policy tests.

### MSVC AddressSanitizer focused writer-hotspot gate

- **Command:** `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure -L "tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer|writer_hotspot_policy"`
- **Status:** PASS
- **Result:** 111/111 selected CTest tests passed.
- **Coverage:** DEDU-01, DEDU-02, DX10-01, and DX10-02 under the supported MSVC AddressSanitizer hardening lane.
- **Notes:** The build emitted expected MSVC ASan linker warnings (`LNK4300`/`LNK4075`) about incremental linking being ignored for ASan-instrumented binaries; tests passed.

## Official Sign-Off Boundary

- Optional local game-corpus tests are advisory and not required for this official v1.1 ship gate.
- Optional BSArchPro comparison tests are advisory and not required for this official v1.1 ship gate.
- Planning closure must remain pending if any Debug, ASan, Release package proof, or public-surface invariant gate records `FAIL` in this file.

## Task 2: Release Package Proof and Public-Surface Invariants

### Release package proof

- **Command:** `cmake --build --preset windows-msvc-release-static && ctest --preset windows-msvc-release-static --output-on-failure -R "package_consumer_smoke|package_consumer_runtime_dll_copy"`
- **Status:** PASS
- **Result:** 2/2 selected CTest tests passed: `package_consumer_smoke` and `package_consumer_runtime_dll_copy`.
- **Coverage:** Proves the supported Release static install/export and runtime-DLL-copy package-consumer path remains runnable from committed assets.

### Public BA2 DX10 writer API stability evidence

- **Command:** `git diff -- include/libbsa/writer.hpp`
- **Status:** PASS
- **Result:** No output; the working tree has no uncommitted diff in `include/libbsa/writer.hpp`.
- **Public signatures changed:** No. Public BA2 DX10 writer declarations and `ba2_dx10_writer_options` fields were not expanded by Plan 17-05.
- **Changed public headers:** None in Plan 17-05.
- **Focused declarations reviewed:** `ba2_dx10_writer_options` still exposes only `overwrite_existing`, `deduplicate_payloads`, `max_decoded_chunk_bytes`, `starfield_unknown1`, `starfield_unknown2`, and `starfield_compression_method`; `ba2_dx10_writer` still exposes constructors, destructor, move operations, `target`, `options`, `add_file`, and the two `write_to` overloads.

### Plan 17-04 writer_hotspot_policy public-surface result

- **Command:** `ctest --preset windows-msvc-debug-static --output-on-failure -L writer_hotspot_policy`
- **Status:** PASS
- **Result:** 5/5 writer_hotspot_policy tests passed, including `writer_hotspot_policy keeps public BA2 DX10 writer declaration shape stable`.
- **Coverage:** Confirms the Plan 17-04 public-surface policy still proves no new public BA2 DX10 writer methods, options, or signatures were introduced.

### Workspace boundary and dependency-surface evidence

- **Command:** `git status --short -- TES5Edit vcpkg.json include/libbsa/writer.hpp`
- **Status:** PASS
- **Result:** No output.
- **TES5Edit boundary:** `TES5Edit/` was not modified.
- **Dependency surface:** `vcpkg.json` did not gain new dependencies.
- **Public writer header:** `include/libbsa/writer.hpp` has no Plan 17-05 modification.
