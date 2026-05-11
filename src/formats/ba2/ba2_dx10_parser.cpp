#include "formats/ba2/ba2_dx10_parser.hpp"

#include "texture/dds_layout.hpp"

#include <detail/archive_path.hpp>
#include <detail/binary_io.hpp>
#include <detail/byte_vector.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

constexpr std::uint32_t ba2_btdx_magic = 0x5844'5442U;
constexpr std::uint32_t ba2_dx10_magic = 0x3031'5844U;
constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
constexpr std::uint16_t ba2_dx10_chunk_header_size = 24U;
constexpr std::uint16_t ba2_dx10_cubemap_raw = 2049U;
constexpr std::uint32_t starfield_v2_version = 2U;
constexpr std::uint32_t starfield_v3_version = 3U;
constexpr std::size_t common_header_size = 24U;
constexpr std::size_t starfield_v2_header_size = 32U;
constexpr std::size_t starfield_v3_header_size = 36U;

struct header_fields {
  std::uint32_t magic;
  std::uint32_t version;
  std::uint32_t subtype;
  std::uint32_t file_count;
  std::uint64_t file_table_offset;
  ba2_archive_metadata ba2;
};

struct dx10_chunk_record {
  std::uint64_t offset;
  std::uint32_t packed_size;
  std::uint32_t raw_size;
  std::uint16_t start_mip;
  std::uint16_t end_mip;
};

struct dx10_record {
  std::uint32_t name_hash;
  std::uint32_t directory_hash;
  std::uint8_t unknown_tex;
  std::uint8_t chunk_count;
  std::uint16_t chunk_header_size;
  std::uint16_t height;
  std::uint16_t width;
  std::uint8_t num_mips;
  std::uint8_t dxgi_format;
  std::uint16_t cube_maps_raw;
  std::vector<dx10_chunk_record> chunks;
};

using detail::add_fits;
using detail::archive_string_from_bytes;
using detail::multiply_fits;
using detail::normalize_display_separators;
using detail::read_file_bytes_at;
using detail::span_fits_u64;

std::size_t header_size_for(std::uint32_t version) noexcept {
  if (version == starfield_v3_version) {
    return starfield_v3_header_size;
  }
  if (version == starfield_v2_version) {
    return starfield_v2_header_size;
  }
  return common_header_size;
}

result<void> append_u16_le(std::vector<std::byte>& output, std::uint16_t value) {
  const std::array bytes{static_cast<std::byte>(value & 0xFFU), static_cast<std::byte>((value >> 8U) & 0xFFU)};
  return detail::append_byte_vector(output, bytes, "BA2 DX10 encoded filename length");
}

result<std::vector<std::byte>> parse_ba2_dx10_names_from_file(std::ifstream& input,
                                                              std::uint64_t file_table_offset,
                                                              std::uint64_t first_payload_offset,
                                                              std::uint32_t file_count) {
  if (file_table_offset > first_payload_offset) {
    return error{error_code::format_error, "BA2 DX10 filename table overlaps payload data"};
  }

  std::vector<std::byte> encoded_names;
  std::size_t minimum_encoded_size = 0;
  if (!multiply_fits(file_count, 2U, minimum_encoded_size)) {
    return error{error_code::format_error, "BA2 DX10 encoded filename table is too large"};
  }
  auto reserved = detail::reserve_byte_vector(encoded_names, minimum_encoded_size, "BA2 DX10 encoded filename table");
  if (!reserved) {
    return reserved.error();
  }

  std::uint64_t cursor = file_table_offset;
  for (std::uint32_t index = 0; index < file_count; ++index) {
    if (first_payload_offset - cursor < 2U) {
      return error{error_code::format_error, "BA2 DX10 filename table is truncated before UInt16 length"};
    }

    auto length_bytes = read_file_bytes_at(input, cursor, 2U, "BA2 DX10 filename length");
    if (!length_bytes) {
      return length_bytes.error();
    }

    detail::binary_reader length_reader{length_bytes.value()};
    const auto length = length_reader.read_u16_le();
    if (!length) {
      return error{error_code::format_error, "BA2 DX10 filename table is truncated before UInt16 length"};
    }
    cursor += 2U;
    if (static_cast<std::uint64_t>(length.value()) > first_payload_offset - cursor) {
      return error{error_code::format_error, "BA2 DX10 filename table name crosses payload data"};
    }

    auto name_bytes = read_file_bytes_at(input, cursor, length.value(), "BA2 DX10 filename bytes");
    if (!name_bytes) {
      return name_bytes.error();
    }

    auto appended_length = append_u16_le(encoded_names, length.value());
    if (!appended_length) {
      return appended_length.error();
    }
    auto appended_name = detail::append_byte_vector(encoded_names, name_bytes.value(), "BA2 DX10 encoded filename bytes");
    if (!appended_name) {
      return appended_name.error();
    }
    cursor += length.value();
  }

  return encoded_names;
}

result<header_fields> read_header(detail::binary_reader& reader) {
  const auto magic = reader.read_u32_le();
  const auto version = reader.read_u32_le();
  const auto subtype = reader.read_u32_le();
  const auto file_count = reader.read_u32_le();
  const auto file_table_offset = reader.read_u64_le();
  if (!magic || !version || !subtype || !file_count || !file_table_offset) {
    return error{error_code::format_error, "BA2 DX10 fixed header is truncated before FileTableOffset"};
  }

  ba2_archive_metadata ba2{};
  if (version.value() >= starfield_v2_version) {
    const auto unknown1 = reader.read_u32_le();
    const auto unknown2 = reader.read_u32_le();
    if (!unknown1 || !unknown2) {
      return error{error_code::format_error, "BA2 DX10 Starfield v2 header fields are truncated"};
    }
    ba2.starfield_unknown1 = unknown1.value();
    ba2.starfield_unknown2 = unknown2.value();
  }
  if (version.value() >= starfield_v3_version) {
    const auto compression_method = reader.read_u32_le();
    if (!compression_method) {
      return error{error_code::format_error, "BA2 DX10 Starfield v3 CompressionMethod is truncated"};
    }
    ba2.compression_method = compression_method.value();
  }

  return header_fields{magic.value(), version.value(), subtype.value(), file_count.value(), file_table_offset.value(), ba2};
}

result<std::vector<dx10_record>> read_records(detail::binary_reader& reader, std::uint32_t file_count,
                                              std::uint64_t file_table_offset) {
  std::vector<dx10_record> records;
  records.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto name_hash = reader.read_u32_le();
    const auto skipped_ext = reader.skip(4U);
    const auto directory_hash = reader.read_u32_le();
    const auto unknown_tex = reader.read_u8();
    const auto chunk_count = reader.read_u8();
    const auto chunk_header_size = reader.read_u16_le();
    const auto height = reader.read_u16_le();
    const auto width = reader.read_u16_le();
    const auto num_mips = reader.read_u8();
    const auto dxgi_format = reader.read_u8();
    const auto cube_maps_raw = reader.read_u16_le();
    if (!name_hash || !skipped_ext || !directory_hash || !unknown_tex || !chunk_count || !chunk_header_size || !height ||
        !width || !num_mips || !dxgi_format || !cube_maps_raw) {
      return error{error_code::format_error, "BA2 DX10 record table is truncated"};
    }
    if (chunk_count.value() == 0U) {
      return error{error_code::format_error, "BA2 DX10 record has no texture chunks"};
    }
    // TES5Edit writes and reads a fixed 24-byte DX10 chunk header. Phase 6 rejects other widths so
    // parser position cannot drift into payload or filename bytes.
    if (chunk_header_size.value() != ba2_dx10_chunk_header_size) {
      return error{error_code::format_error, "BA2 DX10 chunk_header_size is unsupported"};
    }

    dx10_record record{name_hash.value(),
                       directory_hash.value(),
                       unknown_tex.value(),
                       chunk_count.value(),
                       chunk_header_size.value(),
                       height.value(),
                       width.value(),
                       num_mips.value(),
                       dxgi_format.value(),
                       cube_maps_raw.value(),
                       {}};
    record.chunks.reserve(chunk_count.value());
    for (std::uint8_t chunk_index = 0; chunk_index < chunk_count.value(); ++chunk_index) {
      const auto offset = reader.read_u64_le();
      const auto packed_size = reader.read_u32_le();
      const auto raw_size = reader.read_u32_le();
      const auto start_mip = reader.read_u16_le();
      const auto end_mip = reader.read_u16_le();
      const auto sentinel = reader.read_u32_le();
      if (!offset || !packed_size || !raw_size || !start_mip || !end_mip || !sentinel) {
        return error{error_code::format_error, "BA2 DX10 chunk table is truncated"};
      }
      if (sentinel.value() != ba2_record_sentinel) {
        return error{error_code::format_error, "BA2 DX10 chunk BAADF00D sentinel is invalid"};
      }
      record.chunks.push_back(
          dx10_chunk_record{offset.value(), packed_size.value(), raw_size.value(), start_mip.value(), end_mip.value()});
    }
    records.push_back(std::move(record));
  }
  if (reader.position() != file_table_offset) {
    return error{error_code::format_error, "BA2 DX10 FileTableOffset does not match texture record table size"};
  }
  return records;
}

result<std::vector<std::string>> read_names(std::span<const std::byte> name_table, std::uint32_t file_count,
                                            std::size_t& consumed) {
  detail::binary_reader reader{name_table};
  std::vector<std::string> names;
  names.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto length = reader.read_u16_le();
    if (!length) {
      return error{error_code::format_error, "BA2 DX10 filename table is truncated before UInt16 length"};
    }
    const auto bytes = reader.read_bytes(length.value());
    if (!bytes) {
      return error{error_code::format_error, "BA2 DX10 filename table is truncated before name bytes"};
    }
    if (bytes.value().empty()) {
      return error{error_code::format_error, "BA2 DX10 filename table contains an empty name"};
    }
    auto name = archive_string_from_bytes(bytes.value(), "BA2 DX10 filename bytes");
    if (!name) {
      return name.error();
    }
    names.push_back(std::move(name.value()));
  }
  consumed = reader.position();
  return names;
}

entry_compression compression_for(const dx10_chunk_record& chunk, detected_ba2_format detected) noexcept {
  if (chunk.packed_size == 0U) {
    return entry_compression::none;
  }
  // Starfield v3 CompressionMethod 3 is normalized by the detector into raw-LZ4 block metadata here;
  // DX10 chunks still decide raw-vs-compressed from PackedSize, never filename or extension spelling.
  return detected.default_compression;
}

result<std::uint64_t> first_payload_offset_for(std::span<const dx10_record> records, std::uint64_t archive_size) {
  std::uint64_t first_payload_offset = archive_size;
  for (const auto& record : records) {
    for (const auto& chunk : record.chunks) {
      const auto stored_size = static_cast<std::uint64_t>(chunk.packed_size != 0U ? chunk.packed_size : chunk.raw_size);
      if (stored_size == 0U || chunk.raw_size == 0U) {
        return error{error_code::format_error, "BA2 DX10 chunk sizes are inconsistent"};
      }
      if (!span_fits_u64(chunk.offset, stored_size, archive_size)) {
        return error{error_code::format_error, "BA2 DX10 chunk payload span is outside the archive"};
      }
      first_payload_offset = std::min(first_payload_offset, chunk.offset);
    }
  }
  return first_payload_offset;
}

std::uint32_t inferred_array_size(const dx10_record& record) noexcept {
  // TES5Edit treats CubeMaps == 2049 as the cubemap signal. Preserve cube_maps_raw separately so
  // future compatibility work can revisit broader Bethesda-specific flag meanings without data loss.
  std::uint32_t slices = 0;
  for (const auto& chunk : record.chunks) {
    if (chunk.start_mip == 0U) {
      ++slices;
    }
  }
  if (record.cube_maps_raw == ba2_dx10_cubemap_raw) {
    // BA2 stores one start-mip sequence per cubemap face, so logical cube-array count is face
    // groups divided by the six DDS cubemap faces rather than a value encoded in CubeMaps.
    return std::max(1U, slices / 6U);
  }
  return std::max(1U, slices);
}

result<std::vector<texture_chunk_metadata>> public_chunks_for(const dx10_record& record, detected_ba2_format detected) {
  std::vector<texture_chunk_metadata> archive_chunks;
  archive_chunks.reserve(record.chunks.size());
  for (const auto& chunk : record.chunks) {
    const auto compression = compression_for(chunk, detected);
    archive_chunks.push_back(texture_chunk_metadata{chunk.offset,
                                                    chunk.packed_size == 0U ? chunk.raw_size : chunk.packed_size,
                                                    chunk.raw_size,
                                                    chunk.start_mip,
                                                    chunk.end_mip,
                                                    compression});
  }

  texture::dds_texture_layout layout{record.width,
                                     record.height,
                                     record.num_mips,
                                     record.dxgi_format,
                                     inferred_array_size(record),
                                     record.cube_maps_raw == ba2_dx10_cubemap_raw};
  auto ordered_segments = texture::validate_and_order_chunks(layout, archive_chunks);
  if (!ordered_segments) {
    return ordered_segments.error();
  }

  std::vector<texture_chunk_metadata> ordered_chunks;
  ordered_chunks.reserve(ordered_segments.value().size());
  for (const auto& logical_texture_segment : ordered_segments.value()) {
    ordered_chunks.push_back(archive_chunks.at(logical_texture_segment.source_chunk_index));
  }
  return ordered_chunks;
}

result<std::vector<entry_metadata>> materialize_entries(std::size_t archive_size,
                                                        std::size_t name_table_end,
                                                        std::span<const dx10_record> records,
                                                        std::span<const std::string> names,
                                                        detected_ba2_format detected) {
  std::vector<entry_metadata> entries;
  entries.reserve(records.size());
  std::unordered_set<std::string> canonical_paths;

  auto first_payload_offset = first_payload_offset_for(records, archive_size);
  if (!first_payload_offset) {
    return first_payload_offset.error();
  }
  if (name_table_end > first_payload_offset.value()) {
    return error{error_code::format_error, "BA2 DX10 filename table overlaps payload data"};
  }

  for (std::size_t index = 0; index < records.size(); ++index) {
    auto original_path = names[index];
    normalize_display_separators(original_path);
    auto canonical = detail::normalize_archive_path(original_path);
    if (!canonical) {
      return error{error_code::format_error, "BA2 DX10 filename table contains an invalid archive path"};
    }
    if (!canonical_paths.insert(canonical.value().value).second) {
      return error{error_code::format_error, "BA2 DX10 contains duplicate canonical archive paths"};
    }

    auto chunks = public_chunks_for(records[index], detected);
    if (!chunks) {
      return chunks.error();
    }

    std::uint64_t raw_payload_size = 0;
    std::uint64_t stored_payload_size = 0;
    bool has_compressed_chunk = false;
    std::uint64_t payload_offset = std::numeric_limits<std::uint64_t>::max();
    for (const auto& chunk : chunks.value()) {
      raw_payload_size += chunk.raw_size;
      stored_payload_size += chunk.stored_size;
      payload_offset = std::min(payload_offset, chunk.payload_offset);
      has_compressed_chunk = has_compressed_chunk || chunk.compression != entry_compression::none;
    }

    const auto array_size = inferred_array_size(records[index]);
    const auto is_cubemap = records[index].cube_maps_raw == ba2_dx10_cubemap_raw;
    texture_metadata texture{records[index].width,
                             records[index].height,
                             records[index].num_mips,
                             records[index].dxgi_format,
                             array_size,
                             is_cubemap,
                             records[index].unknown_tex,
                             records[index].cube_maps_raw,
                             std::move(chunks.value())};

    entries.push_back(entry_metadata{canonical.value().value,
                                     std::move(original_path),
                                     148U + raw_payload_size,
                                     stored_payload_size,
                                     payload_offset,
                                     records[index].name_hash,
                                     has_compressed_chunk ? detected.default_compression : entry_compression::none,
                                     records[index].unknown_tex,
                                     false,
                                     0U,
                                     std::move(texture)});
  }

  std::sort(entries.begin(), entries.end(), [](const entry_metadata& lhs, const entry_metadata& rhs) {
    return lhs.path < rhs.path;
  });
  return entries;
}

result<ba2_dx10_archive> parse_ba2_dx10_archive_impl(std::span<const std::byte> metadata_bytes, std::size_t archive_size,
                                                     detected_ba2_format detected) {
  if (!detected.is_dx10 || detected.is_gnrl) {
    return error{error_code::unsupported, "detected BA2 format is not DX10"};
  }

  detail::binary_reader reader{metadata_bytes};
  auto header = read_header(reader);
  if (!header) {
    return header.error();
  }
  if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_dx10_magic) {
    return error{error_code::format_error, "BA2 DX10 header magic or subtype is invalid"};
  }
  if (header.value().version != detected.version || header.value().file_count != detected.file_count) {
    return error{error_code::format_error, "BA2 DX10 detected header does not match parsed header"};
  }
  if (header.value().file_table_offset > archive_size) {
    return error{error_code::format_error, "BA2 DX10 FileTableOffset is outside the metadata span"};
  }

  auto records = read_records(reader, header.value().file_count, header.value().file_table_offset);
  if (!records) {
    return records.error();
  }

  const auto file_table_offset = static_cast<std::size_t>(header.value().file_table_offset);
  std::size_t name_table_consumed = 0;
  auto names = read_names(metadata_bytes.subspan(file_table_offset), header.value().file_count, name_table_consumed);
  if (!names) {
    return names.error();
  }
  std::size_t name_table_end = 0;
  if (!add_fits(file_table_offset, name_table_consumed, name_table_end)) {
    return error{error_code::format_error, "BA2 DX10 filename table is too large"};
  }

  auto entries = materialize_entries(archive_size, name_table_end, records.value(), names.value(), detected);
  if (!entries) {
    return entries.error();
  }

  return ba2_dx10_archive{archive_metadata{archive_type::ba2,
                                           detected.variant,
                                           header.value().version,
                                           0U,
                                           header.value().file_count,
                                           detected.default_compression,
                                           header.value().ba2},
                          std::move(entries.value())};
}

} // namespace

result<ba2_dx10_archive> parse_ba2_dx10_archive(std::span<const std::byte> bytes, detected_ba2_format detected) {
  return parse_ba2_dx10_archive_impl(bytes, bytes.size(), detected);
}

result<ba2_dx10_archive> parse_ba2_dx10_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected) {
  if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, "BA2 DX10 archive exceeds platform limits"};
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  auto fixed_header = read_file_bytes_at(input, 0U, header_size_for(detected.version), "BA2 DX10 fixed header");
  if (!fixed_header) {
    return fixed_header.error();
  }
  detail::binary_reader header_reader{fixed_header.value()};
  auto header = read_header(header_reader);
  if (!header) {
    return header.error();
  }
  if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_dx10_magic) {
    return error{error_code::format_error, "BA2 DX10 header magic or subtype is invalid"};
  }
  if (header.value().version != detected.version || header.value().file_count != detected.file_count) {
    return error{error_code::format_error, "BA2 DX10 detected header does not match parsed header"};
  }
  if (header.value().file_table_offset > archive_size || header.value().file_table_offset < header_size_for(header.value().version)) {
    return error{error_code::format_error, "BA2 DX10 FileTableOffset is outside the metadata span"};
  }

  auto metadata_bytes = read_file_bytes_at(input, 0U, static_cast<std::size_t>(header.value().file_table_offset),
                                           "BA2 DX10 header and texture records");
  if (!metadata_bytes) {
    return metadata_bytes.error();
  }
  detail::binary_reader metadata_reader{metadata_bytes.value()};
  auto parsed_header = read_header(metadata_reader);
  if (!parsed_header) {
    return parsed_header.error();
  }
  auto records = read_records(metadata_reader, header.value().file_count, header.value().file_table_offset);
  if (!records) {
    return records.error();
  }

  auto first_payload_offset = first_payload_offset_for(records.value(), archive_size);
  if (!first_payload_offset) {
    return first_payload_offset.error();
  }
  if (header.value().file_table_offset > first_payload_offset.value()) {
    return error{error_code::format_error, "BA2 DX10 filename table overlaps payload data"};
  }
  // BA2 DX10 filename tables are count-delimited, so sparse padding between the last encoded name
  // and first payload must not be allocated during open/list metadata parsing.
  auto name_table_bytes = parse_ba2_dx10_names_from_file(input,
                                                         header.value().file_table_offset,
                                                         first_payload_offset.value(),
                                                         header.value().file_count);
  if (!name_table_bytes) {
    return name_table_bytes.error();
  }
  auto appended_names = detail::append_byte_vector(metadata_bytes.value(), name_table_bytes.value(),
                                                  "BA2 DX10 metadata filename table");
  if (!appended_names) {
    return appended_names.error();
  }
  return parse_ba2_dx10_archive_impl(metadata_bytes.value(), static_cast<std::size_t>(archive_size), detected);
}

} // namespace libbsa::formats::ba2
