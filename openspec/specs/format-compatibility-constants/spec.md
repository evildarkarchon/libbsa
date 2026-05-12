## Purpose

Specify how compatibility-sensitive on-disk constants are centralized per archive family while remaining internal implementation details and preserving existing archive behavior.

## Requirements

### Requirement: Format compatibility constants are authoritative per family
libbsa SHALL define compatibility-sensitive on-disk constants in internal per-family format headers when those values are used by more than one parser, writer, layout, serialization, or fixture-generation path.

#### Scenario: BA2 paths share archive-contract values
- **WHEN** BA2 GNRL or BA2 DX10 parser, writer, layout, serialization, or generated-fixture code needs shared BA2 archive signatures, subtype identifiers, version numbers, header sizes, record sizes, sentinel values, compression method identifiers, or DX10 texture markers
- **THEN** that code uses the internal BA2 constants header as the source of those values
- **THEN** the same value is not redeclared as an unrelated local compatibility literal in each path

#### Scenario: TES4-family BSA paths share archive-contract values
- **WHEN** TES4-family BSA parser, writer, layout, serialization, or generated-fixture code needs shared BSA version numbers, header sizes, folder/file record sizes, archive flag bits, file flag bits, embedded-name bits, or size-flag compression masks
- **THEN** that code uses the internal TES4-family BSA constants header as the source of those values
- **THEN** the same value is not redeclared as an unrelated local compatibility literal in each path

### Requirement: Constants remain internal implementation details
The format constants headers SHALL remain internal to libbsa implementation and test-generation code and MUST NOT become part of the installed public API.

#### Scenario: Public API does not expose constants headers
- **WHEN** consumers include installed `libbsa` public headers
- **THEN** no public header requires or exposes `src/formats/ba2/ba2_constants.hpp`
- **THEN** no public header requires or exposes `src/formats/bsa/tes4_bsa_constants.hpp`

### Requirement: Constant centralization preserves archive behavior
Centralizing compatibility constants SHALL NOT change parser validation outcomes, writer-selected on-disk values, generated fixture bytes, or fixture manifests.

#### Scenario: Existing fixtures keep the same observable behavior
- **WHEN** the BA2 GNRL, BA2 DX10, and TES4-family BSA parser and writer tests run after constants are centralized
- **THEN** existing valid fixtures still parse successfully with the same metadata
- **THEN** existing malformed fixtures still fail with the same error categories
- **THEN** writer round-trip or byte-level assertions continue to pass

#### Scenario: Regenerated fixtures remain compatible
- **WHEN** the BA2 GNRL and BA2 DX10 fixture generator targets are built and run after constants are centralized
- **THEN** generated archive bytes and manifests continue to satisfy the existing fixture tests
- **THEN** fixture generators no longer carry independent copies of shared BA2 archive-contract constants
