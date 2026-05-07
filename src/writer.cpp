#include <libbsa/writer.hpp>

#include <libbsa/archive_path.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace libbsa {
namespace {

constexpr std::uint64_t header_size = 16;
constexpr std::uint64_t entry_record_fixed_size = 32;
constexpr std::uint64_t data_region_record_size = 32;

error writer_layout_overflow()
{
    return {error_code::malformed_archive, "writer layout overflow"};
}

error unsupported_writer_target()
{
    return {error_code::unsupported_format, "unsupported writer target"};
}

error invalid_writer_path()
{
    return {error_code::malformed_archive, "invalid writer path"};
}

error duplicate_writer_path()
{
    return {error_code::malformed_archive, "duplicate writer path"};
}

error writer_finalization_not_implemented()
{
    return {error_code::unsupported_format, "writer finalization is not implemented"};
}

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right) {
        return false;
    }
    result = left + right;
    return true;
}

bool checked_mul(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left) {
        return false;
    }
    result = left * right;
    return true;
}

bool supported_writer_target(archive_format format) noexcept
{
    switch (format) {
    case archive_format::tes3_bsa:
    case archive_format::tes4_bsa:
    case archive_format::fo3_bsa:
    case archive_format::sse_bsa:
    case archive_format::fo4_ba2_gnrl:
    case archive_format::fo4_ba2_dds:
    case archive_format::starfield_ba2_gnrl:
    case archive_format::starfield_ba2_dds:
        return true;
    }

    return false;
}

std::uint64_t layout_header_size_for(const writer_target& target) noexcept
{
    // The current public target has no oversized metadata field. Reserve an
    // otherwise unsupported method value as a test seam for checked arithmetic
    // without allocating huge path or payload buffers.
    if (target.compression_method == std::numeric_limits<std::uint32_t>::max()) {
        return std::numeric_limits<std::uint64_t>::max() - 8;
    }
    return header_size;
}

struct normalized_writer_entry {
    std::string path;
    const writer_entry* source{};
};

struct stored_writer_entry {
    std::string path;
    std::uint64_t unpacked_size{};
    compression_state compression{compression_state::unknown};
    std::vector<std::byte> stored_payload;
};

result<std::vector<normalized_writer_entry>> normalize_entries(std::span<const writer_entry> entries)
{
    std::vector<normalized_writer_entry> normalized_entries;
    normalized_entries.reserve(entries.size());
    std::vector<std::string> normalized_paths;
    normalized_paths.reserve(entries.size());

    for (const auto& entry : entries) {
        auto normalized = normalize_archive_path(entry.path);
        if (!normalized.has_value()) {
            return failure<std::vector<normalized_writer_entry>>(invalid_writer_path());
        }

        auto path = normalized.value().string();
        if (std::find(normalized_paths.begin(), normalized_paths.end(), path) != normalized_paths.end()) {
            return failure<std::vector<normalized_writer_entry>>(duplicate_writer_path());
        }
        normalized_paths.push_back(path);
        normalized_entries.push_back(normalized_writer_entry{std::move(path), &entry});
    }

    std::sort(normalized_entries.begin(), normalized_entries.end(), [](const auto& left, const auto& right) {
        return left.path < right.path;
    });
    return success(std::move(normalized_entries));
}

result<stored_writer_entry> store_entry_payload(const writer_target& target, normalized_writer_entry entry)
{
    auto resolved = resolve_write_compression(target.format, entry.source->compression, target.archive_default_compressed);
    if (!resolved.has_value()) {
        return failure<stored_writer_entry>(resolved.error());
    }

    if (!target.supports_compression && resolved.value() != compression_state::raw && resolved.value() != compression_state::none &&
        resolved.value() != compression_state::archive_default) {
        return failure<stored_writer_entry>({error_code::unsupported_format, "writer target does not support compression"});
    }

    payload_codec_request request{};
    request.format = target.format;
    request.entry_state = resolved.value();
    request.compression_method = target.compression_method;
    auto algorithm = resolve_payload_codec(request);
    if (!algorithm.has_value()) {
        return failure<stored_writer_entry>(algorithm.error());
    }

    auto stored = compress_payload(algorithm.value(), std::span<const std::byte>{entry.source->payload});
    if (!stored.has_value()) {
        return failure<stored_writer_entry>(stored.error());
    }

    return success(stored_writer_entry{std::move(entry.path),
                                       static_cast<std::uint64_t>(entry.source->payload.size()),
                                       resolved.value(),
                                       std::move(stored.value())});
}

} // namespace

result<write_plan> plan_archive_write(const writer_target& target,
                                      std::span<const writer_entry> entries,
                                       writer_options options)
{
    if (!supported_writer_target(target.format)) {
        return failure<write_plan>(unsupported_writer_target());
    }

    auto normalized = normalize_entries(entries);
    if (!normalized.has_value()) {
        return failure<write_plan>(normalized.error());
    }

    std::vector<stored_writer_entry> stored_entries;
    stored_entries.reserve(normalized.value().size());
    for (auto& entry : normalized.value()) {
        auto stored = store_entry_payload(target, std::move(entry));
        if (!stored.has_value()) {
            return failure<write_plan>(stored.error());
        }
        stored_entries.push_back(std::move(stored.value()));
    }

    std::uint64_t entry_table_size = 0;
    for (const auto& entry : stored_entries) {
        std::uint64_t record_size_with_path = 0;
        if (!checked_add(entry_record_fixed_size, static_cast<std::uint64_t>(entry.path.size()), record_size_with_path) ||
            !checked_add(entry_table_size, record_size_with_path, entry_table_size)) {
            return failure<write_plan>(writer_layout_overflow());
        }
    }

    const auto data_region_count = static_cast<std::uint64_t>(stored_entries.size());
    std::uint64_t data_region_table_size = 0;
    if (!checked_mul(data_region_count, data_region_record_size, data_region_table_size)) {
        return failure<write_plan>(writer_layout_overflow());
    }

    const auto planned_header_size = layout_header_size_for(target);
    const std::uint64_t entry_table_offset = planned_header_size;
    std::uint64_t data_region_table_offset = 0;
    std::uint64_t payloads_offset = 0;
    if (!checked_add(entry_table_offset, entry_table_size, data_region_table_offset) ||
        !checked_add(data_region_table_offset, data_region_table_size, payloads_offset)) {
        return failure<write_plan>(writer_layout_overflow());
    }

    write_plan plan{};
    plan.target = target;
    plan.options = options;
    plan.entries.reserve(stored_entries.size());
    plan.data_regions.reserve(stored_entries.size());

    std::uint64_t payload_cursor = payloads_offset;
    for (std::size_t index = 0; index < stored_entries.size(); ++index) {
        auto& stored = stored_entries[index];
        const auto stored_size = static_cast<std::uint64_t>(stored.stored_payload.size());

        planned_data_region region{};
        region.id = static_cast<std::uint32_t>(index);
        region.offset = payload_cursor;
        region.stored_size = stored_size;
        region.unpacked_size = stored.unpacked_size;
        region.compression = stored.compression;
        region.stored_payload = std::move(stored.stored_payload);

        planned_entry entry{};
        entry.path = std::move(stored.path);
        entry.size = region.unpacked_size;
        entry.stored_size = region.stored_size;
        entry.offset = region.offset;
        entry.data_region_id = region.id;
        entry.compression = region.compression;

        if (!checked_add(payload_cursor, stored_size, payload_cursor)) {
            return failure<write_plan>(writer_layout_overflow());
        }

        plan.entries.push_back(std::move(entry));
        plan.data_regions.push_back(std::move(region));
    }

    plan.table_regions.push_back(planned_table_region{"header", 0, planned_header_size});
    plan.table_regions.push_back(planned_table_region{"entry_table", entry_table_offset, entry_table_size});
    plan.table_regions.push_back(planned_table_region{"data_region_table", data_region_table_offset, data_region_table_size});
    plan.table_regions.push_back(planned_table_region{"payloads", payloads_offset, payload_cursor - payloads_offset});
    plan.total_size = payload_cursor;
    return success(std::move(plan));
}

result<void> finalize_archive_write(const write_plan& plan, byte_sink& sink)
{
    (void)plan;
    (void)sink;
    return failure<void>(writer_finalization_not_implemented());
}

} // namespace libbsa
