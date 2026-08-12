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
///
/// `zlib` is RFC1950-framed DEFLATE, which is what Bethesda archives actually
/// store. There is deliberately no bare RFC1951 route: the reference
/// implementation only ever selects `ctZlib`, `ctLZ4Frame`, or `ctLZ4Block`
/// (`wbBSArchive.pas:32`, `1790-1813`), so a raw-deflate route would have no
/// caller and would only invite the confusion tracked in issue #42.
enum class compression_method { none, zlib, lz4_frame, lz4_block };

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
/// LZ4 frame payloads use bounded streaming input and output. Zlib and raw
/// LZ4 block payloads remain explicit whole-buffer fallbacks after validating
/// that the decoded output size is representable by one byte vector.
result<void> decompress_payload_exact_to_sink(compression_method method, std::ifstream& input,
                                              std::uint64_t compressed_offset,
                                              std::uint64_t compressed_size,
                                              std::uint64_t expected_size,
                                              libbsa::payload_sink& sink, std::size_t chunk_size,
                                              std::string_view description);

}  // namespace libbsa::detail
