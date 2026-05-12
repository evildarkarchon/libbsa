## ADDED Requirements

### Requirement: Public writer types are non-copyable
The TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writer classes SHALL NOT be copy constructible or copy assignable. This prevents separate public writer objects from aliasing the same mutable staged entries, options, or snapshot state.

#### Scenario: Writer copy operations are unavailable
- **WHEN** compile-time type traits inspect `tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, and `ba2_dx10_writer`
- **THEN** each writer type is not copy constructible
- **AND** each writer type is not copy assignable

### Requirement: Public writer ownership is movable without shared mutable aliases
The TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writer classes SHALL remain move constructible and move assignable. Moving a writer SHALL transfer ownership of its staged writer state to the destination writer rather than creating shared mutable aliases.

#### Scenario: Writer move operations are available
- **WHEN** compile-time type traits inspect `tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, and `ba2_dx10_writer`
- **THEN** each writer type is move constructible
- **AND** each writer type is move assignable

#### Scenario: Moved-to writer finalizes staged state
- **WHEN** a writer with staged entries is move constructed or move assigned into another writer
- **THEN** `write_to` on the destination writer finalizes the staged entries that were transferred
- **AND** the move does not cause the source and destination writer objects to share mutable staged entries

### Requirement: Writer behavior is unchanged apart from copy removal
Removing writer copyability SHALL NOT change add-entry validation, writer options, `write_to` overwrite behavior, compression routing, emitted archive bytes, or BA2 DX10 snapshot cleanup semantics.

#### Scenario: Existing writer behavior remains stable
- **WHEN** existing TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writer tests run after copy removal
- **THEN** tests that verify archive output, validation errors, overwrite behavior, compression metadata, and snapshot cleanup continue to pass without expectation changes
