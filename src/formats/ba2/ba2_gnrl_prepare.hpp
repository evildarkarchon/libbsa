#pragma once

#include "formats/ba2/ba2_gnrl_writer.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <detail/stored_payload.hpp>
#include <detail/writer_publish.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {

struct ba2_gnrl_prepared_entry {
    /// Creates one finalized GNRL record whose Stored Payload is ready for layout.
    ba2_gnrl_prepared_entry(std::string archive_path_original_value,
                            std::string archive_path_canonical_value,
                            std::array<std::byte, 4> extension_value, std::uint32_t name_hash_value,
                            std::uint32_t directory_hash_value, std::uint32_t record_flags_value,
                            std::uint32_t packed_size_value, std::uint32_t raw_size_value,
                            detail::stored_payload payload_value) noexcept
        : archive_path_original{std::move(archive_path_original_value)},
          archive_path_canonical{std::move(archive_path_canonical_value)},
          extension{extension_value},
          name_hash{name_hash_value},
          directory_hash{directory_hash_value},
          record_flags{record_flags_value},
          packed_size{packed_size_value},
          raw_size{raw_size_value},
          payload{std::move(payload_value)} {}

    std::string archive_path_original;
    std::string archive_path_canonical;
    std::array<std::byte, 4> extension{};
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::uint32_t record_flags{};
    std::uint64_t payload_offset{};
    std::uint32_t packed_size{};
    std::uint32_t raw_size{};
    // This temporary expansion-stage marker describes only physical placement;
    // Stored Payload owns its bytes regardless of whether another record reuses
    // the representative's offset.
    bool is_payload_representative{true};
    detail::stored_payload payload;
};

/// Builds the internal writer entry used by public BA2 GNRL add_file/add_bytes
/// calls.
result<ba2_gnrl_writer_entry> ba2_gnrl_make_writer_entry(std::string_view archive_path,
                                                         ba2_gnrl_entry_options options);

/// Validates BA2 GNRL writer entry structure without opening disk sources.
///
/// Source paths are authoritatively resolved and opened only after the
/// Finalization Workspace has been reserved.
result<void> ba2_gnrl_validate_entries(std::span<const ba2_gnrl_writer_entry> entries);

/// Prepares BA2 GNRL entries into owned Stored Payloads and sorts them deterministically.
///
/// Each disk worker opens one stable source session. Compressed sources are
/// read through that session into owned final bytes, while raw sources are
/// copied through bounded memory into deterministic workspace snapshots.
///
/// \param profile Resolved GNRL profile that owns compression-method routing.
/// \param options Archive and per-write behavior, including default compression.
/// \param entries Read-only staged disk or memory sources.
/// \param worker_count Positive indexed-worker concurrency bound.
/// \param workspace Owns raw disk snapshots through layout and serialization.
/// \return Prepared records or the first source, compression, or snapshot error.
result<std::vector<ba2_gnrl_prepared_entry>> ba2_gnrl_prepare_entries(
    const ba2_profile& profile, const ba2_gnrl_writer_options& options,
    std::span<const ba2_gnrl_writer_entry> entries, std::uint32_t worker_count,
    const detail::finalization_workspace& workspace);

}  // namespace libbsa::formats::ba2
