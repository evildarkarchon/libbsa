---
id: T02
parent: S01
milestone: M001-k9wo8b
key_files:
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
  - tests/CMakeLists.txt
key_decisions:
  - Keep matrix-contract enforcement as lightweight token/policy tests over public docs instead of requiring copyrighted fixtures or optional local corpora.
  - Require `coverage_audit_matrix` in test names/tags so `ctest -R coverage_audit_matrix` discovers the policy suite directly.
duration: 
verification_result: passed
completed_at: 2026-05-20T01:36:37.237Z
blocker_discovered: false
---

# T02: Added Catch2 policy coverage that locks the coverage audit matrix contract into the default test suite.

**Added Catch2 policy coverage that locks the coverage audit matrix contract into the default test suite.**

## What Happened

Created `tests/unit/coverage_audit_matrix_docs_tests.cpp` following the existing docs-policy helper pattern: it reads public docs through `LIBBSA_SOURCE_DIR`, reports missing tokens with `INFO`, and keeps checks fixture-free. The new tests cover the current archive families, all required support axes, status vocabulary plus ranked gap policy, advisory/local-evidence separation, the `compatibility_matrix.json` malformed-submatrix boundary, absence of internal planning identifiers, and discoverability from `docs/compatibility-evidence.md`. Registered the source in `tests/CMakeLists.txt` next to the existing docs policy tests so Catch2 discovery exposes `coverage_audit_matrix` CTest cases.

## Verification

Verified configure, build, focused CTest discovery/execution, and the docs-presence slice check. The earlier failed POSIX `test`/`grep` evidence was replaced with a Windows-safe Python check because this Windows workspace runs verification through `cmd.exe`; the matrix content and compatibility-evidence link both pass.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `cmd.exe //d //s //c "set LOCALAPPDATA=C:\Users\evild\AppData\Local&& set APPDATA=C:\Users\evild\AppData\Roaming&& set VCPKG_ROOT=C:\vcpkg&& cmake --preset windows-msvc-debug-static"` | 0 | ✅ pass | 791ms |
| 2 | `cmd.exe //d //s //c "set LOCALAPPDATA=C:\Users\evild\AppData\Local&& set APPDATA=C:\Users\evild\AppData\Roaming&& set VCPKG_ROOT=C:\vcpkg&& cmake --build --preset windows-msvc-debug-static --target libbsa_tests"` | 0 | ✅ pass | 1122ms |
| 3 | `cmd.exe //d //s //c "set LOCALAPPDATA=C:\Users\evild\AppData\Local&& set APPDATA=C:\Users\evild\AppData\Roaming&& set VCPKG_ROOT=C:\vcpkg&& ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure"` | 0 | ✅ pass: 7 coverage_audit_matrix tests passed | 179ms |
| 4 | `python - <<'PY'
from pathlib import Path
matrix = Path('docs/coverage-audit-matrix.md')
compat = Path('docs/compatibility-evidence.md')
assert matrix.is_file() and matrix.stat().st_size > 0, matrix
matrix_text = matrix.read_text(encoding='utf-8')
compat_text = compat.read_text(encoding='utf-8')
for token in ['TES3 BSA', 'TES4-family BSA', 'BA2 GNRL', 'BA2 DX10', 'Ranked gap list']:
    assert token in matrix_text, token
assert 'coverage-audit-matrix.md' in compat_text
print('coverage audit matrix docs check passed')
PY` | 0 | ✅ pass | 82ms |

## Deviations

Implementation followed the plan. Verification was invoked through `cmd.exe` with Windows environment variables because `gsd_exec` uses a bash runtime on this host; MSYS slash conversion requires `cmd.exe //d //s //c`, and the first bare bash configure lacked Windows app-data variables for vcpkg.

## Known Issues

None in tracked code. The local vcpkg/Visual Studio discovery path can fail before project configuration if invoked from the bash runtime without the Windows shell/environment wrapper; the successful verification used the wrapper and existing dependency-ready build tree.

## Files Created/Modified

- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/CMakeLists.txt`
