#include "formats/bsa/tes4_bsa_layout.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/parser_primitives.hpp>
#include <detail/payload_placement.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::bsa {

namespace {

// Table and record geometry below is all 64-bit accumulation. It uses the
// shared overflow-checked add rather than a private copy, which is the same
// primitive the Payload Placement module applies to the payload cursor.
using detail::add_fits_u64;

struct tes4_table_lengths {
    std::uint32_t total_folder_name_length{0};
    std::uint32_t total_file_name_length{0};
    std::uint32_t file_count{0};
};

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds uint32_t limits"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint32_t> checked_stored_size(std::uint64_t value) {
    if (value > std::numeric_limits<std::uint32_t>::max() ||
        (value & tes4_bsa_file_size_compression_toggle) != 0U) {
        // The compression toggle occupies bit 30 of the serialized size field,
        // so an otherwise uint32-sized payload can still be unrepresentable.
        return error{error_code::format_error,
                     "TES4 BSA stored payload size exceeds size-flag limits"};
    }
    return static_cast<std::uint32_t>(value);
}

result<std::uint8_t> checked_name_size(std::size_t size, std::string_view description) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds BSA name length limits"};
    }
    return static_cast<std::uint8_t>(size);
}

std::uint32_t archive_flags_for(const tes4_bsa_profile& profile,
                                const tes4_bsa_writer_options& options) noexcept {
    std::uint32_t archive_flags =
        tes4_bsa_archive_include_directory_names | tes4_bsa_archive_include_file_names;
    if (profile.archive_default_compressed(options.compression_policy)) {
        archive_flags |= tes4_bsa_archive_compress_by_default;
    }
    if (profile.writer_emits_embedded_names(options)) {
        archive_flags |= tes4_bsa_archive_embed_names;
    }
    return archive_flags;
}

result<tes4_table_lengths> calculate_table_lengths(
    const std::vector<tes4_prepared_folder>& folders) {
    std::uint64_t total_folder_name_length64 = 0;
    std::uint64_t total_file_name_length64 = 0;
    std::uint64_t file_count64 = 0;
    for (const auto& folder : folders) {
        // Name plus null terminator only. The one-byte bzstring length prefix is
        // written to the folder-name table but is deliberately not counted here,
        // matching wbBSArchive.pas:1469 ("+ terminator only, length prefix is not
        // counted") and every retail archive header.
        std::uint64_t folder_name_length = 0;
        if (!add_fits_u64(static_cast<std::uint64_t>(folder.name.size()), 1U, folder_name_length) ||
            !add_fits_u64(total_folder_name_length64, folder_name_length,
                          total_folder_name_length64) ||
            !add_fits_u64(file_count64, folder.entries.size(), file_count64)) {
            return error{error_code::format_error, "TES4 BSA table size overflows"};
        }
        auto folder_name_size = checked_name_size(folder.name.size() + 1U, "TES4 BSA folder name");
        if (!folder_name_size) {
            return folder_name_size.error();
        }
        for (const auto& entry : folder.entries) {
            auto file_name_size =
                checked_u32(static_cast<std::uint64_t>(entry.file_name.size()) + 1U,
                            "TES4 BSA file name length");
            if (!file_name_size) {
                return file_name_size.error();
            }
            if (!add_fits_u64(total_file_name_length64, file_name_size.value(),
                              total_file_name_length64)) {
                return error{error_code::format_error, "TES4 BSA file-name table size overflows"};
            }
        }
    }

    auto total_folder_name_length =
        checked_u32(total_folder_name_length64, "TES4 BSA folder-name table size");
    auto total_file_name_length =
        checked_u32(total_file_name_length64, "TES4 BSA file-name table size");
    auto file_count = checked_u32(file_count64, "TES4 BSA file count");
    if (!total_folder_name_length || !total_file_name_length || !file_count) {
        return !total_folder_name_length ? total_folder_name_length.error()
                                         : (!total_file_name_length ? total_file_name_length.error()
                                                                    : file_count.error());
    }

    return tes4_table_lengths{total_folder_name_length.value(), total_file_name_length.value(),
                              file_count.value()};
}

}  // namespace

result<tes4_placement_plan> tes4_plan_placements(std::vector<tes4_prepared_folder> folders,
                                                 const tes4_bsa_profile& profile,
                                                 const tes4_bsa_writer_options& options,
                                                 std::uint32_t file_flags) {
    auto table_lengths = calculate_table_lengths(folders);
    if (!table_lengths) {
        return table_lengths.error();
    }
    auto folder_count = checked_u32(folders.size(), "TES4 BSA folder count");
    if (!folder_count) {
        return folder_count.error();
    }

    tes4_placement_plan plan;
    plan.version = profile.version();
    plan.archive_flags = archive_flags_for(profile, options);
    plan.file_flags = file_flags;
    plan.folder_record_shape = profile.folder_record_shape();
    plan.total_folder_name_length = table_lengths.value().total_folder_name_length;
    plan.total_file_name_length = table_lengths.value().total_file_name_length;
    plan.file_count = table_lengths.value().file_count;
    plan.folders.reserve(folders.size());

    const auto folder_record_size = static_cast<std::uint64_t>(profile.folder_record_size());
    if (folders.size() > std::numeric_limits<std::uint64_t>::max() / folder_record_size) {
        return error{error_code::format_error, "TES4 BSA folder record table size overflows"};
    }
    const std::uint64_t folder_records_size = folder_record_size * folders.size();
    std::uint64_t folder_block_cursor = 0;
    if (!add_fits_u64(tes4_bsa_header_size, folder_records_size, folder_block_cursor)) {
        return error{error_code::format_error, "TES4 BSA folder record table size overflows"};
    }
    std::uint64_t folder_blocks_size = 0;

    for (auto& folder : folders) {
        std::uint64_t folder_name_size = 0;
        if (!add_fits_u64(static_cast<std::uint64_t>(folder.name.size()), 2U, folder_name_size)) {
            return error{error_code::format_error, "TES4 BSA folder name size overflows"};
        }
        if (folder.entries.size() >
            std::numeric_limits<std::uint64_t>::max() / tes4_bsa_file_record_size) {
            return error{error_code::format_error, "TES4 BSA file record table size overflows"};
        }
        const auto file_record_bytes =
            static_cast<std::uint64_t>(folder.entries.size()) * tes4_bsa_file_record_size;

        std::uint64_t folder_block_offset = 0;
        // BSArchPro-compatible offsets include the later file-name table even
        // though folder block bytes are physically serialized before that table.
        if (!add_fits_u64(folder_block_cursor, plan.total_file_name_length, folder_block_offset)) {
            return error{error_code::format_error, "TES4 BSA folder offset overflows"};
        }
        if (profile.folder_record_shape() == tes4_folder_record_shape::legacy_32_bit_offset) {
            auto narrowed = checked_u32(folder_block_offset, "TES4 BSA folder offset");
            if (!narrowed) {
                return narrowed.error();
            }
        }

        std::uint64_t folder_block_size = 0;
        if (!add_fits_u64(folder_name_size, file_record_bytes, folder_block_size) ||
            !add_fits_u64(folder_blocks_size, folder_block_size, folder_blocks_size) ||
            !add_fits_u64(folder_block_cursor, folder_block_size, folder_block_cursor)) {
            return error{error_code::format_error, "TES4 BSA folder block size overflows"};
        }

        tes4_placed_folder placed_folder;
        placed_folder.name = std::move(folder.name);
        placed_folder.hash = folder.hash;
        placed_folder.folder_block_offset = folder_block_offset;
        placed_folder.entries.reserve(folder.entries.size());
        for (auto& entry : folder.entries) {
            placed_folder.entries.push_back(tes4_placed_entry{
                std::move(entry.file_name), entry.file_hash, entry.record_flags, 0U});
        }
        plan.folders.push_back(std::move(placed_folder));
    }

    // The payload area starts past the header, the folder record table, every
    // folder block, and the file-name table. The file-name table is included
    // even though it is serialized after the folder blocks, which is the same
    // BSArchPro-compatible accounting the folder block offsets above use.
    std::uint64_t payload_base_offset = 0;
    if (!add_fits_u64(tes4_bsa_header_size, folder_records_size, payload_base_offset) ||
        !add_fits_u64(payload_base_offset, folder_blocks_size, payload_base_offset) ||
        !add_fits_u64(payload_base_offset, plan.total_file_name_length, payload_base_offset)) {
        return error{error_code::format_error, "TES4 BSA metadata size overflows"};
    }

    // Deduplication stays opt-in, so the writer option selects the policy rather
    // than this layout deciding it.
    //
    // TES4-family BSA deliberately stays on unconstrained Payload Placement:
    // its records carry no decode facts beyond their stored bytes, so differing
    // compression already yields differing stored bytes and exact byte equality
    // refuses the share unaided. Manufacturing facts would claim a constraint
    // exists where none does (ADR-0001).
    detail::payload_placer placer{payload_base_offset,
                                  options.deduplicate_payloads
                                      ? detail::payload_sharing_policy::enabled
                                      : detail::payload_sharing_policy::disabled,
                                  "TES4 BSA"};

    for (std::size_t folder_index = 0U; folder_index < folders.size(); ++folder_index) {
        auto& prepared_entries = folders[folder_index].entries;
        auto& placed_entries = plan.folders[folder_index].entries;
        for (std::size_t entry_index = 0U; entry_index < prepared_entries.size(); ++entry_index) {
            auto& payload = prepared_entries[entry_index].payload;
            auto stored_size = checked_stored_size(payload.size());
            if (!stored_size) {
                return stored_size.error();
            }

            // A disabled placer never reads the narrowing key, so keep owned
            // payload fingerprinting lazy on the default dedupe-off path.
            const detail::payload_narrowing_key key{
                stored_size.value(),
                options.deduplicate_payloads ? payload.fingerprint() : std::uint64_t{0}};
            auto placed = placer.place(
                key, detail::payload_placement_subject::of_payload(std::move(payload)));
            if (!placed) {
                return placed.error();
            }

            placed_entries[entry_index].payload_index = placed.value().payload_index.value();
        }
    }

    auto accepted_payloads = std::move(placer).release();
    plan.payloads.reserve(accepted_payloads.size());
    for (auto& accepted : accepted_payloads) {
        // TES4-family file records serialize a UInt32 offset, so the module's
        // 64-bit cursor narrows here, at read-out. Every accepted payload passed
        // `checked_stored_size` before it was placed, so its size is already
        // known to fit the serialized size field.
        auto offset = checked_u32(accepted.offset, "TES4 BSA payload offset");
        if (!offset) {
            return offset.error();
        }
        const auto stored_size = static_cast<std::uint32_t>(accepted.payload.size());
        plan.payloads.push_back(
            tes4_payload_placement{offset.value(), stored_size, std::move(accepted.payload)});
    }

    return plan;
}

}  // namespace libbsa::formats::bsa
