# libbsa Context

libbsa models Bethesda archive formats as reusable library concepts, with compatibility language kept separate from implementation mechanics.

## Language

**BA2 Profile**:
A resolved description of a BA2 archive family member: its variant, subtype, header shape, and payload compression meaning.
_Avoid_: BA2 mode, BA2 target info, raw version fields

**BA2 Record Identity**:
The subtype-specific lookup facts that bind a BA2 archive path to its stored record fields, including hash input, extension FourCC, and canonical path matching.
_Avoid_: BA2 record key pieces, path helper fields, hash tuple
