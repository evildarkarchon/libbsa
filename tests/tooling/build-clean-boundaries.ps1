#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $SourceDirectory,

    [Parameter(Mandatory = $true)]
    [string] $BinaryDirectory
)

$ErrorActionPreference = 'Stop'

function Invoke-ChildBuild {
    <#
    .SYNOPSIS
    Runs the copied build script so its exit does not end this test process.
    #>
    param(
        [Parameter(Mandatory = $true)][string] $ScriptPath,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory
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

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()

    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        Output   = $stdout.Result + $stderr.Result
    }
}

function Assert-CleanupRejected {
    <#
    .SYNOPSIS
    Requires a cleanup request to fail before removing a protected marker.
    #>
    param(
        [Parameter(Mandatory = $true)][string] $Name,
        [Parameter(Mandatory = $true)][string] $ScriptPath,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $ProtectedPath
    )

    $result = Invoke-ChildBuild -ScriptPath $ScriptPath -Arguments $Arguments -WorkingDirectory $SourceDirectory
    if ($result.ExitCode -eq 0 -or $result.Output -notmatch 'Refusing to clean' -or
        -not (Test-Path -LiteralPath $ProtectedPath)) {
        $script:failures.Add("$Name`: exit=$($result.ExitCode), marker exists=$(Test-Path -LiteralPath $ProtectedPath), output=$($result.Output.Trim())")
    }
    else {
        Write-Host "PASS: $Name"
    }
}

function Assert-CleanupAllowed {
    <#
    .SYNOPSIS
    Requires cleanup of a designated build directory to remove its marker.
    #>
    param(
        [Parameter(Mandatory = $true)][string] $Name,
        [Parameter(Mandatory = $true)][string] $ScriptPath,
        [Parameter(Mandatory = $true)][string[]] $Arguments,
        [Parameter(Mandatory = $true)][string] $RemovedPath
    )

    $result = Invoke-ChildBuild -ScriptPath $ScriptPath -Arguments $Arguments -WorkingDirectory $SourceDirectory
    if ($result.ExitCode -ne 0 -or (Test-Path -LiteralPath $RemovedPath)) {
        $script:failures.Add("$Name`: exit=$($result.ExitCode), marker exists=$(Test-Path -LiteralPath $RemovedPath), output=$($result.Output.Trim())")
    }
    else {
        Write-Host "PASS: $Name"
    }
}

$failures = [System.Collections.Generic.List[string]]::new()
$testRoot = Join-Path $BinaryDirectory ("build-clean-boundaries-" + [System.Guid]::NewGuid().ToString('N'))
$checkout = Join-Path $testRoot 'checkout'
$buildScript = Join-Path $checkout 'Build.ps1'

try {
    New-Item -ItemType Directory -Force -Path $checkout | Out-Null
    Copy-Item -LiteralPath (Join-Path $SourceDirectory 'Build.ps1') -Destination $buildScript

    $sourceTree = Join-Path $checkout 'src'
    New-Item -ItemType Directory -Force -Path $sourceTree | Out-Null
    $sourceMarker = Join-Path $sourceTree 'source-marker.txt'
    Set-Content -LiteralPath $sourceMarker -Value 'keep'
    Assert-CleanupRejected -Name 'source descendant' -ScriptPath $buildScript `
        -Arguments @('-BuildDirectory', 'src', '-Clean') -ProtectedPath $sourceMarker

    $referenceTree = Join-Path $checkout 'TES5Edit'
    New-Item -ItemType Directory -Force -Path $referenceTree | Out-Null
    $referenceMarker = Join-Path $referenceTree 'reference-marker.txt'
    Set-Content -LiteralPath $referenceMarker -Value 'keep'
    Assert-CleanupRejected -Name 'reference submodule build root' -ScriptPath $buildScript `
        -Arguments @('-BuildRoot', 'TES5Edit', '-CleanAll') -ProtectedPath $referenceMarker

    # A path below a junction has no ReparsePoint attribute itself, but Remove-Item
    # follows the parent junction and deletes the target's contents.
    $junctionTarget = Join-Path $sourceTree 'junction-target'
    New-Item -ItemType Directory -Force -Path $junctionTarget | Out-Null
    $junctionMarker = Join-Path $junctionTarget 'junction-marker.txt'
    Set-Content -LiteralPath $junctionMarker -Value 'keep'
    $buildTree = Join-Path $checkout 'build'
    New-Item -ItemType Directory -Force -Path $buildTree | Out-Null
    New-Item -ItemType Junction -Path (Join-Path $buildTree 'alias') -Target $sourceTree | Out-Null
    Assert-CleanupRejected -Name 'build junction into source' -ScriptPath $buildScript `
        -Arguments @('-BuildDirectory', 'build/alias/junction-target', '-Clean') -ProtectedPath $junctionMarker

    New-Item -ItemType Junction -Path (Join-Path $buildTree 'reference-alias') -Target $referenceTree | Out-Null
    $result = Invoke-ChildBuild -ScriptPath $buildScript -Arguments @('-CleanAll') -WorkingDirectory $SourceDirectory
    if (-not (Test-Path -LiteralPath $junctionMarker) -or -not (Test-Path -LiteralPath $referenceMarker)) {
        $failures.Add("build root containing junctions: recursive cleanup followed a junction; output=$($result.Output.Trim())")
    }
    else {
        Write-Host 'PASS: build root containing junctions preserves their targets'
    }

    $allowedBuild = Join-Path $buildTree 'allowed'
    New-Item -ItemType Directory -Force -Path $allowedBuild | Out-Null
    $allowedMarker = Join-Path $allowedBuild 'build-marker.txt'
    Set-Content -LiteralPath $allowedMarker -Value 'remove'
    Assert-CleanupAllowed -Name 'designated build directory' -ScriptPath $buildScript `
        -Arguments @('-BuildDirectory', 'build/allowed', '-Clean') -RemovedPath $allowedMarker

    $externalBuild = Join-Path $testRoot 'external-build'
    New-Item -ItemType Directory -Force -Path $externalBuild | Out-Null
    $externalMarker = Join-Path $externalBuild 'build-marker.txt'
    Set-Content -LiteralPath $externalMarker -Value 'remove'
    Assert-CleanupAllowed -Name 'external custom build directory' -ScriptPath $buildScript `
        -Arguments @('-BuildDirectory', $externalBuild, '-Clean') -RemovedPath $externalMarker

    $ancestorMarker = Join-Path $checkout 'ancestor-marker.txt'
    Set-Content -LiteralPath $ancestorMarker -Value 'keep'
    Assert-CleanupRejected -Name 'checkout ancestor' -ScriptPath $buildScript `
        -Arguments @('-BuildRoot', $testRoot, '-CleanAll') -ProtectedPath $ancestorMarker
}
finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}

if ($failures.Count -gt 0) {
    throw "Build cleanup boundary failures:`n - $($failures -join "`n - ")"
}

Write-Host 'All build cleanup boundary checks passed.'
