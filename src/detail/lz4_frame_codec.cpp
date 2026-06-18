#include <detail/lz4_frame_codec.hpp>

#include <detail/byte_vector.hpp>
#include <detail/payload_stream.hpp>

#include <lz4frame.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

namespace libbsa::detail {
namespace {

struct frame_context_deleter {
    void operator()(LZ4F_dctx* context) const noexcept {
        (void)LZ4F_freeDecompressionContext(context);
    }
};

using frame_context_ptr = std::unique_ptr<LZ4F_dctx, frame_context_deleter>;

libbsa::error frame_error() {
    return {libbsa::error_code::format_error,
            "LZ4 frame payload could not be decoded to the expected size"};
}

}  // namespace

result<std::vector<std::byte>> compress_lz4_frame(std::span<const std::byte> input) {
    const auto bound = LZ4F_compressFrameBound(input.size(), nullptr);
    if (LZ4F_isError(bound)) {
        return libbsa::error{libbsa::error_code::format_error,
                             "LZ4 frame compression bound failed"};
    }
    auto compressed = make_byte_vector(bound, "LZ4 frame compressed output");
    if (!compressed) {
        return compressed.error();
    }
    const auto actual = LZ4F_compressFrame(compressed.value().data(), compressed.value().size(),
                                           input.data(), input.size(), nullptr);
    if (LZ4F_isError(actual)) {
        return libbsa::error{libbsa::error_code::io_error, "LZ4 frame compression failed"};
    }
    compressed.value().resize(actual);
    return std::move(compressed).value();
}

result<std::vector<std::byte>> decompress_lz4_frame_exact(std::span<const std::byte> compressed,
                                                          std::size_t expected_size) {
    LZ4F_dctx* raw_context = nullptr;
    const auto create_status = LZ4F_createDecompressionContext(&raw_context, LZ4F_VERSION);
    if (LZ4F_isError(create_status)) {
        return libbsa::error{libbsa::error_code::io_error,
                             "failed to create LZ4 frame decompressor"};
    }
    frame_context_ptr context{raw_context};

    auto output = make_byte_vector(expected_size, "LZ4 frame output");
    if (!output) {
        return output.error();
    }
    std::size_t source_offset = 0;
    std::size_t output_offset = 0;
    std::size_t status = 0;
    do {
        std::size_t source_size = compressed.size() - source_offset;
        std::size_t output_size = output.value().size() - output_offset;
        status = LZ4F_decompress(context.get(), output.value().data() + output_offset, &output_size,
                                 compressed.data() + source_offset, &source_size, nullptr);
        if (LZ4F_isError(status)) {
            return frame_error();
        }
        source_offset += source_size;
        output_offset += output_size;
        if (source_size == 0 && status != 0) {
            return frame_error();
        }
    } while (status != 0 && source_offset < compressed.size());

    if (status != 0 || output_offset != expected_size || source_offset != compressed.size()) {
        return frame_error();
    }
    return std::move(output).value();
}

result<void> decompress_lz4_frame_exact_to_sink(std::ifstream& input,
                                                std::uint64_t compressed_offset,
                                                std::uint64_t compressed_size,
                                                std::uint64_t expected_size,
                                                libbsa::payload_sink& sink, std::size_t chunk_size,
                                                std::string_view description) {
    if (chunk_size == 0U) {
        return libbsa::error{libbsa::error_code::invalid_argument,
                             std::string{description} + " chunk size must be nonzero"};
    }

    auto limits = validate_payload_stream_range(compressed_offset, compressed_size, description);
    if (!limits) {
        return limits.error();
    }

    LZ4F_dctx* raw_context = nullptr;
    const auto create_status = LZ4F_createDecompressionContext(&raw_context, LZ4F_VERSION);
    if (LZ4F_isError(create_status)) {
        return libbsa::error{libbsa::error_code::io_error,
                             "failed to create LZ4 frame decompressor"};
    }
    frame_context_ptr context{raw_context};

    auto input_buffer_result =
        make_byte_vector(chunk_size, std::string{description} + " compressed scratch buffer");
    if (!input_buffer_result) {
        return input_buffer_result.error();
    }
    auto output_buffer_result =
        make_byte_vector(chunk_size, std::string{description} + " decoded scratch buffer");
    if (!output_buffer_result) {
        return output_buffer_result.error();
    }
    auto input_buffer = std::move(input_buffer_result).value();
    auto output_buffer = std::move(output_buffer_result).value();

    input.clear();
    input.seekg(static_cast<std::streamoff>(compressed_offset), std::ios::beg);
    if (!input) {
        return libbsa::error{libbsa::error_code::io_error,
                             std::string{"failed to seek to "} + std::string{description}};
    }

    std::uint64_t decoded_total = 0;
    std::uint64_t compressed_remaining = compressed_size;
    std::size_t status = 1U;
    while (compressed_remaining != 0U) {
        const auto read_size = static_cast<std::size_t>(
            std::min<std::uint64_t>(compressed_remaining, input_buffer.size()));
        input.read(reinterpret_cast<char*>(input_buffer.data()),
                   static_cast<std::streamsize>(read_size));
        if (input.bad()) {
            return libbsa::error{libbsa::error_code::io_error,
                                 std::string{"failed while reading "} + std::string{description}};
        }
        if (static_cast<std::size_t>(input.gcount()) != read_size) {
            return libbsa::error{libbsa::error_code::format_error,
                                 std::string{description} + " span is outside the archive"};
        }
        compressed_remaining -= read_size;

        std::size_t source_offset = 0;
        while (source_offset < read_size) {
            const auto remaining_output = expected_size - decoded_total;
            const auto output_capacity = static_cast<std::size_t>(
                std::min<std::uint64_t>(remaining_output, output_buffer.size()));
            std::size_t source_size = read_size - source_offset;
            std::size_t output_size = output_capacity;
            status = LZ4F_decompress(context.get(), output_buffer.data(), &output_size,
                                     input_buffer.data() + source_offset, &source_size, nullptr);
            if (LZ4F_isError(status)) {
                return frame_error();
            }
            if (source_size == 0U && output_size == 0U && status != 0U) {
                return frame_error();
            }
            if (output_size > output_capacity || output_size > expected_size - decoded_total) {
                return frame_error();
            }

            source_offset += source_size;
            if (output_size != 0U) {
                auto written = write_payload_exact(
                    sink, std::span<const std::byte>{output_buffer.data(), output_size},
                    description);
                if (!written) {
                    return written.error();
                }
                decoded_total += output_size;
            }

            if (status == 0U) {
                if (decoded_total != expected_size || source_offset != read_size ||
                    compressed_remaining != 0U) {
                    return frame_error();
                }
                return {};
            }
        }
    }

    return frame_error();
}

}  // namespace libbsa::detail
