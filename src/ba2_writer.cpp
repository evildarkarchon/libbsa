#include <libbsa/ba2_writer.hpp>

#include <libbsa/archive_path.hpp>
#include <libbsa/compression.hpp>

#include "hash.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa {
namespace {

constexpr std::uint32_t magic_btdx = 0x58445442U;
constexpr std::uint32_t magic_gnrl = 0x4c524e47U;
constexpr std::uint32_t version_fo4_v1 = 0x01U;
constexpr std::uint32_t version_starfield_v2 = 0x02U;
constexpr std::uint32_t version_starfield_v3 = 0x03U;
constexpr std::uint32_t version_fo4_v7 = 0x07U;
constexpr std::uint32_t version_fo4_v8 = 0x08U;
constexpr std::uint32_t base_header_size = 24U;
constexpr std::uint32_t starfield_v2_header_size = 32U;
constexpr std::uint32_t starfield_v3_header_size = 36U;
constexpr std::uint64_t record_size = 36U;

result<ba2_write_plan> unsupported_ba2_plan(std::string_view message)
{
    return failure<ba2_write_plan>({error_code::unsupported_format, std::string{message}});
}

error writer_layout_overflow()
{
    return {error_code::malformed_archive, "BA2 writer layout overflow"};
}

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right) {
        return false;
    }
    result = left + right;
    return true;
}

result<std::uint32_t> checked_u32(std::uint64_t value)
{
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return failure<std::uint32_t>(writer_layout_overflow());
    }
    return success(static_cast<std::uint32_t>(value));
}

result<std::uint16_t> checked_u16(std::uint64_t value)
{
    if (value > std::numeric_limits<std::uint16_t>::max()) {
        return failure<std::uint16_t>(writer_layout_overflow());
    }
    return success(static_cast<std::uint16_t>(value));
}

void append_u16(std::vector<std::byte>& bytes, std::uint16_t value)
{
    for (int shift = 0; shift < 16; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
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

void append_magic(std::vector<std::byte>& bytes, std::string_view magic)
{
    for (char ch : magic) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
}

void append_extension4(std::vector<std::byte>& bytes, std::string_view path)
{
    const auto dot = path.find_last_of('.');
    const auto extension = dot == std::string_view::npos ? std::string_view{} : path.substr(dot, std::min<std::size_t>(4U, path.size() - dot));
    for (std::size_t i = 0; i < 4U; ++i) {
        bytes.push_back(i < extension.size() ? static_cast<std::byte>(static_cast<unsigned char>(extension[i])) : std::byte{0});
    }
}

struct ba2_target_info {
    std::uint32_t version{};
    std::uint32_t header_size{};
    archive_format format{archive_format::fo4_ba2_gnrl};
    ba2_write_subtype subtype{ba2_write_subtype::gnrl};
};

result<ba2_target_info> target_info_for_gnrl(ba2_write_target target)
{
    switch (target) {
    case ba2_write_target::fallout4_gnrl_v1:
        return success(ba2_target_info{version_fo4_v1, base_header_size, archive_format::fo4_ba2_gnrl});
    case ba2_write_target::fallout4_gnrl_v7:
        return success(ba2_target_info{version_fo4_v7, base_header_size, archive_format::fo4_ba2_gnrl});
    case ba2_write_target::fallout4_gnrl_v8:
        return success(ba2_target_info{version_fo4_v8, base_header_size, archive_format::fo4_ba2_gnrl});
    case ba2_write_target::starfield_gnrl_v2:
        return success(ba2_target_info{version_starfield_v2, starfield_v2_header_size, archive_format::starfield_ba2_gnrl});
    case ba2_write_target::starfield_gnrl_v3:
        return success(ba2_target_info{version_starfield_v3, starfield_v3_header_size, archive_format::starfield_ba2_gnrl});
    default:
        return failure<ba2_target_info>({error_code::unsupported_format, "unsupported BA2 GNRL writer target"});
    }
}

struct normalized_gnrl_entry {
    std::string path;
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

result<std::vector<normalized_gnrl_entry>> normalize_gnrl_entries(std::span<const ba2_gnrl_memory_entry> entries)
{
    std::vector<normalized_gnrl_entry> normalized_entries;
    normalized_entries.reserve(entries.size());
    std::vector<std::string> paths;
    paths.reserve(entries.size());

    for (const auto& entry : entries) {
        auto normalized = normalize_archive_path(entry.path);
        if (!normalized.has_value()) {
            return failure<std::vector<normalized_gnrl_entry>>(normalized.error());
        }
        auto path = normalized.value().string();
        if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
            return failure<std::vector<normalized_gnrl_entry>>({error_code::malformed_archive, "duplicate BA2 writer path"});
        }
        paths.push_back(path);
        normalized_entries.push_back(normalized_gnrl_entry{std::move(path), entry.payload, entry.compression});
    }

    std::sort(normalized_entries.begin(), normalized_entries.end(), [](const auto& left, const auto& right) {
        return left.path < right.path;
    });
    return success(std::move(normalized_entries));
}

std::uint32_t directory_hash_for(std::string_view path)
{
    const auto slash = path.find_last_of('/');
    return slash == std::string_view::npos ? 0U : detail::hash_fo4_path_part(path.substr(0, slash));
}

std::uint32_t name_hash_for(std::string_view path)
{
    const auto slash = path.find_last_of('/');
    return detail::hash_fo4_path_part(slash == std::string_view::npos ? path : path.substr(slash + 1U));
}

result<compression_state> resolve_gnrl_compression(const ba2_target_info& info,
                                                   const normalized_gnrl_entry& entry,
                                                   const ba2_write_options& options)
{
    if (info.version == version_starfield_v3 && options.starfield_v3_compression_method == 3U &&
        entry.compression != compression_policy::force_raw && options.archive_default_compressed) {
        return success(compression_state::lz4_block);
    }
    if (info.version == version_starfield_v3 && options.starfield_v3_compression_method == 3U && entry.compression == compression_policy::force_compressed) {
        return success(compression_state::lz4_block);
    }
    return resolve_write_compression(info.format, entry.compression, options.archive_default_compressed);
}

result<std::vector<std::byte>> stored_payload_for(const ba2_target_info& info,
                                                  const normalized_gnrl_entry& entry,
                                                  compression_state compression,
                                                  const ba2_write_options& options)
{
    if (compression == compression_state::raw) {
        return success(entry.payload);
    }

    payload_codec_request request{};
    request.format = info.format;
    request.entry_state = compression;
    request.compression_method = info.version == version_starfield_v3 ? options.starfield_v3_compression_method : 0U;
    auto algorithm = resolve_payload_codec(request);
    if (!algorithm.has_value()) {
        return failure<std::vector<std::byte>>(algorithm.error());
    }
    return compress_payload(algorithm.value(), std::span<const std::byte>{entry.payload});
}

result<void> write_chunk(byte_sink& sink, std::span<const std::byte> bytes)
{
    return sink.write(bytes);
}

} // namespace

result<ba2_write_plan> plan_ba2_gnrl_write(ba2_write_target target,
                                           std::span<const ba2_gnrl_memory_entry> entries,
                                           ba2_write_options options)
{
    auto info = target_info_for_gnrl(target);
    if (!info.has_value()) {
        return failure<ba2_write_plan>(info.error());
    }
    if (info.value().version == version_starfield_v3 && options.starfield_v3_compression_method != 0U &&
        options.starfield_v3_compression_method != 3U) {
        return failure<ba2_write_plan>({error_code::unsupported_format, "unsupported Starfield BA2 compression method"});
    }

    auto normalized = normalize_gnrl_entries(entries);
    if (!normalized.has_value()) {
        return failure<ba2_write_plan>(normalized.error());
    }

    auto file_count = checked_u32(normalized.value().size());
    if (!file_count.has_value()) {
        return failure<ba2_write_plan>(file_count.error());
    }

    std::vector<std::byte> name_table;
    for (const auto& entry : normalized.value()) {
        auto name_size = checked_u16(entry.path.size());
        if (!name_size.has_value()) {
            return failure<ba2_write_plan>(name_size.error());
        }
        append_u16(name_table, name_size.value());
        append_magic(name_table, entry.path);
    }

    std::uint64_t record_table_size = 0;
    std::uint64_t file_table_offset = 0;
    std::uint64_t payload_cursor = 0;
    if (!checked_add(static_cast<std::uint64_t>(file_count.value()) * record_size, 0, record_table_size) ||
        !checked_add(info.value().header_size, record_table_size, file_table_offset) ||
        !checked_add(file_table_offset, name_table.size(), payload_cursor)) {
        return failure<ba2_write_plan>(writer_layout_overflow());
    }

    ba2_write_plan plan{};
    plan.target = target;
    plan.options = options;
    plan.native = ba2_native_target_preview{ba2_write_subtype::gnrl,
                                            info.value().version,
                                            info.value().header_size,
                                            info.value().version == version_starfield_v3 ? options.starfield_v3_compression_method : 0U};
    plan.gnrl.file_table_offset = file_table_offset;
    plan.gnrl.table_regions.push_back(planned_ba2_table_region{"GNRL record table", info.value().header_size, record_table_size});
    plan.gnrl.table_regions.push_back(planned_ba2_table_region{"FileTableOffset name table", file_table_offset, name_table.size()});
    plan.gnrl.entries.reserve(normalized.value().size());

    for (const auto& entry : normalized.value()) {
        auto compression = resolve_gnrl_compression(info.value(), entry, options);
        if (!compression.has_value()) {
            return failure<ba2_write_plan>(compression.error());
        }
        auto stored = stored_payload_for(info.value(), entry, compression.value(), options);
        if (!stored.has_value()) {
            return failure<ba2_write_plan>(stored.error());
        }
        auto unpacked_size = checked_u32(entry.payload.size());
        auto stored_size = checked_u32(stored.value().size());
        if (!unpacked_size.has_value()) {
            return failure<ba2_write_plan>(unpacked_size.error());
        }
        if (!stored_size.has_value()) {
            return failure<ba2_write_plan>(stored_size.error());
        }

        planned_ba2_gnrl_entry planned{};
        planned.path = entry.path;
        planned.name_hash = name_hash_for(entry.path);
        planned.directory_hash = directory_hash_for(entry.path);
        planned.unpacked_size = unpacked_size.value();
        planned.packed_size = compression.value() == compression_state::raw ? 0U : stored_size.value();
        planned.compression = compression.value();

        const auto shared = options.deduplicate ? std::find_if(plan.data_regions.begin(), plan.data_regions.end(), [&stored](const auto& region) {
            return region.stored_payload == stored.value();
        }) : plan.data_regions.end();

        if (shared != plan.data_regions.end()) {
            planned.offset = shared->offset;
            planned.data_region_id = shared->id;
        } else {
            planned.offset = payload_cursor;
            planned.data_region_id = static_cast<std::uint32_t>(plan.data_regions.size());
            plan.data_regions.push_back(planned_ba2_data_region{planned.data_region_id,
                                                                payload_cursor,
                                                                static_cast<std::uint64_t>(stored.value().size()),
                                                                planned.unpacked_size,
                                                                compression.value(),
                                                                std::move(stored.value())});
            if (!checked_add(payload_cursor, plan.data_regions.back().stored_size, payload_cursor)) {
                return failure<ba2_write_plan>(writer_layout_overflow());
            }
        }
        plan.gnrl.entries.push_back(std::move(planned));
    }

    append_magic(plan.table_bytes, "BTDX");
    append_u32(plan.table_bytes, info.value().version);
    append_magic(plan.table_bytes, "GNRL");
    append_u32(plan.table_bytes, file_count.value());
    append_u64(plan.table_bytes, file_table_offset);
    if (info.value().version == version_starfield_v2) {
        append_u32(plan.table_bytes, 0U);
        append_u32(plan.table_bytes, 0U);
    } else if (info.value().version == version_starfield_v3) {
        append_u32(plan.table_bytes, 0U);
        append_u32(plan.table_bytes, 0U);
        append_u32(plan.table_bytes, options.starfield_v3_compression_method);
    }

    for (const auto& entry : plan.gnrl.entries) {
        auto packed_size = checked_u32(entry.packed_size);
        auto unpacked_size = checked_u32(entry.unpacked_size);
        if (!packed_size.has_value()) {
            return failure<ba2_write_plan>(packed_size.error());
        }
        if (!unpacked_size.has_value()) {
            return failure<ba2_write_plan>(unpacked_size.error());
        }
        append_u32(plan.table_bytes, entry.name_hash);
        append_extension4(plan.table_bytes, entry.path);
        append_u32(plan.table_bytes, entry.directory_hash);
        append_u32(plan.table_bytes, 0U);
        append_u64(plan.table_bytes, entry.offset);
        // Native BA2 GNRL uses packed_size == 0 as the raw marker; the actual stored byte count remains Size.
        append_u32(plan.table_bytes, packed_size.value());
        append_u32(plan.table_bytes, unpacked_size.value());
        append_u32(plan.table_bytes, 0U);
    }
    plan.table_bytes.insert(plan.table_bytes.end(), name_table.begin(), name_table.end());
    plan.total_size = payload_cursor;
    return success(std::move(plan));
}

result<ba2_write_plan> plan_ba2_gnrl_write_from_disk(ba2_write_target target,
                                                     std::span<const ba2_gnrl_disk_entry> entries,
                                                     ba2_write_options options)
{
    std::vector<ba2_gnrl_memory_entry> memory_entries;
    memory_entries.reserve(entries.size());
    for (const auto& entry : entries) {
        auto normalized = normalize_archive_path(entry.path);
        if (!normalized.has_value()) {
            return failure<ba2_write_plan>(normalized.error());
        }

        std::ifstream stream{entry.host_path, std::ios::binary};
        if (!stream) {
            return failure<ba2_write_plan>({error_code::io_failure, "failed to open BA2 GNRL writer input file"});
        }
        std::vector<std::byte> payload;
        char ch = 0;
        while (stream.get(ch)) {
            payload.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
        }
        if (!stream.eof()) {
            return failure<ba2_write_plan>({error_code::io_failure, "failed to read BA2 GNRL writer input file"});
        }
        memory_entries.push_back(ba2_gnrl_memory_entry{entry.path, std::move(payload), entry.compression});
    }
    return plan_ba2_gnrl_write(target, std::span<const ba2_gnrl_memory_entry>{memory_entries}, options);
}

result<ba2_write_plan> plan_ba2_dds_write(ba2_write_target,
                                          std::span<const ba2_dds_memory_entry>,
                                          ba2_write_options)
{
    return unsupported_ba2_plan("BA2 DDS writer planning is not implemented");
}

result<ba2_write_plan> plan_ba2_dds_write_from_disk(ba2_write_target,
                                                    std::span<const ba2_dds_disk_entry>,
                                                    ba2_write_options)
{
    return unsupported_ba2_plan("BA2 DDS writer planning is not implemented");
}

result<void> finalize_ba2_write(const ba2_write_plan& plan, byte_sink& sink)
{
    auto written = write_chunk(sink, std::span<const std::byte>{plan.table_bytes});
    if (!written.has_value()) {
        return failure<void>(written.error());
    }
    for (const auto& region : plan.data_regions) {
        written = write_chunk(sink, std::span<const std::byte>{region.stored_payload});
        if (!written.has_value()) {
            return failure<void>(written.error());
        }
    }
    return success();
}

} // namespace libbsa
