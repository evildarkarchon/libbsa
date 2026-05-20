---
id: T03
parent: S05
milestone: M001-k9wo8b
key_files:
  - .gsd/exec/5c65c624-bc72-48d2-b583-f965df05a464.stdout
  - .gsd/exec/d1a5ccfd-73ca-40f0-b19e-4545699c67ec.stdout
  - .gsd/exec/d916e6bb-4da3-45ae-b93e-b501f3ff1e30.stdout
  - .gsd/exec/1f202f2d-0493-4c47-bbe7-787a84638449.stdout
  - .gsd/exec/e87bb418-9197-4fe4-bab4-fd125465e5e7.stdout
  - .gsd/exec/4c7a60f7-8c46-417e-b8f0-8c2c301c6f99.stdout
  - .gsd/exec/c93e7807-2f93-480e-956b-a72a386372ad.stdout
key_decisions:
  - Treated release static/shared package-consumer failures as advisory environment limitations because vcpkg failed before project configuration or libbsa code execution.
  - Excluded the inconclusive PowerShell retry from green proof because it emitted no CMake/CTest output despite a zero exit code; the recorded advisory release evidence uses `cmd.exe //c` attempts with visible vcpkg failure output.
duration: 
verification_result: mixed
completed_at: 2026-05-20T03:56:22.606Z
blocker_discovered: false
---

# T03: Ran the M001/S05 integrated verification gate and recorded passing debug static build, full CTest, package-consumer proof, and TES5Edit boundary evidence.

**Ran the M001/S05 integrated verification gate and recorded passing debug static build, full CTest, package-consumer proof, and TES5Edit boundary evidence.**

## What Happened

This was a verification-only closeout task after the S05 code, docs, and policy updates from T01/T02. No implementation or documentation edits were needed. I configured the default `windows-msvc-debug-static` preset, built the `libbsa_tests` target, ran the full default CTest suite, explicitly ran the `package_consumer` label, and checked the TES5Edit submodule boundary. The full debug static CTest suite reported `100% tests passed, 0 tests failed out of 438`, with only the two opt-in fixture comparison tests skipped (`local game fixtures are opt-in` and `BSArchPro-derived expected fixture comparisons are opt-in`). The explicit package-consumer label reported `100% tests passed, 0 tests failed out of 4`, making the installed-target runtime proof visible for the milestone closeout. `git status --short -- TES5Edit` emitted no status lines before the command marker, preserving the read-only reference boundary. I also attempted the optional release static and release shared package-consumer lanes. Both stopped before project configuration during vcpkg manifest install with the local vcpkg Visual Studio detector error `internal error: D:\a\_work\1\s\src\vcpkg\visualstudio.cpp(90): Value was null`; per the task plan, those heavier lanes are recorded as advisory environment limitations rather than default green proof.

## Verification

Verified the mandatory default Windows MSVC debug static path with fresh configure/build/test output after the prior S05 edits: `cmake --preset windows-msvc-debug-static` passed; `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` passed; `ctest --preset windows-msvc-debug-static --output-on-failure` passed 438/438 tests; `ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure` passed 4/4 tests; `git status --short -- TES5Edit` exited 0 with no TES5Edit status lines. Optional release static/shared package-consumer attempts were run and recorded, but both failed before libbsa configuration due a local vcpkg/Visual Studio detection internal error.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 805ms |
| 2 | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | 0 | ✅ pass | 1027ms |
| 3 | `ctest --preset windows-msvc-debug-static --output-on-failure` | 0 | ✅ pass — 438/438 tests passed; 2 opt-in tests skipped | 26230ms |
| 4 | `ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure` | 0 | ✅ pass — 4/4 package_consumer-label tests passed | 6054ms |
| 5 | `cmd.exe //c "set VCPKG_ROOT=C:\vcpkg&& set APPDATA=C:\Users\evild\AppData\Roaming&& set LOCALAPPDATA=C:\Users\evild\AppData\Local&& cmake --preset windows-msvc-release-static&& cmake --build --preset windows-msvc-release-static&& ctest --preset windows-msvc-release-static -L package_consumer --output-on-failure"` | 1 | ⚠️ advisory environment limitation — vcpkg install failed before project configure with visualstudio.cpp(90): Value was null | 333ms |
| 6 | `cmd.exe //c "set VCPKG_ROOT=C:\vcpkg&& set APPDATA=C:\Users\evild\AppData\Roaming&& set LOCALAPPDATA=C:\Users\evild\AppData\Local&& cmake --preset windows-msvc-release-shared&& cmake --build --preset windows-msvc-release-shared&& ctest --preset windows-msvc-release-shared -L package_consumer --output-on-failure"` | 1 | ⚠️ advisory environment limitation — vcpkg install failed before project configure with visualstudio.cpp(90): Value was null | 326ms |
| 7 | `git status --short -- TES5Edit` | 0 | ✅ pass — no TES5Edit status lines emitted | 74ms |

## Deviations

The optional release static and release shared package-consumer lanes did not complete. They were attempted with Windows environment variables supplied through `cmd.exe //c`, but both failed during vcpkg manifest install before project configuration with `visualstudio.cpp(90): Value was null`; this matches the plan's advisory-limitation path for heavier release lanes. No code or documentation edits were made.

## Known Issues

Local vcpkg Visual Studio detection prevents configuring the optional release static/shared package-consumer lanes from this harness. The mandatory debug static proof passes, and CI still defines release static/shared package-proof lanes in `.github/workflows/ci.yml`.

## Files Created/Modified

- `.gsd/exec/5c65c624-bc72-48d2-b583-f965df05a464.stdout`
- `.gsd/exec/d1a5ccfd-73ca-40f0-b19e-4545699c67ec.stdout`
- `.gsd/exec/d916e6bb-4da3-45ae-b93e-b501f3ff1e30.stdout`
- `.gsd/exec/1f202f2d-0493-4c47-bbe7-787a84638449.stdout`
- `.gsd/exec/e87bb418-9197-4fe4-bab4-fd125465e5e7.stdout`
- `.gsd/exec/4c7a60f7-8c46-417e-b8f0-8c2c301c6f99.stdout`
- `.gsd/exec/c93e7807-2f93-480e-956b-a72a386372ad.stdout`
