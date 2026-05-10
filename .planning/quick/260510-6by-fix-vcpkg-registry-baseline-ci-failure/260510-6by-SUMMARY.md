---
quick_id: 260510-6by
status: complete
completed: 2026-05-10
commit: ee0ca39
---

# Quick Task 260510-6by Summary

Updated vcpkg registry ownership so the pinned baseline lives only in `vcpkg-configuration.json`.

## Accomplishments

- Removed the redundant `builtin-baseline` from `vcpkg.json`, eliminating the override warning from manifest mode.
- Changed the default registry from `kind: builtin` to `kind: git` with `repository: https://github.com/microsoft/vcpkg`, keeping the existing baseline `12dcccadfe573d0eaa6c67a968413ded7805d256`.
- Preserved the existing dependency set and package baseline; no dependency-version changes were made.

## Verification

- `Get-Content vcpkg.json -Raw | ConvertFrom-Json` - passed.
- `Get-Content vcpkg-configuration.json -Raw | ConvertFrom-Json` - passed.
- `git diff --check` - passed; Git reported only normal LF-to-CRLF working-copy warnings.
- `$env:VCPKG_ROOT='C:\vcpkg'; C:\vcpkg\vcpkg.exe install --dry-run` - passed and fetched registry information from `https://github.com/microsoft/vcpkg`.
- `$env:VCPKG_ROOT='C:\vcpkg'; cmake --fresh --preset windows-msvc-debug-static` - passed.
- `$env:VCPKG_ROOT='C:\vcpkg'; cmake --fresh --preset windows-msvc-debug-shared` - passed.

## Commit

- `ee0ca39` - `fix(ci): use explicit vcpkg git registry baseline`

## Notes

- The failure happened before static/shared build differences mattered; both presets share the same manifest install path.
- The vcpkg dry-run still shows the existing libdeflate default-feature expansion. That is separate dependency policy work and was intentionally left unchanged here.
