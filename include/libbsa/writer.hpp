#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace libbsa {

/// Describes the archive family and writer capabilities for a planning request.
///
/// The target is a libbsa-owned value type so public writer callers do not need
/// private codec, platform, or format implementation headers. The compression
/// fields carry explicit archive policy metadata used during planning instead of
/// inferring writer behavior from file extensions.
struct writer_target {
    archive_format format{};
    bool archive_default_compressed{};
    bool supports_compression{true};
    bool supports_shared_data_regions{};
    std::optional<std::uint32_t> compression_method;
};

/// Carries one in-memory archive entry supplied to writer planning.
///
/// Phase 8 writer inputs deliberately own payload bytes in memory per D-03; disk
/// traversal and source callbacks are deferred to later writer API work. `path`
/// is an archive-virtual path that planning normalizes before layout decisions.
struct writer_entry {
    std::string path;
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

/// Controls optional writer planning behavior.
///
/// Deduplication, when supported by the target, shares byte-identical post-policy
/// data regions in the planned layout. The option only affects the plan; no
/// caller-owned sink is touched until `finalize_archive_write` is invoked.
struct writer_options {
    bool deduplicate{};
};

/// Describes a planned metadata/table region in the final archive stream.
///
/// Offsets are archive-absolute byte positions in the stream that finalization
/// will write to the caller-owned sink from an already computed write plan.
struct planned_table_region {
    std::string name;
    std::uint64_t offset{};
    std::uint64_t size{};
};

/// Describes a planned stored payload region owned by a write plan.
///
/// Region IDs are stable numeric preview handles for dedup sharing; public plans
/// do not expose content digests or private implementation pointers. The stored
/// payload bytes are owned by the plan per D-09 so finalization cannot recompute
/// or drift from the previewed archive-absolute offsets and sizes.
struct planned_data_region {
    std::uint32_t id{};
    std::uint64_t offset{};
    std::uint64_t stored_size{};
    std::uint64_t unpacked_size{};
    compression_state compression{compression_state::unknown};
    std::vector<std::byte> stored_payload;
};

/// Describes one planned archive entry and its resolved data-region reference.
///
/// Offsets are archive-absolute payload offsets in the final stream. `data_region_id`
/// identifies the planned region that finalization emits, making dedup sharing
/// visible without leaking implementation-specific digests or storage objects.
struct planned_entry {
    std::string path;
    std::uint64_t size{};
    std::uint64_t stored_size{};
    std::uint64_t offset{};
    std::uint32_t data_region_id{};
    compression_state compression{compression_state::unknown};
};

/// Owns a deterministic preview of an archive write operation.
///
/// The plan-then-finalize API shape follows D-01/D-09: planning owns all layout
/// decisions and stored region bytes, while finalization only streams the planned
/// bytes to a caller-owned `byte_sink` for the duration of that operation.
struct write_plan {
    writer_target target;
    writer_options options;
    std::vector<planned_table_region> table_regions;
    std::vector<planned_data_region> data_regions;
    std::vector<planned_entry> entries;
    std::uint64_t total_size{};
};

/// Builds a deterministic write plan from caller-owned writer inputs.
///
/// Entries are copied from in-memory payload values, normalized, sorted, and
/// inspected before any sink is touched. The returned plan owns post-policy
/// stored payload bytes so finalization can emit the previewed layout without
/// recomputing compression decisions.
[[nodiscard]] result<write_plan> plan_archive_write(const writer_target& target,
                                                    std::span<const writer_entry> entries,
                                                    writer_options options = {});

/// Streams a previously planned archive to a caller-owned sink.
///
/// The sink object and any backing storage remain owned by the caller and must
/// live only for the duration of this call; the write plan owns all bytes needed
/// for emission. Later behavior plans replace the initial placeholder with
/// deterministic streaming and first-failure propagation.
[[nodiscard]] result<void> finalize_archive_write(const write_plan& plan, byte_sink& sink);

} // namespace libbsa
