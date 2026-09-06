#Requires -Version 7.0
<#
.SYNOPSIS
Times a full `bsa unpack` of one archive and reports the per-entry cost.

.DESCRIPTION
Maintainer tooling for issue #47/#53. `Measure-CommandOpenCost.ps1` deliberately
measures `unpack --path` with a single entry so that the run isolates archive-open
cost. This script measures the other half: a full extraction of every entry, which
is dominated by per-entry filesystem work rather than by opening.

Per-entry cost is reported as `(full unpack - info) / entry count`. Subtracting the
`info` time removes archive opening and process start/teardown, both of which are
paid once per run and neither of which is per-entry work.

Extraction is measured at several worker counts because the surplus that issue #53
removes is filesystem metadata work that serialises regardless of worker count, so
a change that removes it should move the serial number most.

Retail archives are not redistributable. This script records timings, entry counts
and exit codes only.

.PARAMETER Bsa
Path to the `bsa` executable to measure. Use an optimised build.

.PARAMETER Archive
Path to the archive to extract. Prefer one holding many small entries: that is the
shape where per-entry cost dominates.

.PARAMETER ScratchRoot
Directory receiving the extracted output. Created if missing, and emptied before
every run so no run benefits from what a previous one wrote. Must not be inside the
corpus, and must have room for the whole archive.

.PARAMETER Repeats
Number of timed runs per worker count. The reported elapsed time is the minimum.

.PARAMETER Threads
Worker counts to measure, passed to `unpack -j`. Defaults to serial, the CLI
default, and a high count.

.PARAMETER NoWarmUp
Skip the discarded warm-up run before each timed series.

.PARAMETER OutputCsv
Where to write the result table. Defaults to a file under the current directory
named after the label. Must not be inside the corpus.

.PARAMETER Label
Free-form name for this run, recorded in every row so results from several builds
can be concatenated and compared.

.EXAMPLE
.\tools\perf\Measure-UnpackCost.ps1 -Bsa .\build\windows-msvc-release-static\Release\bsa.exe -Archive '...\Starfield - Misc.ba2' -ScratchRoot D:\scratch\perf53 -Label before
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Bsa,

    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [Parameter(Mandatory = $true)]
    [string]$ScratchRoot,

    [ValidateRange(1, 100)]
    [int]$Repeats = 3,

    [string[]]$Threads = @('1', 'auto', '16'),

    [switch]$NoWarmUp,

    [string]$OutputCsv,

    [string]$Label = 'run'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PerfRunner.ps1')

$bsaPath = (Resolve-Path -LiteralPath $Bsa).Path
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
$corpusPath = Split-Path -Parent $archivePath

if (-not $OutputCsv) { $OutputCsv = Join-Path (Get-Location) "unpack-cost-$Label.csv" }
Assert-OutsideCorpus -Path $OutputCsv -CorpusPath $corpusPath

# The scratch tree is emptied between runs, so refuse one that sits inside the
# read-only corpus before anything is created.
Assert-OutsideCorpus -Path $ScratchRoot -CorpusPath $corpusPath
New-Item -ItemType Directory -Force -Path $ScratchRoot | Out-Null
$ScratchRoot = (Resolve-Path -LiteralPath $ScratchRoot).Path

$stdoutFile = Join-Path $ScratchRoot 'stdout.txt'
$stderrFile = Join-Path $ScratchRoot 'stderr.txt'
$unpackRoot = Join-Path $ScratchRoot 'unpack'
$warmUp = -not $NoWarmUp

# `info` is the open-cost-plus-process-floor baseline subtracted from every
# extraction below. Measured with the same repeat count as the extractions so both
# carry the same minimum-of-N bias. Its stdout also carries the entry count, read
# from the fastest run outside the timed window, so the archive is not opened an
# extra time just to count entries.
$infoRun = Measure-BestBsaRun -BsaPath $bsaPath -BsaArgs @('info', $archivePath) `
    -StdoutFile $stdoutFile -StderrFile $stderrFile -Repeats $Repeats -WarmUp:$warmUp
$openMs = $infoRun.ElapsedMs

$infoFields = ConvertFrom-BsaInfoOutput -Lines $infoRun.Stdout
if (-not $infoFields.ContainsKey('file_count')) {
    throw "Could not read file_count from 'bsa info $archivePath'. The archive may have failed to open: $($infoRun.Stderr -join [System.Environment]::NewLine)"
}
$entryCount = [double]$infoFields['file_count']
if ($entryCount -lt 1) { throw "Archive '$archivePath' holds no entries, so per-entry cost is undefined." }

Write-Host "bsa:     $bsaPath"
Write-Host "archive: $(Split-Path -Leaf $archivePath)"
Write-Host ("entries: {0:N0}   repeats: {1}   warm-up: {2}" -f $entryCount, $Repeats, $warmUp)
Write-Host ("info:    {0,10:N1} ms (subtracted as open cost)" -f $openMs)
Write-Host ''

# Emptying rather than deleting keeps the destination an existing directory for
# every run, so no run is charged for creating the output root while another is not.
$reset = {
    if (Test-Path -LiteralPath $unpackRoot) { Remove-Item -LiteralPath $unpackRoot -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $unpackRoot | Out-Null
}

$results = [System.Collections.Generic.List[object]]::new()
foreach ($threadCount in $Threads) {
    $best = Measure-BestBsaRun -BsaPath $bsaPath `
        -BsaArgs @('unpack', $archivePath, $unpackRoot, '--overwrite', '-j', $threadCount) `
        -StdoutFile $stdoutFile -StderrFile $stderrFile -Repeats $Repeats -WarmUp:$warmUp -Reset $reset

    $perEntryMs = ($best.ElapsedMs - $openMs) / $entryCount
    Write-Host ("-j {0,-6} exit={1} {2,12:N1} ms total {3,8:N3} ms/entry" -f $threadCount, $best.ExitCode, $best.ElapsedMs, $perEntryMs)

    $results.Add([pscustomobject]@{
            label         = $Label
            archive       = Split-Path -Leaf $archivePath
            entry_count   = $entryCount
            threads       = $threadCount
            exit_code     = $best.ExitCode
            open_ms       = [math]::Round($openMs, 2)
            elapsed_ms    = [math]::Round($best.ElapsedMs, 2)
            extract_ms    = [math]::Round($best.ElapsedMs - $openMs, 2)
            per_entry_ms  = [math]::Round($perEntryMs, 4)
        })
}

if (Test-Path -LiteralPath $unpackRoot) { Remove-Item -LiteralPath $unpackRoot -Recurse -Force }
Remove-Item -LiteralPath $stdoutFile, $stderrFile -Force -ErrorAction SilentlyContinue

$results | Export-Csv -LiteralPath $OutputCsv -NoTypeInformation -Encoding utf8
Write-Host ''
Write-Host "wrote $OutputCsv"
$results | Format-Table threads, exit_code, elapsed_ms, extract_ms, per_entry_ms -AutoSize | Out-String -Width 120 | Write-Host
