#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "formats/ba2/ba2_archive_header.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
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

/// BA2 header facts read straight off disk, without going through libbsa.
///
/// The first twelve bytes of a BA2 are version-independent, so probing them
/// gives the tests an oracle that cannot be wrong in the same way the parser
/// under test might be.
struct ba2_header_probe {
    std::uint32_t version{0U};
    std::uint32_t subtype_magic{0U};
};

// Restated rather than pulled from ba2_constants.hpp on purpose: the probe is
// meant to be an oracle independent of the code it checks.
constexpr std::uint32_t btdx_magic = 0x5844'5442U;
constexpr std::uint32_t gnrl_magic = 0x4C52'4E47U;
constexpr std::uint32_t dx10_magic = 0x3031'5844U;

/// Widest BA2 fixed header across every supported version (Starfield v3).
///
/// Reading this many leading bytes is enough to decode any supported header.
constexpr std::size_t widest_ba2_fixed_header_size = 36U;

// Restated rather than pulled from ba2_constants.hpp for the same reason as the
// magics above: a corpus oracle must not agree with the code it checks by
// construction. NameHash u32, Ext FourCC, DirHash u32, Unknown u32, Offset u64,
// PackedSize u32, Size u32, BAADF00D u32.
constexpr std::size_t retail_gnrl_record_size = 36U;
constexpr std::size_t retail_gnrl_extension_offset = 4U;

/// Reads BA2 magic, version, and subtype from `path`.
///
/// Returns `std::nullopt` when the file is unreadable, shorter than the twelve
/// probed bytes, or does not start with BTDX, so a corpus holding BSA archives
/// alongside BA2 archives can be walked in one pass.
std::optional<ba2_header_probe> probe_ba2_header(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return std::nullopt;
    }
    std::array<unsigned char, 12U> bytes{};
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (stream.gcount() != static_cast<std::streamsize>(bytes.size())) {
        return std::nullopt;
    }

    const auto read_u32 = [&bytes](std::size_t offset) {
        return static_cast<std::uint32_t>(bytes[offset]) |
               (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
               (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
    };
    if (read_u32(0U) != btdx_magic) {
        return std::nullopt;
    }
    return ba2_header_probe{read_u32(4U), read_u32(8U)};
}

/// Reads the header version of a TES4-family BSA at `path`.
///
/// Returns `std::nullopt` for anything that is not a `BSA\0` archive, so a mixed
/// corpus of BSA, BA2, and TES3 archives can be walked in one pass. Like
/// `probe_ba2_header`, this decodes the bytes directly rather than going through
/// the parser under test.
std::optional<std::uint32_t> probe_tes4_bsa_version(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return std::nullopt;
    }
    std::array<unsigned char, 8U> bytes{};
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (stream.gcount() != static_cast<std::streamsize>(bytes.size())) {
        return std::nullopt;
    }
    if (bytes[0] != 'B' || bytes[1] != 'S' || bytes[2] != 'A' || bytes[3] != 0U) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(bytes[4]) | (static_cast<std::uint32_t>(bytes[5]) << 8U) |
           (static_cast<std::uint32_t>(bytes[6]) << 16U) |
           (static_cast<std::uint32_t>(bytes[7]) << 24U);
}

/// Reads up to `count` leading bytes of `path` for direct header decoding.
///
/// Returns `std::nullopt` when the file cannot be opened, and a short buffer
/// when the file itself is shorter than `count`.
std::optional<std::vector<std::byte>> read_leading_bytes(const std::filesystem::path& path,
                                                         std::size_t count) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return std::nullopt;
    }
    std::vector<std::byte> bytes(count);
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(count));
    bytes.resize(static_cast<std::size_t>(stream.gcount()));
    return bytes;
}

/// Restates the BA2 header version to game-family mapping independently of the
/// production table, so a regression there cannot silently satisfy the test.
///
/// Fallout 4's next-gen versions 7 and 8 are Fallout 4 despite sorting above
/// Starfield's 2 and 3.
std::optional<libbsa::archive_variant> expected_variant_for_ba2_version(std::uint32_t version) {
    switch (version) {
        case 1U:
        case 7U:
        case 8U:
            return libbsa::archive_variant::fallout4;
        case 2U:
        case 3U:
            return libbsa::archive_variant::starfield;
        default:
            return std::nullopt;
    }
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

TEST_CASE("retail BA2 headers resolve at every shipped version",
          "[requires-game-fixture][unit][ba2][compat]") {
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    std::size_t probed_archives = 0U;
    std::error_code iteration_error;
    std::filesystem::directory_iterator iterator{*fixture_root, iteration_error};
    REQUIRE_FALSE(iteration_error);

    for (const auto& directory_entry : iterator) {
        if (!directory_entry.is_regular_file()) {
            continue;
        }
        auto probe = probe_ba2_header(directory_entry.path());
        if (!probe.has_value()) {
            continue;
        }
        ++probed_archives;

        const auto name = directory_entry.path().filename().string();
        INFO("archive=" << name << " version=" << probe->version
                        << " subtype=" << probe->subtype_magic);

        const auto expected_variant = expected_variant_for_ba2_version(probe->version);
        REQUIRE(expected_variant.has_value());
        CHECK((probe->subtype_magic == gnrl_magic || probe->subtype_magic == dx10_magic));

        // The fixed header is what this case owns. Decoding it directly keeps
        // the assertion sharp while later parsing stages carry their own known
        // defects against retail archives.
        auto header_bytes =
            read_leading_bytes(directory_entry.path(), widest_ba2_fixed_header_size);
        REQUIRE(header_bytes.has_value());
        std::error_code size_error;
        const auto archive_size = std::filesystem::file_size(directory_entry.path(), size_error);
        REQUIRE_FALSE(size_error);

        auto header = libbsa::formats::ba2::decode_ba2_archive_header(*header_bytes, archive_size);

        REQUIRE(header.has_value());
        const auto metadata = header.value().materialize_metadata();
        CHECK(metadata.type == libbsa::archive_type::ba2);
        CHECK(metadata.version == probe->version);
        CHECK(metadata.variant == *expected_variant);
        CHECK(metadata.file_count > 0U);
        CHECK(header.value().filename_table_offset() >= header.value().profile().header_size());
    }

    if (probed_archives == 0U) {
        SKIP("The local corpus holds no BA2 archives to probe.");
    }
}

TEST_CASE("every retail BA2 GNRL record stores a lowercase extension FourCC",
          "[requires-game-fixture][unit][ba2][gnrl][compat]") {
    // libbsa lowercases the Ext FourCC on write because the reference does
    // (wbBSArchive.pas:1543) and FindFileRecordFO4 matches the stored TMagic4
    // exactly against String2Magic(LowerCase(ext)) -- an uppercase byte makes
    // the record unreachable in game (issue #44). libbsa's own read path
    // compares case-insensitively, so no round-trip test can observe a
    // regression here; only retail bytes can, which is why this claim lives
    // against the corpus rather than in a comment.
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    std::size_t scanned_archives = 0U;
    std::size_t scanned_records = 0U;
    std::size_t uppercase_records = 0U;
    std::error_code iteration_error;
    std::filesystem::directory_iterator iterator{*fixture_root, iteration_error};
    REQUIRE_FALSE(iteration_error);

    for (const auto& directory_entry : iterator) {
        if (!directory_entry.is_regular_file()) {
            continue;
        }
        auto probe = probe_ba2_header(directory_entry.path());
        if (!probe.has_value() || probe->subtype_magic != gnrl_magic) {
            continue;
        }

        auto header_bytes =
            read_leading_bytes(directory_entry.path(), widest_ba2_fixed_header_size);
        REQUIRE(header_bytes.has_value());
        std::error_code size_error;
        const auto archive_size = std::filesystem::file_size(directory_entry.path(), size_error);
        REQUIRE_FALSE(size_error);
        auto header = libbsa::formats::ba2::decode_ba2_archive_header(*header_bytes, archive_size);
        REQUIRE(header.has_value());
        ++scanned_archives;

        const auto name = directory_entry.path().filename().string();
        const auto file_count = header.value().materialize_metadata().file_count;

        std::ifstream stream{directory_entry.path(), std::ios::binary};
        REQUIRE(stream.is_open());
        stream.seekg(static_cast<std::streamoff>(header.value().profile().header_size()));
        REQUIRE(stream.good());

        // Records are walked one at a time rather than slurped: the largest
        // retail GNRL tables run to hundreds of thousands of records, and the
        // corpus is read-only reference data, not something to buffer whole.
        std::array<char, retail_gnrl_record_size> record{};
        for (std::uint32_t index = 0; index < file_count; ++index) {
            stream.read(record.data(), static_cast<std::streamsize>(record.size()));
            REQUIRE(stream.gcount() == static_cast<std::streamsize>(record.size()));
            ++scanned_records;

            for (std::size_t byte_index = 0; byte_index < 4U; ++byte_index) {
                const auto value =
                    static_cast<unsigned char>(record[retail_gnrl_extension_offset + byte_index]);
                if (value >= 'A' && value <= 'Z') {
                    // Reported by position, never by content: retail archive
                    // bytes must not leak into test output.
                    UNSCOPED_INFO("uppercase Ext byte in " << name << " record " << index
                                                           << " byte " << byte_index);
                    ++uppercase_records;
                    break;
                }
            }
        }
    }

    if (scanned_archives == 0U) {
        SKIP("The local corpus holds no BA2 GNRL archives to scan.");
    }

    INFO("scanned " << scanned_records << " GNRL records across " << scanned_archives
                    << " archives");
    CHECK(scanned_records > 0U);
    CHECK(uppercase_records == 0U);
}

TEST_CASE("a retail BA2 GNRL archive opens, lists, and extracts end to end",
          "[requires-game-fixture][unit][ba2][gnrl][compat]") {
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    // Pick the smallest GNRL archive in the corpus. Retail GNRL archives run to
    // tens of thousands of records, and this case only needs one archive that
    // libbsa did not write; walking the largest would dominate the suite.
    std::optional<std::filesystem::path> smallest;
    std::uintmax_t smallest_size = 0U;
    std::error_code iteration_error;
    std::filesystem::directory_iterator iterator{*fixture_root, iteration_error};
    REQUIRE_FALSE(iteration_error);

    for (const auto& directory_entry : iterator) {
        if (!directory_entry.is_regular_file()) {
            continue;
        }
        auto probe = probe_ba2_header(directory_entry.path());
        if (!probe.has_value() || probe->subtype_magic != gnrl_magic) {
            continue;
        }
        std::error_code size_error;
        const auto size = std::filesystem::file_size(directory_entry.path(), size_error);
        if (size_error) {
            continue;
        }
        if (!smallest.has_value() || size < smallest_size) {
            smallest = directory_entry.path();
            smallest_size = size;
        }
    }

    if (!smallest.has_value()) {
        SKIP("The local corpus holds no BA2 GNRL archives.");
    }

    INFO("archive=" << smallest->filename().string());
    auto opened = libbsa::archive_reader::open(smallest->string());
    REQUIRE(opened.has_value());

    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::ba2);
    CHECK(metadata.value().file_count > 0U);

    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == metadata.value().file_count);

    // Every listed path must resolve through the reader's own hash-backed
    // lookup. That is the property the GNRL NameHash basis governs: if the
    // stored hash and the recomputed hash disagree, the archive does not open
    // at all, and if lookup used a different basis than storage, find() misses.
    const auto& first = entries.value().front();
    auto found = opened.value().find(first.path);
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->path == first.path);

    // Raw entries stream straight out of the archive, so prove the sink path on
    // one when the archive has one.
    const auto raw_entry = std::find_if(
        entries.value().begin(), entries.value().end(), [](const libbsa::entry_metadata& entry) {
            return entry.compression == libbsa::entry_compression::none && entry.raw_size > 0U;
        });
    if (raw_entry != entries.value().end()) {
        fnv1a32_sink sink;
        auto extracted = opened.value().extract(raw_entry->path, sink);
        REQUIRE(extracted.has_value());
        CHECK(sink.size() == raw_entry->raw_size);
    }

    // Compressed entries are the claim issue #42 turned on: every retail BA2
    // stores them zlib-wrapped (RFC1950), and libbsa decoded raw deflate
    // (RFC1951), so no compressed retail entry could be extracted at all.
    // Assert against every compressed entry rather than a sample, since a
    // partial fix would still leave most of an archive unreadable.
    std::size_t compressed_entries = 0U;
    for (const auto& entry : entries.value()) {
        if (entry.compression == libbsa::entry_compression::none) {
            continue;
        }
        ++compressed_entries;
        INFO("entry=" << entry.path);
        fnv1a32_sink sink;
        auto extracted = opened.value().extract(entry.path, sink);
        REQUIRE(extracted.has_value());
        CHECK(sink.size() == entry.raw_size);
    }
    INFO("compressed_entries=" << compressed_entries);
}

TEST_CASE("every retail TES4-family BSA opens and lists its full entry count",
          "[requires-game-fixture][unit][bsa][tes4][compat]") {
    // Every committed TES4 fixture is written by libbsa's own writer, so the
    // default suite can only prove libbsa agrees with itself about physical
    // layout. Issue #45 was a class of defect only retail bytes could expose:
    // libbsa enforced invariants over derived header metadata -- the folder
    // record `Offset` bias by `TotalFileNameLength`, the folder-name total, and
    // exact `TotalFileNameLength` consumption -- that Bethesda's packer does not
    // honour, and every one of the 29 archives here was rejected outright.
    //
    // Unlike the BA2 header case above, this opens each archive fully rather
    // than probing bytes: the defects were in table parsing, not in the fixed
    // header, so nothing short of a real open would have caught them. A full
    // open-and-list sweep of the corpus runs in well under a minute.
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    std::size_t opened_archives = 0U;
    std::error_code iteration_error;
    std::filesystem::directory_iterator iterator{*fixture_root, iteration_error};
    REQUIRE_FALSE(iteration_error);

    for (const auto& directory_entry : iterator) {
        if (!directory_entry.is_regular_file()) {
            continue;
        }
        auto version = probe_tes4_bsa_version(directory_entry.path());
        if (!version.has_value()) {
            continue;
        }

        const auto name = directory_entry.path().filename().string();
        INFO("archive=" << name << " version=" << *version);

        // 103 is Oblivion, 104 is Fallout 3 / New Vegas / Skyrim LE, and 105 is
        // Skyrim SE/AE. Anything else in a vanilla corpus is a detection gap.
        REQUIRE((*version == 103U || *version == 104U || *version == 105U));

        auto opened = libbsa::archive_reader::open(directory_entry.path().string());
        REQUIRE(opened.has_value());
        ++opened_archives;

        auto metadata = opened.value().metadata();
        REQUIRE(metadata.has_value());
        CHECK(metadata.value().type == libbsa::archive_type::bsa);
        CHECK(metadata.value().variant == libbsa::archive_variant::tes4);
        CHECK(metadata.value().version == *version);
        CHECK(metadata.value().file_count > 0U);

        // Listing the declared count is the claim: a folder table walked with a
        // biased offset, or one cut short by a name-total mismatch, cannot
        // produce every entry the header promises.
        auto entries = opened.value().entries();
        REQUIRE(entries.has_value());
        REQUIRE(entries.value().size() == metadata.value().file_count);

        // Hash-backed lookup has to reach a listed path, which pins the folder
        // and file hash bases against archives libbsa did not write.
        const auto& first = entries.value().front();
        auto found = opened.value().find(first.path);
        REQUIRE(found.has_value());
        REQUIRE(found.value().has_value());
        CHECK(found.value()->path == first.path);
    }

    if (opened_archives == 0U) {
        SKIP("The local corpus holds no TES4-family BSA archives.");
    }
    INFO("opened " << opened_archives << " retail TES4-family BSA archives");
}

TEST_CASE("retail TES4-family BSA file-name table slack is a warning, not a rejection",
          "[requires-game-fixture][unit][bsa][tes4][compat]") {
    // At least one vanilla archive -- `Fallout - Voices1.bsa` -- declares a
    // `TotalFileNameLength` longer than its names consume, and libbsa used to
    // reject it outright (issue #45). The reference reads exactly `FileCount`
    // names and never inspects the surplus (`wbBSArchive.pas:1235-1237`).
    //
    // The corpus a given machine holds is not fixed, so this asserts the
    // implication rather than a specific archive: whenever the flag is set, the
    // archive must still validate, and whenever it is clear, no such warning may
    // appear. `compatibility_warning_tests.cpp` owns the positive case against a
    // reproducible writer-output archive.
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    std::size_t checked_archives = 0U;
    std::size_t archives_with_slack = 0U;
    std::error_code iteration_error;
    std::filesystem::directory_iterator iterator{*fixture_root, iteration_error};
    REQUIRE_FALSE(iteration_error);

    for (const auto& directory_entry : iterator) {
        if (!directory_entry.is_regular_file()) {
            continue;
        }
        if (!probe_tes4_bsa_version(directory_entry.path()).has_value()) {
            continue;
        }

        const auto name = directory_entry.path().filename().string();
        INFO("archive=" << name);

        auto validated = libbsa::validate_archive(directory_entry.path().string());
        REQUIRE(validated.has_value());
        CHECK(validated.value().is_valid());
        ++checked_archives;

        REQUIRE(validated.value().metadata.has_value());
        const bool has_slack = validated.value().metadata.value().file_name_table_has_trailing_bytes;
        const auto warned = std::any_of(
            validated.value().warnings.begin(), validated.value().warnings.end(),
            [](const libbsa::compatibility_warning& warning) {
                return warning.code ==
                       libbsa::compatibility_warning_code::bsa_file_name_table_trailing_bytes;
            });
        CHECK(warned == has_slack);
        if (has_slack) {
            ++archives_with_slack;
        }
    }

    if (checked_archives == 0U) {
        SKIP("The local corpus holds no TES4-family BSA archives.");
    }
    INFO("checked " << checked_archives << " archives, " << archives_with_slack
                    << " with file-name table slack");
}

TEST_CASE("a retail TES4-family BSA extracts every zlib-compressed entry",
          "[requires-game-fixture][unit][bsa][tes4][compat]") {
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    // Oblivion/FO3/FNV/Skyrim LE archives (v103/v104) are the BSA half of the
    // zlib route. v105 is Skyrim SE and uses LZ4 frame, so it proves nothing
    // here. Pick the smallest matching archive: retail BSAs run to gigabytes and
    // this case extracts every compressed entry it finds.
    std::optional<std::filesystem::path> smallest;
    std::uintmax_t smallest_size = 0U;
    std::error_code iteration_error;
    std::filesystem::directory_iterator iterator{*fixture_root, iteration_error};
    REQUIRE_FALSE(iteration_error);

    for (const auto& directory_entry : iterator) {
        if (!directory_entry.is_regular_file()) {
            continue;
        }
        auto version = probe_tes4_bsa_version(directory_entry.path());
        if (!version.has_value() || (*version != 103U && *version != 104U)) {
            continue;
        }
        std::error_code size_error;
        const auto size = std::filesystem::file_size(directory_entry.path(), size_error);
        if (size_error) {
            continue;
        }
        if (!smallest.has_value() || size < smallest_size) {
            smallest = directory_entry.path();
            smallest_size = size;
        }
    }

    if (!smallest.has_value()) {
        SKIP("The local corpus holds no TES4-family BSA v103 or v104 archives.");
    }

    INFO("archive=" << smallest->filename().string());
    auto opened = libbsa::archive_reader::open(smallest->string());
    REQUIRE(opened.has_value());

    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());

    std::size_t compressed_entries = 0U;
    for (const auto& entry : entries.value()) {
        if (entry.compression == libbsa::entry_compression::none) {
            continue;
        }
        ++compressed_entries;
        INFO("entry=" << entry.path << " raw_size=" << entry.raw_size);
        fnv1a32_sink sink;
        auto extracted = opened.value().extract(entry.path, sink);
        REQUIRE(extracted.has_value());
        CHECK(sink.size() == entry.raw_size);
    }

    if (compressed_entries == 0U) {
        SKIP("The smallest TES4-family BSA in the corpus stores no compressed entries.");
    }
}

TEST_CASE("a retail BA2 DX10 archive opens and stores its filename table last",
          "[requires-game-fixture][unit][ba2][dx10][compat]") {
    auto fixture_root = local_fixture_root();
    if (!fixture_root.has_value()) {
        SKIP(
            "Set LIBBSA_GAME_FIXTURES or place local game archives under "
            "tests/fixtures/local; these files are not committed.");
    }

    // Smallest DX10 archive in the corpus, for the same reason the GNRL case
    // picks the smallest: retail texture archives run to gigabytes and this case
    // only needs one archive libbsa did not write.
    std::optional<std::filesystem::path> smallest;
    std::uintmax_t smallest_size = 0U;
    std::error_code iteration_error;
    std::filesystem::directory_iterator iterator{*fixture_root, iteration_error};
    REQUIRE_FALSE(iteration_error);

    for (const auto& directory_entry : iterator) {
        if (!directory_entry.is_regular_file()) {
            continue;
        }
        auto probe = probe_ba2_header(directory_entry.path());
        if (!probe.has_value() || probe->subtype_magic != dx10_magic) {
            continue;
        }
        std::error_code size_error;
        const auto size = std::filesystem::file_size(directory_entry.path(), size_error);
        if (size_error) {
            continue;
        }
        if (!smallest.has_value() || size < smallest_size) {
            smallest = directory_entry.path();
            smallest_size = size;
        }
    }

    if (!smallest.has_value()) {
        SKIP("The local corpus holds no BA2 DX10 archives.");
    }

    INFO("archive=" << smallest->filename().string());

    // Opening at all is the first half of the claim: libbsa used to derive the
    // DX10 record table width from FileTableOffset - HeaderSize, which spans the
    // whole payload area on a Bethesda-written archive and was rejected outright.
    auto opened = libbsa::archive_reader::open(smallest->string());
    REQUIRE(opened.has_value());

    auto metadata = opened.value().metadata();
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().type == libbsa::archive_type::ba2);
    CHECK(metadata.value().file_count > 0U);

    auto entries = opened.value().entries();
    REQUIRE(entries.has_value());
    REQUIRE(entries.value().size() == metadata.value().file_count);

    // The second half pins the ordering claim to an observed archive rather than
    // to a reading of TwbBSArchive.Save: decode FileTableOffset straight from the
    // header and compare it against the lowest chunk payload offset the archive
    // actually declares. Reference order is header -> records -> payloads ->
    // names, so the filename table must sit past every payload.
    auto header_bytes = read_leading_bytes(*smallest, widest_ba2_fixed_header_size);
    REQUIRE(header_bytes.has_value());
    auto header = libbsa::formats::ba2::decode_ba2_archive_header(
        *header_bytes, static_cast<std::uint64_t>(smallest_size));
    REQUIRE(header.has_value());

    std::uint64_t lowest_payload_offset = std::numeric_limits<std::uint64_t>::max();
    std::size_t chunk_count = 0U;
    for (const auto& entry : entries.value()) {
        REQUIRE(entry.texture.has_value());
        for (const auto& chunk : entry.texture->chunks) {
            lowest_payload_offset = std::min(lowest_payload_offset, chunk.payload_offset);
            ++chunk_count;
        }
    }
    REQUIRE(chunk_count > 0U);

    INFO("file_table_offset=" << header.value().filename_table_offset()
                              << " lowest_payload_offset=" << lowest_payload_offset);
    CHECK(header.value().filename_table_offset() > lowest_payload_offset);

    // Retail DX10 chunks are zlib-wrapped just like GNRL payloads, so issue #42
    // blocked texture extraction too. Reassembling a DDS exercises the chunk
    // decode path end to end. Unlike the GNRL case this samples rather than
    // sweeps: the smallest retail texture archive still holds thousands of
    // textures and megabytes of surface per entry.
    constexpr std::size_t sampled_textures = 8U;
    std::size_t sampled = 0U;
    for (const auto& entry : entries.value()) {
        const auto has_compressed_chunk =
            std::any_of(entry.texture->chunks.begin(), entry.texture->chunks.end(),
                        [](const libbsa::texture_chunk_metadata& chunk) {
                            return chunk.compression != libbsa::entry_compression::none;
                        });
        if (!has_compressed_chunk) {
            continue;
        }
        INFO("entry=" << entry.path);
        fnv1a32_sink sink;
        auto extracted = opened.value().extract(entry.path, sink);
        REQUIRE(extracted.has_value());
        CHECK(sink.size() == entry.raw_size);
        if (++sampled == sampled_textures) {
            break;
        }
    }
    INFO("sampled_compressed_textures=" << sampled);
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
