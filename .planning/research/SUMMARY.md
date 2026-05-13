# Research Summary — v1.1 Hardening

**Project:** libbsa  
**Milestone:** v1.1 Hardening  
**Researched:** 2026-05-12  
**Confidence:** HIGH overall; MEDIUM for the exact BA2 DX10 staging redesign details until implemented against fixtures

## Executive Summary

v1.1 should be planned as a **hardening-only milestone**. The research is consistent: do not expand the public API, do not add new archive support, and do not introduce new runtime dependencies. The milestone should instead close known correctness gaps, make the verification story truthful, and reduce risk in the most fragile internal hotspots.

The highest-value sequence is: **fix Windows host-path correctness first**, **reconcile and enforce the supported verification lanes second**, then perform **small internal seam extractions** that make reader dispatch and BA2 DX10 staging safer to maintain. Dedupe and DX10 cleanup are in scope, but only as targeted reliability work under existing behavior rules.

## Stack / Tooling Additions or Changes

### Keep unchanged
- **No new runtime dependencies.** Keep `libdeflate`, `lz4`, `DirectXTex`, `nlohmann-json`, and existing Win32 usage.
- **Keep C++20 and Windows/MSVC-first policy.** No C++23 public API changes, no cross-platform expansion.
- **Keep vcpkg manifest mode and current dependency baselines.**

### Add / change for v1.1
- **Standardize host filesystem handling** on `std::filesystem::path` internally, with wide Win32 boundaries where needed.
- **Enable MSVC `/utf-8`** for library and test targets to make non-ASCII literals and fixtures deterministic.
- **Add a real optimized verification lane:** `windows-msvc-release-static`.
- **Add a real sanitizer lane:** `windows-msvc-asan-static` using MSVC AddressSanitizer, preferably `RelWithDebInfo`.
- **Update CI, presets, and policy tests together** so supported hardening lanes are explicit and enforced.

### Explicit non-goals
- No ICU / Boost.Nowide / fmt / spdlog / new path helper libraries.
- No Linux/WSL sanitizer lanes.
- No fuzzing program as a required v1.1 deliverable.

## Requirement Categories / Table Stakes

### Must ship
1. **Host Path Reliability**
   - Fix non-ASCII Windows host-path handling across open, validate, parser, reader, and writer disk-source paths.
2. **Verification Policy Reconciliation**
   - Make `.planning`, `CMakePresets.json`, CI, and policy tests agree on what hardening lanes are supported.
3. **Stronger Hardening Lanes**
   - Ship at least Release coverage plus a supported ASan lane.
4. **Reader Dispatch Stability**
   - Remove repeated archive-family branching after open by dispatching once into a stored backend.
5. **Fragile Parser / Preparer Refactor**
   - Extract only the highest-risk helpers from TES4 BSA parsing and BA2 DX10 preparation.
6. **Dedupe Hotspot Cleanup**
   - Reduce cost/fragility in TES4 and BA2 GNRL dedupe without changing exact-byte correctness rules.
7. **BA2 DX10 Temp-Staging Cleanup**
   - Shorten temp-data lifetime and make cleanup deterministic during normal write flow.

### Should-have if capacity remains
- Planning/codebase wording cleanup.
- Focused regression-test expansion around extracted helpers.
- Better local real-corpus verification workflow guidance.
- Honest documentation of any residual DX10 temp-data leakage risk after abnormal termination.

### Out of scope for v1.1
- New archive families.
- New public API surfaces.
- Full writer redesign.
- Broad performance program.
- Major fuzzing subsystem.

## Key Architectural Sequencing Guidance

### Recommended order
1. **Windows host-path boundary hardening**
   - Introduce one internal host-path I/O boundary and route all host-file opens through it.
2. **Verification-lane reconciliation**
   - Add the real Release + ASan lanes and update policy/tests/docs in the same slice.
3. **Reader backend dispatch cleanup**
   - Store open-time backend operations in `archive_reader::state` so branching happens once.
4. **Targeted parser/preparer extraction**
   - Extract narrow helpers from `tes4_bsa_parser` and `ba2_dx10_prepare`; avoid broad rewrites.
5. **Dedupe hotspot hardening**
   - Optimize candidate narrowing only; preserve final stored-byte equality and finalization revalidation.
6. **BA2 DX10 temp-staging lifecycle hardening**
   - Move staging ownership from writer-object lifetime to write-session lifetime.
7. **Milestone ship gate**
   - Re-run Release lane, malformed suites, package/export smoke, and opt-in compatibility checks for touched families.

### Internal seam recommendations
- **One host-path boundary** for all host open/read/size operations.
- **One reader backend seam** selected at open time.
- **One DX10 staging/session seam** scoped to `write_to`, not writer lifetime.

### Architecture constraints to preserve
- Keep public headers and public result/error model unchanged.
- Keep format-family directory ownership intact.
- Keep validation reusing strict reader/parser behavior.
- Do not build a generic plugin/registry framework.

## Major Pitfalls / Watch-Outs

1. **Fixing Unicode host paths in only one entry point**
   - Prevent by centralizing all host-file I/O behind one helper and testing open + validate across families.

2. **Mixing host-path fixes with archive virtual path semantics**
   - Prevent by keeping `std::filesystem::path` out of archive lookup/hash logic.

3. **Adding Release/ASan lanes without updating policy tests and docs**
   - Prevent by landing presets, CI, `.planning`, and `validation_policy_tests` together.

4. **Refactoring large parser/preparer files before characterization tests exist**
   - Prevent by adding narrow boundary tests first and separating mechanical moves from semantic edits.

5. **Weakening dedupe correctness while optimizing**
   - Prevent by using hashes/digests only as candidate filters and preserving exact stored-byte comparisons.

6. **Cleaning DX10 temp data only on the happy path**
   - Prevent by testing success and failure cleanup paths and documenting residual crash/termination risk honestly.

7. **Over-scoping into architectural redesign**
   - Prevent by limiting v1.1 to targeted seam cleanup, not a new reader/writer framework.

## Recommended Scope Boundaries

### In scope
- Windows host-path correctness.
- Supported Release + ASan lane truthfulness.
- Small internal dispatch and staging seams.
- Narrow hotspot extraction in TES4 parser and BA2 DX10 preparer.
- Dedupe hardening under current semantics.
- DX10 temp lifecycle cleanup for normal execution and ordinary failure unwinding.

### Defer
- Release shared lane.
- Fuzz harness / sanitizer expansion beyond ASan.
- Deep DX10 staging redesign for memory/performance.
- Broad benchmark/performance work unrelated to known hotspots.
- Any public API redesign.

## Planning Implications

- Plan v1.1 as **correctness boundary → verification boundary → structural cleanup → hotspot hardening → ship gate**.
- Treat **Host Path Reliability** and **Verification Policy Reconciliation + Stronger Hardening Lanes** as the first mandatory slices.
- Keep parser/preparer refactors **file-by-file and test-backed**.
- Keep dedupe and DX10 work **semantics-preserving**; any behavioral drift should be treated as a bug unless explicitly proven necessary.

## Sources

- `.planning/research/STACK.md`
- `.planning/research/FEATURES.md`
- `.planning/research/ARCHITECTURE.md`
- `.planning/research/PITFALLS.md`
