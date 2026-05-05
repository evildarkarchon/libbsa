#pragma once

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace libbsa {

/// Identifies the foundational categories of failures reported by libbsa APIs.
///
/// These categories are intentionally coarse in Phase 1 so parser-specific
/// context can be added later without exposing implementation details now.
enum class error_code {
    unsupported_format,
    malformed_archive,
    io_failure,
    decompression_failure,
};

/// Describes a structured libbsa failure with a stable code and human-readable message.
///
/// The message is intended for concise diagnostic text; richer parser-specific
/// location details are deliberately outside the foundation API.
struct error {
    error_code code;
    std::string message;
};

/// C++20-compatible result type for fallible operations that return a value.
///
/// A `result<T>` stores either a successful `T` or a structured `libbsa::error`.
/// Callers should check `has_value()` before using `value()` or `error()`;
/// calling the wrong observer is a programmer precondition violation and throws
/// `std::logic_error`.
template <class T>
class result {
    static_assert(!std::is_void_v<T>, "use result<void> for void fallible operations");

public:
    /// Returns true when this result contains a successful value.
    [[nodiscard]] bool has_value() const noexcept
    {
        return std::holds_alternative<T>(storage_);
    }

    /// Returns the contained value or throws `std::logic_error` when this is a failure.
    T& value()
    {
        if (!has_value()) {
            throw std::logic_error("libbsa::result has no value");
        }

        return std::get<T>(storage_);
    }

    /// Returns the contained value or throws `std::logic_error` when this is a failure.
    const T& value() const
    {
        if (!has_value()) {
            throw std::logic_error("libbsa::result has no value");
        }

        return std::get<T>(storage_);
    }

    /// Returns the contained error or throws `std::logic_error` when this is successful.
    const libbsa::error& error() const
    {
        if (has_value()) {
            throw std::logic_error("libbsa::result has no error");
        }

        return std::get<libbsa::error>(storage_);
    }

private:
    explicit result(T value) : storage_(std::move(value)) {}
    explicit result(libbsa::error err) : storage_(std::move(err)) {}

    std::variant<T, libbsa::error> storage_;

    template <class U>
    friend result<U> success(U value);

    template <class U>
    friend result<U> failure(libbsa::error err);
};

/// C++20-compatible result specialization for fallible operations with no return value.
///
/// `result<void>` represents operations that can fail without producing a value.
/// The same observer preconditions as `result<T>` apply: `value()` is only valid
/// for success, and `error()` is only valid for failure.
template <>
class result<void> {
public:
    /// Returns true when this result represents successful completion.
    [[nodiscard]] bool has_value() const noexcept
    {
        return std::holds_alternative<std::monostate>(storage_);
    }

    /// Observes successful completion or throws `std::logic_error` when this is a failure.
    void value() const
    {
        if (!has_value()) {
            throw std::logic_error("libbsa::result<void> has no value");
        }
    }

    /// Returns the contained error or throws `std::logic_error` when this is successful.
    const libbsa::error& error() const
    {
        if (has_value()) {
            throw std::logic_error("libbsa::result<void> has no error");
        }

        return std::get<libbsa::error>(storage_);
    }

private:
    result() : storage_(std::monostate{}) {}
    explicit result(libbsa::error err) : storage_(std::move(err)) {}

    std::variant<std::monostate, libbsa::error> storage_;

    friend result<void> success();

    template <class U>
    friend result<U> failure(libbsa::error err);

    friend result<void> failure(libbsa::error err);
};

/// Creates a successful `result<T>` containing `value`.
template <class T>
result<T> success(T value)
{
    return result<T>(std::move(value));
}

/// Creates a successful `result<void>` for operations that completed without a value.
inline result<void> success()
{
    return result<void>();
}

/// Creates a failed `result<T>` containing the supplied structured error.
template <class T>
result<T> failure(libbsa::error err)
{
    return result<T>(std::move(err));
}

/// Creates a failed `result<void>` containing the supplied structured error.
inline result<void> failure(libbsa::error err)
{
    return result<void>(std::move(err));
}

} // namespace libbsa
