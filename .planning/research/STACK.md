# Technology Stack — v1.1 Hardening

**Project:** libbsa  
**Milestone:** v1.1 Hardening  
**Researched:** 2026-05-12  
**Scope:** Only additions or changes needed for the new hardening milestone. Existing archive-format, dependency, benchmark, docs, and packaging choices are intentionally not re-decided here.

## Recommendation Summary

This milestone should **not add any new runtime/library dependency**. The right stack move is to harden the existing Windows-first toolchain: fix host-path handling with the standard library plus Win32 APIs already in use, add one real optimized verification lane, add one real sanitizer lane, and keep the rest of the repo dependency-light.

## Required Changes

### 1) Keep the library stack unchanged

| Item | Decision | Why it fits this repo |
|------|----------|-----------------------|
| Runtime dependencies | **No new runtime deps** | The milestone is about correctness and verification, not new product capability. `libdeflate`, `lz4`, `DirectXTex`, `nlohmann-json`, and Win32 APIs already cover the problem space. |
| Unicode/path handling | **Use `std::filesystem::path` end-to-end for host paths, with wide Win32 boundaries where needed** | The known bug is Windows host-path correctness, not archive-internal path encoding. This is a codepath fix, not a dependency problem. |
| Archive-internal path model | **Do not change** | The target issue is host filesystem access. Archive keys/hashes should remain their current normalized archive-string model. |

### 2) Add one compiler/toolchain hardening flag set

| Change | Required | Why |
|--------|----------|-----|
| MSVC `/utf-8` for all library and test targets | Yes | Microsoft documents `/utf-8` as setting both source and execution character sets to UTF-8. For this repo, that makes non-ASCII test literals and path fixture names deterministic across developer machines and CI. It does **not** fix filesystem opening by itself, but it removes source-encoding drift while the host-path fix lands. |
| Keep C++20 public API contract | Yes | Hardening does not justify raising the public standard or exposing C++23-only types. |
| Keep Windows/MSVC-first toolchain | Yes | The repo is explicitly Windows-only; this milestone should reinforce that instead of adding portability work. |

### 3) Add two new supported preset/lane families

#### A. Required: optimized verification lane

| Preset | Suggested config | Purpose | Required |
|--------|------------------|---------|----------|
| `windows-msvc-release-static` | `Release`, `BUILD_SHARED_LIBS=OFF`, tests on | Catch optimization-sensitive parser/writer regressions and verify the installable static package in a real optimized build | Yes |

**Why static first:** most hardening risks here are parser math, staging lifetime, and dedupe behavior, not DLL boundary behavior. A Release static lane gives the highest value with the least matrix expansion.

#### B. Required: sanitizer lane

| Preset | Suggested config | Purpose | Required |
|--------|------------------|---------|----------|
| `windows-msvc-asan-static` | `RelWithDebInfo` preferred, `BUILD_SHARED_LIBS=OFF`, `/fsanitize=address`, debug info on | Catch memory-safety bugs in malformed parser, codec, temp-staging, and dedupe paths that normal Debug/Release tests can miss | Yes |

**Implementation notes:**

- Use **MSVC AddressSanitizer**, not a Linux Clang lane.
- Prefer **`RelWithDebInfo`** over plain Debug so hardening coverage sees more release-like code generation while keeping usable call stacks.
- Ensure the ASan preset disables incompatible MSVC settings called out by Microsoft docs, especially **Edit-and-Continue (`/ZI`)**, **incremental linking**, and **`/RTC`**.

### 4) Update CI matrix and policy tests together

| Area | Required change | Why |
|------|-----------------|-----|
| `CMakePresets.json` | Add real configure/build/test presets for `windows-msvc-release-static` and `windows-msvc-asan-static` | The current preset surface only supports Debug static/shared. The planning docs and supported presets must stop drifting. |
| `.github/workflows/ci.yml` | Run the new lanes in CI | A preset that never runs is not real coverage. |
| `tests/unit/validation_policy_tests.cpp` | Replace the current “sanitizers absent” assumptions with checks for the supported hardening lane names and Windows-only policy | The current tests intentionally enforce stale policy. They must be updated as part of the milestone, not after it. |
| Fixture/test docs | Document which tests are expected in ASan and Release lanes, and whether any expensive corpus checks stay opt-in | Maintainers need an honest supported-matrix contract. |

## Optional but Sensible

These are good follow-ons if the required work lands cleanly, but they are **not necessary to complete v1.1**.

| Option | Keep Optional Because |
|-------|------------------------|
| `windows-msvc-release-shared` lane | Useful for fuller package/export confidence, but the current hardening concerns are not primarily shared-library-specific. |
| ASan dump-file capture via `ASAN_SAVE_DUMPS` in CI artifacts | Helpful for post-mortem debugging, but not required to establish the lane. |
| Dedicated fuzz harness target using MSVC `/fsanitize=fuzzer` | Valuable later, but it is a bigger workflow commitment than this milestone needs. Treat as future hardening work, not v1.1 minimum scope. |
| Workflow presets in `CMakePresets.json` | Nice cleanup for `configure → build → test`, but they are ergonomics, not hardening. |

## Explicitly Do NOT Add

| Do not add | Why not |
|------------|---------|
| ICU, Boost.Nowide, `fmt`, `spdlog`, or any new Unicode/path helper library | Windows host-path correctness should be solved with `std::filesystem::path` plus the Win32 boundary already present in the repo. New libraries would increase surface area without solving the real archive-specific risks. |
| Cross-platform sanitizer lanes (`linux-clang-asan-ubsan`, WSL jobs, POSIX fixes) | Out of scope for a Windows-only library and directly conflicts with the repo boundary. |
| UBSan/TSan as required milestone gates | Microsoft’s current first-party sanitizer story is AddressSanitizer-focused. Do not invent a fake portable sanitizer policy the repo does not actually support. |
| New archive/runtime dependencies | The milestone is internal hardening only. |
| A persistent temp-file database, background cleanup service, or service-style helper | Over-engineered for the stated BA2 DX10 temp-staging risk. Fix lifecycle and cleanup behavior inside the existing writer flow first. |
| A public CLI or GUI just to exercise hardening lanes | Validation should remain library- and test-driven. |
| Reworking archive-internal path/hash semantics while fixing host-path Unicode | Different problem, high regression risk, not required for the milestone goal. |

## Practical Version / Policy Guidance

| Item | Recommendation |
|------|----------------|
| CMake | Keep the repo’s current **CMake 4.3.2** CI install and CMake 4.0 minimum unless a lane implementation proves otherwise. No upgrade is needed for this milestone. |
| vcpkg | Keep manifest mode and the pinned default-registry baseline already in `vcpkg-configuration.json`. No lockfile or package-manager change is needed. |
| MSVC / Visual Studio toolset | Require an MSVC toolset with **AddressSanitizer** support. Current Microsoft docs support `/fsanitize=address` on Windows x86/x64 and document CMake-based usage. |
| Dependency versions | Do not churn dependency versions just to “harden.” Only update a package if the new lane work proves a concrete incompatibility or bugfix need. |

## Milestone Planning Cut

### Must ship in v1.1

1. **No new dependency policy remains intact**.
2. **Host-path handling is standardized on `std::filesystem::path` / wide Windows opens**.
3. **`/utf-8` is applied consistently**.
4. **`windows-msvc-release-static` preset + CI lane exists and runs tests**.
5. **`windows-msvc-asan-static` preset + CI lane exists and runs tests**.
6. **Policy tests and docs are updated to reflect the new supported matrix**.

### Safe to defer

1. Release shared lane.
2. Fuzz harness.
3. ASan dump artifact plumbing.
4. Any broader dependency/toolchain modernization unrelated to the listed concerns.

## Confidence

| Area | Confidence | Notes |
|------|------------|-------|
| No-new-dependency recommendation | HIGH | Strongly supported by project constraints and the nature of the bugs. |
| Release lane recommendation | HIGH | Directly matches the documented gap in current presets/CI. |
| MSVC ASan lane recommendation | HIGH | Supported by current Microsoft documentation and by the repo’s Windows-only scope. |
| `/utf-8` recommendation | MEDIUM-HIGH | Officially supported by MSVC docs; valuable for deterministic source/test encoding, though it is supportive rather than sufficient for host-path correctness. |
| Deferring fuzzing/shared-release expansion | MEDIUM-HIGH | Good tradeoff for milestone focus, but future evidence could justify promoting them later. |

## Sources

- Project milestone and scope: `.planning/PROJECT.md`
- Current concerns and hardening gaps: `.planning/codebase/CONCERNS.md`
- Current stack snapshot: `.planning/codebase/STACK.md`
- Current preset surface: `CMakePresets.json`
- Current CI matrix: `.github/workflows/ci.yml`
- Current policy enforcement: `tests/unit/validation_policy_tests.cpp`
- Microsoft Learn — AddressSanitizer overview and MSVC/CMake usage: https://learn.microsoft.com/cpp/sanitizers/asan?view=msvc-170
- Microsoft Learn — MSVC sanitizer compiler options: https://learn.microsoft.com/cpp/build/reference/fsanitize?view=msvc-170
- Microsoft Learn — Visual Studio/CMake Presets guidance for AddressSanitizer: https://learn.microsoft.com/cpp/build/cmake-presets-vs?view=msvc-170#enable-addresssanitizer-for-windows-and-linux
- Microsoft Learn — MSVC `/utf-8`: https://learn.microsoft.com/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8?view=msvc-170
- Context7 `/kitware/cmake` — current `CMakePresets.json` schema and `workflowPresets` support
- Microsoft Learn — vcpkg manifest mode and version locking concepts: https://learn.microsoft.com/vcpkg/concepts/manifest-mode and https://learn.microsoft.com/vcpkg/consume/lock-package-versions
