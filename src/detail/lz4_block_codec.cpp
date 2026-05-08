#include <detail/lz4_block_codec.hpp>

#include <lz4.h>

#include <limits>
#include <string>

namespace libbsa::detail {
namespace {

libbsa::error block_error() {
  return {libbsa::error_code::format_error, "raw LZ4 block could not be decoded to the expected size"};
}

result<int> checked_int_size(std::size_t size, const char* label) {
  if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    return libbsa::error{libbsa::error_code::invalid_argument, std::string{label} + " exceeds LZ4 one-shot size limit"};
  }
  return static_cast<int>(size);
}

} // namespace

result<std::vector<std::byte>> compress_lz4_block(std::span<const std::byte> input) {
  auto source_size = checked_int_size(input.size(), "input");
  if (!source_size) {
    return source_size.error();
  }
  const auto bound = LZ4_compressBound(source_size.value());
  std::vector<std::byte> compressed(static_cast<std::size_t>(bound));
  const auto actual = LZ4_compress_default(reinterpret_cast<const char*>(input.data()), reinterpret_cast<char*>(compressed.data()),
                                          source_size.value(), bound);
  if (actual <= 0) {
    return libbsa::error{libbsa::error_code::io_error, "raw LZ4 block compression failed"};
  }
  compressed.resize(static_cast<std::size_t>(actual));
  return compressed;
}

result<std::vector<std::byte>> decompress_lz4_block_exact(std::span<const std::byte> compressed, std::size_t expected_size) {
  auto compressed_size = checked_int_size(compressed.size(), "compressed input");
  if (!compressed_size) {
    return compressed_size.error();
  }
  auto output_size = checked_int_size(expected_size, "expected output");
  if (!output_size) {
    return output_size.error();
  }
  std::vector<std::byte> output(expected_size);
  const auto actual = LZ4_decompress_safe(reinterpret_cast<const char*>(compressed.data()), reinterpret_cast<char*>(output.data()),
                                         compressed_size.value(), output_size.value());
  if (actual < 0 || static_cast<std::size_t>(actual) != expected_size) {
    return block_error();
  }
  return output;
}

} // namespace libbsa::detail
