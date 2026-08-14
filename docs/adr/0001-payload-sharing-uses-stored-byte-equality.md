# ADR-0001: Payload sharing uses Stored Payload byte equality

- Status: Accepted
- Date: 2026-08-11

## Context

Payload Placement may share one payload-area location between several archive records when their
payloads are equal. libbsa and the BSArchPro reference decide "equal" differently, and the
difference is observable in written archive bytes, so it needs a recorded decision rather than an
implicit one.

The reference (`TES5Edit/Core/wbBSArchive.pas`) shares payloads through `FindPackedData`, gated on
the opt-in `ShareData` property (`-share` on the CLI, off by default). It matches candidates on
`{uncompressed source size, MD5 of uncompressed source bytes}`:

```pascal
for i := 0 to Pred(fPackedDataCount) do
  if (aSize = fPackedData[i].Size) and CompareMem(@aHash, @fPackedData[i].Hash, SizeOf(aHash)) then
```

On a match it copies the found record's `Offset`, `Size` and `PackedSize` onto the new record and
writes no new bytes. There is no byte-for-byte verification: the MD5 match alone authorises sharing.
The candidate key also carries no compression state, and `FindPackedData` is called at the top of
`PackData` before compression runs.

libbsa instead keys on Stored Payload facts and treats byte equality as the sharing authority. The
key narrows candidates; `stored_payload::exactly_equals` decides. `CONTEXT.md` already states this as
a Behavior: *"Stored Payload equality is exact byte equality. A fingerprint may narrow equality
candidates but never establishes equality on its own."*

Three facts determined the decision:

1. A dedupe key is only a bucketing device. Because confirmation is authoritative, changing the key
   alone changes no output. Matching the reference's sharing scope would require moving the
   *confirmation* to uncompressed source bytes.
2. Confirming on source bytes conflicts with a stated constraint. `AGENTS.md` requires streaming I/O
   and bounded scratch buffers rather than whole-archive memory loading, and the BA2 DX10 chunk
   assembler deliberately releases raw bytes once compression completes
   (`ba2_dx10_chunk_assembler.cpp`, "The compressed Stored Payload is the only final-byte owner after
   this point"). Source-byte confirmation would require retaining, re-reading, or decompressing
   payloads at placement time.
3. The two rules agree almost everywhere. Compression is deterministic for fixed parameters, so
   identical source bytes under identical parameters always produce identical stored bytes. The rules
   therefore select the same set except when two entries share source bytes but carry different
   per-entry compression policy. That is possible only for TES4-family BSA and BA2 GNRL, which expose
   `entry_compression_policy`. It is impossible for BA2 DX10, where compression is forced constant,
   so libbsa's DX10 sharing already matches the reference's scope exactly.

In that single divergent case the reference is defective: it copies the matched record's sizes and
packed size without comparing compression state, so a raw entry can inherit a compressed entry's
record fields.

## Decision

Payload Placement shares a location only when the Stored Payloads are exactly byte-equal. A
fingerprint or other narrowing key may reduce the candidate set but never authorises sharing on its
own.

libbsa does not adopt the reference's uncompressed-source MD5 rule.

## Consequences

- For BA2 DX10, output matches the reference's sharing scope exactly, because forced compression makes
  stored-byte equality and source-byte equality the same relation.
- For TES4-family BSA and BA2 GNRL, libbsa declines one class of share the reference would make: two
  entries with identical source bytes but different per-entry compression policy. libbsa emits a
  slightly larger, correct archive where the reference emits a smaller one with mis-copied record
  sizes. This is a deliberate divergence from the reference under the `AGENTS.md` allowance for a
  documented reason to diverge.
- A hash collision can never produce a wrong share, because no hash is trusted as an equality proof.
- Placement needs no access to uncompressed source bytes, so the bounded-memory constraint holds and
  the DX10 assembler can keep releasing raw bytes at the earliest opportunity.
- Sharing remains opt-in, matching the reference's default. libbsa's `deduplicate_payloads` defaults
  to `false`, as `ShareData` does.

## Related divergence: empty BA2 DX10 chunks

Recorded here because it is a second deliberate departure in the same area.

The reference applies no per-chunk size guard to texture archives. Its only BA2 DX10 emptiness check
in `Save` is `Length(fFilesFO4[i].TexChunks) = 0`, on chunk *count*, so a zero-length chunk would be
written and would pass validation. libbsa rejects an empty texture chunk during placement with a
`format_error`.

libbsa keeps the stricter rule: a zero-length mip chunk carries no recoverable texture data, and
accepting one would produce an archive whose records describe content that cannot be extracted. This
divergence is confined to the write path; the reader is unaffected.

BA2 DX10 is now the only family that treats an empty payload specially. TES3 BSA, TES4-family BSA,
and BA2 GNRL all accept one and place it at the write cursor as it stands, matching
`TwbBSArchive.PackData`'s unconditional `Offset := Position`. See issue #37, which aligned GNRL with
that rule.

## Amendment (2026-08-12): the narrowing-key claim did not hold for BA2 DX10

Context fact 1 says a dedupe key is only a bucketing device, so changing the key alone changes no
output. That generalised one family too far.

BA2 DX10's candidate key carried `raw_size`, `packed_size` and `compression` alongside size and
fingerprint (`ba2_dx10_layout.cpp`), and those fields were load-bearing rather than narrowing:
removing them would have permitted a share between byte-equal chunks whose records declare different
decode sizes, leaving one record describing content it cannot produce. For that family the key was a
sharing precondition. The original claim did hold for TES4-family BSA and BA2 GNRL, where differing
compression yields differing stored bytes and `exactly_equals` refuses the share unaided.

`CONTEXT.md` names the missing concept as Sharing Eligibility, kept distinct from the narrowing key.
The Payload Placement module (`src/detail/payload_placement.hpp`) carries the eligibility predicate
that DX10's decode facts belong in. All three sharing families now place through that module.
TES4-family BSA and BA2 GNRL deliberately supply no predicate, because they have no eligibility
constraint to state; BA2 DX10 supplies its three decode facts as one, and its key is now stored size
and fingerprint like theirs.

Fact 1 therefore holds again for every family that shares. It survives not because DX10's constraint
went away but because the constraint moved to where it is visible as a correctness rule instead of
being mistaken for performance narrowing.
