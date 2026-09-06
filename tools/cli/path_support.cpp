#include "path_support.hpp"

#include <limits>
#include <string>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace libbsa::cli {
namespace {

libbsa::error make_error(libbsa::error_code code, std::string message) {
    return libbsa::error{code, std::move(message)};
}

/// Inspects Windows reparse attributes without treating a missing path as a failure.
libbsa::result<bool> is_reparse_point(const std::filesystem::path& path, std::string_view context) {
#if defined(_WIN32)
    const auto attributes = ::GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const auto last_error = ::GetLastError();
        if (last_error == ERROR_FILE_NOT_FOUND || last_error == ERROR_PATH_NOT_FOUND) {
            return false;
        }
        return make_error(
            libbsa::error_code::io_error,
            "cannot inspect " + std::string{context} + " for reparse points: " + path.string());
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    (void)path;
    (void)context;
    return false;
#endif
}

}  // namespace

libbsa::result<std::filesystem::path> path_from_utf8(std::string_view utf8_path) {
    if (utf8_path.find('\0') != std::string_view::npos) {
        return make_error(libbsa::error_code::invalid_argument, "path contains embedded NUL bytes");
    }
    // Empty host paths resolve as the current working directory in later
    // std::filesystem::absolute() calls, which would silently pack or extract CWD.
    if (utf8_path.empty()) {
        return make_error(libbsa::error_code::invalid_argument, "path is empty");
    }

#if defined(_WIN32)
    if (utf8_path.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        return make_error(libbsa::error_code::invalid_argument,
                          "path is too long to decode as UTF-8");
    }

    // MSVC decodes narrow filesystem paths through the active ANSI code page, but
    // archive paths are UTF-8.
    const auto source_size = static_cast<int>(utf8_path.size());
    const auto wide_size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(),
                                                 source_size, nullptr, 0);
    if (wide_size <= 0) {
        return make_error(libbsa::error_code::invalid_argument, "path is not valid UTF-8");
    }

    std::wstring wide_path(static_cast<std::size_t>(wide_size), L'\0');
    const auto converted = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_path.data(),
                                                 source_size, wide_path.data(), wide_size);
    if (converted != wide_size) {
        return make_error(libbsa::error_code::invalid_argument, "path is not valid UTF-8");
    }

    return std::filesystem::path{std::move(wide_path)};
#else
    std::u8string path;
    path.reserve(utf8_path.size());
    for (const unsigned char ch : utf8_path) {
        path.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path{std::move(path)};
#endif
}

libbsa::result<void> reject_reparse_point(const std::filesystem::path& path,
                                          std::string_view context) {
    auto reparse = is_reparse_point(path, context);
    if (!reparse) {
        return reparse.error();
    }
    if (reparse.value()) {
        return make_error(libbsa::error_code::io_error,
                          "refusing reparse-point " + std::string{context} + ": " + path.string());
    }
    return {};
}

}  // namespace libbsa::cli
