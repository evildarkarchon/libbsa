# Phase 01: Foundation, API Boundary, and Test Harness - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-07
**Phase:** 01-Foundation, API Boundary, and Test Harness
**Areas discussed:** Public facade shape, Result/error ergonomics, Build/test presets, Fixture layout policy

---

## Public Facade Shape

| Decision Point | Options Considered | User's Choice |
|----------------|--------------------|---------------|
| First callable entry point | Archive object; free functions; capability probe only | Archive object |
| Read/write scope | Read/open only; read and write shells; neutral archive type | Read/open only |
| Initial stub input | Path string view; byte span; no input | Path string view |
| Header organization | Umbrella plus focused headers; single umbrella only; focused headers only | Umbrella plus focused headers |
| Type name | `archive_reader`; `archive`; `reader` | `archive_reader` |
| Fallible construction | Explicit open; constructor opens; factory only | Explicit open |
| Stub metadata behavior | No fake metadata; library metadata only; placeholder archive metadata | No fake metadata |
| Documentation scope | Stub contract plus future intent; stub contract only; full future behavior | Stub contract plus future intent |
| C++20 modules | Headers only; experimental module; defer explicitly | Defer explicitly |
| Namespace boundary | `libbsa` plus `libbsa::detail`; only `libbsa`; versioned namespace | `libbsa` plus `libbsa::detail` |
| Install/export | Install/export now; build in-tree only; prepare but do not test | Install/export now |
| API stability | Soft lock until Phase 2; hard lock now; prototype only | Soft lock until Phase 2 |

**Notes:** The facade should prove the future reader entry point without fake parser behavior.

---

## Result/Error Ergonomics

| Decision Point | Options Considered | User's Choice |
|----------------|--------------------|---------------|
| Error contents | Code plus message; code only; rich object now | Code plus message |
| Initial taxonomy | Small future-safe core; only not implemented; full archive taxonomy | Small future-safe core |
| Boost.Outcome idea | Reject Boost; defer Boost; challenge constraint | Challenge constraint |
| Boost.Outcome scope | Research gate only; use Boost if viable; no Boost after all | Use Boost if viable |
| Public Boost exposure | No public Boost; public Boost allowed; research decides | No public Boost |
| Public result surface | Expected-like basics; minimal ok/error only; outcome-like helpers | Expected-like basics |
| `result<void>` | Yes; no; research decides | Yes |
| Bad `value()` access | Throw programmer-error exception; assert/terminate; optional/reference | Throw programmer-error exception |
| `noexcept` policy | Yes where practical; no noexcept; always noexcept | Yes where practical |
| Boost viability | Strict viability gate; basic build gate; researcher judgment | Basic build gate, later constrained by no-public-Boost decision |
| Authoritative Boost/API resolution | Allow public Boost if viable; keep public API Boost-free; research and decide later | Keep public API Boost-free |
| Error-code integration | Plain enum first; `std::error_code`; both | Plain enum first |
| Message stability | Diagnostic-only; stable strings; no messages | Diagnostic-only |

**Notes:** User's reason for challenging the dependency policy was avoiding homegrown result bugs. Final resolution: Boost.Outcome may be investigated/used only if the public API remains Boost-free; otherwise use local `libbsa::result<T>`.

---

## Build/Test Presets

| Decision Point | Options Considered | User's Choice |
|----------------|--------------------|---------------|
| Primary developer preset | MSVC Debug static; MSVC Debug shared; both by default | MSVC Debug static |
| Static/shared presets | Explicit static and shared presets; one preset plus option; static only now | Explicit static and shared presets |
| Warning strictness | High warnings, not errors; warnings as errors; default warnings only | High warnings, not errors |
| CI validation | Windows static+shared test; Windows plus Linux; single Windows build | Windows static+shared test |
| Install/package smoke test | Yes minimal; no build-tree only; install rules only | Yes minimal |
| Catch2 labels | Catch tags become CTest labels; manual labels; both | Catch tags become CTest labels |
| vcpkg dependency features | Direct dependencies for Phase 1; use tests feature; research decides | Direct dependencies for Phase 1 |
| Preset naming | Descriptive names; short names; CMake default style | Descriptive names |

**Notes:** CI should prove both linkage variants on the primary Windows/MSVC platform.

---

## Fixture Layout Policy

| Decision Point | Options Considered | User's Choice |
|----------------|--------------------|---------------|
| Committed legal fixtures | `tests/fixtures/generated`; `tests/fixtures/legal`; `tests/data` | `tests/fixtures/generated` |
| Local game fixtures | `tests/fixtures/local` ignored; external env var path; both | `tests/fixtures/local` ignored |
| Manifest/policy | README policy only; simple manifest now; no fixture docs yet | README policy only |
| Local-game tests by default | Excluded unless selected; discovered but skipped; not added until later | Discovered but skipped |
| Provenance detail | Generator/source recipe; name and purpose only; full byte manifest | Generator/source recipe |
| TES5Edit in fixture docs | Explicit prohibition; only AGENTS/PROJECT; brief reference only | Explicit prohibition |
| Placeholder taxonomy | README placeholders; labels only in CMake; only unit tests now | README placeholders |
| Skip setup instructions | Skip message with env/path hint; README only; fail if missing | Skip message with env/path hint |
| External local fixture path | Env var plus local dir; local dir only; env var only | Env var plus local dir |
| Env var name | `LIBBSA_GAME_FIXTURES`; `LIBBSA_FIXTURES`; planner decides | `LIBBSA_GAME_FIXTURES` |
| Generated binaries in git | Tiny legal binaries allowed; text manifests only; ask per fixture | Tiny legal binaries allowed |
| Source/archive layout | Inputs and archives separate; single generated folder; planner decides | Inputs and archives separate |

**Notes:** Fixture policy should be explicit about legal provenance and the read-only `TES5Edit/` boundary.

## Agent Discretion

- Exact file names and CMake internals remain planner discretion as long as they satisfy `01-CONTEXT.md` and `01-SPEC.md`.

## Deferred Ideas

- C++20 modules are explicitly deferred; Phase 1 should use conventional headers.
