#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace libbsa::detail
{

    /// Compresses bytes as a raw LZ4 block for Starfield BA2 v3-style chunks.
    result<std::vector<std::byte>> compress_lz4_block(std::span<const std::byte> input);

    /// Decompresses a raw LZ4 block and requires exactly `expected_size` output bytes.
    result<std::vector<std::byte>> decompress_lz4_block_exact(std::span<const std::byte> compressed, std::size_t expected_size);

} // namespace libbsa::detail
