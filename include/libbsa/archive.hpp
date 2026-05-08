#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <libbsa/result.hpp>

namespace libbsa {

/// Supported top-level Bethesda archive container families.
enum class archive_type {
  bsa,
  ba2,
};

/// Public archive variants known to libbsa.
///
/// Phase 3 opens TES4-family BSA variants first; later reader phases can reuse
/// the same metadata field for TES3 and BA2 without changing the reader shape.
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

/// Archive-level metadata exposed by an opened reader.
///
/// The structure is intentionally limited to stable Phase 3 fields: container
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
};

/// Synchronous sink used by archive extraction APIs.
///
/// Implementations must return the number of bytes accepted from `bytes`. The
/// extractor treats partial acceptance as `error_code::io_error` so callers
/// never observe ambiguous partial-success extraction.
class payload_sink {
 public:
  virtual ~payload_sink() = default;

  /// Writes a chunk of payload bytes and returns the accepted byte count.
  virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
};

/// Public archive reader for supported Bethesda archive files.
///
/// Use `open()` for fallible construction. A successfully opened reader exposes
/// archive metadata, deterministic entry listings, canonical path lookup, and
/// synchronous extraction for the archive variants implemented by libbsa.
class archive_reader {
 public:
  /// Attempts to open an archive from a host path string.
  ///
  /// Empty host paths return `error_code::invalid_argument`, missing or
  /// unreadable files return `error_code::io_error`, unsupported archive bytes
  /// return `error_code::unsupported`, and malformed supported archives return
  /// `error_code::format_error`.
  static result<archive_reader> open(std::string_view host_path);

  /// Returns archive-level metadata for a successfully opened archive.
  [[nodiscard]] result<archive_metadata> metadata() const;

  /// Returns deterministic entry metadata sorted by canonical archive path.
  [[nodiscard]] result<std::vector<entry_metadata>> entries() const;

  /// Finds metadata for a normalized archive path if it exists.
  ///
  /// Invalid archive path syntax is reported as `error_code::invalid_argument`;
  /// valid missing paths return an empty optional so lookup can distinguish
  /// absence from malformed caller input.
  [[nodiscard]] result<std::optional<entry_metadata>> find(std::string_view path) const;

  /// Returns whether a valid archive path exists in the opened archive.
  [[nodiscard]] result<bool> contains(std::string_view path) const;

  /// Extracts an entry by archive path into a synchronous caller-owned sink.
  [[nodiscard]] result<void> extract(std::string_view path, payload_sink& sink) const;

  /// Extracts an entry into a bounded in-memory byte vector convenience result.
  [[nodiscard]] result<std::vector<std::byte>> extract_bytes(std::string_view path) const;

 private:
  struct state;

  explicit archive_reader(archive_metadata metadata);

  std::shared_ptr<const state> state_;
};

} // namespace libbsa
