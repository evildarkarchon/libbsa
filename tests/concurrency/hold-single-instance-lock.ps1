<#
.SYNOPSIS
    Holds the libbsa_tests single-instance lock and runs one command while it is held.

.DESCRIPTION
    A primitive for `concurrent-test-instances.cmake`, which cannot do this itself:
    CMake script mode has no mutex API and no way to start a process without also
    waiting for it. This script takes the named mutex the guard in
    `tests/support/single_instance_guard.cpp` contends on, runs the command handed
    to it, records that command's exit code and output, and releases the mutex.

    Holding the lock here rather than racing two real test binaries is what makes
    the refusal deterministic. Two instances launched simultaneously would contend
    with very high probability but not with certainty, and the failure this whole
    change exists to remove is a test that is *usually* right. What is lost by not
    using a real second instance as the holder is the coupling between the lock
    name the guard publishes and the one it actually takes -- and that is already
    pinned directly, from inside the process that holds it, by
    "the running test process holds the single-instance lock" in
    `tests/unit/single_instance_guard_tests.cpp`.

    The mutex is created rather than opened, because the guard's own lock object
    only exists while some libbsa_tests process is running and there is none here.
    Ownership is taken with a zero timeout: if anything else already holds the lock
    this script refuses rather than blocking, since a holder it did not create means
    a stray test process is running and every assertion downstream would be about
    that process instead of about the guard.

.NOTES
    Windows-only, like the rest of libbsa. Written against Windows PowerShell 5.1,
    which is the interpreter `tests/cli/cli_integration.cmake` already reaches for,
    so it avoids .NET Core-only APIs such as ProcessStartInfo.ArgumentList.
#>
[CmdletBinding()]
param(
    # Full mutex name including any `Local\` prefix, passed in from CMake so this
    # script and the guard cannot drift apart on a spelling.
    [Parameter(Mandatory = $true)][string] $LockName,

    # Executable to run while the lock is held. Named ChildCommand rather than
    # Command so that the caller's argument list cannot be misread as powershell.exe's
    # own -Command switch; everything after -File binds to this script's parameters,
    # but a reader should not have to know that to follow the invocation.
    [Parameter(Mandatory = $true)][string] $ChildCommand,

    # Arguments for that executable. None of them may contain spaces: Windows
    # PowerShell 5.1 has no ProcessStartInfo.ArgumentList, so they are joined into
    # the single command-line string ProcessStartInfo.Arguments requires.
    [string[]] $ChildArguments = @(),

    # Where the command's stdout, stderr, and exit code are recorded. The caller
    # asserts on all three; this script only reports them.
    [Parameter(Mandatory = $true)][string] $StdoutFile,
    [Parameter(Mandatory = $true)][string] $StderrFile,
    [Parameter(Mandatory = $true)][string] $ExitCodeFile,

    # Name of the guard's opt-out variable, deleted from this process's environment
    # before the child is started. Passed in for the same reason LockName is: the
    # spelling belongs to tests/support/single_instance_guard.hpp, and a copy here
    # that drifted from it would silently stop clearing the variable the guard reads.
    [Parameter(Mandatory = $true)][string] $OptOutVariable,

    # Upper bound on the command's runtime. The command under test is expected to
    # refuse to start and exit in milliseconds, so this only bounds a hang.
    [int] $TimeoutSeconds = 120
)

# Terminating errors by default, so a failure here is a non-zero exit rather than a
# warning followed by a misleading success.
$ErrorActionPreference = 'Stop'

# Exit codes this script owns, kept distinct from the child's so the caller can tell
# "the harness failed" from "the child exited non-zero", which is the outcome the
# caller is actually testing for.
$exitHarnessLockUnavailable = 90
$exitHarnessChildTimedOut = 91
$exitHarnessChildNotStarted = 92

function Write-Diagnostic([string] $Message) {
    # [Console]::Error rather than Write-Error: $ErrorActionPreference is Stop, so
    # Write-Error would throw here instead of letting the explicit exit code below
    # carry the outcome.
    [Console]::Error.WriteLine($Message)
}

# The child must be refused, so it must not inherit an opt-out from the developer's
# shell. Deleted rather than set to the empty string: both read as "not opted out"
# to the guard, but a deleted variable also cannot be misread by anything else the
# child does. Only this process's environment is touched, and it exits shortly.
$optOutPath = "Env:\$OptOutVariable"
if (Test-Path -LiteralPath $optOutPath) {
    Remove-Item -LiteralPath $optOutPath
}

$mutex = New-Object System.Threading.Mutex($false, $LockName)
$held = $false
$process = $null
$processStarted = $false
try {
    try {
        $held = $mutex.WaitOne(0)
    }
    catch [System.Threading.AbandonedMutexException] {
        # A previous owner died without releasing. Ownership passes to this thread
        # anyway, and it still has to be released, so this counts as held. Same rule
        # the guard applies to WAIT_ABANDONED.
        $held = $true
    }

    if (-not $held) {
        Write-Diagnostic "hold-single-instance-lock.ps1: '$LockName' is already held by another process, so this script cannot stand in as the lock's owner. A stray libbsa_tests process is the usual cause."
        exit $exitHarnessLockUnavailable
    }

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $ChildCommand
    $startInfo.Arguments = $ChildArguments -join ' '
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    # Start the owned Process object directly. On Windows PowerShell 5.1,
    # Start-Process may not retain its native handle long enough for ExitCode to
    # remain observable when a fast child exits before the caller caches Handle.
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            Write-Diagnostic "hold-single-instance-lock.ps1: failed to start '$ChildCommand'."
            exit $exitHarnessChildNotStarted
        }
        $processStarted = $true
    }
    catch {
        Write-Diagnostic "hold-single-instance-lock.ps1: failed to start '$ChildCommand': $($_.Exception.Message)"
        exit $exitHarnessChildNotStarted
    }

    # Drain both redirected streams asynchronously so neither pipe can fill while
    # the parent is waiting for the other stream or for the process itself.
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $exitedBeforeTimeout = $process.WaitForExit($TimeoutSeconds * 1000)
    if (-not $exitedBeforeTimeout) {
        try {
            $process.Kill()
        }
        catch [System.InvalidOperationException] {
            # The child exited between the timed wait and Kill; it still missed the
            # deadline, and the unconditional wait below completes stream draining.
        }
    }

    # The parameterless overload completes asynchronous output handling after the
    # finite wait (or Kill) before the task results and ExitCode are observed.
    $process.WaitForExit()

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($StdoutFile, $stdout.Result, $utf8NoBom)
    [System.IO.File]::WriteAllText($StderrFile, $stderr.Result, $utf8NoBom)

    if (-not $exitedBeforeTimeout) {
        Write-Diagnostic "hold-single-instance-lock.ps1: '$ChildCommand' did not exit within $TimeoutSeconds seconds and was killed."
        exit $exitHarnessChildTimedOut
    }

    Set-Content -LiteralPath $ExitCodeFile -Value ([string] $process.ExitCode) -Encoding Ascii
}
finally {
    if ($null -ne $process) {
        try {
            if ($processStarted) {
                try {
                    if (-not $process.HasExited) {
                        $process.Kill()
                    }
                }
                catch [System.InvalidOperationException] {
                    # The child exited between HasExited and Kill, so there is no
                    # remaining process to terminate before releasing the mutex.
                }

                # An exception after Start must not release the mutex while its
                # child is still alive. Normal paths have already completed this.
                $process.WaitForExit()
            }
        }
        finally {
            $process.Dispose()
        }
    }

    # Released before the handle is disposed, and only when this thread owns it:
    # ReleaseMutex from a non-owning thread throws. Disposing without releasing
    # would abandon the mutex, which the guard recovers from -- but abandonment is
    # meant to mean "the owner died", and producing it on every clean run would
    # empty that signal of content.
    if ($held) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}

exit 0
