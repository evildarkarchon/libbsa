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

} // namespace libbsa

namespace libbsa::detail {

/// Compresses bytes as an LZ4 frame payload for Skyrim SE/AE-style archives.
result<std::vector<std::byte>> compress_lz4_frame(std::span<const std::byte> input);

/// Decompresses an LZ4 frame and requires exactly `expected_size` output bytes.
result<std::vector<std::byte>> decompress_lz4_frame_exact(std::span<const std::byte> compressed, std::size_t expected_size);

/// Decompresses an LZ4 frame archive range and writes exactly `expected_size` bytes to `sink`.
///
/// Input and output scratch buffers are bounded by `chunk_size`. Malformed frame
/// data or exact-size mismatches return `format_error`; a caller sink may have
/// received partial decoded output before a later frame error is detected.
result<void> decompress_lz4_frame_exact_to_sink(std::ifstream& input,
                                                std::uint64_t compressed_offset,
                                                std::uint64_t compressed_size,
                                                std::uint64_t expected_size,
                                                libbsa::payload_sink& sink,
                                                std::size_t chunk_size,
                                                std::string_view description);

} // namespace libbsa::detail
