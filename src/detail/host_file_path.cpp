#include <detail/host_file_path.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <limits>
#include <string>

namespace libbsa::detail {

result<host_file_path> resolve_host_file_path(std::string_view host_path) {
    if (host_path.find('\0') != std::string_view::npos) {
        return error{error_code::invalid_argument, "archive path contains embedded NUL bytes"};
    }

    std::string utf8_path{host_path};
    if (utf8_path.empty()) {
        return host_file_path{std::filesystem::path{}};
    }

    if (utf8_path.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        return error{error_code::invalid_argument, "archive path is too long to decode as UTF-8"};
    }

    // MSVC constructs narrow filesystem paths through the active ANSI code page,
    // so the public UTF-8 contract must be decoded once here before later
    // open/validate/reopen seams reuse the resolved path.
    const auto source_size = static_cast<int>(utf8_path.size());
    const auto wide_size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(),
                                                 source_size, nullptr, 0);
    if (wide_size <= 0) {
        return error{error_code::invalid_argument, "archive path is not valid UTF-8"};
    }

    std::wstring wide_path(static_cast<std::size_t>(wide_size), L'\0');
    const auto converted = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(),
                                                 source_size, wide_path.data(), wide_size);
    if (converted != wide_size) {
        return error{error_code::invalid_argument, "archive path is not valid UTF-8"};
    }

    return host_file_path{std::filesystem::path{std::move(wide_path)}};
}

}  // namespace libbsa::detail
