#include "formats/bsa/tes4_bsa_prepare.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/compression_router.hpp>
#include <detail/host_file.hpp>
#include <detail/parallel_work.hpp>

#include "texture/directxtex_analyzer.hpp"

#include <algorithm>
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
        return error{error_code::format_error,
                     std::string{description} + " exceeds uint32_t limits"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint32_t> checked_size_flags_payload_size(std::uint64_t value,
                                                      std::string_view description) {
    if (value >
        (std::numeric_limits<std::uint32_t>::max() & ~tes4_bsa_file_size_compression_toggle)) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds TES4 BSA size-flag limits"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint8_t> checked_name_size(std::size_t size, std::string_view description) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds BSA name length limits"};
    }
    return static_cast<std::uint8_t>(size);
}

std::uint64_t file_hash_for(std::string_view file_name) {
    const auto dot = file_name.find_last_of('.');
    if (dot == std::string_view::npos) {
        return detail::hash_tes4(file_name, {});
    }
    return detail::hash_tes4(file_name.substr(0, dot), file_name.substr(dot));
}

constexpr detail::host_file_context tes4_prepare_source_context{
    "TES4 BSA writer failed to open disk source", "TES4 BSA writer failed to inspect disk source",
    "TES4 BSA writer failed while reading disk source",
    "TES4 BSA disk source changed during finalization", "TES4 BSA disk source"};

/// Resolves a TES4 writer disk source once so writer seams share the same
/// host-file contract as later reader work.
result<detail::host_file_path> resolve_tes4_source_path(std::string_view host_path) {
    return detail::resolve_host_file_path(host_path);
}

result<std::vector<std::byte>> read_source_bytes(const tes4_writer_entry& entry,
                                                 std::uint32_t expected_size) {
    if (entry.from_memory) {
        return entry.memory_bytes;
    }
    auto source_path = resolve_tes4_source_path(entry.host_path);
    if (!source_path) {
        return source_path.error();
    }
    return detail::read_host_file_exact(source_path.value(), expected_size,
                                        tes4_prepare_source_context);
}

/// Analyzes texture-classified source bytes and delegates compatibility of the
/// resulting libbsa-native metadata to the resolved TES4 BSA Profile.
result<void> validate_parseable_dds_texture(const tes4_writer_entry& entry,
                                            std::uint32_t file_flags,
                                            const tes4_bsa_profile& profile) {
    if ((file_flags & tes4_bsa_file_flag_textures) == 0U) {
        return {};
    }

    result<std::vector<std::byte>> probe = entry.memory_bytes;
    if (!entry.from_memory) {
        auto source_path = resolve_tes4_source_path(entry.host_path);
        if (!source_path) {
            return source_path.error();
        }
        probe = detail::read_host_file_prefix(source_path.value(), dds_metadata_probe_size,
                                              tes4_prepare_source_context);
    }
    if (!probe) {
        return probe.error();
    }

    auto metadata = texture::analyze_dds_metadata(probe.value());
    if (!metadata) {
        // The generic BSA writer can store arbitrary payloads under .dds paths.
        // Only parseable DDS metadata is target-gated so malformed or synthetic
        // test bytes keep their container behavior.
        return {};
    }

    return profile.validate_texture_metadata(metadata.value());
}

result<std::uint32_t> disk_payload_size(const std::string& host_path) {
    auto source_path = resolve_tes4_source_path(host_path);
    if (!source_path) {
        return source_path.error();
    }
    auto size = detail::inspect_host_file_size(source_path.value(), tes4_prepare_source_context);
    if (!size) {
        return size.error();
    }
    return checked_u32(size.value(), "TES4 BSA disk source size");
}

void append_u32_le(std::vector<std::byte>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xFFU));
    bytes.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
    bytes.push_back(static_cast<std::byte>((value >> 16U) & 0xFFU));
    bytes.push_back(static_cast<std::byte>((value >> 24U) & 0xFFU));
}

result<std::vector<std::byte>> make_embedded_name_prefix(bool emit_embedded_name,
                                                         std::string_view file_name) {
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

/// Encodes one memory-backed payload using the profile-selected codec and
/// archive-specific embedded-name choice.
result<std::vector<std::byte>> encode_stored_payload(const tes4_bsa_profile& profile,
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
    auto compressed = detail::compress_payload(profile.compressed_payload_method(), raw_payload);
    if (!compressed) {
        return compressed.error();
    }

    stored.value().reserve(stored.value().size() + 4U + compressed.value().size());
    append_u32_le(stored.value(), raw_size.value());
    stored.value().insert(stored.value().end(), compressed.value().begin(),
                          compressed.value().end());
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

/// Prepares one entry while leaving source probing and DDS analysis in the
/// preparation stage and delegating version-dependent policy to the TES4 BSA Profile.
result<prepared_entry_result> prepare_one_entry(const tes4_writer_entry& entry,
                                                const tes4_bsa_profile& profile,
                                                const tes4_bsa_writer_options& options) {
    auto [folder, file_name] = split_folder_file(entry.archive_path_original);
    auto [canonical_folder, canonical_file_name] = split_folder_file(entry.archive_path_canonical);
    if (folder.empty() || file_name.empty()) {
        return error{error_code::invalid_argument,
                     "TES4 BSA writer archive paths must include folder and file names"};
    }
    if (canonical_folder.empty() || canonical_file_name.empty()) {
        return error{error_code::invalid_argument,
                     "TES4 BSA writer archive paths must include canonical folders "
                     "and files"};
    }

    const auto entry_file_flags = profile.file_flag_for_path(entry.archive_path_original);
    auto texture_format = validate_parseable_dds_texture(entry, entry_file_flags, profile);
    if (!texture_format) {
        return texture_format.error();
    }
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

    const auto compression =
        profile.writer_entry_compression(options.compression_policy, entry.compression, raw_size);
    const bool effective_compressed = compression.compression != entry_compression::none;
    const bool emit_embedded_names = profile.writer_emits_embedded_names(options);

    tes4_prepared_entry prepared;
    prepared.folder = folder;
    prepared.canonical_folder = std::move(canonical_folder);
    prepared.file_name = std::move(file_name);
    prepared.file_hash = file_hash_for(prepared.file_name);
    prepared.record_flags = compression.record_flags;
    const auto embedded_name = join_folder_file(prepared.folder, prepared.file_name);

    if (!entry.from_memory && !effective_compressed) {
        auto prefix = make_embedded_name_prefix(emit_embedded_names, embedded_name);
        if (!prefix) {
            return prefix.error();
        }
        auto source_path = resolve_tes4_source_path(entry.host_path);
        if (!source_path) {
            return source_path.error();
        }
        const auto stored_size64 = static_cast<std::uint64_t>(prefix.value().size()) + raw_size;
        auto stored_size =
            checked_size_flags_payload_size(stored_size64, "TES4 BSA stored payload size");
        if (!stored_size) {
            return stored_size.error();
        }
        prepared.stored_size = stored_size.value();
        prepared.stored_payload = std::move(prefix.value());
        prepared.raw_disk_host_path = entry.host_path;
        prepared.resolved_raw_disk_host_path = std::move(source_path.value());
        prepared.raw_disk_size = raw_size;
        prepared.stream_raw_disk = true;
        return prepared_entry_result{std::move(prepared), entry_file_flags};
    }

    auto payload = read_source_bytes(entry, raw_size);
    if (!payload) {
        return payload.error();
    }

    auto stored_payload = encode_stored_payload(profile, payload.value(), effective_compressed,
                                                emit_embedded_names, embedded_name);
    if (!stored_payload) {
        return stored_payload.error();
    }
    auto stored_size = checked_size_flags_payload_size(stored_payload.value().size(),
                                                       "TES4 BSA stored payload size");
    if (!stored_size) {
        return stored_size.error();
    }
    prepared.stored_size = stored_size.value();
    prepared.stored_payload = std::move(stored_payload.value());
    return prepared_entry_result{std::move(prepared), entry_file_flags};
}

}  // namespace

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

result<void> tes4_validate_entries(std::span<const tes4_writer_entry> entries) {
    if (entries.empty()) {
        return error{error_code::invalid_argument,
                     "TES4 BSA writer requires at least one file entry"};
    }

    std::unordered_set<std::string> canonical_paths;
    for (const auto& entry : entries) {
        if (!canonical_paths.insert(entry.archive_path_canonical).second) {
            return error{error_code::format_error,
                         "TES4 BSA writer has duplicate canonical archive paths"};
        }

        if (!entry.from_memory) {
            auto source_path = resolve_tes4_source_path(entry.host_path);
            if (!source_path) {
                return source_path.error();
            }
            auto input = detail::open_host_file(source_path.value(), tes4_prepare_source_context);
            if (!input) {
                return error{error_code::io_error, "TES4 BSA writer failed to open disk source"};
            }
        }
    }

    return {};
}

result<std::vector<tes4_prepared_folder>> tes4_prepare_folders(
    std::span<const tes4_writer_entry> entries, const tes4_bsa_profile& profile,
    const tes4_bsa_writer_options& options, std::uint32_t worker_count, std::uint32_t& file_flags) {
    std::vector<std::optional<prepared_entry_result>> prepared_by_index(entries.size());
    auto prepared_work = detail::run_indexed_work(
        entries.size(), worker_count, [&](std::size_t index) -> result<void> {
            auto prepared = prepare_one_entry(entries[index], profile, options);
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
        std::sort(folder_entries.begin(), folder_entries.end(),
                  [](const tes4_prepared_entry& lhs, const tes4_prepared_entry& rhs) {
                      return lhs.file_hash < rhs.file_hash;
                  });
        folders.push_back(tes4_prepared_folder{folder_group.display_name,
                                               detail::hash_tes4(folder_group.display_name, {}), 0U,
                                               std::move(folder_entries)});
    }
    std::sort(folders.begin(), folders.end(),
              [](const tes4_prepared_folder& lhs, const tes4_prepared_folder& rhs) {
                  return lhs.hash < rhs.hash;
              });
    return folders;
}

}  // namespace libbsa::formats::bsa
