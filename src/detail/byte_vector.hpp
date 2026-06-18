#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::detail {

/// Returns a byte-vector allocation error without letting archive-controlled
/// sizes throw.
inline error byte_vector_allocation_error(std::string_view description) {
    return error{error_code::format_error,
                 std::string{description} + " exceeds platform vector limits"};
}

/// Creates a byte vector while translating allocation failures into libbsa
/// results.
inline result<std::vector<std::byte>> make_byte_vector(std::size_t size,
                                                       std::string_view description) {
    if (size > std::vector<std::byte>{}.max_size()) {
        return byte_vector_allocation_error(description);
    }

    try {
        return std::vector<std::byte>(size);
    } catch (const std::bad_alloc&) {
        return byte_vector_allocation_error(description);
    } catch (const std::length_error&) {
        return byte_vector_allocation_error(description);
    }
}

/// Reserves byte-vector capacity while translating allocation failures into
/// libbsa results.
inline result<void> reserve_byte_vector(std::vector<std::byte>& bytes, std::size_t capacity,
                                        std::string_view description) {
    if (capacity > bytes.max_size()) {
        return byte_vector_allocation_error(description);
    }

    try {
        bytes.reserve(capacity);
    } catch (const std::bad_alloc&) {
        return byte_vector_allocation_error(description);
    } catch (const std::length_error&) {
        return byte_vector_allocation_error(description);
    }
    return {};
}

/// Appends a byte span while translating allocation failures into libbsa
/// results.
inline result<void> append_byte_vector(std::vector<std::byte>& bytes,
                                       std::span<const std::byte> appended,
                                       std::string_view description) {
    if (appended.size() > bytes.max_size() - bytes.size()) {
        return byte_vector_allocation_error(description);
    }

    try {
        bytes.insert(bytes.end(), appended.begin(), appended.end());
    } catch (const std::bad_alloc&) {
        return byte_vector_allocation_error(description);
    } catch (const std::length_error&) {
        return byte_vector_allocation_error(description);
    }
    return {};
}

}  // namespace libbsa::detail
