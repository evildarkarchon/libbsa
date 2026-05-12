## Why

Archive compatibility values are currently duplicated across BA2 and TES4-family BSA parsers, writer preparation code, and fixture generators. Centralizing those values reduces parser/writer/fixture drift and makes compatibility-sensitive behavior easier to audit before adding more archive variants.

## What Changes

- Add internal format-constants headers for BA2 and TES4-family BSA compatibility values.
- Move duplicated archive magic values, version numbers, record sizes, sentinel values, compression method identifiers, archive/file flags, and texture constants into those headers where they are shared by multiple implementation paths.
- Update parser, writer preparation, and generated-fixture code to use the shared constants instead of local duplicated literals.
- Preserve existing public APIs, archive bytes, validation behavior, and dependency set.

## Capabilities

### New Capabilities
- `format-compatibility-constants`: Internal format modules expose one authoritative constants surface per archive family for compatibility-sensitive on-disk values used by parsers, writers, and fixture generators.

### Modified Capabilities

None.

## Impact

- Affected implementation files include `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, and `src/formats/bsa/tes4_bsa_prepare.cpp`.
- Affected fixture generators include `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` and `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.
- No public header, package, dependency, or archive-format behavior changes are intended.
