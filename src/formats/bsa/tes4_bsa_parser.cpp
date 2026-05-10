#include "formats/bsa/tes4_bsa_parser.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

constexpr std::uint32_t sse_version = 0x69U;
constexpr std::uint32_t archive_include_directory_names = 0x0001U;
constexpr std::uint32_t archive_include_file_names = 0x0002U;
constexpr std::uint32_t archive_compress_by_default = 0x0004U;
constexpr std::uint32_t archive_embed_names = 0x0100U;
constexpr std::uint32_t file_size_compression_toggle = 0x40000000U;
constexpr std::size_t fixed_header_size = 36U;
constexpr std::size_t legacy_folder_record_size = 16U;
constexpr std::size_t sse_folder_record_size = 24U;
constexpr std::size_t file_record_size = 16U;

struct header_fields {
  std::uint32_t version;
  std::uint32_t folder_offset;
  std::uint32_t archive_flags;
  std::uint32_t folder_count;
  std::uint32_t file_count;
  std::uint32_t total_folder_name_length;
  std::uint32_t total_file_name_length;
  std::uint32_t file_flags;
};

struct folder_record {
  std::uint32_t file_count;
  std::uint64_t offset;
};

struct file_record {
  std::uint64_t hash;
  std::uint32_t size_flags;
  std::uint32_t offset;
};

struct folder_block {
  std::string name;
  std::vector<file_record> files;
};

result<void> skip_checked(detail::binary_reader& reader, std::size_t count) {
  auto skipped = reader.skip(count);
  if (!skipped) {
    return error{error_code::format_error, "TES4 BSA table is truncated"};
  }
  return {};
}

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

bool non_empty_span_intersects_prefix(std::size_t start, std::size_t length, std::size_t prefix_size) noexcept {
  return length != 0U && start < prefix_size;
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

result<header_fields> read_header(detail::binary_reader& reader) {
  const auto magic = reader.read_bytes(4);
  if (!magic) {
    return magic.error();
  }
  if (magic.value().size() != 4U || static_cast<char>(std::to_integer<unsigned char>(magic.value()[0])) != 'B' ||
      static_cast<char>(std::to_integer<unsigned char>(magic.value()[1])) != 'S' ||
      static_cast<char>(std::to_integer<unsigned char>(magic.value()[2])) != 'A' ||
      magic.value()[3] != std::byte{0}) {
    return error{error_code::unsupported, "TES4 BSA magic is not supported"};
  }

  const auto version = reader.read_u32_le();
  const auto folder_offset = reader.read_u32_le();
  const auto archive_flags = reader.read_u32_le();
  const auto folder_count = reader.read_u32_le();
  const auto file_count = reader.read_u32_le();
  const auto total_folder_name_length = reader.read_u32_le();
  const auto total_file_name_length = reader.read_u32_le();
  const auto file_flags = reader.read_u32_le();
  if (!version || !folder_offset || !archive_flags || !folder_count || !file_count || !total_folder_name_length ||
      !total_file_name_length || !file_flags) {
    return error{error_code::format_error, "TES4 BSA fixed header is truncated"};
  }

  return header_fields{version.value(),
                       folder_offset.value(),
                       archive_flags.value(),
                       folder_count.value(),
                       file_count.value(),
                       total_folder_name_length.value(),
                       total_file_name_length.value(),
                        file_flags.value()};
}

result<std::size_t> metadata_table_size(const header_fields& header, std::size_t folder_record_size,
                                        std::size_t archive_size) {
  std::size_t folder_records_size = 0;
  std::size_t file_records_size = 0;
  if (!multiply_fits(header.folder_count, folder_record_size, folder_records_size) ||
      !multiply_fits(header.file_count, file_record_size, file_records_size)) {
    return error{error_code::format_error, "TES4 BSA metadata table is too large"};
  }

  std::size_t total = fixed_header_size;
  if (!add_fits(total, folder_records_size, total) ||
      !add_fits(total, static_cast<std::size_t>(header.total_folder_name_length), total) ||
      !add_fits(total, file_records_size, total) ||
      !add_fits(total, static_cast<std::size_t>(header.total_file_name_length), total)) {
    return error{error_code::format_error, "TES4 BSA metadata table is too large"};
  }
  if (!span_fits(0U, total, archive_size)) {
    return error{error_code::format_error, "TES4 BSA metadata table extends beyond archive bytes"};
  }
  return total;
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

result<std::string> read_bsa_name(detail::binary_reader& reader, std::uint8_t encoded_size,
                                  std::string_view table_name) {
  if (encoded_size == 0U) {
    return error{error_code::unsupported, std::string{"TES4 BSA "} + std::string{table_name} + " is missing"};
  }
  const auto bytes = reader.read_bytes(encoded_size);
  if (!bytes) {
    return error{error_code::format_error, std::string{"TES4 BSA "} + std::string{table_name} + " table is truncated"};
  }
  if (bytes.value().empty() || bytes.value().back() != std::byte{0}) {
    return error{error_code::unsupported, std::string{"TES4 BSA "} + std::string{table_name} + " is not null-terminated"};
  }
  return bytes_to_string(bytes.value().first(bytes.value().size() - 1U));
}

result<void> validate_folder_file_counts(const header_fields& header, std::span<const folder_record> folders) {
  std::size_t file_records_seen = 0;
  const auto expected_records = static_cast<std::size_t>(header.file_count);
  for (const auto& folder : folders) {
    if (folder.file_count > expected_records - file_records_seen) {
      return error{error_code::format_error, "TES4 BSA folder file counts exceed header file count"};
    }
    file_records_seen += folder.file_count;
  }
  if (file_records_seen != expected_records) {
    return error{error_code::format_error, "TES4 BSA folder file counts do not match header file count"};
  }
  return {};
}

result<std::vector<folder_block>> read_folder_blocks(detail::binary_reader& reader, const header_fields& header,
                                                     std::span<const folder_record> folders) {
  std::vector<folder_block> blocks;
  blocks.reserve(folders.size());
  std::size_t file_records_seen = 0;
  std::size_t folder_name_bytes_seen = 0;
  for (const auto& folder : folders) {
    // TES5Edit-compatible folder offsets include the later file-name table length,
    // even though this parser consumes folder blocks sequentially from the stream.
    if (folder.offset != static_cast<std::uint64_t>(reader.position()) + header.total_file_name_length) {
      return error{error_code::format_error, "TES4 BSA folder block offset does not match parsed table layout"};
    }

    const auto name_size = reader.read_u8();
    if (!name_size) {
      return error{error_code::format_error, "TES4 BSA folder name table is truncated"};
    }

    auto folder_name = read_bsa_name(reader, name_size.value(), "folder name");
    if (!folder_name) {
      return folder_name.error();
    }
    folder_name_bytes_seen += 1U + name_size.value();

    std::vector<file_record> file_records;
    file_records.reserve(folder.file_count);
    for (std::uint32_t index = 0; index < folder.file_count; ++index) {
      const auto hash = reader.read_u64_le();
      const auto size_flags = reader.read_u32_le();
      const auto offset = reader.read_u32_le();
      if (!hash || !size_flags || !offset) {
        return error{error_code::format_error, "TES4 BSA file record table is truncated"};
      }
      file_records.push_back(file_record{hash.value(), size_flags.value(), offset.value()});
    }
    file_records_seen += folder.file_count;
    blocks.push_back(folder_block{std::move(folder_name.value()), std::move(file_records)});
  }

  if (folder_name_bytes_seen != header.total_folder_name_length) {
    return error{error_code::format_error, "TES4 BSA folder name lengths do not match header total"};
  }
  if (file_records_seen != header.file_count) {
    return error{error_code::format_error, "TES4 BSA folder file counts do not match header file count"};
  }
  return blocks;
}

result<std::vector<std::string>> read_file_names(detail::binary_reader& reader, std::uint32_t file_count,
                                                 std::uint32_t total_file_name_length) {
  std::vector<std::string> names;
  names.reserve(file_count);
  const auto start = reader.position();
  while (names.size() < file_count) {
    std::string name;
    bool terminated = false;
    while (reader.position() - start < total_file_name_length) {
      const auto ch = reader.read_u8();
      if (!ch) {
        return error{error_code::format_error, "TES4 BSA file name table is truncated"};
      }
      if (ch.value() == 0U) {
        terminated = true;
        break;
      }
      name.push_back(static_cast<char>(ch.value()));
    }
    if (!terminated || name.empty()) {
      return error{error_code::unsupported, "TES4 BSA file name table lacks usable names"};
    }
    names.push_back(std::move(name));
  }
  if (reader.position() - start != total_file_name_length) {
    return error{error_code::format_error, "TES4 BSA file name lengths do not match header total"};
  }
  return names;
}

entry_compression compression_for(const header_fields& header, std::uint32_t size_flags) noexcept {
  const bool default_compressed = (header.archive_flags & archive_compress_by_default) != 0U;
  const bool toggled = (size_flags & file_size_compression_toggle) != 0U;
  if (!(default_compressed ^ toggled)) {
    return entry_compression::none;
  }
  return header.version == sse_version ? entry_compression::lz4_frame : entry_compression::deflate;
}

template <typename PayloadReader>
result<std::uint32_t> embedded_prefix_size(std::size_t archive_size, const file_record& record,
                                           std::uint32_t stored_size, PayloadReader& read_payload_bytes) {
  if (!span_fits(record.offset, 1U, archive_size)) {
    return error{error_code::format_error, "TES4 BSA embedded-name prefix is outside the archive"};
  }
  auto bytes = read_payload_bytes(record.offset, 1U);
  if (!bytes) {
    return bytes.error();
  }
  const auto length = static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[0]));
  const auto prefix_size = length + 1U;
  if (prefix_size > stored_size || !span_fits(record.offset, prefix_size, archive_size)) {
    return error{error_code::format_error, "TES4 BSA embedded-name prefix exceeds stored payload"};
  }
  return prefix_size;
}

template <typename PayloadReader>
result<std::uint32_t> raw_size_for(std::size_t archive_size, const file_record& record, entry_compression compression,
                                   std::uint32_t stored_size, std::uint32_t embedded_prefix,
                                   PayloadReader& read_payload_bytes) {
  if (embedded_prefix > stored_size) {
    return error{error_code::format_error, "TES4 BSA embedded-name prefix exceeds stored payload"};
  }
  const auto cursor = static_cast<std::size_t>(record.offset) + embedded_prefix;
  const auto remaining = stored_size - embedded_prefix;
  if (compression == entry_compression::none) {
    return remaining;
  }
  if (remaining < 4U || !span_fits(cursor, 4U, archive_size)) {
    return error{error_code::format_error, "TES4 BSA compressed payload size prefix is truncated"};
  }
  auto bytes = read_payload_bytes(cursor, 4U);
  if (!bytes) {
    return bytes.error();
  }
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[0])) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[1U])) << 8U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[2U])) << 16U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[3U])) << 24U);
}

template <typename PayloadReader>
result<std::vector<entry_metadata>> materialize_entries(std::size_t archive_size, std::size_t metadata_size,
                                                        const header_fields& header,
                                                        std::span<const folder_block> folders,
                                                        std::span<const std::string> file_names,
                                                        PayloadReader& read_payload_bytes) {
  std::vector<entry_metadata> entries;
  entries.reserve(header.file_count);
  std::unordered_set<std::string> canonical_paths;
  std::size_t name_index = 0;
  const bool has_embedded_names = header.version != 0x67U && (header.archive_flags & archive_embed_names) != 0U;

  for (const auto& folder : folders) {
    auto folder_original = folder.name;
    normalize_original_separators(folder_original);
    for (const auto& record : folder.files) {
      const auto& file_name = file_names[name_index++];
      auto original_path = folder_original + "/" + file_name;
      normalize_original_separators(original_path);
      auto canonical = detail::normalize_archive_path(original_path);
      if (!canonical) {
        return canonical.error();
      }
      if (!canonical_paths.insert(canonical.value().value).second) {
        return error{error_code::format_error, "TES4 BSA contains duplicate canonical archive paths"};
      }

      const auto stored_size = record.size_flags & ~file_size_compression_toggle;
      if (!span_fits(record.offset, stored_size, archive_size)) {
        return error{error_code::format_error, "TES4 BSA entry payload span is outside the archive"};
      }
      // Payload offsets are archive-controlled; non-empty file bytes must not point back into the header or name
      // tables.
      if (non_empty_span_intersects_prefix(record.offset, stored_size, metadata_size)) {
        return error{error_code::format_error, "TES4 BSA entry payload span overlaps metadata"};
      }
      const auto compression = compression_for(header, record.size_flags);
      std::uint32_t prefix_size = 0;
      if (has_embedded_names) {
        auto prefix = embedded_prefix_size(archive_size, record, stored_size, read_payload_bytes);
        if (!prefix) {
          return prefix.error();
        }
        prefix_size = prefix.value();
      }
      auto raw_size = raw_size_for(archive_size, record, compression, stored_size, prefix_size, read_payload_bytes);
      if (!raw_size) {
        return raw_size.error();
      }

      entries.push_back(entry_metadata{canonical.value().value,
                                       std::move(original_path),
                                       raw_size.value(),
                                       stored_size,
                                       record.offset,
                                       detail::hash_tes4(file_name),
                                       compression,
                                       record.size_flags & file_size_compression_toggle,
                                       has_embedded_names,
                                       prefix_size});
    }
  }

  std::sort(entries.begin(), entries.end(), [](const entry_metadata& lhs, const entry_metadata& rhs) {
    return lhs.path < rhs.path;
  });
  return entries;
}

result<std::vector<folder_record>> read_folder_records(detail::binary_reader& reader, const header_fields& header) {
  std::vector<folder_record> records;
  records.reserve(header.folder_count);
  for (std::uint32_t index = 0; index < header.folder_count; ++index) {
    const auto hash = reader.read_u64_le();
    const auto file_count = reader.read_u32_le();
    if (!hash || !file_count) {
      return error{error_code::format_error, "TES4 BSA folder record table is truncated"};
    }

    std::uint64_t offset = 0;
    if (header.version == sse_version) {
      const auto unknown = reader.read_u32_le();
      const auto wide_offset = reader.read_u64_le();
      if (!unknown || !wide_offset) {
        return error{error_code::format_error, "TES4 BSA SSE folder record is truncated"};
      }
      offset = wide_offset.value();
    } else {
      const auto narrow_offset = reader.read_u32_le();
      if (!narrow_offset) {
        return error{error_code::format_error, "TES4 BSA folder record offset is truncated"};
      }
      offset = narrow_offset.value();
    }

    records.push_back(folder_record{file_count.value(), offset});
  }
  return records;
}

result<void> validate_tables(detail::binary_reader& reader, const header_fields& header,
                             std::span<const folder_record> folders, std::size_t archive_size) {
  std::size_t file_records_seen = 0;
  std::size_t folder_name_bytes_seen = 0;
  for (const auto& folder : folders) {
    if (folder.offset > archive_size) {
      return error{error_code::format_error, "TES4 BSA folder block offset is outside the archive"};
    }
    const auto name_size = reader.read_u8();
    if (!name_size) {
      return error{error_code::format_error, "TES4 BSA folder name table is truncated"};
    }
    auto skipped_name = skip_checked(reader, name_size.value());
    if (!skipped_name) {
      return skipped_name.error();
    }
    folder_name_bytes_seen += 1U + name_size.value();

    std::size_t file_record_bytes = 0;
    if (!multiply_fits(folder.file_count, file_record_size, file_record_bytes)) {
      return error{error_code::format_error, "TES4 BSA file record table is too large"};
    }
    auto skipped_records = skip_checked(reader, file_record_bytes);
    if (!skipped_records) {
      return skipped_records.error();
    }
    file_records_seen += folder.file_count;
  }

  if (folder_name_bytes_seen != header.total_folder_name_length) {
    return error{error_code::format_error, "TES4 BSA folder name lengths do not match header total"};
  }

  if (file_records_seen != header.file_count) {
    return error{error_code::format_error, "TES4 BSA folder file counts do not match header file count"};
  }

  const auto file_names_start = reader.position();
  if (!span_fits(file_names_start, header.total_file_name_length, archive_size)) {
    return error{error_code::format_error, "TES4 BSA file name table extends beyond archive bytes"};
  }
  auto skipped_names = skip_checked(reader, header.total_file_name_length);
  if (!skipped_names) {
    return skipped_names.error();
  }

  return {};
}

template <typename PayloadReader>
result<tes4_bsa_archive> parse_tes4_bsa_archive_impl(std::span<const std::byte> table_bytes, std::size_t archive_size,
                                                     detected_bsa_format detected, PayloadReader& read_payload_bytes) {
  if (table_bytes.size() < fixed_header_size) {
    return error{error_code::format_error, "TES4 BSA header is truncated"};
  }

  detail::binary_reader reader{table_bytes};
  auto header = read_header(reader);
  if (!header) {
    return header.error();
  }
  if (header.value().version != detected.version) {
    return error{error_code::format_error, "TES4 BSA detected version does not match parsed header"};
  }
  if (header.value().folder_offset != fixed_header_size) {
    return error{error_code::format_error, "TES4 BSA folder record offset does not match supported table layout"};
  }
  if ((header.value().archive_flags & archive_include_directory_names) == 0U ||
      (header.value().archive_flags & archive_include_file_names) == 0U || header.value().total_folder_name_length == 0U ||
      (header.value().file_count > 0U && header.value().total_file_name_length == 0U)) {
    return error{error_code::unsupported, "TES4 BSA archive does not include usable entry names"};
  }

  const auto folder_record_size = detected.version == sse_version ? sse_folder_record_size : legacy_folder_record_size;
  auto table_size = metadata_table_size(header.value(), folder_record_size, archive_size);
  if (!table_size) {
    return table_size.error();
  }
  std::size_t folder_records_size = 0;
  if (!multiply_fits(header.value().folder_count, folder_record_size, folder_records_size) ||
      !span_fits(fixed_header_size, folder_records_size, table_bytes.size()) || table_bytes.size() < table_size.value()) {
    return error{error_code::format_error, "TES4 BSA folder record span is outside the archive"};
  }

  auto folder_records = read_folder_records(reader, header.value());
  if (!folder_records) {
    return folder_records.error();
  }
  auto folder_counts = validate_folder_file_counts(header.value(), folder_records.value());
  if (!folder_counts) {
    return folder_counts.error();
  }
  detail::binary_reader table_validator{table_bytes};
  auto skipped_header_and_records = table_validator.skip(fixed_header_size + folder_records_size);
  if (!skipped_header_and_records) {
    return skipped_header_and_records.error();
  }
  auto tables = validate_tables(table_validator, header.value(), folder_records.value(), table_bytes.size());
  if (!tables) {
    return tables.error();
  }
  auto folder_blocks = read_folder_blocks(reader, header.value(), folder_records.value());
  if (!folder_blocks) {
    return folder_blocks.error();
  }
  const auto file_names_start = reader.position();
  if (!span_fits(file_names_start, header.value().total_file_name_length, table_bytes.size())) {
    return error{error_code::format_error, "TES4 BSA file name table extends beyond archive bytes"};
  }
  auto file_names = read_file_names(reader, header.value().file_count, header.value().total_file_name_length);
  if (!file_names) {
    return file_names.error();
  }
  auto entries = materialize_entries(archive_size,
                                     table_size.value(),
                                     header.value(),
                                     folder_blocks.value(),
                                     file_names.value(),
                                     read_payload_bytes);
  if (!entries) {
    return entries.error();
  }
  (void)header.value().file_flags;
  return tes4_bsa_archive{archive_metadata{archive_type::bsa,
                                           detected.variant,
                                           header.value().version,
                                           header.value().archive_flags,
                                           header.value().file_count,
                                           detected.default_compression},
                           std::move(entries.value())};
}

} // namespace

result<tes4_bsa_archive> parse_tes4_bsa_archive(std::span<const std::byte> bytes, detected_bsa_format detected) {
  auto read_payload_bytes = [bytes](std::uint64_t offset, std::size_t count) -> result<std::vector<std::byte>> {
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
      return error{error_code::format_error, "TES4 BSA payload offset exceeds platform limits"};
    }
    const auto start = static_cast<std::size_t>(offset);
    if (!span_fits(start, count, bytes.size())) {
      return error{error_code::format_error, "TES4 BSA payload prefix is truncated"};
    }
    const auto payload = bytes.subspan(start, count);
    return std::vector<std::byte>{payload.begin(), payload.end()};
  };
  return parse_tes4_bsa_archive_impl(bytes, bytes.size(), detected, read_payload_bytes);
}

result<tes4_bsa_archive> parse_tes4_bsa_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected) {
  if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, "TES4 BSA archive exceeds platform limits"};
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  auto header_bytes = read_file_bytes_at(input, 0U, fixed_header_size, "TES4 BSA fixed header");
  if (!header_bytes) {
    return header_bytes.error();
  }
  detail::binary_reader header_reader{header_bytes.value()};
  auto header = read_header(header_reader);
  if (!header) {
    return header.error();
  }

  const auto folder_record_size = detected.version == sse_version ? sse_folder_record_size : legacy_folder_record_size;
  auto table_size = metadata_table_size(header.value(), folder_record_size, static_cast<std::size_t>(archive_size));
  if (!table_size) {
    return table_size.error();
  }
  auto table_bytes = read_file_bytes_at(input, 0U, table_size.value(), "TES4 BSA metadata table");
  if (!table_bytes) {
    return table_bytes.error();
  }

  auto read_payload_bytes = [&input](std::uint64_t offset, std::size_t count) -> result<std::vector<std::byte>> {
    return read_file_bytes_at(input, offset, count, "TES4 BSA payload prefix");
  };
  return parse_tes4_bsa_archive_impl(table_bytes.value(), static_cast<std::size_t>(archive_size), detected,
                                     read_payload_bytes);
}

result<archive_metadata> parse_tes4_bsa_metadata(std::span<const std::byte> bytes, detected_bsa_format detected) {
  auto archive = parse_tes4_bsa_archive(bytes, detected);
  if (!archive) {
    return archive.error();
  }
  return archive.value().metadata;
}

} // namespace libbsa::formats::bsa
