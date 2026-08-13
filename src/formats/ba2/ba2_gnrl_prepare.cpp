#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/compression_router.hpp>
#include <detail/host_file.hpp>
#include <detail/parallel_work.hpp>

#include <algorithm>
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

/// Prepares one GNRL entry from one coherent source observation.
///
/// Disk-backed work opens exactly one stable session for sizing and either full
/// compression input or transfer into a workspace-owned raw snapshot.
result<ba2_gnrl_prepared_entry> prepare_entry(const ba2_profile& profile,
                                              const ba2_gnrl_writer_options& options,
                                              const ba2_gnrl_writer_entry& entry,
                                              const detail::finalization_workspace& workspace,
                                              std::size_t preparation_index) {
    const bool archive_compressed = archive_default_compressed(options.compression);
    std::uint64_t source_size = entry.from_memory ? entry.memory_bytes.size() : 0U;
    std::optional<detail::stable_host_file_session> disk_source;
    if (!entry.from_memory) {
        auto source_path = resolve_ba2_gnrl_source_path(entry.host_path);
        if (!source_path) {
            return source_path.error();
        }
        // The session denies writers and delete-capable handles until the
        // authoritative bytes have become writer-owned final state.
        auto opened = detail::stable_host_file_session::open(source_path.value(),
                                                             ba2_gnrl_prepare_source_context);
        if (!opened) {
            return opened.error();
        }
        source_size = opened.value().size();
        disk_source.emplace(std::move(opened).value());
    }

    // Compression remains format policy upstream of Stored Payload. Empty
    // entries stay raw so PackedSize == 0 retains existing BA2 semantics.
    const bool entry_compressed =
        source_size != 0U &&
        requested_entry_compression(archive_compressed, entry.options.compression);
    auto raw_size = checked_u32(source_size, "BA2 GNRL raw payload size");
    if (!raw_size) {
        return raw_size.error();
    }

    std::uint32_t packed_size = ba2_packed_size_raw;
    detail::stored_payload stored_payload =
        detail::stored_payload::from_owned_bytes(std::vector<std::byte>{});
    if (!entry.from_memory && !entry_compressed) {
        // Raw disk bytes are copied once through bounded memory; the workspace,
        // not Stored Payload, owns snapshot cleanup through publication.
        auto snapshotted = detail::stored_payload::from_workspace_snapshot(
            {}, std::move(*disk_source), workspace, preparation_index);
        if (!snapshotted) {
            return snapshotted.error();
        }
        stored_payload = std::move(snapshotted).value();
    } else {
        std::span<const std::byte> raw_payload = entry.memory_bytes;
        std::vector<std::byte> disk_payload;
        if (!entry.from_memory) {
            auto read = disk_source->read_exact(source_size);
            if (!read) {
                return read.error();
            }
            disk_payload = std::move(read).value();
            raw_payload = disk_payload;
        }

        std::vector<std::byte> final_bytes;
        if (entry_compressed) {
            auto compressed =
                detail::compress_payload(profile.compressed_payload_method(), raw_payload);
            if (!compressed) {
                return compressed.error();
            }
            auto packed = checked_u32(compressed.value().size(), "BA2 GNRL packed payload size");
            if (!packed) {
                return packed.error();
            }
            packed_size = packed.value();
            final_bytes = std::move(compressed).value();
        } else {
            final_bytes.assign(raw_payload.begin(), raw_payload.end());
        }
        stored_payload = detail::stored_payload::from_owned_bytes(std::move(final_bytes));
    }

    auto identity = make_ba2_record_identity(
        ba2_subtype::gnrl,
        ba2_record_path{entry.archive_path_original, entry.archive_path_canonical},
        ba2_record_identity_source::writer_entry);
    if (!identity) {
        return identity.error();
    }

    return ba2_gnrl_prepared_entry{identity.value().stored_path,
                                   identity.value().canonical_path,
                                   identity.value().extension,
                                   identity.value().name_hash,
                                   identity.value().directory_hash,
                                   entry.options.record_flags.value_or(0U),
                                   packed_size,
                                   raw_size.value(),
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
    entry.archive_path_original = std::move(path.value().stored_path);
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
    std::span<const ba2_gnrl_writer_entry> entries, std::uint32_t worker_count,
    const detail::finalization_workspace& workspace) {
    if (!profile.is_gnrl()) {
        return error{error_code::invalid_argument, "BA2 GNRL writer profile is not GNRL"};
    }

    std::vector<std::optional<ba2_gnrl_prepared_entry>> prepared_by_index(entries.size());
    auto work = [&](std::size_t index) -> result<void> {
        auto prepared = prepare_entry(profile, options, entries[index], workspace, index);
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
