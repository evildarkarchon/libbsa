# ADR-0002: Payload Span Exclusivity is enforced when an archive is opened

- Status: Accepted
- Date: 2026-08-12

## Context

Payload Span Exclusivity says that any two Stored Payload spans in one archive are either identical in
placement or completely disjoint. libbsa enforces it while opening an archive: a record whose payload
span partially overlaps another record's span fails the open with a `format_error`.

The reference performs no such check. `TwbBSArchive.LoadFromFile` reads record fields straight into
its arrays and validates no relationship between them — not against the archive size, not against the
metadata tables, and not against each other. For BA2 GNRL (`wbBSArchive.pas:1150-1159`) each iteration
reads `NameHash`, `Ext`, `DirHash`, `Unknown`, `Offset`, `PackedSize`, `Size` and discards the
`BAADF00D` sentinel. BA2 DX10 (`:1181-1189`) does the same per texture chunk, and TES3 (`:1116-1127`)
reads bare `Size`/`Offset` pairs. A partially overlapping span therefore loads cleanly in BSArchPro and
only misbehaves later, when two entries extract bytes that alias each other's payloads.

So this is a libbsa-added safety property, not a compatibility requirement, and a future reader will
reasonably ask why libbsa rejects archives the reference opens without complaint. That is what makes it
worth recording.

Enforcement was originally written as a scan of every previously accepted span for every record
(BA2 GNRL, BA2 DX10 per chunk, and TES4-family BSA). That is quadratic, and on real archives it
dominated every operation that opens one. Measured on a retail Starfield BA2 with 320,483 entries,
`bsa info` — which prints archive-level metadata and no entries at all — took 141 seconds; a same-sized
4 GiB archive with 2,937 entries took 146 milliseconds. Cost tracked entry count and was independent of
archive bytes. The invariant was never the problem; the way it was checked was.

Two constraints shaped the replacement.

1. **Exact duplicates must stay exempt.** Payload Placement may deliberately share one location between
   records (ADR-0001), so byte-identical spans are legal and only *partial* overlap is not.
2. **Error precedence is observable.** This repository already pins which diagnostic wins when an
   archive has more than one defect: `tes3_bsa_reader_tests.cpp` asserts that unsorted hashes are
   reported before payload overlap. Hoisting the check out of the record loop, as TES3 does, would make
   overlap lose to every per-record check that currently runs after it.

## Decision

Payload Span Exclusivity remains a condition of opening an archive. libbsa does not relax it to match
the reference, and does not defer it to `validate_archive`.

It is enforced incrementally, inside the existing per-record loop, against an ordered collection of
accepted spans. Checking a new span against its immediate predecessor and successor is sufficient: if a
span overlaps any member of a set whose distinct members are already pairwise disjoint, it overlaps one
of those two neighbours. Exact duplicates are exempt, and overlap arithmetic goes through
`detail::spans_overlap_u64` so its saturating end computation is preserved rather than reimplemented.

BA2 GNRL, BA2 DX10 and TES4-family BSA share one helper. TES3 BSA keeps its own post-loop sorted check:
its semantics genuinely differ — it exempts no duplicates — and its error precedence is pinned by an
existing test.

## Consequences

- The set of accepted and rejected archives is unchanged. The check is reformulated, not relaxed, so no
  archive that opened before fails now and none that failed before opens now.
- Error precedence is unchanged, because the check stays interleaved with the bounds, header-intersect,
  name-table-intersect, duplicate-path and hash-mismatch checks exactly where it was.
- Which overlapping pair gets reported may differ, since the neighbour found in offset order need not be
  the first one in record order. This is unobservable: the diagnostics name the archive family and the
  defect, never the offsets or records involved.
- Opening an archive becomes O(n log n) in entries rather than O(n²), which is what makes large retail
  archives usable at all. `bsa info`, `list`, `validate` and `unpack` all benefit, because all four open
  the archive.
- libbsa continues to reject a class of archive BSArchPro accepts. This is a deliberate divergence under
  the `AGENTS.md` allowance for a documented reason to diverge: an aliased payload span means at least
  one entry extracts bytes that are not its own, and failing the open is more useful than returning
  silently wrong content.

## Open question: TES3 exempts no duplicates

TES3's check rejects any overlap, including exact duplicates, while the other three families exempt them
precisely because writer dedupe may produce them. If libbsa's TES3 writer shares payload locations under
ADR-0001, it can emit a TES3 archive its own reader refuses to reopen. This has not been verified and is
recorded here so it is not mistaken for a settled decision.
