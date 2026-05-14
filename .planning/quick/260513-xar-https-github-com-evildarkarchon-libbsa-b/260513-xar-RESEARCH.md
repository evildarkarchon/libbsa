# Quick Research: Decode public host paths as UTF-8 on Windows

**Researched:** 2026-05-14  
**Target:** `src/detail/host_file_path.cpp`  
**Confidence:** HIGH

## Summary

The safest plan is to stop constructing `std::filesystem::path` directly from the public `std::string_view` and instead perform an explicit UTF-8-to-UTF-16 conversion at the `resolve_host_file_path()` boundary using `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)`, then build `std::filesystem::path` from the resulting wide string. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar] [CITED: https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page] [CITED: https://learn.microsoft.com/en-us/cpp/standard-library/path-class?view=msvc-170]

This fits the existing libbsa architecture: Phase 13 already established `detail::resolve_host_file_path()` as the one shared public-host-path intake point, and all later open/validate/parser/reopen flows already consume `detail::host_file_path::resolved`. [VERIFIED: `src/detail/host_file_path.cpp`] [VERIFIED: `src/detail/host_file_path.hpp`] [VERIFIED: `src/archive.cpp`] [VERIFIED: `src/detail/host_file.cpp`] [VERIFIED: `src/validation.cpp`]

## Project Constraints (from AGENTS.md)

- Windows-only/MSVC/vcpkg is the supported target, so a Win32 conversion call at the host-path boundary is within project scope. [VERIFIED: `AGENTS.md`] 
- Do not add speculative third-party dependencies; prefer the standard library or platform APIs when justified. [VERIFIED: `AGENTS.md`] 
- Keep the public API reusable and dependency-light; implementation details may stay internal. [VERIFIED: `AGENTS.md`] 
- Add focused regression tests for compatibility/correctness behavior. [VERIFIED: `AGENTS.md`] 

## Best Pattern

1. Keep the public API as UTF-8 `std::string_view`. [VERIFIED: `include/libbsa/archive.hpp`] [VERIFIED: `tests/unit/host_path_correctness_boundary_tests.cpp`] 
2. In `resolve_host_file_path()`, copy the original bytes into `original_utf8` unchanged for diagnostics. [VERIFIED: `src/detail/host_file_path.hpp`] 
3. Convert that byte sequence with `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)`. Microsoft documents `CP_UTF8` as the explicit UTF-8 code page and documents `MB_ERR_INVALID_CHARS` as the strict mode that fails on invalid UTF-8 instead of silently replacing or dropping bad sequences. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar] 
4. Build `std::filesystem::path` from the resulting `std::wstring`, which matches the native Windows `wchar_t` path representation used by MSVC's `std::filesystem::path`. [CITED: https://learn.microsoft.com/en-us/cpp/standard-library/path-class?view=msvc-170] 

### Recommended implementation shape

```cpp
// Derived from Microsoft Learn guidance for MultiByteToWideChar + MSVC path docs.
result<host_file_path> resolve_host_file_path(std::string_view host_path) {
  std::string original{host_path};

  if (original.empty()) {
    return host_file_path{std::move(original), std::filesystem::path{}};
  }

  const int wide_size = ::MultiByteToWideChar(
      CP_UTF8,
      MB_ERR_INVALID_CHARS,
      original.data(),
      static_cast<int>(original.size()),
      nullptr,
      0);
  if (wide_size <= 0) {
    return error{error_code::invalid_argument, "archive path is not valid UTF-8"};
  }

  std::wstring wide(static_cast<std::size_t>(wide_size), L'\0');
  const int converted = ::MultiByteToWideChar(
      CP_UTF8,
      MB_ERR_INVALID_CHARS,
      original.data(),
      static_cast<int>(original.size()),
      wide.data(),
      wide_size);
  if (converted != wide_size) {
    return error{error_code::invalid_argument, "archive path is not valid UTF-8"};
  }

  return host_file_path{std::move(original), std::filesystem::path{std::move(wide)}};
}
```

Using `std::filesystem::path{std::string{host_path}}` as the UTF-8 boundary is not trustworthy enough on MSVC/Windows for this API contract; the Microsoft STL issue tracker contains a direct report showing ordinary narrow string construction being interpreted through the active code page instead of UTF-8, while `u8` input behaves differently. [CITED: https://github.com/microsoft/STL/issues/2511]

## Common Pitfalls

- **Do not use `CP_ACP` or thread ANSI code pages.** Microsoft explicitly warns those code pages can differ across machines and recommends using `CP_UTF8` explicitly. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar] [CITED: https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page]
- **Do not skip `MB_ERR_INVALID_CHARS`.** Without it, invalid UTF-8 may be replaced instead of rejected on modern Windows. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar]
- **Do not rely on `.string()` in new non-ASCII tests.** The existing public-path regression suite already converts native paths back to explicit UTF-8 with `path.u8string()` before calling the API, which is the correct test pattern here. [VERIFIED: `tests/unit/host_path_correctness_boundary_tests.cpp`]
- **Do not add conversion logic at every reopen.** The codebase already centralized path resolution in `resolve_host_file_path()` and downstream I/O uses `host_file_path::resolved`; keep the fix there. [VERIFIED: `src/archive.cpp`] [VERIFIED: `src/detail/host_file.cpp`]

## Integration Points

- **Primary code change:** `src/detail/host_file_path.cpp`. [VERIFIED: `src/detail/host_file_path.cpp`]
- **Header/API shape likely unchanged:** `src/detail/host_file_path.hpp` already carries both `original_utf8` and `resolved`. [VERIFIED: `src/detail/host_file_path.hpp`]
- **Main runtime consumers already benefit automatically:** `archive_reader::open`, `validate_archive`, parser entry points, and reopen/extract paths all flow through the resolved path object. [VERIFIED: `src/archive.cpp`] [VERIFIED: `src/validation.cpp`] [VERIFIED: `src/detail/host_file.cpp`]
- **Writer-side callers using `resolve_host_file_path()` also pick up the fix automatically.** [VERIFIED: `src/formats/bsa/tes4_bsa_prepare.cpp`] [VERIFIED: `src/formats/bsa/tes4_bsa_layout.cpp`] [VERIFIED: `src/formats/ba2/ba2_gnrl_prepare.cpp`] [VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`]

## Test Guidance

- Keep the existing black-box `host_path_correctness_boundary` suite as the main regression proof for open/validate/extract over non-ASCII public paths. [VERIFIED: `tests/unit/host_path_correctness_boundary_tests.cpp`]
- Add one focused unit test for `resolve_host_file_path()` that creates a native non-ASCII filesystem path, converts it to explicit UTF-8 with `u8string()`, resolves it, and checks `resolved == original native path`. [ASSUMED]
- Add one invalid-UTF-8 unit test if the team wants a locked error contract for malformed public input. Returning `error_code::invalid_argument` is the most consistent fit with current API behavior, but that specific mapping is not yet established in the codebase. [ASSUMED]

## Open Question

1. **What exact error message/code should malformed UTF-8 return?**  
   The architecture strongly supports failing early in `resolve_host_file_path()`, but the exact libbsa error contract for invalid UTF-8 bytes is not yet codified. `invalid_argument` looks most consistent with the existing empty-path guard, but this should be an explicit planning choice. [VERIFIED: `src/archive.cpp`] [ASSUMED]

## Sources

- Microsoft Learn: `MultiByteToWideChar` API docs — strict UTF-8 conversion rules, `CP_UTF8`, `MB_ERR_INVALID_CHARS`. [CITED: https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar]
- Microsoft Learn: UTF-8 code pages in Windows apps — recommends `CP_UTF8` explicitly and explains ACP caveats. [CITED: https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page]
- Microsoft Learn: MSVC `std::filesystem::path` docs — Windows `value_type`/native storage is `wchar_t`. [CITED: https://learn.microsoft.com/en-us/cpp/standard-library/path-class?view=msvc-170]
- Microsoft STL issue #2511 — direct evidence that ordinary narrow `path` construction on MSVC/Windows is not a safe UTF-8 boundary. [CITED: https://github.com/microsoft/STL/issues/2511]
- Local codebase inspection for current seam/integration points. [VERIFIED: `src/detail/host_file_path.cpp`] [VERIFIED: `src/detail/host_file_path.hpp`] [VERIFIED: `src/archive.cpp`] [VERIFIED: `src/detail/host_file.cpp`] [VERIFIED: `tests/unit/host_path_correctness_boundary_tests.cpp`]
