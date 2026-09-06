#Requires -Version 7.0
<#
.SYNOPSIS
Measures how long `bsa` takes to open each archive in the local retail corpus.

.DESCRIPTION
Maintainer tooling for issue #47/#52. Archive opening used to be quadratic in
entry count, so the cost of every command that opens an archive tracked entry
count rather than archive bytes. This script sweeps a corpus directory, runs one
`bsa` subcommand per archive, and records the entry count alongside the elapsed
wall-clock time so the two can be correlated.

`bsa info` is the cleanest probe of open cost: it prints archive-level metadata
and no entries at all, so essentially all of its runtime is archive opening.
`list` and `validate` open the archive too and can be swept the same way.

The script also captures the exit code and the diagnostic text for archives that
fail to open, so that two builds can be compared for compatibility as well as for
speed: the set of archives accepted and rejected, and the wording of each
rejection, must be identical across builds.

Only read-only subcommands are swept. `unpack` is measured by
`Measure-CommandOpenCost.ps1` against a single archive instead, so that this
script never needs an archive-output destination. Harness scratch files and the
result CSV are its only writes, and both must remain outside the corpus.

Retail archives are not redistributable. This script records file names, byte
sizes, entry counts, header metadata and timings only. It never reads or emits
archive payload content.

.PARAMETER Bsa
Path to the `bsa` executable to measure. Use an optimised build; a Debug build
measures the debug allocator more than it measures the parser.

.PARAMETER Corpus
Directory holding the archives. Defaults to $env:LIBBSA_GAME_FIXTURES when set,
otherwise to tests/fixtures/local relative to the repository root.

.PARAMETER Command
Which read-only subcommand to time: info, list or validate.

.PARAMETER Repeats
Number of timed runs per archive. The reported elapsed time is the minimum across
runs. Use 1 when measuring a slow (pre-optimisation) build.

.PARAMETER WarmUp
Run each archive once before the timed runs so the OS file cache state is the same
for every measurement. Skip this on a slow build, where the warm-up costs as much
as the measurement.

.PARAMETER Filter
Optional wildcard applied to archive file names, e.g. 'Starfield*'.

.PARAMETER OutputCsv
Where to write the per-archive result table. Defaults to a file under the current
directory named after the label and command. Must not be inside the corpus.

.PARAMETER Label
Free-form name for this measurement run, recorded in every row so that results
from several builds can be concatenated and compared.

.PARAMETER ScratchRoot
Directory for the redirected stdout and stderr files. Defaults to a temporary
directory. Must not be inside the corpus.

.EXAMPLE
.\tools\perf\Measure-ArchiveOpenCost.ps1 -Bsa .\build\windows-msvc-release-static\Release\bsa.exe -Label after

Times `bsa info` over the whole corpus and writes archive-open-cost-after-info.csv.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Bsa,

    [string]$Corpus,

    [ValidateSet('info', 'list', 'validate')]
    [string]$Command = 'info',

    [ValidateRange(1, 100)]
    [int]$Repeats = 3,

    [switch]$WarmUp,

    [string]$Filter = '*',

    [string]$OutputCsv,

    [string]$Label = 'run',

    [string]$ScratchRoot
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PerfRunner.ps1')

$corpusPath = Resolve-CorpusPath -Explicit $Corpus
$bsaPath = (Resolve-Path -LiteralPath $Bsa).Path

if (-not $OutputCsv) { $OutputCsv = Join-Path (Get-Location) "archive-open-cost-$Label-$Command.csv" }
Assert-OutsideCorpus -Path $OutputCsv -CorpusPath $corpusPath

if (-not $ScratchRoot) { $ScratchRoot = Join-Path ([System.IO.Path]::GetTempPath()) "libbsa-perf-$Label-$Command" }
Assert-OutsideCorpus -Path $ScratchRoot -CorpusPath $corpusPath
New-Item -ItemType Directory -Force -Path $ScratchRoot | Out-Null
$ScratchRoot = (Resolve-Path -LiteralPath $ScratchRoot).Path
$stdoutFile = Join-Path $ScratchRoot 'stdout.txt'
$stderrFile = Join-Path $ScratchRoot 'stderr.txt'

$archives = Get-ChildItem -LiteralPath $corpusPath -Recurse -File -Filter $Filter |
    Where-Object { $_.Extension -in @('.bsa', '.ba2') } |
    Sort-Object Length -Descending

if (-not $archives) { throw "No .bsa/.ba2 archives matched '$Filter' under '$corpusPath'." }

Write-Host "corpus:  $corpusPath"
Write-Host "bsa:     $bsaPath"
Write-Host "command: $Command   repeats: $Repeats   label: $Label"
Write-Host "archives: $($archives.Count)"
Write-Host ''

$results = [System.Collections.Generic.List[object]]::new()
$index = 0

foreach ($archive in $archives) {
    $index++
    $bsaArgs = @($Command, $archive.FullName)

    $measureArgs = @{
        BsaPath    = $bsaPath
        BsaArgs    = $bsaArgs
        StdoutFile = $stdoutFile
        StderrFile = $stderrFile
        Repeats    = $Repeats
        WarmUp     = $WarmUp
    }
    $best = Measure-BestBsaRun @measureArgs

    # Entry count comes from `bsa info` regardless of which command was timed, so
    # that every row can be correlated against entry count. For anything but
    # `info` that is an extra call, made outside the timed window.
    $infoFields = if ($Command -eq 'info') {
        ConvertFrom-BsaInfoOutput -Lines $best.Stdout
    }
    else {
        $info = Invoke-TimedBsa -BsaPath $bsaPath -BsaArgs @('info', $archive.FullName) `
            -StdoutFile $stdoutFile -StderrFile $stderrFile
        ConvertFrom-BsaInfoOutput -Lines @(Get-Content -LiteralPath $stdoutFile -ErrorAction SilentlyContinue)
    }

    $entryCount = if ($infoFields.ContainsKey('file_count')) { [int64]$infoFields['file_count'] } else { $null }

    $results.Add([pscustomobject]@{
            label       = $Label
            command     = $Command
            archive     = $archive.Name
            size_bytes  = $archive.Length
            entry_count = $entryCount
            type        = $infoFields['type']
            variant     = $infoFields['variant']
            version     = $infoFields['version']
            exit_code   = $best.ExitCode
            elapsed_ms  = [math]::Round($best.ElapsedMs, 2)
            # Rejection wording is the compatibility signal, so keep the whole
            # stderr text verbatim; it is diagnostic text, never archive content.
            diagnostic  = ($best.Stderr -join ' | ')
        })

    $shown = if ($null -ne $entryCount) { $entryCount } else { 'n/a' }
    Write-Host ("[{0,3}/{1}] {2,-45} entries={3,-8} exit={4} {5,10:N1} ms" -f
        $index, $archives.Count, $archive.Name, $shown, $best.ExitCode, $best.ElapsedMs)
}

$results | Export-Csv -LiteralPath $OutputCsv -NoTypeInformation -Encoding utf8
Remove-Item -LiteralPath $stdoutFile, $stderrFile -Force -ErrorAction SilentlyContinue

Write-Host ''
Write-Host "wrote $OutputCsv"

$opened = @($results | Where-Object { $_.exit_code -eq 0 })
$rejected = @($results | Where-Object { $_.exit_code -ne 0 })
$slowest = $results | Sort-Object elapsed_ms -Descending | Select-Object -First 1

Write-Host ("opened: {0}   rejected: {1}" -f $opened.Count, $rejected.Count)
if ($slowest) {
    Write-Host ("slowest: {0} ({1} entries) {2:N1} ms" -f $slowest.archive, $slowest.entry_count, $slowest.elapsed_ms)
}
