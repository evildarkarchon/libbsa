#include "formats/bsa/tes4_bsa_layout.hpp"

#include <detail/writer_disk_source.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

namespace {

constexpr std::uint32_t skyrim_se_version = 0x69U;
constexpr std::uint32_t header_size = 36U;

struct payload_assignment {
  std::uint32_t offset{0};
  std::uint32_t stored_size{0};
};

struct assigned_payload {
  const tes4_prepared_entry* entry{nullptr};
  payload_assignment assignment;
};

bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
  if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
    return false;
  }
  total = lhs + rhs;
  return true;
}

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds uint32_t limits"};
  }
  return static_cast<std::uint32_t>(value);
}

result<std::uint8_t> checked_name_size(std::size_t size, std::string_view description) {
  if (size > static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())) {
    return error{error_code::format_error, std::string{description} + " exceeds BSA name length limits"};
  }
  return static_cast<std::uint8_t>(size);
}

constexpr detail::writer_disk_source_context tes4_dedupe_source_context{
    "TES4 BSA writer failed to open disk source",
    "TES4 BSA writer failed to inspect disk source",
    "TES4 BSA writer failed while reading disk source",
    "TES4 BSA disk source changed during dedupe preparation",
    "TES4 BSA disk source"};

result<tes4_layout_result> calculate_table_lengths(std::span<const tes4_prepared_folder> folders) {
  std::uint64_t total_folder_name_length64 = 0;
  std::uint64_t total_file_name_length64 = 0;
  std::uint64_t file_count64 = 0;
  for (const auto& folder : folders) {
    std::uint64_t folder_name_length = 0;
    if (!add_fits_u64(static_cast<std::uint64_t>(folder.name.size()), 2U, folder_name_length) ||
        !add_fits_u64(total_folder_name_length64, folder_name_length, total_folder_name_length64) ||
        !add_fits_u64(file_count64, folder.entries.size(), file_count64)) {
      return error{error_code::format_error, "TES4 BSA table size overflows"};
    }
    auto folder_name_size = checked_name_size(folder.name.size() + 1U, "TES4 BSA folder name");
    if (!folder_name_size) {
      return folder_name_size.error();
    }
    for (const auto& entry : folder.entries) {
      auto file_name_size = checked_u32(static_cast<std::uint64_t>(entry.file_name.size()) + 1U,
                                        "TES4 BSA file name length");
      if (!file_name_size) {
        return file_name_size.error();
      }
      if (!add_fits_u64(total_file_name_length64, file_name_size.value(), total_file_name_length64)) {
        return error{error_code::format_error, "TES4 BSA file-name table size overflows"};
      }
    }
  }

  auto total_folder_name_length = checked_u32(total_folder_name_length64, "TES4 BSA folder-name table size");
  auto total_file_name_length = checked_u32(total_file_name_length64, "TES4 BSA file-name table size");
  auto file_count = checked_u32(file_count64, "TES4 BSA file count");
  if (!total_folder_name_length || !total_file_name_length || !file_count) {
    return !total_folder_name_length ? total_folder_name_length.error()
                                     : (!total_file_name_length ? total_file_name_length.error() : file_count.error());
  }

  return tes4_layout_result{total_folder_name_length.value(), total_file_name_length.value(), file_count.value()};
}

result<bool> disk_payload_equals_bytes(const tes4_prepared_entry& entry, std::span<const std::byte> expected) {
  if (expected.size() != entry.raw_disk_size) {
    return false;
  }

  bool equal = true;
  std::size_t offset = 0;
  auto compared = detail::for_each_disk_source_chunk(
      entry.raw_disk_host_path,
      entry.raw_disk_size,
      tes4_dedupe_source_context,
      [&](std::span<const std::byte> chunk) -> result<void> {
        if (equal && !std::equal(chunk.begin(), chunk.end(), expected.begin() + static_cast<std::ptrdiff_t>(offset))) {
          equal = false;
        }
        offset += chunk.size();
        return {};
      });
  if (!compared) {
    return compared.error();
  }
  return equal;
}

result<bool> disk_stored_payload_equals_bytes(const tes4_prepared_entry& entry,
                                              std::span<const std::byte> expected) {
  if (expected.size() != static_cast<std::size_t>(entry.stored_payload.size()) + entry.raw_disk_size) {
    return false;
  }
  if (!std::equal(entry.stored_payload.begin(), entry.stored_payload.end(), expected.begin())) {
    return false;
  }
  return disk_payload_equals_bytes(entry, expected.subspan(entry.stored_payload.size()));
}

result<bool> disk_stored_payloads_equal(const tes4_prepared_entry& lhs, const tes4_prepared_entry& rhs) {
  if (lhs.stored_payload != rhs.stored_payload || lhs.raw_disk_size != rhs.raw_disk_size) {
    return false;
  }

  auto rhs_payload = detail::read_disk_source_exact(rhs.raw_disk_host_path,
                                                   rhs.raw_disk_size,
                                                   tes4_dedupe_source_context);
  if (!rhs_payload) {
    return rhs_payload.error();
  }
  return disk_payload_equals_bytes(lhs, rhs_payload.value());
}

} // namespace

result<bool> tes4_stored_payloads_equal(const tes4_prepared_entry& lhs, const tes4_prepared_entry& rhs) {
  if (lhs.stored_size != rhs.stored_size) {
    return false;
  }
  if (!lhs.stream_raw_disk && !rhs.stream_raw_disk) {
    return lhs.stored_payload == rhs.stored_payload;
  }
  if (lhs.stream_raw_disk && rhs.stream_raw_disk) {
    return disk_stored_payloads_equal(lhs, rhs);
  }
  if (lhs.stream_raw_disk) {
    return disk_stored_payload_equals_bytes(lhs, rhs.stored_payload);
  }
  return disk_stored_payload_equals_bytes(rhs, lhs.stored_payload);
}

result<tes4_layout_result> tes4_assign_offsets(std::span<tes4_prepared_folder> folders,
                                               std::uint32_t version,
                                               bool deduplicate_payloads) {
  auto layout = calculate_table_lengths(folders);
  if (!layout) {
    return layout.error();
  }

  const std::uint64_t folder_record_size = version == skyrim_se_version ? 24U : 16U;
  const std::uint64_t folder_records_size = folder_record_size * folders.size();
  std::uint64_t folder_block_cursor = header_size + folder_records_size;
  std::uint64_t folder_blocks_size = 0;

  for (auto& folder : folders) {
    std::uint64_t folder_name_size = 0;
    if (!add_fits_u64(static_cast<std::uint64_t>(folder.name.size()), 2U, folder_name_size)) {
      return error{error_code::format_error, "TES4 BSA folder name size overflows"};
    }
    std::uint64_t file_record_bytes = 0;
    if (folder.entries.size() > std::numeric_limits<std::uint64_t>::max() / 16U) {
      return error{error_code::format_error, "TES4 BSA file record table size overflows"};
    }
    file_record_bytes = static_cast<std::uint64_t>(folder.entries.size()) * 16U;

    // Reference-compatible folder offsets include the later file-name table length,
    // even though the folder block bytes are serialized before that table.
    if (!add_fits_u64(folder_block_cursor, layout.value().total_file_name_length, folder.folder_block_offset)) {
      return error{error_code::format_error, "TES4 BSA folder offset overflows"};
    }
    std::uint64_t folder_block_size = 0;
    if (!add_fits_u64(folder_name_size, file_record_bytes, folder_block_size) ||
        !add_fits_u64(folder_blocks_size, folder_block_size, folder_blocks_size) ||
        !add_fits_u64(folder_block_cursor, folder_block_size, folder_block_cursor)) {
      return error{error_code::format_error, "TES4 BSA folder block size overflows"};
    }
  }

  std::uint64_t payload_cursor = 0;
  if (!add_fits_u64(header_size, folder_records_size, payload_cursor) ||
      !add_fits_u64(payload_cursor, folder_blocks_size, payload_cursor) ||
      !add_fits_u64(payload_cursor, layout.value().total_file_name_length, payload_cursor)) {
    return error{error_code::format_error, "TES4 BSA metadata size overflows"};
  }

  std::vector<assigned_payload> deduplicated_payloads;
  for (auto& folder : folders) {
    for (auto& entry : folder.entries) {
      if (deduplicate_payloads) {
        // Dedupe compares the complete final stored byte stream. Raw disk sources
        // are compared on demand so the publish path can still stream unique files.
        for (const auto& candidate : deduplicated_payloads) {
          auto duplicate = tes4_stored_payloads_equal(entry, *candidate.entry);
          if (!duplicate) {
            return duplicate.error();
          }
          if (duplicate.value()) {
            entry.payload_offset = candidate.assignment.offset;
            entry.stored_size = candidate.assignment.stored_size;
            entry.owns_payload_bytes = false;
            break;
          }
        }
        if (!entry.owns_payload_bytes) {
          continue;
        }
      }

      auto offset = checked_u32(payload_cursor, "TES4 BSA payload offset");
      if (!offset) {
        return offset.error();
      }
      entry.payload_offset = offset.value();
      entry.owns_payload_bytes = true;
      if (deduplicate_payloads) {
        deduplicated_payloads.push_back(assigned_payload{&entry, payload_assignment{entry.payload_offset, entry.stored_size}});
      }
      if (!add_fits_u64(payload_cursor, entry.stored_size, payload_cursor)) {
        return error{error_code::format_error, "TES4 BSA payload span overflows"};
      }
    }
  }
  return layout.value();
}

} // namespace libbsa::formats::bsa
