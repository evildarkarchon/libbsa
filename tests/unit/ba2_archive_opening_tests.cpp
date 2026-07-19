#include "formats/ba2/ba2_archive_opening.hpp"
#include "formats/ba2/ba2_constants.hpp"

#include <detail/host_file_path.hpp>

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
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

/// Owns best-effort cleanup for one synthetic BA2 archive fixture.
class temporary_archive final {
   public:
    /// Allocates a unique path across concurrent test processes.
    temporary_archive() {
        static std::atomic_uint64_t sequence{0U};
        path_ = std::filesystem::temp_directory_path() /
                ("libbsa-ba2-opening-" +
                 std::to_string(static_cast<std::uint64_t>(::GetCurrentProcessId())) + "-" +
                 std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed)) + ".ba2");
    }

    /// Removes the synthetic fixture after its test completes.
    ~temporary_archive() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    temporary_archive(const temporary_archive&) = delete;
    temporary_archive& operator=(const temporary_archive&) = delete;

    /// Returns the host path used by the fixture.
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

    /// Replaces the fixture contents with exactly `bytes`.
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

/// Appends one little-endian 32-bit value to a synthetic header.
void append_u32_le(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (std::size_t index = 0; index < 4U; ++index) {
        bytes.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
    }
}

/// Appends one little-endian 16-bit value to a synthetic table.
void append_u16_le(std::vector<std::byte>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xFFU));
    bytes.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
}

/// Appends one little-endian 64-bit value to a synthetic header.
void append_u64_le(std::vector<std::byte>& bytes, std::uint64_t value) {
    for (std::size_t index = 0; index < 8U; ++index) {
        bytes.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
    }
}

/// Returns the serialized fixed-header width for a supported BA2 version.
std::size_t header_size_for(std::uint32_t version) {
    switch (version) {
        case libbsa::formats::ba2::ba2_fallout4_version:
            return libbsa::formats::ba2::ba2_common_header_size;
        case libbsa::formats::ba2::ba2_starfield_v2_version:
            return libbsa::formats::ba2::ba2_starfield_v2_header_size;
        case libbsa::formats::ba2::ba2_starfield_v3_version:
            return libbsa::formats::ba2::ba2_starfield_v3_header_size;
        default:
            return libbsa::formats::ba2::ba2_common_header_size;
    }
}

/// Builds a zero-entry BA2 whose filename table begins after its fixed header.
std::vector<std::byte> make_empty_ba2_header(
    std::uint32_t version, std::uint32_t subtype,
    std::uint32_t compression_method = libbsa::formats::ba2::ba2_starfield_compression_deflate,
    std::uint32_t unknown1 = 0x1122'3344U, std::uint32_t unknown2 = 0x5566'7788U,
    std::uint32_t file_count = 0U, std::optional<std::uint64_t> filename_table_offset = {}) {
    std::vector<std::byte> bytes;
    append_u32_le(bytes, libbsa::formats::ba2::ba2_btdx_magic);
    append_u32_le(bytes, version);
    append_u32_le(bytes, subtype);
    append_u32_le(bytes, file_count);
    append_u64_le(bytes, filename_table_offset.value_or(header_size_for(version)));
    if (version == libbsa::formats::ba2::ba2_starfield_v2_version ||
        version == libbsa::formats::ba2::ba2_starfield_v3_version) {
        append_u32_le(bytes, unknown1);
        append_u32_le(bytes, unknown2);
    }
    if (version == libbsa::formats::ba2::ba2_starfield_v3_version) {
        append_u32_le(bytes, compression_method);
    }
    return bytes;
}

/// Appends one structurally complete placeholder GNRL record.
void append_placeholder_gnrl_record(std::vector<std::byte>& bytes) {
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, 0U);
    append_u64_le(bytes, 0U);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, 0U);
    append_u32_le(bytes, libbsa::formats::ba2::ba2_record_sentinel);
}

/// Resolves a synthetic archive and opens it through the internal BA2 seam.
libbsa::result<libbsa::formats::ba2::opened_ba2_archive> open_synthetic_ba2(
    const temporary_archive& archive) {
    auto resolved = libbsa::detail::resolve_host_file_path(archive.path().string());
    REQUIRE(resolved.has_value());
    return libbsa::formats::ba2::open_ba2_archive(resolved.value());
}

/// Expected normalized result for one reader-supported BA2 header profile.
struct supported_profile_case {
    std::string_view name;
    std::uint32_t version;
    std::uint32_t subtype_magic;
    libbsa::formats::ba2::ba2_subtype subtype;
    libbsa::archive_variant variant;
    std::uint32_t compression_method;
    libbsa::entry_compression default_compression;
};

}  // namespace

TEST_CASE("BA2 Archive Opening returns normalized supported profile metadata and subtype",
          "[unit][ba2_archive_opening][fixed_header]") {
    using namespace libbsa::formats::ba2;
    constexpr auto profiles = std::to_array<supported_profile_case>({
        {"Fallout 4 GNRL", ba2_fallout4_version, ba2_gnrl_magic, ba2_subtype::gnrl,
         libbsa::archive_variant::fallout4, ba2_starfield_compression_deflate,
         libbsa::entry_compression::deflate},
        {"Fallout 4 DX10", ba2_fallout4_version, ba2_dx10_magic, ba2_subtype::dx10,
         libbsa::archive_variant::fallout4, ba2_starfield_compression_deflate,
         libbsa::entry_compression::deflate},
        {"Starfield v2 GNRL", ba2_starfield_v2_version, ba2_gnrl_magic, ba2_subtype::gnrl,
         libbsa::archive_variant::starfield, ba2_starfield_compression_deflate,
         libbsa::entry_compression::deflate},
        {"Starfield v2 DX10", ba2_starfield_v2_version, ba2_dx10_magic, ba2_subtype::dx10,
         libbsa::archive_variant::starfield, ba2_starfield_compression_deflate,
         libbsa::entry_compression::deflate},
        {"Starfield v3 GNRL deflate", ba2_starfield_v3_version, ba2_gnrl_magic, ba2_subtype::gnrl,
         libbsa::archive_variant::starfield, ba2_starfield_compression_deflate,
         libbsa::entry_compression::deflate},
        {"Starfield v3 GNRL LZ4", ba2_starfield_v3_version, ba2_gnrl_magic, ba2_subtype::gnrl,
         libbsa::archive_variant::starfield, ba2_starfield_compression_lz4_block,
         libbsa::entry_compression::lz4_block},
        {"Starfield v3 DX10 deflate", ba2_starfield_v3_version, ba2_dx10_magic, ba2_subtype::dx10,
         libbsa::archive_variant::starfield, ba2_starfield_compression_deflate,
         libbsa::entry_compression::deflate},
        {"Starfield v3 DX10 LZ4", ba2_starfield_v3_version, ba2_dx10_magic, ba2_subtype::dx10,
         libbsa::archive_variant::starfield, ba2_starfield_compression_lz4_block,
         libbsa::entry_compression::lz4_block},
    });

    for (const auto& profile : profiles) {
        INFO(profile.name);
        temporary_archive archive;
        archive.write(make_empty_ba2_header(profile.version, profile.subtype_magic,
                                            profile.compression_method));

        auto opened = open_synthetic_ba2(archive);

        REQUIRE(opened.has_value());
        CHECK(opened.value().subtype == profile.subtype);
        CHECK(opened.value().entries.empty());
        CHECK(opened.value().metadata.type == libbsa::archive_type::ba2);
        CHECK(opened.value().metadata.variant == profile.variant);
        CHECK(opened.value().metadata.version == profile.version);
        CHECK(opened.value().metadata.file_count == 0U);
        CHECK(opened.value().metadata.default_compression == profile.default_compression);
        REQUIRE(opened.value().metadata.ba2.has_value());
        if (profile.version == ba2_fallout4_version) {
            CHECK_FALSE(opened.value().metadata.ba2->starfield_unknown1.has_value());
            CHECK_FALSE(opened.value().metadata.ba2->starfield_unknown2.has_value());
            CHECK_FALSE(opened.value().metadata.ba2->compression_method.has_value());
        } else {
            REQUIRE(opened.value().metadata.ba2->starfield_unknown1.has_value());
            REQUIRE(opened.value().metadata.ba2->starfield_unknown2.has_value());
            CHECK(*opened.value().metadata.ba2->starfield_unknown1 == 0x1122'3344U);
            CHECK(*opened.value().metadata.ba2->starfield_unknown2 == 0x5566'7788U);
        }
        if (profile.version == ba2_starfield_v3_version) {
            REQUIRE(opened.value().metadata.ba2->compression_method.has_value());
            CHECK(*opened.value().metadata.ba2->compression_method == profile.compression_method);
        }
    }
}

TEST_CASE("BA2 Archive Opening classifies every fixed-header truncation as format_error",
          "[unit][ba2_archive_opening][fixed_header][malformed]") {
    using namespace libbsa::formats::ba2;
    constexpr auto versions = std::to_array<std::uint32_t>(
        {ba2_fallout4_version, ba2_starfield_v2_version, ba2_starfield_v3_version});

    for (const auto version : versions) {
        const auto complete = make_empty_ba2_header(version, ba2_gnrl_magic);
        for (std::size_t truncated_size = 0U; truncated_size < complete.size(); ++truncated_size) {
            INFO("version=" << version << " truncated_size=" << truncated_size);
            temporary_archive archive;
            archive.write(std::span<const std::byte>{complete.data(), truncated_size});

            auto opened = open_synthetic_ba2(archive);

            REQUIRE_FALSE(opened.has_value());
            CHECK(opened.error().code == libbsa::error_code::format_error);
        }
    }
}

TEST_CASE("BA2 Archive Opening preserves fixed-header error categories",
          "[unit][ba2_archive_opening][fixed_header][malformed]") {
    using namespace libbsa::formats::ba2;
    temporary_archive archive;

    SECTION("non-BTDX bytes are unsupported") {
        auto bytes = make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic);
        bytes[0] = std::byte{0U};
        archive.write(bytes);
        auto opened = open_synthetic_ba2(archive);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::unsupported);
    }

    SECTION("unsupported versions are unsupported") {
        archive.write(make_empty_ba2_header(99U, ba2_gnrl_magic));
        auto opened = open_synthetic_ba2(archive);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::unsupported);
    }

    SECTION("unsupported subtypes are unsupported") {
        archive.write(make_empty_ba2_header(ba2_fallout4_version, 0x2144'4142U));
        auto opened = open_synthetic_ba2(archive);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::unsupported);
    }

    SECTION("unsupported Starfield compression methods are unsupported") {
        archive.write(make_empty_ba2_header(ba2_starfield_v3_version, ba2_gnrl_magic, 99U));
        auto opened = open_synthetic_ba2(archive);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::unsupported);
    }

    SECTION("excessive file counts are malformed") {
        archive.write(make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic,
                                            ba2_starfield_compression_deflate, 0U, 0U, 1'000'001U));
        auto opened = open_synthetic_ba2(archive);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::format_error);
    }

    SECTION("filename tables past end of file are malformed") {
        archive.write(make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic,
                                            ba2_starfield_compression_deflate, 0U, 0U, 0U,
                                            ba2_common_header_size + 1U));
        auto opened = open_synthetic_ba2(archive);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::format_error);
    }

    SECTION("filename tables inside the fixed header are malformed") {
        archive.write(make_empty_ba2_header(ba2_fallout4_version, ba2_dx10_magic,
                                            ba2_starfield_compression_deflate, 0U, 0U, 0U, 0U));
        auto opened = open_synthetic_ba2(archive);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("BA2 Archive Opening validates GNRL record and encoded-name bounds",
          "[unit][ba2_archive_opening][gnrl][malformed]") {
    using namespace libbsa::formats::ba2;
    temporary_archive archive;

    SECTION("filename table cannot begin before the complete record table") {
        archive.write(make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic,
                                            ba2_starfield_compression_deflate, 0U, 0U, 1U,
                                            ba2_common_header_size));

        auto opened = open_synthetic_ba2(archive);

        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::format_error);
    }

    SECTION("encoded filename length cannot extend past the stable source") {
        auto bytes = make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic,
                                           ba2_starfield_compression_deflate, 0U, 0U, 1U,
                                           ba2_common_header_size + ba2_gnrl_record_size);
        append_placeholder_gnrl_record(bytes);
        append_u16_le(bytes, 4U);
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>('a')));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>('b')));
        archive.write(bytes);

        auto opened = open_synthetic_ba2(archive);

        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::format_error);
    }

    SECTION("encoded filenames cannot be empty") {
        auto bytes = make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic,
                                           ba2_starfield_compression_deflate, 0U, 0U, 1U,
                                           ba2_common_header_size + ba2_gnrl_record_size);
        append_placeholder_gnrl_record(bytes);
        append_u16_le(bytes, 0U);
        archive.write(bytes);

        auto opened = open_synthetic_ba2(archive);

        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("BA2 Archive Opening maps host open and incompatible sharing failures to io_error",
          "[unit][ba2_archive_opening][windows_sharing]") {
    using namespace libbsa::formats::ba2;

    SECTION("missing host file") {
        temporary_archive archive;
        auto resolved = libbsa::detail::resolve_host_file_path(archive.path().string());
        REQUIRE(resolved.has_value());
        auto opened = open_ba2_archive(resolved.value());
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::io_error);
    }

    SECTION("existing writer conflicts even when it permits read sharing") {
        temporary_archive archive;
        archive.write(make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic));
        const auto writer =
            ::CreateFileW(archive.path().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                          nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(writer != INVALID_HANDLE_VALUE);

        auto opened = open_synthetic_ba2(archive);

        CHECK(::CloseHandle(writer) != FALSE);
        REQUIRE_FALSE(opened.has_value());
        CHECK(opened.error().code == libbsa::error_code::io_error);
    }
}

TEST_CASE("BA2 Archive Opening is compatible with an existing native reader",
          "[unit][ba2_archive_opening][windows_sharing]") {
    using namespace libbsa::formats::ba2;
    temporary_archive archive;
    archive.write(make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic));
    const auto existing_reader =
        ::CreateFileW(archive.path().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(existing_reader != INVALID_HANDLE_VALUE);

    auto opened = open_synthetic_ba2(archive);

    CHECK(::CloseHandle(existing_reader) != FALSE);
    REQUIRE(opened.has_value());
}

TEST_CASE("BA2 Archive Opening permits concurrent metadata readers",
          "[unit][ba2_archive_opening][windows_sharing]") {
    using namespace libbsa::formats::ba2;
    temporary_archive archive;
    archive.write(make_empty_ba2_header(ba2_starfield_v3_version, ba2_gnrl_magic,
                                        ba2_starfield_compression_lz4_block));
    auto resolved = libbsa::detail::resolve_host_file_path(archive.path().string());
    REQUIRE(resolved.has_value());

    constexpr std::size_t reader_count = 8U;
    std::barrier start{static_cast<std::ptrdiff_t>(reader_count)};
    std::vector<std::future<bool>> readers;
    readers.reserve(reader_count);
    for (std::size_t index = 0U; index < reader_count; ++index) {
        readers.push_back(std::async(std::launch::async, [&] {
            start.arrive_and_wait();
            return open_ba2_archive(resolved.value()).has_value();
        }));
    }

    for (auto& reader : readers) {
        CHECK(reader.get());
    }
}

TEST_CASE("BA2 Archive Opening releases its native session after materialization",
          "[unit][ba2_archive_opening][windows_sharing]") {
    using namespace libbsa::formats::ba2;
    temporary_archive archive;
    archive.write(make_empty_ba2_header(ba2_fallout4_version, ba2_gnrl_magic));

    auto opened = open_synthetic_ba2(archive);
    REQUIRE(opened.has_value());

    const auto writer = ::CreateFileW(archive.path().c_str(), GENERIC_READ | GENERIC_WRITE, 0U,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(writer != INVALID_HANDLE_VALUE);
    CHECK(::CloseHandle(writer) != FALSE);

    auto public_reader = libbsa::archive_reader::open(archive.path().string());
    REQUIRE(public_reader.has_value());
    const auto writer_while_reader_lives =
        ::CreateFileW(archive.path().c_str(), GENERIC_READ | GENERIC_WRITE, 0U, nullptr,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(writer_while_reader_lives != INVALID_HANDLE_VALUE);
    CHECK(::CloseHandle(writer_while_reader_lives) != FALSE);
}
