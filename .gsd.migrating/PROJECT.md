# Project

## What This Is

libbsa is a reusable Windows-only C++20 library for reading, extracting, validating, and writing Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. It is already a relatively established library with substantial implementation, tests, generated fixtures, package-consumer checks, documentation, compression support, and texture handling in place.

The current project work is productization and stabilization: close gaps, including testing gaps; fix bugs discovered by evidence; and shape a proper public API surface because the current surface feels basic from prior interactions even though existing headers may already expose more capability than remembered.

## Core Value

libbsa must be a trustworthy embeddable archive library: consumers should be able to read, inspect, extract, validate, and write supported Bethesda archives through a clean dependency-light API, with support claims backed by repeatable tests and compatibility evidence.

## Project Shape

- **Complexity:** complex
- **Why:** The codebase already spans multiple archive families, compression routes, DDS texture handling, generated fixtures, package-consumer integration, validation, and optional compatibility evidence; the unknowns are API ergonomics, exhaustive compatibility confidence, performance/stress proof, and release readiness rather than greenfield implementation.

## Current State

Most of the library appears built and M001 has established a verified support-truth baseline. Repository evidence shows:

- Public headers under `include/libbsa/` for archive reading, writing, validation, result/error handling, versioning, and export control.
- Format implementations under `src/formats/bsa/` and `src/formats/ba2/` for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Internal compression and file/path primitives under `src/detail/` using libdeflate, LZ4 frame/block, and Windows-oriented host path handling.
- DDS/texture analysis internals under `src/texture/` using DirectXTex behind libbsa-native metadata.
- Catch2/CTest tests under `tests/unit/`, generated fixture tools under `tests/fixtures/generated/`, package-consumer smoke tests under `tests/package-consumer/`, and documentation under `docs/`.
- `docs/coverage-audit-matrix.md` is the current support-truth source for M001-covered families and capability axes; COV-GAP-001 and COV-GAP-003 are closed/former gaps, while COV-GAP-002 and COV-GAP-004 remain deferred with rationale and future ownership.
- Optional local game archive and BSArchPro-derived comparison paths are documented and gated by environment variables rather than required for default CI.

No external users are known yet, so the project can still improve public API shape without migration burden.

## Architecture / Key Patterns

- Keep the explicit public core: `archive_reader` for read/list/find/extract, `validate_archive` for structured validation, and family-specific write-new classes for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Public headers remain dependency-light and C++20-compatible. They must not expose libdeflate, LZ4, DirectXTex, DXGI, Windows implementation details, or C++23-only public types.
- Use `libbsa::result<T>` and stable `error_code` categories for expected I/O, format, validation, compression, and caller-input failures.
- Use generated legal fixtures, always-on CTest policy checks, and package-consumer tests as default proof; keep local game archives and BSArchPro-derived expected manifests as opt-in compatibility evidence.
- Use docs-policy and target-format policy tests as guardrails for public support claims, dependency-light public headers, optional-evidence boundaries, and TES5Edit coupling.
- Installed package-consumer runtime proof should create representative archives at runtime through `<libbsa/libbsa.hpp>` and `libbsa::libbsa`, then reopen, validate, and extract them through public APIs without source-tree fixture coupling.
- `TES5Edit/` is read-only prior art and compatibility reference only. Do not edit, format, stage, compile, vendor, or use it as a mutable fixture workspace.

## Capability Contract

See `.gsd/REQUIREMENTS.md` for the explicit capability contract, requirement status, and coverage mapping.

## Milestone Sequence

- [x] M001: Audit and Stabilization Baseline - Made current coverage truthful, audited the API story, closed a risk-bounded tranche of high-risk gaps with durable proof, and proved the default debug static build/test/package-consumer gate.

Completed GSD milestone directory: `M001-k9wo8b`.

Future likely sequence after M001:

- Public API Polish — Use M001 evidence to add ergonomic helpers or small API refinements without prematurely introducing a broad facade.
- Compatibility Hardening — Deepen BSArchPro/game-corpus comparison workflows and format-specific edge-case confidence.
- Performance and Operational Polish — Stress large archives, benchmark throughput, and harden parallel extraction/packing behavior.
- Release Readiness — Finalize docs, packaging, versioning, support matrix, and publishable library contract.
