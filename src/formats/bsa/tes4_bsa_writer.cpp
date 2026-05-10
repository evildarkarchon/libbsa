#include "formats/bsa/tes4_bsa_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/atomic_file_ops.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/compression_router.hpp>
#include <detail/parallel_work.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libbsa {

struct tes4_bsa_writer::state {
  tes4_bsa_target target;
  tes4_bsa_writer_options options;
  std::vector<formats::bsa::tes4_writer_entry> entries;
};

namespace {

std::string stored_tes4_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  // TES4 BSA folder records and embedded-name prefixes use Bethesda-style
  // backslashes even though public lookup keys normalize both separator forms.
  std::replace(preserved.begin(), preserved.end(), '/', '\\');
  return preserved;
}

result<formats::bsa::tes4_writer_entry> make_entry(std::string_view archive_path,
                                                   entry_compression_policy compression) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::bsa::tes4_writer_entry entry;
  entry.archive_path_original = stored_tes4_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.compression = compression;
  return entry;
}

} // namespace

tes4_bsa_writer::tes4_bsa_writer(tes4_bsa_target target)
    : tes4_bsa_writer(target, tes4_bsa_writer_options{}) {}

tes4_bsa_writer::tes4_bsa_writer(tes4_bsa_target target, tes4_bsa_writer_options options)
    : state_(std::make_shared<state>(state{target, options, {}})) {}

tes4_bsa_target tes4_bsa_writer::target() const noexcept { return state_->target; }

const tes4_bsa_writer_options& tes4_bsa_writer::options() const noexcept { return state_->options; }

result<void> tes4_bsa_writer::add_file(std::string_view archive_path,
                                       std::string_view host_path,
                                       entry_compression_policy compression) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA disk source host path must not be empty"};
  }

  auto entry = make_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }

  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes4_bsa_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        entry_compression_policy compression) {
  auto entry = make_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes4_bsa_writer::write_to(std::string_view host_path) const {
  return write_to(host_path, write_execution_options{});
}

result<void> tes4_bsa_writer::write_to(std::string_view host_path, write_execution_options execution) const {
  if (execution.worker_count == 0U) {
    return error{error_code::invalid_argument, "TES4 BSA writer worker_count must be positive"};
  }
  return formats::bsa::write_tes4_bsa_archive(state_->target, state_->options, state_->entries, host_path,
                                              execution.worker_count);
}

} // namespace libbsa

namespace libbsa::formats::bsa {

namespace {

constexpr std::uint32_t oblivion_version = 0x67U;
constexpr std::uint32_t fallout3_version = 0x68U;
constexpr std::uint32_t skyrim_se_version = 0x69U;
constexpr std::uint32_t header_size = 36U;
constexpr std::uint32_t include_directory_names = 0x0001U;
constexpr std::uint32_t include_file_names = 0x0002U;
constexpr std::uint32_t archive_compress_by_default = 0x0004U;
constexpr std::uint32_t archive_embed_names = 0x0100U;
constexpr std::uint32_t file_size_compression_toggle = 0x40000000U;

constexpr std::uint32_t file_flag_meshes = 0x0001U;
constexpr std::uint32_t file_flag_textures = 0x0002U;
constexpr std::uint32_t file_flag_sounds = 0x0004U;
constexpr std::uint32_t file_flag_scripts = 0x0008U;
constexpr std::uint32_t file_flag_menus = 0x0010U;
constexpr std::uint32_t file_flag_misc = 0x0100U;
constexpr std::size_t payload_stream_chunk_size = 64U * 1024U;

struct prepared_entry {
  std::string folder;
  std::string file_name;
  std::uint64_t file_hash{0};
  std::uint32_t stored_size{0};
  std::uint32_t record_flags{0};
  std::uint32_t payload_offset{0};
  bool owns_payload_bytes{true};
  bool stream_raw_disk{false};
  std::uint32_t raw_disk_size{0};
  std::vector<std::byte> stored_payload;
  std::string raw_disk_host_path;
};

struct prepared_folder {
  std::string name;
  std::uint64_t hash{0};
  std::uint64_t folder_block_offset{0};
  std::vector<prepared_entry> entries;
};

struct prepared_entry_result {
  prepared_entry entry;
  std::uint32_t file_flags{0};
};

result<std::uint32_t> version_for(tes4_bsa_target target) {
  switch (target) {
  case tes4_bsa_target::oblivion:
    return oblivion_version;
  case tes4_bsa_target::fallout3:
    return fallout3_version;
  case tes4_bsa_target::skyrim_se:
    return skyrim_se_version;
  }
  return error{error_code::invalid_argument, "TES4 BSA writer target profile is not supported"};
}

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

result<std::uint32_t> checked_size_flags_payload_size(std::uint64_t value, std::string_view description) {
  if (value > (std::numeric_limits<std::uint32_t>::max() & ~file_size_compression_toggle)) {
    return error{error_code::format_error, std::string{description} + " exceeds TES4 BSA size-flag limits"};
  }
  return static_cast<std::uint32_t>(value);
}

result<std::uint8_t> checked_name_size(std::size_t size, std::string_view description) {
  if (size > static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())) {
    return error{error_code::format_error, std::string{description} + " exceeds BSA name length limits"};
  }
  return static_cast<std::uint8_t>(size);
}

std::string lower_ascii(std::string_view value) {
  std::string lowered{value};
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return lowered;
}

std::string extension_of(std::string_view file_name) {
  const auto dot = file_name.find_last_of('.');
  if (dot == std::string_view::npos) {
    return {};
  }
  return lower_ascii(file_name.substr(dot));
}

std::uint64_t file_hash_for(std::string_view file_name) {
  const auto dot = file_name.find_last_of('.');
  if (dot == std::string_view::npos) {
    return detail::hash_tes4(file_name, {});
  }
  return detail::hash_tes4(file_name.substr(0, dot), file_name.substr(dot));
}

std::uint32_t file_flag_for_extension(std::string_view extension, std::uint32_t version) noexcept {
  if (extension == ".nif" || extension == ".kf") {
    return file_flag_meshes;
  }
  if (extension == ".dds") {
    return file_flag_textures;
  }
  if (extension == ".wav") {
    return file_flag_sounds;
  }
  if (extension == ".pex" || extension == ".psc") {
    return file_flag_scripts;
  }
  if (extension == ".xml") {
    return version == oblivion_version ? file_flag_menus : 0U;
  }
  if (extension == ".txt" || extension == ".html" || extension == ".bat" || extension == ".scc") {
    return version == skyrim_se_version ? 0U : file_flag_misc;
  }
  return 0U;
}

result<std::vector<std::byte>> read_disk_source_bytes(const std::string& host_path) {
  std::ifstream input{host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "TES4 BSA writer failed to open disk source"};
  }
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "TES4 BSA writer failed while reading disk source"};
  }
  return bytes;
}

result<std::vector<std::byte>> read_source_bytes(const tes4_writer_entry& entry) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }
  return read_disk_source_bytes(entry.host_path);
}

result<std::uint32_t> disk_payload_size(const std::string& host_path) {
  const auto path = std::filesystem::path{host_path};
  std::error_code fs_error;
  const bool regular_file = std::filesystem::is_regular_file(path, fs_error);
  if (fs_error || !regular_file) {
    return error{error_code::io_error, "TES4 BSA writer failed to inspect disk source"};
  }
  const auto size = std::filesystem::file_size(path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "TES4 BSA writer failed to size disk source"};
  }
  return checked_u32(size, "TES4 BSA disk source size");
}

bool archive_default_compressed(tes4_bsa_target target, archive_compression_policy policy) noexcept {
  switch (policy) {
  case archive_compression_policy::target_default:
    return target != tes4_bsa_target::oblivion;
  case archive_compression_policy::all_raw:
    return false;
  case archive_compression_policy::all_compressed:
    return true;
  }
  return false;
}

bool requested_entry_compression(bool archive_default, entry_compression_policy policy) noexcept {
  switch (policy) {
  case entry_compression_policy::inherit:
    return archive_default;
  case entry_compression_policy::raw:
    return false;
  case entry_compression_policy::compressed:
    return true;
  }
  return false;
}

result<detail::compression_method> compression_method_for_target(tes4_bsa_target target) {
  switch (target) {
  case tes4_bsa_target::oblivion:
  case tes4_bsa_target::fallout3:
    return detail::compression_method::deflate;
  case tes4_bsa_target::skyrim_se:
    return detail::compression_method::lz4_frame;
  }
  return error{error_code::invalid_argument, "TES4 BSA writer target profile has no compression method"};
}

void append_u32_le(std::vector<std::byte>& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<std::byte>(value & 0xFFU));
  bytes.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
  bytes.push_back(static_cast<std::byte>((value >> 16U) & 0xFFU));
  bytes.push_back(static_cast<std::byte>((value >> 24U) & 0xFFU));
}

result<std::vector<std::byte>> make_embedded_name_prefix(bool emit_embedded_name, std::string_view file_name) {
  std::vector<std::byte> stored;
  if (!emit_embedded_name) {
    return stored;
  }

  auto embedded_name_length = checked_name_size(file_name.size(), "TES4 BSA embedded file name");
  if (!embedded_name_length) {
    return embedded_name_length.error();
  }
  stored.reserve(1U + file_name.size());
  stored.push_back(static_cast<std::byte>(embedded_name_length.value()));
  for (const char ch : file_name) {
    stored.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return stored;
}

result<std::vector<std::byte>> encode_stored_payload(tes4_bsa_target target,
                                                      std::span<const std::byte> raw_payload,
                                                      bool effective_compressed,
                                                      bool emit_embedded_name,
                                                      std::string_view file_name) {
  auto stored = make_embedded_name_prefix(emit_embedded_name, file_name);
  if (!stored) {
    return stored.error();
  }

  if (!effective_compressed) {
    stored.value().reserve(stored.value().size() + raw_payload.size());
    stored.value().insert(stored.value().end(), raw_payload.begin(), raw_payload.end());
    return stored.value();
  }

  auto raw_size = checked_u32(raw_payload.size(), "TES4 BSA compressed raw payload size");
  if (!raw_size) {
    return raw_size.error();
  }
  auto method = compression_method_for_target(target);
  if (!method) {
    return method.error();
  }
  auto compressed = detail::compress_payload(method.value(), raw_payload);
  if (!compressed) {
    return compressed.error();
  }

  stored.value().reserve(stored.value().size() + 4U + compressed.value().size());
  append_u32_le(stored.value(), raw_size.value());
  stored.value().insert(stored.value().end(), compressed.value().begin(), compressed.value().end());
  return stored.value();
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

result<void> write_folder_name(detail::binary_writer& writer, std::string_view value) {
  auto size = checked_name_size(value.size() + 1U, "TES4 BSA folder name");
  if (!size) {
    return size.error();
  }
  auto written = writer.write_u8(size.value());
  if (!written) {
    return written.error();
  }
  return write_string_terminated(writer, value);
}

std::pair<std::string, std::string> split_folder_file(std::string_view path) {
  const auto separator = path.find_last_of("/\\");
  if (separator == std::string_view::npos) {
    return {{}, std::string{path}};
  }
  return {std::string{path.substr(0, separator)}, std::string{path.substr(separator + 1U)}};
}

std::string join_folder_file(std::string_view folder, std::string_view file_name) {
  std::string path;
  path.reserve(folder.size() + 1U + file_name.size());
  path.append(folder);
  path.push_back('\\');
  path.append(file_name);
  return path;
}

result<prepared_entry_result> prepare_one_entry(const tes4_writer_entry& entry,
                                                tes4_bsa_target target,
                                                bool archive_default_is_compressed,
                                                bool emit_embedded_names,
                                                std::uint32_t version) {
  auto [folder, file_name] = split_folder_file(entry.archive_path_original);
  if (folder.empty() || file_name.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA writer archive paths must include folder and file names"};
  }

  const auto entry_file_flags = file_flag_for_extension(extension_of(file_name), version);
  const bool entry_wants_compression = requested_entry_compression(archive_default_is_compressed, entry.compression);

  std::uint32_t raw_size = 0U;
  if (entry.from_memory) {
    auto memory_size = checked_u32(entry.memory_bytes.size(), "TES4 BSA raw payload size");
    if (!memory_size) {
      return memory_size.error();
    }
    raw_size = memory_size.value();
  } else {
    auto file_size = disk_payload_size(entry.host_path);
    if (!file_size) {
      return file_size.error();
    }
    raw_size = file_size.value();
  }

  const bool effective_compressed = entry_wants_compression && raw_size != 0U;
  std::uint32_t record_flags = 0U;
  if (archive_default_is_compressed != effective_compressed) {
    // Zero-byte entries are forced raw, so they still need the XOR toggle when
    // the archive default is compressed or readers will expect a size prefix.
    record_flags |= file_size_compression_toggle;
  }

  prepared_entry prepared;
  prepared.folder = folder;
  prepared.file_name = std::move(file_name);
  prepared.file_hash = file_hash_for(prepared.file_name);
  prepared.record_flags = record_flags;
  const auto embedded_name = join_folder_file(prepared.folder, prepared.file_name);

  if (!entry.from_memory && !effective_compressed) {
    auto prefix = make_embedded_name_prefix(emit_embedded_names, embedded_name);
    if (!prefix) {
      return prefix.error();
    }
    const auto stored_size64 = static_cast<std::uint64_t>(prefix.value().size()) + raw_size;
    auto stored_size = checked_size_flags_payload_size(stored_size64, "TES4 BSA stored payload size");
    if (!stored_size) {
      return stored_size.error();
    }
    prepared.stored_size = stored_size.value();
    prepared.stored_payload = std::move(prefix.value());
    prepared.raw_disk_host_path = entry.host_path;
    prepared.raw_disk_size = raw_size;
    prepared.stream_raw_disk = true;
    return prepared_entry_result{std::move(prepared), entry_file_flags};
  }

  auto payload = read_source_bytes(entry);
  if (!payload) {
    return payload.error();
  }
  if (!entry.from_memory && payload.value().size() != raw_size) {
    return error{error_code::io_error, "TES4 BSA disk source changed during finalization"};
  }

  auto stored_payload = encode_stored_payload(target, payload.value(), effective_compressed, emit_embedded_names,
                                              embedded_name);
  if (!stored_payload) {
    return stored_payload.error();
  }
  auto stored_size = checked_size_flags_payload_size(stored_payload.value().size(), "TES4 BSA stored payload size");
  if (!stored_size) {
    return stored_size.error();
  }
  prepared.stored_size = stored_size.value();
  prepared.stored_payload = std::move(stored_payload.value());
  return prepared_entry_result{std::move(prepared), entry_file_flags};
}

result<std::vector<prepared_folder>> prepare_folders(std::span<const tes4_writer_entry> entries,
                                                      tes4_bsa_target target,
                                                      bool archive_default_is_compressed,
                                                      bool emit_embedded_names,
                                                      std::uint32_t version,
                                                      std::uint32_t worker_count,
                                                      std::uint32_t& file_flags) {
  std::vector<std::optional<prepared_entry_result>> prepared_by_index(entries.size());
  auto prepared_work = detail::run_indexed_work(entries.size(), worker_count, [&](std::size_t index) -> result<void> {
    auto prepared = prepare_one_entry(entries[index], target, archive_default_is_compressed, emit_embedded_names, version);
    if (!prepared) {
      return prepared.error();
    }
    prepared_by_index[index] = std::move(prepared.value());
    return {};
  });
  if (!prepared_work) {
    return prepared_work.error();
  }

  std::map<std::string, std::vector<prepared_entry>> grouped;
  file_flags = 0U;
  for (auto& prepared : prepared_by_index) {
    if (!prepared.has_value()) {
      return error{error_code::io_error, "TES4 BSA writer failed to prepare an entry"};
    }
    file_flags |= prepared->file_flags;
    grouped[prepared->entry.folder].push_back(std::move(prepared->entry));
  }

  std::vector<prepared_folder> folders;
  folders.reserve(grouped.size());
  for (auto& [folder_name, folder_entries] : grouped) {
    std::sort(folder_entries.begin(), folder_entries.end(), [](const prepared_entry& lhs, const prepared_entry& rhs) {
      return lhs.file_hash < rhs.file_hash;
    });
    folders.push_back(prepared_folder{folder_name, detail::hash_tes4(folder_name, {}), 0U, std::move(folder_entries)});
  }
  std::sort(folders.begin(), folders.end(), [](const prepared_folder& lhs, const prepared_folder& rhs) {
    return lhs.hash < rhs.hash;
  });
  return folders;
}

struct payload_assignment {
  std::uint32_t offset{0};
  std::uint32_t stored_size{0};
};

struct assigned_payload {
  const prepared_entry* entry{nullptr};
  payload_assignment assignment;
};

result<std::vector<std::byte>> materialize_stored_payload(const prepared_entry& entry) {
  if (!entry.stream_raw_disk) {
    return entry.stored_payload;
  }

  auto raw_payload = read_disk_source_bytes(entry.raw_disk_host_path);
  if (!raw_payload) {
    return raw_payload.error();
  }
  if (raw_payload.value().size() != entry.raw_disk_size) {
    return error{error_code::io_error, "TES4 BSA disk source changed during dedupe preparation"};
  }

  std::vector<std::byte> bytes;
  bytes.reserve(entry.stored_payload.size() + raw_payload.value().size());
  bytes.insert(bytes.end(), entry.stored_payload.begin(), entry.stored_payload.end());
  bytes.insert(bytes.end(), raw_payload.value().begin(), raw_payload.value().end());
  return bytes;
}

result<bool> stored_payloads_equal(const prepared_entry& lhs, const prepared_entry& rhs) {
  if (lhs.stored_size != rhs.stored_size) {
    return false;
  }
  if (!lhs.stream_raw_disk && !rhs.stream_raw_disk) {
    return lhs.stored_payload == rhs.stored_payload;
  }

  auto lhs_bytes = materialize_stored_payload(lhs);
  if (!lhs_bytes) {
    return lhs_bytes.error();
  }
  auto rhs_bytes = materialize_stored_payload(rhs);
  if (!rhs_bytes) {
    return rhs_bytes.error();
  }
  return lhs_bytes.value() == rhs_bytes.value();
}

result<void> assign_offsets(std::span<prepared_folder> folders,
                            std::uint32_t version,
                            std::uint32_t file_names_length,
                            bool deduplicate_payloads) {
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
    if (!add_fits_u64(folder_block_cursor, file_names_length, folder.folder_block_offset)) {
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
      !add_fits_u64(payload_cursor, file_names_length, payload_cursor)) {
    return error{error_code::format_error, "TES4 BSA metadata size overflows"};
  }

  std::vector<assigned_payload> deduplicated_payloads;
  for (auto& folder : folders) {
    for (auto& entry : folder.entries) {
      if (deduplicate_payloads) {
        // Dedupe compares the complete final stored byte stream. Raw disk sources
        // are compared on demand so the publish path can still stream unique files.
        for (const auto& candidate : deduplicated_payloads) {
          auto duplicate = stored_payloads_equal(entry, *candidate.entry);
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
  return {};
}

result<void> validate_entries(std::span<const tes4_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA writer requires at least one file entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "TES4 BSA writer has duplicate canonical archive paths"};
    }

    if (!entry.from_memory) {
      std::ifstream input{entry.host_path, std::ios::binary};
      if (!input) {
        return error{error_code::io_error, "TES4 BSA writer failed to open disk source"};
      }
    }
  }

  return {};
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
    return error{error_code::io_error, "TES4 BSA writer failed to open disk source"};
  }

  std::vector<std::byte> scratch(payload_stream_chunk_size);
  std::uint64_t remaining = expected_size;
  while (remaining > 0U) {
    const auto requested = static_cast<std::size_t>(
        std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.size())));
    input.read(reinterpret_cast<char*>(scratch.data()), static_cast<std::streamsize>(requested));
    if (input.gcount() != static_cast<std::streamsize>(requested)) {
      return error{error_code::io_error, "TES4 BSA writer failed while streaming disk source"};
    }
    auto written = write_span_to_stream(output, std::span<const std::byte>{scratch.data(), requested},
                                        "TES4 BSA writer");
    if (!written) {
      return written.error();
    }
    remaining -= requested;
  }

  // Metadata offsets are assigned before publishing; fail if the caller mutates
  // a disk source while finalization is streaming it into the temporary archive.
  char extra = '\0';
  if (input.get(extra)) {
    return error{error_code::io_error, "TES4 BSA disk source changed during finalization"};
  }
  if (input.bad()) {
    return error{error_code::io_error, "TES4 BSA writer failed while reading disk source"};
  }
  return {};
}

result<void> write_archive_bytes(std::span<const prepared_folder> folders,
                                 std::uint32_t version,
                                 std::uint32_t archive_flags,
                                 std::uint32_t file_flags,
                                 std::uint32_t total_folder_name_length,
                                 std::uint32_t total_file_name_length,
                                 std::uint32_t file_count,
                                 const std::filesystem::path& output_path) {
  detail::binary_writer writer;
  const std::byte magic[] = {std::byte{'B'}, std::byte{'S'}, std::byte{'A'}, std::byte{0}};
  auto written = writer.write_bytes(magic);
  if (!written) {
    return written.error();
  }
  if (!(written = writer.write_u32_le(version)) || !(written = writer.write_u32_le(header_size)) ||
      !(written = writer.write_u32_le(archive_flags)) ||
      !(written = writer.write_u32_le(static_cast<std::uint32_t>(folders.size()))) ||
      !(written = writer.write_u32_le(file_count)) || !(written = writer.write_u32_le(total_folder_name_length)) ||
      !(written = writer.write_u32_le(total_file_name_length)) || !(written = writer.write_u32_le(file_flags))) {
    return written.error();
  }

  for (const auto& folder : folders) {
    if (!(written = writer.write_u64_le(folder.hash)) ||
        !(written = writer.write_u32_le(static_cast<std::uint32_t>(folder.entries.size())))) {
      return written.error();
    }
    if (version == skyrim_se_version) {
      if (!(written = writer.write_u32_le(0U)) || !(written = writer.write_u64_le(folder.folder_block_offset))) {
        return written.error();
      }
    } else {
      auto offset = checked_u32(folder.folder_block_offset, "TES4 BSA folder offset");
      if (!offset) {
        return offset.error();
      }
      if (!(written = writer.write_u32_le(offset.value()))) {
        return written.error();
      }
    }
  }

  for (const auto& folder : folders) {
    if (!(written = write_folder_name(writer, folder.name))) {
      return written.error();
    }
    for (const auto& entry : folder.entries) {
      if (!(written = writer.write_u64_le(entry.file_hash)) ||
          !(written = writer.write_u32_le(entry.stored_size | entry.record_flags)) ||
          !(written = writer.write_u32_le(entry.payload_offset))) {
        return written.error();
      }
    }
  }

  for (const auto& folder : folders) {
    for (const auto& entry : folder.entries) {
      if (!(written = write_string_terminated(writer, entry.file_name))) {
        return written.error();
      }
    }
  }
  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "TES4 BSA writer failed to create temporary output"};
  }
  auto metadata_written = write_span_to_stream(output, writer.bytes(), "TES4 BSA writer metadata");
  if (!metadata_written) {
    return metadata_written.error();
  }
  for (const auto& folder : folders) {
    for (const auto& entry : folder.entries) {
      if (!entry.owns_payload_bytes) {
        continue;
      }
      auto prefix_written = write_span_to_stream(output, entry.stored_payload, "TES4 BSA writer payload");
      if (!prefix_written) {
        return prefix_written.error();
      }
      if (entry.stream_raw_disk) {
        auto streamed = stream_disk_payload_to_output(entry.raw_disk_host_path, entry.raw_disk_size, output);
        if (!streamed) {
          return streamed.error();
        }
      }
    }
  }
  return {};
}

result<bool> path_exists_noexcept(const std::filesystem::path& path) {
  std::error_code fs_error;
  const bool exists = std::filesystem::exists(path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "TES4 BSA writer failed to inspect output host path"};
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
    // Keep temporary archives isolated from caller-owned deterministic siblings such as `<archive>.tmp`.
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate;
    }
    if (fs_error) {
      return error{error_code::io_error, "TES4 BSA writer failed to reserve temporary output directory"};
    }
  }
  return error{error_code::io_error, "TES4 BSA writer exhausted temporary output directory names"};
}

void cleanup_publish_directory(const std::filesystem::path& temp_dir) noexcept {
  std::error_code fs_error;
  // Cleanup is best-effort because callers should receive the primary write/publish failure, not cleanup noise.
  std::filesystem::remove_all(temp_dir, fs_error);
}

} // namespace

result<void> write_tes4_bsa_archive(tes4_bsa_target target,
                                    const tes4_bsa_writer_options& options,
                                    std::span<const tes4_writer_entry> entries,
                                    std::string_view output_host_path,
                                    std::uint32_t worker_count) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA output host path must not be empty"};
  }
  if (worker_count == 0U) {
    return error{error_code::invalid_argument, "TES4 BSA writer worker_count must be positive"};
  }

  const auto output_path = std::filesystem::path{output_host_path};
  auto output_exists = path_exists_noexcept(output_path);
  if (!output_exists) {
    return output_exists.error();
  }
  if (!options.overwrite_existing && output_exists.value()) {
    return error{error_code::io_error, "TES4 BSA output host path already exists"};
  }

  auto validated = validate_entries(entries);
  if (!validated) {
    return validated.error();
  }

  auto version = version_for(target);
  if (!version) {
    return version.error();
  }

  const bool archive_default_is_compressed = archive_default_compressed(target, options.compression_policy);
  const bool emit_embedded_names = options.embed_file_names && version.value() != oblivion_version;

  std::uint32_t file_flags = 0U;
  auto folders = prepare_folders(entries, target, archive_default_is_compressed, emit_embedded_names, version.value(),
                                 worker_count, file_flags);
  if (!folders) {
    return folders.error();
  }

  std::uint64_t total_folder_name_length64 = 0;
  std::uint64_t total_file_name_length64 = 0;
  std::uint64_t file_count64 = 0;
  for (const auto& folder : folders.value()) {
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

  auto offsets = assign_offsets(folders.value(), version.value(), total_file_name_length.value(),
                                options.deduplicate_payloads);
  if (!offsets) {
    return offsets.error();
  }

  std::uint32_t archive_flags = include_directory_names | include_file_names;
  if (archive_default_is_compressed) {
    archive_flags |= archive_compress_by_default;
  }
  if (emit_embedded_names) {
    archive_flags |= archive_embed_names;
  }

  // D-11 keeps overwrite publishing in the same no-gap contract as other writer families.
  auto temp_dir = make_unique_publish_directory(output_path);
  if (!temp_dir) {
    return temp_dir.error();
  }
  const auto temp_path = temp_dir.value() / output_path.filename();

  auto written = write_archive_bytes(folders.value(), version.value(), archive_flags, file_flags,
                                     total_folder_name_length.value(), total_file_name_length.value(), file_count.value(),
                                     temp_path);
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
        return error{error_code::io_error, "TES4 BSA writer refuses to replace non-regular output host path"};
      }

      auto published = detail::replace_file_atomically(temp_path, output_path);
      if (!published) {
        cleanup_publish_directory(temp_dir.value());
        return error{error_code::io_error, "TES4 BSA writer failed to publish output host path"};
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
    return error{error_code::io_error, "TES4 BSA output host path already exists"};
  }

  auto published = detail::publish_file_without_replace(temp_path, output_path);
  if (!published) {
    cleanup_publish_directory(temp_dir.value());
    return error{error_code::io_error, "TES4 BSA writer failed to publish output host path"};
  }
  cleanup_publish_directory(temp_dir.value());
  return {};
}

} // namespace libbsa::formats::bsa
