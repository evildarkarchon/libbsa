#pragma once

#include "formats/ba2/ba2_gnrl_writer.hpp"

#include <detail/host_file_path.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

struct ba2_gnrl_prepared_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::string source_path;
  // Raw finalization must reuse the prepare-time resolved path so Windows UTF-8 host text is not reinterpreted later.
  detail::host_file_path resolved_source_path;
  std::array<std::byte, 4> extension{};
  std::uint32_t name_hash{};
  std::uint32_t directory_hash{};
  std::uint32_t record_flags{};
  std::uint64_t payload_offset{};
  std::uint32_t packed_size{};
  std::uint32_t raw_size{};
  std::uint64_t payload_hash{};
  // Dedupe narrowing uses a deliberately named final-stored fingerprint so policy checks can distinguish it
  // from caller-source hashes; exact equality in layout remains the authority for shared offsets.
  std::uint64_t final_stored_dedupe_hash{};
  bool stream_from_disk{false};
  bool owns_payload_bytes{true};
  std::vector<std::byte> stored_payload;
};

/// Builds the internal writer entry used by public BA2 GNRL add_file/add_bytes calls.
result<ba2_gnrl_writer_entry> ba2_gnrl_make_writer_entry(std::string_view archive_path,
                                                         ba2_gnrl_entry_options options);

/// Validates BA2 GNRL target-specific writer options before preparation.
result<void> ba2_gnrl_validate_target_options(ba2_gnrl_target target, const ba2_gnrl_writer_options& options);

/// Validates BA2 GNRL writer entries before source preparation.
result<void> ba2_gnrl_validate_entries(std::span<const ba2_gnrl_writer_entry> entries);

/// Prepares BA2 GNRL entries by routing compression, hashing payloads, and sorting records deterministically.
result<std::vector<ba2_gnrl_prepared_entry>> ba2_gnrl_prepare_entries(ba2_gnrl_target target,
                                                                      const ba2_gnrl_writer_options& options,
                                                                      std::span<const ba2_gnrl_writer_entry> entries,
                                                                      std::uint32_t worker_count);

} // namespace libbsa::formats::ba2
