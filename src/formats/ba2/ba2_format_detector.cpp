#include "formats/ba2/ba2_format_detector.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/binary_io.hpp>

#include <utility>

namespace libbsa::formats::ba2 {
namespace {

result<detected_ba2_format> detected_header(archive_variant variant,
                                            std::uint32_t version,
                                            entry_compression default_compression,
                                            ba2_archive_metadata ba2,
                                            bool is_gnrl,
                                            bool is_dx10,
                                            std::uint32_t file_count) {
  return detected_ba2_format{variant, version, default_compression, std::move(ba2), is_gnrl, is_dx10, file_count};
}

} // namespace

result<detected_ba2_format> detect_ba2_format(std::span<const std::byte> bytes) {
  detail::binary_reader reader{bytes};
  const auto magic = reader.read_u32_le();
  if (!magic) {
    return magic.error();
  }
  if (magic.value() != ba2_btdx_magic) {
    return error{error_code::unsupported, "archive bytes do not start with BTDX magic"};
  }

  const auto version = reader.read_u32_le();
  if (!version) {
    return error{error_code::format_error, "BA2 header is truncated before version"};
  }

  const auto subtype = reader.read_u32_le();
  if (!subtype) {
    return error{error_code::format_error, "BA2 header is truncated before subtype"};
  }
  const auto is_gnrl = subtype.value() == ba2_gnrl_magic;
  const auto is_dx10 = subtype.value() == ba2_dx10_magic;
  if (!is_gnrl && !is_dx10) {
    return error{error_code::unsupported, "BA2 subtype is not GNRL or DX10"};
  }

  const auto file_count = reader.read_u32_le();
  if (!file_count) {
    return error{error_code::format_error, "BA2 header is truncated before file count"};
  }
  const auto file_table_offset = reader.read_u64_le();
  if (!file_table_offset) {
    return error{error_code::format_error, "BA2 header is truncated before file table offset"};
  }

  ba2_archive_metadata ba2{};
  switch (version.value()) {
  case ba2_fallout4_version:
    return detected_header(archive_variant::fallout4, version.value(), entry_compression::deflate, ba2, is_gnrl, is_dx10,
                           file_count.value());
  case ba2_starfield_v2_version: {
    const auto unknown1 = reader.read_u32_le();
    const auto unknown2 = reader.read_u32_le();
    if (!unknown1 || !unknown2) {
      return error{error_code::format_error, "Starfield BA2 v2 header is truncated before Unknown fields"};
    }
    ba2.starfield_unknown1 = unknown1.value();
    ba2.starfield_unknown2 = unknown2.value();
    return detected_header(archive_variant::starfield, version.value(), entry_compression::deflate, ba2, is_gnrl, is_dx10,
                           file_count.value());
  }
  case ba2_starfield_v3_version: {
    const auto unknown1 = reader.read_u32_le();
    const auto unknown2 = reader.read_u32_le();
    const auto compression_method = reader.read_u32_le();
    if (!unknown1 || !unknown2 || !compression_method) {
      return error{error_code::format_error, "Starfield BA2 v3 header is truncated before CompressionMethod"};
    }
    ba2.starfield_unknown1 = unknown1.value();
    ba2.starfield_unknown2 = unknown2.value();
    ba2.compression_method = compression_method.value();

    // TES5Edit routes CompressionMethod 3 to raw LZ4 block. The generated fixture corpus keeps
    // method 0 as the evidence-bounded non-LZ4 deflate path and rejects unknown methods.
    if (compression_method.value() == ba2_starfield_compression_lz4_block) {
      return detected_header(archive_variant::starfield, version.value(), entry_compression::lz4_block, ba2, is_gnrl,
                             is_dx10, file_count.value());
    }
    if (compression_method.value() == ba2_starfield_compression_deflate) {
      return detected_header(archive_variant::starfield, version.value(), entry_compression::deflate, ba2, is_gnrl,
                             is_dx10, file_count.value());
    }
    return error{error_code::unsupported, "Starfield BA2 v3 CompressionMethod is unsupported"};
  }
  default:
    return error{error_code::unsupported, "BA2 header version is not supported"};
  }
}

} // namespace libbsa::formats::ba2
