# libbsa Context

libbsa models Bethesda archive formats as reusable library concepts, with compatibility language kept separate from implementation mechanics.

## Language

**Archive Entry Catalog**:
The ordered collection of canonical archive paths and parsed entry metadata materialized when an archive is opened. It is independent of archive family once established.
_Avoid_: File list, backend entries

**TES4 BSA Profile**:
A resolved description of a TES4-family BSA format member: its version and version-dependent layout, compression, embedded-name, and file-classification semantics. The same profile applies whether an archive is being read or written.
_Avoid_: TES4 mode, TES4 target behavior, raw version checks

**BA2 Profile**:
A resolved description of a BA2 archive family member: its variant, subtype, header shape, and payload compression meaning.
_Avoid_: BA2 mode, BA2 target info, raw version fields

**BA2 Archive Header**:
The validated fixed-header facts belonging to one BA2 archive, including its resolved BA2 Profile, entry count, filename-table location, and version-specific stored metadata.
_Avoid_: Detected BA2 format, BA2 header info

**BA2 Archive Opening**:
A coherent observation that resolves one BA2 archive's header and materializes its metadata while the observed host archive remains stable.
_Avoid_: BA2 detection pass, BA2 parser dispatch

**BA2 Record Identity**:
The subtype-specific lookup facts that bind a BA2 archive path to its stored record fields, including hash input, extension FourCC, and canonical path matching.
_Avoid_: BA2 record key pieces, path helper fields, hash tuple
