#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
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

/// BA2 GNRL archive target profiles supported by the write-new API.
///
/// The selected profile controls the serialized BA2 version and the default
/// compression metadata used when entries inherit archive-wide policy.
enum class ba2_gnrl_target {
  fallout4,
  starfield_v2,
  starfield_v3,
};

/// BA2 DX10/DDS texture archive target profiles supported by the write-new API.
///
/// The selected profile controls the serialized BA2 texture archive version and
/// target-routed compressed chunk codec while keeping codec details out of the
/// public C++20 header surface.
enum class ba2_dx10_target {
  fallout4,
  starfield_v3,
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

/// Options controlling TES3/Morrowind write-new archive finalization.
struct tes3_bsa_writer_options {
  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing{false};
};

/// Options controlling BA2 GNRL write-new archive finalization.
struct ba2_gnrl_writer_options {
  /// Archive-wide compression behavior used by entries whose policy is `inherit`.
  archive_compression_policy compression = archive_compression_policy::target_default;

  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing = false;

  /// Shares identical stored payload regions only when explicitly enabled.
  bool deduplicate_payloads = false;

  /// Starfield v2/v3 Unknown1 header value; ignored for Fallout 4 v1 targets.
  ///
  /// xEdit/BSArchPro-derived Starfield write defaults use `1`, while callers can
  /// override this compatibility field when preserving known archive metadata.
  std::uint32_t starfield_unknown1 = 1U;

  /// Starfield v2/v3 Unknown2 header value; ignored for Fallout 4 v1 targets.
  std::uint32_t starfield_unknown2 = 0U;

  /// Starfield v3 archive-wide compression method; ignored by v1/v2 targets.
  ///
  /// Method `3` maps compressed GNRL entries to raw LZ4 blocks, while method `0`
  /// maps them to deflate. Unsupported methods fail during finalization.
  std::uint32_t starfield_compression_method = 3U;
};

/// Options controlling BA2 DX10/DDS write-new archive finalization.
struct ba2_dx10_writer_options {
  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing = false;

  /// Shares identical final stored texture chunk payloads only when explicitly enabled.
  bool deduplicate_payloads = false;

  /// Archive-wide decoded byte cap for texture chunk planning.
  ///
  /// `0` selects the reference-derived default chunking behavior; nonzero values
  /// ask the writer to split at mip boundaries where the requested cap is representable.
  std::uint32_t max_decoded_chunk_bytes = 0U;

  /// Starfield v3 Unknown1 header value; ignored for Fallout 4 v1 targets.
  ///
  /// xEdit/BSArchPro-derived Starfield write defaults use `1`, while callers can
  /// override this compatibility field when preserving known archive metadata.
  std::uint32_t starfield_unknown1 = 1U;

  /// Starfield v3 Unknown2 header value; ignored for Fallout 4 v1 targets.
  std::uint32_t starfield_unknown2 = 0U;

  /// Starfield v3 archive-wide compression method; ignored by Fallout 4 v1 targets.
  ///
  /// Method `3` is the default compressed chunk route for Starfield v3 texture
  /// archives, while method `0` remains available for compatibility cases.
  std::uint32_t starfield_compression_method = 3U;
};

/// Per-entry options for BA2 GNRL payload and record metadata.
struct ba2_gnrl_entry_options {
  /// Per-entry compression override relative to the archive-wide policy.
  entry_compression_policy compression = entry_compression_policy::inherit;

  /// Optional advanced BA2 GNRL record-flags override for compatibility cases.
  ///
  /// Hashes, payload offsets, stored sizes, raw sizes, and the `BAADF00D`
  /// sentinel remain writer-owned and are not caller-controlled.
  std::optional<std::uint32_t> record_flags = std::nullopt;
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

/// Public writer for creating new TES3/Morrowind BSA archives.
///
/// TES3 writer output is raw/uncompressed. The public surface intentionally has
/// no compression, dedupe, or embedded-name controls because Morrowind BSA
/// archives in this phase are one raw payload per archive entry.
class tes3_bsa_writer {
 public:
  /// Creates a raw/uncompressed TES3 writer using default writer options.
  tes3_bsa_writer();

  /// Creates a raw/uncompressed TES3 writer using explicit finalization options.
  explicit tes3_bsa_writer(tes3_bsa_writer_options options);

  /// Returns the immutable TES3 writer options selected at construction time.
  [[nodiscard]] const tes3_bsa_writer_options& options() const noexcept;

  /// Adds a host-file payload with an explicit TES3 archive-internal path.
  ///
  /// The archive path and non-empty host path are validated at add time; source
  /// file existence is checked when `write_to` finalizes the archive.
  result<void> add_file(std::string_view archive_path, std::string_view host_path);

  /// Adds bytes copied from caller memory with an explicit TES3 archive-internal path.
  ///
  /// The writer owns an independent copy after this call, so callers may release
  /// or mutate the original memory before `write_to` is called.
  result<void> add_bytes(std::string_view archive_path, std::span<const std::byte> bytes);

  /// Finalizes the writer state into a raw/uncompressed TES3 archive at `host_path`.
  ///
  /// Existing destinations fail unless `tes3_bsa_writer_options::overwrite_existing`
  /// was enabled, and validation or I/O failures are returned as structured errors.
  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;

  std::shared_ptr<state> state_;
};

/// Public writer for creating new BA2 GNRL archives.
///
/// Entries are added with explicit archive-internal paths and finalized only to
/// a host-path archive. Memory-buffer entries are copied into writer-owned state,
/// and codec implementation details stay private behind the selected target profile.
class ba2_gnrl_writer {
 public:
  /// Creates a writer for `target` using default BA2 GNRL writer options.
  explicit ba2_gnrl_writer(ba2_gnrl_target target);

  /// Creates a writer for `target` using the supplied compatibility options.
  explicit ba2_gnrl_writer(ba2_gnrl_target target, ba2_gnrl_writer_options options);

  /// Returns the BA2 GNRL target profile selected for this writer.
  [[nodiscard]] ba2_gnrl_target target() const noexcept;

  /// Returns the immutable BA2 GNRL writer options selected at construction time.
  [[nodiscard]] const ba2_gnrl_writer_options& options() const noexcept;

  /// Adds a host-file payload with an explicit archive-internal path.
  result<void> add_file(std::string_view archive_path,
                        std::string_view host_path,
                        entry_compression_policy compression = entry_compression_policy::inherit);

  /// Adds a host-file payload with explicit BA2 GNRL per-entry options.
  result<void> add_file(std::string_view archive_path,
                        std::string_view host_path,
                        ba2_gnrl_entry_options options);

  /// Adds bytes copied from caller memory with an explicit archive-internal path.
  ///
  /// The writer owns an independent copy after this call, so callers may release
  /// or mutate the original memory before `write_to` is called.
  result<void> add_bytes(std::string_view archive_path,
                         std::span<const std::byte> bytes,
                         entry_compression_policy compression = entry_compression_policy::inherit);

  /// Adds copied memory bytes with explicit BA2 GNRL per-entry options.
  result<void> add_bytes(std::string_view archive_path,
                         std::span<const std::byte> bytes,
                         ba2_gnrl_entry_options options);

  /// Finalizes the writer state into a new BA2 GNRL archive at `host_path`.
  ///
  /// Existing destinations fail unless `ba2_gnrl_writer_options::overwrite_existing`
  /// was enabled, and validation, compression, or I/O failures are returned as
  /// structured errors.
  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;

  std::shared_ptr<state> state_;
};

/// Public writer for creating new BA2 DX10/DDS texture archives.
///
/// Entries are added from DDS host files with explicit archive-internal paths and
/// finalized to a host-path archive. The public DX10 contract is compressed-only
/// at archive level: callers do not choose raw, per-entry, or per-chunk overrides
/// because uncompressed texture archives are not a stable compatibility target.
class ba2_dx10_writer {
 public:
  /// Creates a writer for `target` using default BA2 DX10 writer options.
  explicit ba2_dx10_writer(ba2_dx10_target target);

  /// Creates a writer for `target` using the supplied texture archive options.
  explicit ba2_dx10_writer(ba2_dx10_target target, ba2_dx10_writer_options options);

  /// Returns the BA2 DX10 target profile selected for this writer.
  [[nodiscard]] ba2_dx10_target target() const noexcept;

  /// Returns the immutable BA2 DX10 writer options selected at construction time.
  [[nodiscard]] const ba2_dx10_writer_options& options() const noexcept;

  /// Adds a DDS host-file payload with an explicit archive-internal texture path.
  ///
  /// The DDS file is validated and snapshotted at add time; expected caller-data
  /// failures are reported through `result<void>`.
  result<void> add_file(std::string_view archive_path, std::string_view dds_host_path);

  /// Finalizes the writer state into a new BA2 DX10 archive at `host_path`.
  ///
  /// Existing destinations fail unless `ba2_dx10_writer_options::overwrite_existing`
  /// was enabled, and validation, compression, or I/O failures are returned as
  /// structured errors.
  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;

  std::shared_ptr<state> state_;
};

} // namespace libbsa
