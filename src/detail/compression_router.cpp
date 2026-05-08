#include <detail/compression_router.hpp>

#include <detail/deflate_codec.hpp>
#include <detail/lz4_block_codec.hpp>
#include <detail/lz4_frame_codec.hpp>

namespace libbsa::detail {
namespace {

std::vector<std::byte> copy_bytes(std::span<const std::byte> input) { return {input.begin(), input.end()}; }

libbsa::error unsupported_method_error() {
  return {libbsa::error_code::invalid_argument, "unsupported compression method"};
}

} // namespace

result<std::vector<std::byte>> compress_payload(compression_method method, std::span<const std::byte> input) {
  switch (method) {
  case compression_method::none:
    return copy_bytes(input);
  case compression_method::deflate:
    return compress_deflate(input);
  case compression_method::lz4_frame:
    return compress_lz4_frame(input);
  case compression_method::lz4_block:
    return compress_lz4_block(input);
  }
  return unsupported_method_error();
}

result<std::vector<std::byte>> decompress_payload_exact(compression_method method, std::span<const std::byte> input,
                                                        std::size_t expected_size) {
  switch (method) {
  case compression_method::none:
    if (input.size() != expected_size) {
      return libbsa::error{libbsa::error_code::format_error, "uncompressed payload size did not match metadata"};
    }
    return copy_bytes(input);
  case compression_method::deflate:
    return decompress_deflate_exact(input, expected_size);
  case compression_method::lz4_frame:
    return decompress_lz4_frame_exact(input, expected_size);
  case compression_method::lz4_block:
    return decompress_lz4_block_exact(input, expected_size);
  }
  return unsupported_method_error();
}

} // namespace libbsa::detail
