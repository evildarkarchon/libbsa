#include "formats/bsa/tes4_bsa_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <cctype>
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

namespace libbsa {

struct tes4_bsa_writer::state {
  tes4_bsa_target target;
  tes4_bsa_writer_options options;
  std::vector<formats::bsa::tes4_writer_entry> entries;
};

namespace {

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

result<formats::bsa::tes4_writer_entry> make_entry(std::string_view archive_path,
                                                   entry_compression_policy compression) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::bsa::tes4_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
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
  return formats::bsa::write_tes4_bsa_archive(state_->target, state_->options, state_->entries, host_path);
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

struct prepared_entry {
  std::string folder;
  std::string file_name;
  std::uint64_t file_hash{0};
  std::uint32_t stored_size{0};
  std::uint32_t record_flags{0};
  std::uint32_t payload_offset{0};
  bool owns_payload_bytes{true};
  std::vector<std::byte> stored_payload;
};

struct prepared_folder {
  std::string name;
  std::uint64_t hash{0};
  std::uint64_t folder_block_offset{0};
  std::vector<prepared_entry> entries;
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

result<std::vector<std::byte>> read_source_bytes(const tes4_writer_entry& entry) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }

  std::ifstream input{entry.host_path, std::ios::binary};
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

result<std::vector<std::byte>> encode_stored_payload(tes4_bsa_target target,
                                                      std::span<const std::byte> raw_payload,
                                                      bool effective_compressed,
                                                      bool emit_embedded_name,
                                                      std::string_view file_name) {
  std::vector<std::byte> stored;
  if (emit_embedded_name) {
    auto embedded_name_length = checked_name_size(file_name.size(), "TES4 BSA embedded file name");
    if (!embedded_name_length) {
      return embedded_name_length.error();
    }
    stored.reserve(1U + file_name.size() + raw_payload.size());
    stored.push_back(static_cast<std::byte>(embedded_name_length.value()));
    for (const char ch : file_name) {
      stored.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
  }

  if (!effective_compressed) {
    stored.insert(stored.end(), raw_payload.begin(), raw_payload.end());
    return stored;
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

  stored.reserve(stored.size() + 4U + compressed.value().size());
  append_u32_le(stored, raw_size.value());
  stored.insert(stored.end(), compressed.value().begin(), compressed.value().end());
  return stored;
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
  const auto separator = path.find_last_of('/');
  if (separator == std::string_view::npos) {
    return {{}, std::string{path}};
  }
  return {std::string{path.substr(0, separator)}, std::string{path.substr(separator + 1U)}};
}

result<std::vector<prepared_folder>> prepare_folders(std::span<const tes4_writer_entry> entries,
                                                      tes4_bsa_target target,
                                                      bool archive_default_is_compressed,
                                                      bool emit_embedded_names,
                                                      std::uint32_t version,
                                                      std::uint32_t& file_flags) {
  std::map<std::string, std::vector<prepared_entry>> grouped;
  file_flags = 0U;

  for (const auto& entry : entries) {
    auto payload = read_source_bytes(entry);
    if (!payload) {
      return payload.error();
    }

    auto [folder, file_name] = split_folder_file(entry.archive_path_original);
    if (folder.empty() || file_name.empty()) {
      return error{error_code::invalid_argument, "TES4 BSA writer archive paths must include folder and file names"};
    }

    file_flags |= file_flag_for_extension(extension_of(file_name), version);
    const bool entry_wants_compression = requested_entry_compression(archive_default_is_compressed, entry.compression);
    const bool effective_compressed = entry_wants_compression && !payload.value().empty();
    auto stored_payload = encode_stored_payload(target, payload.value(), effective_compressed, emit_embedded_names, file_name);
    if (!stored_payload) {
      return stored_payload.error();
    }
    auto stored_size = checked_size_flags_payload_size(stored_payload.value().size(), "TES4 BSA stored payload size");
    if (!stored_size) {
      return stored_size.error();
    }
    std::uint32_t record_flags = 0U;
    if (archive_default_is_compressed != effective_compressed) {
      // Zero-byte entries are forced raw, so they still need the XOR toggle when
      // the archive default is compressed or readers will expect a size prefix.
      record_flags |= file_size_compression_toggle;
    }

    const auto file_hash = file_hash_for(file_name);
    grouped[folder].push_back(prepared_entry{folder,
                                              std::move(file_name),
                                              file_hash,
                                               stored_size.value(),
                                               record_flags,
                                               0U,
                                               true,
                                               std::move(stored_payload.value())});
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

    // TES5Edit-compatible folder offsets include the later file-name table length,
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

  std::map<std::vector<std::byte>, payload_assignment> deduplicated_payloads;
  for (auto& folder : folders) {
    for (auto& entry : folder.entries) {
      if (deduplicate_payloads) {
        // D-19 requires dedupe after the complete stored encoding is built, so
        // the key includes embedded-name prefixes, raw-size prefixes, and codec bytes.
        const auto duplicate = deduplicated_payloads.find(entry.stored_payload);
        if (duplicate != deduplicated_payloads.end()) {
          entry.payload_offset = duplicate->second.offset;
          entry.stored_size = duplicate->second.stored_size;
          entry.owns_payload_bytes = false;
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
        deduplicated_payloads.emplace(entry.stored_payload, payload_assignment{entry.payload_offset, entry.stored_size});
      }
      if (!add_fits_u64(payload_cursor, entry.stored_payload.size(), payload_cursor)) {
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
  for (const auto& folder : folders) {
    for (const auto& entry : folder.entries) {
      if (!entry.owns_payload_bytes) {
        continue;
      }
      if (!(written = writer.write_bytes(entry.stored_payload))) {
        return written.error();
      }
    }
  }

  std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
  if (!output) {
    return error{error_code::io_error, "TES4 BSA writer failed to create temporary output"};
  }
  const auto bytes = writer.bytes();
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    return error{error_code::io_error, "TES4 BSA writer failed while writing temporary output"};
  }
  return {};
}

} // namespace

result<void> write_tes4_bsa_archive(tes4_bsa_target target,
                                    const tes4_bsa_writer_options& options,
                                    std::span<const tes4_writer_entry> entries,
                                    std::string_view output_host_path) {
  if (output_host_path.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA output host path must not be empty"};
  }

  const auto output_path = std::filesystem::path{output_host_path};
  if (!options.overwrite_existing && std::filesystem::exists(output_path)) {
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
  auto folders = prepare_folders(entries, target, archive_default_is_compressed, emit_embedded_names, version.value(), file_flags);
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

  auto temp_path = output_path;
  temp_path += ".tmp";
  std::error_code fs_error;
  std::filesystem::remove(temp_path, fs_error);
  auto written = write_archive_bytes(folders.value(), version.value(), archive_flags, file_flags,
                                     total_folder_name_length.value(), total_file_name_length.value(), file_count.value(),
                                     temp_path);
  if (!written) {
    std::filesystem::remove(temp_path, fs_error);
    return written.error();
  }

  if (options.overwrite_existing) {
    std::filesystem::remove(output_path, fs_error);
    if (fs_error) {
      std::filesystem::remove(temp_path, fs_error);
      return error{error_code::io_error, "TES4 BSA writer failed to replace output host path"};
    }
  }
  std::filesystem::rename(temp_path, output_path, fs_error);
  if (fs_error) {
    std::filesystem::remove(temp_path, fs_error);
    return error{error_code::io_error, "TES4 BSA writer failed to publish output host path"};
  }
  return {};
}

} // namespace libbsa::formats::bsa
