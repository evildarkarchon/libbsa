#pragma once

#include <libbsa/result.hpp>

#include <cstdint>
#include <filesystem>
#include <string>

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

/// Publishes a completed temporary regular file to its final host path without replacing an existing path.
/// The operation fails if another actor already owns the destination at publish time.
inline result<void> publish_file_without_replace(const std::filesystem::path& temp_path,
                                                 const std::filesystem::path& output_path) {
#if defined(_WIN32)
  // MoveFileEx without MOVEFILE_REPLACE_EXISTING gives Windows' atomic no-replace publish semantics.
  if (!MoveFileExW(temp_path.c_str(), output_path.c_str(), MOVEFILE_WRITE_THROUGH)) {
    return error{error_code::io_error, "failed to publish host path without replacing an existing file"};
  }
#else
  std::error_code fs_error;
  // POSIX hard-link creation is atomic with respect to the destination name and fails if it already exists.
  std::filesystem::create_hard_link(temp_path, output_path, fs_error);
  if (fs_error) {
    return error{error_code::io_error, "failed to publish host path without replacing an existing file"};
  }
  std::filesystem::remove(temp_path, fs_error);
#endif
  return {};
}

/// Reserves a unique backup directory and returns the controlled path where the old archive can be moved.
/// Creating the directory reserves the namespace atomically so backup publication cannot clobber caller files.
inline result<std::filesystem::path> reserve_backup_path_in_unique_directory(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-bakdir-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    std::error_code fs_error;
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate / filename;
    }
    if (fs_error) {
      return error{error_code::io_error, "failed to reserve backup directory"};
    }
  }
  return error{error_code::io_error, "exhausted backup directory names"};
}

} // namespace libbsa::detail
