#include "formats/bsa/tes4_bsa_layout.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::bsa {

namespace {

struct tes4_table_lengths {
    std::uint32_t total_folder_name_length{0};
    std::uint32_t total_file_name_length{0};
    std::uint32_t file_count{0};
};

struct tes4_dedupe_identity {
    std::uint32_t stored_size{0};
    std::uint64_t fingerprint{0};

    bool operator<(const tes4_dedupe_identity& other) const noexcept {
        if (stored_size != other.stored_size) {
            return stored_size < other.stored_size;
        }
        return fingerprint < other.fingerprint;
    }
};

bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
    if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
        return false;
    }
    total = lhs + rhs;
    return true;
}

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
        std::uint64_t folder_name_length = 0;
        if (!add_fits_u64(static_cast<std::uint64_t>(folder.name.size()), 2U, folder_name_length) ||
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

    std::uint64_t payload_cursor = 0;
    if (!add_fits_u64(tes4_bsa_header_size, folder_records_size, payload_cursor) ||
        !add_fits_u64(payload_cursor, folder_blocks_size, payload_cursor) ||
        !add_fits_u64(payload_cursor, plan.total_file_name_length, payload_cursor)) {
        return error{error_code::format_error, "TES4 BSA metadata size overflows"};
    }

    std::map<tes4_dedupe_identity, std::vector<std::size_t>> candidate_buckets;
    for (std::size_t folder_index = 0U; folder_index < folders.size(); ++folder_index) {
        auto& prepared_entries = folders[folder_index].entries;
        auto& placed_entries = plan.folders[folder_index].entries;
        for (std::size_t entry_index = 0U; entry_index < prepared_entries.size(); ++entry_index) {
            auto& payload = prepared_entries[entry_index].payload;
            auto stored_size = checked_stored_size(payload.size());
            if (!stored_size) {
                return stored_size.error();
            }

            std::size_t payload_index = plan.payloads.size();
            std::optional<tes4_dedupe_identity> identity;
            if (options.deduplicate_payloads) {
                identity.emplace(tes4_dedupe_identity{stored_size.value(), payload.fingerprint()});
                const auto bucket = candidate_buckets.find(*identity);
                if (bucket != candidate_buckets.end()) {
                    // Fingerprints are only narrowing keys; prefix bytes and
                    // snapshot bodies both participate in authoritative equality.
                    for (const auto candidate_index : bucket->second) {
                        auto equal = payload.exactly_equals(plan.payloads[candidate_index].payload);
                        if (!equal) {
                            return equal.error();
                        }
                        if (equal.value()) {
                            payload_index = candidate_index;
                            break;
                        }
                    }
                }
            }

            if (payload_index == plan.payloads.size()) {
                auto offset = checked_u32(payload_cursor, "TES4 BSA payload offset");
                if (!offset) {
                    return offset.error();
                }
                plan.payloads.push_back(tes4_payload_placement{offset.value(), stored_size.value(),
                                                               std::move(payload)});
                if (identity.has_value()) {
                    candidate_buckets[*identity].push_back(payload_index);
                }
                if (!add_fits_u64(payload_cursor, stored_size.value(), payload_cursor)) {
                    return error{error_code::format_error, "TES4 BSA payload span overflows"};
                }
            }

            placed_entries[entry_index].payload_index = payload_index;
        }
    }

    return plan;
}

}  // namespace libbsa::formats::bsa
