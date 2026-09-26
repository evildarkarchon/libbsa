#pragma once

#include "formats/bsa/tes4_bsa_profile.hpp"
#include "formats/bsa/tes4_bsa_writer.hpp"

#include <detail/stored_payload.hpp>
#include <detail/writer_publish.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::bsa {

struct tes4_prepared_entry {
    /// Creates one fully finalized entry whose Stored Payload is ready for placement.
    tes4_prepared_entry(std::string folder_value, std::string canonical_folder_value,
                        std::string file_name_value, std::uint64_t file_hash_value,
                        std::uint32_t record_flags_value,
                        detail::stored_payload stored_payload_value) noexcept
        : folder{std::move(folder_value)},
          canonical_folder{std::move(canonical_folder_value)},
          file_name{std::move(file_name_value)},
          file_hash{file_hash_value},
          record_flags{record_flags_value},
          payload{std::move(stored_payload_value)} {}

    std::string folder;
    std::string canonical_folder;
    std::string file_name;
    std::uint64_t file_hash{0};
    std::uint32_t record_flags{0};
    detail::stored_payload payload;
};

struct tes4_prepared_folder {
    std::string name;
    std::uint64_t hash{0};
    std::vector<tes4_prepared_entry> entries;
};

/// Builds the internal writer entry used by public TES4 BSA add_file/add_bytes
/// calls.
result<tes4_writer_entry> tes4_make_writer_entry(std::string_view archive_path,
                                                 entry_compression_policy compression);

/// Validates TES4 BSA writer entry structure without opening disk sources.
///
/// Source paths are authoritatively resolved and opened only after the
/// Finalization Workspace has been reserved.
result<void> tes4_validate_entries(std::span<const tes4_writer_entry> entries);

/// Prepares TES4 BSA folders by validating sources, routing compression,
/// grouping, hashing, and sorting records.
///
/// \param entries Read-only staged entries whose sources are prepared independently.
/// \param profile Profile resolved once at writer finalization; it owns
/// compression, embedded-name, file-classification, and analyzed-texture
/// compatibility policy.
/// \param options Per-archive compression and embedded-name requests.
/// \param worker_count Positive number of parallel preparation workers. Results
/// are joined by entry index before deterministic grouping and sorting.
/// \param file_flags Receives the aggregate serialized file-classification mask.
/// \param workspace Owns deterministic raw-source snapshots through publication.
/// \return Prepared folders or the first source, format, or compression error.
result<std::vector<tes4_prepared_folder>> tes4_prepare_folders(
    std::span<const tes4_writer_entry> entries, const tes4_bsa_profile& profile,
    const tes4_bsa_writer_options& options, std::uint32_t worker_count, std::uint32_t& file_flags,
    const detail::finalization_workspace& workspace);

}  // namespace libbsa::formats::bsa
