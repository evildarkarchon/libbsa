#include <detail/payload_stream.hpp>

#include <algorithm>
#include <vector>

namespace libbsa::detail {

result<void> transfer_payload(payload_source& source, payload_sink& sink, std::size_t chunk_size) {
  if (chunk_size == 0) {
    return libbsa::error{libbsa::error_code::invalid_argument, "payload chunk size must be nonzero"};
  }

  std::vector<std::byte> scratch(chunk_size);
  while (source.remaining() > 0) {
    const auto requested = std::min(chunk_size, source.remaining());
    auto read = source.read(std::span<std::byte>{scratch.data(), requested});
    if (!read) {
      return read.error();
    }
    if (read.value() > requested) {
      return libbsa::error{libbsa::error_code::io_error, "payload source exceeded requested chunk size"};
    }
    if (read.value() == 0) {
      return libbsa::error{libbsa::error_code::io_error, "payload source made no progress"};
    }

    const auto bytes = std::span<const std::byte>{scratch.data(), read.value()};
    auto written = sink.write(bytes);
    if (!written) {
      return written.error();
    }
    if (written.value() != bytes.size()) {
      return libbsa::error{libbsa::error_code::io_error, "payload sink accepted a partial chunk"};
    }
  }

  return {};
}

} // namespace libbsa::detail
