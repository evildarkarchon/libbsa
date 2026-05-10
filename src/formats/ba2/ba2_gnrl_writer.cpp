#include "formats/ba2/ba2_gnrl_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libbsa {

struct ba2_gnrl_writer::state {
  ba2_gnrl_target target;
  ba2_gnrl_writer_options options;
  std::vector<formats::ba2::ba2_gnrl_writer_entry> entries;
};

namespace {

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<formats::ba2::ba2_gnrl_writer_entry> make_entry(std::string_view archive_path,
                                                       ba2_gnrl_entry_options options) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::ba2::ba2_gnrl_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.options = options;
  return entry;
}

} // namespace

ba2_gnrl_writer::ba2_gnrl_writer(ba2_gnrl_target target)
    : ba2_gnrl_writer(target, ba2_gnrl_writer_options{}) {}

ba2_gnrl_writer::ba2_gnrl_writer(ba2_gnrl_target target, ba2_gnrl_writer_options options)
    : state_(std::make_shared<state>(state{target, options, {}})) {}

ba2_gnrl_target ba2_gnrl_writer::target() const noexcept { return state_->target; }

const ba2_gnrl_writer_options& ba2_gnrl_writer::options() const noexcept { return state_->options; }

result<void> ba2_gnrl_writer::add_file(std::string_view archive_path,
                                       std::string_view host_path,
                                       entry_compression_policy compression) {
  ba2_gnrl_entry_options entry_options;
  entry_options.compression = compression;
  return add_file(archive_path, host_path, entry_options);
}

result<void> ba2_gnrl_writer::add_file(std::string_view archive_path,
                                       std::string_view host_path,
                                       ba2_gnrl_entry_options options) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL disk source host path must not be empty"};
  }

  auto entry = make_entry(archive_path, options);
  if (!entry) {
    return entry.error();
  }

  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> ba2_gnrl_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        entry_compression_policy compression) {
  ba2_gnrl_entry_options entry_options;
  entry_options.compression = compression;
  return add_bytes(archive_path, bytes, entry_options);
}

result<void> ba2_gnrl_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        ba2_gnrl_entry_options options) {
  auto entry = make_entry(archive_path, options);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> ba2_gnrl_writer::write_to(std::string_view host_path) const {
  return write_to(host_path, write_execution_options{});
}

result<void> ba2_gnrl_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "BA2 GNRL writer worker_count must be positive"};
  }
  return formats::ba2::write_ba2_gnrl_archive(state_->target, state_->options, state_->entries, host_path);
}

} // namespace libbsa

namespace libbsa::formats::ba2 {

namespace {

constexpr std::uint32_t starfield_deflate_method = 0U;
constexpr std::uint32_t starfield_lz4_block_method = 3U;
// Serialize the BA2 `BTDX`/`GNRL` header and required `0xBAADF00D` record sentinel explicitly.
constexpr std::uint32_t ba2_btdx_magic = 0x5844'5442U;
constexpr std::uint32_t ba2_gnrl_magic = 0x4C52'4E47U;
constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
constexpr std::size_t common_header_size = 24U;
constexpr std::size_t starfield_v2_header_size = 32U;
constexpr std::size_t starfield_v3_header_size = 36U;
constexpr std::size_t gnrl_record_size = 36U;

struct prepared_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::array<std::byte, 4> extension{};
  std::uint32_t name_hash{};
  std::uint32_t directory_hash{};
  std::uint32_t record_flags{};
  std::uint64_t payload_offset{};
  std::uint32_t packed_size{};
  std::uint32_t raw_size{};
  bool owns_payload_bytes{true};
  std::vector<std::byte> stored_payload;
};

struct payload_assignment {
  std::uint64_t offset{};
  std::uint32_t stored_size{};
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
    return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
  }
  return static_cast<std::uint32_t>(value);
}

result<std::uint16_t> checked_u16(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint16_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds UInt16 range"};
  }
  return static_cast<std::uint16_t>(value);
}

std::uint32_t version_for(ba2_gnrl_target target) noexcept {
  switch (target) {
  case ba2_gnrl_target::fallout4:
    return 1U;
  case ba2_gnrl_target::starfield_v2:
    return 2U;
  case ba2_gnrl_target::starfield_v3:
    return 3U;
  }
  return 0U;
}

std::size_t header_size_for(std::uint32_t version) noexcept {
  if (version >= 3U) {
    return starfield_v3_header_size;
  }
  if (version >= 2U) {
    return starfield_v2_header_size;
  }
  return common_header_size;
}

std::pair<std::string_view, std::string_view> split_directory_file(std::string_view archive_path) noexcept {
  const auto slash = archive_path.find_last_of('/');
  if (slash == std::string_view::npos) {
    return {{}, archive_path};
  }
  return {archive_path.substr(0, slash), archive_path.substr(slash + 1U)};
}

result<std::vector<std::byte>> read_source_bytes(const ba2_gnrl_writer_entry& entry) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }

  std::ifstream input{entry.host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
  }
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "BA2 GNRL writer failed while reading disk source"};
  }
  return bytes;
}

bool archive_default_compressed(archive_compression_policy policy) noexcept {
  switch (policy) {
  case archive_compression_policy::target_default:
  case archive_compression_policy::all_compressed:
    return true;
  case archive_compression_policy::all_raw:
    return false;
  }
  return true;
}

bool requested_entry_compression(bool archive_compressed, entry_compression_policy policy) noexcept {
  switch (policy) {
  case entry_compression_policy::inherit:
    return archive_compressed;
  case entry_compression_policy::raw:
    return false;
  case entry_compression_policy::compressed:
    return true;
  }
  return archive_compressed;
}

result<detail::compression_method> compression_method_for_compressed_entry(ba2_gnrl_target target,
                                                                           std::uint32_t starfield_method) {
  switch (target) {
  case ba2_gnrl_target::fallout4:
  case ba2_gnrl_target::starfield_v2:
    return detail::compression_method::deflate;
  case ba2_gnrl_target::starfield_v3:
    if (starfield_method == starfield_deflate_method) {
      return detail::compression_method::deflate;
    }
    if (starfield_method == starfield_lz4_block_method) {
      return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, "BA2 GNRL Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 GNRL writer target profile is not supported"};
}

result<std::array<std::byte, 4>> extension_fourcc_for(std::string_view archive_path);

result<std::vector<prepared_entry>> prepare_entries(ba2_gnrl_target target,
                                                    const ba2_gnrl_writer_options& options,
                                                    std::span<const ba2_gnrl_writer_entry> entries) {
  const bool archive_compressed = archive_default_compressed(options.compression);
  std::vector<prepared_entry> prepared;
  prepared.reserve(entries.size());

  for (const auto& entry : entries) {
    auto payload = read_source_bytes(entry);
    if (!payload) {
      return payload.error();
    }
    const bool entry_compressed = !payload.value().empty() &&
                                  requested_entry_compression(archive_compressed, entry.options.compression);
    auto raw_size = checked_u32(payload.value().size(), "BA2 GNRL raw payload size");
    if (!raw_size) {
      return raw_size.error();
    }
    std::vector<std::byte> stored_payload;
    std::uint32_t packed_size = 0U;
    if (entry_compressed) {
      auto method = compression_method_for_compressed_entry(target, options.starfield_compression_method);
      if (!method) {
        return method.error();
      }
      auto compressed = detail::compress_payload(method.value(), payload.value());
      if (!compressed) {
        return compressed.error();
      }
      auto packed = checked_u32(compressed.value().size(), "BA2 GNRL packed payload size");
      if (!packed) {
        return packed.error();
      }
      packed_size = packed.value();
      stored_payload = std::move(compressed.value());
    } else {
      stored_payload = std::move(payload.value());
    }
    auto extension = extension_fourcc_for(entry.archive_path_original);
    if (!extension) {
      return extension.error();
    }

    const auto [directory, file_name] = split_directory_file(entry.archive_path_canonical);
    if (file_name.empty()) {
      return error{error_code::invalid_argument, "BA2 GNRL archive path must include a file name"};
    }

    prepared.push_back(prepared_entry{entry.archive_path_original,
                                      entry.archive_path_canonical,
                                      extension.value(),
                                      detail::hash_fo4(file_name),
                                      detail::hash_fo4(directory),
                                       entry.options.record_flags.value_or(0U),
                                       0U,
                                       packed_size,
                                       raw_size.value(),
                                       true,
                                       std::move(stored_payload)});
  }

  // D-17 keeps Phase 8 deterministic with a canonical-path fallback because traced BA2 writer evidence does not
  // prove a stricter hash sort requirement. Records and final filename-table entries stay paired by this order.
  std::sort(prepared.begin(), prepared.end(), [](const prepared_entry& lhs, const prepared_entry& rhs) {
    return lhs.archive_path_canonical < rhs.archive_path_canonical;
  });
  return prepared;
}

result<void> assign_payload_offsets(std::span<prepared_entry> entries,
                                    std::uint32_t version,
                                    bool deduplicate_payloads,
                                    std::uint64_t& file_table_offset) {
  std::uint64_t record_bytes = 0;
  if (entries.size() > std::numeric_limits<std::uint64_t>::max() / gnrl_record_size) {
    return error{error_code::format_error, "BA2 GNRL record table size overflows"};
  }
  record_bytes = static_cast<std::uint64_t>(entries.size()) * gnrl_record_size;

  std::uint64_t cursor = 0;
  if (!add_fits_u64(header_size_for(version), record_bytes, cursor)) {
    return error{error_code::format_error, "BA2 GNRL metadata size overflows"};
  }
  const auto first_payload_offset = cursor;

  std::map<std::vector<std::byte>, payload_assignment> deduplicated_payloads;
  for (auto& entry : entries) {
    if (deduplicate_payloads) {
      // D-23 requires dedupe after raw-vs-compressed routing, so this key is the exact byte span
      // the writer would store in the BA2 payload area rather than the caller's source bytes.
      const auto duplicate = deduplicated_payloads.find(entry.stored_payload);
      if (duplicate != deduplicated_payloads.end()) {
        entry.payload_offset = duplicate->second.offset;
        entry.owns_payload_bytes = false;
        continue;
      }
    }

    // Empty BA2 GNRL entries do not own a physical payload span. Point them at the first payload byte so
    // FileTableOffset remains after every real payload while readers validate the zero-length span safely.
    entry.payload_offset = entry.stored_payload.empty() ? first_payload_offset : cursor;
    entry.owns_payload_bytes = true;
    auto stored_size = checked_u32(entry.stored_payload.size(), "BA2 GNRL stored payload size");
    if (!stored_size) {
      return stored_size.error();
    }
    if (deduplicate_payloads) {
      deduplicated_payloads.emplace(entry.stored_payload, payload_assignment{entry.payload_offset, stored_size.value()});
    }
    if (!add_fits_u64(cursor, entry.stored_payload.size(), cursor)) {
      return error{error_code::format_error, "BA2 GNRL payload span overflows"};
    }
  }

  file_table_offset = cursor;
  return {};
}

result<void> write_name(detail::binary_writer& writer, std::string_view name) {
  auto length = checked_u16(name.size(), "BA2 GNRL filename-table entry length");
  if (!length) {
    return length.error();
  }
  auto written = writer.write_u16_le(length.value());
  if (!written) {
    return written.error();
  }
  for (const char ch : name) {
    if (!(written = writer.write_u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch))))) {
      return written.error();
    }
  }
  return {};
}

result<void> write_archive_bytes(ba2_gnrl_target target,
                                 const ba2_gnrl_writer_options& options,
                                 std::span<const prepared_entry> entries,
                                 std::uint32_t version,
                                 std::uint64_t file_table_offset,
                                 const std::filesystem::path& output_path) {
  detail::binary_writer writer;
  auto written = writer.write_u32_le(ba2_btdx_magic);
  if (!(written = writer.write_u32_le(version)) || !(written = writer.write_u32_le(ba2_gnrl_magic))) {
    return written.error();
  }
  auto file_count = checked_u32(entries.size(), "BA2 GNRL file count");
  if (!file_count) {
    return file_count.error();
  }
  if (!(written = writer.write_u32_le(file_count.value())) || !(written = writer.write_u64_le(file_table_offset))) {
    return written.error();
  }
  if (version >= 2U) {
    // xEdit/BSArchPro initializes Starfield writer Unknown1/Unknown2 to 1/0; options can override these raw
    // compatibility fields while keeping them library-owned and version-gated in public metadata.
    if (!(written = writer.write_u32_le(options.starfield_unknown1)) ||
        !(written = writer.write_u32_le(options.starfield_unknown2))) {
      return written.error();
    }
  }
  if (version >= 3U) {
    // Phase 8 treats v3 GNRL as a structurally supported profile. Method 3 remains the default raw-LZ4-block
    // method for later compression support; raw entries still serialize with PackedSize == 0 in this plan.
    if (!(written = writer.write_u32_le(options.starfield_compression_method))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!(written = writer.write_u32_le(entry.name_hash)) || !(written = writer.write_bytes(entry.extension)) ||
        !(written = writer.write_u32_le(entry.directory_hash)) || !(written = writer.write_u32_le(entry.record_flags)) ||
        !(written = writer.write_u64_le(entry.payload_offset)) || !(written = writer.write_u32_le(entry.packed_size)) ||
        !(written = writer.write_u32_le(entry.raw_size)) || !(written = writer.write_u32_le(ba2_record_sentinel))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!entry.owns_payload_bytes) {
      continue;
    }
    if (!(written = writer.write_bytes(entry.stored_payload))) {
      return written.error();
    }
  }

  for (const auto& entry : entries) {
    if (!(written = write_name(writer, entry.archive_path_original))) {
      return written.error();
    }
  }

  (void)target;
  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed to create temporary output"};
  }
  const auto bytes = writer.bytes();
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    return error{error_code::io_error, "BA2 GNRL writer failed while writing temporary output"};
  }
  return {};
}

result<bool> path_exists_noexcept(const std::filesystem::path& path) {
  std::error_code fs_error;
  const bool exists = std::filesystem::exists(path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "BA2 GNRL writer failed to inspect output host path"};
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
      return error{error_code::io_error, "BA2 GNRL writer failed to reserve temporary output directory"};
    }
  }
  return error{error_code::io_error, "BA2 GNRL writer exhausted temporary output directory names"};
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
  return error{error_code::io_error, "BA2 GNRL writer exhausted backup output path names"};
}

result<void> validate_target_options(ba2_gnrl_target target, const ba2_gnrl_writer_options& options) {
  switch (target) {
  case ba2_gnrl_target::fallout4:
  case ba2_gnrl_target::starfield_v2:
    return {};
  case ba2_gnrl_target::starfield_v3:
    if (options.starfield_compression_method == starfield_deflate_method ||
        options.starfield_compression_method == starfield_lz4_block_method) {
      return {};
    }
    return error{error_code::unsupported, "BA2 GNRL Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 GNRL writer target profile is not supported"};
}

bool is_ascii_extension_byte(unsigned char value) noexcept { return value > 0x20U && value <= 0x7EU; }

result<std::array<std::byte, 4>> extension_fourcc_for(std::string_view archive_path) {
  const auto slash = archive_path.find_last_of('/');
  const auto file_name = slash == std::string_view::npos ? archive_path : archive_path.substr(slash + 1U);
  const auto dot = file_name.find_last_of('.');
  if (dot == std::string_view::npos || dot + 1U == file_name.size()) {
    return error{error_code::invalid_argument, "BA2 GNRL archive path must include a file extension"};
  }

  const auto extension = file_name.substr(dot + 1U);
  if (extension.size() > 4U) {
    return error{error_code::invalid_argument, "BA2 GNRL extension exceeds four-byte record field"};
  }

  std::array<std::byte, 4> fourcc{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
  for (std::size_t index = 0; index < extension.size(); ++index) {
    const auto value = static_cast<unsigned char>(extension[index]);
    if (!is_ascii_extension_byte(value)) {
      return error{error_code::invalid_argument, "BA2 GNRL extension must contain printable ASCII bytes"};
    }
    fourcc[index] = static_cast<std::byte>(value);
  }
  return fourcc;
}

result<void> validate_entries(std::span<const ba2_gnrl_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL writer requires at least one file entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "BA2 GNRL writer has duplicate canonical archive paths"};
    }

    auto fourcc = extension_fourcc_for(entry.archive_path_original);
    if (!fourcc) {
      return fourcc.error();
    }

    if (!entry.from_memory) {
      std::ifstream input{entry.host_path, std::ios::binary};
      if (!input) {
        return error{error_code::io_error, "BA2 GNRL writer failed to open disk source"};
      }
    }
  }

  return {};
}

} // namespace

result<void> write_ba2_gnrl_archive(ba2_gnrl_target target,
                                    const ba2_gnrl_writer_options& options,
                                    std::span<const ba2_gnrl_writer_entry> entries,
                                    std::string_view output_host_path) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "BA2 GNRL output host path must not be empty"};
  }

  auto target_options = validate_target_options(target, options);
  if (!target_options) {
    return target_options.error();
  }

  const auto output_path = std::filesystem::path{output_host_path};
  auto output_exists = path_exists_noexcept(output_path);
  if (!output_exists) {
    return output_exists.error();
  }
  if (!options.overwrite_existing && output_exists.value()) {
    return error{error_code::io_error, "BA2 GNRL output host path already exists"};
  }

  auto validated = validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  const auto version = version_for(target);
  auto prepared = prepare_entries(target, options, entries);
  if (!prepared) {
    return prepared.error();
  }

  std::uint64_t file_table_offset = 0;
  auto offsets = assign_payload_offsets(prepared.value(), version, options.deduplicate_payloads, file_table_offset);
  if (!offsets) {
    return offsets.error();
  }

  auto temp_dir = make_unique_publish_directory(output_path);
  if (!temp_dir) {
    return temp_dir.error();
  }
  const auto temp_path = temp_dir.value() / output_path.filename();

  auto written = write_archive_bytes(target, options, prepared.value(), version, file_table_offset, temp_path);
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
        return error{error_code::io_error, "BA2 GNRL writer refuses to replace non-regular output host path"};
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
        return error{error_code::io_error, "BA2 GNRL writer failed to reserve output backup"};
      }

      std::filesystem::rename(temp_path, output_path, fs_error);
      if (fs_error) {
        std::error_code rollback_error;
        std::filesystem::rename(backup_path.value(), output_path, rollback_error);
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "BA2 GNRL writer failed to publish output host path"};
      }

      std::filesystem::remove(backup_path.value(), fs_error);
      cleanup_publish_directory(temp_dir.value());
      return {};
    }
  }

  std::filesystem::rename(temp_path, output_path, fs_error);
  if (fs_error) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "BA2 GNRL writer failed to publish output host path"};
  }
  cleanup_publish_directory(temp_dir.value());
  return {};
}

} // namespace libbsa::formats::ba2
