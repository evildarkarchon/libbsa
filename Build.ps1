<#
.SYNOPSIS
Builds, tests, or cleans the libbsa CMake project.

.DESCRIPTION
This script is a beginner-friendly wrapper around the CMake commands that are
normally typed by hand. It supports the common inner-loop actions for this repo:
building the library target, building the CLI target, cleaning build directories,
and running CTest. The CLI links against the library in CMake, so building the
CLI target also builds the library first whenever CMake sees that it is missing
or out of date.

By default the script uses this repository's checked-in CMake presets. Presets are
the recommended day-to-day path because they keep the build directory, vcpkg
toolchain file, test settings, and Debug/Release lane names in one shared place:

    cmake --preset windows-msvc-debug-static
    cmake --build --preset windows-msvc-debug-static --target libbsa
    ctest --preset windows-msvc-debug-static --output-on-failure

If you want to experiment with a different generator or build configuration,
pass -Generator, -Configuration, -BuildRoot, -BuildDirectory, or -NoPreset. That
switches the script to explicit configure/build commands such as:

    cmake -S <source> -B <build> -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build <build> --config Debug --target libbsa
    ctest --test-dir <build> -C Debug --output-on-failure

.EXAMPLE
.\Build.ps1 -Library

Configures with the default preset and builds only the libbsa library target.

.EXAMPLE
.\Build.ps1 -Cli

Configures with the default preset and builds the bsa command-line tool. CMake
also builds the libbsa library target first if the CLI dependency needs it.

.EXAMPLE
.\Build.ps1 -Clean -Library -Cli

Deletes the active build directory, configures again, then builds the library and
CLI targets.

.EXAMPLE
.\Build.ps1 -Tests

Configures, builds the default targets needed by the test graph, then runs CTest.

.EXAMPLE
.\Build.ps1 -Library -Generator Ninja -Configuration Release

Uses an explicit build directory instead of presets, configures with Ninja, and
builds the library as Release.
#>

[CmdletBinding()]
param(
    # Build the CMake library target. In this repository the target is named "libbsa".
    [switch] $Library,

    # Build the CMake CLI executable target. In this repository the target is named "bsa".
    # The CMake target links libbsa::libbsa, so CMake builds the library dependency first.
    [switch] $Cli,

    # Build the default test prerequisites and then run CTest.
    [Alias('Test', 'Tests')]
    [switch] $RunTests,

    # Remove the active build directory before doing any requested build or test work.
    [switch] $Clean,

    # Remove the whole build root. Use this when you want to delete all preset/custom builds.
    [switch] $CleanAll,

    # CMake preset used by the normal workflow. See CMakePresets.json for all supported lanes.
    [string] $Preset = 'windows-msvc-debug-static',

    # Bypass CMake presets and use explicit cmake -S/-B commands instead.
    [switch] $NoPreset,

    # Optional CMake generator for explicit mode, for example: Ninja or Visual Studio 18 2026.
    [string] $Generator = '',

    # Build configuration used in explicit mode and passed to multi-config generators at build time.
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string] $Configuration = 'Debug',

    # Root directory for explicit-mode build folders. Relative paths are resolved from this script.
    [string] $BuildRoot = (Join-Path -Path $PSScriptRoot -ChildPath 'build'),

    # Exact explicit-mode build directory. If omitted, the script derives one from generator/config.
    [string] $BuildDirectory = '',

    # vcpkg checkout root. The checked-in presets read the VCPKG_ROOT environment variable.
    [string] $VcpkgRoot = $env:VCPKG_ROOT,

    # Extra CMake cache options, for example: -CMakeOption '-DLIBBSA_BUILD_BENCHMARKS=OFF'.
    [string[]] $CMakeOption = @(),

    # Override target names if the CMakeLists.txt target names change later.
    [string] $LibraryTarget = 'libbsa',
    [string] $CliTarget = 'bsa'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$SourceDirectory = (Resolve-Path -LiteralPath $PSScriptRoot).ProviderPath

function ConvertTo-AbsolutePath {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $BaseDirectory
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path -Path $BaseDirectory -ChildPath $Path))
}

function Get-SafePathForDisplay {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path
    )

    if ($Path.Contains(' ')) {
        return '"{0}"' -f $Path
    }

    return $Path
}

function Invoke-ExternalCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string] $FilePath,

        [string[]] $Arguments = @()
    )

    # The call operator (&) runs external programs without going through a shell.
    # Passing arguments as an array preserves spaces in paths and avoids quoting bugs.
    $displayArguments = @($Arguments | ForEach-Object { Get-SafePathForDisplay -Path $_ })
    Write-Host "> $FilePath $($displayArguments -join ' ')"

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command '$FilePath' failed with exit code $LASTEXITCODE."
    }
}

function Assert-CommandAvailable {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Name
    )

    if (-not (Get-Command -Name $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found on PATH. Install it or open a shell where it is available."
    }
}

function Assert-SafeCleanPath {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $Description
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $sourcePath = [System.IO.Path]::GetFullPath($SourceDirectory)
    $trimChars = [char[]] @([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)

    $normalizedPath = $fullPath.TrimEnd($trimChars)
    $normalizedSource = $sourcePath.TrimEnd($trimChars)

    if ($normalizedPath -eq $normalizedSource) {
        throw "Refusing to clean $Description because it resolves to the source directory: $fullPath"
    }

    # A custom -BuildRoot such as '..' resolves to an ancestor of the checkout. Recursive removal
    # there would take the repository (and any sibling projects) with it, so ancestors are rejected
    # outright rather than only the exact source directory.
    $sourceWithSeparator = $normalizedSource + [System.IO.Path]::DirectorySeparatorChar
    if ($sourceWithSeparator.StartsWith($normalizedPath + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean $Description because it contains the source directory: $fullPath"
    }

    # Volume roots ('D:\') and UNC share roots ('\\server\share') have no parent to fall back on,
    # so a recursive delete there is never a build-tree cleanup.
    $pathRoot = [System.IO.Path]::GetPathRoot($fullPath)
    if (-not [string]::IsNullOrEmpty($pathRoot) -and $normalizedPath -eq $pathRoot.TrimEnd($trimChars)) {
        throw "Refusing to clean $Description because it resolves to a filesystem root: $fullPath"
    }

    if ([string]::IsNullOrWhiteSpace($fullPath) -or $fullPath.Length -lt 4) {
        throw "Refusing to clean suspiciously short $Description path: $fullPath"
    }
}

function Remove-BuildDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $Description
    )

    Assert-SafeCleanPath -Path $Path -Description $Description

    if (-not (Test-Path -LiteralPath $Path)) {
        Write-Host "$Description does not exist: $Path"
        return
    }

    Write-Host "Removing $Description`: $Path"
    Remove-Item -LiteralPath $Path -Recurse -Force
}

function Get-Slug {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Value
    )

    $slug = $Value -replace '[^A-Za-z0-9._-]+', '-'
    return $slug.Trim('-')
}

function Resolve-VcpkgToolchain {
    param(
        [string] $Root
    )

    if ([string]::IsNullOrWhiteSpace($Root)) {
        throw 'VCPKG_ROOT is not set. Set $env:VCPKG_ROOT to your vcpkg checkout, for example: $env:VCPKG_ROOT = ''C:\vcpkg''.'
    }

    $resolvedRoot = (Resolve-Path -LiteralPath $Root).ProviderPath
    $toolchainFile = Join-Path -Path $resolvedRoot -ChildPath 'scripts/buildsystems/vcpkg.cmake'

    if (-not (Test-Path -LiteralPath $toolchainFile)) {
        throw "Could not find the vcpkg CMake toolchain file at: $toolchainFile"
    }

    # CMakePresets.json uses $env{VCPKG_ROOT}; update it after resolving the path so
    # child cmake processes see a normalized vcpkg location.
    $env:VCPKG_ROOT = $resolvedRoot
    return $toolchainFile
}

$customModeRequested =
    $NoPreset -or
    $PSBoundParameters.ContainsKey('Generator') -or
    $PSBoundParameters.ContainsKey('Configuration') -or
    $PSBoundParameters.ContainsKey('BuildRoot') -or
    $PSBoundParameters.ContainsKey('BuildDirectory')

if ($customModeRequested -and $PSBoundParameters.ContainsKey('Preset')) {
    throw 'Do not combine -Preset with custom-mode options such as -Generator, -Configuration, -BuildRoot, -BuildDirectory, or -NoPreset.'
}

$UsePreset = -not $customModeRequested
$ResolvedBuildRoot = ConvertTo-AbsolutePath -Path $BuildRoot -BaseDirectory $SourceDirectory

if ($UsePreset) {
    # The preset's binaryDir is "${sourceDir}/build/${presetName}" in CMakePresets.json.
    # Keep this calculation in sync so -Clean knows which directory to remove.
    $ActiveBuildDirectory = Join-Path -Path $ResolvedBuildRoot -ChildPath $Preset
}
else {
    if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
        $generatorPart = if ([string]::IsNullOrWhiteSpace($Generator)) { 'default-generator' } else { Get-Slug -Value $Generator }
        $configurationPart = Get-Slug -Value $Configuration.ToLowerInvariant()
        $ActiveBuildDirectory = Join-Path -Path $ResolvedBuildRoot -ChildPath "custom-$configurationPart-$generatorPart"
    }
    else {
        $ActiveBuildDirectory = ConvertTo-AbsolutePath -Path $BuildDirectory -BaseDirectory $SourceDirectory
    }
}

$anyActionRequested = $Library -or $Cli -or $RunTests -or $Clean -or $CleanAll
if (-not $anyActionRequested) {
    # A useful beginner default: running .\Build.ps1 builds the two primary products.
    # Add -Tests when you also want to run CTest, or -Clean when you want a fresh build.
    Write-Host 'No action specified; defaulting to -Library -Cli.'
    $Library = $true
    $Cli = $true
}

if ($CleanAll) {
    Remove-BuildDirectory -Path $ResolvedBuildRoot -Description 'build root'
}
elseif ($Clean) {
    Remove-BuildDirectory -Path $ActiveBuildDirectory -Description 'active build directory'
}

$needsConfigure = $Library -or $Cli -or $RunTests
if (-not $needsConfigure) {
    Write-Host 'No build or test action requested after cleaning.'
    exit 0
}

Assert-CommandAvailable -Name 'cmake'
if ($RunTests) {
    Assert-CommandAvailable -Name 'ctest'
}

$vcpkgToolchain = Resolve-VcpkgToolchain -Root $VcpkgRoot

if ($UsePreset) {
    Write-Host "Using CMake preset '$Preset'."
}
else {
    Write-Host "Using explicit CMake configure mode in: $ActiveBuildDirectory"
}

# Configure step:
#   cmake --preset <name>
# reads CMakePresets.json and writes generated build files into the preset's
# binaryDir. In explicit mode, cmake -S points to the source tree and -B points
# to the out-of-source build directory that CMake can create for us.
if ($UsePreset) {
    $configureArguments = @('--preset', $Preset)
    $configureArguments += $CMakeOption
}
else {
    $configureArguments = @(
        '-S', $SourceDirectory,
        '-B', $ActiveBuildDirectory,
        "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain",
        "-DCMAKE_BUILD_TYPE=$Configuration",
        '-DBUILD_TESTING=ON',
        '-DLIBBSA_BUILD_TESTS=ON',
        '-DLIBBSA_BUILD_CLI=ON'
    )

    if (-not [string]::IsNullOrWhiteSpace($Generator)) {
        # -G selects the build-system generator. Common choices on Windows are
        # "Ninja" for fast single-config builds or a Visual Studio generator for
        # IDE/MSBuild projects.
        $configureArguments += @('-G', $Generator)
    }

    $configureArguments += $CMakeOption
}

Invoke-ExternalCommand -FilePath 'cmake' -Arguments $configureArguments

function Invoke-CMakeBuildTarget {
    param(
        [string] $TargetName,

        [Parameter(Mandatory = $true)]
        [string] $Description
    )

    Write-Host "Building $Description."

    # Build step:
    #   cmake --build <build-dir-or-preset> --target <target>
    # asks CMake to call the underlying build tool for us. That means the same
    # command works with Ninja, MSBuild/Visual Studio, and other generators.
    if ($UsePreset) {
        $buildArguments = @('--build', '--preset', $Preset)
    }
    else {
        $buildArguments = @('--build', $ActiveBuildDirectory, '--config', $Configuration)
    }

    if (-not [string]::IsNullOrWhiteSpace($TargetName)) {
        $buildArguments += @('--target', $TargetName)
    }

    Invoke-ExternalCommand -FilePath 'cmake' -Arguments $buildArguments
}

if ($RunTests) {
    # CTest runs already-built executables; it does not compile them first.
    # Building the default target before CTest ensures libbsa_tests, the CLI used
    # by cli_integration, and other normal test prerequisites are available.
    Invoke-CMakeBuildTarget -TargetName '' -Description 'default targets required by tests'
}
else {
    if ($Library) {
        Invoke-CMakeBuildTarget -TargetName $LibraryTarget -Description "library target '$LibraryTarget'"
    }

    if ($Cli) {
        Invoke-CMakeBuildTarget -TargetName $CliTarget -Description "CLI target '$CliTarget'"
    }
}

if ($RunTests) {
    Write-Host 'Running tests with CTest.'

    # Test step:
    #   ctest --preset <name>
    # reuses the checked-in test preset. In explicit mode, --test-dir points CTest
    # at the configured build tree and -C selects the configuration for Visual
    # Studio style multi-config generators. --output-on-failure prints failed test
    # logs immediately, which is the most useful behavior during local debugging.
    if ($UsePreset) {
        $testArguments = @('--preset', $Preset, '--output-on-failure')
    }
    else {
        $testArguments = @('--test-dir', $ActiveBuildDirectory, '-C', $Configuration, '--output-on-failure')
    }

    Invoke-ExternalCommand -FilePath 'ctest' -Arguments $testArguments
}

Write-Host 'CMake workflow completed successfully.'
