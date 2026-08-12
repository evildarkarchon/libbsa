# libbsa Context

libbsa models Bethesda archive formats as reusable library concepts, with compatibility language kept separate from implementation mechanics.

## Language

**Archive Entry Catalog**:
The ordered collection of canonical archive paths and parsed entry metadata materialized when an archive is opened. It is independent of archive family once established.
_Avoid_: File list, backend entries

**Stored Payload**:
The exact byte sequence an archive entry references in the archive payload area, after any format-required prefixing and compression. It is distinct from the decoded entry bytes and from the location assigned during archive layout.
_Avoid_: Source payload, raw payload, final stored buffer

**Payload Placement**:
The assignment of a Stored Payload to a location in the archive payload area, together with any sharing of that location between records. Placement is authoritative for location and sharing; it is not authoritative for payload creation, compression, or record geometry.
_Avoid_: dedupe, payload sharing, offset assignment

**TES4 BSA Profile**:
A resolved description of a TES4-family BSA format member: its version and version-dependent layout, compression, embedded-name, and file-classification semantics. The same profile applies whether an archive is being read or written.
_Avoid_: TES4 mode, TES4 target behavior, raw version checks

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

**BA2 Record Identity**:
The subtype-specific lookup facts that bind a BA2 archive path to its stored record fields, including hash input, extension FourCC, and canonical path matching.
_Avoid_: BA2 record key pieces, path helper fields, hash tuple

**BA2 DX10 Placement Plan**:
The resolved physical layout of a BA2 DX10 archive: its ordered record and chunk geometry, references from chunks to Stored Payloads, unique Stored Payloads in physical emission order, and filename-table location. It is authoritative for placement and sharing, not for DDS interpretation, compression, or Stored Payload creation.
_Avoid_: DX10 layout result, assigned chunks, payload ownership flags

## Relationships

- Archive layout assigns a Stored Payload to a payload-area location. The location and any sharing of that location are not properties of the Stored Payload itself.

## Behaviors

- Stored Payload equality is exact byte equality. A fingerprint may narrow equality candidates but never establishes equality on its own.
