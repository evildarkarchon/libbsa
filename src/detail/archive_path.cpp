#include <detail/archive_path.hpp>

namespace libbsa::detail {
namespace {

libbsa::error invalid_path_error() {
    return {libbsa::error_code::invalid_argument, "invalid archive virtual path"};
}

char lower_ascii(char value) noexcept {
    if (value >= 'A' && value <= 'Z') {
        return static_cast<char>(value - 'A' + 'a');
    }
    return value;
}

bool is_drive_rooted(std::string_view input) noexcept {
    return input.size() >= 2 &&
           ((input[0] >= 'A' && input[0] <= 'Z') || (input[0] >= 'a' && input[0] <= 'z')) &&
           input[1] == ':';
}

}  // namespace

result<archive_path_key> normalize_archive_path(std::string_view input) {
    if (input.empty() || input.front() == '/' || input.front() == '\\' || is_drive_rooted(input)) {
        return invalid_path_error();
    }

    archive_path_key key;
    key.value.reserve(input.size());
    std::string segment;

    for (const char raw : input) {
        if (raw == '\0') {
            return invalid_path_error();
        }
        const char normalized = raw == '\\' ? '/' : lower_ascii(raw);
        if (normalized == '/') {
            if (segment.empty() || segment == "." || segment == "..") {
                return invalid_path_error();
            }
            if (!key.value.empty()) {
                key.value.push_back('/');
            }
            key.value.append(segment);
            segment.clear();
            continue;
        }
        segment.push_back(normalized);
    }

    if (segment.empty() || segment == "." || segment == "..") {
        return invalid_path_error();
    }
    if (!key.value.empty()) {
        key.value.push_back('/');
    }
    key.value.append(segment);
    return key;
}

}  // namespace libbsa::detail
