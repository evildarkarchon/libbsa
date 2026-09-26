#pragma once

#include <libbsa/result.hpp>

#include <filesystem>
#include <string_view>

namespace libbsa::cli {

/// Decodes a nonempty UTF-8 host or archive path for CLI filesystem operations.
/// Returns invalid_argument for empty, embedded-NUL, or invalid UTF-8 input.
result<std::filesystem::path> path_from_utf8(std::string_view utf8_path);

/// Rejects a Windows reparse point while preserving the caller's diagnostic context.
/// Missing paths are accepted; inspection failures and reparse points return io_error.
result<void> reject_reparse_point(const std::filesystem::path& path, std::string_view context);

}  // namespace libbsa::cli
