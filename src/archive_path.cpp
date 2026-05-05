#include <libbsa/archive_path.hpp>

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa {
namespace {

bool is_separator(char c) noexcept
{
    return c == '/' || c == '\\';
}

bool is_drive_absolute(std::string_view input) noexcept
{
    return input.size() >= 3 && ((input[0] >= 'A' && input[0] <= 'Z') || (input[0] >= 'a' && input[0] <= 'z')) &&
        input[1] == ':' && is_separator(input[2]);
}

char normalize_char(char c) noexcept
{
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c + ('a' - 'A'));
    }

    return c == '\\' ? '/' : c;
}

result<archive_path> invalid_path()
{
    return failure<archive_path>({error_code::malformed_archive, "invalid archive path"});
}

} // namespace

archive_path::archive_path(std::string value) : value_(std::move(value)) {}

const std::string& archive_path::string() const noexcept
{
    return value_;
}

result<archive_path> normalize_archive_path(std::string input)
{
    if (input.empty() || is_drive_absolute(input) || is_separator(input.front())) {
        return invalid_path();
    }

    std::string normalized;
    normalized.reserve(input.size());
    for (char c : input) {
        normalized.push_back(normalize_char(c));
    }

    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start <= normalized.size()) {
        const auto end = normalized.find('/', start);
        const auto count = (end == std::string::npos) ? std::string::npos : end - start;
        auto segment = normalized.substr(start, count);
        if (!segment.empty() && segment != ".") {
            if (segment == "..") {
                return invalid_path();
            }
            segments.push_back(std::move(segment));
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    if (segments.empty()) {
        return invalid_path();
    }

    std::string joined;
    for (const auto& segment : segments) {
        if (!joined.empty()) {
            joined.push_back('/');
        }
        joined += segment;
    }

    return success(archive_path{std::move(joined)});
}

} // namespace libbsa
