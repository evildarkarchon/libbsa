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
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

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

result<void> validate_host_path(std::string_view host_path, std::string_view description) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, std::string{description} + " must not be empty"};
  }
  if (host_path.find('\0') != std::string_view::npos) {
    return error{error_code::invalid_argument, std::string{description} + " must not contain NUL bytes"};
  }
  return {};
}

} // namespace

tes3_bsa_writer::tes3_bsa_writer() : tes3_bsa_writer(tes3_bsa_writer_options{}) {}

tes3_bsa_writer::tes3_bsa_writer(tes3_bsa_writer_options options)
    : state_(std::make_shared<state>(state{options, {}})) {}

const tes3_bsa_writer_options& tes3_bsa_writer::options() const noexcept { return state_->options; }

result<void> tes3_bsa_writer::add_file(std::string_view archive_path, std::string_view host_path) {
  auto validated_host_path = validate_host_path(host_path, "TES3 BSA disk source host path");
  if (!validated_host_path) {
    return validated_host_path.error();
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
  return write_to(host_path, write_execution_options{});
}

result<void> tes3_bsa_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "TES3 BSA writer worker_count must be positive"};
  }
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
constexpr std::size_t payload_stream_chunk_size = 64U * 1024U;

struct prepared_entry {
  std::string archive_path_original;
  std::vector<std::byte> payload;
  std::string host_path;
  std::uint64_t hash{0};
  std::uint32_t raw_offset{0};
  std::uint32_t payload_size{0};
  bool from_memory{false};
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

result<std::uint32_t> disk_payload_size(const std::string& host_path) {
  const auto path = std::filesystem::path{host_path};
  std::error_code fs_error;
  const bool regular_file = std::filesystem::is_regular_file(path, fs_error);
  if (fs_error || !regular_file) {
    return error{error_code::io_error, "TES3 BSA writer failed to inspect disk source"};
  }
  const auto size = std::filesystem::file_size(path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "TES3 BSA writer failed to size disk source"};
  }
  return checked_u32(size, "TES3 BSA disk source size");
}

result<std::vector<prepared_entry>> prepare_entries(std::span<const tes3_writer_entry> entries) {
  std::vector<prepared_entry> prepared;
  prepared.reserve(entries.size());

  for (const auto& entry : entries) {
    prepared_entry prepared_entry;
    prepared_entry.archive_path_original = entry.archive_path_original;
    prepared_entry.hash = detail::hash_tes3(entry.archive_path_original);
    prepared_entry.from_memory = entry.from_memory;
    if (entry.from_memory) {
      auto payload_size = checked_u32(entry.memory_bytes.size(), "TES3 BSA payload size");
      if (!payload_size) {
        return payload_size.error();
      }
      prepared_entry.payload = entry.memory_bytes;
      prepared_entry.payload_size = payload_size.value();
    } else {
      auto payload_size = disk_payload_size(entry.host_path);
      if (!payload_size) {
        return payload_size.error();
      }
      prepared_entry.host_path = entry.host_path;
      prepared_entry.payload_size = payload_size.value();
    }

    // TES3 hashes are computed from the preserved serialized name, not the canonical lowercase lookup key.
    prepared.push_back(std::move(prepared_entry));
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
    auto next = checked_add_u32(cursor, entry.payload_size, "TES3 BSA payload span");
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

result<void> write_span_to_stream(std::ofstream& output,
                                  std::span<const std::byte> bytes,
                                  std::string_view description) {
  while (!bytes.empty()) {
    const auto chunk_size = std::min<std::size_t>(bytes.size(), payload_stream_chunk_size);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(chunk_size));
    if (!output) {
      return error{error_code::io_error, std::string{description} + " failed while writing temporary output"};
    }
    bytes = bytes.subspan(chunk_size);
  }
  return {};
}

result<void> stream_disk_payload_to_output(const std::string& host_path,
                                           std::uint32_t expected_size,
                                           std::ofstream& output) {
  std::ifstream input{host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "TES3 BSA writer failed to open disk source"};
  }

  std::vector<std::byte> scratch(payload_stream_chunk_size);
  std::uint64_t remaining = expected_size;
  while (remaining > 0U) {
    const auto requested = static_cast<std::size_t>(
        std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.size())));
    input.read(reinterpret_cast<char*>(scratch.data()), static_cast<std::streamsize>(requested));
    if (input.gcount() != static_cast<std::streamsize>(requested)) {
      return error{error_code::io_error, "TES3 BSA writer failed while streaming disk source"};
    }
    auto written = write_span_to_stream(output, std::span<const std::byte>{scratch.data(), requested},
                                        "TES3 BSA writer");
    if (!written) {
      return written.error();
    }
    remaining -= requested;
  }

  // The metadata was sized before reserving a publish path. Fail if a caller mutates the source during finalization.
  char extra = '\0';
  if (input.get(extra)) {
    return error{error_code::io_error, "TES3 BSA disk source changed during finalization"};
  }
  if (input.bad()) {
    return error{error_code::io_error, "TES3 BSA writer failed while reading disk source"};
  }
  return {};
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
    if (!(written = writer.write_u32_le(entry.payload_size)) || !(written = writer.write_u32_le(entry.raw_offset))) {
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

  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "TES3 BSA writer failed to create temporary output"};
  }
  auto metadata_written = write_span_to_stream(output, writer.bytes(), "TES3 BSA writer metadata");
  if (!metadata_written) {
    return metadata_written.error();
  }
  for (const auto& entry : entries) {
    if (entry.from_memory) {
      auto payload_written = write_span_to_stream(output, entry.payload, "TES3 BSA writer memory payload");
      if (!payload_written) {
        return payload_written.error();
      }
      continue;
    }

    auto payload_written = stream_disk_payload_to_output(entry.host_path, entry.payload_size, output);
    if (!payload_written) {
      return payload_written.error();
    }
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

} // namespace

result<void> write_tes3_bsa_archive(const tes3_bsa_writer_options& options,
                                    std::span<const tes3_writer_entry> entries,
                                    std::string_view output_host_path) {
  auto validated_host_path = validate_host_path(output_host_path, "TES3 BSA output host path");
  if (!validated_host_path) {
    return validated_host_path.error();
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

  // D-16 keeps disk-backed source bytes path-backed until this point, but sizes
  // and source readability are validated before any publish path is reserved.
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

      auto published = detail::replace_file_atomically(temp_path, output_path);
      if (!published) {
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "TES3 BSA writer failed to publish output host path"};
      }

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
