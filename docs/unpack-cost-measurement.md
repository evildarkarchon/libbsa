# Unpack Cost Measurement

This note records the measured per-entry cost of `bsa unpack`, taken against the local retail archive
corpus on an optimised build. It is the extraction half of issue #47; the archive-opening half is in
[Archive Open Cost Measurement](archive-open-cost-measurement.md). Issue #53 is the work measured here:
`unpack` used to verify destination containment and create the destination directory once per extracted
entry, inside the sink factory, on a worker thread, even though real archives hold tens to thousands of
entries per directory.

Retail archives are not redistributable. This note records entry counts, byte sizes and elapsed times
only. No archive content appears here, and none may be added.

## Summary

| Issue #53 acceptance criterion | Verdict |
| --- | --- |
| Extraction measurably approaches the host filesystem floor | Met — serial per-entry cost fell from 2.46x the plain-create floor to 1.49x |
| Before-and-after per-entry cost recorded | Met — see the table below, and issue #53 |

## How to reproduce

```powershell
cmake --preset windows-msvc-release-static
cmake --build --preset windows-msvc-release-static --target bsa

.\tools\perf\Measure-UnpackCost.ps1 `
    -Bsa .\build\windows-msvc-release-static\Release\bsa.exe `
    -Archive '<corpus>\Starfield - Misc.ba2' `
    -ScratchRoot <scratch> -Repeats 5 -Label after
```

`Measure-UnpackCost.ps1` shares its timed-invocation helpers with the open-cost scripts through
`PerfRunner.ps1`. It refuses a scratch directory that resolves inside the corpus, because it empties that
directory between runs.

Timings are report-only. Nothing in CTest gates on them, in keeping with the existing benchmark policy in
`benchmarks/README.md`.

## Measurement conditions

| | |
| --- | --- |
| Build | `windows-msvc-release-static`, MSVC, optimised |
| Host | Windows 11, 8-core CPU, NVMe storage, on-access scanning active |
| Archive | `Starfield - Misc.ba2`, 4,753 entries, 38 MB, roughly 8 KB per entry |
| Repeats | Minimum of 5 timed runs after one discarded warm-up |
| Baseline ("before") | commit `cc4fc4b0`, the last commit before the unpack pre-pass |

Per-entry cost is `(full unpack - info) / entry count`. Subtracting `bsa info` on the same archive
(34-37 ms across these runs) removes archive opening and process start/teardown, neither of which is
per-entry work. The output tree is deleted and recreated before every run, including the warm-up, so no
run benefits from what a previous one wrote.

An archive of many small entries is the right probe precisely because per-entry cost dominates there. On
an archive of a few large entries the change is invisible, which is the point: the work removed was
proportional to entry count.

## Result: per-entry extraction cost

| `-j` | before (ms/entry) | after (ms/entry) | speedup |
| --- | ---: | ---: | ---: |
| `1` (serial) | 0.537 | 0.326 | 1.65x |
| `auto` (8 workers) | 0.320 | 0.268 | 1.19x |
| `16` | 0.320 | 0.265 | 1.21x |

The serial column is where the change shows, and that is the expected shape. The work removed was
filesystem metadata work that serialises regardless of worker count, so before the change worker threads
were partly absorbing it and partly being blocked by it.

That shows up in what threading is now worth. Before, going from `-j 1` to `-j auto` bought 1.68x; after,
it buys 1.22x. Threading did not get worse — serial got faster, so there is far less left for threads to
recover. **The serialising surplus is essentially gone.**

## Result: distance from the host filesystem floor

Measured on the same host and the same directory shape (4,753 files of 8 KB spread over 32 directories),
with paths and temporary names built outside the timed window so the number is filesystem work rather
than harness work:

| operation | ms/file |
| --- | ---: |
| plain create, write, close | 0.218 |
| create temp, write, close, rename | 0.381 |

Against the plain-create floor, serial extraction went from **2.46x to 1.49x**. The remaining 0.11 ms per
entry over that floor covers reading and decompressing the payload, the destination-exists check, and
atomic publishing — all of which is work the floor harness does not do.

The atomic row is *not* a lower bound for `bsa`, and the serial after figure of 0.326 ms sits below it.
The harness publishes with .NET `File.Move`, a path-based rename; libbsa's CLI publishes with
`SetFileInformationByHandle(FileRenameInfo)` on the handle it already holds, which is cheaper. The row is
kept because it shows atomic publishing costs real time — roughly 0.16 ms per file here — which is why
issue #53 states outright that it is retained rather than traded away for speed.

## What was not measured

- **Antivirus configuration.** On-access scanning inspects every created file, and it is inside every
  number above, including the floor. No code change can move that; it is a host configuration matter and
  issue #47 puts it out of scope.
- **A corpus-wide sweep.** One archive with many small entries is enough to show the effect, and a sweep
  would write millions of files for no additional conclusion. Archives with few entries are dominated by
  archive-open cost, which the sibling note already covers.
