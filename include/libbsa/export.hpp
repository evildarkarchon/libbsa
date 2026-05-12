#pragma once

/// Defines libbsa's public DLL import/export decoration.
///
/// CMake defines `LIBBSA_BUILDING_LIBRARY` only while compiling the shared
/// library itself, and defines `LIBBSA_STATIC_DEFINE` for static library
/// builds and consumers. Shared-library consumers see `dllimport`; static
/// builds keep the public headers undecorated.
#if defined(LIBBSA_STATIC_DEFINE)
#define LIBBSA_API
#elif defined(LIBBSA_BUILDING_LIBRARY)
#define LIBBSA_API __declspec(dllexport)
#else
#define LIBBSA_API __declspec(dllimport)
#endif
