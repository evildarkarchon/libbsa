#include <detail/lz4_block_codec.hpp>

#include <detail/byte_vector.hpp>

#include <lz4.h>

#include <limits>
#include <string>
#include <utility>

namespace libbsa::detail {
namespace {

libbsa::error block_error() {
    return {libbsa::error_code::format_error,
            "raw LZ4 block could not be decoded to the expected size"};
}

result<int> checked_int_size(std::size_t size, const char* label,
                             libbsa::error_code code = libbsa::error_code::invalid_argument) {
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return libbsa::error{code, std::string{label} + " exceeds LZ4 one-shot size limit"};
    }
    return static_cast<int>(size);
}

}  // namespace

result<std::vector<std::byte>> compress_lz4_block(std::span<const std::byte> input) {
    auto source_size = checked_int_size(input.size(), "input");
    if (!source_size) {
        return source_size.error();
    }
    const auto bound = LZ4_compressBound(source_size.value());
    auto compressed =
        make_byte_vector(static_cast<std::size_t>(bound), "raw LZ4 block compressed output");
    if (!compressed) {
        return compressed.error();
    }
    const auto actual = LZ4_compress_default(reinterpret_cast<const char*>(input.data()),
                                             reinterpret_cast<char*>(compressed.value().data()),
                                             source_size.value(), bound);
    if (actual <= 0) {
        return libbsa::error{libbsa::error_code::io_error, "raw LZ4 block compression failed"};
    }
    compressed.value().resize(static_cast<std::size_t>(actual));
    return std::move(compressed).value();
}

result<std::vector<std::byte>> decompress_lz4_block_exact(std::span<const std::byte> compressed,
                                                          std::size_t expected_size) {
    auto compressed_size = checked_int_size(compressed.size(), "compressed input");
    if (!compressed_size) {
        return compressed_size.error();
    }
    auto output_size =
        checked_int_size(expected_size, "expected output", libbsa::error_code::format_error);
    if (!output_size) {
        return output_size.error();
    }
    auto output = make_byte_vector(expected_size, "raw LZ4 block output");
    if (!output) {
        return output.error();
    }
    const auto actual = LZ4_decompress_safe(reinterpret_cast<const char*>(compressed.data()),
                                            reinterpret_cast<char*>(output.value().data()),
                                            compressed_size.value(), output_size.value());
    if (actual < 0 || static_cast<std::size_t>(actual) != expected_size) {
        return block_error();
    }
    return std::move(output).value();
}

}  // namespace libbsa::detail
