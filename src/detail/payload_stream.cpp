#include <detail/payload_stream.hpp>

#include <detail/byte_vector.hpp>

#include <libbsa/archive.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace libbsa::detail {

namespace {

std::uint64_t streamoff_limit() noexcept {
    return static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max());
}

std::uint64_t streamsize_limit() noexcept {
    return static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max());
}

}  // namespace

result<void> transfer_payload(payload_source& source, payload_sink& sink, std::size_t chunk_size) {
    if (chunk_size == 0) {
        return libbsa::error{libbsa::error_code::invalid_argument,
                             "payload chunk size must be nonzero"};
    }

    std::vector<std::byte> scratch(chunk_size);
    while (source.remaining() > 0) {
        const auto requested = std::min(chunk_size, source.remaining());
        auto read = source.read(std::span<std::byte>{scratch.data(), requested});
        if (!read) {
            return read.error();
        }
        if (read.value() > requested) {
            return libbsa::error{libbsa::error_code::io_error,
                                 "payload source exceeded requested chunk size"};
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
            return libbsa::error{libbsa::error_code::io_error,
                                 "payload sink accepted a partial chunk"};
        }
    }

    return {};
}

result<std::size_t> checked_payload_size(std::uint64_t value, std::string_view description) {
    if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return libbsa::error{libbsa::error_code::format_error,
                             std::string{description} + " exceeds platform limits"};
    }
    return static_cast<std::size_t>(value);
}

result<std::size_t> checked_materialized_payload_size(std::uint64_t value,
                                                      std::string_view description) {
    auto checked = checked_payload_size(value, description);
    if (!checked) {
        return checked.error();
    }
    if (checked.value() > std::vector<std::byte>{}.max_size()) {
        return byte_vector_allocation_error(description);
    }
    return checked.value();
}

result<void> validate_payload_stream_range(std::uint64_t offset, std::uint64_t size,
                                           std::string_view description) {
    const auto max_offset = streamoff_limit();
    if (offset > max_offset) {
        return libbsa::error{libbsa::error_code::format_error,
                             std::string{description} + " offset exceeds stream limits"};
    }
    if (size > streamsize_limit()) {
        return libbsa::error{libbsa::error_code::format_error,
                             std::string{description} + " size exceeds stream limits"};
    }
    if (size > max_offset || offset > max_offset - size) {
        return libbsa::error{libbsa::error_code::format_error,
                             std::string{description} + " span exceeds stream limits"};
    }
    return {};
}

result<void> write_payload_exact(libbsa::payload_sink& sink, std::span<const std::byte> bytes,
                                 std::string_view description) {
    auto written = sink.write(bytes);
    if (!written) {
        return written.error();
    }
    if (written.value() != bytes.size()) {
        return libbsa::error{libbsa::error_code::io_error,
                             std::string{description} + " sink accepted a partial chunk"};
    }
    return {};
}

result<void> write_payload_chunks(libbsa::payload_sink& sink, std::span<const std::byte> bytes,
                                  std::size_t chunk_size, std::string_view description) {
    if (chunk_size == 0) {
        return libbsa::error{libbsa::error_code::invalid_argument,
                             std::string{description} + " chunk size must be nonzero"};
    }

    for (std::size_t offset = 0; offset < bytes.size();) {
        const auto count = std::min(chunk_size, bytes.size() - offset);
        auto written = write_payload_exact(sink, bytes.subspan(offset, count), description);
        if (!written) {
            return written.error();
        }
        offset += count;
    }
    return {};
}

result<std::vector<std::byte>> read_payload_bytes_at(std::ifstream& input, std::uint64_t offset,
                                                     std::uint64_t size,
                                                     std::string_view description) {
    auto limits = validate_payload_stream_range(offset, size, description);
    if (!limits) {
        return limits.error();
    }
    auto checked = checked_payload_size(size, description);
    if (!checked) {
        return checked.error();
    }

    auto bytes = make_byte_vector(checked.value(), description);
    if (!bytes) {
        return bytes.error();
    }

    input.clear();
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!input) {
        return libbsa::error{libbsa::error_code::io_error,
                             std::string{"failed to seek to "} + std::string{description}};
    }
    input.read(reinterpret_cast<char*>(bytes.value().data()),
               static_cast<std::streamsize>(bytes.value().size()));
    if (input.bad()) {
        return libbsa::error{libbsa::error_code::io_error,
                             std::string{"failed while reading "} + std::string{description}};
    }
    if (static_cast<std::size_t>(input.gcount()) != bytes.value().size()) {
        return libbsa::error{libbsa::error_code::format_error,
                             std::string{description} + " is truncated"};
    }
    return std::move(bytes).value();
}

result<void> stream_payload_range(std::ifstream& input, std::uint64_t offset, std::uint64_t size,
                                  libbsa::payload_sink& sink, std::size_t chunk_size,
                                  std::string_view description) {
    if (chunk_size == 0) {
        return libbsa::error{libbsa::error_code::invalid_argument,
                             std::string{description} + " chunk size must be nonzero"};
    }

    auto limits = validate_payload_stream_range(offset, size, description);
    if (!limits) {
        return limits.error();
    }

    auto buffer_result = make_byte_vector(chunk_size, std::string{description} + " scratch buffer");
    if (!buffer_result) {
        return buffer_result.error();
    }
    auto buffer = std::move(buffer_result).value();

    input.clear();
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!input) {
        return libbsa::error{libbsa::error_code::io_error,
                             std::string{"failed to seek to "} + std::string{description}};
    }

    std::uint64_t remaining = size;
    while (remaining != 0U) {
        const auto count =
            static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(count));
        if (input.bad()) {
            return libbsa::error{libbsa::error_code::io_error,
                                 std::string{"failed while reading "} + std::string{description}};
        }
        if (static_cast<std::size_t>(input.gcount()) != count) {
            return libbsa::error{libbsa::error_code::format_error,
                                 std::string{description} + " span is outside the archive"};
        }

        auto written = write_payload_exact(sink, std::span<const std::byte>{buffer.data(), count},
                                           description);
        if (!written) {
            return written.error();
        }
        remaining -= count;
    }
    return {};
}

}  // namespace libbsa::detail
