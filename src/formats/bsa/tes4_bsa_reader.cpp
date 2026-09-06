#include "formats/bsa/tes4_bsa_reader.hpp"

#include <detail/compression_router.hpp>
#include <detail/host_file.hpp>
#include <detail/payload_stream.hpp>

#include <cstddef>
#include <span>

namespace libbsa::formats::bsa {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

detail::host_file_context tes4_extraction_host_context() noexcept {
    return detail::host_file_context{"failed to open archive host path for TES4 BSA extraction",
                                     "failed to inspect archive host path for TES4 BSA extraction",
                                     "failed while reading TES4 archive payload",
                                     "archive host path changed while reading TES4 BSA payload",
                                     "TES4 BSA payload bytes"};
}

std::uint32_t read_u32_le(std::span<const std::byte> bytes) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[0])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[3])) << 24U);
}

detail::compression_method compression_method_for(entry_compression compression) noexcept {
    switch (compression) {
        case entry_compression::none:
            return detail::compression_method::none;
        case entry_compression::deflate:
            return detail::compression_method::zlib;
        case entry_compression::lz4_frame:
            return detail::compression_method::lz4_frame;
        case entry_compression::lz4_block:
            return detail::compression_method::lz4_block;
    }
    return detail::compression_method::none;
}

result<void> extract_file_payload(std::ifstream& input, const entry_metadata& entry,
                                  payload_sink& sink) {
    if (entry.embedded_name_prefix_size > entry.stored_size) {
        return error{error_code::format_error,
                     "TES4 BSA embedded-name prefix exceeds stored payload"};
    }
    const auto payload_offset = entry.payload_offset + entry.embedded_name_prefix_size;
    if (payload_offset < entry.payload_offset) {
        return error{error_code::format_error, "TES4 BSA consumer payload offset overflows"};
    }
    const auto payload_size = entry.stored_size - entry.embedded_name_prefix_size;
    if (entry.compression == entry_compression::none) {
        return detail::stream_payload_range(input, payload_offset, payload_size, sink,
                                            extraction_chunk_size, "TES4 BSA payload");
    }
    if (payload_size < 4U) {
        return error{error_code::format_error,
                     "TES4 BSA compressed payload size prefix is truncated"};
    }

    auto expected_size_bytes = detail::read_payload_bytes_at(
        input, payload_offset, 4U, "TES4 BSA compressed payload size prefix");
    if (!expected_size_bytes) {
        return expected_size_bytes.error();
    }
    const auto expected_size = read_u32_le(std::span<const std::byte>{
        expected_size_bytes.value().data(), expected_size_bytes.value().size()});
    if (expected_size != entry.raw_size) {
        return error{error_code::format_error,
                     "TES4 BSA compressed payload size prefix does not match metadata"};
    }

    const auto compressed_payload_offset = payload_offset + 4U;
    if (compressed_payload_offset < payload_offset) {
        return error{error_code::format_error, "TES4 BSA compressed payload offset overflows"};
    }
    return detail::decompress_payload_exact_to_sink(
        compression_method_for(entry.compression), input, compressed_payload_offset,
        payload_size - 4U, expected_size, sink, extraction_chunk_size,
        "TES4 BSA compressed payload");
}

}  // namespace

result<void> extract_tes4_bsa_payload_from_file(const detail::host_file_path& host_path,
                                                const entry_metadata& entry, payload_sink& sink) {
    auto input = detail::open_host_file(host_path, tes4_extraction_host_context());
    if (!input) {
        return input.error();
    }
    return extract_file_payload(input.value(), entry, sink);
}

}  // namespace libbsa::formats::bsa
