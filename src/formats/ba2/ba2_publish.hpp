#pragma once

#include <libbsa/result.hpp>

#include <filesystem>
#include <system_error>
#include <utility>

namespace libbsa::formats::ba2::publish_detail {

/// Restores a reserved output backup after a publish failure and returns the publish error.
/// The rename operation is injectable so unit tests can exercise the rollback behavior without
/// compiling environment-variable-controlled fault injection into the production library target.
template <typename RenameFile>
result<void> restore_backup_after_publish_failure(const std::filesystem::path& backup_path,
                                                  const std::filesystem::path& output_path,
                                                  RenameFile&& rename_file) {
  std::error_code rollback_error;
  std::forward<RenameFile>(rename_file)(backup_path, output_path, rollback_error);
  if (rollback_error) {
    return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path and failed to restore backup"};
  }
  return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path"};
}

} // namespace libbsa::formats::ba2::publish_detail
