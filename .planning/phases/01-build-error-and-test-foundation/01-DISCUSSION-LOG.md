# Phase 1: Build, Error, and Test Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-05
**Phase:** 1-Build, Error, and Test Foundation
**Areas discussed:** Build shape, Result API, Test layout, Boundary docs

---

## Build Shape

| Option | Description | Selected |
|--------|-------------|----------|
| One library target | Create `libbsa` only, with tests as separate executables. | |
| Object + wrapper | Use an object library internally plus static/shared wrapper targets from day one. | |
| Static/shared options | Expose CMake options to build both static and shared library variants immediately. | yes |
| You decide | Let the planner choose the minimal target strategy that satisfies SPEC.md. | |

**User's choice:** Static/shared options
**Notes:** Phase 1 should make static/shared output configurable rather than hard-code one output mode.

| Option | Description | Selected |
|--------|-------------|----------|
| Build-tree only | No install/export yet; just configure, build, and test locally. | |
| Basic install | Install library and public headers, but no generated package config yet. | yes |
| Full package config | Add install/export targets and CMake package config/version files now. | |
| You decide | Let the planner pick the smallest correct package surface. | |

**User's choice:** Basic install
**Notes:** Install library and headers, but keep package-config complexity optional/deferred.

| Option | Description | Selected |
|--------|-------------|----------|
| Windows vcpkg preset | Add a primary Windows/MSVC vcpkg preset now. | yes |
| Windows + Linux | Add both Windows and Linux presets now, even if Linux CI comes later. | |
| No presets yet | Document raw CMake commands only for Phase 1. | |
| You decide | Let the planner choose based on current repo needs. | |

**User's choice:** Windows vcpkg preset
**Notes:** Windows is the primary target; cross-platform presets can wait.

| Option | Description | Selected |
|--------|-------------|----------|
| Root CMake finds all | Root `CMakeLists.txt` finds libdeflate, lz4, DirectXTex, Catch2 immediately. | yes |
| Private adapter stubs | Root finds runtime deps privately but only links them through internal placeholder/stub source files if needed. | |
| Manifest only | Declare all deps in vcpkg but only find/link Catch2 until later phases. | |
| You decide | Let the planner keep dependency resolution aligned with SPEC acceptance. | |

**User's choice:** Root CMake finds all
**Notes:** Dependency resolution must be proven in Phase 1.

---

## Result API

| Option | Description | Selected |
|--------|-------------|----------|
| `result<T>` class | A small value type with success/error state and checked access. | yes |
| Alias-like wrapper | Keep it minimal, close to a tagged union wrapper over value/error. | |
| Output-param style | Prefer explicit error codes with output parameters, despite SPEC favoring result/error. | |
| You decide | Let planner choose a C++20-compatible shape. | |

**User's choice:** `result<T>` class
**Notes:** Use a local C++20-compatible class, not `std::expected`.

| Option | Description | Selected |
|--------|-------------|----------|
| `result<void>` | One result concept for value and no-value operations. | yes |
| Separate status | Use a separate `status`/`status_code` type for no-value operations. | |
| Bool + error | Use boolean success plus separate error object for void operations. | |
| You decide | Let planner pick the smallest coherent API. | |

**User's choice:** `result<void>`
**Notes:** Keep one result concept for void and valued operations.

| Option | Description | Selected |
|--------|-------------|----------|
| Category + message | Error category/code plus short message; no offsets/context yet. | yes |
| Category only | Just enum category/code in Phase 1; messages wait until parsers exist. | |
| Rich context now | Include offset, path, source location, and nested causes immediately. | |
| You decide | Let planner choose enough detail for tests and future growth. | |

**User's choice:** Category + message
**Notes:** Rich parser context waits until parser phases.

| Option | Description | Selected |
|--------|-------------|----------|
| Value/error access | Test construction, `has_value`, value access, error access, and propagation helpers. | yes |
| Minimal construction | Only test success and error construction initially. | |
| Monadic helpers | Also require `and_then`/`transform` style helpers now. | |
| You decide | Let planner decide exact ergonomic tests. | |

**User's choice:** Value/error access
**Notes:** Monadic helpers are not required in Phase 1.

---

## Test Layout

| Option | Description | Selected |
|--------|-------------|----------|
| One foundation test exe | Single `libbsa_tests` target with initial result/error tests. | yes |
| Per-domain executables | Separate test executables from day one, e.g. `foundation_tests`, later `format_tests`. | |
| Library + smoke app | Catch2 tests plus a tiny consumer smoke executable. | |
| You decide | Let planner choose the cleanest test target layout. | |

**User's choice:** One foundation test exe
**Notes:** Avoid premature test target fragmentation.

| Option | Description | Selected |
|--------|-------------|----------|
| Catch discover | Use Catch2's CMake discovery helper if available. | yes |
| Manual add_test | Register the test executable manually with `add_test`. | |
| Both smoke + discover | Use discovery for unit tests and a manual consumer smoke test. | |
| You decide | Let planner choose based on Catch2 integration details. | |

**User's choice:** Catch discover
**Notes:** Prefer idiomatic Catch2/CTest integration.

| Option | Description | Selected |
|--------|-------------|----------|
| `unit` only | Only label current tests as unit; fixture/compat labels wait until relevant. | |
| Future labels now | Create labels such as unit, fixture, roundtrip, compat, slow now even if unused. | |
| Unit + smoke | Use `unit` for result API and `smoke` for build/link/public-header checks. | yes |
| You decide | Let planner decide labels while preserving future extensibility. | |

**User's choice:** Unit + smoke
**Notes:** Keep future labels deferred until relevant.

| Option | Description | Selected |
|--------|-------------|----------|
| Compile-only test | Add a test translation unit that includes public headers and no private deps. | yes |
| Static grep check | Use a script/test that scans headers for forbidden dependency includes. | |
| Both compile + scan | Add both a compile-only consumer test and a forbidden-include scan. | |
| You decide | Let planner pick a practical check for Phase 1. | |

**User's choice:** Compile-only test
**Notes:** Public-header isolation should be verified by compiling a consumer-style translation unit.

---

## Boundary Docs

| Option | Description | Selected |
|--------|-------------|----------|
| README + CMake comment | Mention in README and CMake near source lists. | yes |
| README only | Keep boundary documentation in README and existing AGENTS.md only. | |
| Dedicated docs file | Add a new compatibility/reference doc just for TES5Edit rules. | |
| You decide | Let planner choose the least noisy visible placement. | |

**User's choice:** README + CMake comment
**Notes:** Make the boundary visible to both users and implementers.

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit source lists | Use explicit `target_sources`; no recursive globbing. | yes |
| Exclude rule | Allow globbing but explicitly exclude `TES5Edit/`. | |
| CMake assertion | Add a configure-time check that errors if any source path contains `TES5Edit`. | |
| You decide | Let planner choose a practical safeguard. | |

**User's choice:** Explicit source lists
**Notes:** Avoid accidental source inclusion by not globbing sources.

| Option | Description | Selected |
|--------|-------------|----------|
| Defer notes file | No new notes file until first format behavior is traced. | yes |
| Create notes stub | Add a placeholder docs file for future BSArchPro compatibility notes. | |
| Use code comments | Only add notes when code implements a compatibility constraint. | |
| You decide | Let planner decide how to avoid empty docs clutter. | |

**User's choice:** Defer notes file
**Notes:** Avoid empty compatibility docs until there is real traced behavior.

| Option | Description | Selected |
|--------|-------------|----------|
| Public + src private | Public headers in `include/libbsa`, private headers under `src/`. | yes |
| Public + private include | Use `include/libbsa` and `include/libbsa/detail` for internal details. | |
| Flat public now | Only one public header now; private organization waits until more code exists. | |
| You decide | Let planner pick the cleanest foundation. | |

**User's choice:** Public + src private
**Notes:** Keep implementation details out of public include trees.

---

## the agent's Discretion

No selected areas were left to the agent's discretion.

## Deferred Ideas

None.
