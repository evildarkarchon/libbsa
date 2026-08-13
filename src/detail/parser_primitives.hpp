#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace libbsa::detail {

// Internal safety policy for untrusted archive metadata. These are deliberately
// not public options until callers have real-world evidence that larger
// verified-local archives need a configurable override.
inline constexpr std::uint64_t metadata_entry_count_limit = 1'000'000ULL;
inline constexpr std::uint64_t metadata_bsa_folder_count_limit = 65'536ULL;
inline constexpr std::uint64_t metadata_dx10_chunk_count_limit = 1'000'000ULL;

/// Returns a parser metadata allocation error without letting
/// archive-controlled counts throw.
inline error metadata_allocation_error(std::string_view description) {
    return error{error_code::format_error,
                 std::string{description} + " exceeds platform metadata limits"};
}

/// Rejects archive-declared metadata counts that exceed an internal parser
/// safety policy.
result<void> validate_metadata_count(std::uint64_t count, std::uint64_t limit,
                                     std::string_view description);

/// Reserves typed parser metadata vectors while translating allocation failures
/// into libbsa results.
template <typename T>
result<void> reserve_metadata_vector(std::vector<T>& values, std::size_t capacity,
                                     std::string_view description) {
    if (capacity > values.max_size()) {
        return metadata_allocation_error(description);
    }

    try {
        values.reserve(capacity);
    } catch (const std::bad_alloc&) {
        return metadata_allocation_error(description);
    } catch (const std::length_error&) {
        return metadata_allocation_error(description);
    }
    return {};
}

/// Reserves duplicate-detection metadata sets while translating allocation
/// failures into libbsa results.
template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
result<void> reserve_metadata_set(std::unordered_set<Key, Hash, KeyEqual, Allocator>& values,
                                  std::size_t capacity, std::string_view description) {
    if (capacity > values.max_size()) {
        return metadata_allocation_error(description);
    }

    try {
        values.reserve(capacity);
    } catch (const std::bad_alloc&) {
        return metadata_allocation_error(description);
    } catch (const std::length_error&) {
        return metadata_allocation_error(description);
    }
    return {};
}

/// Multiplies an archive-declared element count by a byte width without
/// wrapping.
bool multiply_fits(std::uint32_t count, std::size_t width, std::size_t& total) noexcept;

/// Adds two archive-derived byte counts without wrapping.
bool add_fits(std::size_t lhs, std::size_t rhs, std::size_t& total) noexcept;

/// Adds two 64-bit archive-derived byte counts without wrapping.
bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept;

/// Returns true when a `size_t` byte span is fully contained in a bounded
/// archive buffer.
bool span_fits(std::size_t start, std::size_t length, std::size_t total) noexcept;

/// Returns true when a 64-bit archive byte span is fully contained in the host
/// file size.
bool span_fits_u64(std::uint64_t start, std::uint64_t length, std::uint64_t total) noexcept;

/// Returns true when two non-empty 64-bit archive byte spans intersect without
/// overflowing end offsets.
bool spans_overlap_u64(std::uint64_t first_start, std::uint64_t first_length,
                       std::uint64_t second_start, std::uint64_t second_length) noexcept;

/// Reads exactly `count` bytes at `offset`, rejecting unrepresentable stream
/// positions and truncation.
///
/// The helper translates byte-buffer allocation failures into `format_error`,
/// reports seek/read failures as `io_error`, and returns a `format_error` when
/// the host file is shorter than the archive metadata declared.
result<std::vector<std::byte>> read_file_bytes_at(std::ifstream& input, std::uint64_t offset,
                                                  std::size_t count, std::string_view description);

/// Materializes archive-controlled bytes into a string without leaking
/// allocation exceptions.
///
/// `description` names the caller-owned archive field and is preserved in
/// allocation diagnostics so parsers can keep family-specific error messages
/// while sharing the exception boundary.
result<std::string> archive_string_from_bytes(std::span<const std::byte> bytes,
                                              std::string_view description);

/// Converts archive display path separators to `\` after a parser has accepted
/// the stored path spelling.
///
/// libbsa is Windows-only, so the display spelling reported to consumers uses
/// the platform separator. This is deliberately independent of what a format
/// stores: the BSA families store `\`, while BA2 stores `/` because Bethesda's
/// own packer does (`wbBSArchive.pas:1539-1540`, "archive2.exe uses /"). Every
/// parser funnels its stored spelling through here so one rule governs display
/// regardless of format.
///
/// This never affects lookup. `normalize_archive_path` folds `\` to `/` per
/// character when building the canonical key, so both spellings resolve
/// identically.
void normalize_display_separators(std::string& value) noexcept;

}  // namespace libbsa::detail
