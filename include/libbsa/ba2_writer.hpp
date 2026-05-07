#pragma once

#include <libbsa/ba2.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace libbsa {

/// Identifies the native BA2 archive variant requested for writer planning.
///
/// Targets are intentionally variant-specific so callers never infer subtype,
/// version, or Starfield compression-method behavior from archive filenames.
enum class ba2_write_target {
    fallout4_gnrl_v1,
    fallout4_gnrl_v7,
    fallout4_gnrl_v8,
    starfield_gnrl_v2,
    starfield_gnrl_v3,
    fallout4_dx10_v1,
    fallout4_dx10_v7,
    fallout4_dx10_v8,
    starfield_dx10_v3,
};

/// Identifies the BA2 native subtype represented by a write target.
enum class ba2_write_subtype {
    gnrl,
    dx10,
};

/// Controls BA2-specific planning behavior that applies across an archive.
///
/// `starfield_v3_compression_method` models the native archive-level field for
/// Starfield v3 BA2 files. Per-entry policies still choose raw versus compressed
/// storage, but method 0 versus method 3 remains an archive-level decision.
struct ba2_write_options {
    bool archive_default_compressed{};
    bool deduplicate{};
    std::uint32_t starfield_v3_compression_method{0};
};

/// Carries one memory-backed BA2 GNRL input entry.
///
/// `path` is an archive-virtual path normalized during planning. Payload bytes
/// are copied into the returned plan after compression and layout decisions.
struct ba2_gnrl_memory_entry {
    std::string path;
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

/// Carries one disk-backed BA2 GNRL input entry.
///
/// `host_path` names the caller-selected file to read during planning, while
/// `path` is the archive-virtual path stored in the BA2. The resulting plan owns
/// the bytes and never retains the host path or file handle.
struct ba2_gnrl_disk_entry {
    std::string host_path;
    std::string path;
    compression_policy compression{compression_policy::archive_default};
};

/// Carries one memory-backed BA2 DDS input entry.
///
/// `dds_bytes` must contain an actual DDS file. Planning analyzes these bytes
/// behind the private texture boundary and stores only libbsa-owned metadata in
/// the returned preview.
struct ba2_dds_memory_entry {
    std::string path;
    std::vector<std::byte> dds_bytes;
    compression_policy compression{compression_policy::archive_default};
};

/// Carries one disk-backed BA2 DDS input entry.
///
/// The disk file is read and analyzed during planning. Finalization streams only
/// plan-owned BA2 table and payload bytes and does not reopen this host path.
struct ba2_dds_disk_entry {
    std::string host_path;
    std::string path;
    compression_policy compression{compression_policy::archive_default};
};

/// Echoes native BA2 target fields chosen by writer planning.
///
/// Consumers and tests can inspect subtype, version, header width, and native
/// compression method without parsing emitted table bytes back out of the plan.
struct ba2_native_target_preview {
    ba2_write_subtype subtype{ba2_write_subtype::gnrl};
    std::uint32_t version{};
    std::uint32_t header_size{};
    std::uint32_t compression_method{};
};

/// Describes a native BA2 table or metadata region in the final stream.
///
/// Offsets are archive-absolute byte positions so dry-run tooling and tests can
/// compare planned layout directly against emitted bytes.
struct planned_ba2_table_region {
    std::string name;
    std::uint64_t offset{};
    std::uint64_t size{};
};

/// Describes a planned BA2 payload data region owned by the write plan.
///
/// `id` is stable within one plan and may be referenced by multiple entries or
/// chunks when deduplication shares identical post-policy stored bytes.
struct planned_ba2_data_region {
    std::uint32_t id{};
    std::uint64_t offset{};
    std::uint64_t stored_size{};
    std::uint64_t unpacked_size{};
    compression_state compression{compression_state::unknown};
    std::vector<std::byte> stored_payload;
};

/// Describes one planned native BA2 GNRL file record.
///
/// Hashes, offsets, packed sizes, and unpacked sizes mirror the native record
/// fields. `data_region_id` links the record to the plan-owned payload bytes.
struct planned_ba2_gnrl_entry {
    std::string path;
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::uint64_t offset{};
    std::uint64_t packed_size{};
    std::uint64_t unpacked_size{};
    std::uint32_t data_region_id{};
    compression_state compression{compression_state::unknown};
};

/// Groups BA2 GNRL-specific preview data inside a write plan.
///
/// GNRL and DDS/DX10 layouts expose different native concepts; this section keeps
/// ordinary file record detail separate from texture and chunk previews.
struct planned_ba2_gnrl_section {
    std::uint64_t file_table_offset{};
    std::vector<planned_ba2_table_region> table_regions;
    std::vector<planned_ba2_gnrl_entry> entries;
};

/// Describes one planned BA2 DDS/DX10 chunk record and payload region.
///
/// Mip range fields are native DX10 chunk descriptors. Offsets and sizes are
/// archive-absolute and byte-count values after writer compression decisions.
struct planned_ba2_dds_chunk {
    std::uint32_t data_region_id{};
    std::uint32_t mip_level{};
    std::uint16_t start_mip{};
    std::uint16_t end_mip{};
    std::uint64_t offset{};
    std::uint64_t packed_size{};
    std::uint64_t unpacked_size{};
    compression_state compression{compression_state::unknown};
};

/// Describes one planned BA2 DDS/DX10 texture record.
///
/// Texture metadata uses libbsa-owned value types and is derived from real DDS
/// input bytes during planning rather than caller-supplied native descriptors.
struct planned_ba2_dds_texture {
    std::string path;
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    dxgi_format format{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t mip_count{};
    std::uint32_t array_size{};
    bool is_cubemap{};
    std::vector<planned_ba2_dds_chunk> chunks;
};

/// Groups BA2 DDS/DX10-specific preview data inside a write plan.
///
/// DDS previews include the native file table location plus derived texture and
/// chunk records needed by compatibility tests and dry-run consumers.
struct planned_ba2_dds_section {
    std::uint64_t file_table_offset{};
    std::vector<planned_ba2_table_region> table_regions;
    std::vector<planned_ba2_dds_texture> textures;
};

/// Owns a deterministic native BA2 write preview and all bytes needed to emit it.
///
/// Planning owns table bytes, payload bytes, target fields, compression choices,
/// and deduplication decisions. Finalization only streams already planned bytes
/// to a caller-owned sink.
struct ba2_write_plan {
    ba2_write_target target{};
    ba2_write_options options{};
    ba2_native_target_preview native{};
    std::vector<std::byte> table_bytes;
    std::vector<planned_ba2_data_region> data_regions;
    planned_ba2_gnrl_section gnrl;
    planned_ba2_dds_section dds;
    std::uint64_t total_size{};
};

/// Builds a BA2 GNRL write plan from memory-backed entries.
///
/// Entries are normalized and copied during planning. On success, the returned
/// plan owns all native GNRL table and payload bytes required for finalization.
[[nodiscard]] result<ba2_write_plan> plan_ba2_gnrl_write(ba2_write_target target,
                                                        std::span<const ba2_gnrl_memory_entry> entries,
                                                        ba2_write_options options = {});

/// Builds a BA2 GNRL write plan from disk-backed entries.
///
/// Each `host_path` is read during planning and paired with its archive-virtual
/// `path`; no file handles or host path state are retained by the plan.
[[nodiscard]] result<ba2_write_plan> plan_ba2_gnrl_write_from_disk(ba2_write_target target,
                                                                  std::span<const ba2_gnrl_disk_entry> entries,
                                                                  ba2_write_options options = {});

/// Builds a BA2 DDS/DX10 write plan from memory-backed DDS inputs.
///
/// Input DDS bytes are analyzed during planning, and the plan owns all derived
/// texture metadata, native table bytes, and stored chunk payload regions.
[[nodiscard]] result<ba2_write_plan> plan_ba2_dds_write(ba2_write_target target,
                                                       std::span<const ba2_dds_memory_entry> entries,
                                                       ba2_write_options options = {});

/// Builds a BA2 DDS/DX10 write plan from disk-backed DDS inputs.
///
/// Disk DDS files are read and analyzed during planning. Successful plans are
/// independent of caller-owned input files and can be finalized later.
[[nodiscard]] result<ba2_write_plan> plan_ba2_dds_write_from_disk(ba2_write_target target,
                                                                 std::span<const ba2_dds_disk_entry> entries,
                                                                 ba2_write_options options = {});

/// Streams a previously planned BA2 archive to a caller-owned sink.
///
/// The sink remains owned by the caller for the duration of this call. The first
/// structured sink write failure is returned unchanged.
[[nodiscard]] result<void> finalize_ba2_write(const ba2_write_plan& plan, byte_sink& sink);

} // namespace libbsa
