#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace libbsa {

/// Provides caller-owned random-access bytes to archive operations.
///
/// Implementations must report the complete readable extent and reject reads
/// that cannot be satisfied exactly. The source object and backing storage are
/// owned by the caller for the duration of each operation.
class byte_source {
public:
    virtual ~byte_source() = default;

    /// Returns the number of bytes available from this source.
    [[nodiscard]] virtual std::uint64_t size() const noexcept = 0;

    /// Reads exactly `destination.size()` bytes starting at `offset`.
    ///
    /// Returns `error_code::io_failure` when the requested range lies outside
    /// the source extent; successful reads fill the destination span in order.
    [[nodiscard]] virtual result<void> read_at(std::uint64_t offset, std::span<std::byte> destination) const = 0;
};

/// Receives bytes produced by future archive extraction operations.
///
/// The sink does not prescribe where bytes are stored; callers keep ownership
/// of the concrete sink object for the duration of each write operation.
class byte_sink {
public:
    virtual ~byte_sink() = default;

    /// Writes the supplied bytes to the sink in order.
    [[nodiscard]] virtual result<void> write(std::span<const std::byte> bytes) = 0;
};

/// Non-owning byte source backed by a caller-owned memory span.
///
/// The caller must keep the referenced bytes alive and unchanged for any read
/// operation that uses this source.
class memory_source final : public byte_source {
public:
    /// Creates a source view over caller-owned memory.
    explicit memory_source(std::span<const std::byte> bytes) noexcept;

    /// Returns the size of the backing span.
    [[nodiscard]] std::uint64_t size() const noexcept override;

    /// Copies a bounded range from the backing span into `destination`.
    [[nodiscard]] result<void> read_at(std::uint64_t offset, std::span<std::byte> destination) const override;

private:
    std::span<const std::byte> bytes_;
};

/// Owning byte sink that appends writes to an in-memory buffer.
class memory_sink final : public byte_sink {
public:
    /// Appends `bytes` to the owned buffer.
    [[nodiscard]] result<void> write(std::span<const std::byte> bytes) override;

    /// Returns the bytes written so far.
    [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept;

private:
    std::vector<std::byte> bytes_;
};

} // namespace libbsa
