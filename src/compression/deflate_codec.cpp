#include "deflate_codec.hpp"

#include <libdeflate.h>

#include <limits>

namespace libbsa::detail {
namespace {

result<std::vector<std::byte>> deflate_failure(const char* message)
{
    return failure<std::vector<std::byte>>({error_code::decompression_failure, message});
}

} // namespace

result<std::vector<std::byte>> deflate_decompress(std::span<const std::byte> packed, std::uint64_t expected_size)
{
    if (expected_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return deflate_failure("deflate size mismatch");
    }

    std::vector<std::byte> unpacked(static_cast<std::size_t>(expected_size));
    auto* decompressor = libdeflate_alloc_decompressor();
    if (decompressor == nullptr) {
        return deflate_failure("deflate decompression failed");
    }

    std::size_t actual_out = 0;
    const auto status = libdeflate_deflate_decompress(decompressor,
                                                      packed.data(),
                                                      packed.size(),
                                                      unpacked.data(),
                                                      unpacked.size(),
                                                      &actual_out);
    libdeflate_free_decompressor(decompressor);

    if (status == LIBDEFLATE_SUCCESS && actual_out != static_cast<std::size_t>(expected_size)) {
        return deflate_failure("deflate size mismatch");
    }
    if (status != LIBDEFLATE_SUCCESS) {
        return deflate_failure("deflate decompression failed");
    }

    return success(std::move(unpacked));
}

result<std::vector<std::byte>> deflate_compress(std::span<const std::byte> unpacked)
{
    auto* compressor = libdeflate_alloc_compressor(6);
    if (compressor == nullptr) {
        return deflate_failure("deflate compression failed");
    }

    std::vector<std::byte> packed(libdeflate_deflate_compress_bound(compressor, unpacked.size()));
    const auto written = libdeflate_deflate_compress(compressor,
                                                     unpacked.data(),
                                                     unpacked.size(),
                                                     packed.data(),
                                                     packed.size());
    libdeflate_free_compressor(compressor);

    if (written == 0) {
        return deflate_failure("deflate compression failed");
    }

    packed.resize(written);
    return success(std::move(packed));
}

} // namespace libbsa::detail
