#pragma once

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <system_error>

namespace libbsa::tests {

inline bool is_source_root(const std::filesystem::path& candidate) {
    return std::filesystem::is_regular_file(candidate / "CMakeLists.txt") &&
           std::filesystem::is_regular_file(candidate / "include/libbsa/libbsa.hpp") &&
           std::filesystem::is_regular_file(candidate /
                                            "tests/fixtures/generated/compatibility_matrix.json");
}

inline std::filesystem::path find_source_root_from(std::filesystem::path start) {
    std::error_code error;
    auto candidate = std::filesystem::absolute(std::move(start), error);
    if (error) {
        return {};
    }

    while (!candidate.empty()) {
        if (is_source_root(candidate)) {
            return candidate;
        }

        const auto parent = candidate.parent_path();
        if (parent == candidate) {
            break;
        }
        candidate = parent;
    }

    return {};
}

inline std::filesystem::path source_root() {
    if (const auto* override_root = std::getenv("LIBBSA_TEST_SOURCE_DIR");
        override_root != nullptr) {
        auto root = find_source_root_from(override_root);
        if (!root.empty()) {
            return root;
        }
    }

    std::error_code error;
    auto root = find_source_root_from(std::filesystem::current_path(error));
    if (!error && !root.empty()) {
        return root;
    }

    throw std::runtime_error{
        "Unable to locate the libbsa source root. Run tests from the repository or set "
        "LIBBSA_TEST_SOURCE_DIR."};
}

}  // namespace libbsa::tests

#define LIBBSA_SOURCE_DIR (::libbsa::tests::source_root())
