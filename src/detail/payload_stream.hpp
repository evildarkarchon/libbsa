#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa
{

  class payload_sink;

} // namespace libbsa

namespace libbsa::detail
{

  /// Bounded synchronous source for archive payload bytes.
  class payload_source
  {
  public:
    virtual ~payload_source() = default;

    /// Reads up to `destination.size()` bytes and returns the number produced.
    virtual result<std::size_t> read(std::span<std::byte> destination) = 0;

    /// Returns the exact number of bytes still available from this payload.
    [[nodiscard]] virtual std::size_t remaining() const noexcept = 0;
  };

  /// Bounded synchronous sink for archive payload bytes.
  class payload_sink
  {
  public:
    virtual ~payload_sink() = default;

    /// Writes `bytes` and returns the number accepted by the sink.
    virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
  };

  /// Copies all remaining payload bytes using bounded chunks.
  ///
  /// Partial sink acceptance is reported as `io_error` so callers never observe an
  /// ambiguous partial-success extraction or packing operation.
  result<void> transfer_payload(payload_source &source, payload_sink &sink, std::size_t chunk_size);

  /// Converts an archive-declared payload size to a platform allocation size.
  result<std::size_t> checked_payload_size(std::uint64_t value, std::string_view description);

  /// Converts an archive-declared payload size to a single-vector allocation size.
  result<std::size_t> checked_materialized_payload_size(std::uint64_t value, std::string_view description);

  /// Validates that an archive-controlled byte range is representable by host streams.
  result<void> validate_payload_stream_range(std::uint64_t offset, std::uint64_t size, std::string_view description);

  /// Writes one payload span and fails if the sink accepts only part of it.
  result<void> write_payload_exact(libbsa::payload_sink &sink,
                                   std::span<const std::byte> bytes,
                                   std::string_view description);

  /// Writes a payload span using bounded chunks.
  result<void> write_payload_chunks(libbsa::payload_sink &sink,
                                    std::span<const std::byte> bytes,
                                    std::size_t chunk_size,
                                    std::string_view description);

  /// Reads exactly one archive byte range from an already-open host stream.
  result<std::vector<std::byte>> read_payload_bytes_at(std::ifstream &input,
                                                       std::uint64_t offset,
                                                       std::uint64_t size,
                                                       std::string_view description);

  /// Streams an archive byte range to a sink without allocating the whole payload.
  result<void> stream_payload_range(std::ifstream &input,
                                    std::uint64_t offset,
                                    std::uint64_t size,
                                    libbsa::payload_sink &sink,
                                    std::size_t chunk_size,
                                    std::string_view description);

} // namespace libbsa::detail
