#pragma once

#include "formats/bsa/tes3_bsa_writer.hpp"

#include <detail/host_file_path.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

struct tes3_prepared_entry {
    std::string archive_path_original;
    std::vector<std::byte> payload;
    std::string host_path;
    detail::host_file_path resolved_host_path;
    std::uint64_t hash{0};
    /// Data-section-relative payload offset as assigned by Payload Placement.
    ///
    /// Held at the module's cursor width. The UInt32 the file record carries is
    /// narrowed by serialization, when the record is written.
    std::uint64_t raw_offset{0};
    std::uint32_t payload_size{0};
    bool from_memory{false};
};

/// Builds the internal writer entry used by public TES3 add_file/add_bytes
/// calls.
result<tes3_writer_entry> tes3_make_writer_entry(std::string_view archive_path);

/// Validates a TES3 writer host path with the existing format-specific
/// diagnostics.
result<void> tes3_validate_host_path(std::string_view host_path, std::string_view description);

/// Validates the TES3 entry list before preparing payload metadata.
result<void> tes3_validate_entries(std::span<const tes3_writer_entry> entries);

/// Prepares TES3 writer entries by sizing payloads, preserving serialized
/// names, and sorting by TES3 hash order.
result<std::vector<tes3_prepared_entry>> tes3_prepare_entries(
    std::span<const tes3_writer_entry> entries);

}  // namespace libbsa::formats::bsa
