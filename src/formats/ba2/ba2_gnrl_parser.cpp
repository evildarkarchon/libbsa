#include "formats/ba2/ba2_gnrl_parser.hpp"

#include <detail/archive_path.hpp>
#include <detail/binary_io.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

constexpr std::uint32_t ba2_btdx_magic = 0x5844'5442U;
constexpr std::uint32_t ba2_gnrl_magic = 0x4C52'4E47U;
// TES5Edit's BA2 GNRL record reader expects the BAADF00D sentinel after each record.
constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
constexpr std::uint32_t fallout4_version = 1U;
constexpr std::uint32_t starfield_v2_version = 2U;
constexpr std::uint32_t starfield_v3_version = 3U;
constexpr std::size_t common_header_size = 24U;
constexpr std::size_t starfield_v2_header_size = 32U;
constexpr std::size_t starfield_v3_header_size = 36U;
constexpr std::size_t gnrl_record_size = 36U;
// PackedSize == 0 denotes a raw BA2 GNRL payload; otherwise the parsed archive profile selects the codec.

struct header_fields {
  std::uint32_t magic;
  std::uint32_t version;
  std::uint32_t subtype;
  std::uint32_t file_count;
  std::uint64_t file_table_offset;
  ba2_archive_metadata ba2;
};

struct gnrl_record {
  std::uint32_t name_hash;
  std::uint32_t directory_hash;
  std::uint32_t unknown;
  std::uint64_t offset;
  std::uint32_t packed_size;
  std::uint32_t size;
};

bool multiply_fits(std::uint32_t count, std::size_t width, std::size_t& total) noexcept {
  if (width != 0U && count > std::numeric_limits<std::size_t>::max() / width) {
    return false;
  }
  total = static_cast<std::size_t>(count) * width;
  return true;
}

bool add_fits(std::size_t lhs, std::size_t rhs, std::size_t& total) noexcept {
  if (lhs > std::numeric_limits<std::size_t>::max() - rhs) {
    return false;
  }
  total = lhs + rhs;
  return true;
}

bool span_fits(std::size_t start, std::size_t length, std::size_t total) noexcept {
  return start <= total && length <= total - start;
}

bool span_fits_u64(std::uint64_t start, std::uint64_t length, std::uint64_t total) noexcept {
  return start <= total && length <= total - start;
}

bool spans_overlap_u64(std::uint64_t first_start, std::uint64_t first_length, std::uint64_t second_start,
                       std::uint64_t second_length) noexcept {
  if (first_length == 0U || second_length == 0U) {
    return false;
  }
  return first_start < second_start + second_length && second_start < first_start + first_length;
}

std::size_t header_size_for(std::uint32_t version) noexcept {
  if (version == starfield_v3_version) {
    return starfield_v3_header_size;
  }
  if (version == starfield_v2_version) {
    return starfield_v2_header_size;
  }
  return common_header_size;
}

result<std::vector<std::byte>> read_file_bytes_at(std::ifstream& input, std::uint64_t offset, std::size_t count,
                                                  std::string_view description) {
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, std::string{description} + " offset exceeds stream limits"};
  }
  if (count > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
    return error{error_code::format_error, std::string{description} + " size exceeds stream limits"};
  }

  std::vector<std::byte> bytes(count);
  input.clear();
  input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, std::string{"failed to seek while reading "} + std::string{description}};
  }
  input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (input.bad()) {
    return error{error_code::io_error, std::string{"failed while reading "} + std::string{description}};
  }
  if (static_cast<std::size_t>(input.gcount()) != bytes.size()) {
    return error{error_code::format_error, std::string{description} + " is truncated"};
  }
  return bytes;
}

std::string bytes_to_string(std::span<const std::byte> bytes) {
  std::string result;
  result.reserve(bytes.size());
  for (const auto value : bytes) {
    result.push_back(static_cast<char>(std::to_integer<unsigned char>(value)));
  }
  return result;
}

void normalize_original_separators(std::string& value) {
  std::replace(value.begin(), value.end(), '\\', '/');
}

result<header_fields> read_header(detail::binary_reader& reader) {
  const auto magic = reader.read_u32_le();
  const auto version = reader.read_u32_le();
  const auto subtype = reader.read_u32_le();
  const auto file_count = reader.read_u32_le();
  const auto file_table_offset = reader.read_u64_le();
  if (!magic || !version || !subtype || !file_count || !file_table_offset) {
    return error{error_code::format_error, "BA2 GNRL fixed header is truncated before FileTableOffset"};
  }

  ba2_archive_metadata ba2{};
  if (version.value() >= starfield_v2_version) {
    const auto unknown1 = reader.read_u32_le();
    const auto unknown2 = reader.read_u32_le();
    if (!unknown1 || !unknown2) {
      return error{error_code::format_error, "BA2 GNRL Starfield v2 header fields are truncated"};
    }
    ba2.starfield_unknown1 = unknown1.value();
    ba2.starfield_unknown2 = unknown2.value();
  }
  if (version.value() >= starfield_v3_version) {
    const auto compression_method = reader.read_u32_le();
    if (!compression_method) {
      return error{error_code::format_error, "BA2 GNRL Starfield v3 CompressionMethod is truncated"};
    }
    ba2.compression_method = compression_method.value();
  }

  return header_fields{magic.value(), version.value(), subtype.value(), file_count.value(), file_table_offset.value(), ba2};
}

result<std::vector<gnrl_record>> read_records(detail::binary_reader& reader, std::uint32_t file_count) {
  std::vector<gnrl_record> records;
  records.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto name_hash = reader.read_u32_le();
    auto skipped_ext = reader.skip(4U);
    const auto directory_hash = reader.read_u32_le();
    const auto unknown = reader.read_u32_le();
    const auto offset = reader.read_u64_le();
    const auto packed_size = reader.read_u32_le();
    const auto size = reader.read_u32_le();
    const auto sentinel = reader.read_u32_le();
    if (!name_hash || !skipped_ext || !directory_hash || !unknown || !offset || !packed_size || !size || !sentinel) {
      return error{error_code::format_error, "BA2 GNRL record table is truncated"};
    }
    if (sentinel.value() != ba2_record_sentinel) {
      return error{error_code::format_error, "BA2 GNRL record BAADF00D sentinel is invalid"};
    }
    records.push_back(gnrl_record{name_hash.value(), directory_hash.value(), unknown.value(), offset.value(),
                                  packed_size.value(), size.value()});
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
      return error{error_code::format_error, "BA2 GNRL filename table is truncated before UInt16 length"};
    }
    const auto bytes = reader.read_bytes(length.value());
    if (!bytes) {
      return error{error_code::format_error, "BA2 GNRL filename table is truncated before name bytes"};
    }
    if (bytes.value().empty()) {
      return error{error_code::format_error, "BA2 GNRL filename table contains an empty name"};
    }
    names.push_back(bytes_to_string(bytes.value()));
  }
  consumed = reader.position();
  return names;
}

result<std::vector<std::string>> read_names_from_file(std::ifstream& input, std::uint64_t file_table_offset,
                                                      std::uint32_t file_count, std::uint64_t archive_size,
                                                      std::size_t& consumed) {
  if (file_table_offset > archive_size) {
    return error{error_code::format_error, "BA2 GNRL FileTableOffset is outside archive bytes"};
  }

  std::vector<std::string> names;
  names.reserve(file_count);
  std::uint64_t cursor = file_table_offset;
  for (std::uint32_t index = 0; index < file_count; ++index) {
    if (!span_fits_u64(cursor, 2U, archive_size)) {
      return error{error_code::format_error, "BA2 GNRL filename table is truncated before UInt16 length"};
    }
    auto length_bytes = read_file_bytes_at(input, cursor, 2U, "BA2 GNRL filename length");
    if (!length_bytes) {
      return length_bytes.error();
    }
    detail::binary_reader length_reader{length_bytes.value()};
    const auto length = length_reader.read_u16_le();
    if (!length) {
      return error{error_code::format_error, "BA2 GNRL filename table is truncated before UInt16 length"};
    }
    cursor += 2U;
    if (!span_fits_u64(cursor, length.value(), archive_size)) {
      return error{error_code::format_error, "BA2 GNRL filename table is truncated before name bytes"};
    }
    auto name_bytes = read_file_bytes_at(input, cursor, length.value(), "BA2 GNRL filename bytes");
    if (!name_bytes) {
      return name_bytes.error();
    }
    if (name_bytes.value().empty()) {
      return error{error_code::format_error, "BA2 GNRL filename table contains an empty name"};
    }
    names.push_back(bytes_to_string(name_bytes.value()));
    cursor += length.value();
  }

  const auto consumed_u64 = cursor - file_table_offset;
  if (consumed_u64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, "BA2 GNRL filename table size exceeds platform limits"};
  }
  consumed = static_cast<std::size_t>(consumed_u64);
  return names;
}

entry_compression compression_for(const gnrl_record& record, detected_ba2_format detected) noexcept {
  if (record.packed_size == 0U) {
    return entry_compression::none;
  }
  return detected.default_compression;
}

result<std::vector<entry_metadata>> materialize_entries(std::uint64_t archive_size,
                                                        std::uint64_t records_end,
                                                        std::uint64_t name_table_offset,
                                                        std::uint64_t name_table_end,
                                                        std::span<const gnrl_record> records,
                                                        std::span<const std::string> names,
                                                        detected_ba2_format detected) {
  std::vector<entry_metadata> entries;
  entries.reserve(records.size());
  std::unordered_set<std::string> canonical_paths;

  for (std::size_t index = 0; index < records.size(); ++index) {
    auto original_path = names[index];
    normalize_original_separators(original_path);
    auto canonical = detail::normalize_archive_path(original_path);
    if (!canonical) {
      return error{error_code::format_error, "BA2 GNRL filename table contains an invalid archive path"};
    }
    if (!canonical_paths.insert(canonical.value().value).second) {
      return error{error_code::format_error, "BA2 GNRL contains duplicate canonical archive paths"};
    }

    const auto stored_size = records[index].packed_size != 0U ? records[index].packed_size : records[index].size;
    if (!span_fits_u64(records[index].offset, stored_size, archive_size)) {
      return error{error_code::format_error, "BA2 GNRL entry payload span is outside the archive"};
    }
    // BSArchPro writes GNRL payloads after the fixed header and records; spans into this prefix
    // would later extract archive metadata bytes as if they were file payload.
    if (spans_overlap_u64(records[index].offset, stored_size, 0U, records_end)) {
      return error{error_code::format_error, "BA2 GNRL entry payload span intersects header or record table"};
    }
    if (spans_overlap_u64(records[index].offset, stored_size, name_table_offset, name_table_end - name_table_offset)) {
      return error{error_code::format_error, "BA2 GNRL filename table intersects payload data"};
    }

    entries.push_back(entry_metadata{canonical.value().value,
                                     std::move(original_path),
                                     records[index].size,
                                     stored_size,
                                     records[index].offset,
                                     records[index].name_hash,
                                     compression_for(records[index], detected),
                                     records[index].unknown,
                                     false,
                                     0U});
  }

  std::sort(entries.begin(), entries.end(), [](const entry_metadata& lhs, const entry_metadata& rhs) {
    return lhs.path < rhs.path;
  });
  return entries;
}

result<ba2_gnrl_archive> parse_ba2_gnrl_archive_impl(std::span<const std::byte> metadata_bytes, std::size_t archive_size,
                                                     detected_ba2_format detected) {
  if (!detected.is_gnrl || detected.is_dx10) {
    return error{error_code::unsupported, "detected BA2 format is not GNRL"};
  }

  detail::binary_reader reader{metadata_bytes};
  auto header = read_header(reader);
  if (!header) {
    return header.error();
  }
  if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_gnrl_magic) {
    return error{error_code::format_error, "BA2 GNRL header magic or subtype is invalid"};
  }
  if (header.value().version != detected.version || header.value().file_count != detected.file_count) {
    return error{error_code::format_error, "BA2 GNRL detected header does not match parsed header"};
  }

  std::size_t records_size = 0;
  if (!multiply_fits(header.value().file_count, gnrl_record_size, records_size)) {
    return error{error_code::format_error, "BA2 GNRL record table is too large"};
  }
  std::size_t records_end = 0;
  if (!add_fits(header_size_for(header.value().version), records_size, records_end) ||
      header.value().file_table_offset < records_end || header.value().file_table_offset > archive_size) {
    return error{error_code::format_error, "BA2 GNRL FileTableOffset is outside the metadata span"};
  }

  auto records = read_records(reader, header.value().file_count);
  if (!records) {
    return records.error();
  }

  const auto file_table_offset = static_cast<std::size_t>(header.value().file_table_offset);
  if (!span_fits(file_table_offset, metadata_bytes.size() - file_table_offset, metadata_bytes.size())) {
    return error{error_code::format_error, "BA2 GNRL FileTableOffset is outside archive bytes"};
  }
  std::size_t name_table_consumed = 0;
  auto names = read_names(metadata_bytes.subspan(file_table_offset), header.value().file_count, name_table_consumed);
  if (!names) {
    return names.error();
  }
  std::size_t name_table_end = 0;
  if (!add_fits(file_table_offset, name_table_consumed, name_table_end)) {
    return error{error_code::format_error, "BA2 GNRL filename table is too large"};
  }

  auto entries = materialize_entries(static_cast<std::uint64_t>(archive_size), static_cast<std::uint64_t>(records_end),
                                     file_table_offset, name_table_end, records.value(), names.value(), detected);
  if (!entries) {
    return entries.error();
  }

  return ba2_gnrl_archive{archive_metadata{archive_type::ba2,
                                           detected.variant,
                                           header.value().version,
                                           0U,
                                           header.value().file_count,
                                           detected.default_compression,
                                           header.value().ba2},
                          std::move(entries.value())};
}

} // namespace

result<ba2_gnrl_archive> parse_ba2_gnrl_archive(std::span<const std::byte> bytes, detected_ba2_format detected) {
  return parse_ba2_gnrl_archive_impl(bytes, bytes.size(), detected);
}

result<ba2_gnrl_archive> parse_ba2_gnrl_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected) {
  if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, "BA2 GNRL archive exceeds platform limits"};
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  auto fixed_header = read_file_bytes_at(input, 0U, header_size_for(detected.version), "BA2 GNRL fixed header");
  if (!fixed_header) {
    return fixed_header.error();
  }
  detail::binary_reader header_reader{fixed_header.value()};
  auto header = read_header(header_reader);
  if (!header) {
    return header.error();
  }
  if (header.value().magic != ba2_btdx_magic || header.value().subtype != ba2_gnrl_magic) {
    return error{error_code::format_error, "BA2 GNRL header magic or subtype is invalid"};
  }
  if (header.value().version != detected.version || header.value().file_count != detected.file_count) {
    return error{error_code::format_error, "BA2 GNRL detected header does not match parsed header"};
  }

  std::size_t records_size = 0;
  if (!multiply_fits(header.value().file_count, gnrl_record_size, records_size)) {
    return error{error_code::format_error, "BA2 GNRL record table is too large"};
  }
  std::size_t records_end = 0;
  if (!add_fits(header_size_for(header.value().version), records_size, records_end) ||
      header.value().file_table_offset < records_end || header.value().file_table_offset > archive_size) {
    return error{error_code::format_error, "BA2 GNRL FileTableOffset is outside the metadata span"};
  }

  const auto metadata_size = records_end;
  auto metadata_bytes = read_file_bytes_at(input, 0U, metadata_size, "BA2 GNRL header and record table");
  if (!metadata_bytes) {
    return metadata_bytes.error();
  }

  detail::binary_reader metadata_reader{metadata_bytes.value()};
  auto parsed_header = read_header(metadata_reader);
  if (!parsed_header) {
    return parsed_header.error();
  }
  auto records = read_records(metadata_reader, header.value().file_count);
  if (!records) {
    return records.error();
  }

  std::size_t name_table_consumed = 0;
  // The writer-required BA2 layout can place payload bytes before the final filename table, so host-file open
  // parses exactly file_count length-prefixed names from FileTableOffset instead of deriving a table size from
  // the first payload offset. This preserves bounded open behavior for both sparse payloads and end tables.
  auto names = read_names_from_file(input, header.value().file_table_offset, header.value().file_count, archive_size,
                                    name_table_consumed);
  if (!names) {
    return names.error();
  }
  const auto name_table_end = header.value().file_table_offset + static_cast<std::uint64_t>(name_table_consumed);
  auto entries = materialize_entries(archive_size, static_cast<std::uint64_t>(records_end), header.value().file_table_offset,
                                     name_table_end, records.value(), names.value(), detected);
  if (!entries) {
    return entries.error();
  }

  return ba2_gnrl_archive{archive_metadata{archive_type::ba2,
                                           detected.variant,
                                           header.value().version,
                                           0U,
                                           header.value().file_count,
                                           detected.default_compression,
                                           header.value().ba2},
                          std::move(entries.value())};
}

} // namespace libbsa::formats::ba2
