#pragma once

#include <libbsa/result.hpp>

#include <filesystem>
#include <string_view>

namespace libbsa::detail {

/// Stores the resolved native host path shared by reader and writer file I/O
/// seams.
struct host_file_path {
    std::filesystem::path resolved;
};

/// Resolves caller-provided UTF-8 text once so later seams reuse a
/// Windows-native path for file I/O.
result<host_file_path> resolve_host_file_path(std::string_view host_path);

}  // namespace libbsa::detail
