#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa {

class payload_sink;

}  // namespace libbsa

namespace libbsa::detail {

/// Explicit compression method selected from archive metadata by format
/// parsers.
enum class compression_method { none, deflate, lz4_frame, lz4_block };

/// Compresses `input` using the explicitly selected compression method.
result<std::vector<std::byte>> compress_payload(compression_method method,
                                                std::span<const std::byte> input);

/// Decompresses `input` with the selected method and exact expected size.
result<std::vector<std::byte>> decompress_payload_exact(compression_method method,
                                                        std::span<const std::byte> input,
                                                        std::size_t expected_size);

/// Decompresses an archive byte range with the selected method and writes
/// exactly `expected_size` bytes to `sink`.
///
/// LZ4 frame payloads use bounded streaming input and output. Deflate and raw
/// LZ4 block payloads remain explicit whole-buffer fallbacks after validating
/// that the decoded output size is representable by one byte vector.
result<void> decompress_payload_exact_to_sink(compression_method method, std::ifstream& input,
                                              std::uint64_t compressed_offset,
                                              std::uint64_t compressed_size,
                                              std::uint64_t expected_size,
                                              libbsa::payload_sink& sink, std::size_t chunk_size,
                                              std::string_view description);

}  // namespace libbsa::detail
