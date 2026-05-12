## ADDED Requirements

### Requirement: Writer disk source whole-buffer reads are bounded and centralized
Writer preparation paths that require an entire disk source in memory SHALL use a shared internal disk-source read helper that sizes the source before allocation, allocates the output buffer through libbsa result-based byte-vector helpers, and reads the source without byte-at-a-time vector growth.

#### Scenario: Compressed disk payload is read through a bounded helper
- **WHEN** a TES4 BSA or BA2 GNRL writer prepares a compressed entry from a disk source
- **THEN** the writer materializes the raw source bytes through the shared bounded read helper before invoking the selected whole-buffer codec
- **AND** the writer does not use a local `std::ifstream::get` loop that appends one byte at a time

#### Scenario: DDS add-time analysis is read through a bounded helper
- **WHEN** a BA2 DX10 writer adds a texture entry from a DDS disk source
- **THEN** the writer materializes the DDS bytes through the shared bounded read helper before invoking DirectXTex analysis
- **AND** the DirectXTex whole-buffer boundary remains explicit at the call site

### Requirement: Writer disk source prefix reads are bounded
Writer preparation paths that need only a source prefix SHALL use a shared internal prefix-read helper that allocates no more than the requested prefix length and treats shorter sources as successful short prefixes unless the underlying stream reports an I/O failure.

#### Scenario: TES4 DDS metadata probe reads only the requested prefix
- **WHEN** a TES4 BSA writer probes a disk-backed `.dds` entry for target texture-format validation
- **THEN** the writer reads at most the DDS metadata probe byte count
- **AND** the writer does not materialize the full disk source for that probe alone

### Requirement: Writer disk source chunk iteration is used when full materialization is unnecessary
Writer paths that hash, stream, or compare disk-backed payload bytes without requiring a codec or DDS analyzer SHALL use a shared chunked read helper with a bounded scratch buffer instead of materializing full source files or appending bytes one at a time.

#### Scenario: BA2 GNRL raw disk payload hashing uses chunk iteration
- **WHEN** a BA2 GNRL writer prepares a raw disk-backed entry that will be streamed into the archive
- **THEN** the payload hash is computed by iterating bounded source chunks
- **AND** the writer does not allocate a whole-file payload buffer only to compute the hash

#### Scenario: Disk-backed deduplication avoids duplicate whole-file buffers when possible
- **WHEN** a writer compares disk-backed payloads for deduplication and neither side requires codec input materialization
- **THEN** the comparison uses bounded chunks or existing staged bytes to avoid holding duplicate full disk payload buffers
- **AND** deduplication outcomes remain unchanged from the pre-change writer behavior

### Requirement: Writer source read failures preserve result-based error semantics
Writer disk-source helper failures SHALL return `result<T>` errors with existing libbsa error codes and family-specific diagnostic wording rather than leaking allocation exceptions or generic stream failures.

#### Scenario: Disk source allocation cannot be satisfied
- **WHEN** a writer disk-source helper cannot allocate a requested byte buffer for a source read
- **THEN** the helper returns a failed `result` with a libbsa allocation/format diagnostic
- **AND** `std::bad_alloc` and `std::length_error` do not escape the writer path

#### Scenario: Disk source changes during preparation or finalization
- **WHEN** a disk source size no longer matches the size used to prepare writer metadata
- **THEN** the affected writer path returns an I/O failure using the existing source-changed diagnostic for that archive family
- **AND** no archive with stale offsets, sizes, or payload metadata is published

### Requirement: Writer source read refactor preserves archive behavior
Replacing local disk read loops with shared writer source helpers SHALL NOT change public writer APIs, archive ordering, compression method selection, deduplication policy results, snapshot cleanup semantics, or emitted archive bytes for existing writer fixtures.

#### Scenario: Existing writer fixtures remain byte-stable
- **WHEN** the TES4 BSA, BA2 GNRL, and BA2 DX10 writer fixture tests run after local disk read loops are replaced
- **THEN** every archive byte sequence produced by previously covered fixture cases remains unchanged
- **AND** previously covered writer validation and overwrite errors remain in the same error-code category
