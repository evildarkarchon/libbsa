---
id: 260510-7if
status: complete
date: 2026-05-10
commit: uncommitted
---

# Quick Task 260510-7if Summary

## Completed

- Added root `README.md` and `CLAUDE.md` entries that declare libbsa Windows-only.
- Updated `AGENTS.md`, current planning docs, and PRD language so review agents do not treat Linux, macOS, POSIX, or cross-platform portability as active scope.
- Removed the `linux-clang-asan-ubsan` configure/build/test presets from `CMakePresets.json`.
- Replaced the sanitizer-profile fixture policy with a Windows-only testing policy.
- Updated the validation policy test to assert the Windows-only docs and absence of the removed Linux preset.

## Verification

- `cmake --list-presets=all` listed only `windows-msvc-debug-static` and `windows-msvc-debug-shared` across configure/build/test presets.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` passed.
- `ctest --preset windows-msvc-debug-static -R "configured build profiles are Windows-only|validation_policy|public_include_boundary" --output-on-failure` passed 5/5 tests.
- `git diff --check` passed.
- `git status --short TES5Edit` returned clean.

## Notes

No comments were removed or rewritten.
