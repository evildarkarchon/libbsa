#pragma once

#include <string_view>

#include <libbsa/result.hpp>

namespace libbsa {

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
