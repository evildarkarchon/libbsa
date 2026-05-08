# libbsa Fixture Policy

This directory separates committed legal fixtures from local game-derived data.
The goal is to make compatibility tests reproducible without committing
copyrighted Bethesda archives or mutating the `TES5Edit/` reference submodule.

## Committed generated fixtures

Committed fixtures must be tiny, legal, and generated specifically for tests.

- Source inputs belong under `tests/fixtures/generated/source`.
- Generated archive outputs belong under `tests/fixtures/generated/archives`.
- Generated binary fixtures are not required in Phase 1 and may be added later
  only when their provenance and recipe are documented.

## Local game-derived fixtures

Game-derived archives must not be committed to this repository.

- Local copies may be placed under ignored `tests/fixtures/local`.
- Larger local datasets may be stored outside the repository and referenced with
  the `LIBBSA_GAME_FIXTURES` environment variable.
- Tests that require local game data must be tagged `requires-game-fixture` and
  skipped by default when no local fixture path is configured.

## Provenance requirements

Each committed generated fixture must document:

1. The generator or source recipe used to create it.
2. The legal provenance that makes it safe to commit.
3. The behavior it proves, such as header parsing, fixture extraction,
   round-trip metadata, compatibility warnings, or malformed-input handling.

## TES5Edit boundary

TES5Edit/ must not be used as a fixture workspace, mutable test data location,
or source of committed fixture files. It is a read-only behavior reference only.

Do not edit, format, compile, stage, or copy generated outputs into `TES5Edit/`
while preparing libbsa tests.

## Test labels

CTest labels mirror Catch2 tags and use the following taxonomy:

- `unit` — Small deterministic tests for public and internal units.
- `fixture` — Tests using committed legal generated fixtures.
- `roundtrip` — Pack/open/extract comparisons for writer phases.
- `compat` — Compatibility checks against BSArchPro-derived or official-tool data.
- `malformed` — Invalid, truncated, oversized, or inconsistent archive inputs.
- `slow` — Longer-running tests that are not part of the quick path.
- `requires-game-fixture` — Tests discovered by default but skipped unless local
  game-derived data is available.
