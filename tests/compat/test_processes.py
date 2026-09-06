"""Exercise the real Windows subprocess boundary, including descendant cleanup."""

import ctypes
from ctypes import wintypes
from pathlib import Path
import sys
import tempfile
import unittest

import processes


@unittest.skipUnless(sys.platform == "win32", "Windows Job Objects are required")
class ProcessContracts(unittest.TestCase):
    """Require complete process cleanup and bounded execution, not just an exception."""

    def setUp(self):
        """Give each test its own output directory and ordinary generous bounds."""
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.addCleanup(self.temporary.cleanup)
        self.log = self.root / "process.log"
        self.options = dict(log=self.log, cwd=self.root, timeout=10.0,
                            memory_bytes=512 * 1024 * 1024,
                            scratch_bytes=512 * 1024 * 1024)

    def run_python(self, script, **overrides):
        """Run an actual Python child with selected resource constraints."""
        processes.run_process([sys.executable, "-c", script], **(self.options | overrides))

    def assert_process_ended(self, pid):
        """Check the kernel process state independently of the runner's result."""
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        kernel.OpenProcess.restype = wintypes.HANDLE
        kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        kernel.WaitForSingleObject.restype = wintypes.DWORD
        kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        kernel.CloseHandle.restype = wintypes.BOOL
        handle = kernel.OpenProcess(0x00100000, False, pid)
        if not handle:
            self.assertEqual(ctypes.get_last_error(), 87)
            return
        try:
            self.assertEqual(kernel.WaitForSingleObject(handle, 3000), 0)
        finally:
            kernel.CloseHandle(handle)

    def test_arguments_are_literal_and_both_output_streams_are_logged(self):
        """Shell metacharacters and spaces reach argv unchanged and stay local."""
        literal = 'space & $(echo secret) "quote" \\tail'
        script = "import json,sys; print(json.dumps(sys.argv[1])); print('stderr marker',file=sys.stderr)"
        processes.run_process([sys.executable, "-c", script, literal], **self.options)
        import json
        lines = self.log.read_text().splitlines()
        self.assertIn(json.dumps(literal), lines)
        self.assertIn("stderr marker", lines)

    def test_nonzero_exit_reports_only_local_log_path(self):
        """Child diagnostics must not be copied into raised error text."""
        with self.assertRaises(RuntimeError) as raised:
            self.run_python("import sys; print('private fixture detail',file=sys.stderr); sys.exit(7)")
        self.assertIn(str(self.log), str(raised.exception))
        self.assertNotIn("private fixture detail", str(raised.exception))
        self.assertIn("7", str(raised.exception))

    def test_exit_metadata_is_distinct_from_resource_failures(self):
        """Only a real tool exit may be considered for a reviewed oracle exception."""
        with self.assertRaises(RuntimeError) as exited:
            self.run_python("import sys; sys.exit(7)")
        self.assertEqual(getattr(exited.exception, "exit_code", None), 7)
        self.assertEqual(getattr(exited.exception, "log", None), self.log)
        with self.assertRaises(RuntimeError) as timed_out:
            self.run_python("import time; time.sleep(30)", timeout=0.2)
        self.assertIsNone(getattr(timed_out.exception, "exit_code", None))

    def test_timeout_kills_parent_and_descendant(self):
        """A timeout cannot leave a child holding corpus scratch files open."""
        script = (
            "import os,pathlib,subprocess,sys,time; "
            "child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(60)']); "
            "pathlib.Path('pids.txt').write_text(str(os.getpid())+' '+str(child.pid)); "
            "time.sleep(60)"
        )
        with self.assertRaisesRegex(RuntimeError, "timeout"):
            self.run_python(script, timeout=1.0)
        pids = [int(value) for value in (self.root / "pids.txt").read_text().split()]
        for pid in pids:
            self.assert_process_ended(pid)

    def test_success_cleans_up_lingering_descendants(self):
        """Even a successful parent cannot leave an unowned worker running."""
        self.run_python(
            "import pathlib,subprocess,sys; "
            "child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(60)']); "
            "pathlib.Path('child.txt').write_text(str(child.pid))"
        )
        self.assert_process_ended(int((self.root / "child.txt").read_text()))

    def test_timeout_releases_inherited_log_before_returning(self):
        """A resource exception must leave its local log immediately removable."""
        for _ in range(8):
            with self.assertRaisesRegex(RuntimeError, "timeout"):
                self.run_python("import time; time.sleep(60)", timeout=0.1)
            self.log.unlink()

    def test_memory_limit_kills_child_that_catches_allocation_failure(self):
        """A trapped allocation failure must still end the complete job."""
        script = (
            "import os,pathlib,time\n"
            "pathlib.Path('pid.txt').write_text(str(os.getpid()))\n"
            "blocks=[]\n"
            "try:\n"
            " while True: blocks.append(bytearray(8*1024*1024))\n"
            "except MemoryError:\n"
            " time.sleep(60)\n"
        )
        with self.assertRaisesRegex(RuntimeError, "memory"):
            self.run_python(script, memory_bytes=96 * 1024 * 1024)
        self.assert_process_ended(int((self.root / "pid.txt").read_text()))

    def test_scratch_limit_kills_child_and_preserves_caller_files(self):
        """Resource enforcement kills processes without deleting their evidence."""
        sentinel = self.root / "owned-by-caller.txt"
        sentinel.write_text("keep")
        script = (
            "import os,pathlib,time; "
            "pathlib.Path('pid.txt').write_text(str(os.getpid())); "
            "pathlib.Path('scratch.bin').write_bytes(os.urandom(16*1024*1024)); "
            "time.sleep(60)"
        )
        with self.assertRaisesRegex(RuntimeError, "scratch"):
            self.run_python(script, scratch_bytes=1024 * 1024)
        self.assert_process_ended(int((self.root / "pid.txt").read_text()))
        self.assertEqual(sentinel.read_text(), "keep")
        self.assertTrue((self.root / "scratch.bin").is_file())


if __name__ == "__main__":
    unittest.main()
