#include "formats/bsa/tes4_bsa_parser.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"
#include "formats/bsa/tes4_bsa_payload_descriptor.hpp"
#include "formats/bsa/tes4_bsa_table.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/byte_vector.hpp>
#include <detail/host_file.hpp>
#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

using detail::normalize_display_separators;
using detail::read_file_bytes_at;
using detail::span_fits;

template <typename PayloadReader>
result<std::vector<entry_metadata>> materialize_entries(std::size_t archive_size,
                                                        const tes4_bsa_raw_table& table,
                                                        PayloadReader& read_payload_bytes) {
  try {
    std::vector<entry_metadata> entries;
    auto reserved_entries = detail::reserve_metadata_vector(entries, table.header.file_count, "TES4 BSA entry metadata");
    if (!reserved_entries) {
      return reserved_entries.error();
    }
    std::unordered_set<std::string> canonical_paths;
    auto reserved_paths = detail::reserve_metadata_set(canonical_paths, table.header.file_count, "TES4 BSA canonical path set");
    if (!reserved_paths) {
      return reserved_paths.error();
    }
    std::size_t name_index = 0;

    for (const auto& folder : table.folder_blocks) {
      auto folder_original = folder.name;
      normalize_display_separators(folder_original);
      for (const auto& record : folder.files) {
        const auto& file_name = table.file_names[name_index++];
        auto original_path = folder_original + "/" + file_name;
        normalize_display_separators(original_path);
        auto canonical = detail::normalize_archive_path(original_path);
        if (!canonical) {
          return canonical.error();
        }
        if (!canonical_paths.insert(canonical.value().value).second) {
          return error{error_code::format_error, "TES4 BSA contains duplicate canonical archive paths"};
        }
        const auto file_hash = detail::hash_tes4(file_name);
        // TES4 lookup tables are hash-driven; accepting a mismatched record would
        // publish an entry that game-style lookup cannot resolve from its name.
        if (record.hash != file_hash) {
          return error{error_code::format_error, "TES4 BSA file record hash does not match filename table"};
        }

        auto payload = make_tes4_bsa_payload_descriptor(table.header,
                                                        record,
                                                        archive_size,
                                                        table.metadata_table_size,
                                                        read_payload_bytes);
        if (!payload) {
          return payload.error();
        }

        entries.push_back(entry_metadata{canonical.value().value,
                                         std::move(original_path),
                                         payload.value().raw_size,
                                         payload.value().stored_size,
                                         payload.value().payload_offset,
                                         record.hash,
                                         payload.value().compression,
                                         record.size_flags & tes4_bsa_file_size_compression_toggle,
                                         tes4_bsa_has_embedded_names(table.header),
                                         payload.value().embedded_prefix_size});
      }
    }

    std::sort(entries.begin(), entries.end(), [](const entry_metadata& lhs, const entry_metadata& rhs) {
      return lhs.path < rhs.path;
    });
    return entries;
  } catch (const std::bad_alloc&) {
    return detail::metadata_allocation_error("TES4 BSA entry metadata");
  } catch (const std::length_error&) {
    return detail::metadata_allocation_error("TES4 BSA entry metadata");
  }
}

template <typename PayloadReader>
result<tes4_bsa_archive> parse_tes4_bsa_archive_impl(std::span<const std::byte> table_bytes,
                                                     std::size_t archive_size,
                                                     detected_bsa_format detected,
                                                     PayloadReader& read_payload_bytes) {
  auto table = read_tes4_bsa_raw_table(table_bytes, archive_size, detected);
  if (!table) {
    return table.error();
  }
  auto entries = materialize_entries(archive_size, table.value(), read_payload_bytes);
  if (!entries) {
    return entries.error();
  }
  (void)table.value().header.file_flags;
  return tes4_bsa_archive{archive_metadata{archive_type::bsa,
                                           detected.variant,
                                           table.value().header.version,
                                           table.value().header.archive_flags,
                                           table.value().header.file_count,
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
    auto copied = detail::make_byte_vector(payload.size(), "TES4 BSA payload prefix");
    if (!copied) {
      return copied.error();
    }
    std::copy(payload.begin(), payload.end(), copied.value().begin());
    return std::move(copied).value();
  };
  return parse_tes4_bsa_archive_impl(bytes, bytes.size(), detected, read_payload_bytes);
}

result<tes4_bsa_archive> parse_tes4_bsa_archive_file(const detail::host_file_path& host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected) {
  if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, "TES4 BSA archive exceeds platform limits"};
  }

  const detail::host_file_context host_context{"failed to open archive host path",
                                                "failed to determine archive host path size",
                                                "failed while reading archive host path",
                                                "archive host path changed while reading",
                                                "TES4 BSA metadata table"};
  auto input = detail::open_host_file(host_path, host_context);
  if (!input) {
    return input.error();
  }
  auto header_bytes = read_file_bytes_at(input.value(), 0U, tes4_bsa_header_size, "TES4 BSA fixed header");
  if (!header_bytes) {
    return header_bytes.error();
  }

  auto header_table = read_tes4_bsa_raw_table(header_bytes.value(), static_cast<std::size_t>(archive_size), detected);
  std::size_t table_size = 0;
  if (header_table) {
    table_size = header_table.value().metadata_table_size;
  } else if (header_table.error().code != error_code::format_error || header_bytes.value().size() < tes4_bsa_header_size) {
    return header_table.error();
  } else {
    detail::binary_reader header_reader{header_bytes.value()};
    const auto magic = header_reader.read_u32_le();
    const auto version = header_reader.read_u32_le();
    const auto folder_offset = header_reader.read_u32_le();
    const auto archive_flags = header_reader.read_u32_le();
    const auto folder_count = header_reader.read_u32_le();
    const auto file_count = header_reader.read_u32_le();
    const auto total_folder_name_length = header_reader.read_u32_le();
    const auto total_file_name_length = header_reader.read_u32_le();
    const auto file_flags = header_reader.read_u32_le();
    if (!magic || !version || !folder_offset || !archive_flags || !folder_count || !file_count || !total_folder_name_length ||
        !total_file_name_length || !file_flags || magic.value() != tes4_bsa_magic) {
      return header_table.error();
    }
    const tes4_bsa_header_fields header{version.value(),
                                        folder_offset.value(),
                                        archive_flags.value(),
                                        folder_count.value(),
                                        file_count.value(),
                                        total_folder_name_length.value(),
                                        total_file_name_length.value(),
                                        file_flags.value()};
    auto folder_count_limit = detail::validate_metadata_count(header.folder_count,
                                                             detail::metadata_bsa_folder_count_limit,
                                                             "TES4 BSA folder count");
    if (!folder_count_limit) {
      return folder_count_limit.error();
    }
    auto file_count_limit = detail::validate_metadata_count(header.file_count,
                                                           detail::metadata_entry_count_limit,
                                                           "TES4 BSA file count");
    if (!file_count_limit) {
      return file_count_limit.error();
    }
    const auto folder_record_size = detected.version == tes4_bsa_skyrim_se_version ? tes4_bsa_sse_folder_record_size
                                                                                   : tes4_bsa_legacy_folder_record_size;
    auto computed_table_size = tes4_bsa_metadata_table_size(header, folder_record_size, static_cast<std::size_t>(archive_size));
    if (!computed_table_size) {
      return computed_table_size.error();
    }
    table_size = computed_table_size.value();
  }

  auto table_bytes = read_file_bytes_at(input.value(), 0U, table_size, "TES4 BSA metadata table");
  if (!table_bytes) {
    return table_bytes.error();
  }

  auto read_payload_bytes = [&input](std::uint64_t offset, std::size_t count) -> result<std::vector<std::byte>> {
    return read_file_bytes_at(input.value(), offset, count, "TES4 BSA payload prefix");
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
