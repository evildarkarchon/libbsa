#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <span>

namespace libbsa::detail {

/// Bounded synchronous source for archive payload bytes.
class payload_source {
 public:
  virtual ~payload_source() = default;

  /// Reads up to `destination.size()` bytes and returns the number produced.
  virtual result<std::size_t> read(std::span<std::byte> destination) = 0;

  /// Returns the exact number of bytes still available from this payload.
  [[nodiscard]] virtual std::size_t remaining() const noexcept = 0;
};

/// Bounded synchronous sink for archive payload bytes.
class payload_sink {
 public:
  virtual ~payload_sink() = default;

  /// Writes `bytes` and returns the number accepted by the sink.
  virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
};

/// Copies all remaining payload bytes using bounded chunks.
///
/// Partial sink acceptance is reported as `io_error` so callers never observe an
/// ambiguous partial-success extraction or packing operation.
result<void> transfer_payload(payload_source& source, payload_sink& sink, std::size_t chunk_size);

} // namespace libbsa::detail
