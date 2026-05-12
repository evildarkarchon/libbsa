#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::detail {

/// Multiplies an archive-declared element count by a byte width without wrapping.
bool multiply_fits(std::uint32_t count, std::size_t width, std::size_t& total) noexcept;

/// Adds two archive-derived byte counts without wrapping.
bool add_fits(std::size_t lhs, std::size_t rhs, std::size_t& total) noexcept;

/// Adds two 64-bit archive-derived byte counts without wrapping.
bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept;

/// Returns true when a `size_t` byte span is fully contained in a bounded archive buffer.
bool span_fits(std::size_t start, std::size_t length, std::size_t total) noexcept;

/// Returns true when a 64-bit archive byte span is fully contained in the host file size.
bool span_fits_u64(std::uint64_t start, std::uint64_t length, std::uint64_t total) noexcept;

/// Reads exactly `count` bytes at `offset`, rejecting unrepresentable stream positions and truncation.
///
/// The helper translates byte-buffer allocation failures into `format_error`, reports seek/read failures as
/// `io_error`, and returns a `format_error` when the host file is shorter than the archive metadata declared.
result<std::vector<std::byte>> read_file_bytes_at(std::ifstream& input,
                                                  std::uint64_t offset,
                                                  std::size_t count,
                                                  std::string_view description);

/// Materializes archive-controlled bytes into a string without leaking allocation exceptions.
///
/// `description` names the caller-owned archive field and is preserved in allocation diagnostics so parsers can keep
/// family-specific error messages while sharing the exception boundary.
result<std::string> archive_string_from_bytes(std::span<const std::byte> bytes, std::string_view description);

/// Converts archive display path separators to `/` after a parser has accepted the stored path spelling.
void normalize_display_separators(std::string& value) noexcept;

} // namespace libbsa::detail
