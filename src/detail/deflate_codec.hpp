#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace libbsa::detail {

/// Compresses a raw DEFLATE payload through the private libdeflate adapter.
///
/// The returned bytes are raw deflate data, not zlib or gzip wrapped streams.
result<std::vector<std::byte>> compress_deflate(std::span<const std::byte> input, int compression_level = 6);

/// Decompresses a raw DEFLATE payload and requires exactly `expected_size` bytes.
///
/// Malformed data or mismatched output size returns `format_error`, matching the
/// archive metadata validation later format parsers need.
result<std::vector<std::byte>> decompress_deflate_exact(std::span<const std::byte> compressed, std::size_t expected_size);

} // namespace libbsa::detail
