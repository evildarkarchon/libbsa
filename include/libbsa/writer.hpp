#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

#include <libbsa/result.hpp>

namespace libbsa {

/// TES4-family BSA archive target profiles supported by the write-new API.
///
/// The selected profile controls archive version and target-specific default
/// compression behavior without exposing raw archive flag bits to callers.
enum class tes4_bsa_target {
  oblivion,
  fallout3,
  skyrim_se,
};

/// Archive-wide compression policy applied before per-entry overrides.
///
/// `target_default` lets libbsa choose the target profile default while
/// `all_raw` and `all_compressed` provide explicit compatibility-safe modes.
enum class archive_compression_policy {
  target_default,
  all_raw,
  all_compressed,
};

/// Per-entry compression override relative to the archive-wide policy.
///
/// `compressed` maps to the selected target profile's supported codec; codec
/// tuning remains internal so public headers stay dependency-light.
enum class entry_compression_policy {
  inherit,
  raw,
  compressed,
};

/// Options controlling TES4-family write-new archive finalization.
struct tes4_bsa_writer_options {
  /// Archive-wide compression behavior used by entries whose policy is `inherit`.
  archive_compression_policy compression_policy{archive_compression_policy::target_default};

  /// Emits target-compatible embedded file-name payload prefixes when true.
  bool embed_file_names{false};

  /// Shares identical stored payload regions only when explicitly enabled.
  bool deduplicate_payloads{false};

  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing{false};
};

/// Public writer for creating new TES4-family BSA archives.
///
/// Entries are added with explicit archive-internal paths and finalized to a
/// host-path archive. Memory-buffer entries are copied into writer-owned state.
class tes4_bsa_writer {
 public:
  /// Creates a writer for `target` using default writer options.
  explicit tes4_bsa_writer(tes4_bsa_target target);

  /// Creates a writer for `target` using the supplied compatibility options.
  explicit tes4_bsa_writer(tes4_bsa_target target, tes4_bsa_writer_options options);

  /// Returns the target profile selected for this writer.
  [[nodiscard]] tes4_bsa_target target() const noexcept;

  /// Returns the immutable writer options selected at construction time.
  [[nodiscard]] const tes4_bsa_writer_options& options() const noexcept;

  /// Adds a host-file payload with an explicit archive-internal path.
  ///
  /// Implementations validate both paths and report expected I/O or format
  /// failures through `result<void>` instead of throwing for caller data errors.
  result<void> add_file(std::string_view archive_path,
                        std::string_view host_path,
                        entry_compression_policy compression = entry_compression_policy::inherit);

  /// Adds bytes copied from caller memory with an explicit archive-internal path.
  ///
  /// The writer owns an independent copy after this call, so callers may release
  /// or mutate the original memory before `write_to` is called.
  result<void> add_bytes(std::string_view archive_path,
                         std::span<const std::byte> bytes,
                         entry_compression_policy compression = entry_compression_policy::inherit);

  /// Finalizes the writer state into a new archive at `host_path`.
  ///
  /// Existing destinations fail unless `tes4_bsa_writer_options::overwrite_existing`
  /// was enabled, and compression or I/O failures are returned as structured errors.
  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;

  std::shared_ptr<state> state_;
};

} // namespace libbsa
