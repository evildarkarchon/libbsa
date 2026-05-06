#include <libbsa/compression.hpp>

#include "compression/deflate_codec.hpp"
#include "compression/lz4_block_codec.hpp"
#include "compression/lz4_frame_codec.hpp"

#include <limits>

namespace libbsa {
namespace {

result<compression_algorithm> unsupported_route()
{
    return failure<compression_algorithm>({error_code::unsupported_format, "unsupported compression route"});
}

result<compression_state> unsupported_policy()
{
    return failure<compression_state>({error_code::unsupported_format, "unsupported compression route"});
}

result<std::vector<std::byte>> unsupported_payload_codec()
{
    return failure<std::vector<std::byte>>({error_code::unsupported_format, "unsupported compression route"});
}

bool is_fo4_ba2(archive_format format)
{
    return format == archive_format::fo4_ba2_gnrl || format == archive_format::fo4_ba2_dds;
}

bool is_starfield_ba2(archive_format format)
{
    return format == archive_format::starfield_ba2_gnrl || format == archive_format::starfield_ba2_dds;
}

bool supports_deflate(archive_format format)
{
    return format == archive_format::tes4_bsa || format == archive_format::fo3_bsa || is_fo4_ba2(format) ||
           is_starfield_ba2(format);
}

} // namespace

result<compression_algorithm> resolve_payload_codec(const payload_codec_request& request)
{
    switch (request.entry_state) {
    case compression_state::none:
    case compression_state::raw:
        return success(compression_algorithm::none);
    case compression_state::deflate:
        if (supports_deflate(request.format)) {
            return success(compression_algorithm::deflate);
        }
        return unsupported_route();
    case compression_state::lz4_frame:
        if (request.format == archive_format::sse_bsa) {
            return success(compression_algorithm::lz4_frame);
        }
        return unsupported_route();
    case compression_state::lz4_block:
        if (is_starfield_ba2(request.format) && request.compression_method == 3) {
            return success(compression_algorithm::lz4_block);
        }
        return unsupported_route();
    case compression_state::archive_default:
        if (request.format == archive_format::sse_bsa) {
            return success(compression_algorithm::lz4_frame);
        }
        if (is_starfield_ba2(request.format) && request.compression_method == 3) {
            return success(compression_algorithm::lz4_block);
        }
        if (supports_deflate(request.format)) {
            return success(compression_algorithm::deflate);
        }
        return unsupported_route();
    case compression_state::unknown:
        return unsupported_route();
    }

    return unsupported_route();
}

result<compression_state> resolve_write_compression(archive_format format,
                                                    compression_policy policy,
                                                    bool archive_default_compressed)
{
    switch (policy) {
    case compression_policy::force_raw:
        return success(compression_state::raw);
    case compression_policy::archive_default:
        if (!archive_default_compressed) {
            return success(compression_state::raw);
        }
        [[fallthrough]];
    case compression_policy::force_compressed:
        if (format == archive_format::sse_bsa) {
            return success(compression_state::lz4_frame);
        }
        if (supports_deflate(format)) {
            return success(compression_state::deflate);
        }
        return unsupported_policy();
    }

    return unsupported_policy();
}

result<std::vector<std::byte>> decompress_payload(compression_algorithm algorithm,
                                                  std::span<const std::byte> packed,
                                                  std::uint64_t expected_size)
{
    switch (algorithm) {
    case compression_algorithm::none:
        if (expected_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
            packed.size() != static_cast<std::size_t>(expected_size)) {
            return failure<std::vector<std::byte>>({error_code::decompression_failure, "uncompressed size mismatch"});
        }
        return success(std::vector<std::byte>{packed.begin(), packed.end()});
    case compression_algorithm::deflate:
        return detail::deflate_decompress(packed, expected_size);
    case compression_algorithm::lz4_frame:
        return detail::lz4_frame_decompress(packed, expected_size);
    case compression_algorithm::lz4_block:
        return detail::lz4_block_decompress(packed, expected_size);
    }

    return unsupported_payload_codec();
}

result<std::vector<std::byte>> compress_payload(compression_algorithm algorithm, std::span<const std::byte> unpacked)
{
    switch (algorithm) {
    case compression_algorithm::none:
        return success(std::vector<std::byte>{unpacked.begin(), unpacked.end()});
    case compression_algorithm::deflate:
        return detail::deflate_compress(unpacked);
    case compression_algorithm::lz4_frame:
        return detail::lz4_frame_compress(unpacked);
    case compression_algorithm::lz4_block:
        return detail::lz4_block_compress(unpacked);
    }

    return unsupported_payload_codec();
}

} // namespace libbsa
