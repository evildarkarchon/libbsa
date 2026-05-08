#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace libbsa::detail {

/// Compresses bytes as an LZ4 frame payload for Skyrim SE/AE-style archives.
result<std::vector<std::byte>> compress_lz4_frame(std::span<const std::byte> input);

/// Decompresses an LZ4 frame and requires exactly `expected_size` output bytes.
result<std::vector<std::byte>> decompress_lz4_frame_exact(std::span<const std::byte> compressed, std::size_t expected_size);

} // namespace libbsa::detail
