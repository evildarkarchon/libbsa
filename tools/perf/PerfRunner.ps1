#Requires -Version 7.0
<#
.SYNOPSIS
Shared timed-invocation helpers for the tools/perf measurement scripts.

.DESCRIPTION
Maintainer tooling for issue #47/#52. The measurement scripts need the same
thing: run `bsa` once, time only the process, and keep the harness out of the
measurement. That shape lived in both scripts and drifted — one captured stdout
into a PowerShell collection inside the stopwatch while the other redirected to a
file, so a `list` over 350,000 entries was charged for harness work in one script
and not the other. It lives here once so the two cannot diverge again.

Dot-source this file; it defines functions and does nothing on its own.
#>

# Timing only the process is the whole point, so both streams go to files rather
# than into PowerShell objects. Collecting 350,000 stdout lines into a
# List[string] inside the stopwatch measures the harness, not the command.
#
# PowerShell 7.4 can be configured to turn a non-zero native exit code into a
# terminating error via $PSNativeCommandUseErrorActionPreference. These
# measurements deliberately include archives that fail to open, so that behavior
# is suppressed for the duration of the call and restored afterwards.
function Invoke-TimedBsa {
    <#
    .SYNOPSIS
    Runs one `bsa` invocation and returns its exit code and elapsed milliseconds.

    .PARAMETER BsaPath
    Fully resolved path to the executable. Resolve once at the caller so the
    measurement is not charged for path resolution.

    .PARAMETER BsaArgs
    Argument array. Passed with splatting so archive paths containing spaces are
    quoted correctly; Start-Process would not do this.

    .PARAMETER StdoutFile
    File receiving stdout. Overwritten on every call.

    .PARAMETER StderrFile
    File receiving stderr. Overwritten on every call.
    #>
    param(
        [Parameter(Mandatory = $true)][string]$BsaPath,
        [Parameter(Mandatory = $true)][string[]]$BsaArgs,
        [Parameter(Mandatory = $true)][string]$StdoutFile,
        [Parameter(Mandatory = $true)][string]$StderrFile
    )

    $previousErrorAction = $ErrorActionPreference
    $previousNativePreference = $PSNativeCommandUseErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $PSNativeCommandUseErrorActionPreference = $false
    try {
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        & $BsaPath @BsaArgs > $StdoutFile 2> $StderrFile
        $stopwatch.Stop()
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorAction
        $PSNativeCommandUseErrorActionPreference = $previousNativePreference
    }

    return [pscustomobject]@{
        ExitCode  = $exitCode
        ElapsedMs = $stopwatch.Elapsed.TotalMilliseconds
    }
}

function Measure-BestBsaRun {
    <#
    .SYNOPSIS
    Runs one `bsa` invocation several times and returns the fastest result.

    .DESCRIPTION
    Reporting the minimum rather than the mean is the standard way to suppress
    scheduler and on-access-scanner noise: the fastest run is the one least
    disturbed by work that has nothing to do with the command.

    The returned object carries the stdout and stderr text of the fastest run, so
    callers can parse metadata or capture a rejection diagnostic without paying
    for it inside the timed window.

    .PARAMETER Repeats
    Number of timed runs.

    .PARAMETER WarmUp
    Perform one discarded run first so that OS file cache state is the same for
    every timed run. Skip on a slow build, where the warm-up costs as much as the
    measurement.

    .PARAMETER Reset
    Optional script block run before every invocation, including the warm-up.
    Used to clear an output directory so one run cannot benefit from what a
    previous one wrote.
    #>
    param(
        [Parameter(Mandatory = $true)][string]$BsaPath,
        [Parameter(Mandatory = $true)][string[]]$BsaArgs,
        [Parameter(Mandatory = $true)][string]$StdoutFile,
        [Parameter(Mandatory = $true)][string]$StderrFile,
        [int]$Repeats = 3,
        [switch]$WarmUp,
        [scriptblock]$Reset
    )

    if ($WarmUp) {
        if ($Reset) { & $Reset }
        Invoke-TimedBsa -BsaPath $BsaPath -BsaArgs $BsaArgs -StdoutFile $StdoutFile -StderrFile $StderrFile | Out-Null
    }

    $best = $null
    $bestStdout = @()
    $bestStderr = @()
    for ($run = 0; $run -lt $Repeats; $run++) {
        if ($Reset) { & $Reset }
        $attempt = Invoke-TimedBsa -BsaPath $BsaPath -BsaArgs $BsaArgs -StdoutFile $StdoutFile -StderrFile $StderrFile
        if (-not $best -or $attempt.ElapsedMs -lt $best.ElapsedMs) {
            $best = $attempt
            # Read the streams of the fastest run only, and outside the stopwatch.
            $bestStdout = @(Get-Content -LiteralPath $StdoutFile -ErrorAction SilentlyContinue)
            $bestStderr = @(Get-Content -LiteralPath $StderrFile -ErrorAction SilentlyContinue)
        }
    }

    return [pscustomobject]@{
        ExitCode  = $best.ExitCode
        ElapsedMs = $best.ElapsedMs
        Stdout    = $bestStdout
        Stderr    = $bestStderr
    }
}

function ConvertFrom-BsaInfoOutput {
    <#
    .SYNOPSIS
    Parses the `label: value` lines `bsa info` prints into a hashtable.

    .DESCRIPTION
    Returns an empty hashtable when the archive failed to open, so callers can
    test for a field rather than handling a parse failure.
    #>
    param([string[]]$Lines)

    $fields = @{}
    foreach ($line in $Lines) {
        $separator = $line.IndexOf(': ')
        if ($separator -lt 1) { continue }
        $fields[$line.Substring(0, $separator)] = $line.Substring($separator + 2)
    }
    return $fields
}

function Resolve-CorpusPath {
    <#
    .SYNOPSIS
    Resolves the retail archive corpus directory.

    .DESCRIPTION
    LIBBSA_GAME_FIXTURES wins when set, matching how the test suite resolves the
    same corpus; otherwise tests/fixtures/local/ relative to the repository root.
    #>
    param([string]$Explicit)

    if ($Explicit) { return (Resolve-Path -LiteralPath $Explicit).Path }
    if ($env:LIBBSA_GAME_FIXTURES) { return (Resolve-Path -LiteralPath $env:LIBBSA_GAME_FIXTURES).Path }

    $repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    $default = Join-Path $repoRoot 'tests\fixtures\local'
    if (-not (Test-Path -LiteralPath $default)) {
        throw "No corpus found. Set LIBBSA_GAME_FIXTURES or pass -Corpus. Looked for '$default'."
    }
    return (Resolve-Path -LiteralPath $default).Path
}

function Assert-OutsideCorpus {
    <#
    .SYNOPSIS
    Throws if a path that will be deleted or written sits inside the corpus.

    .DESCRIPTION
    AGENTS.md makes the retail corpus strictly read-only: it must never be
    modified, moved, renamed or deleted. These scripts delete scratch directories
    and write result CSVs, so one mistyped argument could otherwise modify the
    corpus. Checked before anything is created, removed or overwritten.
    #>
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$CorpusPath
    )

    $full = [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
    $corpus = [System.IO.Path]::GetFullPath($CorpusPath).TrimEnd('\')
    if ($full -eq $corpus -or $full.StartsWith($corpus + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to use '$Path': it is inside the read-only archive corpus '$CorpusPath'."
    }
}
