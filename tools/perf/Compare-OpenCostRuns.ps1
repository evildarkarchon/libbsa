#Requires -Version 7.0
<#
.SYNOPSIS
Compares two Measure-ArchiveOpenCost.ps1 result files for speed and compatibility.

.DESCRIPTION
Maintainer tooling for issue #47/#52. Takes the CSV produced by
`Measure-ArchiveOpenCost.ps1` on two builds and reports:

- the speedup per archive and overall, and
- whether the two builds agree on which archives open and which are rejected,
  and on the exact wording of every rejection.

The compatibility half is the point of the comparison. A performance change must
not quietly move an archive between the accepted and rejected sets, and must not
change the diagnostic a rejected archive produces. Any disagreement is reported
as a mismatch and makes the script exit non-zero.

The compared diagnostic text has its archive-path context prefix stripped. The two
builds normally read the same corpus and so produce the same prefix, but a build
may be measured against a corpus reached by another path — a different drive
letter, or LIBBSA_GAME_FIXTURES pointing elsewhere — and that difference says
nothing about compatibility. The error code and message are still compared
verbatim.

.PARAMETER Before
CSV from the baseline build.

.PARAMETER After
CSV from the build under test.

.EXAMPLE
.\tools\perf\Compare-OpenCostRuns.ps1 -Before before.csv -After after.csv
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Before,

    [Parameter(Mandatory = $true)]
    [string]$After
)

$ErrorActionPreference = 'Stop'

# `error (D:\some\where\Foo.ba2): format_error: ...` can differ between two runs
# only in the path when the corpus is reached by a different route, so compare the
# part after the context prefix.
function Get-ComparableDiagnostic {
    param([string]$Text)

    if (-not $Text) { return '' }
    return ($Text -replace '^error \([^)]*\): ', 'error: ').Trim()
}

$beforeRows = @{}
foreach ($row in Import-Csv -LiteralPath $Before) { $beforeRows[$row.archive] = $row }

$afterRows = @{}
foreach ($row in Import-Csv -LiteralPath $After) { $afterRows[$row.archive] = $row }

$onlyBefore = @($beforeRows.Keys | Where-Object { -not $afterRows.ContainsKey($_) })
$onlyAfter = @($afterRows.Keys | Where-Object { -not $beforeRows.ContainsKey($_) })
foreach ($name in $onlyBefore) { Write-Warning "present only in Before: $name" }
foreach ($name in $onlyAfter) { Write-Warning "present only in After:  $name" }

$comparisons = [System.Collections.Generic.List[object]]::new()
foreach ($name in ($beforeRows.Keys | Where-Object { $afterRows.ContainsKey($_) } | Sort-Object)) {
    $b = $beforeRows[$name]
    $a = $afterRows[$name]

    $outcomeMatch = ($b.exit_code -eq $a.exit_code)
    $diagnosticMatch = ((Get-ComparableDiagnostic $b.diagnostic) -eq (Get-ComparableDiagnostic $a.diagnostic))
    $beforeMs = [double]$b.elapsed_ms
    $afterMs = [double]$a.elapsed_ms

    $comparisons.Add([pscustomobject]@{
            archive          = $name
            entry_count      = if ($a.entry_count) { [int64]$a.entry_count } else { $null }
            before_ms        = $beforeMs
            after_ms         = $afterMs
            speedup          = if ($afterMs -gt 0) { [math]::Round($beforeMs / $afterMs, 2) } else { $null }
            before_exit      = [int]$b.exit_code
            after_exit       = [int]$a.exit_code
            outcome_match    = $outcomeMatch
            diagnostic_match = $diagnosticMatch
        })
}

$mismatches = @($comparisons | Where-Object { -not $_.outcome_match -or -not $_.diagnostic_match })

$openedBefore = @($comparisons | Where-Object { $_.before_exit -eq 0 })
$openedAfter = @($comparisons | Where-Object { $_.after_exit -eq 0 })

Write-Host ''
Write-Host ("archives compared:   {0}" -f $comparisons.Count)
Write-Host ("opened  before/after: {0} / {1}" -f $openedBefore.Count, $openedAfter.Count)
Write-Host ("rejected before/after: {0} / {1}" -f ($comparisons.Count - $openedBefore.Count), ($comparisons.Count - $openedAfter.Count))
Write-Host ("compatibility mismatches: {0}" -f $mismatches.Count)
Write-Host ''

Write-Host 'slowest 15 archives after, with before/after and speedup:'
$comparisons | Sort-Object after_ms -Descending | Select-Object -First 15 |
    Format-Table archive, entry_count, before_ms, after_ms, speedup -AutoSize |
    Out-String -Width 140 | Write-Host

$totalBefore = ($comparisons | Measure-Object before_ms -Sum).Sum
$totalAfter = ($comparisons | Measure-Object after_ms -Sum).Sum
if ($totalAfter -gt 0) {
    Write-Host ("whole-corpus total: {0:N1} ms -> {1:N1} ms  ({2:N2}x)" -f $totalBefore, $totalAfter, ($totalBefore / $totalAfter))
}
else {
    Write-Host ("whole-corpus total: {0:N1} ms -> {1:N1} ms" -f $totalBefore, $totalAfter)
}

if ($mismatches.Count -gt 0) {
    Write-Host ''
    Write-Host 'COMPATIBILITY MISMATCHES:' -ForegroundColor Red
    $mismatches | Format-List archive, before_exit, after_exit, outcome_match, diagnostic_match |
        Out-String -Width 200 | Write-Host
    exit 1
}

Write-Host 'compatibility: identical accept/reject sets and identical diagnostics.'
