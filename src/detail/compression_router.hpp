#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace libbsa::detail {

/// Explicit compression method selected from archive metadata by format parsers.
enum class compression_method { none, deflate, lz4_frame, lz4_block };

/// Compresses `input` using the explicitly selected compression method.
result<std::vector<std::byte>> compress_payload(compression_method method, std::span<const std::byte> input);

/// Decompresses `input` with the selected method and exact expected size.
result<std::vector<std::byte>> decompress_payload_exact(compression_method method, std::span<const std::byte> input,
                                                        std::size_t expected_size);

} // namespace libbsa::detail
