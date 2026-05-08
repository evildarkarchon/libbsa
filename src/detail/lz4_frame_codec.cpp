#include <detail/lz4_frame_codec.hpp>

#include <lz4frame.h>

#include <memory>

namespace libbsa::detail {
namespace {

struct frame_context_deleter {
  void operator()(LZ4F_dctx* context) const noexcept { (void)LZ4F_freeDecompressionContext(context); }
};

using frame_context_ptr = std::unique_ptr<LZ4F_dctx, frame_context_deleter>;

libbsa::error frame_error() {
  return {libbsa::error_code::format_error, "LZ4 frame payload could not be decoded to the expected size"};
}

} // namespace

result<std::vector<std::byte>> compress_lz4_frame(std::span<const std::byte> input) {
  const auto bound = LZ4F_compressFrameBound(input.size(), nullptr);
  std::vector<std::byte> compressed(bound);
  const auto actual = LZ4F_compressFrame(compressed.data(), compressed.size(), input.data(), input.size(), nullptr);
  if (LZ4F_isError(actual)) {
    return libbsa::error{libbsa::error_code::io_error, "LZ4 frame compression failed"};
  }
  compressed.resize(actual);
  return compressed;
}

result<std::vector<std::byte>> decompress_lz4_frame_exact(std::span<const std::byte> compressed, std::size_t expected_size) {
  LZ4F_dctx* raw_context = nullptr;
  const auto create_status = LZ4F_createDecompressionContext(&raw_context, LZ4F_VERSION);
  if (LZ4F_isError(create_status)) {
    return libbsa::error{libbsa::error_code::io_error, "failed to create LZ4 frame decompressor"};
  }
  frame_context_ptr context{raw_context};

  std::vector<std::byte> output(expected_size);
  std::size_t source_offset = 0;
  std::size_t output_offset = 0;
  std::size_t status = 0;
  do {
    std::size_t source_size = compressed.size() - source_offset;
    std::size_t output_size = output.size() - output_offset;
    status = LZ4F_decompress(context.get(), output.data() + output_offset, &output_size, compressed.data() + source_offset,
                             &source_size, nullptr);
    if (LZ4F_isError(status)) {
      return frame_error();
    }
    source_offset += source_size;
    output_offset += output_size;
    if (source_size == 0 && status != 0) {
      return frame_error();
    }
  } while (status != 0 && source_offset < compressed.size());

  if (status != 0 || output_offset != expected_size || source_offset != compressed.size()) {
    return frame_error();
  }
  return output;
}

} // namespace libbsa::detail
