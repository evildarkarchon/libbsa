#include <detail/deflate_codec.hpp>

#include <detail/byte_vector.hpp>

#include <libdeflate.h>

#include <memory>
#include <utility>

namespace libbsa::detail {
namespace {

struct compressor_deleter {
    void operator()(libdeflate_compressor* compressor) const noexcept {
        libdeflate_free_compressor(compressor);
    }
};

struct decompressor_deleter {
    void operator()(libdeflate_decompressor* decompressor) const noexcept {
        libdeflate_free_decompressor(decompressor);
    }
};

using compressor_ptr = std::unique_ptr<libdeflate_compressor, compressor_deleter>;
using decompressor_ptr = std::unique_ptr<libdeflate_decompressor, decompressor_deleter>;

libbsa::error codec_error() {
    return {libbsa::error_code::format_error,
            "raw deflate payload could not be decoded to the expected size"};
}

}  // namespace

result<std::vector<std::byte>> compress_deflate(std::span<const std::byte> input,
                                                int compression_level) {
    compressor_ptr compressor{libdeflate_alloc_compressor(compression_level)};
    if (!compressor) {
        return libbsa::error{libbsa::error_code::io_error, "failed to allocate deflate compressor"};
    }

    const auto bound = libdeflate_deflate_compress_bound(compressor.get(), input.size());
    auto compressed = make_byte_vector(bound, "raw deflate compressed output");
    if (!compressed) {
        return compressed.error();
    }
    const auto actual =
        libdeflate_deflate_compress(compressor.get(), input.data(), input.size(),
                                    compressed.value().data(), compressed.value().size());
    if (actual == 0) {
        return libbsa::error{libbsa::error_code::io_error, "raw deflate compression failed"};
    }
    compressed.value().resize(actual);
    return std::move(compressed).value();
}

result<std::vector<std::byte>> decompress_deflate_exact(std::span<const std::byte> compressed,
                                                        std::size_t expected_size) {
    decompressor_ptr decompressor{libdeflate_alloc_decompressor()};
    if (!decompressor) {
        return libbsa::error{libbsa::error_code::io_error,
                             "failed to allocate deflate decompressor"};
    }

    auto output = make_byte_vector(expected_size, "raw deflate output");
    if (!output) {
        return output.error();
    }
    std::size_t actual_out = 0;
    const auto status =
        libdeflate_deflate_decompress(decompressor.get(), compressed.data(), compressed.size(),
                                      output.value().data(), output.value().size(), &actual_out);
    if (status != LIBDEFLATE_SUCCESS || actual_out != expected_size) {
        return codec_error();
    }
    return std::move(output).value();
}

}  // namespace libbsa::detail
