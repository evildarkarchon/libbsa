#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/compression_router.hpp>
#include <detail/host_file.hpp>
#include <detail/parallel_work.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <unordered_set>
#include <utility>

namespace libbsa::formats::ba2 {

namespace {

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
    }
    return static_cast<std::uint32_t>(value);
}

constexpr detail::host_file_context ba2_gnrl_prepare_source_context{
    "BA2 GNRL writer failed to open disk source",
    "BA2 GNRL writer failed to inspect disk source size",
    "BA2 GNRL writer failed while reading disk source",
    "BA2 GNRL disk source changed during preparation", "BA2 GNRL disk source"};

/// Resolves BA2 GNRL disk sources once so writer preparation matches the shared
/// host-file boundary policy.
result<detail::host_file_path> resolve_ba2_gnrl_source_path(std::string_view host_path) {
    return detail::resolve_host_file_path(host_path);
}

result<std::vector<std::byte>> read_source_bytes(const ba2_gnrl_writer_entry& entry,
                                                 std::uint64_t expected_size) {
    if (entry.from_memory) {
        return entry.memory_bytes;
    }
    auto source_path = resolve_ba2_gnrl_source_path(entry.host_path);
    if (!source_path) {
        return source_path.error();
    }
    return detail::read_host_file_exact(source_path.value(), expected_size,
                                        ba2_gnrl_prepare_source_context);
}

result<std::uint64_t> disk_file_size(const std::string& host_path) {
    auto source_path = resolve_ba2_gnrl_source_path(host_path);
    if (!source_path) {
        return source_path.error();
    }
    auto size =
        detail::inspect_host_file_size(source_path.value(), ba2_gnrl_prepare_source_context);
    if (!size) {
        return size.error();
    }
    return size.value();
}

std::uint64_t hash_bytes(std::span<const std::byte> bytes) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto byte : bytes) {
        hash ^= std::to_integer<std::uint8_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

result<std::uint64_t> hash_disk_payload(const std::string& host_path, std::uint64_t expected_size) {
    std::uint64_t hash = 14695981039346656037ULL;
    auto source_path = resolve_ba2_gnrl_source_path(host_path);
    if (!source_path) {
        return source_path.error();
    }
    auto hashed = detail::for_each_host_file_chunk(
        source_path.value(), expected_size, ba2_gnrl_prepare_source_context,
        [&](std::span<const std::byte> chunk) -> result<void> {
            for (const auto byte : chunk) {
                hash ^= std::to_integer<std::uint8_t>(byte);
                hash *= 1099511628211ULL;
            }
            return {};
        });
    if (!hashed) {
        return hashed.error();
    }
    return hash;
}

std::uint64_t ba2_gnrl_final_stored_dedupe_hash(std::uint64_t payload_hash) noexcept {
    return payload_hash;
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

bool requested_entry_compression(bool archive_compressed,
                                 entry_compression_policy policy) noexcept {
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

result<ba2_gnrl_prepared_entry> prepare_entry(const ba2_profile& profile,
                                              const ba2_gnrl_writer_options& options,
                                              const ba2_gnrl_writer_entry& entry) {
    const bool archive_compressed = archive_default_compressed(options.compression);
    std::vector<std::byte> stored_payload;
    std::uint64_t source_size = entry.from_memory ? entry.memory_bytes.size() : 0U;
    if (!entry.from_memory) {
        auto disk_size = disk_file_size(entry.host_path);
        if (!disk_size) {
            return disk_size.error();
        }
        source_size = disk_size.value();
    }

    const bool entry_compressed =
        source_size != 0U &&
        requested_entry_compression(archive_compressed, entry.options.compression);
    auto raw_size = checked_u32(source_size, "BA2 GNRL raw payload size");
    if (!raw_size) {
        return raw_size.error();
    }

    std::uint32_t packed_size = ba2_packed_size_raw;
    bool stream_from_disk = !entry.from_memory && !entry_compressed;
    std::uint64_t payload_hash = 0U;
    std::uint64_t final_stored_dedupe_hash = 0U;
    if (entry_compressed || entry.from_memory) {
        auto payload = read_source_bytes(entry, source_size);
        if (!payload) {
            return payload.error();
        }
        if (entry_compressed) {
            auto compressed =
                detail::compress_payload(profile.compressed_payload_method(), payload.value());
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
        payload_hash = hash_bytes(stored_payload);
    } else {
        auto hash = hash_disk_payload(entry.host_path, source_size);
        if (!hash) {
            return hash.error();
        }
        payload_hash = hash.value();
    }

    final_stored_dedupe_hash = ba2_gnrl_final_stored_dedupe_hash(payload_hash);

    auto identity = make_ba2_record_identity(
        ba2_subtype::gnrl,
        ba2_record_path{entry.archive_path_original, entry.archive_path_canonical},
        ba2_record_identity_source::writer_entry);
    if (!identity) {
        return identity.error();
    }

    detail::host_file_path resolved_source_path;
    if (stream_from_disk) {
        auto source_path = resolve_ba2_gnrl_source_path(entry.host_path);
        if (!source_path) {
            return source_path.error();
        }
        resolved_source_path = std::move(source_path.value());
    }

    return ba2_gnrl_prepared_entry{identity.value().display_path,
                                   identity.value().canonical_path,
                                   entry.host_path,
                                   std::move(resolved_source_path),
                                   identity.value().extension,
                                   identity.value().name_hash,
                                   identity.value().directory_hash,
                                   entry.options.record_flags.value_or(0U),
                                   0U,
                                   packed_size,
                                   raw_size.value(),
                                   payload_hash,
                                   final_stored_dedupe_hash,
                                   stream_from_disk,
                                   true,
                                   std::move(stored_payload)};
}

}  // namespace

result<ba2_gnrl_writer_entry> ba2_gnrl_make_writer_entry(std::string_view archive_path,
                                                         ba2_gnrl_entry_options options) {
    auto path = resolve_ba2_record_path(ba2_subtype::gnrl, archive_path,
                                        ba2_record_identity_source::writer_entry);
    if (!path) {
        return path.error();
    }

    ba2_gnrl_writer_entry entry;
    entry.archive_path_original = std::move(path.value().display_path);
    entry.archive_path_canonical = std::move(path.value().canonical_path);
    entry.options = options;
    return entry;
}

result<void> ba2_gnrl_validate_entries(std::span<const ba2_gnrl_writer_entry> entries) {
    if (entries.empty()) {
        return error{error_code::invalid_argument,
                     "BA2 GNRL writer requires at least one file entry"};
    }

    std::unordered_set<std::string> canonical_paths;
    for (const auto& entry : entries) {
        if (!canonical_paths.insert(entry.archive_path_canonical).second) {
            return error{error_code::format_error,
                         "BA2 GNRL writer has duplicate canonical archive paths"};
        }

        auto identity = make_ba2_record_identity(
            ba2_subtype::gnrl,
            ba2_record_path{entry.archive_path_original, entry.archive_path_canonical},
            ba2_record_identity_source::writer_entry);
        if (!identity) {
            return identity.error();
        }
    }

    return {};
}

result<std::vector<ba2_gnrl_prepared_entry>> ba2_gnrl_prepare_entries(
    const ba2_profile& profile, const ba2_gnrl_writer_options& options,
    std::span<const ba2_gnrl_writer_entry> entries, std::uint32_t worker_count) {
    if (!profile.is_gnrl()) {
        return error{error_code::invalid_argument, "BA2 GNRL writer profile is not GNRL"};
    }

    std::vector<std::optional<ba2_gnrl_prepared_entry>> prepared_by_index(entries.size());
    auto work = [&](std::size_t index) -> result<void> {
        auto prepared = prepare_entry(profile, options, entries[index]);
        if (!prepared) {
            return prepared.error();
        }
        prepared_by_index[index] = std::move(prepared.value());
        return {};
    };

    auto prepared_work = detail::run_indexed_work(entries.size(), worker_count, work);
    if (!prepared_work) {
        return prepared_work.error();
    }

    std::vector<ba2_gnrl_prepared_entry> prepared;
    prepared.reserve(entries.size());
    for (auto& entry : prepared_by_index) {
        if (!entry.has_value()) {
            return error{error_code::io_error, "BA2 GNRL worker did not prepare an entry"};
        }
        prepared.push_back(std::move(entry.value()));
    }

    // D-17 keeps Phase 8 deterministic with a canonical-path fallback because
    // traced BA2 writer evidence does not prove a stricter hash sort requirement.
    // Records and final filename-table entries stay paired by this order.
    std::sort(prepared.begin(), prepared.end(),
              [](const ba2_gnrl_prepared_entry& lhs, const ba2_gnrl_prepared_entry& rhs) {
                  return lhs.archive_path_canonical < rhs.archive_path_canonical;
              });
    return prepared;
}

}  // namespace libbsa::formats::ba2
