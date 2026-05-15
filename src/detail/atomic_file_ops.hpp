#pragma once

#include <libbsa/result.hpp>

#include <filesystem>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace libbsa::detail
{

  /// Publishes a completed temporary regular file to its final host path without replacing an existing path.
  /// The operation fails if another actor already owns the destination at publish time.
  inline result<void> publish_file_without_replace(const std::filesystem::path &temp_path,
                                                   const std::filesystem::path &output_path)
  {
#if defined(_WIN32)
    // MoveFileEx without MOVEFILE_REPLACE_EXISTING gives Windows' atomic no-replace publish semantics.
    if (!MoveFileExW(temp_path.c_str(), output_path.c_str(), MOVEFILE_WRITE_THROUGH))
    {
      return error{error_code::io_error, "failed to publish host path without replacing an existing file"};
    }
#else
    std::error_code fs_error;
    // POSIX hard-link creation is atomic with respect to the destination name and fails if it already exists.
    std::filesystem::create_hard_link(temp_path, output_path, fs_error);
    if (fs_error)
    {
      return error{error_code::io_error, "failed to publish host path without replacing an existing file"};
    }
    std::filesystem::remove(temp_path, fs_error);
#endif
    return {};
  }

  /// Replaces an existing host file with a completed temporary regular file using the platform's atomic
  /// replacement primitive. If the primitive fails, the destination name remains owned by the original file.
  inline result<void> replace_file_atomically(const std::filesystem::path &temp_path,
                                              const std::filesystem::path &output_path)
  {
#if defined(_WIN32)
    if (!MoveFileExW(temp_path.c_str(), output_path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
      return error{error_code::io_error, "failed to atomically replace output host path"};
    }
#else
    std::error_code fs_error;
    // POSIX rename replaces the destination atomically without an externally visible missing-name gap.
    std::filesystem::rename(temp_path, output_path, fs_error);
    if (fs_error)
    {
      return error{error_code::io_error, "failed to atomically replace output host path"};
    }
#endif
    return {};
  }

} // namespace libbsa::detail
