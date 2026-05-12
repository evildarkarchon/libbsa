#include "formats/bsa/tes4_bsa_prepare.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/compression_router.hpp>
#include <detail/parallel_work.hpp>
#include <detail/writer_disk_source.hpp>

#include "texture/directxtex_analyzer.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <unordered_set>
#include <utility>

namespace libbsa::formats::bsa {

namespace {

constexpr std::size_t dds_metadata_probe_size = 148U;

struct prepared_entry_result {
  tes4_prepared_entry entry;
  std::uint32_t file_flags{0};
};

struct prepared_folder_group {
  std::string display_name;
  std::vector<tes4_prepared_entry> entries;
};

std::string stored_tes4_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  // TES4 BSA folder records and embedded-name prefixes use Bethesda-style
  // backslashes even though public lookup keys normalize both separator forms.
  std::replace(preserved.begin(), preserved.end(), '/', '\\');
  return preserved;
}

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds uint32_t limits"};
  }
  return static_cast<std::uint32_t>(value);
}

result<std::uint32_t> checked_size_flags_payload_size(std::uint64_t value, std::string_view description) {
  if (value > (std::numeric_limits<std::uint32_t>::max() & ~tes4_bsa_file_size_compression_toggle)) {
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

bool is_dx9_bsa_texture_format(std::uint32_t dxgi_format) noexcept {
  switch (dxgi_format) {
  case 28U: // DXGI_FORMAT_R8G8B8A8_UNORM, used as a D3D9-era 32-bit color equivalent.
  case 61U: // DXGI_FORMAT_R8_UNORM, used as an L8-style single-channel equivalent.
  case 65U: // DXGI_FORMAT_A8_UNORM.
  case 71U: // DXGI_FORMAT_BC1_UNORM / DXT1.
  case 74U: // DXGI_FORMAT_BC2_UNORM / DXT3.
  case 77U: // DXGI_FORMAT_BC3_UNORM / DXT5.
  case 87U: // DXGI_FORMAT_B8G8R8A8_UNORM.
  case 88U: // DXGI_FORMAT_B8G8R8X8_UNORM.
    return true;
  default:
    return false;
  }
}

bool is_fallout4_compatible_bsa_texture_format(std::uint32_t dxgi_format) noexcept {
  if (is_dx9_bsa_texture_format(dxgi_format)) {
    return true;
  }

  switch (dxgi_format) {
  case 80U: // DXGI_FORMAT_BC4_UNORM.
  case 83U: // DXGI_FORMAT_BC5_UNORM.
  case 98U: // DXGI_FORMAT_BC7_UNORM.
    return true;
  default:
    return false;
  }
}

result<void> validate_bsa_texture_format_for_target(tes4_bsa_target target, std::uint32_t dxgi_format) {
  switch (target) {
  case tes4_bsa_target::oblivion:
  case tes4_bsa_target::fallout3:
    if (!is_dx9_bsa_texture_format(dxgi_format)) {
      return error{error_code::format_error,
                   "TES4-family BSA target supports only DX9 DDS texture formats before Skyrim SE"};
    }
    return {};
  case tes4_bsa_target::skyrim_se:
    if (!is_fallout4_compatible_bsa_texture_format(dxgi_format)) {
      return error{error_code::format_error,
                   "Skyrim SE BSA target supports the same DDS texture format set as Fallout 4"};
    }
    return {};
  }
  return error{error_code::invalid_argument, "TES4 BSA writer target profile is not supported"};
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
    return tes4_bsa_file_flag_meshes;
  }
  if (extension == ".dds") {
    return tes4_bsa_file_flag_textures;
  }
  if (extension == ".wav") {
    return tes4_bsa_file_flag_sounds;
  }
  if (extension == ".pex" || extension == ".psc") {
    return tes4_bsa_file_flag_scripts;
  }
  if (extension == ".xml") {
    return version == tes4_bsa_oblivion_version ? tes4_bsa_file_flag_menus : 0U;
  }
  if (extension == ".txt" || extension == ".html" || extension == ".bat" || extension == ".scc") {
    return version == tes4_bsa_skyrim_se_version ? 0U : tes4_bsa_file_flag_misc;
  }
  return 0U;
}

constexpr detail::writer_disk_source_context tes4_prepare_source_context{
    "TES4 BSA writer failed to open disk source",
    "TES4 BSA writer failed to inspect disk source",
    "TES4 BSA writer failed while reading disk source",
    "TES4 BSA disk source changed during finalization",
    "TES4 BSA disk source"};

result<std::vector<std::byte>> read_source_bytes(const tes4_writer_entry& entry, std::uint32_t expected_size) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }
  return detail::read_disk_source_exact(entry.host_path, expected_size, tes4_prepare_source_context);
}

result<void> validate_parseable_dds_texture_for_target(const tes4_writer_entry& entry,
                                                       std::string_view file_extension,
                                                       tes4_bsa_target target) {
  if (file_extension != ".dds") {
    return {};
  }

  auto probe = entry.from_memory ? result<std::vector<std::byte>>{entry.memory_bytes}
                                 : detail::read_disk_source_prefix(entry.host_path,
                                                                   dds_metadata_probe_size,
                                                                   tes4_prepare_source_context);
  if (!probe) {
    return probe.error();
  }

  auto metadata = texture::analyze_dds_metadata(probe.value());
  if (!metadata) {
    // The generic BSA writer can store arbitrary payloads under .dds paths. Only parseable DDS
    // metadata is target-gated so malformed or synthetic test bytes keep their container behavior.
    return {};
  }

  return validate_bsa_texture_format_for_target(target, metadata.value().dxgi_format);
}

result<std::uint32_t> disk_payload_size(const std::string& host_path) {
  auto size = detail::inspect_disk_source_size(host_path, tes4_prepare_source_context);
  if (!size) {
    return size.error();
  }
  return checked_u32(size.value(), "TES4 BSA disk source size");
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
  auto [canonical_folder, canonical_file_name] = split_folder_file(entry.archive_path_canonical);
  if (folder.empty() || file_name.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA writer archive paths must include folder and file names"};
  }
  if (canonical_folder.empty() || canonical_file_name.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA writer archive paths must include canonical folders and files"};
  }

  const auto entry_extension = extension_of(file_name);
  const auto entry_file_flags = file_flag_for_extension(entry_extension, version);
  auto texture_format = validate_parseable_dds_texture_for_target(entry, entry_extension, target);
  if (!texture_format) {
    return texture_format.error();
  }
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
    record_flags |= tes4_bsa_file_size_compression_toggle;
  }

  tes4_prepared_entry prepared;
  prepared.folder = folder;
  prepared.canonical_folder = std::move(canonical_folder);
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

  auto payload = read_source_bytes(entry, raw_size);
  if (!payload) {
    return payload.error();
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

} // namespace

result<tes4_writer_entry> tes4_make_writer_entry(std::string_view archive_path,
                                                 entry_compression_policy compression) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  tes4_writer_entry entry;
  entry.archive_path_original = stored_tes4_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.compression = compression;
  return entry;
}

result<std::uint32_t> tes4_version_for(tes4_bsa_target target) {
  switch (target) {
  case tes4_bsa_target::oblivion:
    return tes4_bsa_oblivion_version;
  case tes4_bsa_target::fallout3:
    return tes4_bsa_fallout3_version;
  case tes4_bsa_target::skyrim_se:
    return tes4_bsa_skyrim_se_version;
  }
  return error{error_code::invalid_argument, "TES4 BSA writer target profile is not supported"};
}

bool tes4_archive_default_compressed(tes4_bsa_target target, archive_compression_policy policy) noexcept {
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

bool tes4_should_emit_embedded_names(const tes4_bsa_writer_options& options, std::uint32_t version) noexcept {
  return options.embed_file_names && version != tes4_bsa_oblivion_version;
}

result<void> tes4_validate_entries(std::span<const tes4_writer_entry> entries) {
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

result<std::vector<tes4_prepared_folder>> tes4_prepare_folders(std::span<const tes4_writer_entry> entries,
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

  std::map<std::string, prepared_folder_group> grouped;
  file_flags = 0U;
  for (auto& prepared : prepared_by_index) {
    if (!prepared.has_value()) {
      return error{error_code::io_error, "TES4 BSA writer failed to prepare an entry"};
    }
    file_flags |= prepared->file_flags;
    // TES4 folder hashes fold ASCII case, so mixed-case spellings of the
    // same virtual folder must serialize as one folder record.
    auto [group, inserted] = grouped.try_emplace(prepared->entry.canonical_folder);
    if (inserted) {
      group->second.display_name = prepared->entry.folder;
    }
    group->second.entries.push_back(std::move(prepared->entry));
  }

  std::vector<tes4_prepared_folder> folders;
  folders.reserve(grouped.size());
  for (auto& [canonical_folder, folder_group] : grouped) {
    (void)canonical_folder;
    auto& folder_entries = folder_group.entries;
    std::sort(folder_entries.begin(), folder_entries.end(), [](const tes4_prepared_entry& lhs,
                                                               const tes4_prepared_entry& rhs) {
      return lhs.file_hash < rhs.file_hash;
    });
    folders.push_back(tes4_prepared_folder{folder_group.display_name,
                                           detail::hash_tes4(folder_group.display_name, {}),
                                           0U,
                                           std::move(folder_entries)});
  }
  std::sort(folders.begin(), folders.end(), [](const tes4_prepared_folder& lhs,
                                               const tes4_prepared_folder& rhs) {
    return lhs.hash < rhs.hash;
  });
  return folders;
}

} // namespace libbsa::formats::bsa
