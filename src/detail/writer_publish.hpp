#pragma once

#include <libbsa/result.hpp>

#include <filesystem>
#include <string_view>
#include <utility>

namespace libbsa::detail
{

  result<void> validate_writer_output_path_before_publish(const std::filesystem::path &output_path,
                                                          bool overwrite_existing,
                                                          std::string_view diagnostic_prefix);

  result<std::filesystem::path> reserve_writer_publish_directory(const std::filesystem::path &output_path,
                                                                 std::string_view diagnostic_prefix);

  void cleanup_writer_publish_directory(const std::filesystem::path &temp_dir) noexcept;

  result<void> publish_completed_writer_output(const std::filesystem::path &temp_path,
                                               const std::filesystem::path &output_path,
                                               bool overwrite_existing,
                                               std::string_view diagnostic_prefix);

  /// Writes an archive to an isolated temporary path, then publishes that completed file to the final host path.
  ///
  /// The callback receives the reserved temporary file path and must return `result<void>` after writing a complete
  /// archive there. The helper owns existence/race checks, atomic/no-replace publication, and best-effort cleanup.
  template <typename WriteTemp>
  result<void> publish_writer_output(const std::filesystem::path &output_path,
                                     bool overwrite_existing,
                                     std::string_view diagnostic_prefix,
                                     WriteTemp &&write_temp)
  {
    auto validated_output = validate_writer_output_path_before_publish(output_path, overwrite_existing, diagnostic_prefix);
    if (!validated_output)
    {
      return validated_output.error();
    }

    auto temp_dir = reserve_writer_publish_directory(output_path, diagnostic_prefix);
    if (!temp_dir)
    {
      return temp_dir.error();
    }

    const auto temp_path = temp_dir.value() / output_path.filename();
    auto written = std::forward<WriteTemp>(write_temp)(temp_path);
    if (!written)
    {
      cleanup_writer_publish_directory(temp_dir.value());
      return written.error();
    }

    auto published = publish_completed_writer_output(temp_path, output_path, overwrite_existing, diagnostic_prefix);
    if (!published)
    {
      cleanup_writer_publish_directory(temp_dir.value());
      return published.error();
    }

    cleanup_writer_publish_directory(temp_dir.value());
    return {};
  }

} // namespace libbsa::detail
