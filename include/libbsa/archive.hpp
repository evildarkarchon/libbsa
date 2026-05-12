#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <libbsa/export.hpp>
#include <libbsa/result.hpp>

namespace libbsa {

/// Supported top-level Bethesda archive container families.
enum class archive_type {
  bsa,
  ba2,
};

/// Public archive variants known to libbsa.
///
/// The same metadata field represents supported BSA and BA2 format families
/// without changing the public reader shape as archive coverage expands.
enum class archive_variant {
  tes3,
  tes4,
  fallout4,
  starfield,
};

/// Compression representation selected by archive metadata for an entry.
///
/// The values describe observable archive payload encoding without exposing the
/// private codec libraries used to implement each mode.
enum class entry_compression {
  none,
  deflate,
  lz4_frame,
  lz4_block,
};

/// BA2-specific archive-level metadata exposed when `archive_metadata::type` is `archive_type::ba2`.
///
/// Starfield BA2 revisions append raw header fields after the common `BTDX`/subtype header.
/// These fields are intentionally version-gated optionals so Fallout 4 archives can distinguish
/// "field absent" from a Starfield field that is present with a zero value.
struct ba2_archive_metadata {
  /// Raw Starfield v2+ header field conventionally named `Unknown1` in current references.
  std::optional<std::uint32_t> starfield_unknown1;

  /// Raw Starfield v2+ header field conventionally named `Unknown2` in current references.
  std::optional<std::uint32_t> starfield_unknown2;

  /// Raw Starfield v3 compression method field; method `3` selects raw LZ4 block payloads.
  std::optional<std::uint32_t> compression_method;
};

/// Per-chunk BA2 texture payload metadata exposed for dependency-light inspection.
///
/// Offsets are archive-absolute and sizes describe the stored chunk bytes plus the decoded
/// payload bytes. The compression route is a libbsa-owned value so callers never depend on
/// private codec or texture-analysis libraries.
struct texture_chunk_metadata {
  /// Archive-absolute byte offset of this chunk's stored payload.
  std::uint64_t payload_offset;

  /// Stored byte count in the archive; zero-sized stored metadata is represented as raw chunks by parsers.
  std::uint32_t stored_size;

  /// Decoded chunk byte count before DDS header reconstruction.
  std::uint32_t raw_size;

  /// First mip level covered by this chunk, inclusive.
  std::uint16_t start_mip;

  /// Last mip level covered by this chunk, inclusive.
  std::uint16_t end_mip;

  /// Compression route selected from archive metadata for this chunk.
  entry_compression compression;
};

/// Dependency-light BA2 texture metadata attached only to texture archive entries.
///
/// The format field stores the raw numeric graphics format identifier used by DDS DXT10
/// metadata, preserving the value without exposing platform or analyzer types. For texture
/// entries, `entry_metadata::raw_size` is the consumer-visible reconstructed DDS size including
/// its header; archive payload sizes remain on the per-chunk metadata below.
struct texture_metadata {
  /// Texture width in pixels.
  std::uint32_t width;

  /// Texture height in pixels.
  std::uint32_t height;

  /// Number of mip levels described by the texture record.
  std::uint32_t mip_count;

  /// Raw numeric DDS DXT10 graphics format identifier.
  std::uint32_t dxgi_format;

  /// Texture array element count represented independently from cubemap state.
  std::uint32_t array_size;

  /// True when the BA2 record represents cubemap texture data.
  bool is_cubemap;

  /// Raw BA2 texture record byte with currently unknown Bethesda-specific semantics.
  std::uint8_t unknown_tex;

  /// Raw BA2 cubemap/array field preserved for compatibility evidence.
  std::uint16_t cube_maps_raw;

  /// Chunk layout and compression metadata in archive order unless a later parser documents otherwise.
  std::vector<texture_chunk_metadata> chunks;
};

/// Archive-level metadata exposed by an opened reader.
///
/// The structure is intentionally limited to stable public fields: container
/// type, archive variant/version, raw archive flags, file count, the default
/// compression behavior advertised by the archive family, and optional
/// format-family metadata that remains dependency-light.
struct archive_metadata {
  archive_type type;
  archive_variant variant;
  std::uint32_t version;
  std::uint32_t archive_flags;
  std::uint32_t file_count;
  entry_compression default_compression;
  std::optional<ba2_archive_metadata> ba2;
};

/// Entry-level metadata exposed for lookup, listing, and extraction.
///
/// `path` is the canonical normalized lookup key. `original_path` preserves the
/// archive-derived display spelling joined with `/` separators, independent of
/// host filesystem path rules. `payload_offset` is always an archive-absolute
/// byte offset for every archive variant; format-specific relative offsets stay
/// inside parser internals and fixture manifests.
struct entry_metadata {
  std::string path;
  std::string original_path;
  std::uint64_t raw_size;
  std::uint64_t stored_size;
  std::uint64_t payload_offset;
  std::uint64_t archive_hash;
  entry_compression compression;
  std::uint32_t record_flags;
  bool has_embedded_name;
  std::uint32_t embedded_name_prefix_size;
  /// Optional texture metadata populated only for BA2 DX10 texture entries.
  std::optional<texture_metadata> texture;
};

/// Synchronous sink used by archive extraction APIs.
///
/// Implementations must return the number of bytes accepted from `bytes`. The
/// extractor treats partial acceptance as `error_code::io_error` so callers
/// never observe ambiguous partial-success extraction.
///
/// Thread-safety: sinks are caller-owned; concurrent extraction requires
/// distinct sinks or caller-owned synchronization for intentionally shared
/// output state. See `docs/thread-safety.md`.
class LIBBSA_API payload_sink {
 public:
  virtual ~payload_sink() = default;

  /// Writes a chunk of payload bytes and returns the accepted byte count.
  virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
};

/// Options controlling a bulk extraction call.
///
/// The default is intentionally serial. Pass a positive value greater than one
/// to opt into parallel extraction of independent entries.
///
/// Thread-safety: options are copied into the extraction call; see
/// `docs/thread-safety.md` for caller mutation rules.
struct bulk_extract_options {
  /// Number of worker threads to use; `0` is invalid and never means "auto".
  std::uint32_t worker_count{1U};
};

/// A single archive path requested from `archive_reader::extract_entries`.
struct bulk_extract_request {
  /// Archive path to extract, using the same lookup rules as `archive_reader::extract`.
  std::string path;
};

/// Factory used by bulk extraction to create sinks for unique requested paths.
///
/// Implementations must return a distinct sink for every successful `create`
/// call. `archive_reader::extract_entries` coalesces duplicate exact request
/// paths before extraction, so duplicate result records mirror the first
/// occurrence and do not trigger additional `create` calls. When
/// `bulk_extract_options::worker_count` is greater than one, `create` may be
/// called concurrently for independent unique paths and the returned sinks may
/// be written on worker threads. libbsa does not call user factory or sink
/// methods while holding an internal mutex.
///
/// Thread-safety: caller-owned factories must protect shared state and return
/// distinct sinks for concurrent unique-path extraction.
class LIBBSA_API bulk_extract_sink_factory {
 public:
  virtual ~bulk_extract_sink_factory() = default;

  /// Creates the sink that will receive the payload for the unique request `path`.
  ///
  /// Returning an error records a per-entry failure and does not abort
  /// independent sibling entries.
  virtual result<std::unique_ptr<payload_sink>> create(std::string_view path, const entry_metadata& entry) = 0;
};

/// Per-request result record returned by `archive_reader::extract_entries`.
///
/// Thread-safety: result records are independent values after the caller-owned
/// result vector is no longer being mutated.
struct bulk_extract_entry_result {
  /// Requested archive path in the same order supplied by the caller.
  std::string path;

  /// Entry metadata when lookup succeeded before extraction.
  std::optional<entry_metadata> entry;

  /// Per-entry failure, if lookup, sink creation, or extraction failed.
  std::optional<error> failure;

  /// Returns true when this record completed without a per-entry failure.
  [[nodiscard]] bool succeeded() const noexcept { return !failure.has_value(); }
};

/// Public archive reader for supported Bethesda archive files.
///
/// Use `open()` for fallible construction. A successfully opened reader exposes
/// archive metadata, deterministic entry listings, canonical path lookup, and
/// synchronous extraction for the archive variants implemented by libbsa.
///
/// Thread-safety: independently opened readers may be used concurrently; one
/// reader may run concurrent const extraction calls only with distinct sinks as
/// described in `docs/thread-safety.md`.
class archive_reader {
 public:
  /// Attempts to open an archive from a host path string.
  ///
  /// Empty host paths return `error_code::invalid_argument`, missing or
  /// unreadable files return `error_code::io_error`, unsupported archive bytes
  /// return `error_code::unsupported`, and malformed supported archives return
  /// `error_code::format_error`.
  static LIBBSA_API result<archive_reader> open(std::string_view host_path);

  /// Returns archive-level metadata for a successfully opened archive.
  [[nodiscard]] LIBBSA_API result<archive_metadata> metadata() const;

  /// Returns deterministic entry metadata sorted by canonical archive path.
  [[nodiscard]] LIBBSA_API result<std::vector<entry_metadata>> entries() const;

  /// Finds metadata for a normalized archive path if it exists.
  ///
  /// Invalid archive path syntax is reported as `error_code::invalid_argument`;
  /// valid missing paths return an empty optional so lookup can distinguish
  /// absence from malformed caller input.
  [[nodiscard]] LIBBSA_API result<std::optional<entry_metadata>> find(std::string_view path) const;

  /// Returns whether a valid archive path exists in the opened archive.
  [[nodiscard]] LIBBSA_API result<bool> contains(std::string_view path) const;

  /// Extracts an entry by archive path into a synchronous caller-owned sink.
  [[nodiscard]] LIBBSA_API result<void> extract(std::string_view path, payload_sink& sink) const;

  /// Extracts an entry into a bounded in-memory byte vector convenience result.
  [[nodiscard]] LIBBSA_API result<std::vector<std::byte>> extract_bytes(std::string_view path) const;

  /// Extracts multiple request records into caller-created unique-path sinks.
  ///
  /// Setup failures such as an unopened reader or `worker_count == 0` fail the
  /// outer result. Lookup, sink-creation, and extraction failures are recorded
  /// on the corresponding request-order result record so independent sibling
  /// entries can still complete. Duplicate exact request paths are coalesced:
  /// the first occurrence owns lookup, sink creation, and extraction, while
  /// later duplicate result records preserve their request path and mirror the
  /// first occurrence's entry and failure outcome.
  [[nodiscard]] LIBBSA_API result<std::vector<bulk_extract_entry_result>> extract_entries(
      std::span<const bulk_extract_request> requests,
      bulk_extract_sink_factory& sink_factory,
      bulk_extract_options options = {}) const;

 private:
  struct state;

  explicit archive_reader(archive_metadata metadata);

  std::shared_ptr<const state> state_;
};

} // namespace libbsa
