# Phase 1: Foundation, API Boundary, and Test Harness - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-07
**Phase:** 1-Foundation, API Boundary, and Test Harness
**Areas discussed:** Build and package boundary, Public API and error model, Dependency and reference boundaries, Fixture and CI test harness

---

## Build and Package Boundary

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal reusable library foundation | Create C++20 library targets, CMake/vcpkg structure, static/shared build options, and CTest wiring first. | Yes |
| Application/tool scaffold | Start with a CLI or GUI tool and extract a library later. | |
| Delay packaging | Write source first, organize package structure later. | |

**User's choice:** Auto-selected recommended default.
**Notes:** The PRD and research both define libbsa as an embeddable library, not a tool-first project.

---

## Public API and Error Model

| Option | Description | Selected |
|--------|-------------|----------|
| Small C++20-owned facade | Define libbsa-owned public shells for result/error, sources/sinks, format metadata, and future archive operations. | Yes |
| Expose implementation libraries | Let public headers include dependency and implementation types directly. | |
| Wait for parser implementation | Delay API boundary until parsing code exists. | |

**User's choice:** Auto-selected recommended default.
**Notes:** C++20 compatibility means public `std::expected` is avoided in favor of a local result/error approach.

---

## Dependency and Reference Boundaries

| Option | Description | Selected |
|--------|-------------|----------|
| Strict internal adapters | Keep libdeflate, lz4, DirectXTex, and TES5Edit reference behavior behind internal boundaries. | Yes |
| Expose dependencies for convenience | Allow dependency types into public headers. | |
| Vendor/reference code directly | Compile or copy TES5Edit/BSArchPro code into libbsa. | |

**User's choice:** Auto-selected recommended default.
**Notes:** `TES5Edit/` remains read-only reference material only.

---

## Fixture and CI Test Harness

| Option | Description | Selected |
|--------|-------------|----------|
| Test harness first | Create Catch2/CTest labels, fixture layout, legal-fixture policy, and CI-ready commands in Phase 1. | Yes |
| Tests after parsers | Add test infrastructure only after the first parser exists. | |
| Manual validation only | Rely on ad-hoc manual archive checks. | |

**User's choice:** Auto-selected recommended default.
**Notes:** Later compatibility work depends on a stable fixture and test layout.

---

## Claude's Discretion

Claude may choose exact file names, target names, and CMake helper organization while preserving the locked phase boundaries and project constraints.

## Deferred Ideas

None.
