#include <detail/compression_router.hpp>

#include <detail/byte_vector.hpp>
#include <detail/deflate_codec.hpp>
#include <detail/lz4_block_codec.hpp>
#include <detail/lz4_frame_codec.hpp>
#include <detail/payload_stream.hpp>

#include <algorithm>
#include <fstream>
#include <string>

namespace libbsa::detail
{
  namespace
  {

    result<std::vector<std::byte>> copy_bytes(std::span<const std::byte> input)
    {
      auto output = make_byte_vector(input.size(), "compression router uncompressed payload");
      if (!output)
      {
        return output.error();
      }
      std::copy(input.begin(), input.end(), output.value().begin());
      return std::move(output).value();
    }

    libbsa::error unsupported_method_error()
    {
      return {libbsa::error_code::invalid_argument, "unsupported compression method"};
    }

  } // namespace

  result<std::vector<std::byte>> compress_payload(compression_method method, std::span<const std::byte> input)
  {
    switch (method)
    {
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
                                                          std::size_t expected_size)
  {
    switch (method)
    {
    case compression_method::none:
      if (input.size() != expected_size)
      {
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

  result<void> decompress_payload_exact_to_sink(compression_method method,
                                                std::ifstream &input,
                                                std::uint64_t compressed_offset,
                                                std::uint64_t compressed_size,
                                                std::uint64_t expected_size,
                                                libbsa::payload_sink &sink,
                                                std::size_t chunk_size,
                                                std::string_view description)
  {
    if (chunk_size == 0U)
    {
      return libbsa::error{libbsa::error_code::invalid_argument, std::string{description} + " chunk size must be nonzero"};
    }

    switch (method)
    {
    case compression_method::none:
      if (compressed_size != expected_size)
      {
        return libbsa::error{libbsa::error_code::format_error, "uncompressed payload size did not match metadata"};
      }
      return stream_payload_range(input, compressed_offset, compressed_size, sink, chunk_size, description);
    case compression_method::lz4_frame:
      return decompress_lz4_frame_exact_to_sink(input, compressed_offset, compressed_size, expected_size, sink, chunk_size,
                                                description);
    case compression_method::deflate:
    case compression_method::lz4_block:
    {
      auto checked_size = checked_materialized_payload_size(expected_size, std::string{description} + " decoded payload");
      if (!checked_size)
      {
        return checked_size.error();
      }
      auto stored = read_payload_bytes_at(input, compressed_offset, compressed_size, description);
      if (!stored)
      {
        return stored.error();
      }
      auto decoded = decompress_payload_exact(method, stored.value(), checked_size.value());
      if (!decoded)
      {
        return decoded.error();
      }
      return write_payload_chunks(sink, decoded.value(), chunk_size, description);
    }
    }
    return unsupported_method_error();
  }

} // namespace libbsa::detail
