---
phase: 17-writer-hotspot-hardening-and-ship-gate
artifact: verification
status: in-progress
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
