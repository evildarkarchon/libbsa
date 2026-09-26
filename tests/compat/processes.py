"""Execute compatibility tools with Windows Job Object process-tree ownership."""

import ctypes
from ctypes import wintypes
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


class ProcessExitError(RuntimeError):
    """A completed tool returned nonzero, distinct from resource or teardown failure."""

    def __init__(self, exit_code: int, log: Path | None = None):
        """Retain the native exit code and local log without disclosing child output."""
        self.exit_code = exit_code
        self.log = log
        super().__init__(f"process exited with code {exit_code}" +
                         (f"; log: {log}" if log is not None else ""))


class _BasicLimits(ctypes.Structure):
    """Match JOBOBJECT_BASIC_LIMIT_INFORMATION with pointer-sized working sets."""

    _fields_ = [("PerProcessUserTimeLimit", ctypes.c_longlong),
                ("PerJobUserTimeLimit", ctypes.c_longlong),
                ("LimitFlags", wintypes.DWORD),
                ("MinimumWorkingSetSize", ctypes.c_size_t),
                ("MaximumWorkingSetSize", ctypes.c_size_t),
                ("ActiveProcessLimit", wintypes.DWORD),
                ("Affinity", ctypes.c_size_t),
                ("PriorityClass", wintypes.DWORD),
                ("SchedulingClass", wintypes.DWORD)]


class _IoCounters(ctypes.Structure):
    """Match IO_COUNTERS, whose six counters are unsigned 64-bit values."""

    _fields_ = [(name, ctypes.c_ulonglong) for name in (
        "ReadOperationCount", "WriteOperationCount", "OtherOperationCount",
        "ReadTransferCount", "WriteTransferCount", "OtherTransferCount")]


class _ExtendedLimits(ctypes.Structure):
    """Match JOBOBJECT_EXTENDED_LIMIT_INFORMATION for aggregate memory limits."""

    _fields_ = [("BasicLimitInformation", _BasicLimits), ("IoInfo", _IoCounters),
                ("ProcessMemoryLimit", ctypes.c_size_t),
                ("JobMemoryLimit", ctypes.c_size_t),
                ("PeakProcessMemoryUsed", ctypes.c_size_t),
                ("PeakJobMemoryUsed", ctypes.c_size_t)]


class _CompletionPort(ctypes.Structure):
    """Associate job notifications with an owned I/O completion port."""

    _fields_ = [("CompletionKey", ctypes.c_void_p), ("CompletionPort", wintypes.HANDLE)]


class _Accounting(ctypes.Structure):
    """Match basic job accounting so teardown can wait for every descendant."""

    _fields_ = [(name, ctypes.c_longlong) for name in (
        "TotalUserTime", "TotalKernelTime", "ThisPeriodTotalUserTime",
        "ThisPeriodTotalKernelTime")] + [(name, wintypes.DWORD) for name in (
        "TotalPageFaultCount", "TotalProcesses", "ActiveProcesses", "TotalTerminatedProcesses")]


class _StartupInfo(ctypes.Structure):
    """Match STARTUPINFOW, including full-width inherited standard handles."""

    _fields_ = [("cb", wintypes.DWORD), ("lpReserved", wintypes.LPWSTR),
                ("lpDesktop", wintypes.LPWSTR), ("lpTitle", wintypes.LPWSTR),
                ("dwX", wintypes.DWORD), ("dwY", wintypes.DWORD),
                ("dwXSize", wintypes.DWORD), ("dwYSize", wintypes.DWORD),
                ("dwXCountChars", wintypes.DWORD), ("dwYCountChars", wintypes.DWORD),
                ("dwFillAttribute", wintypes.DWORD), ("dwFlags", wintypes.DWORD),
                ("wShowWindow", wintypes.WORD), ("cbReserved2", wintypes.WORD),
                ("lpReserved2", ctypes.POINTER(wintypes.BYTE)),
                ("hStdInput", wintypes.HANDLE), ("hStdOutput", wintypes.HANDLE),
                ("hStdError", wintypes.HANDLE)]


class _ProcessInfo(ctypes.Structure):
    """Retain both initial handles until job assignment and resume are complete."""

    _fields_ = [("hProcess", wintypes.HANDLE), ("hThread", wintypes.HANDLE),
                ("dwProcessId", wintypes.DWORD), ("dwThreadId", wintypes.DWORD)]


def _kernel():
    """Declare every Win32 ABI explicitly so handles are never truncated on x64."""
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    signatures = {
        "CreateJobObjectW": ([ctypes.c_void_p, wintypes.LPCWSTR], wintypes.HANDLE),
        "SetInformationJobObject": ([wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p,
                                     wintypes.DWORD], wintypes.BOOL),
        "QueryInformationJobObject": ([wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p,
                                       wintypes.DWORD, ctypes.c_void_p], wintypes.BOOL),
        "CreateIoCompletionPort": ([wintypes.HANDLE, wintypes.HANDLE, ctypes.c_size_t,
                                    wintypes.DWORD], wintypes.HANDLE),
        "GetQueuedCompletionStatus": ([wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD),
                                        ctypes.POINTER(ctypes.c_size_t),
                                        ctypes.POINTER(ctypes.c_void_p), wintypes.DWORD], wintypes.BOOL),
        "CreateProcessW": ([wintypes.LPCWSTR, wintypes.LPWSTR, ctypes.c_void_p,
                            ctypes.c_void_p, wintypes.BOOL, wintypes.DWORD,
                            ctypes.c_void_p, wintypes.LPCWSTR, ctypes.POINTER(_StartupInfo),
                            ctypes.POINTER(_ProcessInfo)], wintypes.BOOL),
        "AssignProcessToJobObject": ([wintypes.HANDLE, wintypes.HANDLE], wintypes.BOOL),
        "ResumeThread": ([wintypes.HANDLE], wintypes.DWORD),
        "TerminateJobObject": ([wintypes.HANDLE, wintypes.UINT], wintypes.BOOL),
        "TerminateProcess": ([wintypes.HANDLE, wintypes.UINT], wintypes.BOOL),
        "WaitForSingleObject": ([wintypes.HANDLE, wintypes.DWORD], wintypes.DWORD),
        "GetExitCodeProcess": ([wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)], wintypes.BOOL),
        "CloseHandle": ([wintypes.HANDLE], wintypes.BOOL),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(kernel, name)
        function.argtypes = arguments
        function.restype = result
    return kernel


def _checked(value, operation):
    """Fail closed when an ownership or enforcement API is unavailable."""
    if not value:
        raise RuntimeError(f"{operation} failed (Windows error {ctypes.get_last_error()})")
    return value


def _notification(kernel, port, milliseconds):
    """Read one job event; return None only for a normal completion-port timeout."""
    message = wintypes.DWORD()
    key = ctypes.c_size_t()
    overlapped = ctypes.c_void_p()
    success = kernel.GetQueuedCompletionStatus(port, ctypes.byref(message), ctypes.byref(key),
                                               ctypes.byref(overlapped), milliseconds)
    if not success and ctypes.get_last_error() == 258:
        return None
    _checked(success, "Read job resource notification")
    return message.value


def _stop_tree(kernel, job, process, assigned):
    """Terminate and reap the owned tree; never remove caller filesystem data."""
    if not process.hProcess:
        return
    if assigned:
        _checked(kernel.TerminateJobObject(job, 1), "Terminate process job")
    else:
        _checked(kernel.TerminateProcess(process.hProcess, 1), "Terminate suspended process")
    deadline = time.monotonic() + 5.0
    while time.monotonic() < deadline:
        if assigned:
            accounting = _Accounting()
            _checked(kernel.QueryInformationJobObject(job, 1, ctypes.byref(accounting),
                ctypes.sizeof(accounting), None), "Read process job accounting")
            # Job accounting can reach zero before the primary process finishes
            # closing inherited handles. Its signaled state is also required so
            # callers can immediately remove or reopen the local process log.
            if (accounting.ActiveProcesses == 0 and
                    kernel.WaitForSingleObject(process.hProcess, 0) == 0):
                return
        elif kernel.WaitForSingleObject(process.hProcess, 0) == 0:
            return
        # A short poll also covers descendants after the primary process exited.
        time.sleep(0.05)
    raise RuntimeError("Process tree did not terminate within the cleanup deadline")


def run_process(args: list[str], log: Path, cwd: Path, timeout: float,
                memory_bytes: int, scratch_bytes: int) -> None:
    """Run an exact argument vector without a shell and retain stdout/stderr in log.

    The initially suspended child joins a kill-on-close Job Object before any code
    runs. Aggregate committed memory, wall time, and conservatively measured free
    space consumed on cwd's volume are bounded. Every error kills the entire job;
    normal completion also closes lingering descendants. No scratch files are deleted.
    RuntimeError reports only a terse reason and the local log path.
    """
    if sys.platform != "win32":
        raise RuntimeError("Compatibility process execution requires Windows")
    if not args or any(not isinstance(value, str) or "\0" in value for value in args):
        raise ValueError("args must be a nonempty list of strings without NUL")
    if not math.isfinite(timeout) or timeout <= 0 or memory_bytes <= 0 or scratch_bytes <= 0:
        raise ValueError("timeout, memory_bytes and scratch_bytes must be positive")
    if memory_bytes > ctypes.c_size_t(-1).value:
        raise ValueError("memory_bytes exceeds the platform job limit field")
    import msvcrt

    log = Path(log).absolute()
    cwd = Path(cwd).absolute()
    executable = shutil.which(args[0])
    if not executable:
        raise RuntimeError(f"Executable is unavailable; log: {log}")
    kernel = _kernel()
    job = port = None
    process = _ProcessInfo()
    assigned = False
    failure = None
    try:
        log.parent.mkdir(parents=True, exist_ok=True)
        baseline_free = shutil.disk_usage(cwd).free
        job = _checked(kernel.CreateJobObjectW(None, None), "Create process job")
        limits = _ExtendedLimits()
        # KILL_ON_JOB_CLOSE forbids orphan processes even if this supervisor exits.
        limits.BasicLimitInformation.LimitFlags = 0x00002000 | 0x00000200
        limits.JobMemoryLimit = memory_bytes
        _checked(kernel.SetInformationJobObject(job, 9, ctypes.byref(limits),
            ctypes.sizeof(limits)), "Configure job memory boundary")
        port = _checked(kernel.CreateIoCompletionPort(wintypes.HANDLE(-1), None, 0, 1),
                        "Create resource notification port")
        association = _CompletionPort(1, port)
        _checked(kernel.SetInformationJobObject(job, 7, ctypes.byref(association),
            ctypes.sizeof(association)), "Subscribe to job resource limits")
        with log.open("wb") as output, open(os.devnull, "rb") as input_stream:
            output_handle = msvcrt.get_osfhandle(output.fileno())
            input_handle = msvcrt.get_osfhandle(input_stream.fileno())
            os.set_handle_inheritable(output_handle, True)
            os.set_handle_inheritable(input_handle, True)
            startup = _StartupInfo()
            startup.cb = ctypes.sizeof(startup)
            startup.dwFlags = 0x00000100  # STARTF_USESTDHANDLES
            startup.hStdInput = input_handle
            startup.hStdOutput = startup.hStdError = output_handle
            command_line = ctypes.create_unicode_buffer(subprocess.list2cmdline(args))
            # Assignment while suspended prevents any descendant from escaping the
            # resource boundary between CreateProcessW and AssignProcessToJobObject.
            _checked(kernel.CreateProcessW(str(Path(executable).absolute()), command_line,
                None, None, True, 0x08000000 | 0x00000004, None, str(cwd),
                ctypes.byref(startup), ctypes.byref(process)), "Create suspended child")
            os.set_handle_inheritable(output_handle, False)
            os.set_handle_inheritable(input_handle, False)
            _checked(kernel.AssignProcessToJobObject(job, process.hProcess),
                     "Assign child to resource job (nested jobs must permit assignment)")
            assigned = True
            if kernel.ResumeThread(process.hThread) == 0xFFFFFFFF:
                raise RuntimeError("Resume assigned child failed")
            deadline = time.monotonic() + timeout
            while True:
                # Memory refusal must end the tree even when a child catches its
                # allocation error and continues; the completion port observes it.
                message = _notification(kernel, port, 100)
                while message is not None:
                    if message in (9, 10):
                        raise RuntimeError("job memory limit exceeded")
                    message = _notification(kernel, port, 0)
                if baseline_free - shutil.disk_usage(cwd).free > scratch_bytes:
                    raise RuntimeError("scratch space limit exceeded")
                state = kernel.WaitForSingleObject(process.hProcess, 0)
                if state == 0:
                    exit_code = wintypes.DWORD()
                    _checked(kernel.GetExitCodeProcess(process.hProcess, ctypes.byref(exit_code)),
                             "Read child exit status")
                    if exit_code.value:
                        raise ProcessExitError(exit_code.value)
                    break
                if state != 258:
                    raise RuntimeError("Wait for child process failed")
                if time.monotonic() >= deadline:
                    raise RuntimeError("process timeout exceeded")
    except BaseException as error:
        failure = error
    finally:
        try:
            _stop_tree(kernel, job, process, assigned)
        except BaseException as cleanup_error:
            failure = cleanup_error if failure is None else RuntimeError(
                f"{failure}; process-tree cleanup also failed")
        finally:
            for handle in (process.hThread, process.hProcess, job, port):
                if handle:
                    kernel.CloseHandle(handle)
    if failure is not None:
        if isinstance(failure, (KeyboardInterrupt, SystemExit)):
            raise failure
        if isinstance(failure, ProcessExitError):
            raise ProcessExitError(failure.exit_code, log) from None
        raise RuntimeError(f"{failure}; log: {log}") from None
