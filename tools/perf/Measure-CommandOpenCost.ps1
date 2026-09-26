#Requires -Version 7.0
<#
.SYNOPSIS
Times every `bsa` command that opens an archive against one archive.

.DESCRIPTION
Maintainer tooling for issue #47/#52. `info`, `list`, `validate` and `unpack` all
open the archive before doing their own work, so all four paid the old quadratic
archive-open cost. This script runs each of them against a single archive so the
shared open cost and the command-specific work can be told apart.

`info` is the baseline: it prints archive-level metadata and no entries, so its
elapsed time is essentially the open cost alone. Every other command should land
at that number plus its own work.

`unpack` is measured in two ways. `--path` with a single entry keeps extraction
work to one file, which isolates open cost the same way `info` does. A full
extraction is optionally measured as well, but it is dominated by per-entry
filesystem work rather than by opening, so it is reported separately and is not
evidence about open cost.

Retail archives are not redistributable. This script records timings, entry counts
and exit codes only.

.PARAMETER Bsa
Path to the `bsa` executable to measure. Use an optimised build.

.PARAMETER Archive
Path to the archive to measure.

.PARAMETER Repeats
Number of timed runs per command. The reported elapsed time is the minimum.

.PARAMETER NoWarmUp
Skip the discarded warm-up run before each command's timed runs. Use on a slow
(pre-optimisation) build, where the warm-up costs as much as the measurement.

.PARAMETER ScratchRoot
Directory for redirected output and for unpack output. Created if missing and
cleaned between runs. Must not be inside the corpus, and must have room for a full
extraction if -FullUnpack is used.

.PARAMETER FullUnpack
Also time a full extraction of every entry. Off by default because on a large
archive this writes hundreds of thousands of files.

.PARAMETER Threads
Value passed to `unpack -j`. Left unset the CLI default ("auto") applies, which is
what open-cost measurements should use: open cost does not depend on worker count.

.PARAMETER OutputCsv
Where to write the result table. Defaults to a file under the current directory
named after the label. Must not be inside the corpus.

.PARAMETER Label
Free-form name for this run, recorded in every row so results from several builds
can be concatenated and compared.

.EXAMPLE
.\tools\perf\Measure-CommandOpenCost.ps1 -Bsa .\build\windows-msvc-release-static\Release\bsa.exe -Archive '...\Starfield - MeshesPatch.ba2' -ScratchRoot D:\scratch\perf52 -Label after
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Bsa,

    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [ValidateRange(1, 100)]
    [int]$Repeats = 3,

    [switch]$NoWarmUp,

    [Parameter(Mandatory = $true)]
    [string]$ScratchRoot,

    [switch]$FullUnpack,

    [string]$Threads,

    [string]$OutputCsv,

    [string]$Label = 'run'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PerfRunner.ps1')

$bsaPath = (Resolve-Path -LiteralPath $Bsa).Path
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
$corpusPath = Split-Path -Parent $archivePath

if (-not $OutputCsv) { $OutputCsv = Join-Path (Get-Location) "command-open-cost-$Label.csv" }
Assert-OutsideCorpus -Path $OutputCsv -CorpusPath $corpusPath

# The scratch tree is deleted between runs, so refuse one that sits inside the
# read-only corpus before anything is created.
Assert-OutsideCorpus -Path $ScratchRoot -CorpusPath $corpusPath
New-Item -ItemType Directory -Force -Path $ScratchRoot | Out-Null
$ScratchRoot = (Resolve-Path -LiteralPath $ScratchRoot).Path

$stdoutFile = Join-Path $ScratchRoot 'stdout.txt'
$stderrFile = Join-Path $ScratchRoot 'stderr.txt'
$warmUp = -not $NoWarmUp

function Measure-OneCommand {
    <#
    .SYNOPSIS
    Times one subcommand and prints a progress line.
    #>
    param([string]$Name, [string[]]$BsaArgs, [scriptblock]$Reset)

    $measureArgs = @{
        BsaPath    = $bsaPath
        BsaArgs    = $BsaArgs
        StdoutFile = $stdoutFile
        StderrFile = $stderrFile
        Repeats    = $Repeats
        WarmUp     = $warmUp
    }
    if ($Reset) { $measureArgs['Reset'] = $Reset }
    $best = Measure-BestBsaRun @measureArgs

    Write-Host ("{0,-24} exit={1} {2,12:N1} ms" -f $Name, $best.ExitCode, $best.ElapsedMs)
    return [pscustomobject]@{
        label      = $Label
        archive    = Split-Path -Leaf $archivePath
        command    = $Name
        exit_code  = $best.ExitCode
        elapsed_ms = [math]::Round($best.ElapsedMs, 2)
    }
}

# Entry count is read outside every timed window.
Invoke-TimedBsa -BsaPath $bsaPath -BsaArgs @('info', $archivePath) -StdoutFile $stdoutFile -StderrFile $stderrFile | Out-Null
$infoFields = ConvertFrom-BsaInfoOutput -Lines @(Get-Content -LiteralPath $stdoutFile -ErrorAction SilentlyContinue)
if (-not $infoFields.ContainsKey('file_count')) {
    throw "Could not read file_count from 'bsa info $archivePath'. The archive may have failed to open: $(Get-Content -LiteralPath $stderrFile -Raw -ErrorAction SilentlyContinue)"
}
$entryCount = $infoFields['file_count']

Write-Host "bsa:     $bsaPath"
Write-Host "archive: $(Split-Path -Leaf $archivePath)"
Write-Host "entries: $entryCount   repeats: $Repeats   warm-up: $warmUp"
Write-Host ''

$results = [System.Collections.Generic.List[object]]::new()

$results.Add((Measure-OneCommand -Name 'info' -BsaArgs @('info', $archivePath)))
$results.Add((Measure-OneCommand -Name 'list' -BsaArgs @('list', $archivePath)))
$results.Add((Measure-OneCommand -Name 'validate' -BsaArgs @('validate', $archivePath)))

# One entry chosen from the head of the listing. Which entry it is does not matter
# for open cost; only that exactly one is extracted, so the run is open cost plus
# one file's worth of extraction.
#
# `bsa list` prints entry paths through the CLI's escaping helper, so a path
# holding a control byte would come back as `\xHH` text that `--path` could never
# match. Scan down the listing for the first path that survives the round trip
# unescaped rather than assuming the first line is usable.
Invoke-TimedBsa -BsaPath $bsaPath -BsaArgs @('list', $archivePath) -StdoutFile $stdoutFile -StderrFile $stderrFile | Out-Null
$singlePath = $null
# An explicit reader that is disposed in `finally`, rather than
# [System.IO.File]::ReadLines: that returns a lazy enumerator, and breaking out of
# the loop early leaves its handle open, so the next run's redirect into this same
# file fails with a sharing violation.
$reader = [System.IO.File]::OpenText($stdoutFile)
try {
    while ($null -ne ($line = $reader.ReadLine())) {
        if ($line -and $line -notmatch '\\x[0-9A-F]{2}|\\[nrt]') { $singlePath = $line; break }
    }
}
finally {
    $reader.Dispose()
}
if (-not $singlePath) { throw "No entry path in '$archivePath' survives CLI escaping, so --path cannot be measured." }

$unpackRoot = Join-Path $ScratchRoot 'unpack'
$reset = { if (Test-Path -LiteralPath $unpackRoot) { Remove-Item -LiteralPath $unpackRoot -Recurse -Force } }

$unpackArgs = @('unpack', $archivePath, $unpackRoot, '--overwrite', '--path', $singlePath)
if ($Threads) { $unpackArgs += @('-j', $Threads) }
$results.Add((Measure-OneCommand -Name 'unpack --path (1 entry)' -BsaArgs $unpackArgs -Reset $reset))

if ($FullUnpack) {
    $fullArgs = @('unpack', $archivePath, $unpackRoot, '--overwrite')
    if ($Threads) { $fullArgs += @('-j', $Threads) }
    $results.Add((Measure-OneCommand -Name 'unpack (all entries)' -BsaArgs $fullArgs -Reset $reset))
}

& $reset
Remove-Item -LiteralPath $stdoutFile, $stderrFile -Force -ErrorAction SilentlyContinue

$results | Export-Csv -LiteralPath $OutputCsv -NoTypeInformation -Encoding utf8
Write-Host ''
Write-Host "wrote $OutputCsv"
$results | Format-Table command, exit_code, elapsed_ms -AutoSize | Out-String -Width 120 | Write-Host
