#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

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

/// Archive-level metadata exposed by an opened reader.
///
/// The structure is intentionally limited to stable Phase 3 fields: container
/// type, archive variant/version, raw archive flags, file count, and the default
/// compression behavior advertised by the archive family.
struct archive_metadata {
  archive_type type;
  archive_variant variant;
  std::uint32_t version;
  std::uint32_t archive_flags;
  std::uint32_t file_count;
  entry_compression default_compression;
};

/// Entry-level metadata exposed for lookup, listing, and extraction.
///
/// `path` is the canonical normalized lookup key. `original_path` preserves the
/// archive-derived display spelling joined with `/` separators, independent of
/// host filesystem path rules.
struct entry_metadata {
  std::string path;
  std::string original_path;
  std::uint64_t raw_size;
  std::uint64_t stored_size;
  std::uint64_t payload_offset;
  std::uint64_t tes4_hash;
  entry_compression compression;
  std::uint32_t record_flags;
  bool has_embedded_name;
  std::uint32_t embedded_name_prefix_size;
};

/// Synchronous sink used by archive extraction APIs.
///
/// Implementations must return the number of bytes accepted from `bytes`. A
/// future extractor treats partial acceptance as `error_code::io_error` so
/// callers never observe ambiguous partial-success extraction.
class payload_sink {
 public:
  virtual ~payload_sink() = default;

  /// Writes a chunk of payload bytes and returns the accepted byte count.
  virtual result<std::size_t> write(std::span<const std::byte> bytes) = 0;
};

/// Minimal public archive reader facade for Phase 1.
///
/// The class establishes the future read/open API shape without implementing
/// archive detection or parsing yet. Use `open()` for fallible construction;
/// default construction exists only so successful future opens can return a
/// value object.
class archive_reader {
 public:
  /// Attempts to open an archive from a host path string.
  ///
  /// Phase 1 deliberately returns `error_code::unsupported` for non-empty paths
  /// because real format detection begins in later phases. Empty paths are
  /// rejected as `error_code::invalid_argument`.
  static result<archive_reader> open(std::string_view host_path);
};

} // namespace libbsa
