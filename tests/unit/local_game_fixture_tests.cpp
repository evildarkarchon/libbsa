#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "test_source_root.hpp"

namespace {

/// Resolves the opt-in local game archive corpus root.
///
/// `LIBBSA_GAME_FIXTURES` takes precedence when set. Otherwise the uncommitted
/// `tests/fixtures/local` directory is used when it actually holds archives,
/// which is the fallback the skip messages in this file have always advertised
/// but which was never implemented. Returns `std::nullopt` when neither source
/// is available, so `[requires-game-fixture]` cases skip rather than fail.
std::optional<std::filesystem::path> local_fixture_root() {
    const char* fixture_root = std::getenv("LIBBSA_GAME_FIXTURES");
    if (fixture_root != nullptr && !std::string_view{fixture_root}.empty()) {
        return std::filesystem::path{fixture_root};
    }

    // source_root() throws when the repository layout cannot be located. An
    // opt-in corpus that is simply absent must stay a skip, never an error.
    std::filesystem::path candidate;
    try {
        candidate = LIBBSA_SOURCE_DIR / "tests" / "fixtures" / "local";
    } catch (const std::runtime_error&) {
        return std::nullopt;
    }

    std::error_code error;
    if (!std::filesystem::is_directory(candidate, error) || error) {
        return std::nullopt;
    }

    // The committed .gitkeep is not a corpus. Require at least one other entry
    // so a clean checkout still skips instead of reporting an empty corpus.
    std::filesystem::directory_iterator iterator{candidate, error};
    if (error) {
        return std::nullopt;
    }
    for (const auto& entry : iterator) {
        if (entry.path().filename() != ".gitkeep") {
            return candidate;
        }
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> bsarchpro_expected_manifest_path() {
    const char* manifest_path = std::getenv("LIBBSA_BSARCHPRO_EXPECTED");
    if (manifest_path != nullptr && !std::string_view{manifest_path}.empty()) {
        return std::filesystem::path{manifest_path};
    }

    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        return std::nullopt;
    }
    return *fixture_root / "bsarchpro_expected.json";
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());
    return nlohmann::json::parse(stream);
}

std::filesystem::path archive_path_from_manifest(const std::filesystem::path& manifest_path,
                                                 const nlohmann::json& test_case) {
    auto archive_path = std::filesystem::path{test_case.at("archive").get<std::string>()};
    if (archive_path.is_absolute()) {
        return archive_path;
    }
    return manifest_path.parent_path() / archive_path;
}

libbsa::archive_type archive_type_from_string(std::string_view value) {
    if (value == "bsa") {
        return libbsa::archive_type::bsa;
    }
    if (value == "ba2") {
        return libbsa::archive_type::ba2;
    }
    FAIL("unknown BSArchPro-derived archive type: " << value);
    return libbsa::archive_type::bsa;
}

libbsa::archive_variant archive_variant_from_string(std::string_view value) {
    if (value == "tes3") {
        return libbsa::archive_variant::tes3;
    }
    if (value == "tes4") {
        return libbsa::archive_variant::tes4;
    }
    if (value == "fallout4") {
        return libbsa::archive_variant::fallout4;
    }
    if (value == "starfield") {
        return libbsa::archive_variant::starfield;
    }
    FAIL("unknown BSArchPro-derived archive variant: " << value);
    return libbsa::archive_variant::tes4;
}

libbsa::entry_compression entry_compression_from_string(std::string_view value) {
    if (value == "none" || value == "raw") {
        return libbsa::entry_compression::none;
    }
    if (value == "deflate") {
        return libbsa::entry_compression::deflate;
    }
    if (value == "lz4_frame") {
        return libbsa::entry_compression::lz4_frame;
    }
    if (value == "lz4_block") {
        return libbsa::entry_compression::lz4_block;
    }
    FAIL("unknown BSArchPro-derived entry compression: " << value);
    return libbsa::entry_compression::none;
}

std::uint32_t u32_from_json(const nlohmann::json& value) {
    if (value.is_number_unsigned()) {
        return value.get<std::uint32_t>();
    }
    const auto text = value.get<std::string>();
    const int base = text.starts_with("0x") || text.starts_with("0X") ? 0 : 16;
    return static_cast<std::uint32_t>(std::stoul(text, nullptr, base));
}

std::uint64_t u64_from_json(const nlohmann::json& value) {
    if (value.is_number_unsigned()) {
        return value.get<std::uint64_t>();
    }
    const auto text = value.get<std::string>();
    const int base = text.starts_with("0x") || text.starts_with("0X") ? 0 : 10;
    return std::stoull(text, nullptr, base);
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

class fnv1a32_sink final : public libbsa::payload_sink {
   public:
    /// Hashes bytes as they stream out of libbsa without storing local corpus
    /// payloads.
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        for (const auto byte : bytes) {
            hash_ ^= std::to_integer<std::uint32_t>(byte);
            hash_ *= 16777619U;
        }
        size_ += bytes.size();
        return bytes.size();
    }

    [[nodiscard]] std::uint32_t hash() const noexcept { return hash_; }

    [[nodiscard]] std::uint64_t size() const noexcept { return size_; }

   private:
    std::uint32_t hash_{2166136261U};
    std::uint64_t size_{0U};
};

void require_metadata_matches(const nlohmann::json& expected,
                              const libbsa::archive_metadata& metadata) {
    if (expected.contains("type")) {
        CHECK(metadata.type == archive_type_from_string(expected.at("type").get<std::string>()));
    }
    if (expected.contains("variant")) {
        CHECK(metadata.variant ==
              archive_variant_from_string(expected.at("variant").get<std::string>()));
    }
    if (expected.contains("version")) {
        CHECK(metadata.version == u32_from_json(expected.at("version")));
    }
    if (expected.contains("file_count")) {
        CHECK(metadata.file_count == u32_from_json(expected.at("file_count")));
    }
}

void require_entry_metadata_matches(const nlohmann::json& expected,
                                    const libbsa::entry_metadata& entry) {
    if (expected.contains("raw_size")) {
        CHECK(entry.raw_size == u64_from_json(expected.at("raw_size")));
    }
    if (expected.contains("stored_size")) {
        CHECK(entry.stored_size == u64_from_json(expected.at("stored_size")));
    }
    if (expected.contains("compression")) {
        CHECK(entry.compression ==
              entry_compression_from_string(expected.at("compression").get<std::string>()));
    }
    if (expected.contains("has_embedded_name")) {
        CHECK(entry.has_embedded_name == expected.at("has_embedded_name").get<bool>());
    }
    if (expected.contains("embedded_name_prefix_size")) {
        CHECK(entry.embedded_name_prefix_size ==
              u32_from_json(expected.at("embedded_name_prefix_size")));
    }
}

void require_entry_bytes_match(const libbsa::archive_reader& reader, const std::string& path,
                               const nlohmann::json& expected) {
    if (!expected.contains("expected")) {
        return;
    }

    const auto& payload = expected.at("expected");
    if (payload.contains("bytes_hex")) {
        auto extracted = reader.extract_bytes(path);
        REQUIRE(extracted.has_value());
        CHECK(extracted.value() == bytes_from_hex(payload.at("bytes_hex").get<std::string>()));
    }
    if (payload.contains("fnv1a32")) {
        fnv1a32_sink sink;
        auto extracted = reader.extract(path, sink);
        REQUIRE(extracted.has_value());
        CHECK(sink.hash() == u32_from_json(payload.at("fnv1a32")));
        if (payload.contains("size")) {
            CHECK(sink.size() == u64_from_json(payload.at("size")));
        }
    }
}

}  // namespace

TEST_CASE("local game fixtures are opt-in", "[requires-game-fixture][unit]") {
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    REQUIRE_FALSE(fixture_root->empty());
}

TEST_CASE("BSArchPro-derived expected fixture comparisons are opt-in",
          "[requires-game-fixture][unit][compat]") {
    auto manifest_path = bsarchpro_expected_manifest_path();
    if (!manifest_path.has_value() || !std::filesystem::is_regular_file(*manifest_path)) {
        SKIP(
            "Set LIBBSA_BSARCHPRO_EXPECTED to a BSArchPro-derived manifest, or "
            "place bsarchpro_expected.json under LIBBSA_GAME_FIXTURES.");
    }

    const auto manifest = read_json_file(*manifest_path);
    REQUIRE(manifest.contains("provenance"));
    REQUIRE(manifest.contains("cases"));
    REQUIRE_FALSE(manifest.at("cases").empty());

    for (const auto& test_case : manifest.at("cases")) {
        INFO("BSArchPro-derived compare case: " << test_case.at("archive").get<std::string>());
        auto opened = libbsa::archive_reader::open(
            archive_path_from_manifest(*manifest_path, test_case).string());
        REQUIRE(opened.has_value());

        auto metadata = opened.value().metadata();
        REQUIRE(metadata.has_value());
        require_metadata_matches(test_case, metadata.value());

        REQUIRE(test_case.contains("entries"));
        REQUIRE_FALSE(test_case.at("entries").empty());
        for (const auto& expected_entry : test_case.at("entries")) {
            const auto path = expected_entry.at("path").get<std::string>();
            INFO("BSArchPro-derived compare entry: " << path);

            auto found = opened.value().find(path);
            REQUIRE(found.has_value());
            REQUIRE(found.value().has_value());
            require_entry_metadata_matches(expected_entry, *found.value());
            require_entry_bytes_match(opened.value(), path, expected_entry);
        }
    }
}
