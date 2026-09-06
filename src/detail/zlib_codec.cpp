#include <detail/zlib_codec.hpp>

#include <detail/byte_vector.hpp>

#include <libdeflate.h>

#include <cstdint>
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

/// Size of the RFC1950 header (CMF + FLG) that precedes the DEFLATE stream.
constexpr std::size_t zlib_header_size = 2U;

/// Size of the RFC1950 Adler-32 trailer that follows the DEFLATE stream.
constexpr std::size_t zlib_trailer_size = 4U;

libbsa::error codec_error() {
    return {libbsa::error_code::format_error,
            "zlib payload could not be decoded to the expected size"};
}

/// Validates the two RFC1950 header bytes the way a zlib decoder would.
///
/// Checked before the missing-trailer fallback so that a bare RFC1951 stream
/// cannot be smuggled through the tolerance path: the fallback decodes from
/// `compressed[2]`, which for raw deflate input would silently skip two bytes of
/// real payload.
bool has_valid_zlib_header(std::span<const std::byte> compressed) noexcept {
    if (compressed.size() < zlib_header_size) {
        return false;
    }
    const auto cmf = std::to_integer<std::uint32_t>(compressed[0]);
    const auto flg = std::to_integer<std::uint32_t>(compressed[1]);
    if ((cmf & 0x0FU) != 8U) {  // CM must be DEFLATE.
        return false;
    }
    if ((cmf >> 4U) > 7U) {  // CINFO must not exceed the 32 KiB window.
        return false;
    }
    if ((flg & 0x20U) != 0U) {  // FDICT is never used by Bethesda archives.
        return false;
    }
    return (((cmf << 8U) | flg) % 31U) == 0U;  // FCHECK.
}

/// Second-chance decode for the vanilla `Fallout - Misc.bsa` entry whose
/// Adler-32 trailer is missing.
///
/// Only accepts input whose DEFLATE stream is complete, yields exactly
/// `expected_size` bytes, and is followed by fewer than four bytes. A full
/// trailer that is present means the strict decode failed on verification, not
/// on truncation, and that is corruption the reference re-raises rather than
/// swallows.
result<std::vector<std::byte>> decompress_without_adler32_trailer(
    libdeflate_decompressor* decompressor, std::span<const std::byte> compressed,
    std::size_t expected_size) {
    if (!has_valid_zlib_header(compressed)) {
        return codec_error();
    }

    auto output = make_byte_vector(expected_size, "zlib output");
    if (!output) {
        return output.error();
    }

    const auto body = compressed.subspan(zlib_header_size);
    std::size_t actual_in = 0;
    std::size_t actual_out = 0;
    const auto status = libdeflate_deflate_decompress_ex(
        decompressor, body.data(), body.size(), output.value().data(), output.value().size(),
        &actual_in, &actual_out);
    if (status != LIBDEFLATE_SUCCESS || actual_out != expected_size) {
        return codec_error();
    }
    if (body.size() - actual_in >= zlib_trailer_size) {
        return codec_error();
    }
    return std::move(output).value();
}

}  // namespace

result<std::vector<std::byte>> compress_zlib(std::span<const std::byte> input,
                                             int compression_level) {
    compressor_ptr compressor{libdeflate_alloc_compressor(compression_level)};
    if (!compressor) {
        return libbsa::error{libbsa::error_code::io_error, "failed to allocate zlib compressor"};
    }

    const auto bound = libdeflate_zlib_compress_bound(compressor.get(), input.size());
    auto compressed = make_byte_vector(bound, "zlib compressed output");
    if (!compressed) {
        return compressed.error();
    }
    const auto actual =
        libdeflate_zlib_compress(compressor.get(), input.data(), input.size(),
                                 compressed.value().data(), compressed.value().size());
    if (actual == 0) {
        return libbsa::error{libbsa::error_code::io_error, "zlib compression failed"};
    }
    compressed.value().resize(actual);
    return std::move(compressed).value();
}

result<std::vector<std::byte>> decompress_zlib_exact(std::span<const std::byte> compressed,
                                                     std::size_t expected_size) {
    // Vanilla archives store zero-length files as compressed entries with a
    // zero-length payload, which carries no zlib framing to decode.
    if (expected_size == 0U && compressed.empty()) {
        return std::vector<std::byte>{};
    }

    decompressor_ptr decompressor{libdeflate_alloc_decompressor()};
    if (!decompressor) {
        return libbsa::error{libbsa::error_code::io_error, "failed to allocate zlib decompressor"};
    }

    auto output = make_byte_vector(expected_size, "zlib output");
    if (!output) {
        return output.error();
    }
    std::size_t actual_out = 0;
    const auto status =
        libdeflate_zlib_decompress(decompressor.get(), compressed.data(), compressed.size(),
                                   output.value().data(), output.value().size(), &actual_out);
    if (status == LIBDEFLATE_SUCCESS && actual_out == expected_size) {
        return std::move(output).value();
    }
    if (status == LIBDEFLATE_SUCCESS) {
        // The stream decoded cleanly but disagrees with the archive metadata.
        // That is a size contract failure, not the truncated-trailer shape.
        return codec_error();
    }
    return decompress_without_adler32_trailer(decompressor.get(), compressed, expected_size);
}

}  // namespace libbsa::detail
