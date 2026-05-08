#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace libbsa {

/// Stable error categories returned by libbsa public APIs.
///
/// The enum intentionally stays small and format-neutral so callers can branch
/// on durable categories without depending on parser implementation details.
enum class error_code {
  unsupported,
  invalid_argument,
  not_found,
  io_error,
  format_error,
};

/// Structured error value returned by result-producing libbsa APIs.
///
/// `code` is stable and suitable for tests and programmatic handling. `message`
/// is diagnostic text for humans and should not be compared exactly by tests.
struct error {
  error_code code;
  std::string message;
};

/// C++20-compatible result type used by libbsa public APIs.
///
/// This is a deliberately small expected-like type. I/O and format failures are
/// represented as `error` values. Calling `value()` on an error result or
/// `error()` on a successful result is a programmer mistake and throws
/// `std::logic_error`.
template <typename T>
class result {
 public:
  /// Creates a successful result containing `value`.
  result(T value) : storage_(std::move(value)) {}

  /// Creates a failed result containing `err`.
  result(error err) : storage_(std::move(err)) {}

  /// Returns true when this result contains a value.
  [[nodiscard]] bool has_value() const noexcept {
    return std::holds_alternative<T>(storage_);
  }

  /// Returns true when this result contains a value.
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

  /// Returns the contained value, or throws if this result contains an error.
  T& value() & {
    if (!has_value()) {
      throw std::logic_error("libbsa::result has no value");
    }
    return std::get<T>(storage_);
  }

  /// Returns the contained value, or throws if this result contains an error.
  const T& value() const& {
    if (!has_value()) {
      throw std::logic_error("libbsa::result has no value");
    }
    return std::get<T>(storage_);
  }

  /// Returns the contained value, or throws if this result contains an error.
  T&& value() && {
    if (!has_value()) {
      throw std::logic_error("libbsa::result has no value");
    }
    return std::move(std::get<T>(storage_));
  }

  /// Returns the contained error, or throws if this result contains a value.
  [[nodiscard]] const libbsa::error& error() const {
    if (has_value()) {
      throw std::logic_error("libbsa::result has no error");
    }
    return std::get<libbsa::error>(storage_);
  }

 private:
  std::variant<T, libbsa::error> storage_;
};

/// C++20-compatible result specialization for operations that return no value.
template <>
class result<void> {
 public:
  /// Creates a successful void result.
  result() noexcept = default;

  /// Creates a failed void result containing `err`.
  result(error err) : error_(std::move(err)), has_value_(false) {}

  /// Returns true when this result represents success.
  [[nodiscard]] bool has_value() const noexcept { return has_value_; }

  /// Returns true when this result represents success.
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

  /// Throws if this result contains an error.
  void value() const {
    if (!has_value()) {
      throw std::logic_error("libbsa::result<void> has no value");
    }
  }

  /// Returns the contained error, or throws if this result represents success.
  [[nodiscard]] const libbsa::error& error() const {
    if (has_value()) {
      throw std::logic_error("libbsa::result<void> has no error");
    }
    return error_;
  }

 private:
  libbsa::error error_{error_code::unsupported, {}};
  bool has_value_{true};
};

} // namespace libbsa
