#include "formats/bsa/tes4_bsa_parser.hpp"

#include <detail/binary_io.hpp>

#include <limits>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

constexpr std::uint32_t sse_version = 0x69U;
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

bool span_fits(std::size_t start, std::size_t length, std::size_t total) noexcept {
  return start <= total && length <= total - start;
}

result<header_fields> read_header(detail::binary_reader& reader) {
  const auto magic = reader.read_bytes(4);
  if (!magic) {
    return magic.error();
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

} // namespace

result<archive_metadata> parse_tes4_bsa_metadata(std::span<const std::byte> bytes, detected_bsa_format detected) {
  if (bytes.size() < fixed_header_size) {
    return error{error_code::format_error, "TES4 BSA header is truncated"};
  }

  detail::binary_reader reader{bytes};
  auto header = read_header(reader);
  if (!header) {
    return header.error();
  }
  if (header.value().version != detected.version) {
    return error{error_code::format_error, "TES4 BSA detected version does not match parsed header"};
  }

  const auto folder_record_size = detected.version == sse_version ? sse_folder_record_size : legacy_folder_record_size;
  std::size_t folder_records_size = 0;
  if (!multiply_fits(header.value().folder_count, folder_record_size, folder_records_size) ||
      !span_fits(fixed_header_size, folder_records_size, bytes.size())) {
    return error{error_code::format_error, "TES4 BSA folder record span is outside the archive"};
  }

  auto folders = read_folder_records(reader, header.value());
  if (!folders) {
    return folders.error();
  }
  auto tables = validate_tables(reader, header.value(), folders.value(), bytes.size());
  if (!tables) {
    return tables.error();
  }

  (void)header.value().folder_offset;
  (void)header.value().total_folder_name_length;
  (void)header.value().file_flags;
  return archive_metadata{archive_type::bsa,
                          detected.variant,
                          header.value().version,
                          header.value().archive_flags,
                          header.value().file_count,
                          detected.default_compression};
}

} // namespace libbsa::formats::bsa
