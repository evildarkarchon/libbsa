#include "lz4_frame_codec.hpp"

#include <lz4frame.h>

#include <limits>

namespace libbsa::detail {
namespace {

result<std::vector<std::byte>> lz4_frame_failure(const char* message)
{
    return failure<std::vector<std::byte>>({error_code::decompression_failure, message});
}

} // namespace

result<std::vector<std::byte>> lz4_frame_decompress(std::span<const std::byte> packed, std::uint64_t expected_size)
{
    if (expected_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return lz4_frame_failure("lz4 frame size mismatch");
    }

    LZ4F_decompressionContext_t context = nullptr;
    const auto create_status = LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
    if (LZ4F_isError(create_status)) {
        return lz4_frame_failure("lz4 frame decompression failed");
    }

    std::vector<std::byte> unpacked(static_cast<std::size_t>(expected_size));
    std::size_t source_size = packed.size();
    std::size_t output_size = unpacked.size();
    const auto ret = LZ4F_decompress(context,
                                     unpacked.data(),
                                     &output_size,
                                     packed.data(),
                                     &source_size,
                                     nullptr);
    LZ4F_freeDecompressionContext(context);

    if (LZ4F_isError(ret) || ret != 0 || source_size != packed.size()) {
        return lz4_frame_failure("lz4 frame decompression failed");
    }
    if (output_size != static_cast<std::size_t>(expected_size)) {
        return lz4_frame_failure("lz4 frame size mismatch");
    }

    return success(std::move(unpacked));
}

result<std::vector<std::byte>> lz4_frame_compress(std::span<const std::byte> unpacked)
{
    const auto bound = LZ4F_compressFrameBound(unpacked.size(), nullptr);
    if (LZ4F_isError(bound)) {
        return lz4_frame_failure("lz4 frame compression failed");
    }

    std::vector<std::byte> packed(bound);
    const auto written = LZ4F_compressFrame(packed.data(), packed.size(), unpacked.data(), unpacked.size(), nullptr);
    if (LZ4F_isError(written)) {
        return lz4_frame_failure("lz4 frame compression failed");
    }

    packed.resize(written);
    return success(std::move(packed));
}

} // namespace libbsa::detail
