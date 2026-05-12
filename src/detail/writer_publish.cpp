#include <detail/writer_publish.hpp>

#include <detail/atomic_file_ops.hpp>

#include <cstdint>
#include <string>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace libbsa::detail {
namespace {

std::string prefixed_message(std::string_view diagnostic_prefix, std::string_view message) {
  std::string composed{diagnostic_prefix};
  composed += ' ';
  composed += message;
  return composed;
}

result<bool> path_exists_noexcept(const std::filesystem::path& path, std::string_view diagnostic_prefix) {
  std::error_code fs_error;
  const bool exists = std::filesystem::exists(path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, prefixed_message(diagnostic_prefix, "failed to inspect output host path")};
  }
  return exists;
}

result<bool> path_is_reparse_point_noexcept(const std::filesystem::path& path, std::string_view diagnostic_prefix) {
#if defined(_WIN32)
  const auto attributes = GetFileAttributesW(path.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) {
    const auto last_error = GetLastError();
    if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND) {
      return false;
    }
    return error{error_code::io_error, prefixed_message(diagnostic_prefix, "failed to inspect output host path")};
  }

  // Symlinks to files can still look regular through std::filesystem, so check the Windows path itself.
  return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
  (void)path;
  (void)diagnostic_prefix;
  return false;
#endif
}

} // namespace

result<void> validate_writer_output_path_before_publish(const std::filesystem::path& output_path,
                                                        bool overwrite_existing,
                                                        std::string_view diagnostic_prefix) {
  auto output_exists = path_exists_noexcept(output_path, diagnostic_prefix);
  if (!output_exists) {
    return output_exists.error();
  }

  if (!overwrite_existing) {
    if (output_exists.value()) {
      return error{error_code::io_error, prefixed_message(diagnostic_prefix, "output host path already exists")};
    }
    return {};
  }

  auto output_is_reparse_point = path_is_reparse_point_noexcept(output_path, diagnostic_prefix);
  if (!output_is_reparse_point) {
    return output_is_reparse_point.error();
  }
  if (output_is_reparse_point.value()) {
    return error{error_code::io_error, prefixed_message(diagnostic_prefix, "refuses to replace reparse-point output host path")};
  }

  if (output_exists.value()) {
    std::error_code fs_error;
    const bool is_regular = std::filesystem::is_regular_file(output_path, fs_error);
    if (fs_error || !is_regular) {
      return error{error_code::io_error, prefixed_message(diagnostic_prefix, "refuses to replace non-regular output host path")};
    }
  }

  return {};
}

result<std::filesystem::path> reserve_writer_publish_directory(const std::filesystem::path& output_path,
                                                               std::string_view diagnostic_prefix) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-tmp-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    std::error_code fs_error;
    // Isolated directories keep cleanup from touching caller-owned deterministic siblings like `<archive>.tmp`.
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate;
    }
    if (fs_error) {
      return error{error_code::io_error, prefixed_message(diagnostic_prefix, "failed to reserve temporary output directory")};
    }
  }

  return error{error_code::io_error, prefixed_message(diagnostic_prefix, "exhausted temporary output directory names")};
}

void cleanup_writer_publish_directory(const std::filesystem::path& temp_dir) noexcept {
  std::error_code fs_error;
  // Cleanup is best-effort so callers see the primary writer or publish failure.
  std::filesystem::remove_all(temp_dir, fs_error);
}

result<void> publish_completed_writer_output(const std::filesystem::path& temp_path,
                                             const std::filesystem::path& output_path,
                                             bool overwrite_existing,
                                             std::string_view diagnostic_prefix) {
  if (overwrite_existing) {
    auto output_is_reparse_point = path_is_reparse_point_noexcept(output_path, diagnostic_prefix);
    if (!output_is_reparse_point) {
      return output_is_reparse_point.error();
    }
    if (output_is_reparse_point.value()) {
      return error{error_code::io_error, prefixed_message(diagnostic_prefix, "refuses to replace reparse-point output host path")};
    }

    auto published = detail::replace_file_atomically(temp_path, output_path);
    if (!published) {
      return error{error_code::io_error, prefixed_message(diagnostic_prefix, "failed to publish output host path")};
    }
    return {};
  }

  auto published = detail::publish_file_without_replace(temp_path, output_path);
  if (!published) {
    return error{error_code::io_error, prefixed_message(diagnostic_prefix, "failed to publish output host path without overwrite")};
  }
  return {};
}

} // namespace libbsa::detail
