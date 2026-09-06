#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $SourceDirectory,

    [Parameter(Mandatory = $true)]
    [string] $BinaryDirectory
)

$ErrorActionPreference = 'Stop'

function Invoke-ChildPowerShell {
    <#
    .SYNOPSIS
    Runs one repository script in an isolated PowerShell process.

    .DESCRIPTION
    Captures both output streams and optionally prepends a test-tool directory
    to PATH. The separate process lets the test observe terminating errors and
    exit codes without changing this driver's error preferences.
    #>
    param(
        [Parameter(Mandatory = $true)][string] $ScriptPath,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory,
        [string] $PathPrefix
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = (Get-Process -Id $PID).Path
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    foreach ($argument in @('-NoLogo', '-NoProfile', '-NonInteractive', '-File', $ScriptPath) + $Arguments) {
        [void] $startInfo.ArgumentList.Add($argument)
    }

    if ($PathPrefix) {
        $startInfo.Environment['PATH'] = $PathPrefix + [System.IO.Path]::PathSeparator +
            [System.Environment]::GetEnvironmentVariable('PATH')
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()

    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        Output   = $stdout.Result + $stderr.Result
    }
}

function Test-CorpusOutputBoundary {
    <#
    .SYNOPSIS
    Verifies that a measurement script rejects a CSV path inside its corpus.
    #>
    param(
        [Parameter(Mandatory = $true)][string] $Name,
        [Parameter(Mandatory = $true)][string] $ScriptPath,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory,
        [Parameter(Mandatory = $true)][string] $OutputPath
    )

    $result = Invoke-ChildPowerShell -ScriptPath $ScriptPath -Arguments $Arguments `
        -WorkingDirectory $WorkingDirectory

    $problems = [System.Collections.Generic.List[string]]::new()
    if ($result.ExitCode -eq 0) {
        $problems.Add('the script exited successfully')
    }
    if ($result.Output -notmatch 'inside the read-only archive corpus') {
        $problems.Add("the diagnostic did not identify the read-only corpus boundary: $($result.Output.Trim())")
    }
    if (Test-Path -LiteralPath $OutputPath) {
        $problems.Add("the script wrote '$OutputPath'")
    }

    if ($problems.Count -gt 0) {
        $script:failures.Add("$Name`: $($problems -join '; ')")
    }
    else {
        Write-Host "PASS: $Name"
    }
}

function Test-FormatterBoundary {
    <#
    .SYNOPSIS
    Verifies that Format-Cpp rejects a root inside the reference submodule.

    .DESCRIPTION
    A no-op clang-format is placed first on PATH so a missing pre-traversal guard
    cannot modify reference files while this regression test observes the bug.
    #>
    param(
        [Parameter(Mandatory = $true)][string] $Name,
        [Parameter(Mandatory = $true)][string] $FormatScript,
        [Parameter(Mandatory = $true)][string] $Root,
        [Parameter(Mandatory = $true)][string] $FakeToolDirectory,
        [Parameter(Mandatory = $true)][string] $InvocationSentinel
    )

    Remove-Item -LiteralPath $InvocationSentinel -Force -ErrorAction SilentlyContinue
    $result = Invoke-ChildPowerShell -ScriptPath $FormatScript -Arguments @('-Root', $Root) `
        -WorkingDirectory $SourceDirectory -PathPrefix $FakeToolDirectory

    $problems = [System.Collections.Generic.List[string]]::new()
    if ($result.ExitCode -eq 0) {
        $problems.Add('the formatter exited successfully')
    }
    if ($result.Output -notmatch 'read-only TES5Edit') {
        $problems.Add("the diagnostic did not identify the read-only TES5Edit boundary: $($result.Output.Trim())")
    }
    if (Test-Path -LiteralPath $InvocationSentinel) {
        $problems.Add('clang-format was invoked before the formatter rejected the root')
    }

    if ($problems.Count -gt 0) {
        $script:failures.Add("$Name`: $($problems -join '; ')")
    }
    else {
        Write-Host "PASS: $Name"
    }
}

$failures = [System.Collections.Generic.List[string]]::new()
$testRoot = Join-Path $BinaryDirectory ("readonly-tool-boundaries-" + [System.Guid]::NewGuid().ToString('N'))
$corpusRoot = Join-Path $testRoot 'corpus'
$scratchRoot = Join-Path $testRoot 'scratch'
$fakeToolDirectory = Join-Path $testRoot 'bin'
$formatSandbox = Join-Path $testRoot 'format-sandbox'
$formatSentinel = Join-Path $testRoot 'clang-format-invoked.txt'

try {
    New-Item -ItemType Directory -Force -Path $corpusRoot, $scratchRoot, $fakeToolDirectory, $formatSandbox | Out-Null

    $archivePath = Join-Path $corpusRoot 'sample.ba2'
    Set-Content -LiteralPath $archivePath -Value 'synthetic archive marker' -NoNewline

    $fakeBsa = Join-Path $fakeToolDirectory 'fake-bsa.cmd'
    Set-Content -LiteralPath $fakeBsa -Encoding ascii -Value @'
@echo off
if /I "%~1"=="info" (
  echo file_count: 1
  echo type: BA2
  echo variant: Fallout 4
  echo version: 1
  exit /b 0
)
if /I "%~1"=="list" (
  echo meshes\sample.nif
  exit /b 0
)
exit /b 0
'@

    Set-Content -LiteralPath (Join-Path $fakeToolDirectory 'clang-format.cmd') -Encoding ascii -Value @"
@echo off
echo invoked>>"$formatSentinel"
exit /b 0
"@

    $archiveScript = Join-Path $SourceDirectory 'tools\perf\Measure-ArchiveOpenCost.ps1'
    $commandScript = Join-Path $SourceDirectory 'tools\perf\Measure-CommandOpenCost.ps1'
    $unpackScript = Join-Path $SourceDirectory 'tools\perf\Measure-UnpackCost.ps1'

    $archiveOutput = Join-Path $corpusRoot 'archive-explicit.csv'
    Test-CorpusOutputBoundary -Name 'archive sweep rejects explicit corpus output' `
        -ScriptPath $archiveScript -WorkingDirectory $SourceDirectory -OutputPath $archiveOutput `
        -Arguments @('-Bsa', $fakeBsa, '-Corpus', $corpusRoot, '-Repeats', '1',
            '-ScratchRoot', (Join-Path $scratchRoot 'archive-explicit'), '-OutputCsv', $archiveOutput)

    $defaultOutput = Join-Path $corpusRoot 'archive-open-cost-default-boundary-info.csv'
    Test-CorpusOutputBoundary -Name 'archive sweep rejects current-directory default output' `
        -ScriptPath $archiveScript -WorkingDirectory $corpusRoot -OutputPath $defaultOutput `
        -Arguments @('-Bsa', $fakeBsa, '-Corpus', $corpusRoot, '-Repeats', '1',
            '-ScratchRoot', (Join-Path $scratchRoot 'archive-default'), '-Label', 'default-boundary')

    $commandOutput = Join-Path $corpusRoot 'command.csv'
    Test-CorpusOutputBoundary -Name 'command sweep rejects corpus output' `
        -ScriptPath $commandScript -WorkingDirectory $SourceDirectory -OutputPath $commandOutput `
        -Arguments @('-Bsa', $fakeBsa, '-Archive', $archivePath, '-Repeats', '1', '-NoWarmUp',
            '-ScratchRoot', (Join-Path $scratchRoot 'command'), '-OutputCsv', $commandOutput)

    $unpackOutput = Join-Path $corpusRoot 'unpack.csv'
    Test-CorpusOutputBoundary -Name 'unpack sweep rejects corpus output' `
        -ScriptPath $unpackScript -WorkingDirectory $SourceDirectory -OutputPath $unpackOutput `
        -Arguments @('-Bsa', $fakeBsa, '-Archive', $archivePath, '-Repeats', '1', '-NoWarmUp',
            '-Threads', '1', '-ScratchRoot', (Join-Path $scratchRoot 'unpack'), '-OutputCsv', $unpackOutput)

    # Copying the real script beside a synthetic TES5Edit tree exercises its
    # PSScriptRoot-relative boundary without traversing the reference submodule.
    $formatScript = Join-Path $formatSandbox 'Format-Cpp.ps1'
    Copy-Item -LiteralPath (Join-Path $SourceDirectory 'Format-Cpp.ps1') -Destination $formatScript
    $syntheticTes5Edit = Join-Path $formatSandbox 'TES5Edit'
    $syntheticDescendant = Join-Path $syntheticTes5Edit 'nested'
    New-Item -ItemType Directory -Force -Path $syntheticDescendant | Out-Null
    Set-Content -LiteralPath (Join-Path $syntheticDescendant 'probe.cpp') -Value 'int probe;' -NoNewline

    Test-FormatterBoundary -Name 'formatter rejects the TES5Edit root' -FormatScript $formatScript `
        -Root $syntheticTes5Edit -FakeToolDirectory $fakeToolDirectory -InvocationSentinel $formatSentinel
    Test-FormatterBoundary -Name 'formatter rejects a TES5Edit descendant' -FormatScript $formatScript `
        -Root $syntheticDescendant -FakeToolDirectory $fakeToolDirectory -InvocationSentinel $formatSentinel
}
finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}

if ($failures.Count -gt 0) {
    throw "Read-only tooling boundary failures:`n - $($failures -join "`n - ")"
}

Write-Host 'All read-only tooling boundary checks passed.'
