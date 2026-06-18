#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace libbsa::detail {

/// Reads fixed-width little-endian archive fields from a bounded byte span.
///
/// The reader never advances after failed reads, which lets format parsers
/// report truncation without losing the offset that caused the failure.
class binary_reader {
   public:
    /// Creates a reader over immutable archive bytes owned by the caller.
    explicit binary_reader(std::span<const std::byte> bytes) noexcept;

    /// Returns the current byte offset from the start of the input span.
    [[nodiscard]] std::size_t position() const noexcept;

    /// Returns the number of bytes available before a read would be truncated.
    [[nodiscard]] std::size_t remaining() const noexcept;

    /// Reads an unsigned 8-bit value and advances one byte on success.
    result<std::uint8_t> read_u8();

    /// Reads an unsigned 16-bit little-endian value and advances two bytes.
    result<std::uint16_t> read_u16_le();

    /// Reads an unsigned 32-bit little-endian value and advances four bytes.
    result<std::uint32_t> read_u32_le();

    /// Reads an unsigned 64-bit little-endian value and advances eight bytes.
    result<std::uint64_t> read_u64_le();

    /// Returns `count` bytes from the current position and advances on success.
    result<std::span<const std::byte>> read_bytes(std::size_t count);

    /// Skips `count` bytes without exposing them and advances on success.
    result<void> skip(std::size_t count);

   private:
    [[nodiscard]] bool can_read(std::size_t count) const noexcept;

    std::span<const std::byte> bytes_;
    std::size_t position_{0};
};

/// Writes little-endian archive fields into an owned byte buffer.
class binary_writer {
   public:
    /// Appends an unsigned 8-bit value.
    result<void> write_u8(std::uint8_t value);

    /// Appends an unsigned 16-bit value in little-endian byte order.
    result<void> write_u16_le(std::uint16_t value);

    /// Appends an unsigned 32-bit value in little-endian byte order.
    result<void> write_u32_le(std::uint32_t value);

    /// Appends an unsigned 64-bit value in little-endian byte order.
    result<void> write_u64_le(std::uint64_t value);

    /// Appends raw bytes preserving their exact order.
    result<void> write_bytes(std::span<const std::byte> bytes);

    /// Returns all bytes written so far.
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept;

   private:
    std::vector<std::byte> bytes_;
};

}  // namespace libbsa::detail
