#pragma once

/// Marks public symbols that must cross the dynamic-library boundary.
#if defined(_WIN32)
#  if defined(LIBBSA_SHARED)
#    if defined(LIBBSA_BUILDING_LIBRARY)
#      define LIBBSA_API __declspec(dllexport)
#    else
#      define LIBBSA_API __declspec(dllimport)
#    endif
#  else
#    define LIBBSA_API
#  endif
#elif defined(LIBBSA_SHARED) && defined(__GNUC__) && __GNUC__ >= 4
#  define LIBBSA_API __attribute__((visibility("default")))
#else
#  define LIBBSA_API
#endif
