# libbsa Context

libbsa models Bethesda archive formats as reusable library concepts, with compatibility language kept separate from implementation mechanics.

## Language

**Archive Interoperability**:
Agreement between libbsa and an independent archive tool on archive paths, decoded entry contents, and format-significant metadata, demonstrated by reading independently produced archives and independently reading libbsa-produced archives. It is distinct from Game Acceptance.
_Avoid_: Self-roundtrip proof, game compatibility proof

**Accepted Oracle Failure**:
A specifically reviewed failure of the independent archive tool whose precisely identified evidence gap is permitted by the release policy while all remaining comparisons stay required. It does not establish Archive Interoperability for the original contents the oracle could not extract.
_Avoid_: Oracle pass, archive exclusion, proven compatibility

**Game Acceptance**:
Evidence that a target game's engine can load and use an archive's entries as intended. Archive Interoperability alone does not establish Game Acceptance.
_Avoid_: Tool acceptance, oracle acceptance

**Archive Entry Catalog**:
The ordered collection of canonical archive paths and parsed entry metadata materialized when an archive is opened. It is independent of archive family once established.
_Avoid_: File list, backend entries

**CLI Extraction**:
One requested unpack operation from a host archive into a destination directory, covering archive opening, entry selection, destination preparation, and publication or cleanup of extracted files. An operation may succeed for some requested entries and fail for others while preserving their individual outcomes.
_Avoid_: Unpack pipeline, extraction orchestration

**Stored Payload**:
The exact byte sequence an archive entry references in the archive payload area, after any format-required prefixing and compression. It is distinct from the decoded entry bytes and from the location assigned during archive layout.
_Avoid_: Source payload, raw payload, final stored buffer

**Payload Placement**:
The assignment of a Stored Payload to a location in the archive payload area, together with any sharing of that location between records. Placement is authoritative for location and sharing; it is not authoritative for payload creation, compression, or record geometry.
_Avoid_: dedupe, payload sharing, offset assignment

**Sharing Eligibility**:
The format-specific condition that decides whether two byte-equal Stored Payloads are allowed to occupy one location. It exists because a shared location must also satisfy every record field describing how the payload is decoded, which byte equality alone does not guarantee.
_Avoid_: dedupe key, decode contract, chunk compatibility

**Payload Span Exclusivity**:
The property that any two Stored Payload spans in one archive are either identical in placement or completely disjoint. Distinct spans never partially overlap, so no entry can read bytes that belong to another entry's payload.
_Avoid_: overlap check, span collision, payload aliasing

**TES4 BSA Profile**:
A resolved description of a TES4-family BSA format member: its version and version-dependent layout, compression, embedded-name, and file-classification semantics. The same profile applies whether an archive is being read or written.
_Avoid_: TES4 mode, TES4 target behavior, raw version checks

**BSA Archive Opening**:
A coherent observation that resolves one TES3 or TES4-family BSA archive's header and materializes its metadata while the observed host archive remains stable. The observation ends when metadata materialization completes; later payload extraction is a separate observation.
_Avoid_: BSA detection pass, BSA parser dispatch

**BA2 Profile**:
A resolved description of a BA2 archive family member: its variant, subtype, header shape, and payload compression meaning.
_Avoid_: BA2 mode, BA2 target info, raw version fields

**BA2 Header Layout**:
The version-determined shape of a BA2 fixed header: its serialized width and which optional trailing fields it carries. BA2 header versions are an unordered tag set, not a capability ladder, so layout is always resolved by explicit per-version lookup.
_Avoid_: header size ladder, version threshold, newer BA2 version

**BA2 Archive Header**:
The validated fixed-header facts belonging to one BA2 archive, including its resolved BA2 Profile, entry count, filename-table location, and version-specific stored metadata.
_Avoid_: Detected BA2 format, BA2 header info

**BA2 Archive Opening**:
A coherent observation that resolves one BA2 archive's header and materializes its metadata while the observed host archive remains stable.
_Avoid_: BA2 detection pass, BA2 parser dispatch

**BA2 Archive Serialization**:
The ordered byte representation of one complete BA2 archive: its fixed header, subtype record table, unique Stored Payloads in physical order, and filename table. It preserves the order and placement already established by format-specific planning; it does not create, compress, or place payloads.
_Avoid_: BA2 envelope serialization, BA2 writer serialization, BA2 save pipeline

**BA2 Record Identity**:
The subtype-specific lookup facts that bind a BA2 archive path to its stored record fields, including hash input, extension FourCC, and canonical path matching.
_Avoid_: BA2 record key pieces, path helper fields, hash tuple

**BA2 DX10 Placement Plan**:
The resolved physical layout of a BA2 DX10 archive: its ordered record and chunk geometry, references from chunks to Stored Payloads, unique Stored Payloads in physical emission order, and filename-table location. It is authoritative for placement and sharing, not for DDS interpretation, compression, or Stored Payload creation.
_Avoid_: DX10 layout result, assigned chunks, payload ownership flags

## Relationships

- Archive layout assigns a Stored Payload to a payload-area location. The location and any sharing of that location are not properties of the Stored Payload itself.
- Payload Placement shares a location only when the Stored Payloads are byte-equal and Sharing Eligibility holds. Byte equality alone is not sufficient.

## Behaviors

- Stored Payload equality is exact byte equality. A fingerprint may narrow equality candidates but never establishes equality on its own.
- Sharing Eligibility is a precondition, never a proof. It can forbid a share that byte equality would permit, and never authorises one that byte equality forbids.
- Payload Placement gives a newly placed Stored Payload the payload cursor as it stands at that moment; a Stored Payload that shares an earlier location inherits that location instead. A zero-length Stored Payload takes the cursor and does not advance it, so it shares an offset with whatever is placed next — another payload, or the filename table when nothing follows. BA2 DX10 is the exception: it rejects a zero-length texture chunk rather than placing one (ADR-0001).
