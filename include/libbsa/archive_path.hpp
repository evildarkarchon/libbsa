#pragma once

#include <libbsa/result.hpp>

#include <string>

namespace libbsa {

/// Owns a normalized Bethesda archive-virtual path.
///
/// Archive paths use forward slashes, ASCII-lowercase path components, and are
/// always relative to archive root rather than host filesystem roots.
class archive_path {
public:
    /// Returns the normalized UTF-8 path string owned by this value.
    [[nodiscard]] const std::string& string() const noexcept;

    /// Compares normalized path strings for equality.
    [[nodiscard]] friend bool operator==(const archive_path& lhs, const archive_path& rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }

    /// Orders archive paths lexicographically by normalized string.
    [[nodiscard]] friend bool operator<(const archive_path& lhs, const archive_path& rhs) noexcept
    {
        return lhs.value_ < rhs.value_;
    }

private:
    explicit archive_path(std::string value);

    std::string value_;

    friend result<archive_path> normalize_archive_path(std::string input);
};

/// Normalizes and validates caller-provided archive path text.
///
/// Returns `error_code::malformed_archive` for empty, absolute, UNC-like, or
/// traversal-containing paths. The resulting value is independent of host
/// filesystem path rules.
[[nodiscard]] result<archive_path> normalize_archive_path(std::string input);

} // namespace libbsa
