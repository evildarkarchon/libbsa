#include "formats/bsa/tes3_bsa_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/atomic_file_ops.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace libbsa {

struct tes3_bsa_writer::state {
  tes3_bsa_writer_options options;
  std::vector<formats::bsa::tes3_writer_entry> entries;
};

namespace {

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  // TES3 serializes a flat name table, but libbsa still normalizes separators so
  // callers get stable archive keys without losing the caller's path casing.
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<formats::bsa::tes3_writer_entry> make_entry(std::string_view archive_path) {
  if (archive_path.find('\0') != std::string_view::npos) {
    return error{error_code::invalid_argument, "TES3 BSA archive path must not contain NUL bytes"};
  }

  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::bsa::tes3_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  return entry;
}

} // namespace

tes3_bsa_writer::tes3_bsa_writer() : tes3_bsa_writer(tes3_bsa_writer_options{}) {}

tes3_bsa_writer::tes3_bsa_writer(tes3_bsa_writer_options options)
    : state_(std::make_shared<state>(state{options, {}})) {}

const tes3_bsa_writer_options& tes3_bsa_writer::options() const noexcept { return state_->options; }

result<void> tes3_bsa_writer::add_file(std::string_view archive_path, std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "TES3 BSA disk source host path must not be empty"};
  }

  auto entry = make_entry(archive_path);
  if (!entry) {
    return entry.error();
  }

  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes3_bsa_writer::add_bytes(std::string_view archive_path, std::span<const std::byte> bytes) {
  auto entry = make_entry(archive_path);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes3_bsa_writer::write_to(std::string_view host_path) const {
  return formats::bsa::write_tes3_bsa_archive(state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::bsa {

namespace {

constexpr std::uint32_t tes3_magic_version = 0x00000100U;
constexpr std::uint32_t fixed_header_size = 12U;
constexpr std::uint32_t file_record_size = 8U;
constexpr std::uint32_t name_offset_size = 4U;
constexpr std::uint32_t hash_record_size = 8U;

struct prepared_entry {
  std::string archive_path_original;
  std::vector<std::byte> payload;
  std::uint64_t hash{0};
  std::uint32_t raw_offset{0};
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

result<std::uint32_t> checked_add_u32(std::uint32_t lhs, std::uint32_t rhs, std::string_view description) {
  return checked_u32(static_cast<std::uint64_t>(lhs) + rhs, description);
}

result<std::uint32_t> checked_mul_u32(std::uint32_t lhs, std::uint32_t rhs, std::string_view description) {
  return checked_u32(static_cast<std::uint64_t>(lhs) * rhs, description);
}

result<void> validate_entries(std::span<const tes3_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "TES3 BSA writer requires at least one file entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "TES3 BSA writer has duplicate canonical archive paths"};
    }
  }

  return {};
}

result<std::vector<std::byte>> read_source_bytes(const tes3_writer_entry& entry) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }

  // Disk sources stay path-backed until finalization so repeated write_to calls
  // can observe the current source bytes without making add_file consume I/O.
  std::ifstream input{entry.host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "TES3 BSA writer failed to open disk source"};
  }
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "TES3 BSA writer failed while reading disk source"};
  }
  return bytes;
}

result<std::vector<prepared_entry>> prepare_entries(std::span<const tes3_writer_entry> entries) {
  std::vector<prepared_entry> prepared;
  prepared.reserve(entries.size());

  for (const auto& entry : entries) {
    auto payload = read_source_bytes(entry);
    if (!payload) {
      return payload.error();
    }

    auto payload_size = checked_u32(payload.value().size(), "TES3 BSA payload size");
    if (!payload_size) {
      return payload_size.error();
    }
    (void)payload_size;

    // TES3 hashes are computed from the preserved serialized name, not the canonical lowercase lookup key.
    prepared.push_back(prepared_entry{entry.archive_path_original,
                                      std::move(payload.value()),
                                      detail::hash_tes3(entry.archive_path_original),
                                      0U});
  }

  std::sort(prepared.begin(), prepared.end(), [](const prepared_entry& lhs, const prepared_entry& rhs) {
    // TES3 table order compares hash low32 first and high32 second; the helper packs that order for sorting.
    return detail::tes3_hash_sort_key(lhs.hash) < detail::tes3_hash_sort_key(rhs.hash);
  });
  return prepared;
}

result<void> assign_raw_offsets(std::span<prepared_entry> entries) {
  std::uint32_t cursor = 0;
  for (auto& entry : entries) {
    // On disk TES3 stores data-section-relative raw offsets; readers add the computed data section start back.
    entry.raw_offset = cursor;
    auto payload_size = checked_u32(entry.payload.size(), "TES3 BSA payload size");
    if (!payload_size) {
      return payload_size.error();
    }
    auto next = checked_add_u32(cursor, payload_size.value(), "TES3 BSA payload span");
    if (!next) {
      return next.error();
    }
    cursor = next.value();
  }
  return {};
}

result<void> write_string_terminated(detail::binary_writer& writer, std::string_view value) {
  for (const char ch : value) {
    auto written = writer.write_u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch)));
    if (!written) {
      return written.error();
    }
  }
  return writer.write_u8(0U);
}

result<void> write_archive_bytes(std::span<const prepared_entry> entries, const std::filesystem::path& output_path) {
  std::uint64_t name_table_size64 = 0;
  for (const auto& entry : entries) {
    if (!add_fits_u64(name_table_size64, static_cast<std::uint64_t>(entry.archive_path_original.size()) + 1U,
                      name_table_size64)) {
      return error{error_code::format_error, "TES3 BSA name table size overflows"};
    }
  }

  const auto file_count = checked_u32(entries.size(), "TES3 BSA file count");
  const auto name_table_size = checked_u32(name_table_size64, "TES3 BSA name table size");
  if (!file_count || !name_table_size) {
    return !file_count ? file_count.error() : name_table_size.error();
  }

  const auto records_size = checked_mul_u32(file_count.value(), file_record_size, "TES3 BSA file records");
  const auto name_offsets_size = checked_mul_u32(file_count.value(), name_offset_size, "TES3 BSA name offsets");
  const auto hash_records_size = checked_mul_u32(file_count.value(), hash_record_size, "TES3 BSA hash records");
  if (!records_size || !name_offsets_size || !hash_records_size) {
    return !records_size ? records_size.error() : (!name_offsets_size ? name_offsets_size.error() : hash_records_size.error());
  }

  auto hash_table_start = checked_add_u32(fixed_header_size, records_size.value(), "TES3 BSA hash table offset");
  if (hash_table_start) {
    hash_table_start = checked_add_u32(hash_table_start.value(), name_offsets_size.value(), "TES3 BSA hash table offset");
  }
  if (hash_table_start) {
    hash_table_start = checked_add_u32(hash_table_start.value(), name_table_size.value(), "TES3 BSA hash table offset");
  }
  if (!hash_table_start) {
    return hash_table_start.error();
  }
  auto data_section_start = checked_add_u32(hash_table_start.value(), hash_records_size.value(), "TES3 BSA data section offset");
  if (!data_section_start) {
    return data_section_start.error();
  }
  const auto hash_offset_minus_header = checked_u32(hash_table_start.value() - fixed_header_size,
                                                    "TES3 BSA hash table relative offset");
  if (!hash_offset_minus_header) {
    return hash_offset_minus_header.error();
  }
  (void)data_section_start;

  detail::binary_writer writer;
  auto written = writer.write_u32_le(tes3_magic_version);
  if (!written) {
    return written.error();
  }
  if (!(written = writer.write_u32_le(hash_offset_minus_header.value())) ||
      !(written = writer.write_u32_le(file_count.value()))) {
    return written.error();
  }

  for (const auto& entry : entries) {
    const auto payload_size = checked_u32(entry.payload.size(), "TES3 BSA file size");
    if (!payload_size) {
      return payload_size.error();
    }
    if (!(written = writer.write_u32_le(payload_size.value())) || !(written = writer.write_u32_le(entry.raw_offset))) {
      return written.error();
    }
  }

  std::uint32_t name_offset = 0;
  for (const auto& entry : entries) {
    if (!(written = writer.write_u32_le(name_offset))) {
      return written.error();
    }
    const auto next_name_offset = checked_u32(static_cast<std::uint64_t>(name_offset) + entry.archive_path_original.size() + 1U,
                                             "TES3 BSA name offset");
    if (!next_name_offset) {
      return next_name_offset.error();
    }
    name_offset = next_name_offset.value();
  }

  for (const auto& entry : entries) {
    if (!(written = write_string_terminated(writer, entry.archive_path_original))) {
      return written.error();
    }
  }
  for (const auto& entry : entries) {
    if (!(written = writer.write_u64_le(entry.hash))) {
      return written.error();
    }
  }
  for (const auto& entry : entries) {
    if (!(written = writer.write_bytes(entry.payload))) {
      return written.error();
    }
  }

  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "TES3 BSA writer failed to create temporary output"};
  }
  const auto bytes = writer.bytes();
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    return error{error_code::io_error, "TES3 BSA writer failed while writing temporary output"};
  }
  return {};
}

result<bool> path_exists_noexcept(const std::filesystem::path& path) {
  std::error_code fs_error;
  const bool exists = std::filesystem::exists(path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "TES3 BSA writer failed to inspect output host path"};
  }
  return exists;
}

result<std::filesystem::path> make_unique_publish_directory(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-tmp-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    std::error_code fs_error;
    // A unique directory avoids deleting caller-owned deterministic siblings such as `<archive>.tmp`.
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate;
    }
    if (fs_error) {
      return error{error_code::io_error, "TES3 BSA writer failed to reserve temporary output directory"};
    }
  }
  return error{error_code::io_error, "TES3 BSA writer exhausted temporary output directory names"};
}

void cleanup_publish_directory(const std::filesystem::path& temp_dir) noexcept {
  std::error_code fs_error;
  // Cleanup is best-effort because callers should receive the primary write/publish failure, not cleanup noise.
  std::filesystem::remove_all(temp_dir, fs_error);
}

result<std::filesystem::path> reserve_backup_path(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-bak-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    auto exists = path_exists_noexcept(candidate);
    if (!exists) {
      return exists.error();
    }
    if (!exists.value()) {
      return candidate;
    }
  }
  return error{error_code::io_error, "TES3 BSA writer exhausted backup output path names"};
}

} // namespace

result<void> write_tes3_bsa_archive(const tes3_bsa_writer_options& options,
                                    std::span<const tes3_writer_entry> entries,
                                    std::string_view output_host_path) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "TES3 BSA output host path must not be empty"};
  }

  const auto output_path = std::filesystem::path{output_host_path};
  auto output_exists = path_exists_noexcept(output_path);
  if (!output_exists) {
    return output_exists.error();
  }
  if (!options.overwrite_existing && output_exists.value()) {
    return error{error_code::io_error, "TES3 BSA output host path already exists"};
  }

  auto validated = validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  auto prepared = prepare_entries(entries);
  if (!prepared) {
    return prepared.error();
  }
  auto offsets = assign_raw_offsets(prepared.value());
  if (!offsets) {
    return offsets.error();
  }

  // D-08 requires all caller-controlled sources to be validated and loaded before
  // any publish path is reserved, so missing disk files cannot leave partial output.
  auto temp_dir = make_unique_publish_directory(output_path);
  if (!temp_dir) {
    return temp_dir.error();
  }
  const auto temp_path = temp_dir.value() / output_path.filename();

  auto written = write_archive_bytes(prepared.value(), temp_path);
  if (!written) {
    cleanup_publish_directory(temp_dir.value());
    return written.error();
  }

  std::error_code fs_error;
  if (options.overwrite_existing) {
    output_exists = path_exists_noexcept(output_path);
    if (!output_exists) {
      cleanup_publish_directory(temp_dir.value());
      return output_exists.error();
    }
    if (output_exists.value()) {
      const bool is_regular = std::filesystem::is_regular_file(output_path, fs_error);
      if (fs_error || !is_regular) {
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "TES3 BSA writer refuses to replace non-regular output host path"};
      }

      auto backup_path = reserve_backup_path(output_path);
      if (!backup_path) {
        cleanup_publish_directory(temp_dir.value());
        return backup_path.error();
      }

      // Move the old archive aside before publish so a failed replacement can roll back to the last good file.
      std::filesystem::rename(output_path, backup_path.value(), fs_error);
      if (fs_error) {
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "TES3 BSA writer failed to reserve output backup"};
      }

      std::filesystem::rename(temp_path, output_path, fs_error);
      if (fs_error) {
        std::error_code rollback_error;
        std::filesystem::rename(backup_path.value(), output_path, rollback_error);
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "TES3 BSA writer failed to publish output host path"};
      }

      std::filesystem::remove(backup_path.value(), fs_error);
      cleanup_publish_directory(temp_dir.value());
      return {};
    }
  }

  output_exists = path_exists_noexcept(output_path);
  if (!output_exists) {
    cleanup_publish_directory(temp_dir.value());
    return output_exists.error();
  }
  if (output_exists.value()) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "TES3 BSA output host path already exists"};
  }

  auto published = detail::publish_file_without_replace(temp_path, output_path);
  if (!published) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "TES3 BSA writer failed to publish output host path"};
  }
  cleanup_publish_directory(temp_dir.value());
  return {};
}

} // namespace libbsa::formats::bsa
