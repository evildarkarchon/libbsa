#include <libbsa/libbsa.hpp>

#include <catch2/catch_test_macros.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

/// Owns one temporary BSA fixture; committed and retail archives are never modified.
class temporary_archive final {
   public:
    /// Allocates a path unique to this test process and fixture instance.
    temporary_archive() {
        static std::atomic_uint64_t sequence{0U};
        path_ = std::filesystem::temp_directory_path() /
                ("libbsa-bsa-opening-" +
                 std::to_string(static_cast<std::uint64_t>(::GetCurrentProcessId())) + "-" +
                 std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed)) + ".bsa");
    }

    /// Removes only this temporary fixture, including after a failed assertion.
    ~temporary_archive() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    temporary_archive(const temporary_archive&) = delete;
    temporary_archive& operator=(const temporary_archive&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

    /// Copies a committed generated fixture to a private mutable host file.
    void copy_generated(std::string_view filename) const {
        const auto source = std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" /
                            "generated" / "archives" / filename;
        REQUIRE(std::filesystem::copy_file(source, path_));
    }

    /// Writes exactly the supplied synthetic bytes before any native handles are acquired.
    void write(std::span<const std::byte> bytes) const {
        std::ofstream output{path_, std::ios::binary | std::ios::trunc};
        REQUIRE(output.is_open());
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(output.good());
    }

   private:
    std::filesystem::path path_;
};

/// Owns a real native handle so assertion failures cannot leave a sharing conflict behind.
class scoped_handle final {
   public:
    /// Opens the existing fixture with the access and sharing contract under test.
    scoped_handle(const std::filesystem::path& path, DWORD access, DWORD sharing)
        : handle_{::CreateFileW(path.c_str(), access, sharing, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr)} {}

    /// Releases any successfully opened native handle.
    ~scoped_handle() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
        }
    }

    scoped_handle(const scoped_handle&) = delete;
    scoped_handle& operator=(const scoped_handle&) = delete;

    [[nodiscard]] bool valid() const noexcept { return handle_ != INVALID_HANDLE_VALUE; }

   private:
    HANDLE handle_;
};

constexpr auto supported_fixtures = std::to_array<std::string_view>(
    {"tes3_success.bsa", "tes4_v103.bsa", "tes4_v104.bsa", "tes4_v105.bsa"});

/// Reads only a committed generated fixture's fixed header for truncation cases.
std::vector<std::byte> generated_header(std::string_view filename, std::size_t size) {
    const auto source = std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" /
                        "generated" / "archives" / filename;
    std::ifstream input{source, std::ios::binary};
    REQUIRE(input.is_open());
    std::vector<std::byte> bytes(size);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    REQUIRE(input.gcount() == static_cast<std::streamsize>(size));
    return bytes;
}

}  // namespace

TEST_CASE("BSA Archive Opening rejects existing write or delete access despite permissive sharing",
          "[unit][fixture][bsa_archive_opening][windows_sharing]") {
    for (const auto filename : supported_fixtures) {
        for (const DWORD access : {GENERIC_WRITE, DELETE}) {
            INFO(filename);
            INFO("existing access=" << access);
            temporary_archive archive;
            archive.copy_generated(filename);
            const scoped_handle conflicting_handle{
                archive.path(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE};
            REQUIRE(conflicting_handle.valid());

            auto opened = libbsa::archive_reader::open(archive.path().string());

            REQUIRE_FALSE(opened.has_value());
            CHECK(opened.error().code == libbsa::error_code::io_error);
        }
    }
}

TEST_CASE("BSA Archive Opening coexists with an existing native reader",
          "[unit][fixture][bsa_archive_opening][windows_sharing]") {
    for (const auto filename : supported_fixtures) {
        INFO(filename);
        temporary_archive archive;
        archive.copy_generated(filename);
        const scoped_handle existing_reader{archive.path(), GENERIC_READ, FILE_SHARE_READ};
        REQUIRE(existing_reader.valid());

        auto opened = libbsa::archive_reader::open(archive.path().string());

        REQUIRE(opened.has_value());
        auto entries = opened.value().entries();
        REQUIRE(entries.has_value());
        CHECK_FALSE(entries.value().empty());
    }
}

TEST_CASE("BSA Archive Opening permits concurrent metadata readers",
          "[unit][fixture][bsa_archive_opening][windows_sharing]") {
    for (const auto filename : supported_fixtures) {
        INFO(filename);
        temporary_archive archive;
        archive.copy_generated(filename);
        const auto path = archive.path().string();
        constexpr std::size_t reader_count = 8U;
        std::barrier start{static_cast<std::ptrdiff_t>(reader_count)};
        std::vector<std::future<bool>> readers;
        readers.reserve(reader_count);
        for (std::size_t index = 0U; index < reader_count; ++index) {
            readers.push_back(std::async(std::launch::async, [&] {
                start.arrive_and_wait();
                auto opened = libbsa::archive_reader::open(path);
                if (!opened) {
                    return false;
                }
                auto entries = opened.value().entries();
                return entries.has_value() && !entries.value().empty();
            }));
        }

        for (auto& reader : readers) {
            CHECK(reader.get());
        }
    }
}

TEST_CASE("BSA Archive Opening releases its host session while the returned reader lives",
          "[unit][fixture][bsa_archive_opening][windows_sharing]") {
    for (const auto filename : supported_fixtures) {
        INFO(filename);
        temporary_archive archive;
        archive.copy_generated(filename);
        auto opened = libbsa::archive_reader::open(archive.path().string());
        REQUIRE(opened.has_value());

        // Exclusive write/delete access proves that metadata ownership does not
        // accidentally extend the stable session into the reader's lifetime.
        const scoped_handle exclusive_writer{archive.path(), GENERIC_WRITE | DELETE, 0U};
        REQUIRE(exclusive_writer.valid());
        auto entries = opened.value().entries();
        REQUIRE(entries.has_value());
        CHECK_FALSE(entries.value().empty());
    }
}

TEST_CASE("BSA Archive Opening rejects every truncated fixed header and releases failed sessions",
          "[unit][fixture][malformed][bsa_archive_opening][fixed_header][windows_sharing]") {
    for (const auto filename : supported_fixtures) {
        INFO(filename);
        const auto full_header =
            generated_header(filename, filename == "tes3_success.bsa" ? 12U : 36U);
        for (std::size_t size = 0U; size < full_header.size(); ++size) {
            INFO("prefix bytes=" << size);
            temporary_archive archive;
            archive.write(std::span{full_header}.first(size));

            auto opened = libbsa::archive_reader::open(archive.path().string());

            REQUIRE_FALSE(opened.has_value());
            CHECK(opened.error().code == libbsa::error_code::format_error);
            const scoped_handle exclusive_writer{archive.path(), GENERIC_WRITE | DELETE, 0U};
            CHECK(exclusive_writer.valid());
        }
    }
}

TEST_CASE("BSA Archive Opening reports unsupported TES4 versions before fixed header truncation",
          "[unit][fixture][malformed][bsa_archive_opening][fixed_header][unsupported_future_bsa]") {
    auto header = generated_header("tes4_v103.bsa", 36U);
    header[4] = std::byte{106};
    header[5] = std::byte{0};
    header[6] = std::byte{0};
    header[7] = std::byte{0};
    for (std::size_t size = 8U; size < header.size(); ++size) {
        INFO("prefix bytes=" << size);
        temporary_archive archive;
        archive.write(std::span{header}.first(size));

        auto opened = libbsa::archive_reader::open(archive.path().string());

        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::unsupported);
        const scoped_handle exclusive_writer{archive.path(), GENERIC_WRITE | DELETE, 0U};
        CHECK(exclusive_writer.valid());
    }
}
