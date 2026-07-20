#pragma once

#include <libbsa/result.hpp>

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <string_view>
#include <utility>

namespace libbsa::detail {

/// Owns all temporary files created while finalizing one archive write.
///
/// The workspace is reserved beside the destination so its completed archive
/// can use same-volume publication. Destruction removes the entire private
/// directory on a best-effort basis after success or any failure.
class finalization_workspace final {
   public:
    /// Reserves a private sibling workspace for one destination.
    static result<finalization_workspace> reserve(const std::filesystem::path& output_path,
                                                  std::string_view diagnostic_prefix);

    /// Cleans the owned workspace without reporting cleanup failures.
    ~finalization_workspace() noexcept;

    finalization_workspace(const finalization_workspace&) = delete;
    finalization_workspace& operator=(const finalization_workspace&) = delete;

    /// Transfers cleanup ownership from `other`.
    finalization_workspace(finalization_workspace&& other) noexcept;

    finalization_workspace& operator=(finalization_workspace&& other) = delete;

    /// Returns the path where the callback must serialize the completed archive.
    [[nodiscard]] const std::filesystem::path& temporary_archive_path() const noexcept;

    /// Returns the deterministic snapshot path for a stable preparation identity.
    ///
    /// Distinct identities map to distinct names without shared mutable state, so
    /// indexed workers may request paths independently of completion order.
    [[nodiscard]] std::filesystem::path snapshot_path(
        std::size_t stable_preparation_identity) const;

   private:
    /// Takes ownership of an already-reserved private workspace directory.
    explicit finalization_workspace(std::filesystem::path workspace_path);

    /// Removes the private directory without changing the primary write result.
    void cleanup() noexcept;

    std::filesystem::path workspace_path_;
    std::filesystem::path temporary_archive_path_;
};

result<void> validate_writer_output_path_before_publish(const std::filesystem::path& output_path,
                                                        bool overwrite_existing,
                                                        std::string_view diagnostic_prefix);

result<void> publish_completed_writer_output(const std::filesystem::path& temp_path,
                                             const std::filesystem::path& output_path,
                                             bool overwrite_existing,
                                             std::string_view diagnostic_prefix);

/// Finalizes an archive in an isolated workspace, then publishes its completed
/// temporary archive to the final host path.
///
/// The callback receives the Finalization Workspace and must perform
/// preparation, layout, and serialization before returning success. The helper
/// validates and reserves the destination before invoking it, then owns
/// atomic/no-replace publication and best-effort workspace cleanup.
template <typename Finalize>
    requires requires(Finalize&& callback, const finalization_workspace& workspace) {
        { std::forward<Finalize>(callback)(workspace) } -> std::same_as<result<void>>;
    }
result<void> publish_writer_output(const std::filesystem::path& output_path,
                                   bool overwrite_existing, std::string_view diagnostic_prefix,
                                   Finalize&& finalize) {
    auto validated_output = validate_writer_output_path_before_publish(
        output_path, overwrite_existing, diagnostic_prefix);
    if (!validated_output) {
        return validated_output.error();
    }

    auto reserved_workspace = finalization_workspace::reserve(output_path, diagnostic_prefix);
    if (!reserved_workspace) {
        return reserved_workspace.error();
    }

    auto workspace = std::move(reserved_workspace).value();
    auto finalized = std::forward<Finalize>(finalize)(workspace);
    if (!finalized) {
        return finalized.error();
    }

    auto published = publish_completed_writer_output(
        workspace.temporary_archive_path(), output_path, overwrite_existing, diagnostic_prefix);
    if (!published) {
        return published.error();
    }

    return {};
}

}  // namespace libbsa::detail
