#pragma once

#include <libbsa/result.hpp>

#include <string>
#include <string_view>

namespace libbsa::detail {

/// Canonical archive virtual path key used for internal lookup and hashing.
struct archive_path_key {
  std::string value;
};

/// Normalizes an archive-internal path to a lowercase forward-slash key.
///
/// Rejects host-rooted or traversal-like names because archive keys must remain
/// virtual paths, not filesystem paths interpreted by the host platform.
result<archive_path_key> normalize_archive_path(std::string_view input);

} // namespace libbsa::detail
