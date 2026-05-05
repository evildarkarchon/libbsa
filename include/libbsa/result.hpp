#pragma once

#include <string>

namespace libbsa {

/// Identifies the foundational categories of failures reported by libbsa APIs.
enum class error_code {
    unsupported_format,
    malformed_archive,
    io_failure,
    decompression_failure,
};

/// Describes a structured libbsa failure with a stable code and human-readable message.
struct error {
    error_code code;
    std::string message;
};

/// C++20-compatible result type for fallible operations that return a value.
///
/// The full observer and construction surface is introduced by the dedicated result API plan.
template <class T>
class result;

/// C++20-compatible result specialization for fallible operations with no return value.
///
/// This specialization reserves the public API name without exposing C++23-only facilities.
template <>
class result<void>;

} // namespace libbsa
