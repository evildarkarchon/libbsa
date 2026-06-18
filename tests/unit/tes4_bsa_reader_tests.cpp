#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" /
           "archives";
}

std::filesystem::path generated_archive_path(std::string_view filename) {
    return generated_archive_dir() / std::string{filename};
}

libbsa::entry_compression expected_default_compression(std::uint32_t version) {
    return version == 105U ? libbsa::entry_compression::lz4_frame
                           : libbsa::entry_compression::deflate;
}

libbsa::archive_variant expected_variant(std::uint32_t version) {
    (void)version;
    return libbsa::archive_variant::tes4;
}

struct success_fixture {
    std::string archive_filename;
    std::uint32_t version;
    std::uint32_t flags;
    std::uint32_t file_count;
};

std::vector<success_fixture> success_fixtures() {
    return {
        {.archive_filename = "tes4_v103.bsa", .version = 103U, .flags = 3U, .file_count = 2U},
        {.archive_filename = "tes4_v104.bsa", .version = 104U, .flags = 259U, .file_count = 2U},
        {.archive_filename = "tes4_v105.bsa", .version = 105U, .flags = 259U, .file_count = 2U},
    };
}

libbsa::error_code error_code_from_manifest(std::string_view value) {
    if (value == "format_error") {
        return libbsa::error_code::format_error;
    }
    if (value == "unsupported") {
        return libbsa::error_code::unsupported;
    }
    FAIL("unknown TES4 malformed expected_error: " << value);
    return libbsa::error_code::format_error;
}

libbsa::entry_compression entry_compression_from_manifest(std::string_view value) {
    if (value == "raw") {
        return libbsa::entry_compression::none;
    }
    if (value == "deflate") {
        return libbsa::entry_compression::deflate;
    }
    if (value == "lz4_frame") {
        return libbsa::entry_compression::lz4_frame;
    }
    return libbsa::entry_compression::lz4_block;
}

std::uint64_t hex_u64_from_manifest(const nlohmann::json& value) {
    return std::stoull(value.get<std::string>(), nullptr, 16);
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    return nlohmann::json::parse(stream);
}

std::string archive_original_path_from_manifest(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input{path};
    REQUIRE(input.is_open());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::vector<std::byte> bytes_from_hex(std::string_view hex) {
    REQUIRE(hex.size() % 2U == 0U);
    std::vector<std::byte> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t offset = 0; offset < hex.size(); offset += 2U) {
        const auto pair = std::string{hex.substr(offset, 2U)};
        bytes.push_back(static_cast<std::byte>(std::stoul(pair, nullptr, 16)));
    }
    return bytes;
}

class collecting_sink final : public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        return bytes.size();
    }

    [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

   private:
    std::vector<std::byte> bytes_;
};

class recording_sink final : public libbsa::payload_sink {
   public:
    /// Captures extracted bytes and each write size for bounded sink extraction
    /// assertions.
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        write_sizes_.push_back(bytes.size());
        return bytes.size();
    }

    [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }
    [[nodiscard]] const std::vector<std::size_t>& write_sizes() const noexcept {
        return write_sizes_;
    }

   private:
    std::vector<std::byte> bytes_;
    std::vector<std::size_t> write_sizes_;
};

class partial_sink final : public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        return bytes.empty() ? 0U : bytes.size() - 1U;
    }
};

void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
    std::ofstream output{path, std::ios::binary};
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
}

void overwrite_u32_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4U; ++index) {
        bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
    }
}

std::uint32_t read_u32_le(const std::vector<std::byte>& bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (std::uint32_t index = 0; index < 4U; ++index) {
        value |=
            static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.at(offset + index)))
            << (index * 8U);
    }
    return value;
}

}  // namespace

TEST_CASE("tes4_bsa_detection opens byte-driven TES4-family BSA variants",
          "[unit][fixture][tes4_bsa_detection]") {
    for (const auto& fixture : success_fixtures()) {
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());

        REQUIRE(opened.has_value());
    }
}

TEST_CASE("tes4_bsa_detection rejects non-BSA bytes regardless of host extension",
          "[unit][fixture][tes4_bsa_detection]") {
    auto opened = libbsa::archive_reader::open(
        generated_archive_path("malformed_non_bsa_bytes.bsa").string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::unsupported);
}

TEST_CASE("tes4_bsa_metadata exposes archive-level open state",
          "[unit][fixture][tes4_bsa_metadata]") {
    for (const auto& fixture : success_fixtures()) {
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
        REQUIRE(opened.has_value());

        auto metadata = opened.value().metadata();
        REQUIRE(metadata.has_value());
        REQUIRE(metadata.value().type == libbsa::archive_type::bsa);
        REQUIRE(metadata.value().variant == expected_variant(fixture.version));
        REQUIRE(metadata.value().version == fixture.version);
        REQUIRE(metadata.value().archive_flags == fixture.flags);
        REQUIRE(metadata.value().file_count == fixture.file_count);
        REQUIRE(metadata.value().default_compression ==
                expected_default_compression(fixture.version));
    }
}

TEST_CASE(
    "unsupported_future_bsa reports unsupported for recognized future "
    "BSA versions",
    "[unit][fixture][unsupported_future_bsa]") {
    auto opened = libbsa::archive_reader::open(
        generated_archive_path("malformed_unsupported_version.bsa").string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::unsupported);
}

TEST_CASE(
    "tes4_bsa_malformed_open rejects malformed open-phase fixtures with "
    "stable error codes",
    "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    std::ifstream manifest_stream{generated_archive_path("malformed_manifest.json")};
    const auto manifest = nlohmann::json::parse(manifest_stream);

    for (const auto& test_case : manifest.at("cases")) {
        if (test_case.at("phase").get<std::string>() != "open") {
            continue;
        }
        if (test_case.at("id").get<std::string>() == "duplicate_canonical_path") {
            continue;
        }
        const auto archive = test_case.at("archive").get<std::string>();
        const auto expected =
            error_code_from_manifest(test_case.at("expected_error").get<std::string>());
        auto opened = libbsa::archive_reader::open(generated_archive_path(archive).string());

        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == expected);
    }
}

TEST_CASE(
    "tes4_bsa_malformed_open rejects count-derived table spans before "
    "allocation",
    "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    overwrite_u32_le(bytes, 24U, 0xFFFF'FFFFU);

    const auto mutated =
        std::filesystem::temp_directory_path() / "libbsa_oversized_folder_names.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE(
    "tes4_bsa_malformed_open returns format_error for oversized declared "
    "file-name bytes",
    "[unit][fixture][malformed][tes4_bsa_malformed_open][allocation]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    overwrite_u32_le(bytes, 28U, 0xFFFF'FFFFU);

    const auto mutated = std::filesystem::temp_directory_path() / "libbsa_oversized_file_names.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE("tes4_bsa_malformed_open rejects folder record counts before allocation",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    overwrite_u32_le(bytes, 44U, 0xFFFF'FFFFU);

    const auto mutated =
        std::filesystem::temp_directory_path() / "libbsa_oversized_folder_file_count.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE(R"(tes4_bsa_malformed_open rejects matched oversized file counts before allocation)",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    overwrite_u32_le(bytes, 20U, 0xFFFF'FFFFU);
    overwrite_u32_le(bytes, 44U, 0xFFFF'FFFFU);

    const auto mutated =
        std::filesystem::temp_directory_path() / "libbsa_matched_oversized_file_counts.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE("tes4_bsa_malformed_open rejects metadata counts above parser limits",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    SECTION("header folder count") {
        auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
        overwrite_u32_le(
            bytes, 16U,
            static_cast<std::uint32_t>(libbsa::detail::metadata_bsa_folder_count_limit + 1U));

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_tes4_excessive_folder_count.bsa";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());

        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);
    }

    SECTION("header file count") {
        auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
        overwrite_u32_le(
            bytes, 20U,
            static_cast<std::uint32_t>(libbsa::detail::metadata_entry_count_limit + 1U));

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_tes4_excessive_file_count.bsa";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());

        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);
    }

    SECTION("per-folder file count") {
        auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
        overwrite_u32_le(
            bytes, 44U,
            static_cast<std::uint32_t>(libbsa::detail::metadata_entry_count_limit + 1U));

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_tes4_excessive_folder_file_count.bsa";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());

        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("tes4_bsa_malformed_open rejects inconsistent table offsets",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    SECTION("header folder offset") {
        auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
        overwrite_u32_le(bytes, 8U, 40U);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_bad_header_folder_offset.bsa";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());

        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);
    }

    SECTION("folder record offset") {
        auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
        overwrite_u32_le(bytes, 48U, 52U);

        const auto mutated =
            std::filesystem::temp_directory_path() / "libbsa_bad_folder_record_offset.bsa";
        write_binary_file(mutated, bytes);

        auto opened = libbsa::archive_reader::open(mutated.string());

        REQUIRE_FALSE(opened.has_value());
        REQUIRE(opened.error().code == libbsa::error_code::format_error);

        auto validated = libbsa::validate_archive(mutated.string());
        REQUIRE(validated.has_value());
        CHECK_FALSE(validated.value().is_valid());
        REQUIRE(validated.value().errors.size() == 1U);
        CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("tes4_bsa_malformed_open rejects payload spans inside metadata",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    const auto folder_count = read_u32_le(bytes, 16U);
    const auto folder_name_bytes = read_u32_le(bytes, 24U);
    const auto first_file_record = 36U + folder_count * 16U + folder_name_bytes;
    overwrite_u32_le(bytes, first_file_record + 12U, 0U);

    const auto mutated =
        std::filesystem::temp_directory_path() / "libbsa_payload_inside_metadata.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE("tes4_bsa_malformed_open rejects partially overlapping payload spans",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    const auto folder_count = read_u32_le(bytes, 16U);
    const auto folder_name_bytes = read_u32_le(bytes, 24U);
    const auto first_file_record = 36U + folder_count * 16U + folder_name_bytes;
    const auto second_file_record = first_file_record + 16U;
    const auto first_payload_offset = read_u32_le(bytes, first_file_record + 12U);
    overwrite_u32_le(bytes, second_file_record + 12U, first_payload_offset + 10U);

    const auto mutated =
        std::filesystem::temp_directory_path() / "libbsa_tes4_partial_payload_overlap.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(mutated.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
}

TEST_CASE(
    "tes4_bsa_entry_metadata accepts exact duplicate payload spans for "
    "writer dedupe",
    "[unit][fixture][tes4_bsa_entry_metadata]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    const auto folder_count = read_u32_le(bytes, 16U);
    const auto folder_name_bytes = read_u32_le(bytes, 24U);
    const auto first_file_record = 36U + folder_count * 16U + folder_name_bytes;
    const auto second_file_record = first_file_record + 16U;
    const auto first_size_flags = read_u32_le(bytes, first_file_record + 8U);
    const auto second_size_flags = read_u32_le(bytes, second_file_record + 8U);
    const auto first_payload_offset = read_u32_le(bytes, first_file_record + 12U);
    overwrite_u32_le(bytes, second_file_record + 8U,
                     (second_size_flags & 0x4000'0000U) | first_size_flags);
    overwrite_u32_le(bytes, second_file_record + 12U, first_payload_offset);

    const auto mutated =
        std::filesystem::temp_directory_path() / "libbsa_tes4_exact_duplicate_payload_span.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());
    REQUIRE(opened.has_value());
    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());

    const auto raw = std::find_if(entries.value().begin(), entries.value().end(),
                                  [](const libbsa::entry_metadata& entry) {
                                      return entry.path == "meshes/tiny/rawmesh.nif";
                                  });
    const auto packed = std::find_if(entries.value().begin(), entries.value().end(),
                                     [](const libbsa::entry_metadata& entry) {
                                         return entry.path == "meshes/tiny/packedmesh.nif";
                                     });
    REQUIRE(raw != entries.value().end());
    REQUIRE(packed != entries.value().end());
    CHECK(packed->payload_offset == raw->payload_offset);
    CHECK(packed->stored_size == raw->stored_size);
}

TEST_CASE("tes4_bsa_malformed_open rejects file record hash mismatches",
          "[unit][fixture][malformed][tes4_bsa_malformed_open][tes4_bsa_hash_"
          "lookup]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    const auto folder_count = read_u32_le(bytes, 16U);
    const auto folder_name_bytes = read_u32_le(bytes, 24U);
    const auto first_file_record = 36U + folder_count * 16U + folder_name_bytes;
    overwrite_u32_le(bytes, first_file_record, read_u32_le(bytes, first_file_record) ^ 0x1000U);

    const auto mutated = std::filesystem::temp_directory_path() / "libbsa_file_hash_mismatch.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(mutated.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
}

TEST_CASE("tes4_bsa_malformed_open rejects folder record hash mismatches",
          "[unit][fixture][malformed][tes4_bsa_malformed_open][tes4_bsa_hash_"
          "lookup]") {
    auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
    overwrite_u32_le(bytes, 36U, read_u32_le(bytes, 36U) ^ 0x1000U);

    const auto mutated = std::filesystem::temp_directory_path() / "libbsa_folder_hash_mismatch.bsa";
    write_binary_file(mutated, bytes);

    auto opened = libbsa::archive_reader::open(mutated.string());
    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);

    auto validated = libbsa::validate_archive(mutated.string());
    REQUIRE(validated.has_value());
    CHECK_FALSE(validated.value().is_valid());
    REQUIRE(validated.value().errors.size() == 1U);
    CHECK(validated.value().errors.front().code == libbsa::error_code::format_error);
}

TEST_CASE(
    "tes4_bsa_entry_metadata materializes table paths, hashes, sizes, "
    "and embedded names",
    "[unit][fixture][tes4_bsa_entry_metadata][tes4_bsa_listing][tes4_bsa_"
    "embedded_name]") {
    for (const auto& fixture : success_fixtures()) {
        const auto manifest = read_json_file(generated_archive_path(
            fixture.archive_filename.substr(0, fixture.archive_filename.size() - 4) +
            "_manifest.json"));
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
        REQUIRE(opened.has_value());

        auto entries = opened.value().entries();
        REQUIRE(entries.has_value());
        REQUIRE(entries.value().size() == manifest.at("entries").size());

        std::vector<std::string> sorted_paths;
        for (const auto& expected : manifest.at("entries")) {
            sorted_paths.push_back(expected.at("path").get<std::string>());
        }
        std::sort(sorted_paths.begin(), sorted_paths.end());

        for (std::size_t index = 0; index < entries.value().size(); ++index) {
            const auto& actual = entries.value().at(index);
            const auto& expected = *std::find_if(
                manifest.at("entries").begin(), manifest.at("entries").end(),
                [&](const auto& candidate) {
                    return candidate.at("path").get<std::string>() == sorted_paths.at(index);
                });

            REQUIRE(actual.path == expected.at("path").get<std::string>());
            REQUIRE(actual.original_path == archive_original_path_from_manifest(
                                                expected.at("original_path").get<std::string>()));
            REQUIRE(actual.raw_size == expected.at("raw_size").get<std::uint64_t>());
            REQUIRE(actual.stored_size == expected.at("stored_size").get<std::uint64_t>());
            REQUIRE(actual.payload_offset == expected.at("offset").get<std::uint64_t>());
            REQUIRE(actual.archive_hash == hex_u64_from_manifest(expected.at("hash")));
            REQUIRE(actual.record_flags == expected.at("record_flags").get<std::uint32_t>());
            REQUIRE(actual.compression ==
                    entry_compression_from_manifest(expected.at("compression").get<std::string>()));
            REQUIRE(actual.has_embedded_name == expected.at("has_embedded_name").get<bool>());
            REQUIRE(actual.embedded_name_prefix_size ==
                    expected.at("embedded_name_prefix_size").get<std::uint32_t>());
        }
    }
}

TEST_CASE(
    "tes4_bsa_malformed_open rejects duplicate canonical paths during entry "
    "parsing",
    "[unit][fixture][malformed][tes4_bsa_malformed_open][tes4_bsa_listing]") {
    auto opened = libbsa::archive_reader::open(
        generated_archive_path("malformed_duplicate_canonical_path.bsa").string());

    REQUIRE_FALSE(opened.has_value());
    REQUIRE(opened.error().code == libbsa::error_code::format_error);
}

TEST_CASE(
    "tes4_bsa_lookup normalizes variants and distinguishes missing from "
    "invalid paths",
    "[unit][fixture][tes4_bsa_lookup][tes4_bsa_hash_lookup]") {
    for (const auto& fixture : success_fixtures()) {
        const auto stem = fixture.archive_filename.substr(0, fixture.archive_filename.size() - 4);
        const auto manifest = read_json_file(generated_archive_path(stem + "_manifest.json"));
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
        REQUIRE(opened.has_value());

        for (const auto& expected : manifest.at("entries")) {
            for (const auto& variant : expected.at("lookup_variants")) {
                auto found = opened.value().find(variant.get<std::string>());
                REQUIRE(found.has_value());
                REQUIRE(found.value().has_value());
                REQUIRE(found.value()->path == expected.at("path").get<std::string>());
                REQUIRE(found.value()->archive_hash == hex_u64_from_manifest(expected.at("hash")));

                auto contains = opened.value().contains(variant.get<std::string>());
                REQUIRE(contains.has_value());
                REQUIRE(contains.value());
            }
        }

        auto missing = opened.value().find("valid/missing/path.txt");
        REQUIRE(missing.has_value());
        REQUIRE_FALSE(missing.value().has_value());

        auto missing_contains = opened.value().contains("valid/missing/path.txt");
        REQUIRE(missing_contains.has_value());
        REQUIRE_FALSE(missing_contains.value());

        for (const std::string invalid :
             {"", "/rooted/file.txt", "..\\escape.txt", "folder//file.txt"}) {
            auto invalid_find = opened.value().find(invalid);
            REQUIRE_FALSE(invalid_find.has_value());
            REQUIRE(invalid_find.error().code == libbsa::error_code::invalid_argument);

            auto invalid_contains = opened.value().contains(invalid);
            REQUIRE_FALSE(invalid_contains.has_value());
            REQUIRE(invalid_contains.error().code == libbsa::error_code::invalid_argument);
        }
    }
}

TEST_CASE("tes4_bsa_v103_extract streams raw and deflate entries by archive path",
          "[unit][fixture][tes4_bsa_v103_extract][tes4_bsa_compression_routing]") {
    const auto manifest = read_json_file(generated_archive_path("tes4_v103_manifest.json"));
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v103.bsa").string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
        collecting_sink sink;

        auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

        REQUIRE(extracted.has_value());
        REQUIRE(sink.bytes() ==
                bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
    }
}

TEST_CASE(
    "tes4_bsa_v104_extract skips embedded names before raw and deflate "
    "payloads",
    "[unit][fixture][tes4_bsa_v104_extract][tes4_bsa_embedded_name][tes4_"
    "bsa_compression_routing]") {
    const auto manifest = read_json_file(generated_archive_path("tes4_v104_manifest.json"));
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v104.bsa").string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
        collecting_sink sink;

        auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

        REQUIRE(extracted.has_value());
        REQUIRE(sink.bytes() ==
                bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
    }
}

TEST_CASE(
    "tes4_bsa_v105_extract routes compressed entries through LZ4 frame "
    "decoding",
    "[unit][fixture][tes4_bsa_v105_extract][tes4_bsa_embedded_name][tes4_"
    "bsa_compression_routing]") {
    const auto manifest = read_json_file(generated_archive_path("tes4_v105_manifest.json"));
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v105.bsa").string());
    REQUIRE(opened.has_value());

    for (const auto& expected : manifest.at("entries")) {
        collecting_sink sink;

        auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

        REQUIRE(extracted.has_value());
        REQUIRE(sink.bytes() ==
                bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
    }
}

TEST_CASE(
    "tes4_bsa_v105_extract writes LZ4 frame payloads through bounded "
    "sink chunks",
    "[unit][fixture][tes4_bsa_v105_extract][tes4_bsa_compression_routing]"
    "[bounded_memory_policy]") {
    constexpr std::size_t extraction_chunk_size = 64U * 1024U;
    const auto manifest = read_json_file(generated_archive_path("tes4_v105_manifest.json"));
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v105.bsa").string());
    REQUIRE(opened.has_value());
    bool saw_lz4_frame = false;

    for (const auto& expected : manifest.at("entries")) {
        if (expected.at("compression").get<std::string>() != "lz4_frame") {
            continue;
        }
        saw_lz4_frame = true;
        recording_sink sink;

        auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);
        auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());

        REQUIRE(extracted.has_value());
        REQUIRE(bytes.has_value());
        REQUIRE(sink.bytes() == bytes.value());
        for (const auto write_size : sink.write_sizes()) {
            REQUIRE(write_size <= extraction_chunk_size);
        }
    }

    REQUIRE(saw_lz4_frame);
}

TEST_CASE("tes4_bsa_sink_errors reports partial sink writes as io_error",
          "[unit][fixture][tes4_bsa_sink_errors]") {
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v103.bsa").string());
    REQUIRE(opened.has_value());
    partial_sink sink;

    auto extracted = opened.value().extract("meshes/tiny/rawmesh.nif", sink);

    REQUIRE_FALSE(extracted.has_value());
    REQUIRE(extracted.error().code == libbsa::error_code::io_error);
}

TEST_CASE("tes4_bsa_extract maps invalid and missing paths to stable errors",
          "[unit][fixture][tes4_bsa_v103_extract]") {
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v103.bsa").string());
    REQUIRE(opened.has_value());
    collecting_sink sink;

    auto invalid = opened.value().extract("/rooted/file.txt", sink);
    REQUIRE_FALSE(invalid.has_value());
    REQUIRE(invalid.error().code == libbsa::error_code::invalid_argument);

    auto missing = opened.value().extract("valid/missing/path.txt", sink);
    REQUIRE_FALSE(missing.has_value());
    REQUIRE(missing.error().code == libbsa::error_code::not_found);
}

TEST_CASE("tes4_bsa_extract reads selected payloads from the host archive on demand",
          "[unit][fixture][tes4_bsa_v103_extract]") {
    const auto temp_archive =
        std::filesystem::temp_directory_path() / "libbsa_on_demand_extract.bsa";
    write_binary_file(temp_archive, read_binary_file(generated_archive_path("tes4_v103.bsa")));
    auto opened = libbsa::archive_reader::open(temp_archive.string());
    REQUIRE(opened.has_value());
    std::filesystem::remove(temp_archive);
    collecting_sink sink;

    auto extracted = opened.value().extract("meshes/tiny/rawmesh.nif", sink);

    REQUIRE_FALSE(extracted.has_value());
    REQUIRE(extracted.error().code == libbsa::error_code::io_error);
}

TEST_CASE(
    "tes4_bsa_compression_routing rejects corrupt compressed payloads "
    "and size mismatches",
    "[unit][fixture][malformed][tes4_bsa_compression_routing]") {
    const auto manifest = read_json_file(generated_archive_path("malformed_manifest.json"));
    for (const auto& test_case : manifest.at("cases")) {
        if (test_case.at("phase").get<std::string>() != "extraction") {
            continue;
        }
        auto opened = libbsa::archive_reader::open(
            generated_archive_path(test_case.at("archive").get<std::string>()).string());
        REQUIRE(opened.has_value());
        collecting_sink sink;

        auto extracted =
            opened.value().extract(test_case.at("target_path").get<std::string>(), sink);

        REQUIRE_FALSE(extracted.has_value());
        REQUIRE(extracted.error().code ==
                error_code_from_manifest(test_case.at("expected_error").get<std::string>()));
    }
}

TEST_CASE(
    "tes4_bsa_extract_bytes returns bounded consumer-visible bytes "
    "through extraction path",
    "[unit][fixture][tes4_bsa_embedded_name][tes4_bsa_entry_metadata]["
    "tes4_bsa_v104_extract][tes4_bsa_v105_extract]") {
    for (const auto& fixture : {std::string{"tes4_v104"}, std::string{"tes4_v105"}}) {
        const auto manifest = read_json_file(generated_archive_path(fixture + "_manifest.json"));
        auto opened =
            libbsa::archive_reader::open(generated_archive_path(fixture + ".bsa").string());
        REQUIRE(opened.has_value());

        for (const auto& expected : manifest.at("entries")) {
            auto found = opened.value().find(expected.at("path").get<std::string>());
            REQUIRE(found.has_value());
            REQUIRE(found.value().has_value());
            REQUIRE(found.value()->has_embedded_name);
            REQUIRE(found.value()->embedded_name_prefix_size ==
                    expected.at("embedded_name_prefix_size").get<std::uint32_t>());

            auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());

            REQUIRE(bytes.has_value());
            REQUIRE(bytes.value() ==
                    bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
        }
    }
}

TEST_CASE(
    "tes4_bsa_extract_bytes matches extract path errors for invalid and "
    "missing paths",
    "[unit][fixture][tes4_bsa_lookup]") {
    auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v103.bsa").string());
    REQUIRE(opened.has_value());

    auto invalid = opened.value().extract_bytes("folder//file.txt");
    REQUIRE_FALSE(invalid.has_value());
    REQUIRE(invalid.error().code == libbsa::error_code::invalid_argument);

    auto missing = opened.value().extract_bytes("valid/missing/path.txt");
    REQUIRE_FALSE(missing.has_value());
    REQUIRE(missing.error().code == libbsa::error_code::not_found);
}

TEST_CASE(
    "archive_reader_extract_bytes preflights materialization before "
    "payload extraction",
    "[unit][bounded_memory_policy][allocation]") {
    const auto text =
        read_text_file(std::filesystem::path{LIBBSA_SOURCE_DIR} / "src" / "archive.cpp");
    const auto function_pos =
        text.find("result<std::vector<std::byte>> archive_reader::extract_bytes");
    REQUIRE(function_pos != std::string::npos);

    const auto preflight_pos = text.find("checked_materialized_payload_size", function_pos);
    const auto extraction_pos = text.find("extract_entry_payload", function_pos);

    REQUIRE(preflight_pos != std::string::npos);
    REQUIRE(extraction_pos != std::string::npos);
    REQUIRE(preflight_pos < extraction_pos);
}
