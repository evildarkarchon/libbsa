#pragma once

#include <libbsa/result.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace libbsa::detail {

/// Stores the caller's original UTF-8 text for diagnostics while file I/O reuses one resolved host path.
struct host_file_path {
  std::string original_utf8;
  std::filesystem::path resolved;
};

/// Resolves caller-provided UTF-8 text once so later seams keep diagnostics text and Windows path I/O together.
result<host_file_path> resolve_host_file_path(std::string_view host_path);

} // namespace libbsa::detail
