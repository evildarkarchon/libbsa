#include <catch2/catch_test_macros.hpp>

#include <detail/host_file.hpp>
#include <detail/host_file_path.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

libbsa::detail::host_file_context test_context() noexcept {
    return {"test failed to open source", "test failed to inspect source",
            "test failed while reading source", "test source changed", "test source allocation"};
}

std::filesystem::path source_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_host_file_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path source_path(std::string_view name) {
    return source_test_dir() / std::string{name};
}

/// Reserves an OS-unique fixture path without process-global mutable state.
std::filesystem::path unique_source_path() {
    std::array<wchar_t, MAX_PATH> buffer{};
    REQUIRE(::GetTempFileNameW(source_test_dir().c_str(), L"bsa", 0U, buffer.data()) != 0U);
    return std::filesystem::path{buffer.data()};
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

/// Attempts to acquire access that must conflict with a stable read session.
HANDLE open_conflicting_handle(const std::filesystem::path& path, DWORD desired_access) {
    return ::CreateFileW(path.c_str(), desired_access,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

/// Requires a conflicting access request to fail for Windows sharing reasons.
void require_sharing_violation(const std::filesystem::path& path, DWORD desired_access) {
    ::SetLastError(ERROR_SUCCESS);
    const auto handle = open_conflicting_handle(path, desired_access);
    const auto native_error = ::GetLastError();
    if (handle != INVALID_HANDLE_VALUE) {
        CHECK(::CloseHandle(handle) != FALSE);
    }
    REQUIRE(handle == INVALID_HANDLE_VALUE);
    CHECK(native_error == ERROR_SHARING_VIOLATION);
}

/// Removes one unique host-file fixture after its handles leave scope.
class scoped_source_cleanup final {
   public:
    /// Takes cleanup responsibility for `path`.
    explicit scoped_source_cleanup(std::filesystem::path path) : path_{std::move(path)} {}

    /// Removes the test fixture without obscuring an earlier assertion failure.
    ~scoped_source_cleanup() {
        std::error_code ignored;
        // Test cleanup is best-effort so it cannot replace the test's real result.
        std::filesystem::remove(path_, ignored);
    }

    /// Fixture cleanup ownership cannot be copied.
    scoped_source_cleanup(const scoped_source_cleanup&) = delete;

    /// Fixture cleanup ownership cannot be copy-assigned.
    scoped_source_cleanup& operator=(const scoped_source_cleanup&) = delete;

   private:
    std::filesystem::path path_;
};

}  // namespace

static_assert(!std::is_copy_constructible_v<libbsa::detail::stable_host_file_session>);
static_assert(!std::is_copy_assignable_v<libbsa::detail::stable_host_file_session>);
static_assert(std::is_nothrow_move_constructible_v<libbsa::detail::stable_host_file_session>);
static_assert(std::is_nothrow_move_assignable_v<libbsa::detail::stable_host_file_session>);

TEST_CASE("stable host-file session keeps prefix, rewind, full read, and copy coherent",
          "[unit][host_file][stable_session]") {
    const auto path = unique_source_path();
    const scoped_source_cleanup cleanup{path};
    const auto expected = bytes_from_text("one coherent source observation");
    write_binary_file(path, expected);

    auto resolved = libbsa::detail::resolve_host_file_path(path.string());
    REQUIRE(resolved.has_value());

    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), test_context());
    REQUIRE(opened.has_value());
    auto session = std::move(opened).value();
    CHECK(session.size() == expected.size());

    auto prefix = session.read_prefix(7U);
    REQUIRE(prefix.has_value());
    CHECK(prefix.value() == std::vector<std::byte>{expected.begin(), expected.begin() + 7});

    auto absolute = session.read_exact_at(static_cast<std::uint64_t>(expected.size() - 3U), 3U,
                                          "test absolute source range");
    REQUIRE(absolute.has_value());
    CHECK(absolute.value() == std::vector<std::byte>{expected.end() - 3, expected.end()});
    auto continued = session.read_prefix(2U);
    REQUIRE(continued.has_value());
    CHECK(continued.value() == std::vector<std::byte>{expected.begin() + 7, expected.begin() + 9});

    auto rewound = session.rewind();
    REQUIRE(rewound.has_value());
    auto full = session.read_exact(static_cast<std::uint64_t>(expected.size()));
    REQUIRE(full.has_value());
    CHECK(full.value() == expected);

    REQUIRE(session.rewind().has_value());
    std::vector<std::byte> copied;
    std::size_t callback_count = 0U;
    auto copied_result = session.copy_exact(
        static_cast<std::uint64_t>(expected.size()),
        [&](std::span<const std::byte> chunk) -> libbsa::result<void> {
            ++callback_count;
            CHECK(chunk.size() <= 5U);
            copied.insert(copied.end(), chunk.begin(), chunk.end());
            return {};
        },
        5U);
    REQUIRE(copied_result.has_value());
    CHECK(callback_count > 1U);
    CHECK(copied == expected);
}

TEST_CASE("stable host-file session enforces its observed source size",
          "[unit][host_file][stable_session]") {
    const auto path = unique_source_path();
    const scoped_source_cleanup cleanup{path};
    const auto expected = bytes_from_text("stable size");
    write_binary_file(path, expected);

    auto resolved = libbsa::detail::resolve_host_file_path(path.string());
    REQUIRE(resolved.has_value());
    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), test_context());
    REQUIRE(opened.has_value());
    auto session = std::move(opened).value();

    SECTION("full reads reject an unexpected trailing byte") {
        auto result = session.read_exact(static_cast<std::uint64_t>(expected.size() - 1U));
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::error_code::io_error);
        CHECK(result.error().message == "test source changed");
    }

    SECTION("full reads reject a short source") {
        auto result = session.read_exact(static_cast<std::uint64_t>(expected.size() + 1U));
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::error_code::io_error);
        CHECK(result.error().message == "test source changed");
    }

    SECTION("full reads reject a short read from an unrewound cursor") {
        REQUIRE(session.read_prefix(1U).has_value());
        auto result = session.read_exact(static_cast<std::uint64_t>(expected.size()));
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::error_code::io_error);
        CHECK(result.error().message == "test source changed");
    }

    SECTION("bounded copies require a non-zero chunk size") {
        auto result = session.copy_exact(
            static_cast<std::uint64_t>(expected.size()),
            [](std::span<const std::byte>) -> libbsa::result<void> { return {}; }, 0U);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::error_code::invalid_argument);
    }
}

TEST_CASE("stable host-file session move assignment transfers exactly one handle",
          "[unit][host_file][stable_session][windows_sharing]") {
    const auto first_path = unique_source_path();
    const auto second_path = unique_source_path();
    const scoped_source_cleanup first_cleanup{first_path};
    const scoped_source_cleanup second_cleanup{second_path};
    write_binary_file(first_path, bytes_from_text("first"));
    write_binary_file(second_path, bytes_from_text("second"));

    auto first_resolved = libbsa::detail::resolve_host_file_path(first_path.string());
    auto second_resolved = libbsa::detail::resolve_host_file_path(second_path.string());
    REQUIRE(first_resolved.has_value());
    REQUIRE(second_resolved.has_value());

    {
        auto first_opened =
            libbsa::detail::stable_host_file_session::open(first_resolved.value(), test_context());
        auto second_opened =
            libbsa::detail::stable_host_file_session::open(second_resolved.value(), test_context());
        REQUIRE(first_opened.has_value());
        REQUIRE(second_opened.has_value());
        auto first = std::move(first_opened).value();
        auto second = std::move(second_opened).value();

        second = std::move(first);

        const auto released_second = open_conflicting_handle(second_path, GENERIC_WRITE);
        REQUIRE(released_second != INVALID_HANDLE_VALUE);
        CHECK(::CloseHandle(released_second) != FALSE);
        require_sharing_violation(first_path, GENERIC_WRITE);
    }

    const auto released_first = open_conflicting_handle(first_path, GENERIC_WRITE);
    REQUIRE(released_first != INVALID_HANDLE_VALUE);
    CHECK(::CloseHandle(released_first) != FALSE);
}

TEST_CASE("stable host-file session move construction outlives the moved-from owner",
          "[unit][host_file][stable_session][windows_sharing]") {
    const auto path = unique_source_path();
    const scoped_source_cleanup cleanup{path};
    write_binary_file(path, bytes_from_text("move construction"));
    auto resolved = libbsa::detail::resolve_host_file_path(path.string());
    REQUIRE(resolved.has_value());

    {
        auto moved_to = [&] {
            auto opened =
                libbsa::detail::stable_host_file_session::open(resolved.value(), test_context());
            REQUIRE(opened.has_value());
            auto original = std::move(opened).value();
            return libbsa::detail::stable_host_file_session{std::move(original)};
        }();

        require_sharing_violation(path, GENERIC_WRITE);
        require_sharing_violation(path, DELETE);
        CHECK(moved_to.size() == bytes_from_text("move construction").size());
    }

    const auto writer = open_conflicting_handle(path, GENERIC_WRITE);
    REQUIRE(writer != INVALID_HANDLE_VALUE);
    CHECK(::CloseHandle(writer) != FALSE);
}

TEST_CASE("stable host-file session permits readers and denies write and delete sharing",
          "[unit][host_file][stable_session][windows_sharing]") {
    const auto path = unique_source_path();
    const scoped_source_cleanup cleanup{path};
    write_binary_file(path, bytes_from_text("shared read only"));
    auto resolved = libbsa::detail::resolve_host_file_path(path.string());
    REQUIRE(resolved.has_value());

    {
        auto opened =
            libbsa::detail::stable_host_file_session::open(resolved.value(), test_context());
        REQUIRE(opened.has_value());
        auto session = std::move(opened).value();

        const auto reader = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(reader != INVALID_HANDLE_VALUE);
        CHECK(::CloseHandle(reader) != FALSE);

        require_sharing_violation(path, GENERIC_WRITE);
        require_sharing_violation(path, DELETE);
    }

    const auto writer = open_conflicting_handle(path, GENERIC_WRITE);
    REQUIRE(writer != INVALID_HANDLE_VALUE);
    CHECK(::CloseHandle(writer) != FALSE);
    const auto deleter = open_conflicting_handle(path, DELETE);
    REQUIRE(deleter != INVALID_HANDLE_VALUE);
    CHECK(::CloseHandle(deleter) != FALSE);
}

TEST_CASE("stable host-file session reports source failures with caller diagnostics",
          "[unit][host_file][stable_session]") {
    const auto path = unique_source_path();
    const scoped_source_cleanup cleanup{path};
    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(path, removal_error));
    REQUIRE_FALSE(removal_error);
    auto resolved = libbsa::detail::resolve_host_file_path(path.string());
    REQUIRE(resolved.has_value());

    auto opened = libbsa::detail::stable_host_file_session::open(resolved.value(), test_context());

    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error().code == libbsa::error_code::io_error);
    CHECK(opened.error().message == "test failed to open source");
}

TEST_CASE("host_file reads exact whole-file payloads", "[unit][host_file]") {
    const auto path = source_path("exact.bin");
    const auto expected = bytes_from_text("exact source bytes");
    write_binary_file(path, expected);

    auto resolved = libbsa::detail::resolve_host_file_path(path.string());

    REQUIRE(resolved.has_value());
    CHECK(resolved.value().resolved == path);

    auto bytes = libbsa::detail::read_host_file_exact(
        resolved.value(), static_cast<std::uint64_t>(expected.size()), test_context());

    REQUIRE(bytes.has_value());
    CHECK(bytes.value() == expected);
}

TEST_CASE("host_file reads bounded short prefixes", "[unit][host_file]") {
    const auto path = source_path("prefix.bin");
    const auto expected = bytes_from_text("abc");
    write_binary_file(path, expected);

    auto resolved = libbsa::detail::resolve_host_file_path(path.string());

    REQUIRE(resolved.has_value());

    auto short_prefix = libbsa::detail::read_host_file_prefix(resolved.value(), 8U, test_context());
    auto bounded_prefix =
        libbsa::detail::read_host_file_prefix(resolved.value(), 2U, test_context());

    REQUIRE(short_prefix.has_value());
    CHECK(short_prefix.value() == expected);
    REQUIRE(bounded_prefix.has_value());
    CHECK(bounded_prefix.value() == std::vector<std::byte>{expected.begin(), expected.begin() + 2});
}

TEST_CASE("host_file iterates bounded chunks", "[unit][host_file]") {
    const auto path = source_path("chunks.bin");
    const auto expected = bytes_from_text("chunked source bytes");
    write_binary_file(path, expected);

    auto resolved = libbsa::detail::resolve_host_file_path(path.string());

    REQUIRE(resolved.has_value());

    std::vector<std::byte> visited;
    std::size_t callback_count = 0;
    auto iterated = libbsa::detail::for_each_host_file_chunk(
        resolved.value(), static_cast<std::uint64_t>(expected.size()), test_context(),
        [&](std::span<const std::byte> chunk) -> libbsa::result<void> {
            ++callback_count;
            visited.insert(visited.end(), chunk.begin(), chunk.end());
            CHECK(chunk.size() <= 5U);
            return {};
        },
        5U);

    REQUIRE(iterated.has_value());
    CHECK(callback_count > 1U);
    CHECK(visited == expected);
}

TEST_CASE("host_file reports missing sources with caller diagnostics", "[unit][host_file]") {
    const auto missing = source_path("missing.bin");
    std::filesystem::remove(missing);

    auto resolved = libbsa::detail::resolve_host_file_path(missing.string());

    REQUIRE(resolved.has_value());

    auto bytes = libbsa::detail::read_host_file_exact(resolved.value(), 1U, test_context());

    REQUIRE_FALSE(bytes.has_value());
    CHECK(bytes.error().code == libbsa::error_code::io_error);
    CHECK(bytes.error().message == "test failed to inspect source");
}

TEST_CASE("host_file rejects sources that shrink or grow", "[unit][host_file]") {
    const auto path = source_path("changed.bin");
    const auto expected = bytes_from_text("stable");

    auto resolved = libbsa::detail::resolve_host_file_path(path.string());

    REQUIRE(resolved.has_value());

    SECTION("shrunk before exact read") {
        write_binary_file(path, std::span<const std::byte>{expected.data(), expected.size() - 1U});

        auto bytes = libbsa::detail::read_host_file_exact(
            resolved.value(), static_cast<std::uint64_t>(expected.size()), test_context());

        REQUIRE_FALSE(bytes.has_value());
        CHECK(bytes.error().code == libbsa::error_code::io_error);
        CHECK(bytes.error().message == "test source changed");
    }

    SECTION("grown before exact read") {
        auto grown = expected;
        grown.push_back(std::byte{0x21});
        write_binary_file(path, grown);

        auto bytes = libbsa::detail::read_host_file_exact(
            resolved.value(), static_cast<std::uint64_t>(expected.size()), test_context());

        REQUIRE_FALSE(bytes.has_value());
        CHECK(bytes.error().code == libbsa::error_code::io_error);
        CHECK(bytes.error().message == "test source changed");
    }
}

TEST_CASE("host_file reports allocation limits through results", "[unit][host_file]") {
    const auto path = source_path("allocation.bin");
    write_binary_file(path, bytes_from_text("small"));

    auto resolved = libbsa::detail::resolve_host_file_path(path.string());

    REQUIRE(resolved.has_value());

    auto bytes = libbsa::detail::read_host_file_prefix(
        resolved.value(), std::numeric_limits<std::size_t>::max(), test_context());

    REQUIRE_FALSE(bytes.has_value());
    CHECK(bytes.error().code == libbsa::error_code::format_error);
}
