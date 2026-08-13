# Archive Open Cost Measurement

This note records the measured cost of opening an archive, taken against the local retail archive
corpus on an optimised build. It exists because archive opening used to be quadratic in entry count
(issue #47): every command that opened an archive paid a cost that tracked entry count and was
independent of archive bytes, so the archives that matter most for compatibility work were the ones
least usable. Issue #52 is the measurement half of that work.

Retail archives are not redistributable. This note records entry counts, byte sizes, header metadata,
diagnostics and elapsed times only. No archive content appears here, and none may be added.

## Summary

| Issue #52 acceptance criterion | Verdict |
| --- | --- |
| `bsa info` on the largest archive returns in under a second | **Not met** — 1,548 ms, down from 124,838 ms |
| Open cost grows no worse than O(n log n) in entry count | Met — fitted exponent 1.94 before, 1.07 after |
| `list`, `validate` and `unpack` show the same improvement | Met — see the per-command table |
| Accept/reject set and diagnostics unchanged | Met — 92 open / 9 rejected on both builds, 0 mismatches |
| Measured with the worker-count flag at its default | Met, and open cost confirmed independent of worker count |
| Counts and timings only, no archive bytes | Met |

The one unmet criterion is a scale problem, not a correctness one, and the measurements show it is not
caused by the code this work changed. See [Where the remaining cost is](#where-the-remaining-cost-is).

## How to reproduce

Four scripts under `tools/perf/` produce every number below. They are maintainer tooling, not part of
the build or the default test sweep. `PerfRunner.ps1` holds the shared timed-invocation helpers; the
other three are entry points.

```powershell
cmake --preset windows-msvc-release-static
cmake --build --preset windows-msvc-release-static --target bsa

# Per-archive open cost across the whole corpus.
.\tools\perf\Measure-ArchiveOpenCost.ps1 `
    -Bsa .\build\windows-msvc-release-static\Release\bsa.exe `
    -Command info -Repeats 3 -WarmUp -Label after -OutputCsv after.csv

# Every command that opens an archive, against one archive.
.\tools\perf\Measure-CommandOpenCost.ps1 `
    -Bsa .\build\windows-msvc-release-static\Release\bsa.exe `
    -Archive '<corpus>\Starfield - MeshesPatch.ba2' -ScratchRoot <scratch> -Label after

# Speed and compatibility comparison between two builds.
.\tools\perf\Compare-OpenCostRuns.ps1 -Before before.csv -After after.csv
```

The corpus is resolved from `LIBBSA_GAME_FIXTURES` when set, otherwise from `tests/fixtures/local/`.
Measurements must use an optimised build: a Debug build measures the debug allocator more than it
measures the parser.

The scripts only ever read the corpus. `Measure-ArchiveOpenCost.ps1` runs read-only subcommands and has
no writable destination at all; `Measure-CommandOpenCost.ps1` needs a scratch directory for extraction
and refuses one that resolves inside the corpus, because it deletes that directory between runs.

Timings are report-only. Nothing in CTest gates on them, in keeping with the existing benchmark policy
in `benchmarks/README.md`.

## Why `bsa info` is the probe

`bsa info` prints archive-level metadata and no entries at all — it reads `file_count` from the header
and never enumerates the catalog — so essentially all of its runtime is archive opening. `list`,
`validate` and `unpack` all open the archive first and then do their own work on top, so measuring
`info` isolates the shared cost and measuring the others shows what each adds.

## Measurement conditions

| | |
| --- | --- |
| Build | `windows-msvc-release-static`, MSVC, optimised |
| Host | Windows 11, 8-core CPU, NVMe storage |
| Corpus | 101 retail archives, 1.7 MiB to 4.00 GiB, 1 to 354,341 entries |
| Baseline ("before") | commit `71c34182`, the last commit before Payload Span Exclusivity enforcement was replaced |
| Under test ("after") | commit `d17924fd`, all three families migrated to the shared enforcement module |
| Worker count | CLI default (`auto`) throughout |

Of the 92 archives that open, the family spread is BA2 Fallout 4 v1/v7/v8 (1/4/11), BA2 Starfield v2/v3
(35/13), and TES4-family BSA v103/v104/v105 (6/5/17). The 9 that do not open are covered under
compatibility below.

Repeat counts differ between the two builds, and this bounds what the numbers can support:

- **After**: 3 timed runs after one discarded warm-up, minimum reported.
- **Before**: 1 timed run, no warm-up, because a single corpus sweep on the quadratic build already
  costs about six minutes and each run of the largest archive costs over two minutes.

So baseline figures for small archives carry real noise. Two independent baseline sweeps recorded the
same 2,937-entry archive at 313.8 ms and 118.8 ms. Only the large-*n* end of the before column, where the
quadratic term dwarfs the noise, should be read as precise.

Elapsed times are whole-process wall clock, so each includes process start and teardown. That floor
measures 14.4 ms on the after build and 16.4 ms on the baseline, taken as the fastest run over the
archives holding 10 or fewer entries, and it is subtracted where per-entry costs and growth exponents are
derived.

One after-build reading is a known host artifact and is excluded from the fits: a 315-entry archive
recorded 281.0 ms during the sweep and 16.8 ms when re-measured immediately afterwards with 5 repeats.
Nothing else in either sweep shows that signature.

## Result: open cost no longer tracks entry count quadratically

`bsa info` over the whole 101-archive corpus went from **341.3 s to 11.6 s**, a 29.3x reduction. The
speedup is not uniform, and that is the point: it is nil for small archives and grows with entry count,
which is the signature of removing a quadratic term.

| entries | before (ms) | after (ms) | speedup |
| ---: | ---: | ---: | ---: |
| 1 | 19.9 | 16.0 | 1.2x |
| 53 | 20.5 | 17.6 | 1.2x |
| 426 | 21.1 | 16.0 | 1.3x |
| 29,716 | 969.3 | 146.8 | 6.6x |
| 48,535 | 2,432.1 | 215.5 | 11.3x |
| 86,290 | 7,903.9 | 367.2 | 21.5x |
| 158,843 | 25,454.0 | 663.7 | 38.4x |
| 320,483 | 99,877.8 | 1,451.1 | 68.8x |
| 354,341 | 124,838.2 | 1,547.8 | 80.7x |

Rows between roughly 1,000 and 30,000 entries are omitted because the single-run baseline noise described
above is the same size as the effect there. Where several archives share an entry count — the corpus holds
four single-entry archives — the row shows one of them; at that size every one of them is at the process
floor and the choice does not matter.

The 320,483-entry archive is the one issue #47 reported at about 141 s. It measures 99.9 s here on the
same baseline code; the difference is host state, not a different archive, and does not affect the
conclusion.

Archive bytes remain irrelevant to open cost, before and after. A 4.0 GiB archive holding 2,937 entries
opens in 40 ms, while a 3.6 GiB archive holding 354,341 entries takes 1,548 ms.

## Result: growth in entry count

Fitting `log(elapsed - process floor)` against `log(entry count)` gives the effective growth exponent.
For reference, 1.00 is linear and 2.00 is quadratic. O(n log n) is not a straight line on these axes, so
its effective slope depends on the range fitted: 1.105 over n >= 1,000, 1.092 over n >= 10,000, and 1.086
over n >= 40,000. Each after-column figure below should be read against the O(n log n) slope for its own
range.

| fit range | before | after |
| --- | ---: | ---: |
| n >= 1,000 (68 archives) | 1.50 (R² 0.895) | 0.88 (R² 0.901) |
| n >= 10,000 (29 archives) | 1.88 (R² 0.994) | 0.95 (R² 0.833) |
| n >= 40,000 (16 archives) | 1.94 (R² 0.988) | 1.07 (R² 0.802) |

The before column converges on 2.0 as the quadratic term comes to dominate the constant, confirming the
defect. The after column stays below the O(n log n) slope for every range fitted, and at the top of the
range where the measurement is cleanest it is 1.07 against O(n log n)'s 1.086, so **measured growth is no
worse than O(n log n)**.

The after column's lower R² is itself informative. Once the quadratic term is gone, what is left is a
per-entry constant that varies by a factor of four depending on what an archive holds, so entry count
alone no longer predicts elapsed time closely. That variation is the subject of the last section.

Per-family exponents are **not** reported. Each family's subset is too small and too skewed by content
type to fit meaningfully — the TES4-family subset in particular is bimodal, and its largest members are
also its cheapest per entry, which produces an exponent far below 1 that reflects the sample, not the
algorithm. The pooled fits above are the ones that carry weight.

## Result: every command that opens an archive inherited the improvement

All four commands measured against the same archive, `Starfield - MeshesPatch.ba2` (354,341 entries,
BA2 GNRL), with `-j` left at its default:

| command | before (ms) | after (ms) | speedup | after, over `info` |
| --- | ---: | ---: | ---: | ---: |
| `info` | 123,871 | 1,622 | 76.4x | — |
| `list` | 123,527 | 2,371 | 52.1x | +749 ms |
| `validate` | 121,977 | 1,754 | 69.5x | +133 ms |
| `unpack --path` (1 entry) | 124,302 | 1,601 | 77.7x | -21 ms |

Every command improved by a similar large factor, and after the change every one of them sits at the
`info` number plus its own work: `list` adds the cost of printing 354,341 lines and `validate` adds its
checks. `unpack --path` lands 21 ms *below* `info`, which is within the run-to-run spread of a 1.6 s
measurement — the honest reading is that extracting one entry costs nothing measurable next to opening
the archive. **No command is left slow merely because it opens an archive.**

Before the change, all four sat within 2% of each other at roughly 123 s, because open cost so completely
dominated that the commands' own work was invisible. After the change that work becomes visible, which is
why `list` now stands out: it is not slow, it simply has the most of its own work to do.

`unpack` is measured with `--path` naming a single entry so that the run is open cost plus one
extraction. A full extraction of this archive is dominated by per-entry filesystem work rather than by
opening, so it measures something else; that is the subject of issue #53 and is not evidence about open
cost. Those extraction measurements live in
[Unpack Cost Measurement](unpack-cost-measurement.md).

## Result: open cost does not depend on worker count

Issue #47 notes that the `-j/--threads` flag could never work around this cost, because the cost is in
archive opening and no worker count touches it. Measured on the 354,341-entry archive via
`unpack --path` with a single entry, so the run is open cost plus one extraction:

| `-j` | elapsed (ms) |
| --- | ---: |
| `auto` (default) | 1,619.4 |
| `1` | 1,559.7 |
| `16` | 1,563.5 |

The spread is under 4%, within run-to-run noise, on an 8-core host — and `-j 1` came out marginally
*faster* than 16 workers, which is what "no dependence" looks like. Open cost is flat across worker
count, so leaving the flag at its default is the correct way to measure it.

Reproduce with `-Threads auto|1|16` on `Measure-CommandOpenCost.ps1`.

## Result: compatibility is unchanged

Across all 101 archives, the two builds agree exactly:

- 92 archives opened before, and the same 92 open after.
- 9 archives were rejected before, and the same 9 are rejected after.
- Every rejection produces the identical diagnostic on both builds.

The comparison strips the archive-path context prefix from each diagnostic before comparing, so that a
build measured against a corpus reached by a different path is not reported as a compatibility change.
The error code and message text are compared verbatim. On these runs both builds read the same corpus, so
the prefixes matched too.

The 9 rejections are pre-existing divergences from BSArchPro, unrelated to this work, and are recorded
here so a future run can tell a genuine regression from the status quo:

| rejected archives | diagnostic |
| ---: | --- |
| 3 | `format_error: DDS layout has unsupported DXGI format` |
| 4 | `format_error: DDS layout chunk raw byte total does not match mip range` |
| 1 | `format_error: TES4 BSA file name lengths do not match header total` |
| 1 | `format_error: TES3 BSA stored hash does not match parsed name` |

The TES3 row was also resolved after this measurement was taken. Issue #46 showed the archive is well
formed and libbsa read the hash field wrongly: a TES3 hash record stores the two half-sums as consecutive
`u32` values, first-half sum first, and libbsa read the eight bytes as one little-endian `u64`, which
transposes the halves relative to `hash_tes3`. All 11090 records of vanilla `Morrowind.bsa` match the
corrected composition and none matched the old one, so the archive was rejected on its very first record.
It now opens. The reference's writer already emitted this order (`wbBSArchive.pas:1613-1616`); only its read
path disagreed with its own writer.

The TES4 row was resolved after this measurement was taken. Issue #45 showed the archive is well formed:
it declares a file-name table 105 bytes longer than its names consume, which the reference never checks.
libbsa now accepts it and reports `bsa_file_name_table_trailing_bytes` instead. That was confirmed by
opening every `.bsa` in the corpus, all 29 TES4-family archives of which now open; the BA2 rows were not
re-measured, so the totals above have not been re-derived. The table is left as recorded so the
comparison it documents stays reproducible.

## Where the remaining cost is

One acceptance criterion from issue #52 is **not met**: `bsa info` on the largest archive in the corpus
takes 1,548 ms, not under a second. The 320,483-entry archive named in issue #47 takes 1,451 ms.

The measurements say what the residual is not. It is not superlinear — the fitted exponent is 1.07 at the
top of the range — so it is not Payload Span Exclusivity enforcement, which is the only term this work
changed and the only one that was superlinear.

What it is tracks per-entry data rather than entry count. Take the nine archives holding between 13,000
and 21,000 entries, a range narrow enough that entry count is effectively held constant:

| archive | family | entries | us/entry |
| --- | --- | ---: | ---: |
| `Skyrim - Misc.bsa` | TES4 | 14,032 | 1.38 |
| `Starfield - LODMeshesPatch.ba2` | BA2 | 20,405 | 4.30 |
| `Starfield - FaceAnimationPatch.ba2` | BA2 | 20,896 | 4.35 |
| `Oblivion - Meshes.bsa` | TES4 | 20,182 | 4.39 |
| `Starfield - LODMeshes.ba2` | BA2 | 20,400 | 4.49 |
| `Skyrim - Meshes0.bsa` | TES4 | 19,443 | 5.34 |
| `Fallout - Meshes.bsa` | TES4 | 19,587 | 5.37 |
| `Oblivion - Textures - Compressed.bsa` | TES4 | 18,040 | 5.60 |
| `Skyrim - Meshes1.bsa` | TES4 | 14,242 | 5.93 |

At essentially the same entry count, per-entry cost varies **4.3x**, and it does not split by archive
family: TES4-family archives occupy both the cheapest and the most expensive rows. Ranked across the
corpus, the cheapest per-entry archives after `Skyrim - Misc.bsa` are the voice and sound archives, whose
entries have short, repetitive paths. Across the 68 opened archives with at least 1,000 entries, the
per-entry range is 3.65-10.39 us for Starfield BA2 (median 4.36, 35 archives), 4.02-8.30 us for
Fallout 4 BA2 (median 4.59, 12 archives), and 1.38-8.34 us for TES4-family BSA (median 5.37, 21
archives) — overlapping ranges, so family is not the variable.

Cost that scales with the size and shape of each entry's name, and not with how many entries there are,
is the signature of Archive Entry Catalog materialisation: per-name `std::string` construction, canonical
path construction, and the `std::unordered_set<std::string>` insert that rejects duplicate canonical
paths. Span-exclusivity enforcement would show the opposite signature, since a span is a fixed-size
`(offset, size)` pair regardless of how long the entry's name is.

Issue #47 deliberately deferred lazy Archive Entry Catalog materialisation as out of scope, predicting it
would be "a few hundred milliseconds at worst" once enforcement was fixed. Measured, it is about 1.5 s on
the largest archive. `bsa info` never enumerates the catalog — it reads `file_count` from the header — so
that whole cost is currently paid for nothing on the cleanest probe. Meeting the sub-second criterion
needs that deferred item, and this measurement is the justification issue #47 asked for before revisiting
it.
