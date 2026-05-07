#include <libbsa/bsa_writer.hpp>

#include <libbsa/archive_path.hpp>
#include <libbsa/compression.hpp>

#include "bsa_reader.hpp"
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

struct normalized_memory_entry {
    std::string path;
    std::string folder;
    std::string file;
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

struct planned_tes4_entry {
    std::string path;
    std::string folder;
    std::string file;
    std::uint64_t folder_hash{};
    std::uint64_t file_hash{};
    std::uint32_t size_field{};
    std::uint32_t payload_offset{};
    std::uint32_t data_region_id{};
    std::uint64_t unpacked_size{};
    compression_state compression{compression_state::raw};
    std::vector<std::byte> stored_payload;
};

struct planned_tes3_entry {
    std::string path;
    std::uint64_t hash{};
    std::uint32_t size{};
    std::uint32_t relative_offset{};
    std::uint32_t name_offset{};
    std::uint32_t data_region_id{};
    std::uint64_t payload_offset{};
    std::vector<std::byte> stored_payload;
};

struct tes4_folder_group {
    std::string folder;
    std::uint64_t hash{};
    std::uint64_t block_offset{};
    std::vector<planned_tes4_entry> entries;
};

error writer_layout_overflow()
{
    return {error_code::malformed_archive, "writer layout overflow"};
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

result<std::uint32_t> checked_u32(std::uint64_t value)
{
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return failure<std::uint32_t>(writer_layout_overflow());
    }
    return success(static_cast<std::uint32_t>(value));
}

archive_format archive_format_for(bsa_write_target target)
{
    switch (target) {
    case bsa_write_target::oblivion_v103:
        return archive_format::tes4_bsa;
    case bsa_write_target::fo3_fnv_skyrim_le_v104:
        return archive_format::fo3_bsa;
    case bsa_write_target::skyrim_se_ae_v105:
        return archive_format::sse_bsa;
    case bsa_write_target::tes3_morrowind:
        break;
    }
    return archive_format::tes3_bsa;
}

std::uint32_t version_for(bsa_write_target target) noexcept
{
    // Native TES4-family archives identify themselves with "BSA\0" plus versions 0x67, 0x68, or 0x69.
    switch (target) {
    case bsa_write_target::oblivion_v103:
        return detail::version_tes4;
    case bsa_write_target::fo3_fnv_skyrim_le_v104:
        return detail::version_fo3;
    case bsa_write_target::skyrim_se_ae_v105:
        return detail::version_sse;
    case bsa_write_target::tes3_morrowind:
        break;
    }
    return 0;
}

void append_u8(std::vector<std::byte>& bytes, std::uint8_t value)
{
    bytes.push_back(static_cast<std::byte>(value));
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

void append_tes3_hash(std::vector<std::byte>& bytes, std::uint64_t value)
{
    // TES5Edit writes TES3 hashes as two little-endian cardinals: Hash shr 32, then Hash and $FFFFFFFF.
    append_u32(bytes, static_cast<std::uint32_t>(value >> 32U));
    append_u32(bytes, static_cast<std::uint32_t>(value & 0xffffffffULL));
}

void append_string_bytes(std::vector<std::byte>& bytes, std::string_view value)
{
    for (char ch : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
}

void append_cstring(std::vector<std::byte>& bytes, std::string_view value)
{
    append_string_bytes(bytes, value);
    bytes.push_back(std::byte{0});
}

std::string embedded_name_for(std::string_view path)
{
    std::string embedded{path};
    std::replace(embedded.begin(), embedded.end(), '/', '\\');
    return embedded;
}

result<void> append_len8_string(std::vector<std::byte>& bytes, std::string_view value)
{
    if (value.size() + 1U > std::numeric_limits<std::uint8_t>::max()) {
        return failure<void>(writer_layout_overflow());
    }
    append_u8(bytes, static_cast<std::uint8_t>(value.size() + 1U));
    append_cstring(bytes, value);
    return success();
}

result<std::vector<normalized_memory_entry>> normalize_entries(std::span<const bsa_memory_entry> entries)
{
    std::vector<normalized_memory_entry> normalized_entries;
    normalized_entries.reserve(entries.size());
    std::vector<std::string> paths;
    paths.reserve(entries.size());

    for (const auto& entry : entries) {
        auto normalized = normalize_archive_path(entry.path);
        if (!normalized.has_value()) {
            return failure<std::vector<normalized_memory_entry>>(normalized.error());
        }
        auto path = normalized.value().string();
        if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
            return failure<std::vector<normalized_memory_entry>>({error_code::malformed_archive, "duplicate BSA writer path"});
        }
        const auto slash = path.find_last_of('/');
        if (slash == std::string::npos || slash == 0 || slash + 1U >= path.size()) {
            return failure<std::vector<normalized_memory_entry>>({error_code::malformed_archive, "BSA writer path requires folder and file"});
        }
        paths.push_back(path);
        normalized_entries.push_back(normalized_memory_entry{std::move(path), paths.back().substr(0, slash), paths.back().substr(slash + 1U),
                                                            entry.payload, entry.compression});
    }

    return success(std::move(normalized_entries));
}

result<void> validate_disk_entry_archive_paths(std::span<const bsa_disk_entry> entries)
{
    std::vector<std::string> paths;
    paths.reserve(entries.size());

    for (const auto& entry : entries) {
        auto normalized = normalize_archive_path(entry.path);
        if (!normalized.has_value()) {
            return failure<void>(normalized.error());
        }

        auto path = normalized.value().string();
        if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
            return failure<void>({error_code::malformed_archive, "duplicate BSA writer path"});
        }
        const auto slash = path.find_last_of('/');
        if (slash == std::string::npos || slash == 0 || slash + 1U >= path.size()) {
            return failure<void>({error_code::malformed_archive, "BSA writer path requires folder and file"});
        }
        paths.push_back(std::move(path));
    }

    return success();
}

std::uint32_t file_flag_for_extension(std::string_view file) noexcept
{
    const auto dot = file.find_last_of('.');
    if (dot == std::string_view::npos) {
        return 0;
    }
    const auto extension = file.substr(dot + 1U);
    if (extension == "nif") {
        return 0x0001;
    }
    if (extension == "dds") {
        return 0x0002;
    }
    if (extension == "xml") {
        return 0x0004;
    }
    if (extension == "wav" || extension == "mp3" || extension == "lip") {
        return 0x0008;
    }
    return 0;
}

result<planned_tes4_entry> plan_tes4_entry(const normalized_memory_entry& entry,
                                           bsa_write_target target,
                                           const bsa_write_options& options)
{
    const auto format = archive_format_for(target);
    auto compression = resolve_write_compression(format, entry.compression, options.archive_default_compressed);
    if (!compression.has_value()) {
        return failure<planned_tes4_entry>(compression.error());
    }

    std::vector<std::byte> native_payload;
    if (options.embedded_names) {
        // TES4-family embedded names use the native archive path byte spelling; the reader only skips by length.
        const auto embedded_name = embedded_name_for(entry.path);
        if (embedded_name.size() > std::numeric_limits<std::uint8_t>::max()) {
            return failure<planned_tes4_entry>(writer_layout_overflow());
        }
        append_u8(native_payload, static_cast<std::uint8_t>(embedded_name.size()));
        append_string_bytes(native_payload, embedded_name);
    }

    if (compression.value() == compression_state::raw) {
        native_payload.insert(native_payload.end(), entry.payload.begin(), entry.payload.end());
    } else {
        auto unpacked_size = checked_u32(entry.payload.size());
        if (!unpacked_size.has_value()) {
            return failure<planned_tes4_entry>(unpacked_size.error());
        }
        payload_codec_request request{};
        request.format = format;
        request.entry_state = compression.value();
        auto algorithm = resolve_payload_codec(request);
        if (!algorithm.has_value()) {
            return failure<planned_tes4_entry>(algorithm.error());
        }
        auto compressed = compress_payload(algorithm.value(), std::span<const std::byte>{entry.payload});
        if (!compressed.has_value()) {
            return failure<planned_tes4_entry>(compressed.error());
        }
        append_u32(native_payload, unpacked_size.value());
        native_payload.insert(native_payload.end(), compressed.value().begin(), compressed.value().end());
    }

    auto stored_size = checked_u32(native_payload.size());
    if (!stored_size.has_value()) {
        return failure<planned_tes4_entry>(stored_size.error());
    }
    const bool archive_default_compressed = options.archive_default_compressed;
    const bool entry_compressed = compression.value() != compression_state::raw;
    auto size_field = stored_size.value();
    if (archive_default_compressed != entry_compressed) {
        size_field |= detail::file_size_compress;
    }

    planned_tes4_entry planned{};
    planned.path = entry.path;
    planned.folder = entry.folder;
    planned.file = entry.file;
    planned.folder_hash = detail::hash_tes4_path(planned.folder);
    planned.file_hash = detail::hash_tes4_path(planned.file);
    planned.size_field = size_field;
    planned.unpacked_size = entry.payload.size();
    planned.compression = compression.value();
    planned.stored_payload = std::move(native_payload);
    return success(std::move(planned));
}

result<bsa_write_plan> plan_tes4_write(bsa_write_target target,
                                       std::span<const bsa_memory_entry> entries,
                                       bsa_write_options options)
{
    auto normalized = normalize_entries(entries);
    if (!normalized.has_value()) {
        return failure<bsa_write_plan>(normalized.error());
    }

    std::vector<planned_tes4_entry> planned_entries;
    planned_entries.reserve(normalized.value().size());
    std::uint32_t file_flags = 0;
    for (const auto& entry : normalized.value()) {
        auto planned = plan_tes4_entry(entry, target, options);
        if (!planned.has_value()) {
            return failure<bsa_write_plan>(planned.error());
        }
        file_flags |= file_flag_for_extension(entry.file);
        planned_entries.push_back(std::move(planned.value()));
    }

    std::sort(planned_entries.begin(), planned_entries.end(), [](const auto& left, const auto& right) {
        if (left.folder_hash != right.folder_hash) {
            return left.folder_hash < right.folder_hash;
        }
        if (left.file_hash != right.file_hash) {
            return left.file_hash < right.file_hash;
        }
        return left.path < right.path;
    });

    std::vector<tes4_folder_group> folders;
    for (auto& entry : planned_entries) {
        if (folders.empty() || folders.back().folder != entry.folder) {
            folders.push_back(tes4_folder_group{entry.folder, entry.folder_hash, 0, {}});
        }
        folders.back().entries.push_back(std::move(entry));
    }

    const auto folder_record_size = target == bsa_write_target::skyrim_se_ae_v105 ? detail::folder_record_size_sse
                                                                                  : detail::folder_record_size_legacy;
    std::uint64_t folder_records_size = 0;
    std::uint64_t cursor = 0;
    if (!checked_mul(folder_record_size, folders.size(), folder_records_size) || !checked_add(detail::header_size, folder_records_size, cursor)) {
        return failure<bsa_write_plan>(writer_layout_overflow());
    }

    std::uint64_t folder_blocks_size = 0;
    std::uint64_t folder_names_size = 0;
    std::uint64_t file_names_size = 0;
    std::uint32_t file_count = 0;
    for (auto& folder : folders) {
        folder.block_offset = cursor;
        std::uint64_t folder_name_record_size = 0;
        std::uint64_t file_records_size = 0;
        std::uint64_t folder_block_size = 0;
        if (!checked_add(folder.folder.size(), 2, folder_name_record_size) ||
            !checked_add(folder_names_size, folder_name_record_size, folder_names_size) ||
            !checked_mul(16, folder.entries.size(), file_records_size) ||
            !checked_add(folder_name_record_size, file_records_size, folder_block_size) ||
            !checked_add(folder_blocks_size, folder_block_size, folder_blocks_size) ||
            !checked_add(cursor, folder_block_size, cursor)) {
            return failure<bsa_write_plan>(writer_layout_overflow());
        }
        for (const auto& entry : folder.entries) {
            if (!checked_add(file_names_size, entry.file.size() + 1U, file_names_size)) {
                return failure<bsa_write_plan>(writer_layout_overflow());
            }
            if (file_count == std::numeric_limits<std::uint32_t>::max()) {
                return failure<bsa_write_plan>(writer_layout_overflow());
            }
            ++file_count;
        }
    }

    std::uint64_t payload_cursor = 0;
    if (!checked_add(detail::header_size, folder_records_size, payload_cursor) || !checked_add(payload_cursor, folder_blocks_size, payload_cursor) ||
        !checked_add(payload_cursor, file_names_size, payload_cursor)) {
        return failure<bsa_write_plan>(writer_layout_overflow());
    }

    bsa_write_plan plan{};
    plan.target = target;
    plan.options = options;
    plan.flags = detail::archive_pathnames | detail::archive_filenames;
    if (options.archive_default_compressed) {
        plan.flags |= detail::archive_compress;
    }
    if (options.embedded_names) {
        plan.flags |= detail::archive_embed_name;
    }

    for (auto& folder : folders) {
        for (auto& entry : folder.entries) {
            const auto shared = options.deduplicate
                ? std::find_if(plan.data_regions.begin(), plan.data_regions.end(), [&entry](const auto& region) {
                      return region.stored_payload == entry.stored_payload;
                  })
                : plan.data_regions.end();

            if (shared != plan.data_regions.end()) {
                auto payload_offset = checked_u32(shared->offset);
                if (!payload_offset.has_value()) {
                    return failure<bsa_write_plan>(payload_offset.error());
                }
                entry.payload_offset = payload_offset.value();
                entry.data_region_id = shared->id;
            } else {
                auto payload_offset = checked_u32(payload_cursor);
                if (!payload_offset.has_value()) {
                    return failure<bsa_write_plan>(payload_offset.error());
                }
                entry.payload_offset = payload_offset.value();
                entry.data_region_id = static_cast<std::uint32_t>(plan.data_regions.size());
                plan.data_regions.push_back(planned_bsa_data_region{entry.data_region_id, payload_cursor, entry.stored_payload.size(), entry.unpacked_size,
                                                                    entry.compression, entry.stored_payload});
                if (!checked_add(payload_cursor, entry.stored_payload.size(), payload_cursor)) {
                    return failure<bsa_write_plan>(writer_layout_overflow());
                }
            }
        }
    }

    plan.table_regions.push_back(planned_bsa_table_region{"tes4 header", 0, detail::header_size});
    plan.table_regions.push_back(planned_bsa_table_region{"tes4 folder records", detail::header_size, folder_records_size});
    plan.table_regions.push_back(planned_bsa_table_region{"tes4 folder blocks", detail::header_size + folder_records_size, folder_blocks_size});
    plan.table_regions.push_back(planned_bsa_table_region{"tes4 file names", detail::header_size + folder_records_size + folder_blocks_size, file_names_size});

    auto folder_count = checked_u32(folders.size());
    auto total_folder_name_length = checked_u32(folder_names_size);
    auto total_file_name_length = checked_u32(file_names_size);
    if (!folder_count.has_value()) {
        return failure<bsa_write_plan>(folder_count.error());
    }
    if (!total_folder_name_length.has_value()) {
        return failure<bsa_write_plan>(total_folder_name_length.error());
    }
    if (!total_file_name_length.has_value()) {
        return failure<bsa_write_plan>(total_file_name_length.error());
    }

    append_u32(plan.table_bytes, detail::bsa_magic);
    append_u32(plan.table_bytes, version_for(target));
    append_u32(plan.table_bytes, static_cast<std::uint32_t>(detail::header_size));
    append_u32(plan.table_bytes, plan.flags);
    append_u32(plan.table_bytes, folder_count.value());
    append_u32(plan.table_bytes, file_count);
    // TES4 header offset 24 stores folder-name bytes only, not the file records that share the folder block region.
    append_u32(plan.table_bytes, total_folder_name_length.value());
    append_u32(plan.table_bytes, total_file_name_length.value());
    append_u32(plan.table_bytes, file_flags);

    for (const auto& folder : folders) {
        append_u64(plan.table_bytes, folder.hash);
        append_u32(plan.table_bytes, static_cast<std::uint32_t>(folder.entries.size()));
        if (target == bsa_write_target::skyrim_se_ae_v105) {
            append_u32(plan.table_bytes, 0);
            append_u64(plan.table_bytes, folder.block_offset);
        } else {
            auto block_offset = checked_u32(folder.block_offset);
            if (!block_offset.has_value()) {
                return failure<bsa_write_plan>(block_offset.error());
            }
            append_u32(plan.table_bytes, block_offset.value());
        }
    }

    for (const auto& folder : folders) {
        auto appended_name = append_len8_string(plan.table_bytes, folder.folder);
        if (!appended_name.has_value()) {
            return failure<bsa_write_plan>(appended_name.error());
        }
        for (const auto& entry : folder.entries) {
            append_u64(plan.table_bytes, entry.file_hash);
            append_u32(plan.table_bytes, entry.size_field);
            append_u32(plan.table_bytes, entry.payload_offset);
            plan.entries.push_back(planned_bsa_entry{entry.path,
                                                     "tes4 folder blocks",
                                                     "tes4 file names",
                                                     entry.folder_hash,
                                                     entry.file_hash,
                                                     entry.size_field & detail::file_size_compress,
                                                     entry.payload_offset,
                                                     entry.unpacked_size,
                                                     entry.stored_payload.size(),
                                                     entry.data_region_id,
                                                     entry.compression});
        }
    }

    for (const auto& folder : folders) {
        for (const auto& entry : folder.entries) {
            append_cstring(plan.table_bytes, entry.file);
        }
    }

    if (plan.table_bytes.size() != detail::header_size + folder_records_size + folder_blocks_size + file_names_size) {
        return failure<bsa_write_plan>(writer_layout_overflow());
    }
    plan.total_size = payload_cursor;
    return success(std::move(plan));
}

result<bsa_write_plan> plan_tes3_write(std::span<const bsa_memory_entry> entries, bsa_write_options options)
{
    if (options.archive_default_compressed) {
        return failure<bsa_write_plan>({error_code::unsupported_format, "TES3 BSA does not support compression"});
    }
    if (options.embedded_names) {
        return failure<bsa_write_plan>({error_code::unsupported_format, "TES3 BSA does not support embedded names"});
    }

    auto normalized = normalize_entries(entries);
    if (!normalized.has_value()) {
        return failure<bsa_write_plan>(normalized.error());
    }

    std::vector<planned_tes3_entry> planned_entries;
    planned_entries.reserve(normalized.value().size());
    for (const auto& entry : normalized.value()) {
        const auto compression = resolve_write_compression(archive_format::tes3_bsa, entry.compression, false);
        if (!compression.has_value()) {
            return failure<bsa_write_plan>(compression.error());
        }
        if (compression.value() != compression_state::raw) {
            return failure<bsa_write_plan>({error_code::unsupported_format, "TES3 BSA does not support compression"});
        }
        auto size = checked_u32(entry.payload.size());
        if (!size.has_value()) {
            return failure<bsa_write_plan>(size.error());
        }

        planned_tes3_entry planned{};
        planned.path = entry.path;
        planned.hash = detail::hash_tes3_path(planned.path);
        planned.size = size.value();
        planned.stored_payload = entry.payload;
        planned_entries.push_back(std::move(planned));
    }

    std::sort(planned_entries.begin(), planned_entries.end(), [](const auto& left, const auto& right) {
        if (left.hash != right.hash) {
            return left.hash < right.hash;
        }
        return left.path < right.path;
    });

    std::uint64_t record_table_size = 0;
    std::uint64_t name_offset_table_size = 0;
    std::uint64_t name_block_size = 0;
    if (!checked_mul(detail::tes3_file_record_size, planned_entries.size(), record_table_size) ||
        !checked_mul(detail::tes3_name_offset_size, planned_entries.size(), name_offset_table_size)) {
        return failure<bsa_write_plan>(writer_layout_overflow());
    }
    for (auto& entry : planned_entries) {
        auto name_offset = checked_u32(name_block_size);
        if (!name_offset.has_value()) {
            return failure<bsa_write_plan>(name_offset.error());
        }
        entry.name_offset = name_offset.value();
        if (!checked_add(name_block_size, entry.path.size() + 1U, name_block_size)) {
            return failure<bsa_write_plan>(writer_layout_overflow());
        }
    }

    std::uint64_t hash_offset = 0;
    std::uint64_t hash_table_offset = 0;
    std::uint64_t data_section_offset = 0;
    std::uint64_t hash_table_size = 0;
    if (!checked_add(record_table_size, name_offset_table_size, hash_offset) ||
        !checked_add(hash_offset, name_block_size, hash_offset) ||
        !checked_add(detail::tes3_header_size, hash_offset, hash_table_offset) ||
        !checked_mul(detail::tes3_hash_size, planned_entries.size(), hash_table_size) ||
        !checked_add(hash_table_offset, hash_table_size, data_section_offset)) {
        return failure<bsa_write_plan>(writer_layout_overflow());
    }

    auto hash_offset_u32 = checked_u32(hash_offset);
    auto file_count_u32 = checked_u32(planned_entries.size());
    if (!hash_offset_u32.has_value()) {
        return failure<bsa_write_plan>(hash_offset_u32.error());
    }
    if (!file_count_u32.has_value()) {
        return failure<bsa_write_plan>(file_count_u32.error());
    }

    bsa_write_plan plan{};
    plan.target = bsa_write_target::tes3_morrowind;
    plan.options = options;

    std::uint64_t payload_cursor = data_section_offset;
    for (auto& entry : planned_entries) {
        const auto shared = options.deduplicate
            ? std::find_if(plan.data_regions.begin(), plan.data_regions.end(), [&entry](const auto& region) {
                  return region.stored_payload == entry.stored_payload;
              })
            : plan.data_regions.end();

        if (shared != plan.data_regions.end()) {
            entry.payload_offset = shared->offset;
            entry.data_region_id = shared->id;
        } else {
            entry.payload_offset = payload_cursor;
            entry.data_region_id = static_cast<std::uint32_t>(plan.data_regions.size());
            plan.data_regions.push_back(planned_bsa_data_region{entry.data_region_id,
                                                                entry.payload_offset,
                                                                entry.stored_payload.size(),
                                                                entry.size,
                                                                compression_state::raw,
                                                                entry.stored_payload});
            if (!checked_add(payload_cursor, entry.stored_payload.size(), payload_cursor)) {
                return failure<bsa_write_plan>(writer_layout_overflow());
            }
        }

        if (entry.payload_offset < data_section_offset) {
            return failure<bsa_write_plan>(writer_layout_overflow());
        }
        auto relative_offset = checked_u32(entry.payload_offset - data_section_offset);
        if (!relative_offset.has_value()) {
            return failure<bsa_write_plan>(relative_offset.error());
        }
        entry.relative_offset = relative_offset.value();
    }

    plan.table_regions.push_back(planned_bsa_table_region{"tes3 header", 0, detail::tes3_header_size});
    plan.table_regions.push_back(planned_bsa_table_region{"tes3 file records", detail::tes3_header_size, record_table_size});
    plan.table_regions.push_back(planned_bsa_table_region{"tes3 name offsets", detail::tes3_header_size + record_table_size, name_offset_table_size});
    plan.table_regions.push_back(planned_bsa_table_region{"tes3 names", detail::tes3_header_size + record_table_size + name_offset_table_size, name_block_size});
    plan.table_regions.push_back(planned_bsa_table_region{"tes3 hash table", hash_table_offset, hash_table_size});

    append_u32(plan.table_bytes, detail::magic_tes3);
    append_u32(plan.table_bytes, hash_offset_u32.value());
    append_u32(plan.table_bytes, file_count_u32.value());
    for (const auto& entry : planned_entries) {
        append_u32(plan.table_bytes, entry.size);
        append_u32(plan.table_bytes, entry.relative_offset);
    }
    for (const auto& entry : planned_entries) {
        append_u32(plan.table_bytes, entry.name_offset);
    }
    for (const auto& entry : planned_entries) {
        append_cstring(plan.table_bytes, entry.path);
    }
    for (const auto& entry : planned_entries) {
        append_tes3_hash(plan.table_bytes, entry.hash);
        plan.entries.push_back(planned_bsa_entry{entry.path,
                                                 "tes3 file records",
                                                 "tes3 names",
                                                 0,
                                                 entry.hash,
                                                 0,
                                                 entry.payload_offset,
                                                 entry.size,
                                                 entry.stored_payload.size(),
                                                 entry.data_region_id,
                                                 compression_state::raw});
    }

    if (plan.table_bytes.size() != data_section_offset) {
        return failure<bsa_write_plan>(writer_layout_overflow());
    }
    plan.total_size = payload_cursor;
    return success(std::move(plan));
}

result<void> write_chunk(byte_sink& sink, std::span<const std::byte> bytes)
{
    auto written = sink.write(bytes);
    if (!written.has_value()) {
        return failure<void>(written.error());
    }
    return success();
}

} // namespace

result<bsa_write_plan> plan_bsa_write(bsa_write_target target,
                                      std::span<const bsa_memory_entry> entries,
                                      bsa_write_options options)
{
    switch (target) {
    case bsa_write_target::oblivion_v103:
    case bsa_write_target::fo3_fnv_skyrim_le_v104:
    case bsa_write_target::skyrim_se_ae_v105:
        return plan_tes4_write(target, entries, options);
    case bsa_write_target::tes3_morrowind:
        return plan_tes3_write(entries, options);
    }
    return failure<bsa_write_plan>({error_code::unsupported_format, "unsupported BSA writer target"});
}

result<bsa_write_plan> plan_bsa_write_from_disk(bsa_write_target target,
                                                std::span<const bsa_disk_entry> entries,
                                                bsa_write_options options)
{
    auto paths_validated = validate_disk_entry_archive_paths(entries);
    if (!paths_validated.has_value()) {
        return failure<bsa_write_plan>(paths_validated.error());
    }

    std::vector<bsa_memory_entry> memory_entries;
    memory_entries.reserve(entries.size());
    for (const auto& entry : entries) {
        std::ifstream stream{entry.host_path, std::ios::binary};
        if (!stream) {
            return failure<bsa_write_plan>({error_code::io_failure, "failed to open BSA writer input file"});
        }
        std::vector<std::byte> payload;
        char ch = 0;
        while (stream.get(ch)) {
            payload.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
        }
        if (!stream.eof()) {
            return failure<bsa_write_plan>({error_code::io_failure, "failed to read BSA writer input file"});
        }
        memory_entries.push_back(bsa_memory_entry{entry.path, std::move(payload), entry.compression});
    }
    return plan_bsa_write(target, std::span<const bsa_memory_entry>{memory_entries}, options);
}

result<void> finalize_bsa_write(const bsa_write_plan& plan, byte_sink& sink)
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
