#pragma once

#include "formats/bsa/tes4_bsa_profile.hpp"
#include "formats/bsa/tes4_bsa_writer.hpp"

#include <detail/host_file_path.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

struct tes4_prepared_entry {
    std::string folder;
    std::string canonical_folder;
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
    // Raw finalization must reuse the prepare-time resolved path so Windows UTF-8
    // host text is not reinterpreted later.
    detail::host_file_path resolved_raw_disk_host_path;
};

struct tes4_prepared_folder {
    std::string name;
    std::uint64_t hash{0};
    std::uint64_t folder_block_offset{0};
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
/// \return Prepared folders or the first source, format, or compression error.
result<std::vector<tes4_prepared_folder>> tes4_prepare_folders(
    std::span<const tes4_writer_entry> entries, const tes4_bsa_profile& profile,
    const tes4_bsa_writer_options& options, std::uint32_t worker_count, std::uint32_t& file_flags);

}  // namespace libbsa::formats::bsa
