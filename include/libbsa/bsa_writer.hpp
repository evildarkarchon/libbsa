#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace libbsa {

/// Identifies the native BSA archive variant requested for writer planning.
///
/// Callers choose the target explicitly so planning never infers archive layout,
/// compression defaults, or table widths from filenames or host paths.
enum class bsa_write_target {
    tes3_morrowind,
    oblivion_v103,
    fo3_fnv_skyrim_le_v104,
    skyrim_se_ae_v105,
};

/// Controls BSA-specific planning behavior that applies to every entry.
///
/// Per-entry compression remains semantic through `compression_policy`; planning
/// translates it into native size flags and payload shapes for the selected BSA
/// target. Embedded names are opt-in because they become stored payload bytes.
struct bsa_write_options {
    bool archive_default_compressed{};
    bool embedded_names{};
    bool deduplicate{};
};

/// Carries one memory-backed BSA input entry.
///
/// `path` is an archive-virtual path normalized during planning. Payload bytes are
/// copied into the returned plan after compression and layout decisions are made.
struct bsa_memory_entry {
    std::string path;
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

/// Carries one disk-backed BSA input entry.
///
/// `host_path` names the caller-selected file to read during planning, while
/// `path` is the archive-virtual path stored in the BSA. Keeping these fields
/// separate prevents invalid memory/disk entry combinations in public code.
struct bsa_disk_entry {
    std::string host_path;
    std::string path;
    compression_policy compression{compression_policy::archive_default};
};

/// Describes a native BSA table or name-block region in the final stream.
///
/// Offsets are archive-absolute byte positions. Region names let tests and tools
/// distinguish header, folder table, file table, name table, and related blocks
/// without exposing private parser structures.
struct planned_bsa_table_region {
    std::string name;
    std::uint64_t offset{};
    std::uint64_t size{};
};

/// Describes a planned BSA payload data region owned by the write plan.
///
/// The stored payload includes any native prefixes required by the selected BSA
/// target. Finalization streams these bytes directly to the caller-owned sink.
struct planned_bsa_data_region {
    std::uint32_t id{};
    std::uint64_t offset{};
    std::uint64_t stored_size{};
    std::uint64_t unpacked_size{};
    compression_state compression{compression_state::unknown};
    std::vector<std::byte> stored_payload;
};

/// Describes one planned native BSA file record and its payload placement.
///
/// Hashes, flags, offsets, and stored sizes are exposed for compatibility tests
/// and dry-run inspection. `data_region_id` identifies the data region emitted by
/// finalization, including shared regions when deduplication is enabled.
struct planned_bsa_entry {
    std::string path;
    std::string folder_table_region_name;
    std::string file_table_region_name;
    std::uint64_t folder_hash{};
    std::uint64_t file_hash{};
    std::uint32_t flags{};
    std::uint64_t offset{};
    std::uint64_t size{};
    std::uint64_t stored_size{};
    std::uint32_t data_region_id{};
    compression_state compression{compression_state::unknown};
};

/// Owns a deterministic native BSA write preview and all bytes needed to emit it.
///
/// Planning owns table, payload, compression, and dedup decisions. Finalization
/// only streams already planned bytes, so no caller-owned input files or sinks are
/// retained after public writer calls return.
struct bsa_write_plan {
    bsa_write_target target{};
    bsa_write_options options{};
    std::uint32_t flags{};
    /// Planned native header and metadata table bytes emitted before payloads.
    ///
    /// Finalization writes these bytes verbatim so compatibility-critical layout
    /// decisions made during planning are not recomputed against mutable inputs.
    std::vector<std::byte> table_bytes;
    std::vector<planned_bsa_table_region> table_regions;
    std::vector<planned_bsa_data_region> data_regions;
    std::vector<planned_bsa_entry> entries;
    std::uint64_t total_size{};
};

/// Builds a BSA write plan from memory-backed entries.
///
/// Entries are normalized and copied during planning. On success, the returned
/// plan owns all native table and payload bytes required for finalization.
[[nodiscard]] result<bsa_write_plan> plan_bsa_write(bsa_write_target target,
                                                   std::span<const bsa_memory_entry> entries,
                                                   bsa_write_options options = {});

/// Builds a BSA write plan from disk-backed entries.
///
/// Each `host_path` is read during planning and paired with its archive-virtual
/// `path`; no file handles or host path state are retained by the resulting plan.
[[nodiscard]] result<bsa_write_plan> plan_bsa_write_from_disk(bsa_write_target target,
                                                             std::span<const bsa_disk_entry> entries,
                                                             bsa_write_options options = {});

/// Streams a previously planned BSA archive to a caller-owned sink.
///
/// The sink remains owned by the caller for the duration of this call. The first
/// structured sink write failure is returned unchanged.
[[nodiscard]] result<void> finalize_bsa_write(const bsa_write_plan& plan, byte_sink& sink);

} // namespace libbsa
