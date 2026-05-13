## ADDED Requirements

### Requirement: TES3 payload overlap validation avoids quadratic scans
The TES3 BSA parser SHALL validate payload overlaps using ordered non-empty archive-absolute payload spans rather than comparing each entry span against every previously seen entry span. It MUST preserve TES3 stored-hash ordering, duplicate stored-hash, stored-hash/name, canonical-path, data-section-relative offset, and archive-bound validation before reporting payload overlap failures.

#### Scenario: Overlapping non-empty TES3 payload spans are rejected
- **WHEN** a TES3 BSA archive contains two non-empty entries whose data-section-relative payload ranges overlap after conversion to archive-absolute offsets
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** no public entry metadata is materialized from the overlapping payload layout

#### Scenario: Adjacent non-overlapping TES3 payload spans are accepted
- **WHEN** a TES3 BSA archive contains non-empty entries whose payload end offsets exactly match the next payload start offsets without crossing
- **THEN** payload overlap validation does not prevent the parser from materializing entries
- **THEN** public entry metadata exposes the same archive-absolute payload offsets derived from the TES3 data section

#### Scenario: Zero-byte TES3 entries do not occupy payload bytes
- **WHEN** a TES3 BSA archive contains a zero-byte entry at an offset that is also the boundary of another entry's payload range
- **THEN** payload overlap validation does not treat the zero-byte entry as overlapping payload data
- **THEN** the zero-byte entry may still be materialized if all other TES3 metadata validation succeeds

#### Scenario: Hash-order validation keeps precedence over overlap validation
- **WHEN** a TES3 BSA archive contains unsorted stored hash records and also contains payload records that would overlap after offset conversion
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error` for the hash-order violation before payload overlap validation changes the failure precedence

#### Scenario: Large non-overlapping TES3 payload sets avoid prior-span scans
- **WHEN** a TES3 BSA archive contains a large number of valid non-overlapping payload spans in hash-table order that differs from payload-offset order
- **THEN** payload overlap validation orders spans for adjacency checking instead of scanning each span against every previously seen span
- **THEN** the parser can materialize the archive entries without quadratic overlap-validation work
