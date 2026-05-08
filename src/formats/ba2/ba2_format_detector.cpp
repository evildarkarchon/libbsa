#include "formats/ba2/ba2_format_detector.hpp"

#include <detail/binary_io.hpp>

#include <utility>

namespace libbsa::formats::ba2 {
namespace {

constexpr std::uint32_t fallout4_version = 1U;
constexpr std::uint32_t starfield_v2_version = 2U;
constexpr std::uint32_t starfield_v3_version = 3U;
constexpr std::uint32_t starfield_lz4_block_method = 3U;
constexpr std::uint32_t starfield_deflate_method = 0U;

bool matches_magic(std::span<const std::byte> bytes, char a, char b, char c, char d) {
  return bytes.size() == 4U && bytes[0] == static_cast<std::byte>(static_cast<unsigned char>(a)) &&
         bytes[1] == static_cast<std::byte>(static_cast<unsigned char>(b)) &&
         bytes[2] == static_cast<std::byte>(static_cast<unsigned char>(c)) &&
         bytes[3] == static_cast<std::byte>(static_cast<unsigned char>(d));
}

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
  const auto magic = reader.read_bytes(4);
  if (!magic) {
    return magic.error();
  }
  if (!matches_magic(magic.value(), 'B', 'T', 'D', 'X')) {
    return error{error_code::unsupported, "archive bytes do not start with BTDX magic"};
  }

  const auto version = reader.read_u32_le();
  if (!version) {
    return error{error_code::format_error, "BA2 header is truncated before version"};
  }

  const auto subtype = reader.read_bytes(4);
  if (!subtype) {
    return error{error_code::format_error, "BA2 header is truncated before subtype"};
  }
  const auto is_gnrl = matches_magic(subtype.value(), 'G', 'N', 'R', 'L');
  const auto is_dx10 = matches_magic(subtype.value(), 'D', 'X', '1', '0');
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

  if (is_dx10) {
    return error{error_code::unsupported, "BA2 DX10 texture archives are deferred to the DDS phase"};
  }

  ba2_archive_metadata ba2{};
  switch (version.value()) {
  case fallout4_version:
    return detected_header(archive_variant::fallout4, version.value(), entry_compression::deflate, ba2, is_gnrl, is_dx10,
                           file_count.value());
  case starfield_v2_version: {
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
  case starfield_v3_version: {
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
    if (compression_method.value() == starfield_lz4_block_method) {
      return detected_header(archive_variant::starfield, version.value(), entry_compression::lz4_block, ba2, is_gnrl,
                             is_dx10, file_count.value());
    }
    if (compression_method.value() == starfield_deflate_method) {
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
