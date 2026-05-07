#include <libbsa/writer.hpp>

#include <libbsa/archive_path.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
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

error unsupported_deduplication()
{
    return {error_code::unsupported_format, "writer target does not support deduplication"};
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

bool is_starfield_ba2(archive_format format) noexcept
{
    return format == archive_format::starfield_ba2_gnrl || format == archive_format::starfield_ba2_dds;
}

compression_state resolved_writer_compression(const writer_target& target, compression_state state) noexcept
{
    // Starfield BA2 v3 stores the native codec in the archive-level compression
    // method. Preserve that route so unsupported method values cannot silently
    // fall back to deflate during writer planning.
    if (is_starfield_ba2(target.format) && target.compression_method.has_value() && *target.compression_method != 0 &&
        state != compression_state::raw && state != compression_state::none) {
        return compression_state::lz4_block;
    }

    return state;
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

struct planned_entry_seed {
    std::string path;
    std::uint64_t unpacked_size{};
    std::uint64_t stored_size{};
    compression_state compression{compression_state::unknown};
    std::uint32_t data_region_id{};
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

    const auto writer_compression = resolved_writer_compression(target, resolved.value());

    if (!target.supports_compression && writer_compression != compression_state::raw && writer_compression != compression_state::none &&
        writer_compression != compression_state::archive_default) {
        return failure<stored_writer_entry>({error_code::unsupported_format, "writer target does not support compression"});
    }

    payload_codec_request request{};
    request.format = target.format;
    request.entry_state = writer_compression;
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
                                        writer_compression,
                                        std::move(stored.value())});
}

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_u64(std::vector<std::byte>& bytes, std::uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_magic(std::vector<std::byte>& bytes)
{
    constexpr char magic[] = "LBSW";
    bytes.push_back(std::byte{magic[0]});
    bytes.push_back(std::byte{magic[1]});
    bytes.push_back(std::byte{magic[2]});
    bytes.push_back(std::byte{magic[3]});
}

std::uint32_t compression_state_value(compression_state state) noexcept
{
    return static_cast<std::uint32_t>(state);
}

const planned_table_region* region_named(const write_plan& plan, std::string_view name) noexcept
{
    for (const auto& region : plan.table_regions) {
        if (region.name == name) {
            return &region;
        }
    }

    return nullptr;
}

result<void> write_chunk(byte_sink& sink, const std::vector<std::byte>& chunk)
{
    auto written = sink.write(std::span<const std::byte>{chunk});
    if (!written.has_value()) {
        return failure<void>(written.error());
    }

    return success();
}

result<std::vector<std::byte>> build_header_chunk(const write_plan& plan)
{
    if (plan.entries.size() > std::numeric_limits<std::uint32_t>::max() ||
        plan.data_regions.size() > std::numeric_limits<std::uint32_t>::max() ||
        plan.total_size > std::numeric_limits<std::uint32_t>::max()) {
        return failure<std::vector<std::byte>>(writer_layout_overflow());
    }

    std::vector<std::byte> header;
    header.reserve(static_cast<std::size_t>(header_size));
    append_magic(header);
    append_u32(header, static_cast<std::uint32_t>(plan.entries.size()));
    append_u32(header, static_cast<std::uint32_t>(plan.data_regions.size()));
    // The Phase 8 harness header is deliberately 16 bytes, so it records the
    // checked 32-bit total size while layout preview keeps the full uint64 value.
    append_u32(header, static_cast<std::uint32_t>(plan.total_size));
    return success(std::move(header));
}

result<std::vector<std::byte>> build_entry_table_chunk(const write_plan& plan)
{
    const auto* entry_table = region_named(plan, "entry_table");
    if (entry_table == nullptr || entry_table->size > std::numeric_limits<std::size_t>::max()) {
        return failure<std::vector<std::byte>>(writer_layout_overflow());
    }

    std::vector<std::byte> table;
    table.reserve(static_cast<std::size_t>(entry_table->size));
    for (const auto& entry : plan.entries) {
        if (entry.path.size() > std::numeric_limits<std::uint32_t>::max()) {
            return failure<std::vector<std::byte>>(writer_layout_overflow());
        }
        append_u32(table, entry.data_region_id);
        append_u64(table, entry.offset);
        append_u64(table, entry.size);
        append_u64(table, entry.stored_size);
        append_u32(table, static_cast<std::uint32_t>(entry.path.size()));
        for (const char ch : entry.path) {
            table.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
        }
    }

    if (table.size() != entry_table->size) {
        return failure<std::vector<std::byte>>(writer_layout_overflow());
    }
    return success(std::move(table));
}

result<std::vector<std::byte>> build_data_region_table_chunk(const write_plan& plan)
{
    const auto* data_region_table = region_named(plan, "data_region_table");
    if (data_region_table == nullptr || data_region_table->size > std::numeric_limits<std::size_t>::max()) {
        return failure<std::vector<std::byte>>(writer_layout_overflow());
    }

    std::vector<std::byte> table;
    table.reserve(static_cast<std::size_t>(data_region_table->size));
    for (const auto& region : plan.data_regions) {
        append_u32(table, region.id);
        append_u64(table, region.offset);
        append_u64(table, region.stored_size);
        append_u64(table, region.unpacked_size);
        append_u32(table, compression_state_value(region.compression));
    }

    if (table.size() != data_region_table->size) {
        return failure<std::vector<std::byte>>(writer_layout_overflow());
    }
    return success(std::move(table));
}

} // namespace

result<write_plan> plan_archive_write(const writer_target& target,
                                       std::span<const writer_entry> entries,
                                        writer_options options)
{
    if (!supported_writer_target(target.format)) {
        return failure<write_plan>(unsupported_writer_target());
    }

    if (options.deduplicate && !target.supports_shared_data_regions) {
        return failure<write_plan>(unsupported_deduplication());
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

    write_plan plan{};
    plan.target = target;
    plan.options = options;
    plan.entries.reserve(stored_entries.size());
    plan.data_regions.reserve(stored_entries.size());
    std::vector<planned_entry_seed> planned_entries;
    planned_entries.reserve(stored_entries.size());

    for (auto& stored : stored_entries) {
        const auto stored_size = static_cast<std::uint64_t>(stored.stored_payload.size());
        std::uint32_t data_region_id = static_cast<std::uint32_t>(plan.data_regions.size());

        if (options.deduplicate) {
            const auto existing = std::find_if(plan.data_regions.begin(), plan.data_regions.end(), [&](const auto& region) {
                return region.stored_payload == stored.stored_payload;
            });
            if (existing != plan.data_regions.end()) {
                data_region_id = existing->id;
            }
        }

        if (!options.deduplicate || data_region_id == plan.data_regions.size()) {
            planned_data_region region{};
            region.id = data_region_id;
            region.stored_size = stored_size;
            region.unpacked_size = stored.unpacked_size;
            region.compression = stored.compression;
            region.stored_payload = std::move(stored.stored_payload);
            plan.data_regions.push_back(std::move(region));
        }

        planned_entries.push_back(planned_entry_seed{std::move(stored.path),
                                                     stored.unpacked_size,
                                                     stored_size,
                                                     stored.compression,
                                                     data_region_id});
    }

    const auto data_region_count = static_cast<std::uint64_t>(plan.data_regions.size());
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

    std::uint64_t payload_cursor = payloads_offset;
    for (auto& region : plan.data_regions) {
        region.offset = payload_cursor;

        if (!checked_add(payload_cursor, region.stored_size, payload_cursor)) {
            return failure<write_plan>(writer_layout_overflow());
        }
    }

    for (auto& seed : planned_entries) {
        const auto region = std::find_if(plan.data_regions.begin(), plan.data_regions.end(), [&](const auto& candidate) {
            return candidate.id == seed.data_region_id;
        });
        if (region == plan.data_regions.end()) {
            return failure<write_plan>(writer_layout_overflow());
        }

        planned_entry entry{};
        entry.path = std::move(seed.path);
        entry.size = seed.unpacked_size;
        entry.stored_size = seed.stored_size;
        entry.offset = region->offset;
        entry.data_region_id = seed.data_region_id;
        entry.compression = seed.compression;

        plan.entries.push_back(std::move(entry));
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
    auto header = build_header_chunk(plan);
    if (!header.has_value()) {
        return failure<void>(header.error());
    }
    if (header.value().size() != header_size) {
        return failure<void>(writer_layout_overflow());
    }
    auto written = write_chunk(sink, header.value());
    if (!written.has_value()) {
        return failure<void>(written.error());
    }

    auto entry_table = build_entry_table_chunk(plan);
    if (!entry_table.has_value()) {
        return failure<void>(entry_table.error());
    }
    written = write_chunk(sink, entry_table.value());
    if (!written.has_value()) {
        return failure<void>(written.error());
    }

    auto data_region_table = build_data_region_table_chunk(plan);
    if (!data_region_table.has_value()) {
        return failure<void>(data_region_table.error());
    }
    written = write_chunk(sink, data_region_table.value());
    if (!written.has_value()) {
        return failure<void>(written.error());
    }

    for (const auto& region : plan.data_regions) {
        written = sink.write(std::span<const std::byte>{region.stored_payload});
        if (!written.has_value()) {
            return failure<void>(written.error());
        }
    }

    return success();
}

} // namespace libbsa
