#pragma once

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

/// Returns the TES4-family BSA archive version for a target profile.
result<std::uint32_t> tes4_version_for(tes4_bsa_target target);

/// Returns whether the target and policy make new entries compressed by
/// default.
bool tes4_archive_default_compressed(tes4_bsa_target target,
                                     archive_compression_policy policy) noexcept;

/// Returns whether file-name prefixes should be embedded in stored payloads for
/// this target version.
bool tes4_should_emit_embedded_names(const tes4_bsa_writer_options& options,
                                     std::uint32_t version) noexcept;

/// Validates TES4 BSA writer entries before source preparation.
result<void> tes4_validate_entries(std::span<const tes4_writer_entry> entries);

/// Prepares TES4 BSA folders by validating sources, routing compression,
/// grouping, hashing, and sorting records.
result<std::vector<tes4_prepared_folder>> tes4_prepare_folders(
    std::span<const tes4_writer_entry> entries, tes4_bsa_target target,
    bool archive_default_is_compressed, bool emit_embedded_names, std::uint32_t version,
    std::uint32_t worker_count, std::uint32_t& file_flags);

}  // namespace libbsa::formats::bsa
