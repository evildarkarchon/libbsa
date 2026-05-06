#include "lz4_block_codec.hpp"

#include <lz4.h>

#include <limits>

namespace libbsa::detail {
namespace {

result<std::vector<std::byte>> lz4_block_failure(const char* message)
{
    return failure<std::vector<std::byte>>({error_code::decompression_failure, message});
}

bool fits_lz4_int(std::size_t value)
{
    return value <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

} // namespace

result<std::vector<std::byte>> lz4_block_decompress(std::span<const std::byte> packed, std::uint64_t expected_size)
{
    if (expected_size > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) || !fits_lz4_int(packed.size())) {
        return lz4_block_failure("lz4 block size mismatch");
    }

    std::vector<std::byte> unpacked(static_cast<std::size_t>(expected_size));
    const auto actual = LZ4_decompress_safe(reinterpret_cast<const char*>(packed.data()),
                                            reinterpret_cast<char*>(unpacked.data()),
                                            static_cast<int>(packed.size()),
                                            static_cast<int>(unpacked.size()));
    if (actual < 0) {
        return lz4_block_failure("lz4 block decompression failed");
    }
    if (actual != static_cast<int>(expected_size)) {
        return lz4_block_failure("lz4 block size mismatch");
    }

    return success(std::move(unpacked));
}

result<std::vector<std::byte>> lz4_block_compress(std::span<const std::byte> unpacked)
{
    if (!fits_lz4_int(unpacked.size())) {
        return lz4_block_failure("lz4 block compression failed");
    }

    std::vector<std::byte> packed(static_cast<std::size_t>(LZ4_compressBound(static_cast<int>(unpacked.size()))));
    const auto written = LZ4_compress_default(reinterpret_cast<const char*>(unpacked.data()),
                                              reinterpret_cast<char*>(packed.data()),
                                              static_cast<int>(unpacked.size()),
                                              static_cast<int>(packed.size()));
    if (written <= 0) {
        return lz4_block_failure("lz4 block compression failed");
    }

    packed.resize(static_cast<std::size_t>(written));
    return success(std::move(packed));
}

} // namespace libbsa::detail
